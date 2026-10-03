#pragma once

#include <eXngine.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <component/component.h>
#include <types/matrix.h>
#include <types/transform.h>

namespace eXngine::Component
{
    // Position, rotation and scale of an entity:
    //   auto *transform = entity->AddComponent<eXtransformComponent>();
    //   transform->Position = eXvec3(1.0f, 0.0f, 0.0f);
    //   draw->model = transform->GetMatrix();
    struct eXtransformComponent : public eXcomponent, public Types::eXtransform
    {
        using Types::eXtransform::eXtransform;

        eXtransformComponent() = default;
        eXtransformComponent(const Types::eXtransform &transform) : Types::eXtransform(transform) {}

        // Euler angles in radians: pitch (x), yaw (y), roll (z).
        Types::eXvec3 GetEulerAngles() const { return EXMATH::eulerAngles(EXMATH::quat(Rotation)); }
        void SetEulerAngles(const Types::eXvec3 &radians) { Rotation = EXMATH::quat(EXMATH::vec3(radians)); }

        // Model matrix: scales, then rotates, then translates.
        Types::eXmat4 GetMatrix() const
        {
            const EXMATH::mat4 translation = EXMATH::translate(EXMATH::mat4(1.0f), EXMATH::vec3(Position));
            const EXMATH::mat4 rotation = EXMATH::mat4_cast(EXMATH::quat(Rotation));

            return EXMATH::scale(translation * rotation, EXMATH::vec3(Scale));
        }
    };
}
