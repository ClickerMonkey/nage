#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <ranges>

// Strings aren't the best for games. Their data is scattered, divided, leaderless!
// Games like contiguous data with constant time operations. 
// Strings and maps aren't ideal, but humans like words to understand things.
// This package is an attempt to...
// - Pack all string data in one place
// - Convert string like data into a unique identifier (uint32)
// - Be able to convert that identifier back to a string
// - Provide a way to have maps with O(1) lookups
// - Control the performance and space usage of the maps for areas with similar data.
// This package will not...
// - Be a place to store all the character data. This is just for efficient string to data storage.
// - Promise constant time conversion from string like data into an int. 
//   This should be done as soon as possible, and the engine should use the uid.
// - Hold identifiers over PAGE_SIZE large. It will crash.
// - Validate you are using it correctly. This is not made for receiving unvalidated user input.
namespace id {
    using id_t = uint32_t;
    using storage_t = uint32_t;

    // id character data is stored in pages
    const id_t PAGE_SHIFT = 12; // 4096 page size
    const id_t PAGE_SIZE = 1 << PAGE_SHIFT;
    const id_t PAGE_MASK = PAGE_SIZE - 1; 

    // areas manage id mapping
    const size_t AREA_INITIAL_CAPACITY = 1024;
    const size_t AREA_AUTO_GROW_SHIFT = 7;
    const size_t AREA_AUTO_GROW = 1 << AREA_AUTO_GROW_SHIFT;

    // when a sparse map doesn't have space for a new entry, how much should we extend the underlying data by?
    const size_t SPARSE_MIN_GROW = 32;

    // a global map of const char* to the unique id.
    std::map<const char*, id_t> ids;
    // pages of where character data is stored for the ids.
    std::vector<const char*> pages;
    // the list of offsets into the pages given the unique id is the index into this vector.
    std::vector<storage_t> storage;
    // the end of the last id placed in storage.
    storage_t storageEnd;

    // Converts the unique id to the character data.
    const char* LookupChars(id_t uid) {
        auto pageIndex = storage[uid];
        auto page = pages[pageIndex >> PAGE_SHIFT];
        return page + (pageIndex & PAGE_MASK);
    }

    struct Identifier;
    const Identifier ID(const char* chars);
    const Identifier ID(std::string str);
    
    // An identifier simply holds an integer which uniquely identifies it.
    // It can be constructed from a constant or a string. There are utility
    // methods on it to convert it back to a `const char*` or a string.
    struct Identifier {
        id_t uid;

        // Creates an identifier with a known unique id.
        Identifier(id_t u): uid(u) {}
        // Creates an identifier given `const char*`, generating one and saving this data if required.
        Identifier(const char* chars): uid(ID(chars).uid) {} 
        // Creates an identifier given `std::string`, generating one and saving this data if required.
        Identifier(const std::string& str): uid(ID(str).uid) {}

        // Returns the chars for this identifier.
        inline const char* Chars() const noexcept { return LookupChars(uid); }
        // Returns the string of this identifier. This resolves in a copy
        // operation from the character storage and into a string instance.
        inline std::string String() const noexcept { return std::string(Chars()); }
        // Returns the char index in the character storage this identifier is stored.
        inline storage_t Storage() const noexcept { return storage[uid]; }
        // Returns the length of this identifier in characters.
        inline size_t Len() const noexcept { return strlen(Chars()); }

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

    const Identifier None(id_t(0));

    // Generates an Identifier for the given input. 
    // If the identifier already exists then it is returned.
    // Otherwise the data given here is copied into internal character data
    // storage and its referenced by the identifier that's returned.
    // This should be used sparingly since it involves a map lookup.
    // During the lifecycle of an application try to call this early on
    // and avoid it after that to get the best performance.
    const Identifier ID(const char* chars) {
        if (chars == nullptr || *chars == 0) {
            return None;
        }
        auto found = ids.find(chars);
        if (found == ids.end()) {
            // Include the terminating character
            auto n = strlen(chars) + 1;
            // Where the new end would be.
            auto end = storageEnd + n;
            // Where the max end is based on the pages allocated.
            auto pageEnd = pages.size() * PAGE_SIZE;
            // Do we need another page?
            if (end > pageEnd) {
                pages.emplace_back(new char[PAGE_SIZE]);
                storageEnd = pageEnd;
            }
            // Which page?
            auto pageIndex = storageEnd >> PAGE_SHIFT;
            // How many characters into the page?
            auto pageOffset = storageEnd & PAGE_MASK;
            // The location in memory to the start of the page.
            auto page = pages[pageIndex];
            // The location in memory of where the character data will be stored.
            auto pagePtr = (void*)(page + pageOffset);
            // Copy the data into character storage.
            memcpy(pagePtr, chars, n);

            // The next uuid is simply generated by looking at how many have been so far.
            auto uid = storage.size();

            // Add the place in storage where the character data will be stored.
            storage.push_back(storageEnd);
            // Add the map for later lookups of the same identifier.
            ids[chars] = uid;
            // Move the place to store the next identifier to after this one.
            storageEnd += n;

            return Identifier(uid);
        } else {
            return Identifier(found->second);
        }
    }

    // Converts a string to an Identifier. See the `const char*` documentation.
    const Identifier ID(std::string str) {
        return ID(str.c_str());
    }

    // An iterable list of all created identifiers.
    auto All = ids | 
        std::views::values | 
        std::views::transform(
            [](const id_t& uid) -> Identifier { 
                return Identifier(uid); 
            }
        );

