#pragma once

#include <eXngine.h>
#include <component/component.h>
#include <types/vector.h>

namespace eXngine::Physics
{
    enum class eXbodyType : EXUINT32
    {
        // Never moves; other bodies collide with it (e.g. the ground).
        Static,
        // Moved by its velocity only: pushes dynamic bodies, is not pushed back (e.g. a lift).
        Kinematic,
        // Moved by gravity, forces and collisions.
        Dynamic,
    };

    // Makes an entity move: eXphysicsSystem integrates its velocity into the entity's
    // eXtransformComponent every step. Without an eXcolliderComponent the body moves but
    // collides with nothing; an eXcolliderComponent without a rigid body is static.
    //   auto *body = entity->AddComponent<eXrigidBodyComponent>();
    //   body->Mass = 2.0f;
    //   body->AddImpulse(eXvec3(0.0f, 5.0f, 0.0f));
    struct eXrigidBodyComponent : public Component::eXcomponent
    {
        eXbodyType Type = eXbodyType::Dynamic;
        // Kilograms; only used by dynamic bodies.
        EXFLOAT Mass = 1.0f;
        // Units per second, world space.
        Types::eXvec3 LinearVelocity = Types::eXvec3(0.0f, 0.0f, 0.0f);
        // Radians per second around each world axis.
        Types::eXvec3 AngularVelocity = Types::eXvec3(0.0f, 0.0f, 0.0f);
        // Fraction of the velocity lost per second (air resistance), 0 = none.
        EXFLOAT LinearDamping = 0.05f;
        EXFLOAT AngularDamping = 0.05f;
        // Multiplies the gravity of the physics system, 0 = floats.
        EXFLOAT GravityScale = 1.0f;

        // Accumulated by AddForce / AddTorque, applied and cleared by the next step.
        Types::eXvec3 Force = Types::eXvec3(0.0f, 0.0f, 0.0f);
        Types::eXvec3 Torque = Types::eXvec3(0.0f, 0.0f, 0.0f);

        eXrigidBodyComponent() = default;
        eXrigidBodyComponent(eXbodyType type, EXFLOAT mass = 1.0f) : Type(type), Mass(mass) {}

        EXBOOL IsDynamic() const { return Type == eXbodyType::Dynamic && Mass > 0.0f; }

        // A force acts over time (e.g. a thruster): call it every frame it should push.
        void AddForce(const Types::eXvec3 &force) { Force = EXMATH::vec3(Force) + EXMATH::vec3(force); }
        void AddTorque(const Types::eXvec3 &torque) { Torque = EXMATH::vec3(Torque) + EXMATH::vec3(torque); }

        // An impulse changes the velocity at once (e.g. a jump or a hit).
        void AddImpulse(const Types::eXvec3 &impulse)
        {
            if (IsDynamic())
                LinearVelocity = EXMATH::vec3(LinearVelocity) + EXMATH::vec3(impulse) / Mass;
        }
    };
}
