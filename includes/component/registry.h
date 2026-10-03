#pragma once

#include <concepts>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <eXngine.h>
#include <component/component.h>
#include <component/manager.h>

namespace eXngine::Component
{
    // Owns one Manager<T> per component type, created the first time a T is added.
    // Types are keyed by std::type_index rather than a per-type static counter: a static
    // would be duplicated in every DLL/EXE, while type_info is compared and hashed by
    // its decorated name, so every module finds the same manager.
    class Registry
    {
    private:
        std::unordered_map<std::type_index, std::unique_ptr<IManager>> m_Managers;

        // Null until a T has been added: lookups must not create managers.
        template <std::derived_from<eXcomponent> T>
        Manager<T> *FindManager() const
        {
            const auto it = m_Managers.find(typeid(T));
            return it != m_Managers.end() ? static_cast<Manager<T> *>(it->second.get()) : EXN_NULL_HANDLE;
        }

    public:
        Registry() = default;
        Registry(const Registry &) = delete;
        Registry &operator=(const Registry &) = delete;

        template <std::derived_from<eXcomponent> T>
        Manager<T> &GetManager()
        {
            std::unique_ptr<IManager> &manager = m_Managers[typeid(T)];

            if (!manager)
                manager = std::make_unique<Manager<T>>();

            return static_cast<Manager<T> &>(*manager);
        }

        template <std::derived_from<eXcomponent> T, typename... Args>
        T *Add(EXUINT entityID, Args &&...args)
        {
            return GetManager<T>().Add(entityID, std::forward<Args>(args)...);
        }

        template <std::derived_from<eXcomponent> T>
        T *Get(EXUINT entityID) const
        {
            Manager<T> *manager = FindManager<T>();
            return manager != EXN_NULL_HANDLE ? manager->Get(entityID) : EXN_NULL_HANDLE;
        }

        template <std::derived_from<eXcomponent> T>
        EXBOOL Has(EXUINT entityID) const
        {
            const Manager<T> *manager = FindManager<T>();
            return manager != EXN_NULL_HANDLE && manager->Has(entityID);
        }

        template <std::derived_from<eXcomponent> T>
        void Remove(EXUINT entityID)
        {
            if (Manager<T> *manager = FindManager<T>())
                manager->Remove(entityID);
        }

        // Removes every component of the entity, whatever its type.
        void RemoveAll(EXUINT entityID)
        {
            for (auto &[type, manager] : m_Managers)
                manager->Remove(entityID);
        }

        void Truncate()
        {
            for (auto &[type, manager] : m_Managers)
                manager->Truncate();
        }
    };
}
