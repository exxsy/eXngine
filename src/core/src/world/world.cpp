#include <world/world.h>

namespace eXngine::World
{
    eXworld::eXworld() = default;

    eXworld::~eXworld()
    {
        // Systems may own things tied to entities (e.g. draw commands): detach them,
        // newest first, while the entities still exist.
        while (!m_Systems.empty())
        {
            m_Systems.back().second->OnDetach(*this);
            m_Systems.pop_back();
        }

        Clear();
    }

    eXworld::EntityType *eXworld::CreateEntity(const std::string &name)
    {
        EntityType *entity = m_Entities.AddEntity();
        entity->AddComponent<Component::eXnameComponent>(name);

        return entity;
    }

    void eXworld::DestroyEntity(EXUINT id)
    {
        // A system may be iterating over the components of this entity right now.
        if (m_bUpdating)
            m_PendingDestroy.push_back(id);
        else
            m_Entities.RemoveEntity(id);
    }

    eXworld::EntityType *eXworld::GetEntity(EXUINT id)
    {
        return m_Entities.GetEntity(id);
    }

    eXworld::EntityType *eXworld::FindEntity(const std::string &name)
    {
        for (auto &[id, component] : GetComponents<Component::eXnameComponent>())
        {
            if (component.Name == name)
                return GetEntity(id);
        }

        return EXN_NULL_HANDLE;
    }

    EXSIZE eXworld::GetEntityCount() const
    {
        return m_Entities.GetCount();
    }

    void eXworld::Clear()
    {
        EX_FATAL(!m_bUpdating, "The world cannot be cleared while it is updating.");

        m_Entities.Truncate();
    }

    void eXworld::Update(EXFLOAT deltaTime)
    {
        m_bUpdating = true;

        for (auto &[type, system] : m_Systems)
            system->OnUpdate(*this, deltaTime);

        m_bUpdating = false;

        for (const EXUINT id : m_PendingDestroy)
            m_Entities.RemoveEntity(id);

        m_PendingDestroy.clear();
    }
}
