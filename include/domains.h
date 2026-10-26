#ifndef HOMM3_DOMAINS_H
#define HOMM3_DOMAINS_H

// One declaration, two expansions for semantic value domains.
//
// VC6 has no scoped enums or fixed underlying enum types. The matching build
// therefore retains each retail field, parameter, and return representation.
// A C++20 analysis pass instead gets scoped enums and width-preserving storage
// proxies, so a value cannot silently decay to an integer or another domain.
// `homm3 enums strict` is that analysis; its --reference run predefines
// H3_STRICT_ENUMS=0 to parse the plain expansion under the same front end.
#if defined(H3_STRICT_ENUMS)
#elif defined(__cplusplus) && __cplusplus >= 202002L
#define H3_STRICT_ENUMS 1
#else
#define H3_STRICT_ENUMS 0
#endif

#if H3_STRICT_ENUMS

#define H3_ENUM_BEGIN(name) enum class name : int {
#define H3_ENUM_END(name)                                                                          \
    }                                                                                              \
    ;                                                                                              \
    using enum name;

#define H3_ENUM_BEGIN_SPLIT(name, storage) enum class name : storage {
#define H3_ENUM_END_SPLIT(name, storage)                                                           \
    }                                                                                              \
    ;                                                                                              \
    using enum name;

#define H3_ENUM_FORWARD(name) enum class name : int
#define H3_ENUM_FORWARD_SPLIT(name, storage) enum class name : storage

// A value catalog: an ordinal or arithmetic domain whose values are read as
// numbers (mastery levels, counts). It stays unscoped, so it decays to int,
// but its fixed type still rejects integers and other domains flowing in.
#define H3_ENUM_VALUES_BEGIN(name) enum name : int {
#define H3_ENUM_VALUES_END(name) };

#define H3_ENUM_STORAGE(name, storage) H3EnumStorage<name, storage>
#define H3_ENUM_STORAGE_STEPPED(name, storage) H3SteppedEnumStorage<name, storage>
#define H3_ENUM_PARAM(name, storage) name
#define H3_ENUM_RETURN(name, storage) name
#define H3_ENUM_BITFIELD(name, storage) name

template<class Enum, class Storage> class H3SteppedEnumStorage;

template<class Enum, class Storage> class H3EnumStorage {
public:
    H3EnumStorage() = default;
    constexpr H3EnumStorage(Enum value) : m_value(static_cast<Storage>(value)) {}

    template<class OtherStorage>
    constexpr H3EnumStorage(H3EnumStorage<Enum, OtherStorage> value)
        : m_value(static_cast<Storage>(static_cast<Enum>(value))) {}

    template<class OtherStorage>
    constexpr H3EnumStorage(H3SteppedEnumStorage<Enum, OtherStorage> value)
        : m_value(static_cast<Storage>(static_cast<Enum>(value))) {}

    constexpr operator Enum() const { return static_cast<Enum>(m_value); }

    H3EnumStorage& operator=(Enum value) {
        m_value = static_cast<Storage>(value);
        return *this;
    }

    template<class OtherStorage>
    H3EnumStorage& operator=(H3EnumStorage<Enum, OtherStorage> value) {
        m_value = static_cast<Storage>(static_cast<Enum>(value));
        return *this;
    }

    template<class OtherStorage>
    H3EnumStorage& operator=(H3SteppedEnumStorage<Enum, OtherStorage> value) {
        m_value = static_cast<Storage>(static_cast<Enum>(value));
        return *this;
    }

private:
    Storage m_value;
};

template<class Enum, class Storage>
constexpr bool operator==(H3EnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) == right;
}

template<class Enum, class Storage>
constexpr bool operator==(Enum left, H3EnumStorage<Enum, Storage> right)
{
    return left == static_cast<Enum>(right);
}

template<class Enum, class Storage>
constexpr bool operator!=(H3EnumStorage<Enum, Storage> left, Enum right)
{
    return !(left == right);
}

template<class Enum, class Storage>
constexpr bool operator!=(Enum left, H3EnumStorage<Enum, Storage> right)
{
    return !(left == right);
}

template<class Enum, class Storage>
constexpr bool operator<(H3EnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) < right;
}

template<class Enum, class Storage>
constexpr bool operator<=(H3EnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) <= right;
}

