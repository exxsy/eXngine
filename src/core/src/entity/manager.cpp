#include <entity/manager.h>

namespace eXngine::Entity
{
    template <std::derived_from<eXentity> T>
    Manager<T>::Manager()
    {
        this->m_Entities.reserve(EXN_ENTITY_CAPACITY);
    }

    template <std::derived_from<eXentity> T>
    Manager<T>::~Manager()
    {
        Truncate();
    }

    template <std::derived_from<eXentity> T>
    T *Manager<T>::AddEntity(const T &entity, const EXUINT id)
    {
        EXUINT entityID = id;

        if (entityID == EXN_ENTITY_AUTO_ID)
        {
            // Skip the ids that were given explicitly.
            while (m_Entities.contains(m_nNextID))
                ++m_nNextID;

            entityID = m_nNextID++;
        }
        else
        {
            m_Components.RemoveAll(entityID);
        }

        T &stored = m_Entities.insert_or_assign(entityID, entity).first->second;
        stored.m_nID = entityID;
        stored.m_pComponents = &m_Components;

        return &stored;
    }

    template <std::derived_from<eXentity> T>
    void Manager<T>::RemoveEntity(EXUINT id)
    {
        m_Components.RemoveAll(id);
        m_Entities.erase(id);
    }

    template <std::derived_from<eXentity> T>
    T *Manager<T>::GetEntity(EXUINT id)
    {
        const auto it = m_Entities.find(id);
        return it != m_Entities.end() ? &it->second : EXN_NULL_HANDLE;
    }

    template <std::derived_from<eXentity> T>
    void Manager<T>::Truncate()
    {
        m_Components.Truncate();
        m_Entities.clear();
    }

    eXentity::eXentity() = default;
    eXentity::~eXentity() = default;

    // Members are defined here, so the manager the renderer uses must be instantiated here.
    template class Manager<eXentity>;
}
