// Crystal Expansion headless play-test harness.
//
// Boots the ROM in libmgba with no window, runs a script of waits, button
// presses, screenshots and memory reads, and prints every read to stdout so a
// test can assert on it. The test suite runs code; this runs the game.
//
// Build with tools/playtest/build.sh.

#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/core/serialize.h>
#include <mgba-util/vfs.h>
#include <mgba/internal/arm/arm.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 240
#define SCREEN_H 160

static struct mCore* core;
static color_t frameBuffer[SCREEN_W * SCREEN_H];
static uint32_t heldKeys;
static unsigned long frameCount;

// mGBA logs every stray I/O write; only the game's own mgba_printf output is
// worth seeing, so everything else is dropped unless PLAYTEST_VERBOSE is set.
static bool sVerboseLog;

// The core, so a crash report can name the code that jumped into the weeds.
static struct mCore* sCore;
static int sCrashReports;
static unsigned long sCrashFrom = 100;   // PLAYTEST_CRASH_FROM: skip early noise

static void QuietLog(struct mLogger* logger, int category, enum mLogLevel level, const char* format, va_list args)
{
	UNUSED(logger);
	UNUSED(level);
	bool isGame = !strcmp(mLogCategoryId(category), "gba.debug");
	// mGBA only complains about a fetch from nowhere; the interesting part is
	// which function did it, so print the ARM registers the first few times.
	if (!isGame && sCore && sCrashReports < 12 && frameCount > sCrashFrom &&
	    (!strncmp(format, "Bad ", 4) || !strncmp(format, "Jump", 4))) {
		struct ARMCore* cpu = sCore->cpu;
		printf("crash: ");
		vprintf(format, args);
		printf("\n  pc=%08X lr=%08X sp=%08X r0=%08X r1=%08X frame=%lu\n",
			cpu->gprs[15], cpu->gprs[14], cpu->gprs[13], cpu->gprs[0], cpu->gprs[1], frameCount);
		for (int i = 0; i < 24; ++i) {
			uint32_t w = sCore->busRead32(sCore, (cpu->gprs[13] & ~3) + i * 4);
			if (w >= 0x08000000 && w < 0x0A000000) {
				printf("  stack[%d] = %08X\n", i, w);
			}
		}
		++sCrashReports;
		fflush(stdout);
	}
	if (!isGame && !sVerboseLog) {
		return;
	}
	fputs(isGame ? "game: " : "mgba: ", stdout);
	vprintf(format, args);
	fputc('\n', stdout);
	fflush(stdout);
}

static struct mLogger sLogger = { .log = QuietLog };

static const struct { const char* name; uint32_t bit; } sKeys[] = {
	{ "A", 1 << 0 }, { "B", 1 << 1 }, { "SELECT", 1 << 2 }, { "START", 1 << 3 },
	{ "RIGHT", 1 << 4 }, { "LEFT", 1 << 5 }, { "UP", 1 << 6 }, { "DOWN", 1 << 7 },
	{ "R", 1 << 8 }, { "L", 1 << 9 },
};

