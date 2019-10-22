#include "Bitmap.hpp"

void MFTexture::reload(NVGcontext *vg, std::string fileName, int imageFlags) {
	if (image)
		nvgDeleteImage(vg, image);
	image = nvgCreateImage(vg, fileName.c_str(), imageFlags);
	name = fileName;
	context = vg;
	refCount++;
	if (!image)
		return;
	nvgImageSize(vg, image, &width, &height);
}
void MFTexture::release() {
	refCount--;
	if (refCount)
		return;
	refCount = 0;
	if (image)
		nvgDeleteImage(context, image);
	image = 0;
}

std::shared_ptr<MFTexture> MFTextureList::load(NVGcontext *vg, std::string fileName, int imageFlags) {
	for (std::shared_ptr<MFTexture> tex : list) {
		if ((tex->context == vg) && !tex->name.compare(fileName)) {
			if (tex->image) {
				tex->refCount++;
				return tex;
			}
			tex->reload(vg, fileName, imageFlags);	
			return tex;
		}
	}
	std::shared_ptr<MFTexture> tex = std::make_shared<MFTexture>(vg, fileName, imageFlags);
	list.push_back(tex);
	return tex;
}

MFTextureList gTextureList;

void BitMap::DrawImage(NVGcontext *vg) {
	if (!loaded) {
		loaded = true;
		bitmap = gTextureList.load(vg, path, 0);
		if (!bitmap->image)
			WARN("ModularFungi: Unable to load %s", path.c_str());
	}
	if (!bitmap->image)
		return;	
	NVGpaint paint = nvgImagePattern(vg, 0, 0, box.size.x, box.size.y, 0.0f, bitmap->image, 1.0f);
	nvgFillPaint(vg, paint);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, box.size.x, box.size.y);
	nvgFill(vg);
	
}
void BitMap::draw(const DrawArgs &args) {
	DrawImage(args.vg);
	TransparentWidget::draw(args);
}
