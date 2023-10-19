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

#include "core.h"

/**
 * The namespace that has the structures for defining types.
 * 
 * The templatized functions in this namespace are not necessary for runtime use - they exist to improve the 
 * development experience with the understanding there's a slight compilation cost associated with them.
 * 
 * A type has an int identifier (auto-incrementing value that may be different each application run), a name, a 
 * size and a few other functions to do basic operations like creating a Value of that type, converting to 
 * and from a string, the properties (actual, virtual, computed) that may exist on the Value, and collection related
 * functionality if the Value contains zero or more key-values (key can be index in an array or a key in a map).
 * 
 * A value has a type, a pointer or a shared value, and flags. The type must be non-null for the value to be valid. 
 * The pointer can point to the reference of a value or a shared pointer of a value. Flags keep track of whether its reference 
 * (setting it directly will update the value in memory) or a copy, if it represents a value that is read-only, and if the value
 * was casted to another type and writing to it may result in unexpected consequences.
 * 
 * A prop exists on a parent type and can return a reference to a property value, a copy, or it can set a property 
 * value.
 * 
 * Usage:
 * 1. Define all types with `auto MyType = types::New<My>("My")`.
 * 2. Provide a callable function which adds the definitions (props, collection types, metadata, castings).
 * 3. Before the type system is used, call the function which adds the definitions.
 */
namespace types {

    // TODO
    // - Add Next to Value which returns prop or value in collection.
    // - Add deep access get & set.
    // - Add strict checking with certain things are #define'd.
    // - Stop copying, refactor modifiers.
    // - Refactor Prop.
    // - Refactor TypeCollection.
    // - Documentation.
    // - Switch over strings to a tag.

    // Holds a value or a reference to a value of a given type.
    class Value;

    // A type defined in the application by the developer.
    class Type;

    // A key-value pair that can be removed.
    template<typename K, typename V>
    struct KeyValue {
        K key;
        V value;
        bool remove;
    };

    // A generic collection of key values.
    class Collection {
    public:
        using key_type = const Value&;
        using value_type = Value;
        using iter_type = KeyValue<key_type, value_type>;

        virtual value_type Get(key_type key) = 0;
        virtual bool Set(key_type key, value_type& value) = 0;
        virtual bool Add(key_type key, value_type& value) = 0;
        virtual bool Contains(key_type key) = 0;
        virtual size_t Size() const = 0;
        virtual void Iterate(std::function<bool(iter_type*)> fn) = 0;
    };

    const std::string& InvalidPropName = "<invalid>";
    
    // A property on a type.
    class Prop {
    public:
        using name_type = std::string;
        using get_type = std::function<Value(Value&)>;
        using ref_type = std::function<Value(Value&)>;
        using set_type = std::function<bool(Value&,Value&)>;
    
        Prop(): name(InvalidPropName) {}
        Prop(name_type n): name(n) {}

        name_type name;
        const Type* type;
        get_type get;
        ref_type ref;
        set_type set;
        bool isVirtual;

        bool isValid() { return name != InvalidPropName; }
    };

    const std::string getTypeName(const Type* t);
    const std::string getPropName(const Prop p);

    // A type family holds the initial type defined for a specific type <T> and all subsequent types
    // that use the same underlying type. 
    class TypeFamily {
    public:
        TypeFamily(): m_compatible(getTypeName, true, false), m_base(nullptr) {}

        constexpr const NameMap<Type*>& Compatible() const noexcept { return m_compatible; }
        constexpr Type* Base() const {
            if (m_base == nullptr) {
                throw std::invalid_argument("base type must be defined before the family is referenced");
            }
            return m_base;
        }
        void Add(Type* t) {
            if (m_base == nullptr) {
                m_base = t;
            }
            m_compatible.Add(t);
        }

    private:
        Type* m_base;
        NameMap<Type*> m_compatible;
    };

    // Returns the TypeFamily for <T>
    template<typename T>
    TypeFamily* FamilyFor() {
        static TypeFamily* family = nullptr;
        if (family == nullptr) {
            family = new TypeFamily();
        }
        return family;
    }