static uint32_t ParseKeys(const char* spec)
{
	uint32_t mask = 0;
	char buf[128];
	strncpy(buf, spec, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = 0;
	for (char* tok = strtok(buf, ",+"); tok; tok = strtok(NULL, ",+")) {
		for (char* c = tok; *c; c++) {
			*c = toupper((unsigned char)*c);
		}
		bool found = false;
		for (size_t i = 0; i < sizeof(sKeys) / sizeof(sKeys[0]); i++) {
			if (!strcmp(tok, sKeys[i].name)) {
				mask |= sKeys[i].bit;
				found = true;
				break;
			}
		}
		if (!found && strcmp(tok, "NONE")) {
			fprintf(stderr, "playtest: unknown key '%s'\n", tok);
			exit(1);
		}
	}
	return mask;
}

// A key tapped on and off for as long as the emulator runs. Progression
// scripts spend most of their time answering dialogue, and a hundred literal
// `press A` lines is both unreadable and a guess at how long the text takes.
static uint32_t mashKeys;
// A second key tapped in the other half of the mash period, so it never shares
// a frame with the first. The opening needs this: A answers everything, but the
// two yes/no prompts in it default to NO and want an UP first, and a menu that
// sees A and UP arrive on one frame takes the A and keeps the NO.
static uint32_t mashAltKeys;
static unsigned long mashPeriod = 16;

static void RunFrames(unsigned long frames)
{
	for (unsigned long i = 0; i < frames; i++) {
		uint32_t keys = heldKeys;
		if (frameCount % mashPeriod < mashPeriod / 2) {
			keys |= mashKeys;
		} else {
			keys |= mashAltKeys;
		}
		core->setKeys(core, keys);
		core->runFrame(core);
		frameCount++;
	}
}

// Read a value of 1, 2 or 4 bytes, either at an address or at an offset from a
// pointer held at one. Everything the game keeps about the player's progress
// hangs off gSaveBlock1Ptr, so the pointer form is the common one.
static uint32_t ReadValue(bool viaPtr, uint32_t addr, uint32_t off, uint32_t size)
{
	uint32_t at = (viaPtr ? core->busRead32(core, addr) : addr) + off;
	switch (size) {
	case 1: return core->busRead8(core, at);
	case 2: return core->busRead16(core, at);
	default: return core->busRead32(core, at);
	}
}

static void WriteValue(bool viaPtr, uint32_t addr, uint32_t off, uint32_t size, uint32_t val)
{
	uint32_t at = (viaPtr ? core->busRead32(core, addr) : addr) + off;
	switch (size) {
	case 1: core->busWrite8(core, at, val); break;
	case 2: core->busWrite16(core, at, val); break;
	default: core->busWrite32(core, at, val); break;
	}
}

static bool Compare(uint32_t v, const char* op, uint32_t want)
{
	if (!strcmp(op, "eq")) return v == want;
	if (!strcmp(op, "ne")) return v != want;
	if (!strcmp(op, "lt")) return v < want;
	if (!strcmp(op, "le")) return v <= want;
	if (!strcmp(op, "gt")) return v > want;
	if (!strcmp(op, "ge")) return v >= want;
	fprintf(stderr, "playtest: unknown comparison '%s'\n", op);
	exit(1);
}

// PPM, not PNG: no image library to link, and the callers convert.
static void Screenshot(const char* path)
{
	FILE* f = fopen(path, "wb");
	if (!f) {
		fprintf(stderr, "playtest: cannot write %s\n", path);
		exit(1);
	}
	fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
	for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
		color_t c = frameBuffer[i];
		// mGBA's desktop build is 8888 with red in the low byte
		// (M_COLOR_RED is 0x000000FF in mgba/core/interface.h), so the
		// channels come out of the frame buffer in R, G, B order already.
		uint8_t rgb[3] = { c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF };
		fwrite(rgb, 1, 3, f);
	}
	fclose(f);
	printf("shot %s frame=%lu\n", path, frameCount);
}

static void Dump(uint32_t addr, uint32_t len, const char* label)
{
	printf("dump %s 0x%08X ", label, addr);
	for (uint32_t i = 0; i < len; i++) {
		printf("%02X", core->busRead8(core, addr + i));
	}
	printf("\n");
}

