//
// ResourceManager.hpp
// Singleton tracking global resource pools (gold, oil, food, etc.)
//

#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <string>
#include <unordered_map>

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

        bool Spend(const std::string &type[], const float amounts[], int count) {
            // Check if all resources are sufficient
            for (int i = 0; i < count; ++i) {
                auto it = m_pools.find(type[i]);
                if (it == m_pools.end() || it->second < amounts[i]) return false;
            }
            // Deduct all resources
            for (int i = 0; i < count; ++i) {
                m_pools[type[i]] -= amounts[i];
            }
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

} // namespace gps

#endif // RESOURCE_MANAGER_HPP
