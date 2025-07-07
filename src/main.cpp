#include "../include/radix_tree.hpp"
#include "../include/database.hpp"
#include <chrono>
#include <memory>  // for std::unique_ptr
#include <array>   // for std::array
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <vector>
#include <algorithm>
#include <curl/curl.h>

// ANSI Color Codes
const std::string RESET = "\033[0m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string RED = "\033[31m";
const std::string CYAN = "\033[36m";
const std::string BOLD_BLUE = "\033[1;34m";
const std::string BOLD_YELLOW = "\033[1;33m";
const std::string BOLD_GREEN = "\033[1;32m";

// Global variables
std::string currentUser;
std::string userPath;
std::unordered_map<std::string, std::string> bookmarks;

// Global database instance
std::unique_ptr<DictionaryDB> db;

// Callback function for cURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch a random word from an API
std::string fetchRandomWord() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    std::string randomWord;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://random-word-api.herokuapp.com/word");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);  // 5 second timeout
        
        res = curl_easy_perform(curl);
        
        if (res == CURLE_OK && !readBuffer.empty() && readBuffer.size() > 4) {
            // The API returns the word in format ["word"], so we need to parse it
            randomWord = readBuffer.substr(2, readBuffer.size() - 4);
        } else {
            std::cerr << "Failed to fetch random word: " << (res != CURLE_OK ? curl_easy_strerror(res) : "Empty response") << std::endl;
        }
        
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize cURL" << std::endl;
    }
    
    return randomWord;
}

// Function to get meaning (tries local DB first, falls back to Python script)
void getMeaningFromPython(const std::string &word) {
    // Try to get meaning from local database first
    std::string meaning = db->get_meaning(word);
    
    if (!meaning.empty()) {
        std::cout << GREEN << "From local database:" << RESET << "\n" << meaning << "\n";
        db->record_search(word);
        return;
    }
    
    // If not found locally, try online API
    std::string command = "python3 get_meaning.py \"" + word + "\"";
    
    // Capture the output
    std::array<char, 128> buffer;
    std::string result;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << RED << "Failed to execute command" << RESET << "\n";
        return;
    }
    
    // Read the output
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    int status = pclose(pipe);
    
    if (status != 0) {
        std::cerr << RED << "Failed to get meaning for '" << word << "'." << RESET << "\n";
        return;
    }
    
    // If we got a valid response, save it to the local database
    if (!result.empty() && result.find("No definition found") == std::string::npos) {
        db->add_word(word, result);
        db->record_search(word);
    }
    
    std::cout << result;
    std::cout << "------------------------------------\n";
}

std::string timeToStr(time_t t) {
  if (t == 0)
    return "N/A";
  char buf[80];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
  return std::string(buf);
}

void loadBookmarks(const std::string &path) {
  std::ifstream file(path);
  if (!file)
    return; // File might not exist on first run

  std::string line, word, note;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    if (std::getline(ss, word, '|') && std::getline(ss, note)) {
      bookmarks[word] = note;
    }
  }
}

void saveBookmarks(const std::string &path) {
  std::ofstream file(path);
  for (const auto &[word, note] : bookmarks) {
    file << word << "|" << note << "\n";
  }
}

void addBookmark() {
  std::string word, note;
  std::cout << CYAN << "Enter the word to bookmark: " << RESET;
  std::getline(std::cin, word);

  if (word.empty()) {
    std::cout << RED << "Word cannot be empty." << RESET << std::endl;
    return;
  }

  std::cout << CYAN << "Enter a note for this bookmark (optional): " << RESET;
  std::getline(std::cin, note);

  bookmarks[word] = note;
  std::cout << GREEN << "Bookmark added for '" << word << "'." << RESET
            << std::endl;
}

void viewBookmarks() {
  if (bookmarks.empty()) {
    std::cout << YELLOW << "You have no bookmarks yet." << RESET << std::endl;
    return;
  }

  std::cout << BOLD_YELLOW << "\n--- Your Bookmarks ---" << RESET << std::endl;
  for (const auto &[word, note] : bookmarks) {
    std::cout << CYAN << "Word: " << RESET << word << std::endl;
    std::cout << CYAN << "Note: " << RESET
              << (note.empty() ? "(No note)" : note) << std::endl;
    std::cout << "------------------------" << std::endl;
  }
}

void removeBookmark() {
  std::string word;
  std::cout << CYAN << "Enter the word to remove from bookmarks: " << RESET;
  std::getline(std::cin, word);

  if (bookmarks.count(word)) {
    bookmarks.erase(word);
    std::cout << GREEN << "Bookmark for '" << word << "' removed." << RESET
              << std::endl;
  } else {
    std::cout << RED << "Bookmark for '" << word << "' not found." << RESET
              << std::endl;
  }
}

