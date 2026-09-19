// Crystal Expansion headless play-test harness.
//
// Boots the ROM in libmgba with no window, runs a script of waits, button
// presses, screenshots and memory reads, and prints every read to stdout so a
// test can assert on it. The test suite runs code; this runs the game.
//
// Build with tools/playtest/build.sh.

#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>

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

static void QuietLog(struct mLogger* logger, int category, enum mLogLevel level, const char* format, va_list args)
{
	UNUSED(logger);
	UNUSED(level);
	bool isGame = !strcmp(mLogCategoryId(category), "gba.debug");
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

static void RunFrames(unsigned long frames)
{
	for (unsigned long i = 0; i < frames; i++) {
		core->setKeys(core, heldKeys);
		core->runFrame(core);
		frameCount++;
	}
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
		// mGBA's desktop build is 8888; mColorFrom555 is not in play here.
		uint8_t rgb[3] = { (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF };
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
		"  shot <file.ppm>\n"
		"  read8|read16|read32 <addr> [label]\n"
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
	mLogSetDefaultLogger(&sLogger);
	core = mCoreFind(romPath);
	if (!core) {
		fprintf(stderr, "playtest: no core for %s\n", romPath);
		return 1;
	}
	core->init(core);
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
		} else if (!strcmp(cmd, "shot")) {
			Screenshot(a);
		} else if (!strncmp(cmd, "read", 4)) {
			uint32_t addr = strtoul(a, NULL, 0);
			const char* label = n > 2 ? b : a;
			uint32_t v = !strcmp(cmd, "read8") ? core->busRead8(core, addr)
				: !strcmp(cmd, "read16") ? core->busRead16(core, addr)
				: core->busRead32(core, addr);
			printf("read %s 0x%08X = %u (0x%X)\n", label, addr, v, v);
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