    // An area produces a mapping from one identifier to another with the
    // assumption that the From space could have many identifiers spread out
    // and the To space will not have that many. This allows collection to use
    // the area id which may be considerably smaller. The largest an area id
    // can be is exactly how many unique values were passed to Translate.
    template<typename From, typename To>
    struct Area {
        std::vector<To> tos;
        To next;

        // Creates an area with the given initial capacity.
        Area(size_t initialCapacity): tos(initialCapacity), next(0) {}
        
        // Returns or generates the area id for the given unique id.
        To Translate(From from) {
            if (from >= tos.size()) {
                auto nextSize = ((from + AREA_AUTO_GROW) >> AREA_AUTO_GROW_SHIFT) << AREA_AUTO_GROW_SHIFT;
                tos.resize(nextSize);
            }
            if (tos[from] == 0) {
                tos[from] = ++next;
            }
            return tos[from] - 1;
        }
        // Returns true if the area has a translated area id for the given unique id.
        inline bool Has(From from) const {
            return from < tos.size() && tos[from] != 0;
        }
        // Returns the area id of the unique id or -1 if it hasn't been translated in the area yet.
        inline int Peek(From from) const noexcept {
            return int(from) < tos.size() ? int(tos[from]) - 1 : -1;
        }
    };

    // A sparse map is a map potentially with empty values scattered throughout.
    // There's no way to know if an identifier has been added to the map - but
    // this does provide a speed and memory conscious way of getting and setting by
    // identifier. The user is responsible for deciding if the value returned looks
    // set or not if the user cares to know (property on the value not being set, 
    // the values are pointers and its null, etc).
    template<typename V, typename sid_t>
    struct SparseMap {
        Area<id_t, sid_t>* area;
        std::vector<V> values;

        SparseMap(): area(nullptr), values() {}
        SparseMap(Area<id_t, sid_t>* customArea): area(customArea), values() {}

        inline void Set(Identifier id, V value) noexcept {
            Take(id) = std::move(value);
        }
        inline const V Get(Identifier id) const noexcept {
            auto mapID = area == nullptr ? id.uid : area->Peek(id.uid);
            if (mapID >= 0 && mapID < values.size()) {
                return values[mapID];
            }
            return V();
        }
        inline V Get(Identifier id) noexcept {
            auto p = Ptr(id);
            return p == nullptr ? V() : *p;
        }
        inline V* Ptr(Identifier id) noexcept {
            auto mapID = area == nullptr ? id.uid : area->Peek(id.uid);
            if (mapID >= 0 && mapID < values.size()) {
                return &values[mapID];
            }
            return nullptr;
        }
        inline V& Take(Identifier id) {
            auto mapID = area == nullptr ? id.uid : area->Translate(id.uid);
            if (mapID >= values.size()) {
                values.resize(mapID + SPARSE_MIN_GROW);
            }
            return values[mapID];
        }
        V& operator [](const char* chars) {
            return Take(chars);
        }
        V& operator [](const Identifier id) {
            return Take(id);
        }
        V& operator [](const std::string& str) {
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

    // A dense map has all values stored in initial set order in continguous memory.
    // It achieves this by having its own local area where it translates unique/area ids
    // to local ids (index into the values vector). When you need to store your data close
    // together and iterate only over the set values this is the map to use.
    template<typename V, typename aid_t, typename lid_t>
    struct DenseMap {
        Area<id_t, aid_t>* area;
        Area<aid_t, lid_t> local;
        std::vector<V> values;

        DenseMap(): area(nullptr), local(0), values() {}
        DenseMap(Area<id_t, aid_t>* customArea): area(customArea), local(0), values() {}

        inline void Set(Identifier id, V value) noexcept {
            Take(id) = std::move(value);
        }
        inline V Get(Identifier id) noexcept {
            auto p = Ptr(id);
            return p == nullptr ? V() : *p;
        }
        inline const V* Ptr(Identifier id) const noexcept {
            return Ptr(id);
        }
        inline V* Ptr(Identifier id) noexcept {
            auto areaID = area == nullptr ? id.uid : area->Peek(id.uid);
            if (areaID < 0) {
                return nullptr;
            }
            auto localID = local.Peek(areaID);
            if (localID < 0 || localID >- values.size()) {
                return nullptr;
            }
            return &values[localID];
        }
        inline V& Take(Identifier id) {
            auto areaID = area == nullptr ? id.uid : area->Translate(id.uid);
            auto localID = local.Translate(areaID);
            if (localID == values.size()) {
                values.emplace_back();
            }
            return values[localID];
        }
        inline std::vector<V>& Values() { return values; }
        V& operator [](const char* chars) {
            return Take(chars);
        }
        V& operator [](const Identifier id) {
            return Take(id);
        }
        V& operator [](const std::string& str) {
            return Take(str);
        }

        // TODO iterators
    };

    template<typename V>
    using DenseMap8 = DenseMap<V, id_t, uint8_t>;

    template<typename V>
    using DenseMap16 = DenseMap<V, id_t, uint16_t>;

    template<typename V>
    using DenseMap32 = DenseMap<V, id_t, uint32_t>;

    // TODO
    // - Change DenseMap to DenseValueMap since the keys are not available
    // - Add DenseMap which also stores keys
    // - Iterator for DenseMap pairs
    // - Iterator for DenseValueMap values
    // - Finish Documentation
    // - Performance testing and cmoparing to std::map<std::string,T>
    // - Refactor structs to classes
    
}