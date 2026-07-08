#include "ImageLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

#include "WawaDir.hpp"

namespace fe {

    ImageData ImageLoader::Load(const std::string& filePath, int desiredChannels) {
        ImageData data;
        
        unsigned char* rawPixels = stbi_load(filePath.c_str(), &data.width, &data.height, &data.channels, desiredChannels);
        
        if (!rawPixels) {
            std::cerr << "[Fenix Error] Failed to load image: " << filePath << " -> " << stbi_failure_reason() << "\n";

// #include "WawaDir.hpp"
			std::string exeDir = GetExecutableDirectorye();
			std::string path2 = exeDir + "/" + filePath; // TODO WINDOWS BLEH BACKLSAHS DOES IT ACCPET FWND SLSH I THINK TI DOES
			rawPixels = stbi_load(path2.c_str(), &data.width, &data.height, &data.channels, desiredChannels);
			if (rawPixels)
			{
				std::cout << "Loaded from exe dir: " << path2 << std::endl;
				return data;
			}


            return data; 
        }

        if (desiredChannels != 0) {
            data.channels = desiredChannels;
        }

        size_t totalSize = data.width * data.height * data.channels;
        data.pixels.assign(rawPixels, rawPixels + totalSize);

        stbi_image_free(rawPixels);

        return data;
    }

}
