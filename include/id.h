#pragma once
#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <ranges>
#include <iterator>
#include <cstddef>

// Strings aren't the best for games. Their data is scattered, divided, leaderless!
// Games like contiguous data with constant time operations. 
// Strings and maps aren't ideal, but humans like words to understand things.
//
// This package is an attempt to...
// - Pack all string data in one place
// - Convert string like data into a unique identifier (uint32)
// - Be able to convert that identifier back to a string
// - Provide a way to have maps with O(1) lookups
// - Control the performance and space usage of the maps for areas with similar data.
//
// This package will not...
// - Be a place to store all the character data. This is just for efficient string to data storage.
// - Promise constant time conversion from string like data into an int. 
//   This should be done as soon as possible, and the engine should use the uid/Identifier in its class members.
// - Validate you are using it correctly. This is not made for receiving unvalidated user input.
// - Keep track of identifiers that are no longer need by the application. Once created it will exist for the duration of the application.
// 
// Some things to remember:
// - Anytime you have Identifier as a function parameter and a `std::string` or `const char*` is passed to 
//   the function it could result in the data generating a new identifier and saving those characters in memory for future reference. 
//   The only time this is not true is if a raw Identifier value is passed to the function that has an existing uid.
// - Anytime you create an identifier it will exist forever. Identifiers are for strings you expect to exist for the length of the game
//   or to be used so frequently its desirable to keep the character data in one place.
namespace id {
    using id_t = uint32_t;
    using storage_t = uint32_t;
    
    // id character data is stored in pages. The user can define this to override the default of 4096
    #ifndef PAGE_POWER
    #define PAGE_POWER 12
    #endif

    // uid 0 is an empty string.
    const char* empty = "";

    // Declare these ahead of time
    class Identifier;

    // Memory keeps all the character data and keeps track of all identifiers created.
    class Memory {
        // The power passed to memory to compute mask & shift for page related operations.
        const id_t m_pagePower;
        const id_t m_pageMask;
        const id_t m_pageSize;

        // a global map of const char* to the unique id.
        std::map<const char*, id_t> m_ids;
        // pages of where character data is stored for the ids.
        std::vector<const char*> m_pages;
        // the list of offsets into the pages given the unique id is the index into this vector.
        std::vector<storage_t> m_storage;
        // the end of the last id placed in storage.
        storage_t m_storageEnd;

    public:
        // Creates memory given a page size expressed as a power of 2 exponent.
        Memory(size_t pagePower): 
            m_pagePower(pagePower),
            m_pageMask((1 << pagePower) - 1),
            m_pageSize(1 << pagePower),
            m_ids(),
            m_pages(),
            m_storage(),
            m_storageEnd()
        {
            // Automatically add in identifier for null/empty identifiers.
            m_ids[empty] = 0;
            m_storage.push_back(0);
            m_storageEnd = 1;
            m_pages.emplace_back(new char[m_pageSize]);
            memcpy((void*)m_pages[0], (void*)empty, 1);
        }
        ~Memory() {
            for (auto page : m_pages) {
                delete(page);
            }
        }

        // Quick access to all defined identifiers.
        auto All();
    
        // Converts the unique id to the character data.
        // The unique id must be valid, otherwise unexpected behavior.
        const char* LookupChars(id_t uid) {
            auto pageIndex = m_storage[uid];
            auto page = m_pages[pageIndex >> m_pagePower];
            return page + (pageIndex & m_pageMask);
        }

        // Returns the unique id for the character data if it exists,
        // otherwise -1 is returned.
        const int32_t Peek(const char* chars) {
            if (chars == nullptr || *chars == '\0') {
                return 0;
            }
            auto found = m_ids.find(chars);
            return found == m_ids.end() ? -1 : found->second;
        }

