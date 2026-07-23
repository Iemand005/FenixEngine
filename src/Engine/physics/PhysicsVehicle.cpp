#include "PhysicsVehicle.hpp"
#include "PhysicsObject.hpp"

#ifndef EXCLUDE_JOLT

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

using namespace JPH;

#endif

using namespace fe;

PhysicsVehicle::PhysicsVehicle() = default;

PhysicsVehicle::~PhysicsVehicle() {
#ifndef EXCLUDE_JOLT
	if (!constraint) return;
	if (physicsSystem) {
		physicsSystem->RemoveStepListener(constraint);
		physicsSystem->RemoveConstraint(constraint);
	}
	constraint->Release();
#endif
}

void PhysicsVehicle::Create(PhysicsObject* body, std::shared_ptr<JPH::PhysicsSystem> system, const std::vector<WheelConfig>& wheelConfigs) {
#ifndef EXCLUDE_JOLT
	this->body = body;
	this->physicsSystem = system;

	BodyID bodyId = body->GetBodyID();
	BodyLockWrite lock(system->GetBodyLockInterface(), bodyId);
	Body& vehicleBody = lock.GetBody();

	if (vehicleBody.GetMotionProperties()) {
		MassProperties mp = vehicleBody.GetShape()->GetMassProperties();
		mp.ScaleToMass(1500.0f);
		vehicleBody.GetMotionProperties()->SetMassProperties(vehicleBody.GetMotionProperties()->GetAllowedDOFs(), mp);
	}

	VehicleConstraintSettings vehicleSettings;
	vehicleSettings.mUp = Vec3(0, 1, 0);
	vehicleSettings.mForward = Vec3(0, 0, 1);
	vehicleSettings.mMaxPitchRollAngle = DegreesToRadians(60.0f);

	WheeledVehicleControllerSettings* controllerSettings = new WheeledVehicleControllerSettings;
	vehicleSettings.mController = controllerSettings;

	controllerSettings->mEngine.mMaxTorque = 1200.0f;
	controllerSettings->mEngine.mMinRPM = 1000.0f;
	controllerSettings->mEngine.mMaxRPM = 8000.0f;
	controllerSettings->mEngine.mInertia = 0.3f;

	controllerSettings->mTransmission.mMode = ETransmissionMode::Auto;
	controllerSettings->mTransmission.mGearRatios = { 2.66f, 1.78f, 1.30f, 1.0f };
	controllerSettings->mTransmission.mReverseGearRatios = { -2.90f };
	controllerSettings->mTransmission.mSwitchTime = 0.05f;
	controllerSettings->mTransmission.mClutchReleaseTime = 0.05f;
	controllerSettings->mTransmission.mSwitchLatency = 0.1f;
	controllerSettings->mTransmission.mShiftUpRPM = 4000.0f;
	controllerSettings->mTransmission.mShiftDownRPM = 2000.0f;
	controllerSettings->mTransmission.mClutchStrength = 10.0f;

	int index = 0;
	int leftDrivenIndex = -1, rightDrivenIndex = -1;
	for (const auto& wc : wheelConfigs) {
		WheelSettingsWV* wheel = new WheelSettingsWV;
		wheel->mPosition = Vec3(wc.position.x, wc.position.y, wc.position.z);
		wheel->mRadius = wc.radius;
		wheel->mWidth = wc.width;
		wheel->mSuspensionMinLength = 0.0f;
		wheel->mSuspensionMaxLength = wc.suspensionMaxLength;
		wheel->mSuspensionSpring.mFrequency = wc.suspensionFrequency;
		wheel->mSuspensionSpring.mDamping = wc.suspensionDamping;
		wheel->mMaxSteerAngle = wc.isSteering ? DegreesToRadians(40.0f) : 0.0f;
		wheel->mMaxBrakeTorque = 1500.0f;
		wheel->mMaxHandBrakeTorque = 4000.0f;

		wheel->mLongitudinalFriction.AddPoint(0.0f, 0.0f);
		wheel->mLongitudinalFriction.AddPoint(0.1f, 1.0f);
		wheel->mLongitudinalFriction.AddPoint(1.0f, 0.8f);

		wheel->mLateralFriction.AddPoint(0.0f, 0.0f);
		wheel->mLateralFriction.AddPoint(5.0f, 1.0f);
		wheel->mLateralFriction.AddPoint(30.0f, 0.8f);

		vehicleSettings.mWheels.push_back(wheel);

		if (wc.isDriven) {
			if (leftDrivenIndex < 0) leftDrivenIndex = index;
			else rightDrivenIndex = index;
		}

		index++;
	}

	if (leftDrivenIndex >= 0 && rightDrivenIndex >= 0) {
		VehicleDifferentialSettings diff;
		diff.mLeftWheel = leftDrivenIndex;
		diff.mRightWheel = rightDrivenIndex;
		diff.mDifferentialRatio = 3.42f;
		diff.mLeftRightSplit = 0.5f;
		diff.mLimitedSlipRatio = FLT_MAX;
		diff.mEngineTorqueRatio = 1.0f;
		controllerSettings->mDifferentials.push_back(diff);
	}

	constraint = new VehicleConstraint(vehicleBody, vehicleSettings);

	constraint->SetVehicleCollisionTester(new VehicleCollisionTesterCastSphere(0, 0.2f));

	system->AddConstraint(constraint);
	system->AddStepListener(constraint);

	controller = static_cast<WheeledVehicleController*>(constraint->GetController());

	// Start in 1st gear with clutch fully engaged (skip neutral delay)
	controller->GetTransmission().Set(1, 1.0f);

#endif
}

void PhysicsVehicle::SetDriverInput(float forward, float right, float brake, float handbrake) {
#ifndef EXCLUDE_JOLT
	if (controller)
		controller->SetDriverInput(forward, right, brake, handbrake);
#endif
}

void PhysicsVehicle::SetMaxPitchRollAngle(float angle) {
#ifndef EXCLUDE_JOLT
	if (constraint) {
		constraint->SetMaxPitchRollAngle(DegreesToRadians(angle));
	}
#endif
}