    // A definition if a type has a collection of key-values.
    struct TypeCollection {
        const Type* key;
        const Type* value;
        const std::function<std::shared_ptr<Collection>(Value&)> create;
    };

    // Helps define a type
    template<typename T>
    class Def;

    // Holds information about a type in the system.
    class Type {
    public:
        using id_type = const uint16_t;
        using name_type = std::string;
        using create_type = std::function<Value()>;
        using tostring_type = std::function<std::string(Value&)>;
        using fromstring_type = std::function<Value(const std::string&)>;
        using cast_type =  std::function<Value(const Value&)>;
        
        Type(id_type id, name_type name, const size_t size, const TypeFamily* family): 
            m_id(id), m_name(name), m_size(size), m_family(family), m_props(getPropName, true, true) {};
        
        auto Name() const noexcept { return m_name; }
        constexpr auto ID() const noexcept { return m_id; }
        constexpr auto Size() const noexcept { return m_size; }
        constexpr auto Family() const noexcept { return m_family; }
        constexpr auto IsBase() const noexcept { return this == m_family->Base(); }
        constexpr auto IsCompatible(const Type* t) const noexcept { return m_family == t->m_family; }
        constexpr auto IsCastCompatible(const Type *t) const noexcept { return m_size == t->m_size; }
        inline Value Create();
        inline std::string ToString(Value& value);
        inline Value FromString(const std::string& str);
        inline auto IsCollection() const noexcept { return !!m_collection; }
        const auto& Collection() const noexcept { return m_collection; }
        auto Props() const noexcept { return m_props; }
        auto GetCast(const Type* to) const {
            auto cast = m_casts.find(to->ID());
            return cast == m_casts.end() ? cast_type() : cast->second;
        }

        template<typename T>
        constexpr auto Is() const noexcept { return this->m_family == FamilyFor<T>(); }
        
        template<typename T>
        void Define(Def<T> def);

        template<typename T>
        Value New(const T value);

        template<typename T>
        friend class Def;

    private:
        id_type m_id;
        name_type m_name;
        const size_t m_size;
        const TypeFamily* m_family;
        NameMap<Prop> m_props;
        create_type m_create;
        tostring_type m_toString;
        fromstring_type m_fromString;
        std::map<id_type, cast_type> m_casts;
        std::unique_ptr<TypeCollection> m_collection;
    };

    const std::string getTypeName(const Type* t) { return t->Name(); }
    const std::string getPropName(const Prop p) { return p.name; }

    // Global types
    auto typeIds = new Incrementor<int>(0, 1);
    auto types = NameMap<Type*>(getTypeName, true, true);

    // Returns all defined types.
    NameMap<Type*>& Types() { return types; }
    
    // A value with a type and flags to track the value's state.
    // When passed as an argument pass as reference, when returned return by value. 
    // The outermost value last referenced will own any allocated data and will free out.
    class Value {
    public:
        using flags_type = uint8_t;
        using data_type  = void*;
        using copy_type = std::shared_ptr<void>;

        struct Flags {
            static constexpr Value::flags_type None      = 0b00000000; 
            static constexpr Value::flags_type ReadOnly  = 0b00000001; // Set will have no effect
            static constexpr Value::flags_type Reference = 0b00000010; // Value refers to a user-managed place in memory, setting will change it immediately
            static constexpr Value::flags_type Copy      = 0b00000100; // Value was heap-allocated and the data will be freed
            static constexpr Value::flags_type Cast      = 0b00001000; // Value was cast to a compatible type and is not the original
        };

        Value(): m_type(nullptr), m_ptr(nullptr), m_copy(), m_flags(Flags::None) {} 
        Value(Type* type, data_type data, flags_type flags = Flags::None): m_type(type), m_ptr(data), m_copy(), m_flags(flags | Flags::Reference) {}
        Value(Type* type, copy_type data, flags_type flags = Flags::None): m_type(type), m_ptr(nullptr), m_copy(std::move(data)), m_flags(flags | Flags::Copy) {}
        Value(const Value& other, flags_type addFlags): m_type(other.m_type), m_ptr(other.m_ptr), m_copy(other.m_copy), m_flags(other.m_flags | addFlags) {}
        Value(const Value& other) = default;

