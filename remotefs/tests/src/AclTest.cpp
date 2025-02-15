//
// Created by Stepan Usatiuk on 08.12.2024.
//

#include <gtest/gtest.h>

#include "Acl.hpp"

// sha256 hash of string "password"
static const std::string pass_hash = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";

TEST(AclTest, Empty) {
    ACL acl;
    acl.load("", "username " + pass_hash);

    ASSERT_TRUE(acl.authorize("username", "password"));
    ASSERT_FALSE(acl.authorize("username", "1234"));
    ASSERT_TRUE(acl.authorize_path("username", "/asdf"));
    ASSERT_TRUE(acl.authorize_path("username", "/asdf/1234"));
    ASSERT_TRUE(acl.authorize_path("username34", "/asdf/1234"));
}

TEST(AclTest, SimplePaths) {
    ACL acl;
    acl.load("/ username\n/public\n", "username " + pass_hash);

    ASSERT_TRUE(acl.authorize_path("username", "/"));
    ASSERT_FALSE(acl.authorize_path("no_username", "/"));
    ASSERT_TRUE(acl.authorize_path("username", "/asdf"));
    ASSERT_FALSE(acl.authorize_path("no_username", "/asdf"));

    ASSERT_TRUE(acl.authorize_path("username", "/public"));
    ASSERT_TRUE(acl.authorize_path("no_username", "/public"));
    ASSERT_TRUE(acl.authorize_path("username", "/public/1234"));
    ASSERT_TRUE(acl.authorize_path("no_username", "/public/1234"));
}

TEST(AclTest, MultipleUsers) {
    ACL acl;
    acl.load("/ user1\n/A user1 user2\n/B user2", "username " + pass_hash);

    ASSERT_TRUE(acl.authorize_path("user1", "/"));
    ASSERT_TRUE(acl.authorize_path("user1", "/asdf"));
    ASSERT_FALSE(acl.authorize_path("user2", "/"));
    ASSERT_FALSE(acl.authorize_path("user2", "/asdf"));

    ASSERT_TRUE(acl.authorize_path("user1", "/A"));
    ASSERT_TRUE(acl.authorize_path("user1", "/A/a"));
    ASSERT_TRUE(acl.authorize_path("user2", "/A"));
    ASSERT_TRUE(acl.authorize_path("user2", "/A/a"));

    ASSERT_TRUE(acl.authorize_path("user2", "/B/a"));
    ASSERT_TRUE(acl.authorize_path("user2", "/B"));
    ASSERT_FALSE(acl.authorize_path("user1", "/B"));
    ASSERT_FALSE(acl.authorize_path("user1", "/B/a"));

    ASSERT_FALSE(acl.authorize_path("user3", "/B"));
    ASSERT_FALSE(acl.authorize_path("user3", "/A"));
    ASSERT_FALSE(acl.authorize_path("user3", "/"));
}

TEST(AclTest, MultipleUsers3) {
    ACL acl;
    acl.load("/ user1\n/A user1 user2\n/B user2\n/C", "username " + pass_hash);

    ASSERT_TRUE(acl.authorize_path("user1", "/"));
    ASSERT_TRUE(acl.authorize_path("user1", "/asdf"));
    ASSERT_FALSE(acl.authorize_path("user2", "/"));
    ASSERT_FALSE(acl.authorize_path("user2", "/asdf"));

    ASSERT_TRUE(acl.authorize_path("user1", "/A"));
    ASSERT_TRUE(acl.authorize_path("user1", "/A/a"));
    ASSERT_TRUE(acl.authorize_path("user2", "/A"));
    ASSERT_TRUE(acl.authorize_path("user2", "/A/a"));

    ASSERT_TRUE(acl.authorize_path("user2", "/B/a"));
    ASSERT_TRUE(acl.authorize_path("user2", "/B"));
    ASSERT_FALSE(acl.authorize_path("user1", "/B"));
    ASSERT_FALSE(acl.authorize_path("user1", "/B/a"));

    ASSERT_FALSE(acl.authorize_path("user3", "/B"));
    ASSERT_FALSE(acl.authorize_path("user3", "/A"));
    ASSERT_FALSE(acl.authorize_path("user3", "/"));

    ASSERT_TRUE(acl.authorize_path("user1", "/C"));
    ASSERT_TRUE(acl.authorize_path("user2", "/C"));
    ASSERT_TRUE(acl.authorize_path("user3", "/C"));

    ASSERT_TRUE(acl.authorize_path("user1", "/C/a"));
    ASSERT_TRUE(acl.authorize_path("user2", "/C/a"));
    ASSERT_TRUE(acl.authorize_path("user3", "/C/a"));
}
