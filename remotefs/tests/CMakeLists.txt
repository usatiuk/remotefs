include(gTest)

add_executable(
        AclTest
        src/AclTest.cpp
)

target_link_libraries(
        AclTest PRIVATE
        GTest::gtest_main remotefs_lib
)

gtest_discover_tests(AclTest DISCOVERY_TIMEOUT 600)
