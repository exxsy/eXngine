#pragma once

#include <concepts>
#include <eXngine.h>

namespace eXngine::Component
{
    // Base of every component. A component is plain data attached to an entity:
    // it lives in the Component::Manager of its type, keyed by the entity's id.
    // Only types derived from eXcomponent can be used with eXentity::AddComponent<T>.
    struct eXcomponent
    {
    };
}
