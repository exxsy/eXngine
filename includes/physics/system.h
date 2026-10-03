#pragma once

#include <unordered_map>
#include <vector>
#include <eXngine.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <component/transform.h>
#include <world/system.h>
#include <world/world.h>
#include <physics/collision.h>
#include <physics/components/collider.h>
#include <physics/components/rigidbody.h>

namespace eXngine::Physics
{
    // Two entities that touched during the last step. Normal points from A to B. A negative
    // Penetration is the gap of a point that is about to touch (within the contact margin).
    struct eXcontact
    {
        EXUINT EntityA = 0;
        EXUINT EntityB = 0;
        Types::eXvec3 Point = Types::eXvec3(0.0f, 0.0f, 0.0f);
        Types::eXvec3 Normal = Types::eXvec3(0.0f, 1.0f, 0.0f);
        EXFLOAT Penetration = 0.0f;
    };

    struct eXraycastHit
    {
        EXUINT Entity = 0;
        Types::eXvec3 Point = Types::eXvec3(0.0f, 0.0f, 0.0f);
        Types::eXvec3 Normal = Types::eXvec3(0.0f, 1.0f, 0.0f);
        EXFLOAT Distance = 0.0f;
    };

    // Rigid body physics for the entities of a world that have an eXtransformComponent and
    // an eXrigidBodyComponent and/or an eXcolliderComponent:
    //   world->AddSystem<eXphysicsSystem>();      // before the render system
    //   auto *box = world->CreateEntity("Box");
    //   box->AddComponent<eXtransformComponent>()->Position = eXvec3(0.0f, 2.0f, 0.0f);
    //   box->AddComponent<eXrigidBodyComponent>();
    //   box->AddComponent<eXcolliderComponent>(eXcolliderComponent::Box());
    //
    // Every step:
    //   1. gravity and forces change the velocities of dynamic bodies,
    //   2. every pair of overlapping colliders becomes a contact manifold,
    //   3. the contacts are solved with sequential impulses: each contact point pushes its
    //      two bodies apart (and rubs them against each other, friction) until none of them
    //      moves into the other; the impulses of the last step are the starting guess
    //      (warm starting), which keeps stacks steady,
    //   4. overlaps are pushed apart with separate "push" velocities that move the bodies
    //      during this step only (split impulses), so fixing an overlap adds no energy,
    //   5. the velocities move and rotate the bodies, written back to their transforms.
    // Steps have a fixed length (FixedTimeStep), so the result does not depend on the frame rate.
    class EXNEXPORT eXphysicsSystem : public World::eXsystem
    {
    public:
        eXphysicsSystem(const Types::eXvec3 &gravity = Types::eXvec3(0.0f, -9.81f, 0.0f));

        // Runs as many fixed steps as fit in the time since the last frame.
        void OnUpdate(World::eXworld &, EXFLOAT deltaTime) override;
        void OnDetach(World::eXworld &) override;

        // Advances one step of timeStep seconds, also while paused (e.g. a "Step" button).
        void Step(World::eXworld &, EXFLOAT timeStep);

        // Closest collider hit by the ray within maxDistance. direction need not be unit length.
        EXBOOL Raycast(World::eXworld &, const Types::eXvec3 &origin, const Types::eXvec3 &direction,
                       EXFLOAT maxDistance, eXraycastHit &hit);

        // Every contact point of the last step.
        const std::vector<eXcontact> &GetContacts() const { return m_Contacts; }

        Types::eXvec3 GetGravity() const { return m_Gravity; }
        void SetGravity(const Types::eXvec3 &gravity) { m_Gravity = gravity; }

        EXFLOAT GetFixedTimeStep() const { return m_fFixedTimeStep; }
        void SetFixedTimeStep(EXFLOAT timeStep) { m_fFixedTimeStep = timeStep > 0.0f ? timeStep : m_fFixedTimeStep; }

        // More iterations: stiffer stacks, more work.
        EXUINT32 GetIterations() const { return m_nIterations; }
        void SetIterations(EXUINT32 iterations) { m_nIterations = iterations > 0 ? iterations : 1; }

        EXBOOL IsPaused() const { return m_bPaused; }
        void SetPaused(EXBOOL paused) { m_bPaused = paused; }

    private:
        // A collider or rigid body as the solver sees it, copied from the components at the
        // start of a step and written back at its end.
        struct Body
        {
            EXUINT Entity = 0;
            Component::eXtransformComponent *Transform = nullptr;
            eXrigidBodyComponent *RigidBody = nullptr;
            const eXcolliderComponent *Collider = nullptr;

