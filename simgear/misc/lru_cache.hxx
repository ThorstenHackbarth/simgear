// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
// SPDX-FileCopyrightText: 2019 Richard Harrison <rjh@zaretto.com>

//------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//
// Changes Copyright (C) 2019  Richard Harrison (rjh@zaretto.com)
//
// As the boost licence is lax and permissive see
// (https://www.gnu.org/licenses/license-list.en.html#boost)
// any changes to this module are covered under the GPL

#ifndef LRU_CACHE_HXX_
#define LRU_CACHE_HXX_

#include <algorithm>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <ranges>
#include <simgear/threads/SGThread.hxx>
#include <utility>

namespace simgear
{
    // Thread-safe LRU cache that evicts the least-recently-used item when full.
    template<class Key, class Value>
    class lru_cache final
    {
    public:
        std::mutex _mutex;

        using key_type   = Key;
        using value_type = Value;
        using list_type  = std::list<key_type>;
        using map_type   = std::map<key_type,
                                    std::pair<value_type, typename list_type::iterator>>;

        explicit lru_cache(size_t capacity) : m_capacity(capacity) {}

        ~lru_cache() = default;

        [[nodiscard]] size_t size() const     { return m_map.size(); }
        [[nodiscard]] size_t capacity() const { return m_capacity; }
        [[nodiscard]] bool   empty() const    { return m_map.empty(); }

        bool contains(const key_type& key)
        {
            std::lock_guard lock(_mutex);
            return m_map.contains(key);
        }

        void insert(const key_type& key, const value_type& value)
        {
            std::lock_guard lock(_mutex);
            if (m_map.contains(key))
                return;
            if (size() >= m_capacity)
                evict();
            m_list.push_front(key);
            m_map.emplace(key, std::pair{value, m_list.begin()});
        }

        // Linear scan mapping a value back to its key.
        std::optional<key_type> findValue(const value_type& requiredValue)
        {
            std::lock_guard lock(_mutex);
            auto it = std::ranges::find_if(m_map, [&](const auto& entry) {
                return entry.second.first == requiredValue;
            });
            if (it != m_map.end())
                return it->first;
            return std::nullopt;
        }

        std::optional<value_type> get(const key_type& key)
        {
            std::lock_guard lock(_mutex);
            auto i = m_map.find(key);
            if (i == m_map.end())
                return std::nullopt;

            auto j = i->second.second;
            if (j != m_list.begin()) {
                m_list.splice(m_list.begin(), m_list, j);
                i->second.second = m_list.begin();
            }
            return i->second.first;
        }

        void clear()
        {
            std::lock_guard lock(_mutex);
            m_map.clear();
            m_list.clear();
        }

    private:
        // Called with _mutex already held.
        void evict()
        {
            auto it = std::prev(m_list.end());
            m_map.erase(*it);
            m_list.erase(it);
        }

        map_type  m_map;
        list_type m_list;
        size_t    m_capacity;
    };

} // namespace simgear

#endif // LRU_CACHE_HXX_
