//
// ResourceManager.hpp
// Singleton tracking global resource pools (gold, oil, food, etc.)
//

#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <initializer_list>
#include <utility>

namespace gps {

    class ResourceManager {
    public:
        static ResourceManager& Instance() {
            static ResourceManager instance;
            return instance;
        }

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // Add amount to a resource pool
        void Deposit(const std::string& type, float amount) {
            m_pools[type] += amount;
        }

        // Spend amount — returns false if not enough
        bool Spend(const std::string& type, float amount) {
            auto it = m_pools.find(type);
            if (it == m_pools.end() || it->second < amount) return false;
            it->second -= amount;
            return true;
        }

        bool Spend(std::initializer_list<std::pair<std::string, float>> costs) {
            for (const auto& c : costs)           
                if (Get(c.first) < c.second) return false;
            for (const auto& c : costs)
                m_pools[c.first] -= c.second;
            return true;
        }

        // Get current amount (0 if unknown type)
        float Get(const std::string& type) const {
            auto it = m_pools.find(type);
            return (it != m_pools.end()) ? it->second : 0.0f;
        }

        // Set starting or override amount
        void Set(const std::string& type, float amount) {
            m_pools[type] = amount;
        }

        // Reset all pools
        void Reset() {
            m_pools.clear();
        }

    private:
        ResourceManager() = default;
        std::unordered_map<std::string, float> m_pools;
    };

}

#endif