        constexpr auto IsReference() const noexcept { return (m_flags & Flags::Reference) != 0; }
        constexpr bool IsCopy() const noexcept { return (m_flags & Flags::Copy ) != 0; }
        constexpr bool IsCast() const noexcept { return (m_flags & Flags::Cast ) != 0; }
        constexpr auto IsValid() const noexcept { return m_type != nullptr && ptr() != nullptr; }
        inline auto IsCollection() const noexcept { return m_type->IsCollection(); }
        constexpr auto Data() const noexcept { return ptr(); }
        const auto GetType() const noexcept { return m_type; }
        constexpr auto Flags() const noexcept { return m_flags; }

        inline std::string ToString() noexcept { return m_type->ToString(*this); }
        
        auto Collection() noexcept {
            auto& c = m_type->Collection();
            return c ? c->create(*this) : nullptr;
        }

        explicit operator bool() const noexcept { return this->IsValid(); }

        template<typename T>
        explicit operator T() const noexcept { return this->Get<T>(); }

        Value ReadOnly() const {
            return Value(*this, Flags::ReadOnly);
        }

        Value StaticCast(const Type* otherType) const {
            auto caster = m_type->GetCast(otherType);
            if (caster) {
                return Value(caster(*this), Flags::Cast);
            }
            return Value::Invalid();
        }

        Value ReinterpretCast(Type* otherType) const noexcept {
            if (m_type->IsCastCompatible(otherType)) {
                if (IsReference()) {
                    return Value(otherType, m_ptr, m_flags | Flags::Cast);
                } else {
                    return Value(otherType, m_copy, m_flags | Flags::Cast);
                }
            }
            return Value::Invalid();
        }

        Value Prop(const Prop::name_type& name) {
            if (this->IsValid()) {
                const auto& prop = m_type->Props().Get(name);
                if (prop.ref) {
                    return prop.ref(*this);
                }
                if (prop.get) {
                    return prop.get(*this);
                }
            }
            return Value::Invalid();
        }

        template<typename T>
        bool Set(const T& v) const {
            auto p = ptr();
            if (p == nullptr) {
                return false;
            }
            if ((m_flags & Flags::ReadOnly) != 0) {
                return false;
            }
            if (!m_type->Is<T>()) {
                return false;
            }
            memcpy(p, &v, m_type->Size());
            // *((T*)m_data) = v;
            return true;
        }
        
        template<typename T>
        constexpr T* Ptr() const noexcept {
            auto p = ptr();
            if (p == nullptr) {
                return nullptr;
            }
            if (!m_type->Is<T>()) {
                return nullptr;
            }
            return (T*)p;
        }
        
        template<typename T>
        constexpr T Get() const noexcept {
            T* p = Ptr<T>();
            return p == nullptr ? T() : *p;
        }

        template<typename T>
        constexpr bool Is() const { return m_type->Is<T>(); }
        
        // Returns an Invalid value.
        static Value Invalid() noexcept { return Value(); }

    private:
        Type* m_type;
        data_type m_ptr;
        copy_type m_copy;
        flags_type m_flags;

        constexpr data_type ptr() const noexcept { 
            return m_ptr != nullptr ? m_ptr : m_copy.get(); 
        }
    };

    template<>
    bool Value::Set(const Value& v) const {
        if (!m_type->IsCompatible(v.m_type)) {
            return false;
        }
        memcpy(ptr(), v.ptr(), m_type->Size());
        return true;
    }

    template<typename T>
    Value ValueOf(T value, Type* specificType = nullptr) {
        if (specificType == nullptr) {
            specificType = FamilyFor<T>()->Base();
        }
        return Value(specificType, std::make_shared<T>(value), Value::Flags::Copy);
    }

    template<typename T>
    Value ValueTo(T* value, Type* specificType = nullptr) {
        if (specificType == nullptr) {
            specificType = FamilyFor<T>()->Base();
        }
        return Value(specificType, value, Value::Flags::Reference);
    }

    template<typename T>
    Type* New(Type::name_type name) {
        auto family = FamilyFor<T>();
        auto t = new Type(typeIds->Get(), name, sizeof(T), family);
    
        types.Add(t);
        family->Add(t);

        return t;
    }

