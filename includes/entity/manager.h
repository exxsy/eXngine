#pragma once

#include <concepts>
#include <unordered_map>
#include <eXngine.h>
#include <entity/entity.h>

#define EXN_ENTITY_AUTO_ID 0xFFFFFFFF

#ifndef EXN_ENTITY_CAPACITY
#define EXN_ENTITY_CAPACITY 10000
#endif

namespace eXngine::Entity
{
    template <std::derived_from<eXentity> T>
    class EXNEXPORT Manager
    {
    private:
        std::unordered_map<EXUINT, T> m_Entities;
    public:
        Manager();
        ~Manager();

        void AddEntity(const T & entity, const EXUINT id = EXN_ENTITY_AUTO_ID);
        void RemoveEntity(EXUINT id);
        T* GetEntity(EXUINT id);
        void Truncate();
    };
}