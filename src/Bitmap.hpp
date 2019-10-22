#pragma once

#include "rack.hpp"

using namespace rack;

struct MFTexture {
	int image = 0;
	std::string name;
	NVGcontext *context;
	int width;
	int height;
	int refCount = 0;
	MFTexture(NVGcontext *vg, std::string fileName, int imageFlags) {
		reload(vg, fileName, imageFlags);
	}
	void reload(NVGcontext *vg, std::string fileName, int imageFlags);
	void release();
	~MFTexture() {
		release();
	}
};

struct MFTextureList {
	std::vector<std::shared_ptr<MFTexture>> list;
	std::shared_ptr<MFTexture> load(NVGcontext *vg, std::string fileName, int imageFlags);
};

extern MFTextureList gTextureList;

struct BitMap : TransparentWidget {
	std::string path;
	int loaded = false;
	std::shared_ptr<MFTexture> bitmap;
	void DrawImage(NVGcontext *vg);
	void draw(const DrawArgs &args) override;
	~BitMap() {
		if (bitmap)
			bitmap->release();
	}
};
