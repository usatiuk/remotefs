include(gTest)

add_executable(
        SerializableHelperTest
        src/SerializableHelperTest.cpp
)

target_link_libraries(
        SerializableHelperTest PRIVATE
        GTest::gtest_main utils
)

gtest_discover_tests(SerializableHelperTest DISCOVERY_TIMEOUT 600)
