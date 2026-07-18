#pragma once
#include <glm/glm.hpp>

namespace fe {
	struct ObjectState {
		glm::vec3 position{0.0f};
		glm::vec3 rotation{0.0f};
		glm::vec3 velocity{0.0f};
		glm::vec3 scale{1.0f};
	};

	enum Direction {
		Forwards,
		Backwards,
		Left,
		Right,
		Up,
		Down
	};
}