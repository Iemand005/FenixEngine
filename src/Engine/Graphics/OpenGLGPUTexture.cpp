
#include <vector>
#include <cstdint>
#include <string>

#if defined(__ANDROID__)
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "OpenGLGPUTexture.hpp"
#include "../ImageLoader.hpp"

namespace fe {

OpenGLGPUTexture::~OpenGLGPUTexture() {
	if (textureId != 0 && !isCached) glDeleteTextures(1, &textureId);
}

bool OpenGLGPUTexture::load(const std::string& textureFilePath, TextureScaling scaling) {
	auto image = fe::ImageLoader::Load(textureFilePath);
	if (image.pixels.size() == 0) return false;
	return uploadFromImage(image, scaling);
}

bool OpenGLGPUTexture::upload(const ImageData& image, TextureScaling scaling) {
	return uploadFromImage(image, scaling);
}

bool OpenGLGPUTexture::uploadFromImage(const ImageData& image, TextureScaling scaling) {
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLint glScaling = (scaling == TextureScaling::Nearest) ? GL_NEAREST : GL_LINEAR;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glScaling);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glScaling);

	GLenum format = (image.channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.pixels.data());
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

static std::string MakeTextureArrayCacheKey(const std::vector<fe::ImageData>& images, fe::TextureScaling scaling) {
	uint64_t h = 1469598103934665603ull;
	auto mix = [&](const unsigned char* d, size_t n) {
		for (size_t i = 0; i < n; ++i) { h ^= d[i]; h *= 1099511628211ull; }
	};
	for (const auto& img : images) {
		int meta[3] = { img.width, img.height, img.channels };
		mix(reinterpret_cast<const unsigned char*>(meta), sizeof(meta));
		if (!img.pixels.empty()) mix(img.pixels.data(), img.pixels.size());
	}
	h ^= static_cast<uint64_t>(static_cast<int>(scaling));
	h *= 1099511628211ull;
	return std::to_string(static_cast<unsigned long long>(h));
}

bool OpenGLGPUTexture::loadTextureArray(const std::vector<ImageData>& images, TextureScaling scaling) {
	if (images.empty()) return false;

	auto& cache = GetTextureArrayCache();
	std::string cacheKey = MakeTextureArrayCacheKey(images, scaling);

	auto cachedIt = cache.find(cacheKey);
	if (cachedIt != cache.end()) {
		textureId = cachedIt->second;
		isCached = true;
		arrayTexture = true;
		layerCount = static_cast<int>(images.size());
		return true;
	}

	int baseWidth = images[0].width;
	int baseHeight = images[0].height;
	int nrChannels = images[0].channels;
	if (baseWidth <= 0 || baseHeight <= 0 || nrChannels <= 0 || images[0].pixels.empty()) return false;

	GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
	GLint internalFormat = (nrChannels == 4) ? GL_RGBA8 : GL_RGB8;
	int layers = static_cast<int>(images.size());

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, baseWidth, baseHeight, layers, 0, format, GL_UNSIGNED_BYTE, nullptr);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, baseWidth, baseHeight, 1, format, GL_UNSIGNED_BYTE, images[0].pixels.data());

	for (int i = 1; i < layers; ++i) {
		const auto& img = images[i];
		if (img.pixels.empty()) continue;
		if (img.width == baseWidth && img.height == baseHeight && img.channels == nrChannels) {
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, baseWidth, baseHeight, 1, format, GL_UNSIGNED_BYTE, img.pixels.data());
		}
	}

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	GLint glScaling = (scaling == TextureScaling::Nearest) ? GL_NEAREST : GL_LINEAR;
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, glScaling);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, glScaling);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	cache[cacheKey] = textureId;
	isCached = true;
	arrayTexture = true;
	layerCount = layers;
	return true;
}

bool OpenGLGPUTexture::loadTextureArray(const std::vector<std::string>& textureFilePaths, TextureScaling scaling) {
	if (textureFilePaths.empty()) return false;

	std::vector<ImageData> images;
	images.reserve(textureFilePaths.size());
	for (const auto& path : textureFilePaths)
		images.push_back(fe::ImageLoader::Load(path));

	return loadTextureArray(images, scaling);
}

std::unordered_map<std::string, unsigned int>& OpenGLGPUTexture::GetTextureArrayCache() {
	static std::unordered_map<std::string, unsigned int> cache;
	return cache;
}

}
