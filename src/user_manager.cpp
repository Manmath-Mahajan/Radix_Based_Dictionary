#include "../include/user_manager.hpp"
#include <stdexcept>

UserManager::UserManager() : usersFile("users/.users") {
    std::filesystem::create_directories("users");
    loadUsers();
}

void UserManager::loadUsers() {
    std::ifstream file(usersFile);
    if (!file) return;

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string username = line.substr(0, pos);
            std::string password = line.substr(pos + 1);
            users[username] = password;
        }
    }
}

void UserManager::saveUsers() {
    std::ofstream file(usersFile);
    if (!file) return;

    for (const auto& [username, password] : users) {
        file << username << ":" << password << "\n";
    }
}

bool UserManager::createUser(const std::string& username, const std::string& password) {
    if (users.find(username) != users.end()) {
        return false; // User already exists
    }

    std::string hashedPassword = hashPassword(password);
    users[username] = hashedPassword;
    saveUsers();

    // Create user directory
    userDir = "users/" + username + "/";
    std::filesystem::create_directories(userDir);
    
    // Create necessary files
    std::ofstream(userDir + "bookmarks.txt");
    std::ofstream(userDir + "stats.txt");
    
    return true;
}

bool UserManager::authenticate(const std::string& username, const std::string& password) {
    auto it = users.find(username);
    if (it == users.end()) {
        return false; // User not found
    }

    std::string hashedPassword = hashPassword(password);
    if (it->second == hashedPassword) {
        currentUser = username;
        userDir = "users/" + username + "/";
        return true;
    }
    return false;
}

bool UserManager::removeUser(const std::string& username) {
    if (users.erase(username) > 0) {
        saveUsers();
        // Remove user directory
        std::string dirToRemove = "users/" + username + "/";
        if (std::filesystem::exists(dirToRemove)) {
            std::filesystem::remove_all(dirToRemove);
        }
        return true;
    }
    return false;
}

std::string UserManager::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string UserManager::getCurrentUser() const {
    return currentUser;
}

std::string UserManager::getUserDir() const {
    return userDir;
}

bool UserManager::userExists(const std::string& username) const {
    return users.find(username) != users.end();
}