    template<typename E>
    class VectorCollection : public Collection {
    public:
        VectorCollection(std::vector<E>* values, Type* value): 
            m_values(values), m_value(value) {}

        value_type Get(key_type key) {
            if (key.IsValid() && key.Is<int>()) {
                auto i = key.Get<int>();
                if (i >= 0 && i < m_values->size()) {
                    return ValueOf(m_values->at(i), m_value);
                }
            }
            return Value::Invalid();
        }
        bool Set(key_type key, value_type& value) {
            auto i = key.Get<int>();
            if (i < 0 || i >= m_values->size()) {
                m_values->resize(i+1);
            } 
            (*m_values)[i] = value.Get<E>();
            return true;
        }
        bool Add(key_type key, value_type& value) {
            if (!key.IsValid()) {
                m_values->push_back(value.Get<E>());
            } else if (key.Is<int>()) {
                auto index = key.Get<int>();
                if (index >= m_values->size()) {
                    m_values->resize(index+1);
                }
                (*m_values)[index] = value.Get<E>();
            } else {
                return false;
            }
            return true;
        }
        bool Contains(key_type key) {
            return key.IsValid() && key.Is<int>() && key.Get<int>() < m_values->size();
        }
        size_t Size() const {
            return m_values->size();
        }
        void Iterate(std::function<bool(iter_type*)> fn) {
            auto kv = iter_type{ValueOf(0), ValueOf<E>(E()), false};
            for (int i = 0; i < m_values->size(); i++) {
                kv.key.Set(i);
                kv.value.Set(m_values->at(i));
                kv.remove = false;
                auto stop = !fn(&kv);
                if (kv.remove) {
                    m_values->erase(m_values->begin()+i);
                    i--;
                }
                if (stop) {
                    break;
                }
            }
        }
    private:
        std::vector<E>* m_values;
        Type* m_value;
    };
    
    template<typename K, typename V>
    class MapCollection : public Collection {
    public:
        MapCollection(std::map<K, V>* values, Type* key, Type* value): 
            m_values(values), m_key(key), m_value(value) {}

        value_type Get(key_type key) {
            if (key.IsValid() && key.Is<K>()) {
                auto i = key.Get<K>();
                auto itr = m_values->find(i);
                if (itr != m_values->end()) {
                    return ValueOf(itr->second, m_value);
                }
            }
            return Value::Invalid();
        }
        bool Set(key_type key, value_type& value) {
            auto k = key.Get<K>();
            (*m_values)[k] = value.Get<V>();
            return true;
        }
        bool Add(key_type key, value_type& value) {
            if (key.IsValid() && key.Is<K>()) {
                auto k = key.Get<K>();
                (*m_values)[k] = value.Get<V>();
                return true;
            }
            return false;
        }
        bool Contains(key_type key) {
            return key.IsValid() && key.Is<K>() && m_values->find(key.Get<K>()) != m_values->end();
        }
        size_t Size() const {
            return m_values->size();
        }
        void Iterate(std::function<bool(iter_type*)> fn) {
            auto kv = iter_type{ValueOf(K(), m_key), ValueOf<V>(V(), m_value), false};
            for (auto& pair : m_values) {
                kv.key.Set(pair->first);
                kv.value.Set(pair->second);
                kv.remove = false;
                auto stop = !fn(&kv);
                if (kv.remove) {
                    m_values->erase(pair);
                }
                if (stop) {
                    break;
                }
            }
        }
    private:
        std::map<K, V>* m_values;
        Type* m_key;
        Type* m_value;
    };

    
    // The class that holds definition. It can be applied to more than one type, and the calls
    // can be chained. Each chained call returns a copy so it doesn't affect the previous calls.
    template<typename T>
    class Def {
    public:
        using apply_type = std::function<void(Type*)>;
        using prop_type = Prop;
        using collection_type = Collection;