        // Generates an Identifier for the given input. 
        // If the identifier already exists then it is returned.
        // Otherwise the data given here is copied into internal character data
        // storage and its referenced by the identifier that's returned.
        // This should be used sparingly since it involves a map lookup.
        // During the lifecycle of an application try to call this early on
        // and avoid it after that to get the best performance.
        const id_t Translate(const char* chars) {
            if (chars == nullptr || *chars == '\0') {
                return 0;
            }
            auto found = m_ids.find(chars);
            if (found == m_ids.end()) {
                // Include the terminating character
                auto n = strlen(chars) + 1;
                // Where the new end would be.
                auto end = m_storageEnd + n;
                // Where the max end is based on the pages allocated.
                auto pageEnd = m_pages.size() * m_pageSize;
                // Do we need another page?
                if (end > pageEnd) {
                    // This is required if they have an identifier over the page size.
                    // We create a special page size for it to avoid error.
                    auto pageSize = n > m_pageSize ? n : m_pageSize;

                    m_pages.emplace_back(new char[pageSize]);
                    m_storageEnd = pageEnd;
                }
                // Which page?
                auto pageIndex = m_storageEnd >> m_pagePower;
                // How many characters into the page?
                auto pageOffset = m_storageEnd & m_pageMask;
                // The location in memory to the start of the page.
                auto page = m_pages[pageIndex];
                // The location in memory of where the character data will be stored.
                auto pagePtr = (void*)(page + pageOffset);
                // Copy the data into character storage.
                memcpy(pagePtr, chars, n);

                // The next uuid is simply generated by looking at how many have been so far.
                auto uid = m_storage.size();

                // Add the place in storage where the character data will be stored.
                m_storage.push_back(m_storageEnd);
                // Add the map for later lookups of the same identifier.
                m_ids[chars] = uid;

                // Move the place to store the next identifier to after this one.
                // We compare n to page size in the event the identifier is larger. 
                // We store really large identifiers in a custom page size.
                m_storageEnd += n > m_pageSize ? m_pageSize : n;

                return uid;
            } else {
                return found->second;
            }
        }

        // Converts a string to an Identifier. See the `const char*` documentation.
        const id_t Translate(std::string str) {
            return Translate(str.c_str());
        }   

        // Converts a uid to a storage location.
        const storage_t Storage(id_t uid) {
            return m_storage[uid];
        }   
    };

    // Stores the character data.
    Memory memory(PAGE_POWER);
    
    // An identifier simply holds an integer which uniquely identifies it.
    // It can be constructed from a constant or a string. There are utility
    // methods on it to convert it back to a `const char*` or a string.
    class Identifier {
    public:
        // A unique id for a string. 
        // Two Identifiers initialized with the same characters will have the same uid.
        id_t uid;

        // Creates an identifier to an empty set of chars
        Identifier(): uid(0) {}
        // Creates an identifier with a known unique id.
        Identifier(id_t u): uid(u) {}
        // Creates an identifier given `const char*`, generating one and saving this data if required.
        Identifier(const char* chars): uid(memory.Translate(chars)) {} 
        // Creates an identifier given `std::string`, generating one and saving this data if required.
        Identifier(const std::string& str): uid(memory.Translate(str)) {}

        // Returns the chars for this identifier.
        inline const char* Chars() const noexcept { return memory.LookupChars(uid); }
        // Returns the string of this identifier. This resolves in a copy
        // operation from the character storage and into a string instance.
        inline std::string String() const noexcept { return std::string(Chars()); }
        // Returns the char index in the character storage this identifier is stored.
        inline storage_t Storage() const noexcept { return memory.Storage(uid); }
        // Returns the length of this identifier in characters.
        inline size_t Len() const noexcept { return strlen(Chars()); }
        // Returns true if this identifier is the nullptr/empty string.
        inline bool Empty() const noexcept { return uid == 0; }

        // Automatically cast an identifier to `const char*`.
        inline operator const char*() const noexcept { return Chars(); }
        // Automatically cast an identifier to `std::string`.
        inline operator const std::string() const noexcept { return String(); }

        // Comparison operator for identifiers.
        constexpr bool operator==(const Identifier& id) { return id.uid == uid; }

        // You can feed an identifier into an ostream and it sends the character data.
        friend std::ostream& operator<<(std::ostream& os, const Identifier& id) {
            os << id.Chars();
            return os;
        }
    };

    
    // A potential identifier. This does not create an identifier, but is a way to see
    // if an identifier has been defined, exists in a collection, etc without creating it.
    class IdentifierMaybe {
    public:
        // A unique id for a string or -1 if it doesn't exist.
        int32_t uid;

        // Creates an identifier to an empty set of chars
        IdentifierMaybe(): uid(0) {}
        // Creates an identifier with a known unique id.
        IdentifierMaybe(id_t u): uid(u) {}
        // Creates an identifier with a known unique id.
        IdentifierMaybe(Identifier id): uid(id.uid) {}
        // Creates an identifier given `const char*`, generating one and saving this data if required.
        IdentifierMaybe(const char* chars): uid(memory.Peek(chars)) {} 
        // Creates an identifier given `std::string`, generating one and saving this data if required.
        IdentifierMaybe(const std::string& str): uid(memory.Peek(str.c_str())) {}