            eXshape Shape;
            // Center of mass (the collider's center) and the transform's position relative to it.
            EXMATH::vec3 Position = EXMATH::vec3(0.0f);
            EXMATH::vec3 PivotOffset = EXMATH::vec3(0.0f);
            EXMATH::quat Orientation = EXMATH::quat(1.0f, 0.0f, 0.0f, 0.0f);
            EXMATH::vec3 LinearVelocity = EXMATH::vec3(0.0f);
            EXMATH::vec3 AngularVelocity = EXMATH::vec3(0.0f);
            // Push velocities: only move the body out of overlaps during this step.
            EXMATH::vec3 PushVelocity = EXMATH::vec3(0.0f);
            EXMATH::vec3 PushAngularVelocity = EXMATH::vec3(0.0f);

            // 0 for static and kinematic bodies: nothing can push them.
            EXFLOAT InverseMass = 0.0f;
            EXMATH::vec3 InverseInertiaLocal = EXMATH::vec3(0.0f);
            EXMATH::mat3 InverseInertia = EXMATH::mat3(0.0f);

            EXMATH::vec3 MinBounds = EXMATH::vec3(0.0f);
            EXMATH::vec3 MaxBounds = EXMATH::vec3(0.0f);
            EXBOOL Bounded = false;
        };

        struct ContactPoint
        {
            // From each body's center of mass to the contact point.
            EXMATH::vec3 ArmA = EXMATH::vec3(0.0f);
            EXMATH::vec3 ArmB = EXMATH::vec3(0.0f);
            // In A's local space, to find this point again next step (warm starting).
            EXMATH::vec3 LocalA = EXMATH::vec3(0.0f);
            EXFLOAT Penetration = 0.0f;

            // 1 / how hard it is to change the relative velocity along the normal / tangents.
            EXFLOAT NormalMass = 0.0f;
            EXFLOAT TangentMass[2] = {0.0f, 0.0f};
            // Target separating speed: the bounce, or how fast a gap may close.
            EXFLOAT Bias = 0.0f;
            // Target push speed out of the overlap.
            EXFLOAT PushBias = 0.0f;

            // Impulses applied so far in this step.
            EXFLOAT NormalImpulse = 0.0f;
            EXFLOAT TangentImpulse[2] = {0.0f, 0.0f};
            EXFLOAT PushImpulse = 0.0f;
        };

        struct Constraint
        {
            EXUINT32 A = 0;
            EXUINT32 B = 0;
            EXMATH::vec3 Normal = EXMATH::vec3(0.0f, 1.0f, 0.0f);
            EXMATH::vec3 Tangents[2] = {EXMATH::vec3(1.0f, 0.0f, 0.0f), EXMATH::vec3(0.0f, 0.0f, 1.0f)};
            EXFLOAT Friction = 0.5f;
            EXFLOAT Restitution = 0.0f;
            EXUINT32 PointCount = 0;
            ContactPoint Points[EXN_MAX_MANIFOLD_POINTS];
        };

        // Impulses of last step's contact points, by entity pair.
        struct CachedImpulse
        {
            EXMATH::vec3 LocalA = EXMATH::vec3(0.0f);
            EXFLOAT NormalImpulse = 0.0f;
            EXFLOAT TangentImpulse[2] = {0.0f, 0.0f};
        };

        void GatherBodies(World::eXworld &);
        void FindContacts(EXFLOAT timeStep);
        void SolveVelocities();
        void SolvePushes();
        void IntegratePositions(EXFLOAT timeStep);
        void StoreResults();

        Types::eXvec3 m_Gravity;
        // Small steps keep tall stacks of small bodies standing; 1/60 is cheaper but wobblier.
        EXFLOAT m_fFixedTimeStep = 1.0f / 120.0f;
        // A slow frame runs at most this many steps, then lets the simulation fall behind
        // instead of taking longer and longer to catch up.
        EXUINT32 m_nMaxSubSteps = 8;
        EXUINT32 m_nIterations = 10;
        EXBOOL m_bPaused = false;
        EXFLOAT m_fAccumulator = 0.0f;

        std::vector<Body> m_Bodies;
        std::vector<Constraint> m_Constraints;
        std::vector<eXcontact> m_Contacts;
        std::unordered_map<EXUINT64, std::vector<CachedImpulse>> m_ImpulseCache;
    };
}
