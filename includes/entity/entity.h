#pragma once

#include <cassert>
#include <concepts>
#include <utility>
#include <eXngine.h>
#include <component/component.h>
#include <component/registry.h>

namespace eXngine::Entity
{
    struct eXentity;

    template <std::derived_from<eXentity> T>
    class Manager;

    // An id plus the registry that stores its components. Entities are created by
    // Entity::Manager::AddEntity; copies of an entity refer to the same components.
    //   eXentity *entity = manager->AddEntity();
    //   entity->AddComponent<eXtransformComponent>()->Position = eXvec3(1.0f, 0.0f, 0.0f);
    struct EXNEXPORT eXentity
    {
    public:
        EXN_PROPERTY(EXUINT, MeshID, MeshID, 0);

        eXentity();
        ~eXentity();

        EXUINT GetID() const { return m_nID; }

        // Constructs T from args and attaches it; replaces the T the entity already has.
        template <std::derived_from<Component::eXcomponent> T, typename... Args>
        T *AddComponent(Args &&...args)
        {
            EX_FATAL(m_pComponents != EXN_NULL_HANDLE, "Entity is not registered: create it with Entity::Manager::AddEntity.");
            return m_pComponents->Add<T>(m_nID, std::forward<Args>(args)...);
        }

        // Null when the entity has no T.
        template <std::derived_from<Component::eXcomponent> T>
        T *GetComponent() const
        {
            return m_pComponents != EXN_NULL_HANDLE ? m_pComponents->Get<T>(m_nID) : EXN_NULL_HANDLE;
        }

        template <std::derived_from<Component::eXcomponent> T>
        EXBOOL HasComponent() const
        {
            return m_pComponents != EXN_NULL_HANDLE && m_pComponents->Has<T>(m_nID);
        }

        template <std::derived_from<Component::eXcomponent> T>
        void RemoveComponent()
        {
            if (m_pComponents != EXN_NULL_HANDLE)
                m_pComponents->Remove<T>(m_nID);
        }

    private:
        // The manager assigns the id and the registry when it stores the entity.
        template <std::derived_from<eXentity> T>
        friend class Manager;

        EXUINT m_nID = 0;
        Component::Registry *m_pComponents = EXN_NULL_HANDLE;
    };
};