        // If this points to an already defined identifier
        constexpr bool Exists() const noexcept { return uid != -1; }
        // Returns the chars for this identifier or a nullptr if the identifier does not exist.
        inline const char* Chars() const noexcept { return uid == -1 ? nullptr : memory.LookupChars(uid); }
        // Returns the string of this identifier. This resolves in a copy
        // operation from the character storage and into a string instance.
        inline std::string String() const noexcept { return std::string(Chars()); }
        // Returns the char index in the character storage this identifier is stored.
        inline int32_t Storage() const noexcept { return memory.Storage(uid); }
        // Returns the length of this identifier in characters.
        inline size_t Len() const noexcept { return strlen(Chars()); }

        // Automatically cast an identifier to `const char*`.
        inline operator const char*() const noexcept { return Chars(); }
        // Automatically cast an identifier to `std::string`.
        inline operator const std::string() const noexcept { return String(); }

        // Comparison operator for identifiers.
        constexpr bool operator==(const Identifier& id) { return id.uid == uid; }
        // Comparison operator for identifier maybe.
        constexpr bool operator==(const IdentifierMaybe& id) { return id.uid == uid; }

        // You can feed an identifier into an ostream and it sends the character data.
        friend std::ostream& operator<<(std::ostream& os, const IdentifierMaybe& id) {
            os << id.Chars();
            return os;
        }
    };

    // Quick access to all defined identifiers.
    auto Memory::All() {
        return m_ids | 
            std::views::values | 
            std::views::transform(
                [](const id_t& uid) -> Identifier { 
                    return Identifier(uid); 
                }
            );
    }

    // An iterable list of all created identifiers.
    inline auto All() {
        return memory.All();
    }

    // An area produces a mapping from one identifier to another with the
    // assumption that the From space could have many identifiers spread out
    // and the To space will not have that many. This allows collections to use
    // the area id which may be considerably smaller. The largest an area id
    // can be is exactly how many unique values were passed to Translate. At
    // the moment areas are used exclusively by maps to reduce their memory usage
    // while still offerring O(1) operations.
    template<typename From, typename To>
    class Area {
        // A translation from to tos where the from is the index into the vector.
        std::vector<To> m_tos;
        // The next to (area id)
        To m_next;
        // When we need to grow the internal translation buffer, how far should we go beyond the last request to reduce
        // to much resizing.
        size_t m_resizeBuffer;

    public:
        // Creates an area the grows only as much as it needs to and takes the minimum amount of memory.
        Area(): m_resizeBuffer(0), m_tos(0), m_next(0) {}
        // Creates an area with the given grow power and an initial capacity of 1^growPower.
        Area(size_t resizeBuffer): m_resizeBuffer(resizeBuffer), m_tos(resizeBuffer), m_next(0) {}
        // Creates an area with the given grow power & override initial capacity.
        Area(size_t resizeBuffer, size_t initialCapacity): m_resizeBuffer(resizeBuffer), m_tos(initialCapacity), m_next(0) {}
        
        // Returns or generates the area id for the given unique id.
        To Translate(From from) {
            if (from >= m_tos.size()) {
                auto nextSize = from + m_resizeBuffer + 1;
                m_tos.resize(nextSize);
            }
            if (m_tos[from] == 0) {
                m_tos[from] = ++m_next;
            }
            return m_tos[from] - 1;
        }
        // Returns true if the area has a translated area id for the given unique id.
        inline bool Has(From from) const {
            return from < m_tos.size() && m_tos[from] != 0;
        }
        // Returns the area id of the unique id or -1 if it hasn't been translated in the area yet.
        inline int Peek(From from) const noexcept {
            return int(from) < m_tos.size() ? int(m_tos[from]) - 1 : -1;
        }
        // Removes the From from the area, optionally maintaining the order
        // of the translation vector.
        // If order needs to be maintained:
        // - After this operation all tos > the the to mapped to the given from
        //   will have their tos be decreased by 1.
        // If order does not need to be maintained:
        // - After this operation only the returned to will need to will
        //   be what was removed infavor of the from.
        // If we didn't remove anything then -1 is returned.
        int Remove(From from, bool maintainOrder) {
            if (from >= m_tos.size() || m_tos[from] == 0) {
                return -1;
            }
            m_next--;
            auto removedTo = m_tos[from];
            m_tos[from] = 0;

            if (removedTo == m_next) {
                return int(removedTo) - 1;
            }

            if (maintainOrder) {
                for (auto& to : m_tos) {
                    if (to > removedTo) {
                        to--;
                    }
                }
            } else {
                for (int i = m_tos.size() - 1; i >= 0; --i) {
                    if (m_tos[i] == m_next) {
                        m_tos[i] = removedTo;
                        break;
                    }
                }
            }
            return int(removedTo) - 1;
        }
        // Clears out the area.
        void Clear() noexcept {
            m_tos.clear();
            m_next = 0;
        }
        // Returns the number of ids in this area.
        auto Size() const noexcept {
            return m_tos.size();
        }
        // Returns true if there are no ids in this area.
        auto Empty() const noexcept {
            return m_tos.empty();
        } 
    };

