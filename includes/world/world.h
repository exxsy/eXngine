#pragma once

#include <algorithm>
#include <concepts>
#include <memory>
#include <string>
#include <tuple>
#include <typeindex>
#include <utility>
#include <vector>
#include <eXngine.h>
#include <component/component.h>
#include <component/manager.h>
#include <component/name.h>
#include <entity/entity.h>
#include <entity/manager.h>
#include <world/system.h>

#ifndef EXN_ENTITY_TYPE
#define EXN_ENTITY_TYPE eXngine::Entity::eXentity
#endif

namespace eXngine::World
{
    // The container of a game: every object is an entity of the world, every piece of
    // data is a component of an entity and every piece of per-frame logic is a system.
    // Destroying the world destroys all of them.
    //   eXworld world;
    //   auto *quad = world.CreateEntity("Quad");
    //   quad->AddComponent<eXtransformComponent>()->Position = eXvec3(1.0f, 0.0f, 0.0f);
    //   world.AddSystem<SpinSystem>();
    //   world.Update(deltaTime); // once per frame
    class EXNEXPORT eXworld
    {
    public:
        using EntityType = EXN_ENTITY_TYPE;

    private:
        Entity::Manager<EntityType> m_Entities;
        // Run in this order; the type identifies the system for GetSystem / RemoveSystem.
        std::vector<std::pair<std::type_index, std::unique_ptr<eXsystem>>> m_Systems;
        // Entities destroyed while the systems run are removed once they are done.
        std::vector<EXUINT> m_PendingDestroy;
        EXBOOL m_bUpdating = false;

        auto FindSystem(const std::type_index &type)
        {
            return std::find_if(m_Systems.begin(), m_Systems.end(), [&type](const auto &system)
                                { return system.first == type; });
        }

    public:
        eXworld();
        ~eXworld();
        eXworld(const eXworld &) = delete;
        eXworld &operator=(const eXworld &) = delete;

        // Creates an entity with an eXnameComponent holding name.
        EntityType *CreateEntity(const std::string &name = "Entity");
        // Destroys the entity together with all of its components. Called from a system,
        // the entity stays alive until every system has run this frame.
        void DestroyEntity(EXUINT id);
        // Null when no entity has this id.
        EntityType *GetEntity(EXUINT id);
        // First entity named name, null when there is none.
        EntityType *FindEntity(const std::string &name);
        EXSIZE GetEntityCount() const;
        // Destroys every entity; the systems stay.
        void Clear();

        // Runs every system once, then removes the entities destroyed meanwhile.
        void Update(EXFLOAT deltaTime);

        // Range-for yields (id, entity) pairs, i.e. every object of the world:
        //   for (auto &[id, entity] : world) ...
        // Do not create or destroy entities while iterating.
        auto begin() { return m_Entities.begin(); }
        auto end() { return m_Entities.end(); }

        // Every component of type C: for (auto &[id, transform] : world.GetComponents<eXtransformComponent>()) ...
        template <std::derived_from<Component::eXcomponent> C>
        Component::Manager<C> &GetComponents()
        {
            return m_Entities.GetComponents<C>();
        }

        // Calls func(entity, first, rest...) for every entity that has all of the listed
        // components, e.g.
        //   world.Each<SpinComponent, eXtransformComponent>([&](auto &entity, auto &spin, auto &transform) { ... });
        // The loop runs over the entities that have First: list the rarest component first.
        // Do not add or remove components of the listed types inside func.
        template <std::derived_from<Component::eXcomponent> First, std::derived_from<Component::eXcomponent>... Rest, typename Func>
        void Each(Func &&func)
        {
            for (auto &[id, first] : m_Entities.GetComponents<First>())
            {
                EntityType *entity = m_Entities.GetEntity(id);

                if (entity == EXN_NULL_HANDLE)
                    continue;

                // Each other component is looked up once; null when the entity lacks it.
                const std::tuple<Rest *...> rest{entity->GetComponent<Rest>()...};
                const EXBOOL hasAll = std::apply([](auto *...components)
                                                 { return ((components != EXN_NULL_HANDLE) && ...); }, rest);

                if (hasAll)
                    std::apply([&](auto *...components)
                               { func(*entity, first, *components...); }, rest);
            }
        }

        // Constructs S from args; it runs after the systems added before it. Replaces the
        // S the world already has, keeping its place in the order.
        template <std::derived_from<eXsystem> S, typename... Args>
        S *AddSystem(Args &&...args)
        {
            EX_FATAL(!m_bUpdating, "Systems cannot be added while the world is updating.");

            auto system = std::make_unique<S>(std::forward<Args>(args)...);
            S *added = system.get();
            const auto it = FindSystem(typeid(S));

            if (it != m_Systems.end())
            {
                it->second->OnDetach(*this);
                it->second = std::move(system);
            }
            else
            {
                m_Systems.emplace_back(typeid(S), std::move(system));
            }

            added->OnAttach(*this);
            return added;
        }

        // Null when the world has no S.
        template <std::derived_from<eXsystem> S>
        S *GetSystem()
        {
            const auto it = FindSystem(typeid(S));
            return it != m_Systems.end() ? static_cast<S *>(it->second.get()) : EXN_NULL_HANDLE;
        }

        template <std::derived_from<eXsystem> S>
        void RemoveSystem()
        {
            EX_FATAL(!m_bUpdating, "Systems cannot be removed while the world is updating.");

            const auto it = FindSystem(typeid(S));

            if (it == m_Systems.end())
                return;

            it->second->OnDetach(*this);
            m_Systems.erase(it);
        }
    };
}
