#pragma once
#include <sqlite3.h>
#include <string>
#include <memory>
#include <vector>

class DictionaryDB {
private:
    sqlite3* db;
    bool create_tables();
    
public:
    // Constructor/Destructor
    DictionaryDB(const std::string& db_path);
    ~DictionaryDB();
    
    // Word operations
    bool add_word(const std::string& word, const std::string& meaning);
    std::string get_meaning(const std::string& word);
    bool word_exists(const std::string& word);
    
    // Stats tracking
    void record_search(const std::string& word);
    std::vector<std::pair<std::string, int>> get_search_history(int limit = 10);
    
    // Prevent copying
    DictionaryDB(const DictionaryDB&) = delete;
    DictionaryDB& operator=(const DictionaryDB&) = delete;
};