    // An area to the given type from a unique identifier.
    template<typename To>
    using BaseArea = Area<id_t, To>;

    // An area to use when you know there won't be any more than 255 unique identiers used by the maps this is passed to.
    using BaseArea8 = Area<id_t, uint8_t>;

    // An area to use when you know there won't be any more than 65,535 unique identiers used by the maps this is passed to.
    using BaseArea16 = Area<id_t, uint16_t>;

    // An area to use when you expect there will be more than 65,535 unique identifiers used by the maps this is passed to.
    // You can use this to be safe, but it only makes sense to use if you translate the most common identifiers in the area
    // first so they have lower area ids.
    using BaseArea32 = Area<id_t, uint16_t>;

    // A sparse map is a map potentially with empty values scattered throughout.
    // There's no way to know if an identifier has been added to the map - but
    // this does provide a speed and partially memory conscious way of getting and setting by
    // identifier. The user is responsible for deciding if the value returned looks
    // set or not if the user cares to know (property on the value not being set, 
    // the values are pointers and its null, etc).
    // There's no way to remove an identifier/value from this map.
    template<typename V, typename sid_t>
    class SparseMap {
        // An area to use for translation, otherwise use the raw unique id as the index into the values vector.
        // If no area is specified and there are many identifiers in the system then a sparse map
        // may have a values vector the size of the number of identifiers defined.
        Area<id_t, sid_t>* m_area;
        // A list of values by area id if it exists, otherwise by the unique id.
        std::vector<V> m_values;
        // When we need to resize the underlying buffer to fit a new value
        // how much should we extend beyond what we need to avoid future resizing?
        size_t m_resizeBuffer;

    public:
        // Creates an empty map that can grow based on the number of global identifiers.
        // It will only grow to fit the largest identifier used.
        SparseMap(): m_resizeBuffer(0), m_area(nullptr), m_values() {}
        // Creates an empty map with the given buffer to use during resize to avoid calculation. 
        // The map can still grow based on the number of global identifiers.
        SparseMap(size_t resizeBuffer): m_area(nullptr), m_resizeBuffer(resizeBuffer), m_values() {}
        // Creates an empty map that can grow based on the number of identifiers in the given area.
        // It will only grow to fit the largest area id used.
        SparseMap(Area<id_t, sid_t>* area): m_area(area), m_resizeBuffer(0), m_values() {}
        // Creates an empty map that can grow based on the number of identifiers in the given area using the given resize
        // buffer to reduce future resizings at the cost of potentially allocating a bit more memory.
        SparseMap(Area<id_t, sid_t>* area, size_t resizeBuffer): m_area(area), m_resizeBuffer(resizeBuffer), m_values() {}

        // Adds or updates the value in the map with the given identifier.
        inline void Set(Identifier id, V value) noexcept {
            Take(id) = std::move(value);
        }
        // Returns the const value at the given identifier or a value created using the empty constructor if it doesn't exist.
        inline const V Get(IdentifierMaybe id) const noexcept {
            if (id.Exists()) {
                auto mapID = m_area == nullptr ? id.uid : m_area->Peek(id.uid);
                if (mapID >= 0 && mapID < m_values.size()) {
                    return m_values[mapID];
                }
            }
            return V();
        }
        // Returns the value at the given identifier or a value created using the empty constructor if it doesn't exist.
        inline V Get(IdentifierMaybe id) noexcept {
            auto p = Ptr(id);
            return p == nullptr ? V() : *p;
        }
        // Returns the pointer of the value with the given identifier or null. The pointer returned
        // if non-null doesn't mean the value was actually added since this is a sparse map.
        inline V* Ptr(IdentifierMaybe id) noexcept {
            if (id.Exists()) {
                auto mapID = m_area == nullptr ? id.uid : m_area->Peek(id.uid);
                if (mapID >= 0 && mapID < m_values.size()) {
                    return &m_values[mapID];
                }
            }
            return nullptr;
        }
        // Returns or creates and returns the reference to the value with the given identifier.
        // The returned reference might point to a value that was never explicitly 
        // set on the map, consider Take a variation on Set.
        V& Take(Identifier id) {
            auto mapID = m_area == nullptr ? id.uid : m_area->Translate(id.uid);
            if (mapID >= m_values.size()) {
                m_values.resize(mapID + m_resizeBuffer + 1);
            }
            return m_values[mapID];
        }
        // Clears out the sparse map of all values.
        void Clear() noexcept {
            m_values.clear();
        }
        // Returns the maximum number of values in this map.
        auto Size() const noexcept {
            return m_values.size();
        }
        // Returns true if there are probably no values in this map.
        auto Empty() const noexcept {
            return m_values.empty();
        }
        // Access the reference to the value identified with the characters.
        // Using this may generate the identifier. 
        inline V& operator [](const char* chars) {
            return Take(chars);
        }
        // Access the reference to the value identified with the identifier.
        inline V& operator [](const Identifier id) {
            return Take(id);
        }
        // Access the reference to the value identified with the string.
        // Using this may generate the identifier.
        inline V& operator [](const std::string& str) {
            return Take(str);
        }
    };