template<class Enum, class Storage>
constexpr bool operator>(H3EnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) > right;
}

template<class Enum, class Storage>
constexpr bool operator>=(H3EnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) >= right;
}

template<class Enum, class LeftStorage, class RightStorage>
constexpr bool operator==(H3EnumStorage<Enum, LeftStorage> left,
    H3EnumStorage<Enum, RightStorage> right)
{
    return static_cast<Enum>(left) == static_cast<Enum>(right);
}

template<class Enum, class LeftStorage, class RightStorage>
constexpr bool operator!=(H3EnumStorage<Enum, LeftStorage> left,
    H3EnumStorage<Enum, RightStorage> right)
{
    return !(left == right);
}

template<class Enum, class Storage> class H3SteppedEnumStorage {
public:
    H3SteppedEnumStorage() = default;
    constexpr H3SteppedEnumStorage(Enum value)
        : m_value(static_cast<Storage>(value)) {}

    template<class OtherStorage>
    constexpr H3SteppedEnumStorage(H3EnumStorage<Enum, OtherStorage> value)
        : m_value(static_cast<Storage>(static_cast<Enum>(value))) {}

    template<class OtherStorage>
    constexpr H3SteppedEnumStorage(
        H3SteppedEnumStorage<Enum, OtherStorage> value)
        : m_value(static_cast<Storage>(static_cast<Enum>(value))) {}

    constexpr operator Enum() const { return static_cast<Enum>(m_value); }
    explicit constexpr operator bool() const { return m_value != 0; }

    H3SteppedEnumStorage& operator=(Enum value) {
        m_value = static_cast<Storage>(value);
        return *this;
    }

    template<class OtherStorage>
    H3SteppedEnumStorage& operator=(H3EnumStorage<Enum, OtherStorage> value) {
        m_value = static_cast<Storage>(static_cast<Enum>(value));
        return *this;
    }

    template<class OtherStorage>
    H3SteppedEnumStorage& operator=(
        H3SteppedEnumStorage<Enum, OtherStorage> value) {
        m_value = static_cast<Storage>(static_cast<Enum>(value));
        return *this;
    }

    H3SteppedEnumStorage& operator++() {
        ++m_value;
        return *this;
    }

    H3SteppedEnumStorage operator++(int) {
        H3SteppedEnumStorage previous = *this;
        ++m_value;
        return previous;
    }

    H3SteppedEnumStorage& operator--() {
        --m_value;
        return *this;
    }

    H3SteppedEnumStorage operator--(int) {
        H3SteppedEnumStorage previous = *this;
        --m_value;
        return previous;
    }

private:
    Storage m_value;
};

template<class Enum, class Storage>
constexpr bool operator==(H3SteppedEnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) == right;
}

template<class Enum, class Storage>
constexpr bool operator==(Enum left, H3SteppedEnumStorage<Enum, Storage> right)
{
    return left == static_cast<Enum>(right);
}

template<class Enum, class Storage>
constexpr bool operator!=(H3SteppedEnumStorage<Enum, Storage> left, Enum right)
{
    return !(left == right);
}

template<class Enum, class Storage>
constexpr bool operator!=(Enum left, H3SteppedEnumStorage<Enum, Storage> right)
{
    return !(left == right);
}

template<class Enum, class Storage>
constexpr bool operator<(H3SteppedEnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) < right;
}

template<class Enum, class Storage>
constexpr bool operator<=(H3SteppedEnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) <= right;
}

template<class Enum, class Storage>
constexpr bool operator>(H3SteppedEnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) > right;
}

template<class Enum, class Storage>
constexpr bool operator>=(H3SteppedEnumStorage<Enum, Storage> left, Enum right)
{
    return static_cast<Enum>(left) >= right;
}

template<class Enum, class Storage>
constexpr int H3EnumIndex(H3EnumStorage<Enum, Storage> value)
{
    return static_cast<int>(static_cast<Enum>(value));
}

template<class Enum, class Storage>
constexpr int H3EnumIndex(H3SteppedEnumStorage<Enum, Storage> value)
{
    return static_cast<int>(static_cast<Enum>(value));
}

template<class Enum> constexpr int H3EnumIndex(Enum value)
{
    static_assert(__is_enum(Enum),
        "H3_IDX/H3_AT require an enum-domain value");
    return static_cast<int>(value);
}

