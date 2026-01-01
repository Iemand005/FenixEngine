#pragma once
#include <glad/glad.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "physics/PhysicsEngine.hpp"

#ifdef OBJ_LOADER
#include <OBJ_Loader.h>
#endif

#include <stb_image.h>

#include "Object.hpp"
#include "Camera.hpp"
#include "Scene.hpp"
#include "Character.hpp"
