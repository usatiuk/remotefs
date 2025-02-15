//
// Created by Stepan Usatiuk on 08.12.2024.
//

#ifndef SERIALIZABLESTRUCT_HPP
#define SERIALIZABLESTRUCT_HPP

#include <variant>
#include <vector>

#include "Serialize.hpp"

#define MAKE_FIELD(type, name) type name;

#define READ_FIELD(type, name) name = Serialize::deserialize<type>(in, end);

#define SERIALIZE_FIELD(type, name) Serialize::serialize(name, out);

#define COMPARE_FIELD(type, name) name == rhs.name&&

#define ARG_FIELD(type, name) type _##name,

#define CONSTRUCT_FIELD(type, name) name(std::move(_##name)),

#define DECLARE_SERIALIZABLE(name, FIELDS)                                                                             \
    class name {                                                                                                       \
    public:                                                                                                            \
        using serializable = std::true_type;                                                                           \
        FIELDS(MAKE_FIELD)                                                                                             \
                                                                                                                       \
        std::monostate _dummy;                                                                                         \
                                                                                                                       \
        explicit name(FIELDS(ARG_FIELD) std::monostate dummy = std::monostate{}) :                                     \
            FIELDS(CONSTRUCT_FIELD) _dummy(dummy) {}                                                                   \
                                                                                                                       \
        explicit name(std::vector<uint8_t>::const_iterator& in, const std::vector<uint8_t>::const_iterator& end) {     \
            FIELDS(READ_FIELD)                                                                                         \
        }                                                                                                              \
        void serialize(std::vector<uint8_t>& out) const { FIELDS(SERIALIZE_FIELD) }                                    \
                                                                                                                       \
        bool operator==(name const& rhs) const { return FIELDS(COMPARE_FIELD) true; }                                  \
                                                                                                                       \
    private:

#define DECLARE_SERIALIZABLE_END                                                                                       \
    }                                                                                                                  \
    ;


#endif // SERIALIZABLESTRUCT_HPP
