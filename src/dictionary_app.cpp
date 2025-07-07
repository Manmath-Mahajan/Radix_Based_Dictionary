#include "../include/database.hpp"
#include "../include/radix_tree.hpp"
#include "../include/ui.hpp"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

class DictionaryApp : public UI {
private:
    RadixTree tree;
    std::unique_ptr<DictionaryDB> db;
    std::string currentUser;
    std::string userPath;
    
public:
    DictionaryApp() {
        // Initialize database
        try {
            db = std::make_unique<DictionaryDB>("dictionary.db");
        } catch (const std::exception& e) {
            endwin();
            std::cerr << "Failed to initialize database: " << e.what() << "\n";
            exit(1);
        }
        
        // Load initial dictionary
        load_dictionary();
    }
    
    void load_dictionary() {
        std::string dict_path = "assets/dictionary.txt";
        std::ifstream file(dict_path);
        if (file.is_open()) {
            std::string word;
            while (std::getline(file, word)) {
                tree.insert(word);
            }
        }
    }
    
    // Override UI callbacks
    std::vector<std::string> on_search(const std::string& query) override {
        std::vector<std::string> results;
        
        // Try to get from local database first
        std::string meaning = db->get_meaning(query);
        if (!meaning.empty()) {
            // Split meaning into lines for display
            std::istringstream iss(meaning);
            std::string line;
            while (std::getline(iss, line)) {
                results.push_back(line);
            }
            db->record_search(query);
            return results;
        }
        
        // If not found locally, try online
        std::string command = "python3 get_meaning.py \"" + query + "\"";
        std::array<char, 128> buffer;
        std::string result;
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            results.push_back("Error: Failed to execute command");
            return results;
        }
        
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        
        int status = pclose(pipe);
        
        if (status != 0) {
            results.push_back("Error: Failed to get meaning from online source");
            return results;
        }
        
        // If we got a valid response, save it to the database
        if (!result.empty() && result.find("No definition found") == std::string::npos) {
            db->add_word(query, result);
            db->record_search(query);
            
            // Split result into lines
            std::istringstream iss(result);
            std::string line;
            while (std::getline(iss, line)) {
                results.push_back(line);
            }
        } else {
            results.push_back("No definition found");
        }
        
        return results;
    }
    
    bool on_add_word(const std::string& word, const std::string& meaning) override {
        if (tree.search(word)) {
            return false;  // Word already exists
        }
        
        tree.insert(word);
        return db->add_word(word, meaning);
    }
    
    std::string get_word_of_the_day() override {
        // Simple implementation - just return the first word for now
        auto words = tree.starts_with("");
        if (!words.empty()) {
            return words[0];
        }
        return "";
    }
    
    void run() {
        UI::run();
    }
};

int main() {
    DictionaryApp app;
    app.run();
    return 0;
}
