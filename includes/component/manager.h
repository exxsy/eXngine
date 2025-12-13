#pragma once

#include <concepts>
#include <unordered_map>
#include <eXngine.h>
#include <entity/entity.h>

using namespace eXngine::Entity;

namespace eXngine::Component
{
    template <std::derived_from<eXentity> T>
    class EXNEXPORT Manager
    {
    private:
        std::unordered_map<EXUINT, T> m_Components;
    public:
        Manager();
        ~Manager();
    };
}