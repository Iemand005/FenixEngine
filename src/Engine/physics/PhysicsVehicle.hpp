#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

namespace JPH {
class PhysicsSystem;
class VehicleConstraint;
class WheeledVehicleController;
class WheelSettingsWV;
}

namespace fe {

class PhysicsObject;

class PhysicsVehicle {
public:
    struct WheelConfig {
        glm::vec3 position;
        float radius = 0.4f;
        float width = 0.3f;
        float suspensionMaxLength = 0.3f;
        float suspensionFrequency = 1.5f;
        float suspensionDamping = 0.5f;
        float friction = 0.8f;
        bool isSteering = false;
        bool isDriven = false;
    };

    PhysicsVehicle();
    ~PhysicsVehicle();

    PhysicsVehicle(const PhysicsVehicle&) = delete;
    PhysicsVehicle& operator=(const PhysicsVehicle&) = delete;

    void Create(PhysicsObject* body, std::shared_ptr<JPH::PhysicsSystem> physicsSystem, const std::vector<WheelConfig>& wheels);

    void SetDriverInput(float forward, float right, float brake, float handbrake);
    void SetMaxPitchRollAngle(float angle);

    struct WheelForceData {
        float lateralLambda = 0.0f;
        float longitudinalLambda = 0.0f;
        float suspensionLambda = 0.0f;
        bool hasContact = false;
    };

    WheelForceData GetWheelForce(uint index);
    int GetNumWheels();
    float GetEngineRPM();

    PhysicsObject* GetBody() { return body; }

private:
    PhysicsObject* body = nullptr;
    std::shared_ptr<JPH::PhysicsSystem> physicsSystem;
    JPH::VehicleConstraint* constraint = nullptr;
    JPH::WheeledVehicleController* controller = nullptr;
    std::vector<JPH::WheelSettingsWV*> rearWheelSettings;
    float driftValue = 0.0f;
};

}
