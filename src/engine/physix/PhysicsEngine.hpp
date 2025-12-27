

// #include <Jolt/Jolt.h>
// #include <Jolt/Geometry/IndexedTriangle.h>
// #include <Jolt/Physics/PhysicsSystem.h>
// #include <Jolt/Physics/Body/BodyCreationSettings.h>
// #include <Jolt/Physics/Body/BodyInterface.h>
// #include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
// #include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <iostream>
#include <cstdarg>
#include <thread>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>



class PhysicsEngine {
    JPH::PhysicsSystem* physicsSystem = new JPH::PhysicsSystem();

  public:

  PhysicsEngine(){
    // b
    //JPH::Factory::sInstance = new JPH::Factory();
    // // Create physics system
    // physicsSystem->Init(1024, 0, 1024, 1024,  nullptr, nullptr, &object_layer_pair_filter);
    
    // // Set gravity and other settings
    // physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));


    RegisterDefaultAllocator();
  JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
  Factory::sInstance = new Factory();
  RegisterTypes();

  TempAllocatorMalloc temp_allocator;
  JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

  const uint cMaxBodies = 1024;
  const uint cNumBodyMutexes = 0;
  const uint cMaxBodyPairs = 1024;
  const uint cMaxContactConstraints = 1024;
  BPLayerInterfaceImpl broad_phase_layer_interface;
  ObjectLayerPairFilterImpl object_vs_object_layer_filter;
  ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

  PhysicsSystem physics_system;
  physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface,
                      object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
  physics_system.SetGravity(Vec3::sZero());
  BodyInterface &body_interface = physics_system.GetBodyInterface();

  BoxShapeSettings body_shape_settings(Vec3(a, b, c));
  body_shape_settings.mConvexRadius = 0.01;
  body_shape_settings.SetDensity(1000.0);
  body_shape_settings.SetEmbedded();
  ShapeSettings::ShapeResult body_shape_result = body_shape_settings.Create();
  ShapeRefC body_shape = body_shape_result.Get();
  BodyCreationSettings body_settings(body_shape, RVec3(0.0, 0.0, 0.0), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
  body_settings.mMaxLinearVelocity = 10000.0;
  body_settings.mApplyGyroscopicForce = true;
  body_settings.mLinearDamping = 0.0;
  body_settings.mAngularDamping = 0.0;
  Body *body = body_interface.CreateBody(body_settings);
  body_interface.AddBody(body->GetID(), EActivation::Activate);
  body_interface.SetLinearVelocity(body->GetID(), Vec3(0.0, 0.0, 0.0));
  body_interface.SetAngularVelocity(body->GetID(), Vec3(0.3, 0.0, 5.0));

  physics_system.OptimizeBroadPhase();
  }

};