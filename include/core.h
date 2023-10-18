#pragma once
#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <algorithm>
#include <functional>
#include <cstdio>
#include <vector>
#include <memory>
#include <variant>

#include "debug.h"

// TODO
// - Finish Documentation
// - Add appropriate modifiers

// Auto incrementing value each time Get() is invoked.
template<typename T>
class Incrementor {
public:
    Incrementor(T start, T increment): m_current(start), m_increment(increment) {};

    constexpr const T Get() noexcept { 
        auto i = m_current; 
        m_current += m_increment; 
        return i; 
    }
    constexpr const T Peek() const noexcept { 
        return this->current; 
    }
private:
    T m_current;
    T m_increment;
};

// Converts a string into lowercase - so type & prop names can be retrieved with any casing.
const std::string lowercase(std::string x) {
    transform(x.begin(), x.end(), x.begin(), ::tolower);
    return x;
}

// A map & list combo for named values. A list for fast in-order traversal and a map for infrequent return by name.
// Not required for the type system, but a useful type to store all types & store all the props for a type.
template<typename T>
class NameMap {
public:
    using name_type = std::string;
    using nameof_type = std::function<const name_type(const T&)>;
    using iterator_type = typename std::vector<T>::iterator;
    
    NameMap(const nameof_type& nameOf, bool insensitive, bool ordered):
        m_nameOf(nameOf), m_insensitive(insensitive), m_ordered(ordered) {}
    
    // Returns the index of the value with the given name or -1 if a value with that name does not exist.
    constexpr int IndexOf(const name_type& name) const noexcept { return indexOf(toKey(name)); }
    // Returns whether a value exists in this map with the given name.
    constexpr bool Has(const name_type& name) const noexcept { return IndexOf(name) != -1; }
    // Returns the number of items in the map.
    constexpr int Size() const noexcept { return m_values.size(); }
    // Adds a named value to the map and returns true on success. False is returned
    // if a value with the given name already exists.
    bool Add(const T& value) {
        auto key = keyOf(value);
        auto missing = !m_indices.count(key);
        if (missing) {
            auto i = m_values.size();
            m_values.push_back(value);
            m_indices[key] = i;
        }
        return missing;
    }
    // Sets the named value to the map whether it exists or not. If it exists
    // already the existing value is replaced.
    void Set(const T& value) {
        auto key = keyOf(value);
        auto index = indexOf(key);
        if (index != -1) {
            m_values[index] = value;
        } else {
            auto i = m_values.size();
            m_values.push_back(value);
            m_indices[key] = i;
        }
    }
    // Removes the value from the map.
    constexpr bool Remove(const T& value) noexcept {
        return RemoveName(keyOf(value));
    }
    // Removes the value with the name from the map.
    bool RemoveName(const name_type& name) noexcept {
        auto key = toKey(name);
        auto index = indexOf(key);
        auto exists = index != -1;
        if (exists) {
            auto last = m_values.size() - 1;
            auto notLast = index < last;
            if (m_ordered && !notLast) {
                m_values.erase(m_values.begin() + index);
                m_indices.erase(key);
                for (auto& pair : m_indices) {
                    if (pair.second > index) {
                        pair.second--;
                    }
                }
            } else {
                if (!notLast) {
                    auto lastKey = keyOf(m_values[last]);
                    m_indices[lastKey] = index;
                    m_values[index] = m_values[last];
                }
                m_values.pop_back();
                m_indices.erase(key);
            }
        }
        return exists;
    }
    // Gets a value with the name from the map and whether it exists at all.
    constexpr T Get(const name_type& name) const noexcept {
        auto key = toKey(name);
        auto index = indexOf(key);
        if (index != -1) {
            return m_values[index];
        }
        return T();
    }
    // Clears the map of all values.
    void Clear() noexcept {
        m_values.clear();
        m_indices.clear();
    }
    // Handles the value being renamed given the old name. If the old name
    // does not exist a rebuild is performed.
    void Rename(name_type oldName) noexcept {
        auto oldKey = toKey(oldName);
        auto index = indexOf(oldKey);
        if (index != -1) {
            auto value = m_values[index];
            auto newKey = keyOf(value);
            m_indices.erase(oldKey);
            m_indices[newKey] = index;
        } else {
            Rebuild();
        }
    }
    // Handles the value being renamed given the old name. If the old name
    // does not exist a rebuild is performed.
    void Rebuild() noexcept {
        m_indices.clear();
        for (int i = 0; i < m_values.size(); i++) {
            auto key = keyOf(m_values[i]);
            m_indices[key] = i;
        }
    }

    // Iteration
    iterator_type begin() noexcept { return m_values.begin(); }
    iterator_type end() noexcept { return m_values.end(); }
private:
    std::vector<T> m_values;
    std::map<name_type, int> m_indices;
    nameof_type m_nameOf;
    bool m_insensitive;
    bool m_ordered;

    // Converts the name to a key used in the index map.
    constexpr const name_type toKey(const name_type& x) const noexcept {
        if (m_insensitive) {
            return lowercase(x);
        }
        return x;
    }
    // Converts the value to a key used in the index map.
    constexpr const name_type keyOf(const T& item) const noexcept {
        auto name = m_nameOf(item);
        return toKey(name);
    }
    constexpr int indexOf(const name_type& key) const noexcept {
        auto it = m_indices.find(key);
        return it != m_indices.end() ? it->second : -1;
    }
};