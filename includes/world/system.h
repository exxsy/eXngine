#pragma once

#include <eXngine.h>

namespace eXngine::World
{
    class eXworld;

    // Per-frame logic of a world. Components only hold data; systems read and change
    // them, e.g. by looping over every entity that has the components they care about:
    //   class GravitySystem : public eXsystem
    //   {
    //       void OnUpdate(eXworld &world, EXFLOAT deltaTime) override
    //       {
    //           world.Each<eXtransformComponent>([&](auto &entity, auto &transform) { ... });
    //       }
    //   };
    //   world.AddSystem<GravitySystem>();
    class EXNEXPORT eXsystem
    {
    public:
        virtual ~eXsystem() = default;

        // Called when the system is added to / removed from the world (also when the
        // world is destroyed), e.g. to create or release what the system owns.
        virtual void OnAttach(eXworld &) {}
        virtual void OnDetach(eXworld &) {}

        // Called by eXworld::Update once per frame, in the order the systems were added.
        virtual void OnUpdate(eXworld &, EXFLOAT deltaTime) = 0;
    };
}