    // A sparse map that uses an area that can fit no more than 2^8-1 identifiers.
    template<typename V>
    using SparseMap8 = SparseMap<V, uint8_t>;

    // A sparse map that uses an area that can fit no more than 2^16-1 identifiers.
    template<typename V>
    using SparseMap16 = SparseMap<V, uint16_t>;

    // A sparse map that uses an area that can fit no more than 2^32-1 identifiers.
    template<typename V>
    using SparseMap32 = SparseMap<V, uint32_t>;

    // A dense map has all values stored in initial set order in contiguous memory.
    // It achieves this by having its own local area where it translates unique/area ids
    // to local ids (index into the values vector). When you need to store your data close
    // together and iterate only over the set values this is the map to use.
    // Identifiers can be removed from this map, and optionally the order can be maintained
    // or it does not matter. A remove operation where order doesn't matter is O(1) where
    // a remove operation where order needs to be preserved is O(n) where n is the size
    // of the map.
    template<typename V, typename aid_t, typename lid_t>
    class DenseMap {
        // An area to use for translation, otherwise use the raw unique id as the id into the local area.
        // If no area is specified and there are many identifiers in the system then a dense map
        // may have a local area the size of the number of identifiers defined.
        Area<id_t, aid_t>* m_area;
        // A local area that maps area/unique ids to an index into the values vector.
        Area<aid_t, lid_t> m_local;
        // Where the value data is stored.
        std::vector<V> m_values;

    public:
        // Creates an empty map that can grow based on the number of global identifiers.
        // The local area will only grow to fit the largest identifier used.
        DenseMap(): m_area(nullptr), m_local(0), m_values() {}
        // Creates an empty map that can grow based on the number of identifiers in the given area.
        // The local area only grow to fit the largest area id used.
        DenseMap(Area<id_t, aid_t>* area): m_area(area), m_local(0), m_values() {}

        // The values in the map, added in order (unless a remove has been performed that didn't preserve order).
        inline std::vector<V>& Values() { return m_values; }