void exportStatsToCSV(const RadixTree &tree, const std::string &path) {
  std::ofstream out(path);
  if (!out) {
    std::cerr << RED << "Failed to open export.csv for writing." << RESET
              << "\n";
    return;
  }

  out << "Word,Frequency,LastUsed\n";
  const auto &stats = tree.getTopNWords(std::numeric_limits<int>::max());
  for (const auto &[word, freq] : stats) {
    out << word << "," << freq << "," << "N/A" << "\n";
  }

  std::cout << GREEN << "Exported stats to '" << path << "' successfully!"
            << RESET << "\n";
}

void selectUser() {
  const std::string usersDir = "assets/users/";
  std::filesystem::create_directories(usersDir);

  while (true) {
    std::cout << "--- Select User ---" << std::endl;
    std::vector<std::string> users;
    int i = 1;
    for (const auto &entry : std::filesystem::directory_iterator(usersDir)) {
      if (entry.is_directory()) {
        std::cout << i << ". " << entry.path().filename().string() << std::endl;
        users.push_back(entry.path().filename().string());
        i++;
      }
    }
    std::cout << i << ". Add New User" << std::endl;

    std::cout << CYAN << "Enter your choice: " << RESET;
    std::string choiceStr;
    std::getline(std::cin, choiceStr);
    int choice;
    try {
      choice = std::stoi(choiceStr);
    } catch (...) {
      std::cout << RED << "Invalid input. Please enter a number." << RESET
                << std::endl;
      continue;
    }

    if (choice > 0 && choice <= static_cast<int>(users.size())) {
      currentUser = users[choice - 1];
      userPath = usersDir + currentUser + "/";
      std::cout << GREEN << "\nWelcome, " << currentUser << "!" << RESET
                << std::endl;
      return;
    } else if (choice == i) {
      std::cout << CYAN << "Enter new username: " << RESET;
      std::string newUsername;
      std::getline(std::cin, newUsername);
      if (newUsername.empty() || newUsername.find(' ') != std::string::npos) {
        std::cout << RED
                  << "Invalid username. Cannot be empty or contain spaces."
                  << RESET << std::endl;
      } else {
        currentUser = newUsername;
        userPath = usersDir + currentUser + "/";
        std::filesystem::create_directories(userPath);
        std::cout << GREEN << "User '" << currentUser << "' created." << RESET
                  << std::endl;
        std::cout << GREEN << "\nWelcome, " << currentUser << "!" << RESET
                  << std::endl;
        return;
      }
    } else {
      std::cout << RED << "Invalid choice. Please try again." << RESET
                << std::endl;
    }
  }
}

void showMenu() {
  std::cout << "\n--- Radix Tree Dictionary ---" << std::endl;
  std::cout << YELLOW << "1. Insert a word" << RESET << std::endl;
  std::cout << YELLOW << "2. Search for a word" << RESET << std::endl;
  std::cout << YELLOW << "3. Remove a word" << RESET << std::endl;
  std::cout << YELLOW << "4. Find words with a prefix" << RESET << std::endl;
  std::cout << YELLOW << "5. Top 5 Searched Words" << RESET << std::endl;
  std::cout << YELLOW << "6. Batch Load Category" << RESET << std::endl;
  std::cout << BOLD_YELLOW << "--- Bookmarks ---" << RESET << std::endl;
  std::cout << YELLOW << "7. Add a Bookmark" << RESET << std::endl;
  std::cout << YELLOW << "8. View Bookmarks" << RESET << std::endl;
  std::cout << YELLOW << "9. Remove a Bookmark" << RESET << std::endl;
  std::cout << BOLD_YELLOW << "--- Other ---" << RESET << std::endl;
  std::cout << YELLOW << "10. Export to CSV" << RESET << std::endl;
  std::cout << YELLOW << "11. Exit" << RESET << std::endl;
  std::cout << CYAN << "Enter your choice: " << RESET;
}

std::string getDictionaryFile() {
  std::cout << "\nChoose Category to Load:\n";
  std::cout << "1. GRE\n2. Medical\n3. Common\n4. Custom path\n";
  std::cout << CYAN << "Choice: " << RESET;
  int c;
  std::cin >> c;
  std::cin.ignore();
  switch (c) {
  case 1:
    return "assets/gre.txt";
  case 2:
    return "assets/medical.txt";
  case 3:
    return "assets/common.txt";
  case 4: {
    std::string path;
    std::cout << "Enter full file path: ";
    std::getline(std::cin, path);
    return path;
  }
  default:
    return "assets/common.txt";
  }
}

// Helper function to clean input strings
std::string cleanInput(const std::string& input) {
    std::string result = input;
    // Remove carriage returns (\r)
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    // Trim trailing whitespace
    result.erase(result.find_last_not_of(" \n\t\r") + 1);
    return result;
}

