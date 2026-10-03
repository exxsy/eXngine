#pragma once

#include <eXngine.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <physics/components/collider.h>
#include <types/transform.h>

namespace eXngine::Physics
{
    // A collider placed in the world: the collider's sizes scaled by the entity's
    // transform, its center and axes in world space. This is what the tests below use.
    struct eXshape
    {
        eXcolliderShape Type = eXcolliderShape::Box;
        EXMATH::vec3 Center = EXMATH::vec3(0.0f);
        EXMATH::quat Orientation = EXMATH::quat(1.0f, 0.0f, 0.0f, 0.0f);
        // Columns are the local x, y and z axes in world space.
        EXMATH::mat3 Axes = EXMATH::mat3(1.0f);
        // Box.
        EXMATH::vec3 HalfExtents = EXMATH::vec3(0.5f);
        // Sphere.
        EXFLOAT Radius = 0.5f;
        // Plane, world space, unit length.
        EXMATH::vec3 Normal = EXMATH::vec3(0.0f, 1.0f, 0.0f);
    };

    // At most 8 points: clipping a box face against another box face.
    constexpr EXUINT32 EXN_MAX_MANIFOLD_POINTS = 8;

    struct eXcontactPoint
    {
        // World space, halfway between the two surfaces.
        EXMATH::vec3 Position = EXMATH::vec3(0.0f);
        // How deep the shapes overlap at this point; < 0 is the gap of a point that is
        // within the contact margin but not touching yet.
        EXFLOAT Penetration = 0.0f;
    };

    // Where two shapes touch. Moving B along Normal (or A against it) separates them.
    struct eXmanifold
    {
        EXMATH::vec3 Normal = EXMATH::vec3(0.0f, 1.0f, 0.0f);
        EXUINT32 PointCount = 0;
        eXcontactPoint Points[EXN_MAX_MANIFOLD_POINTS];
    };

    // Places a collider at a transform.
    EXNEXPORT eXshape MakeShape(const eXcolliderComponent &, const Types::eXtransform &);

    // Axis aligned box around the shape; a plane has no bounds (returns false).
    EXNEXPORT EXBOOL ComputeBounds(const eXshape &, EXMATH::vec3 &min, EXMATH::vec3 &max);

    // Fills manifold and returns true when the shapes overlap or are closer than margin.
    // Every pair is supported except plane / plane.
    EXNEXPORT EXBOOL Collide(const eXshape &a, const eXshape &b, eXmanifold &manifold, EXFLOAT margin = 0.0f);

    // Distance along direction (unit length) to where the ray enters the shape, and the
    // surface normal there. A ray that starts inside the shape hits it at distance 0.
    EXNEXPORT EXBOOL Raycast(const eXshape &, const EXMATH::vec3 &origin, const EXMATH::vec3 &direction,
                             EXFLOAT maxDistance, EXFLOAT &distance, EXMATH::vec3 &normal);
}
