//
// Created by Stepan Usatiuk on 04.01.2025.
//

#ifndef ACL_HPP
#define ACL_HPP

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

class ACL {
public:
    void load(std::string acl, std::string users);
    bool authorize(std::string username, std::string password);
    bool authorize_path(std::string username, std::string path);

private:
    std::unordered_map<std::string, std::string>    _users;
    std::map<std::string, std::unordered_set<std::string>> _filter;
};

#endif // ACL_HPP
