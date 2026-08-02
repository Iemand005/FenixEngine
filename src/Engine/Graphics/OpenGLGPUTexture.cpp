
#include <vector>

#include <glad/glad.h>

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

bool OpenGLGPUTexture::loadTextureArray(const std::vector<std::string>& textureFilePaths, TextureScaling scaling) {
	if (textureFilePaths.empty()) return false;

	auto& cache = GetTextureArrayCache();
	std::string cacheKey;
	for (const auto& path : textureFilePaths) { cacheKey += path; cacheKey += '\0'; }
	cacheKey += std::to_string(static_cast<int>(scaling));

	auto cachedIt = cache.find(cacheKey);
	if (cachedIt != cache.end()) {
		textureId = cachedIt->second;
		isCached = true;
		arrayTexture = true;
		layerCount = static_cast<int>(textureFilePaths.size());
		return true;
	}

	auto firstImage = fe::ImageLoader::Load(textureFilePaths[0]);
	if (firstImage.pixels.empty()) return false;

	int baseWidth = firstImage.width;
	int baseHeight = firstImage.height;
	int nrChannels = firstImage.channels;
	GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
	GLint internalFormat = (nrChannels == 4) ? GL_RGBA8 : GL_RGB8;
	int layers = static_cast<int>(textureFilePaths.size());

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, baseWidth, baseHeight, layers, 0, format, GL_UNSIGNED_BYTE, nullptr);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, baseWidth, baseHeight, 1, format, GL_UNSIGNED_BYTE, firstImage.pixels.data());

	for (int i = 1; i < layers; ++i) {
		auto img = fe::ImageLoader::Load(textureFilePaths[i]);
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

std::unordered_map<std::string, unsigned int>& OpenGLGPUTexture::GetTextureArrayCache() {
	static std::unordered_map<std::string, unsigned int> cache;
	return cache;
}

}
