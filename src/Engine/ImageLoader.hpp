#pragma once
#include <string>
#include <vector>

namespace fe {

    struct ImageData {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::vector<unsigned char> pixels;
    };

    class ImageLoader {
    public:
        static ImageData Load(const std::string& filePath, int desiredChannels = 4);
    };

}
