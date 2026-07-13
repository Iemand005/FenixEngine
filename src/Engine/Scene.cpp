#include "Scene.hpp"
#include "Object.hpp"

using namespace fe;

Scene::Scene() Scene() {
	objects = std::vector<std::shared_ptr<Object>>();
	this->EnableDepthTest();
	this->EnableFaceCulling();
}