        template<typename V>
        Def<T> Prop(Prop::name_type name, Type* type, std::function<V*(T*)> ref) {
            prop_type p(name);   
            p.type = type;
            p.ref = [ref, type](const Value& s) -> Value {
                auto sp = s.Ptr<T>();
                if (sp == nullptr ) {
                    return Value::Invalid();
                }
                return ValueTo(ref(sp), type);
            };
            p.get = [ref, type](const Value& s) -> Value {
                auto sp = s.Ptr<T>();
                if (sp == nullptr ) {
                    return Value::Invalid();
                }
                auto p = ref(sp);
                if (p == nullptr) {
                    return Value::Invalid();
                }
                return ValueOf<V>(*p, type);
            };
            p.set = [ref](const Value& s, const Value& v) -> bool {
                auto sp = s.Ptr<T>();
                if (sp == nullptr) {
                    return false;
                }
                auto p = ref(sp);
                if (p == nullptr) {
                    return false;
                }
                *p = v.Get<V>();
                return true;
            };

            return apply([p](Type* d) {
                d->m_props.Add(p);
            });
        }
        template<typename V>
        Def<T> Virtual(Prop::name_type name, Type *type, std::function<V(T*)> get, std::function<bool(T*,V)> set) {
            prop_type p(name);
            p.type = type;
            p.get = [get, type](const Value& s) -> Value {
                auto sp = s.Ptr<T>();
                if (sp == nullptr) {
                    return Value::Invalid();
                }
                return ValueOf<V>(get(sp), type);
            };
            p.set = [set](const Value& s, const Value& v) -> bool {
                auto sp = s.Ptr<T>();
                if (sp == nullptr) {
                    return false;
                }
                return set(sp, v.Get<V>());
            };
            p.isVirtual = true;

            return apply([p](Type* d) {
                d->m_props.Add(p);
            });
        }
        template<typename V>
        Def<T> Computed(Prop::name_type name, Type *type, std::function<V(T*)> get) {
            prop_type p(name);
            p.type = type;
            p.get = [get, type](const Value& s) -> Value {
                auto sp = s.Ptr<T>();
                if (sp == nullptr) {
                    return Value::Invalid();
                }
                return ValueOf<V>(get(sp), type);
            };
            p.isVirtual = true;

            return apply([p](Type* d) {
                d->m_props.Add(p);
            });
        }
        Def<T> DefaultCreate() {
            return apply([](Type* d) {
                d->m_create = [d]() -> Value {
                    return ValueOf<T>(T(), d);
                };
            });
        }
        Def<T> Create(std::function<T()> create) {
            return apply([create](Type* d) {
                d->m_create = [create, d]() -> Value {
                    return ValueOf<T>(create(), d);
                };
            });
        }
        Def<T> ToString(std::function<std::string(T)> toString) {
            return apply([toString](Type* d) {
                d->m_toString = [toString](Value& v) -> std::string {
                    return toString(v.Get<T>());
                };
            });
        }
        Def<T> FromString(std::function<T(const std::string&)> fromString) {
            return apply([fromString](Type* d) {
                d->m_fromString = [fromString, d](std::string s) -> Value {
                    return ValueOf<T>(fromString(s), d);
                };
            });
        }
        Def<T> Collection(TypeCollection collection) {
            return apply([collection](Type* d) {
                d->m_collection = std::make_unique<TypeCollection>(std::move(collection));
            });
        }
        template<typename E>
        Def<T> Vector(Type* key, Type* value, std::function<std::vector<E>*(T*)> get) {
            return apply([key, value, get](Type* d) {
                d->m_collection = std::make_unique<TypeCollection>(std::move(TypeCollection{
                    key,
                    value,
                    [value, get](Value& source) -> std::shared_ptr<collection_type> {
                        auto s = source.Ptr<T>();
                        if (s == nullptr) {
                            std::shared_ptr<collection_type> empty(nullptr);
                            return empty;
                        }
                        auto v = get(s);
                        if (v == nullptr) {
                            std::shared_ptr<collection_type> empty(nullptr);
                            return empty;
                        }
                        return std::make_shared<VectorCollection<E>>(v, value);
                    }
                }));
            });
        }
        template<typename K, typename V>
        Def<T> Map(Type* key, Type* value, std::function<std::map<K, V>*(T*)> get) {
            return apply([key, value, get](Type* d) {
                d->m_collection = std::make_unique<TypeCollection>(std::move(TypeCollection{
                    key,
                    value,
                    [key, value, get](Value& source) -> std::shared_ptr<collection_type> {
                        auto s = source.Ptr<T>();
                        if (s == nullptr) {
                            std::shared_ptr<collection_type> empty(nullptr);
                            return empty;
                        }
                        auto v = get(s);
                        if (v == nullptr) {
                            std::shared_ptr<collection_type> empty(nullptr);
                            return empty;
                        }
                        return std::make_shared<MapCollection<K, V>>(v, key, value);
                    }
                }));
            });
        }
        template<typename C>
        Def<T> Cast(Type* to, std::function<C(T)> cast) {
            return apply([to, cast](Type* d) {
                d->m_casts[to->ID()] = [to, cast](Value& src) -> Value {
                    return ValueOf<C>(cast(src.Get<T>()), to);
                };
            });
        }
        template<typename F>
        Def<F> For() {
            return Def<F>{m_appliers};
        }

