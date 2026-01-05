
#include <memory>
#include <vector>
#include <fstream>

#include "../Object.hpp"

namespace fe {

struct StringData {
		size_t size = 255;
    uint8_t data[255];
  };

struct ObjectData {
	StringData modelFile;
	ObjectState state;
	};

	struct LevelData {
		int objectCount = 0;
        ObjectData objects[0];
	};

	class Level {
		public:

		bool Save(std::vector<fe::Object> objects) {
			std::vector<ObjectData> datas;

			for (auto &object : objects) {

			}

			return true;
		}

		bool Save(std::vector<std::shared_ptr<fe::Object>> objects) {
			// std::vector<ObjectData> datas = std::vector<ObjectData>(objects.size());
			size_t size = sizeof(LevelData) + sizeof(ObjectData) * objects.size();

			LevelData *level = (LevelData *)malloc(size);

			level->objectCount = objects.size();

			size_t i = 0;
			for (auto &object : objects) {
				ObjectData data;
				data.state = object->state;
				// datas.push_back(data);
				level->objects[i] = data;
				++i;
			}

			char *rawLevel = (char *)level;

			std::ofstream saveFile("save.fes", std::ios::binary | std::ios::ate);

			saveFile.write(rawLevel, size);
			// level.objects = *datas.data();

			return true;
		}

		std::vector<std::shared_ptr<fe::Object>> Load() {

			// std::ifstream saveFile("");

			std::ifstream saveFile("save.fes", std::ios::binary | std::ios::ate);
    // if (!saveFile) return nullptr;
    auto size = saveFile.tellg();
    saveFile.seekg(0);
    char* buf = new char[size];
  	saveFile.read(buf, size);
    // return buf;

			saveFile.close();

			LevelData *level = (LevelData *)buf;

			auto objects = std::vector<std::shared_ptr<fe::Object>>();

			for (size_t i = 0; i < level->objectCount; ++i) {
				auto objData = level->objects[i];
				auto objFileName = std::string((char *)objData.modelFile.data, (int)objData.modelFile.size);
				auto obj = std::make_shared<fe::Object>(objFileName, objData.state);
				objects.push_back(obj);
			}
			delete[] buf;
			return objects;
		}
	};
}