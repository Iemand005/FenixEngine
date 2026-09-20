#pragma once

#if defined(__ANDROID__)
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif
#include "IGPUTexture.hpp"
#include "../ImageLoader.hpp"
#include <unordered_map>
#include <string>

namespace fe {
	class OpenGLGPUTexture : public IGPUTexture {
	public:
		unsigned int textureId = 0;
		bool arrayTexture = false;
		int layerCount = 0;

		~OpenGLGPUTexture() override;

		bool isTextureArray() const override { return arrayTexture; }
		int getLayerCount() const override { return layerCount; }

		bool load(const std::string& textureFilePath, TextureScaling scaling = TextureScaling::Linear) override;

		bool upload(const ImageData& image, TextureScaling scaling = TextureScaling::Linear);

		bool uploadFromImage(const ImageData& image, TextureScaling scaling);

		bool loadTextureArray(const std::vector<std::string>& textureFilePaths, TextureScaling scaling = TextureScaling::Linear);

		static std::unordered_map<std::string, unsigned int>& GetTextureArrayCache();

	private:
		bool isCached = false;
	};
}