        // Adds or updates the value in the map with the given identifier.
        inline void Set(Identifier id, V value) noexcept {
            Take(id) = std::move(value);
        }
        // Returns the value at the given identifier or a value created using the empty constructor if it doesn't exist.
        inline V Get(IdentifierMaybe id) noexcept {
            auto p = Ptr(id);
            return p == nullptr ? V() : *p;
        }
        // Returns the pointer to value in the map set with the identifier.
        // If the identifier was never set or taken then nullptr is returned.
        V* Ptr(IdentifierMaybe id) noexcept {
            if (!id.Exists()) {
                return nullptr;
            }
            auto areaID = m_area == nullptr ? id.uid : m_area->Peek(id.uid);
            if (areaID < 0) {
                return nullptr;
            }
            auto localID = m_local.Peek(areaID);
            if (localID < 0 || localID >= m_values.size()) {
                return nullptr;
            }
            return &m_values[localID];
        }
        // Returns or creates and returns the reference to the value with the given identifier.
        // Consider Take a variation on Set.
        V& Take(Identifier id) {
            auto areaID = m_area == nullptr ? id.uid : m_area->Translate(id.uid);
            auto localID = m_local.Translate(areaID);
            if (localID == m_values.size()) {
                m_values.emplace_back();
            }
            return m_values[localID];
        }
        // Removes the value in the map with the given identifier.
        // If it does not exist false is returned.
        // You can control whether order in the map should be maintained or not.
        // When order should be maintained the operation will take O(n) where n
        // is the current size of the map. When order does not need to be maintained
        // it performs O(1).
        bool Remove(IdentifierMaybe id, bool maintainOrder) {
            if (!id.Exists()) {
                return false;
            }
            auto areaID = m_area == nullptr ? id.uid : m_area->Peek(id.uid);
            if (areaID == -1) {
                return false;
            }
            auto localID = m_local.Remove(areaID, maintainOrder);
            if (localID == -1) {
                return false;
            }
            if (maintainOrder) {
                m_values.erase(m_values.begin() + localID);
            } else {
                m_values[localID] = std::move(m_values.back());
                m_values.pop_back();
            }
            return true;
        }
        // Clears all values from this map.
        void Clear() noexcept {
            m_local.Clear();
            m_values.clear();
        }
        // Returns the number of values in this map.
        auto Size() const noexcept {
            return m_values.size();
        }
        // Returns true if there are no values in this map.
        auto Empty() const noexcept {
            return m_values.empty();
        } 
        // Access the reference to the value identified with the characters.
        // Using this may generate the identifier. 
        inline V& operator [](const char* chars) {
            return Take(chars);
        }
        // Access the reference to the value identified with the identifier.
        inline V& operator [](const Identifier id) {
            return Take(id);
        }
        // Access the reference to the value identified with the string.
        // Using this may generate the identifier.
        inline V& operator [](const std::string& str) {
            return Take(str);
        }
    
        // Returns iterators for all the values in the map.
        auto begin() { return m_values.begin(); }
        auto end()   { return m_values.end(); }
    };

    // A dense map that can fit no more than 2^8-1 values.
    template<typename V>
    using DenseMap8 = DenseMap<V, id_t, uint8_t>;

    // A dense map that can fit no more than 2^16-1 values.
    template<typename V>
    using DenseMap16 = DenseMap<V, id_t, uint16_t>;

    // A dense map that can fit no more than 2^32-1 values.
    template<typename V>
    using DenseMap32 = DenseMap<V, id_t, uint32_t>;


    // A dense key map has all keys & values stored in initial set order in contiguous memory.
    // It achieves this by having its own local area where it translates unique/area ids
    // to local ids (index into the values vector). When you need to store your data close
    // together and iterate only over the set values this is the map to use.
    // Identifiers can be removed from this map, and optionally the order can be maintained
    // or it does not matter. A remove operation where order doesn't matter is O(1) where
    // a remove operation where order needs to be preserved is O(n) where n is the size
    // of the map.
    template<typename V, typename aid_t, typename lid_t>
    class DenseKeyMap {
        // An area to use for translation, otherwise use the raw unique id as the id into the local area.
        // If no area is specified and there are many identifiers in the system then a dense map
        // may have a local area the size of the number of identifiers defined.
        Area<id_t, aid_t>* m_area;
        // A local area that maps area/unique ids to an index into the values vector.
        Area<aid_t, lid_t> m_local;
        // Where the value data is stored.
        std::vector<V> m_values;
        // The keys paired with the value
        std::vector<Identifier> m_keys;

    public:
        // Creates an empty map that can grow based on the number of global identifiers.
        // The local area will only grow to fit the largest identifier used.
        DenseKeyMap(): m_area(nullptr), m_local(0), m_values() {}
        // Creates an empty map that can grow based on the number of identifiers in the given area.
        // The local area only grow to fit the largest area id used.
        DenseKeyMap(Area<id_t, aid_t>* area): m_area(area), m_local(0), m_values() {}

        // The values in the map, added in order (unless a remove has been performed that didn't preserve order).
        inline std::vector<V>& Values() { return m_values; }

        // The keys in the map, added in order (unless a remove has been performed that didn't preserve order).
        inline std::vector<Identifier>& Keys() { return m_keys; }

