#pragma once

#include <string>
#include <utility>
#include <eXngine.h>
#include <component/component.h>

namespace eXngine::Component
{
    // Human readable name of an entity. World::eXworld::CreateEntity adds one to every
    // entity, so tools can list the objects of a world and FindEntity can look them up.
    struct eXnameComponent : public eXcomponent
    {
        std::string Name;

        eXnameComponent() = default;
        eXnameComponent(std::string name) : Name(std::move(name)) {}
    };
}