int main() {
  // Initialize database
  try {
    db = std::make_unique<DictionaryDB>("dictionary.db");
  } catch (const std::exception& e) {
    std::cerr << "Failed to initialize database: " << e.what() << "\n";
    return 1;
  }

  selectUser();

  RadixTree tree;
  tree.loadStats(userPath + "stats.txt");
  loadBookmarks(userPath + "bookmarks.txt");

  // Initial global load
  tree.loadWords("assets/dictionary.txt");

  // Initialize cURL
  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Word of the day
  static const std::string wodFile = userPath + "word_of_day.txt";
  std::string wod = [&]() {
    std::ifstream in(wodFile);
    std::string w;
    time_t t;
    if (in >> w >> t && difftime(time(0), t) < 86400)
      return w;

    std::string randomWord = fetchRandomWord();
    
    // Fallback if API call fails
    if (randomWord.empty()) {
        std::cerr << "Failed to fetch random word from API, using local dictionary" << std::endl;
        auto all_words = tree.starts_with("");
        if (!all_words.empty()) {
            std::mt19937 gen(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<> d(0, all_words.size() - 1);
            randomWord = all_words[d(gen)];
        } else {
            return std::string("");
        }
    }
    
    std::ofstream out(wodFile);
    out << randomWord << " " << time(0);
    return randomWord;
  }();
  
  // Clean up cURL
  curl_global_cleanup();

  if (!wod.empty()) {
    std::cout << BOLD_YELLOW << "\nWord of the Day: " << RESET << wod
              << std::endl;
    getMeaningFromPython(wod);
  }

  int choice;
  std::string input;
  while (true) {
    showMenu();
    std::cout << "Enter your choice (1-11): ";
    
    // Clear any error flags and ignore any leftover characters
    std::cin.clear();
    
    // Get the entire line of input and clean it
    if (!std::getline(std::cin, input)) {
      std::cout << RED << "Error reading input." << RESET << std::endl;
      continue;
    }
    input = cleanInput(input);
    
    // Try to convert the input to an integer
    try {
      choice = std::stoi(input);
    } catch (const std::exception&) {
      std::cout << RED << "Please enter a valid number (1-11)." << RESET << std::endl;
      continue;
    }

    switch (choice) {
    case 1: {
      std::string word;
      std::cout << CYAN << "Enter word to insert: " << RESET;
      std::getline(std::cin, word);
      word = cleanInput(word);
      tree.insert(word);
      std::cout << GREEN << "'" << word << "' inserted." << RESET << std::endl;
      break;
    }
    case 2: {
      std::string word;
      std::cout << CYAN << "Enter word to search: " << RESET;
      std::getline(std::cin, word);
      word = cleanInput(word);
      if (tree.search(word)) {
        std::cout << GREEN << "'" << word << "' found! Fetching meaning..."
                  << RESET << std::endl;
        tree.recordUsage(word);
        getMeaningFromPython(word);
      } else {
        std::cout << RED << "'" << word << "' not found." << RESET << std::endl;
        auto sug = tree.suggest(word);
        if (!sug.empty()) {
          std::cout << YELLOW << "Did you mean:" << RESET;
          for (auto &s : sug)
            std::cout << " " << s;
          std::cout << std::endl;
        }
      }
      break;
    }
    case 3: {
      std::string word;
      std::cout << CYAN << "Enter word to remove: " << RESET;
      std::getline(std::cin, word);
      word = cleanInput(word);
      tree.remove(word);
      std::cout << GREEN << "'" << word << "' removed." << RESET << std::endl;
      break;
    }
    case 4: {
      std::string prefix;
      std::cout << CYAN << "Enter prefix: " << RESET;
      std::getline(std::cin, prefix);
      prefix = cleanInput(prefix);
      auto words = tree.starts_with(prefix);
      if (words.empty())
        std::cout << RED << "No matches." << RESET << std::endl;
      else {
        std::cout << GREEN << "Matches:" << RESET << std::endl;
        for (auto &w : words) {
          std::cout << "- " << w << std::endl;
        }
      }
      break;
    }
    case 5: {
      auto top = tree.getTopNWords(5);
      std::cout << BOLD_YELLOW << "\nTop 5 Searched Words:" << RESET
                << std::endl;
      for (auto &[w, f] : top) {
        std::cout << "- " << w << " (" << f << ")" << std::endl;
      }
      break;
    }
    case 6: {
      std::string path = getDictionaryFile();
      path = cleanInput(path);
      tree.loadWords(path);
      std::cout << GREEN << "Loaded from " << path << RESET << std::endl;
      break;
    }
    case 7: {
      addBookmark();
      break;
    }
    case 8: {
      viewBookmarks();
      break;
    }
    case 9: {
      removeBookmark();
      std::string word;
      std::cout << CYAN << "Enter word to remove bookmark: " << RESET;
      std::getline(std::cin, word);
      if (bookmarks.erase(word))
        std::cout << GREEN << "Removed bookmark for '" << word << "'." << RESET
                  << std::endl;
      else
        std::cout << RED << "No bookmark for '" << word << "'." << RESET
                  << std::endl;
      break;
    }
    case 10:
      exportStatsToCSV(tree, userPath + "export.csv");
      break;
    case 11:
      std::cout << BOLD_BLUE << "Exiting. Goodbye!" << RESET << std::endl;
      tree.saveStats(userPath + "stats.txt");
      saveBookmarks(userPath + "bookmarks.txt");
      return 0;
    default:
      std::cout << RED << "Invalid choice." << RESET << std::endl;
    }
  }
  return 0;
}