        // Adds or updates the value in the map with the given identifier.
        inline void Set(Identifier id, V value) noexcept {
            Take(id) = std::move(value);
        }
        // Returns the value at the given identifier or a value created using the empty constructor if it doesn't exist.
        inline V Get(IdentifierMaybe id) noexcept {
            auto p = Ptr(id);
            return p == nullptr ? V() : *p;
        }
        // Returns the pointer to value in the map set with the identifier.
        // If the identifier was never set or taken then nullptr is returned.
        V* Ptr(IdentifierMaybe id) noexcept {
            if (!id.Exists()) {
                return nullptr;
            }
            auto areaID = m_area == nullptr ? id.uid : m_area->Peek(id.uid);
            if (areaID < 0) {
                return nullptr;
            }
            auto localID = m_local.Peek(areaID);
            if (localID < 0 || localID >= m_values.size()) {
                return nullptr;
            }
            return &m_values[localID];
        }
        // Returns or creates and returns the reference to the value with the given identifier.
        // Consider Take a variation on Set.
        V& Take(Identifier id) {
            auto areaID = m_area == nullptr ? id.uid : m_area->Translate(id.uid);
            auto localID = m_local.Translate(areaID);
            if (localID == m_values.size()) {
                m_values.emplace_back();
                m_keys.push_back(id);
            }
            return m_values[localID];
        }
        // Removes the value in the map with the given identifier.
        // If it does not exist false is returned.
        // You can control whether order in the map should be maintained or not.
        // When order should be maintained the operation will take O(n) where n
        // is the current size of the map. When order does not need to be maintained
        // it performs O(1).
        bool Remove(IdentifierMaybe id, bool maintainOrder) {
            if (!id.Exists()) {
                return false;
            }
            auto areaID = m_area == nullptr ? id.uid : m_area->Peek(id.uid);
            if (areaID == -1) {
                return false;
            }
            auto localID = m_local.Remove(areaID, maintainOrder);
            if (localID == -1) {
                return false;
            }
            if (maintainOrder) {
                m_values.erase(m_values.begin() + localID);
                m_keys.erase(m_keys.begin() + localID);
            } else {
                m_values[localID] = std::move(m_values.back());
                m_values.pop_back();
                m_keys[localID] = m_keys.back();
                m_keys.pop_back();
            }
            return true;
        }
        // Clears all keys and values from this map.
        void Clear() noexcept {
            m_local.Clear();
            m_values.clear();
            m_keys.clear();
        }
        // Returns the number of values in this map.
        auto Size() const noexcept {
            return m_values.size();
        }
        // Returns true if there are no values in this map.
        auto Empty() const noexcept {
            return m_values.empty();
        } 
        // Access the reference to the value identified with the characters.
        // Using this may generate the identifier. 
        inline V& operator [](const char* chars) {
            return Take(chars);
        }
        // Access the reference to the value identified with the identifier.
        inline V& operator [](const Identifier id) {
            return Take(id);
        }
        // Access the reference to the value identified with the string.
        // Using this may generate the identifier.
        inline V& operator [](const std::string& str) {
            return Take(str);
        }
    
        // Returns iterators for all the values in the map.
        auto begin() { return m_values.begin(); }
        auto end()   { return m_values.end(); }
    };

    // A dense key map that can fit no more than 2^8-1 values.
    template<typename V>
    using DenseKeyMap8 = DenseKeyMap<V, id_t, uint8_t>;

    // A dense key map that can fit no more than 2^16-1 values.
    template<typename V>
    using DenseKeyMap16 = DenseKeyMap<V, id_t, uint16_t>;

    // A dense key map that can fit no more than 2^32-1 values.
    template<typename V>
    using DenseKeyMap32 = DenseKeyMap<V, id_t, uint32_t>;

    // A unique unordered set of identifiers.
    // This will potentially grow to the larget identifier added to it (in bits).
    // This is ideal for holding a large number of identifiers and would perform better the longer
    // it exists given that the underlying data can be large if you have a high number of identifiers.
    class Set {
        // The presence of an identifier in the set is kept track of with a bit.
        std::vector<uint64_t> m_bits;

    public:
        // Creates an empty set that can grow based on the number of global identifiers.
        // The bit set will only grow to fit the largest identifier used.
        Set(): m_bits(1) {}
        // Creates an empty set that can grow based on the number of global identifiers.
        // The bit set will only grow to fit the largest identifier used.
        Set(size_t initialCapacity):  m_bits((initialCapacity >> 6) | 1) {}

