//
// Created by Stepan Usatiuk on 08.12.2024.
//

#include <gtest/gtest.h>

#include "Serialize.hpp"
#include "SerializableStruct.hpp"

#define TEST_STRUCT(FIELD)                                                                                             \
    FIELD(long, _test2)                                                                                                \
    FIELD(std::string, _test_str)                                                                                      \
    FIELD(std::vector<uint8_t>, _test_vec)

DECLARE_SERIALIZABLE(TestStruct, TEST_STRUCT)
DECLARE_SERIALIZABLE_END

TEST(SerializableHelperTestStruct, Works) {
    TestStruct test(123, "hello", {});

    std::vector<uint8_t> out;
    test.serialize(out);

    TestStruct deserialized = Serialize::deserialize<TestStruct>(out);

    ASSERT_EQ(test, deserialized);
}

TEST(SerializableHelperTestStruct, Variant) {
    using VarT = std::variant<int, std::string>;

    static_assert(Serialize::is_template_of<VarT, std::variant>::value);

    VarT intV{1};
    VarT strV{"hello"};

    auto serializedInt   = Serialize::serialize(intV);
    auto deserializedInt = Serialize::deserialize<VarT>(serializedInt);
    ASSERT_EQ(intV, deserializedInt);

    auto serializedString   = Serialize::serialize(strV);
    auto deserializedString = Serialize::deserialize<VarT>(serializedString);
    ASSERT_EQ(strV, deserializedString);
}