static void Usage(void)
{
	fprintf(stderr,
		"usage: playtest <rom.gba> <script>\n"
		"script commands (one per line, # comments):\n"
		"  wait <frames>\n"
		"  hold <KEYS|NONE>            keys stay down until changed\n"
		"  press <KEYS> [frames=8] [gap=8]\n"
		"  walk <DIR> <steps>          16 frames down, 4 up, per step\n"
		"  mash <KEYS|NONE> [period=16]  tap a key for as long as frames run\n"
		"  mashalt <KEYS|NONE>         tap a key in the other half of that period\n"
		"                              (set it after mash; mash clears it)\n"
		"  until abs|ptr <addr> <off> <size> <eq|ne|lt|le|gt|ge> <val> <max> [tag]\n"
		"  pread <ptraddr> <off> <size> [label]\n"
		"  pwrite <ptraddr> <off> <size> <value>\n"
		"  pbit <ptraddr> <byteoff> <bit> <read|set|clear> [label]\n"
		"  shot <file.ppm>\n"
		"  savestate|loadstate <file>\n"
		"  reset                       power-cycle, keeping battery save RAM\n"
		"  read8|read16|read32 <addr> [label]\n"
		"  write8|write16|write32 <addr> <value>\n"
		"  dump <addr> <len> [label]\n"
		"  deref <addr> <offset> <len> [label]   read32 a pointer, then dump\n"
		"  echo <text>\n");
	exit(1);
}

