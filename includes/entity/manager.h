#pragma once

#include <concepts>
#include <unordered_map>
#include <eXngine.h>
#include <component/component.h>
#include <component/manager.h>
#include <component/registry.h>
#include <entity/entity.h>

#define EXN_ENTITY_AUTO_ID 0xFFFFFFFF

#ifndef EXN_ENTITY_CAPACITY
#define EXN_ENTITY_CAPACITY 10000
#endif

namespace eXngine::Entity
{
    // Owns the entities and, through its registry, all of their components.
    template <std::derived_from<eXentity> T>
    class EXNEXPORT Manager
    {
    private:
        std::unordered_map<EXUINT, T> m_Entities;
        Component::Registry m_Components;
        EXUINT m_nNextID = 1;

    public:
        Manager();
        ~Manager();

        // Stores a copy of entity under id (or the next free id) and returns the stored
        // entity. An entity already stored under id is replaced and loses its components.
        T *AddEntity(const T &entity = T(), const EXUINT id = EXN_ENTITY_AUTO_ID);
        // Removes the entity together with all of its components.
        void RemoveEntity(EXUINT id);
        // Null when no entity has this id.
        T *GetEntity(EXUINT id);
        void Truncate();

        // Every component of type C, e.g. for a system that updates them all:
        //   for (auto &[id, transform] : manager->GetComponents<eXtransformComponent>()) ...
        template <std::derived_from<Component::eXcomponent> C>
        Component::Manager<C> &GetComponents()
        {
            return m_Components.GetManager<C>();
        }
    };
}