        // Adds the given identifier to the set.
        void Add(Identifier id) {
            uint32_t bitBucket = id.uid >> 6;
            if (bitBucket >= m_bits.size()) {
                m_bits.resize(bitBucket + 1);
            }
            auto bitIndex = id.uid & 63;
            uint64_t bitShift = 1 << bitIndex;
            m_bits[bitBucket] |= bitShift;
        }
        // Returns true if the identifier exists in the set.
        bool Has(IdentifierMaybe id) {
            if (!id.Exists()) {
                return false;
            }
            uint32_t bitBucket = id.uid >> 6;
            if (bitBucket >= m_bits.size()) {
                return false;
            }
            auto bitIndex = id.uid & 63;
            uint64_t bitShift = 1 << bitIndex;
            return (m_bits[bitBucket] & bitShift) != 0;
        }
        // Removes the identifier from the map if it exists.
        void Remove(IdentifierMaybe id) {
            if (!id.Exists()) {
                return;
            }
            uint32_t bitBucket = id.uid >> 6;
            if (bitBucket >= m_bits.size()) {
                return;
            }
            auto bitIndex = id.uid & 63;
            uint64_t bitMask = ~(1 << bitIndex);
            m_bits[bitBucket] &= bitMask;
        }
        // Returns iterators for all the identifiers in the set.
        auto begin() { return Iterator(this); }
        auto end()   { return Iterator(-1); }
    private:
        int32_t OnAfter(int32_t index) {
            auto max = Max();
            uint32_t start = index + 1;
            if (start > uint32_t(max)) {
                return -1;
            }
            auto startIndex = start >> 6;
            auto startBits = start & 63;
            auto valueIndex = startIndex;
            auto value = m_bits[valueIndex];
            value &= ~((1 << startBits) - 1);
            auto valuesMax = m_bits.size();
            while (value == 0) {
                valueIndex++;
                if (valueIndex == valuesMax) {
                    return -1;
                }
                value = m_bits[valueIndex];
            }
            auto next = (valueIndex - startIndex) << 6;
            next += start;
            next += TrailingZeros(value);
            next -= startBits;
            return int32_t(next);
        }
        int32_t Max() {
            return (m_bits.size() << 6) - 1;
        }
        uint32_t TrailingZeros(uint64_t n) {
            uint32_t bits = 0;
            uint64_t x = n;
            if (x) {
                if (!(x & 0x00000000FFFFFFFF)) { bits += 32; x >>= 32; }
                if (!(x & 0x000000000000FFFF)) { bits += 16; x >>= 16; }
                if (!(x & 0x00000000000000FF)) { bits +=  8; x >>=  8; }
                if (!(x & 0x000000000000000F)) { bits +=  4; x >>=  4; }
                if (!(x & 0x0000000000000003)) { bits +=  2; x >>=  2; }
                bits += (x & 1) ^ 1;
            }
            return bits;
        }
        int32_t LastOn() {
            for (int i = m_bits.size() - 1; i >= 0; --i) {
                auto v = m_bits[i];
                if (v != 0) {
                    return int32_t((v << 6) + TrailingZeros(v));
                }
            }
            return -1;
        }
        
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = Identifier;
            using pointer           = Identifier;
            using reference         = Identifier;

            Set* m_set;
            int32_t m_uid;

            Iterator(Set* set): m_set(set), m_uid(set->OnAfter(-1)) {}
            Iterator(int32_t uid): m_set(nullptr), m_uid(uid) {}

            reference operator*() const { return Identifier(m_uid); }
            pointer operator->() { return Identifier(m_uid); }
            Iterator& operator++() { m_uid = m_set->OnAfter(m_uid); return *this; }  
            Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
            friend bool operator== (const Iterator& a, const Iterator& b) { return a.m_uid == b.m_uid; };
            friend bool operator!= (const Iterator& a, const Iterator& b) { return a.m_uid != b.m_uid; };              
        };
    };

    // A unique unordered set of identifiers tailored for a small number of identifiers.
    // The memory usage and performance of this set are relative to the number of unique identifiers in it.
    class SmallSet {
        // All identifiers are stored in a vector.
        std::vector<Identifier> m_ids;

    public:
        // Creates an empty set that can grow based on the number of global identifiers.
        // The bit set will only grow to fit the largest identifier used.
        SmallSet(): m_ids() {}

        // Adds the given identifier to the set.
        void Add(Identifier id) {
            if (!Has(id)) {
                m_ids.push_back(id);
            }
        }
        // Returns true if the identifier exists in the set.
        bool Has(IdentifierMaybe id) {
            if (!id.Exists()) {
                return false;
            }
            for (auto has : m_ids) {
                if (has.uid == id.uid) {
                    return true;
                }
            }
            return false;
        }   
        // Removes the identifier from the map if it exists.
        void Remove(IdentifierMaybe id) {
            if (!id.Exists()) {
                return;
            }
            auto itr = m_ids.begin();
            while (itr != m_ids.end()) {
                if (itr->uid == id.uid) {
                    m_ids.erase(itr);
                    break;
                }
            }
        }
        // Returns iterators for all the identifiers in the set.
        auto begin() { return m_ids.begin(); }
        auto end()   { return m_ids.end(); }
    };


}