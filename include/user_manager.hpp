#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

class UserManager {
private:
    std::string usersFile;
    std::unordered_map<std::string, std::string> users;
    std::string currentUser;
    std::string userDir;

    std::string hashPassword(const std::string& password);
    void loadUsers();
    void saveUsers();

public:
    UserManager();
    bool createUser(const std::string& username, const std::string& password);
    bool authenticate(const std::string& username, const std::string& password);
    bool removeUser(const std::string& username);
    std::string getCurrentUser() const;
    std::string getUserDir() const;
    bool userExists(const std::string& username) const;
};