template<class Enum> constexpr Enum H3OffsetEnum(Enum value, int amount)
{
    return static_cast<Enum>(static_cast<int>(value) + amount);
}

template<class Enum, class Storage> constexpr Enum H3DecodeEnum(Storage value)
{
    return static_cast<Enum>(value);
}

#define H3_ENUM_STEPPED(name)                                                                      \
    inline constexpr name operator+(name value, int amount) {                                     \
        return H3OffsetEnum(value, amount);                                                        \
    }                                                                                              \
    inline constexpr name operator-(name value, int amount) {                                     \
        return H3OffsetEnum(value, -amount);                                                       \
    }                                                                                              \
    inline constexpr int operator-(name left, name right) {                                       \
        return static_cast<int>(left) - static_cast<int>(right);                                  \
    }                                                                                              \
    inline name& operator++(name& value) {                                                        \
        return value = value + 1;                                                                  \
    }                                                                                              \
    inline name operator++(name& value, int) {                                                    \
        name previous = value;                                                                     \
        ++value;                                                                                    \
        return previous;                                                                            \
    }                                                                                              \
    inline name& operator--(name& value) {                                                        \
        return value = value - 1;                                                                  \
    }                                                                                              \
    inline name operator--(name& value, int) {                                                    \
        name previous = value;                                                                     \
        --value;                                                                                    \
        return previous;                                                                            \
    }

// A flag domain: its enumerators are bits and its values are their unions.
// The strict audit gives it the bitwise operators (result: the same domain);
// H3_HAS tests a bit in a boolean context.
#define H3_ENUM_FLAGS(name)                                                                        \
    inline constexpr name operator|(name left, name right) {                                      \
        return static_cast<name>(static_cast<__underlying_type(name)>(left)                        \
                                 | static_cast<__underlying_type(name)>(right));                   \
    }                                                                                              \
    inline constexpr name operator&(name left, name right) {                                      \
        return static_cast<name>(static_cast<__underlying_type(name)>(left)                        \
                                 & static_cast<__underlying_type(name)>(right));                   \
    }                                                                                              \
    inline constexpr name operator^(name left, name right) {                                      \
        return static_cast<name>(static_cast<__underlying_type(name)>(left)                        \
                                 ^ static_cast<__underlying_type(name)>(right));                   \
    }                                                                                              \
    inline constexpr name operator~(name value) {                                                 \
        return static_cast<name>(~static_cast<__underlying_type(name)>(value));                    \
    }                                                                                              \
    inline name& operator|=(name& left, name right) { return left = left | right; }               \
    inline name& operator&=(name& left, name right) { return left = left & right; }               \
    inline name& operator^=(name& left, name right) { return left = left ^ right; }

#else

#define H3_ENUM_BEGIN(name) enum name {
#define H3_ENUM_END(name) };
#define H3_ENUM_BEGIN_SPLIT(name, storage) enum name {
#define H3_ENUM_END_SPLIT(name, storage) };
#define H3_ENUM_FORWARD(name) enum name
#define H3_ENUM_FORWARD_SPLIT(name, storage) enum name
#define H3_ENUM_VALUES_BEGIN(name) enum name {
#define H3_ENUM_VALUES_END(name) };

#define H3_ENUM_STORAGE(name, storage) storage
#define H3_ENUM_STORAGE_STEPPED(name, storage) storage
#define H3_ENUM_PARAM(name, storage) storage
#define H3_ENUM_RETURN(name, storage) storage
#define H3_ENUM_BITFIELD(name, storage) storage

#define H3_ENUM_STEPPED(name)
#define H3_ENUM_FLAGS(name)

#endif

#if H3_STRICT_ENUMS
#define H3_AT(array, index) (array)[H3EnumIndex(index)]
#define H3_IDX(value) H3EnumIndex(value)
#define H3_ENUM_DECODE(name, value) H3DecodeEnum<name>(value)
#define H3_HAS(flags, bit) (H3EnumIndex((flags) & (bit)) != 0)
#else
#define H3_AT(array, index) array[index]
#define H3_IDX(value) value
#define H3_ENUM_DECODE(name, value) (name)(value)
#define H3_HAS(flags, bit) ((flags) & (bit))
#endif

#endif
