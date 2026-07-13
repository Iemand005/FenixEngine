#pragma once

#include <glad/glad.h>
#include "IGPUTexture.hpp"
#include "../ImageLoader.hpp"

namespace fe {
	class OpenGLGPUTexture : public IGPUTexture {
	public:
		unsigned int textureId = 0;

		~OpenGLGPUTexture() override {
			if (textureId != 0) glDeleteTextures(1, &textureId);
		}

		bool load(const std::string& textureFilePath, TextureScaling scaling = TextureScaling::Linear) override {
			auto image = fe::ImageLoader::Load(textureFilePath);
			if (image.pixels.size() == 0) return false;

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
	};
}