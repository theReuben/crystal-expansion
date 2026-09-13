// Headless GBA screenshotter: run N frames, optionally tap keys, dump PNG.
// usage: shot <rom> <out.png> <frames> [frame:KEYMASK ...]
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba-util/vfs.h>
#include <mgba-util/png-io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static color_t buffer[256 * 256];

int main(int argc, char** argv) {
	if (argc < 4) { fprintf(stderr, "usage: shot <rom> <out.png> <frames> [frame:keys ...]\n"); return 2; }
	const char* rom = argv[1];
	const char* out = argv[2];
	int frames = atoi(argv[3]);

	struct mCore* core = mCoreFind(rom);
	if (!core) { fprintf(stderr, "no core for %s\n", rom); return 1; }
	core->init(core);
	core->setVideoBuffer(core, buffer, 256);
	if (!mCoreLoadFile(core, rom)) { fprintf(stderr, "load failed\n"); return 1; }
	mCoreConfigInit(&core->config, "shot");
	struct mCoreOptions opts = {};
	opts.audioSync = false; opts.videoSync = false;
	mCoreConfigLoadDefaults(&core->config, &opts);
	mCoreConfigSetDefaultValue(&core->config, "idleOptimization", "detect");
	mCoreLoadConfig(core);
	core->reset(core);

	for (int f = 0; f < frames; ++f) {
		uint32_t keys = 0;
		for (int a = 4; a < argc; ++a) {
			int at; unsigned k;
			if (sscanf(argv[a], "%d:%u", &at, &k) == 2 && f >= at && f < at + 6) keys |= k;
		}
		core->setKeys(core, keys);
		core->runFrame(core);
	}

	unsigned w, h;
	core->desiredVideoDimensions(core, &w, &h);
	struct VFile* vf = VFileOpen(out, O_WRONLY | O_CREAT | O_TRUNC);
	png_structp png = PNGWriteOpen(vf);
	png_infop info = PNGWriteHeader(png, w, h);
	PNGWritePixels(png, w, h, 256, buffer);
	PNGWriteClose(png, info);
	vf->close(vf);
	printf("wrote %s (%ux%u) after %d frames\n", out, w, h, frames);
	return 0;
}
