
#include <memory>
#include <vector>
#include "../Object.hpp"

namespace fe {

struct StringData {
		size_t size;
    char8_t data[];
  };

struct ObjectData {
	StringData modelFile;
	StringData textureFile;
	};

	struct LevelData {
		int objectCount = 0;
        ObjectData objects[objectCount];
	};

	class Level {

		bool Save(std::vector<fe::Object> objects) {
			return true;
		}
	};
}