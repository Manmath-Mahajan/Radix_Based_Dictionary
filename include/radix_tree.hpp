#pragma once
#include <algorithm>
#include <ctime>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Node of Radix Tree
struct RadixTreeNode {
  bool isEndOfWord = false;
  // Edge label -> child node
  std::unordered_map<std::string, std::shared_ptr<RadixTreeNode>> children;
};

// Statistics for each word
struct WordInfo {
  int frequency = 0;
  time_t lastAccessTime = 0;
};

class RadixTree {
private:
  std::shared_ptr<RadixTreeNode> root;
  std::unordered_map<std::string, WordInfo> wordStats;

  void collect_words(const std::shared_ptr<RadixTreeNode> &node,
                     const std::string &prefix,
                     std::vector<std::string> &words) const;
  bool removeHelper(const std::shared_ptr<RadixTreeNode> &node,
                    const std::string &key, size_t depth);
  size_t commonPrefix(const std::string &s1, const std::string &s2) const;

public:
  RadixTree();
  // Basic operations
  void insert(const std::string &key);
  bool search(const std::string &key) const;
  void remove(const std::string &key);
  void update(const std::string &oldKey, const std::string &newKey);
  std::vector<std::string> starts_with(const std::string &prefix) const;
  // Suggestions (simple edit-distance based brute force)
  std::vector<std::string> suggest(const std::string &word,
                                   int max_distance = 2) const;
  // Statistics
  void recordUsage(const std::string &word);
  void loadStats(const std::string &filename);
  void saveStats(const std::string &filename) const;
  // Batch load
  void loadWords(const std::string &filename);
  // Analytics
  std::vector<std::pair<std::string, int>> getTopNWords(int N) const;
};