        friend class Type;
    private:
        std::vector<apply_type> m_appliers;

        Def<T> apply(apply_type apply) {
            m_appliers.push_back(apply);
            return *this;
        }
    };

    // Defines the type, applying all chained definition calls given.
    template<typename T>
    void Type::Define(Def<T> def) {
        for (auto& a : def.m_appliers) {
            a(this);
        }
    }

    // Returns a new instance of this type.
    template<typename T>
    Value Type::New(const T value) {
        return ValueOf(std::move(value), this);
    }

    inline Value Type::Create() { return m_create(); }
    inline std::string Type::ToString(Value& value) { return m_toString ? m_toString(value) : ""; }
    inline Value Type::FromString(const std::string& str) { return m_fromString ? m_fromString(str) : Value(); }

    // A map of Type* to V
    template<typename V>
    class TypedMap {
    public:

        // Creates a new empty TypedMap.
        TypedMap(): m_values() {}

        // Gets the value stored for the given type. This is a temporary reference, it may point to invalid data
        // if any other values are added to the TypedMap.
        constexpr V* Get(const Type* type) { 
            return type->ID() >= m_values.size() ? nullptr : &m_values[type->ID()]; 
        }

        // Sets the value stored for the given type.
        constexpr void Set(const Type* type, V value) {
            auto i = type->ID();
            if (m_values.size() <= i) {
                m_values.resize(i + 1);
            }
            m_values[i] = std::move(value);
        }
    private:
        std::vector<V> m_values;
    };

    // Metadata storage for M.
    template<typename M>
    struct MetadataStorage {
        static TypedMap<M> m_types;
        static TypedMap<std::map<Prop::name_type, M>> m_props;
    };

    // Initialze metadata typed map.
    template<typename M> 
    TypedMap<M> MetadataStorage<M>::m_types = TypedMap<M>();
    template<typename M> 
    TypedMap<std::map<Prop::name_type, M>> MetadataStorage<M>::m_props = TypedMap<std::map<Prop::name_type, M>>();

    // Gets metadata of the given metadata type for the given type.
    template<typename M>
    M* GetTypeMeta(const Type* type) {
        return MetadataStorage<M>::m_types.Get(type);
    }

    // Gets metadata of the given metadata type for the given type.
    template<typename M>
    void SetTypeMeta(const Type* type, M meta) {
        MetadataStorage<M>::m_types.Set(type, std::move(meta));
    }

    // Gets metadata of the given metadata type for the given type.
    template<typename M>
    M* GetPropMeta(const Type* type, const Prop::name_type& property) {
        auto props = MetadataStorage<M>::m_props.Get(type);
        if (props == nullptr) {
            return nullptr;
        }
        auto m = props->find(property);
        return m == props->end() ? nullptr : &m->second;
    }

    // Gets metadata of the given metadata type for the given type.
    template<typename M>
    void SetPropMeta(const Type* type, const Prop::name_type& property, M meta) {
        auto props = MetadataStorage<M>::m_props.Get(type);
        if (props == nullptr) {
            MetadataStorage<M>::m_props.Set(type, std::map<Prop::name_type, M>());
            props = MetadataStorage<M>::m_props.Get(type);
        }
        props->emplace(property, std::move(meta));
    }
}