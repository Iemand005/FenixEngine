#pragma once

#include <string>
#include <vector>
#include <memory>

#include <glad/glad.h>


namespace fe {
    enum class TextureScaling { Nearest, Linear };

    class IGPUTexture {
    public:
        virtual ~IGPUTexture() = default;

        bool LoadTexture(std::string textureFilePath, TextureScaling newScaling = TextureScaling::Linear) {
                
                auto image = fe::ImageLoader::Load(textureFilePath);
                if (image.pixels.size() == 0) return false;

                // glGenTextures(1, &this->texture);
                // glBindTexture(GL_TEXTURE_2D, this->texture);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                GLint texScaling = GL_LINEAR;
                if (newScaling == TextureScaling::Nearest) texScaling = GL_NEAREST;

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texScaling);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texScaling);

                GLenum format = ( image.channels == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.pixels.data());
                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);
                return true;
            }
    };

    class Texture {
    public:
        std::unique_ptr<IGPUTexture> gpuTexture = nullptr;
    };

}
