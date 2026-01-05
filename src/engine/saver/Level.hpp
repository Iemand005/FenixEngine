
#include <memory>
#include <vector>
#include "../Object.hpp"

namespace fe {

struct StringData {
		size_t size = 255;
    char data[0];
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
	};
}