int main(int argc, char** argv)
{
	if (argc != 3) {
		Usage();
	}
	const char* romPath = argv[1];
	FILE* script = strcmp(argv[2], "-") ? fopen(argv[2], "r") : stdin;
	if (!script) {
		fprintf(stderr, "playtest: cannot read %s\n", argv[2]);
		return 1;
	}

	sVerboseLog = getenv("PLAYTEST_VERBOSE") != NULL;
	if (getenv("PLAYTEST_CRASH_FROM")) {
		sCrashFrom = strtoul(getenv("PLAYTEST_CRASH_FROM"), NULL, 0);
	}
	mLogSetDefaultLogger(&sLogger);
	core = mCoreFind(romPath);
	if (!core) {
		fprintf(stderr, "playtest: no core for %s\n", romPath);
		return 1;
	}
	core->init(core);
	sCore = core;
	if (!mCoreLoadFile(core, romPath)) {
		fprintf(stderr, "playtest: cannot load %s\n", romPath);
		return 1;
	}
	// A save file would make runs order-dependent; every run starts clean.
	mCoreConfigInit(&core->config, "playtest");
	core->setVideoBuffer(core, frameBuffer, SCREEN_W);
	core->reset(core);

	char line[512];
	while (fgets(line, sizeof(line), script)) {
		char* hash = strchr(line, '#');
		if (hash) {
			*hash = 0;
		}
		char cmd[64], a[192], b[64], c[64], d[64];
		int n = sscanf(line, "%63s %191s %63s %63s %63s", cmd, a, b, c, d);
		if (n < 1) {
			continue;
		}
		if (!strcmp(cmd, "wait")) {
			RunFrames(strtoul(a, NULL, 0));
		} else if (!strcmp(cmd, "hold")) {
			heldKeys = ParseKeys(a);
		} else if (!strcmp(cmd, "press")) {
			unsigned long down = n > 2 ? strtoul(b, NULL, 0) : 8;
			unsigned long gap = n > 3 ? strtoul(c, NULL, 0) : 8;
			uint32_t prev = heldKeys;
			heldKeys = prev | ParseKeys(a);
			RunFrames(down);
			heldKeys = prev;
			RunFrames(gap);
		} else if (!strcmp(cmd, "walk")) {
			uint32_t dir = ParseKeys(a);
			unsigned long steps = n > 2 ? strtoul(b, NULL, 0) : 1;
			uint32_t prev = heldKeys;
			for (unsigned long i = 0; i < steps; i++) {
				heldKeys = prev | dir;
				RunFrames(16);
				heldKeys = prev;
				RunFrames(4);
			}
		} else if (!strcmp(cmd, "savestate") || !strcmp(cmd, "loadstate")) {
			// A sweep can restore a known-good point instead of replaying the
			// whole opening after every map that traps it in a cutscene.
			bool saving = cmd[0] == 's';
			struct VFile* vf = VFileOpen(a, saving ? O_WRONLY | O_CREAT | O_TRUNC : O_RDONLY);
			if (!vf) {
				fprintf(stderr, "playtest: cannot open state %s\n", a);
				return 1;
			}
			bool ok = saving ? mCoreSaveStateNamed(core, vf, SAVESTATE_ALL)
				: mCoreLoadStateNamed(core, vf, SAVESTATE_ALL);
			vf->close(vf);
			if (!ok) {
				fprintf(stderr, "playtest: %s failed\n", cmd);
				return 1;
			}
			printf("%s %s\n", cmd, a);
		} else if (!strcmp(cmd, "reset")) {
			// Power-cycle the console: the only way to test that an in-game
			// SAVE can actually be loaded again from the title screen.
			core->reset(core);
			heldKeys = 0;
			printf("reset\n");
		} else if (!strcmp(cmd, "shot")) {
			Screenshot(a);
		} else if (!strncmp(cmd, "read", 4)) {
			uint32_t addr = strtoul(a, NULL, 0);
			const char* label = n > 2 ? b : a;
			uint32_t v = !strcmp(cmd, "read8") ? core->busRead8(core, addr)
				: !strcmp(cmd, "read16") ? core->busRead16(core, addr)
				: core->busRead32(core, addr);
			printf("read %s 0x%08X = %u (0x%X)\n", label, addr, v, v);
		} else if (!strncmp(cmd, "write", 5)) {
			uint32_t addr = strtoul(a, NULL, 0), val = strtoul(b, NULL, 0);
			if (!strcmp(cmd, "write8")) {
				core->busWrite8(core, addr, val);
			} else if (!strcmp(cmd, "write16")) {
				core->busWrite16(core, addr, val);
			} else {
				core->busWrite32(core, addr, val);
			}
			printf("write %s 0x%08X = %u\n", cmd + 5, addr, val);
		} else if (!strcmp(cmd, "dump")) {
			Dump(strtoul(a, NULL, 0), strtoul(b, NULL, 0), n > 3 ? c : a);
		} else if (!strcmp(cmd, "deref")) {
			uint32_t ptr = core->busRead32(core, strtoul(a, NULL, 0));
			Dump(ptr + strtoul(b, NULL, 0), strtoul(c, NULL, 0), n > 4 ? d : a);
		} else if (!strcmp(cmd, "indexed")) {
			// base stride idxAddr off len [label]: for arrays indexed by a
			// byte in RAM, e.g. gObjectEvents[gPlayerAvatar.objectEventId].
			char e[64], f[64];
			uint32_t base, stride, idxAddr, off, len;
			int m = sscanf(line, "%*s %x %x %x %x %x %63s", &base, &stride, &idxAddr, &off, &len, e);
			UNUSED(f);
			if (m < 5) {
				fprintf(stderr, "playtest: bad indexed\n");
				return 1;
			}
			uint32_t idx = core->busRead8(core, idxAddr);
			Dump(base + idx * stride + off, len, m > 5 ? e : "indexed");
		} else if (!strcmp(cmd, "mashalt")) {
			mashAltKeys = ParseKeys(a);
		} else if (!strcmp(cmd, "mash")) {
			// Setting the main key clears the alternate one, so a plain
			// `mash NONE` really does stop every tap.
			mashKeys = ParseKeys(a);
			mashAltKeys = 0;
			if (n > 2) {
				mashPeriod = strtoul(b, NULL, 0);
				if (mashPeriod < 2) {
					mashPeriod = 2;
				}
			}
		} else if (!strcmp(cmd, "until")) {
			// until abs|ptr <addr> <off> <size> <op> <value> <maxframes>
			// Run until the game says it has got where it was told to go, and
			// say so when it never does -- a story beat that silently fails to
			// set its flag is exactly the defect this harness is for.
			char where[8], op[8], sa[32], so[32], ss[32], sw[32], sm[32];
			char tag[32] = "";
			int m = sscanf(line, "%*s %7s %31s %31s %31s %7s %31s %31s %31s",
				where, sa, so, ss, op, sw, sm, tag);
			uint32_t addr = strtoul(sa, NULL, 0), off = strtoul(so, NULL, 0);
			uint32_t size = strtoul(ss, NULL, 0), want = strtoul(sw, NULL, 0);
			uint32_t max = strtoul(sm, NULL, 0);
			if (m < 7) {
				fprintf(stderr, "playtest: bad until\n");
				return 1;
			}
			bool viaPtr = !strcmp(where, "ptr");
			unsigned long start = frameCount, waited = 0;
			bool ok = false;
			while (waited < max) {
				if (Compare(ReadValue(viaPtr, addr, off, size), op, want)) {
					ok = true;
					break;
				}
				RunFrames(1);
				waited++;
			}
			printf("until %s %s frames=%lu value=%u\n", tag[0] ? tag : "-",
				ok ? "ok" : "TIMEOUT", frameCount - start,
				ReadValue(viaPtr, addr, off, size));
		} else if (!strcmp(cmd, "pread") || !strcmp(cmd, "pwrite")) {
			// pread <ptraddr> <off> <size> [label] / pwrite <..> <value>
			uint32_t addr = strtoul(a, NULL, 0), off = strtoul(b, NULL, 0);
			uint32_t size = strtoul(c, NULL, 0);
			if (cmd[1] == 'w') {
				WriteValue(true, addr, off, size, strtoul(d, NULL, 0));
				printf("pwrite 0x%08X+%u = %lu\n", addr, off, strtoul(d, NULL, 0));
			} else {
				uint32_t v = ReadValue(true, addr, off, size);
				printf("pread %s = %u (0x%X)\n", n > 4 ? d : a, v, v);
			}
		} else if (!strcmp(cmd, "pbit")) {
			// pbit <ptraddr> <byteoff> <bit> read|set|clear [label]
			// Flags are a bitfield hanging off the save block; setting one is
			// how a beat is given the prerequisites of the beats before it.
			char what[8], label[64] = "flag", sa[32], so[32], sb[32];
			int m = sscanf(line, "%*s %31s %31s %31s %7s %63s", sa, so, sb, what, label);
			uint32_t addr = strtoul(sa, NULL, 0), off = strtoul(so, NULL, 0);
			uint32_t bit = strtoul(sb, NULL, 0);
			if (m < 4) {
				fprintf(stderr, "playtest: bad pbit\n");
				return 1;
			}
			uint32_t at = core->busRead32(core, addr) + off;
			uint8_t byte = core->busRead8(core, at);
			if (!strcmp(what, "read")) {
				printf("pbit %s = %u\n", label, (byte >> bit) & 1);
			} else {
				byte = !strcmp(what, "set") ? (byte | (1 << bit)) : (byte & ~(1 << bit));
				core->busWrite8(core, at, byte);
				printf("pbit %s %s\n", label, what);
			}
		} else if (!strcmp(cmd, "pc")) {
			// Sample the program counter over a few frames. A game that has
			// hung never leaves its loop, so the samples name the culprit.
			struct ARMCore* cpu = core->cpu;
			int samples = n > 1 ? (int)strtoul(a, NULL, 0) : 4;
			for (int i = 0; i < samples; ++i) {
				printf("pc = %08X lr = %08X\n", cpu->gprs[15], cpu->gprs[14]);
				RunFrames(1);
			}
		} else if (!strcmp(cmd, "echo")) {
			printf("echo %s\n", line + (strstr(line, "echo") - line) + 5);
		} else {
			fprintf(stderr, "playtest: unknown command '%s'\n", cmd);
			return 1;
		}
		fflush(stdout);
	}

	printf("done frames=%lu\n", frameCount);
	core->deinit(core);
	return 0;
}
