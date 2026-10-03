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
    void Manager<T>::AddEntity(const T &entity, const EXUINT id)
    {
        static EXUINT g_EntityID = 1;

        if (id != EXN_ENTITY_AUTO_ID)
            m_Entities[id] = entity;
        else
            m_Entities[g_EntityID++] = entity;
    }

    template <std::derived_from<eXentity> T>
    void Manager<T>::RemoveEntity(EXUINT id)
    {
        m_Entities.erase(id);
    }

    template <std::derived_from<eXentity> T>
    T *Manager<T>::GetEntity(EXUINT id)
    {
        return &m_Entities[id];
    }

    template <std::derived_from<eXentity> T>
    void Manager<T>::Truncate()
    {
        m_Entities.clear();
    }

    eXentity::eXentity() = default;
    eXentity::~eXentity() = default;

    // Members are defined here, so the manager the renderer uses must be instantiated here.
    template class Manager<eXentity>;
}