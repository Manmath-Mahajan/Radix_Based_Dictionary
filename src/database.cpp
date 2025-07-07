#include "../include/database.hpp"
#include <sqlite3.h>
#include <iostream>
#include <filesystem>

// SQL statements
const char* CREATE_TABLES = R"(
    CREATE TABLE IF NOT EXISTS words (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        word TEXT UNIQUE NOT NULL,
        meaning TEXT NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );
    
    CREATE TABLE IF NOT EXISTS search_history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        word TEXT NOT NULL,
        search_count INTEGER DEFAULT 1,
        last_searched TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (word) REFERENCES words(word) ON DELETE CASCADE
    );
    
    CREATE INDEX IF NOT EXISTS idx_word ON words(word);
    CREATE INDEX IF NOT EXISTS idx_search_history ON search_history(word);
)";

// Constructor
DictionaryDB::DictionaryDB(const std::string& db_path) : db(nullptr) {
    // Create user data directory if it doesn't exist
    std::string dir_path = std::string(getenv("HOME")) + "/.local/share/dictionary";
    std::filesystem::create_directories(dir_path);
    
    std::string full_path = dir_path + "/" + db_path;
    
    if (sqlite3_open(full_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }
    
    // Enable foreign keys
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    
    // Create tables
    if (!create_tables()) {
        std::cerr << "Failed to create database tables" << std::endl;
    }
}

// Destructor
DictionaryDB::~DictionaryDB() {
    if (db) {
        sqlite3_close(db);
    }
}

bool DictionaryDB::create_tables() {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, CREATE_TABLES, nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool DictionaryDB::add_word(const std::string& word, const std::string& meaning) {
    std::string sql = "INSERT OR REPLACE INTO words (word, meaning) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, meaning.c_str(), -1, SQLITE_STATIC);
    
    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    return result;
}

std::string DictionaryDB::get_meaning(const std::string& word) {
    std::string sql = "SELECT meaning FROM words WHERE word = ?;";
    sqlite3_stmt* stmt;
    std::string result;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return "";
    }
    
    sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* meaning = sqlite3_column_text(stmt, 0);
        if (meaning) {
            result = reinterpret_cast<const char*>(meaning);
        }
    }
    
    sqlite3_finalize(stmt);
    return result;
}

bool DictionaryDB::word_exists(const std::string& word) {
    std::string sql = "SELECT 1 FROM words WHERE word = ?;";
    sqlite3_stmt* stmt;
    bool exists = false;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
    exists = (sqlite3_step(stmt) == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    return exists;
}

void DictionaryDB::record_search(const std::string& word) {
    // Insert or update search count
    std::string sql = R"(
        INSERT INTO search_history (word, search_count, last_searched)
        VALUES (?, 1, CURRENT_TIMESTAMP)
        ON CONFLICT(word) DO UPDATE SET 
            search_count = search_count + 1,
            last_searched = CURRENT_TIMESTAMP;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }
    
    sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<std::pair<std::string, int>> DictionaryDB::get_search_history(int limit) {
    std::vector<std::pair<std::string, int>> history;
    std::string sql = "SELECT word, search_count FROM search_history ORDER BY last_searched DESC LIMIT ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return history;
    }
    
    sqlite3_bind_int(stmt, 1, limit);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* word = sqlite3_column_text(stmt, 0);
        int count = sqlite3_column_int(stmt, 1);
        
        if (word) {
            history.emplace_back(
                reinterpret_cast<const char*>(word),
                count
            );
        }
    }
    
    sqlite3_finalize(stmt);
    return history;
}
