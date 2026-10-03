#pragma once

#include <concepts>
#include <unordered_map>
#include <utility>
#include <eXngine.h>
#include <component/component.h>

namespace eXngine::Component
{
    // Type-erased view of a Manager<T>, so the Registry can drop the components of an
    // entity without knowing their types.
    class IManager
    {
    public:
        virtual ~IManager() = default;

        virtual void Remove(EXUINT entityID) = 0;
        virtual void Truncate() = 0;
    };

    // Every component of type T, keyed by the id of the entity that owns it.
    // Pointers returned by Add/Get stay valid until that component is removed.
    template <std::derived_from<eXcomponent> T>
    class Manager : public IManager
    {
    private:
        std::unordered_map<EXUINT, T> m_Components;

    public:
        // Constructs T from args; replaces the T the entity already has.
        template <typename... Args>
        T *Add(EXUINT entityID, Args &&...args)
        {
            m_Components.erase(entityID);
            return &m_Components.try_emplace(entityID, std::forward<Args>(args)...).first->second;
        }

        void Remove(EXUINT entityID) override { m_Components.erase(entityID); }
        void Truncate() override { m_Components.clear(); }

        T *Get(EXUINT entityID)
        {
            const auto it = m_Components.find(entityID);
            return it != m_Components.end() ? &it->second : EXN_NULL_HANDLE;
        }

        EXBOOL Has(EXUINT entityID) const { return m_Components.contains(entityID); }
        EXSIZE GetCount() const { return static_cast<EXSIZE>(m_Components.size()); }

        // Range-for yields (entity id, component) pairs:
        //   for (auto &[id, transform] : manager) ...
        // Do not add or remove T components while iterating.
        auto begin() { return m_Components.begin(); }
        auto end() { return m_Components.end(); }
    };
}
