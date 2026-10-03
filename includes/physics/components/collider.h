#pragma once

#include <eXngine.h>
#include <component/component.h>
#include <types/vector.h>

namespace eXngine::Physics
{
    enum class eXcolliderShape : EXUINT32
    {
        Sphere,
        Box,
        // Infinite plane through the collider's center. Everything behind it is pushed out
        // to its front side, so fast bodies cannot fall through. Always static.
        Plane,
    };

    // The shape an entity collides with. Sizes are in the entity's local space: the
    // eXtransformComponent scales them (a sphere by its largest scale axis), so a collider
    // matches a mesh of the same size. The shape is centered on the entity, moved by Offset.
    //   entity->AddComponent<eXcolliderComponent>(eXcolliderComponent::Box(eXvec3(0.5f, 0.5f, 0.5f)));
    struct eXcolliderComponent : public Component::eXcomponent
    {
        eXcolliderShape Shape = eXcolliderShape::Box;
        // Box: half of the size along each local axis.
        Types::eXvec3 HalfExtents = Types::eXvec3(0.5f, 0.5f, 0.5f);
        // Sphere.
        EXFLOAT Radius = 0.5f;
        // Plane: the side that faces away from the plane, in local space.
        Types::eXvec3 Normal = Types::eXvec3(0.0f, 1.0f, 0.0f);
        // Center of the shape relative to the entity, in local space.
        Types::eXvec3 Offset = Types::eXvec3(0.0f, 0.0f, 0.0f);

        // 0 = ice, 1 = rubber. Two colliders use the geometric mean of their frictions.
        EXFLOAT Friction = 0.5f;
        // Bounciness: 0 = stops, 1 = bounces back at full speed. The larger one is used.
        EXFLOAT Restitution = 0.2f;

        static eXcolliderComponent Sphere(EXFLOAT radius = 0.5f)
        {
            eXcolliderComponent collider;
            collider.Shape = eXcolliderShape::Sphere;
            collider.Radius = radius;
            return collider;
        }

        static eXcolliderComponent Box(const Types::eXvec3 &halfExtents = Types::eXvec3(0.5f, 0.5f, 0.5f))
        {
            eXcolliderComponent collider;
            collider.Shape = eXcolliderShape::Box;
            collider.HalfExtents = halfExtents;
            return collider;
        }

        static eXcolliderComponent Plane(const Types::eXvec3 &normal = Types::eXvec3(0.0f, 1.0f, 0.0f))
        {
            eXcolliderComponent collider;
            collider.Shape = eXcolliderShape::Plane;
            collider.Normal = normal;
            return collider;
        }
    };
}
