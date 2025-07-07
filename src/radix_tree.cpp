#include "radix_tree.hpp"

RadixTree::RadixTree() : root(std::make_shared<RadixTreeNode>()) {}

size_t RadixTree::commonPrefix(const std::string &s1,
                               const std::string &s2) const {
  size_t len = std::min(s1.size(), s2.size());
  size_t i = 0;
  while (i < len && s1[i] == s2[i])
    ++i;
  return i;
}

void RadixTree::insert(const std::string &key) {
  auto node = root;
  std::string remaining = key;

  while (!remaining.empty()) {
    bool matched = false;
    for (auto it = node->children.begin(); it != node->children.end(); ++it) {
      const std::string label = it->first;
      auto child = it->second;
      size_t common = commonPrefix(label, remaining);
      if (common == 0)
        continue;
      matched = true;
      if (common < label.size()) {
        // split edge
        std::string labelRemainder = label.substr(common);
        auto split = std::make_shared<RadixTreeNode>();
        split->children[labelRemainder] = child;
        split->isEndOfWord = false;
        // replace original
        node->children.erase(label);
        node->children[label.substr(0, common)] = split;
        child = split;
      }
      remaining = remaining.substr(common);
      node = child;
      break;
    }
    if (!matched) {
      // no match, create new child
      auto leaf = std::make_shared<RadixTreeNode>();
      leaf->isEndOfWord = true;
      node->children[remaining] = leaf;
      recordUsage(key);
      return;
    }
  }
  // mark end of word
  node->isEndOfWord = true;
  recordUsage(key);
}

bool RadixTree::search(const std::string &key) const {
  auto node = root;
  std::string remaining = key;

  while (!remaining.empty()) {
    bool matched = false;
    for (auto &[label, child] : node->children) {
      if (remaining.rfind(label, 0) == 0) {
        remaining = remaining.substr(label.size());
        node = child;
        matched = true;
        break;
      }
    }
    if (!matched)
      return false;
  }
  return node->isEndOfWord;
}

void RadixTree::remove(const std::string &key) { removeHelper(root, key, 0); }

bool RadixTree::removeHelper(const std::shared_ptr<RadixTreeNode> &node,
                             const std::string &key, size_t depth) {
  if (depth == key.size()) {
    if (!node->isEndOfWord)
      return false;
    node->isEndOfWord = false;
    // if leaf
    return node->children.empty();
  }
  for (auto it = node->children.begin(); it != node->children.end(); ++it) {
    const std::string label = it->first;
    if (key.rfind(label, depth) == depth) {
      bool shouldDeleteChild =
          removeHelper(it->second, key, depth + label.size());
      if (shouldDeleteChild) {
        node->children.erase(it);
        return !node->isEndOfWord && node->children.size() == 1;
      }
    }
  }
  return false;
}

void RadixTree::update(const std::string &oldKey, const std::string &newKey) {
  remove(oldKey);
  insert(newKey);
}

void RadixTree::collect_words(const std::shared_ptr<RadixTreeNode> &node,
                              const std::string &prefix,
                              std::vector<std::string> &words) const {
  if (node->isEndOfWord)
    words.push_back(prefix);
  for (auto &[label, child] : node->children) {
    collect_words(child, prefix + label, words);
  }
}

std::vector<std::string>
RadixTree::starts_with(const std::string &prefix) const {
  auto node = root;
  std::string remaining = prefix;
  std::vector<std::string> results;

  // traverse to prefix node
  while (!remaining.empty()) {
    bool matched = false;
    for (auto &[label, child] : node->children) {
      size_t common = commonPrefix(label, remaining);
      if (common == 0)
        continue;
      if (common < remaining.size())
        return results;
      node = child;
      remaining = remaining.substr(common);
      matched = true;
      break;
    }
    if (!matched)
      return results;
  }
  collect_words(node, prefix, results);
  return results;
}

std::vector<std::string> RadixTree::suggest(const std::string &word,
                                            int max_distance) const {
  // naive: collect all words and filter by edit distance
  std::vector<std::string> all;
  collect_words(root, "", all);
  std::vector<std::string> res;
  auto editDist = [&](auto a, auto b) {
    size_t n = a.size(), m = b.size();
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1));
    for (size_t i = 0; i <= n; ++i)
      dp[i][0] = i;
    for (size_t j = 0; j <= m; ++j)
      dp[0][j] = j;
    for (size_t i = 1; i <= n; ++i)
      for (size_t j = 1; j <= m; ++j)
        dp[i][j] =
            std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1,
                      dp[i - 1][j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1)});
    return dp[n][m];
  };
  for (auto &w : all)
    if (editDist(w, word) <= max_distance)
      res.push_back(w);
  return res;
}

void RadixTree::recordUsage(const std::string &word) {
  auto &info = wordStats[word];
  info.frequency++;
  info.lastAccessTime = std::time(nullptr);
}

void RadixTree::loadStats(const std::string &filename) {
  std::ifstream in(filename);
  if (!in)
    return;
  wordStats.clear();
  std::string line;
  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::string w;
    int freq;
    long t;
    if (iss >> w >> freq >> t)
      wordStats[w] = {freq, (time_t)t};
  }
}

void RadixTree::saveStats(const std::string &filename) const {
  std::ofstream out(filename);
  for (auto &p : wordStats) {
    out << p.first << " " << p.second.frequency << " "
        << p.second.lastAccessTime << "\n";
  }
}

void RadixTree::loadWords(const std::string &filename) {
  std::ifstream in(filename);
  if (!in)
    return;
  std::string w;
  while (in >> w)
    insert(w);
}

std::vector<std::pair<std::string, int>> RadixTree::getTopNWords(int N) const {
  std::vector<std::pair<std::string, int>> vec;
  for (auto &p : wordStats)
    vec.emplace_back(p.first, p.second.frequency);
  std::sort(vec.begin(), vec.end(),
            [](auto &a, auto &b) { return a.second > b.second; });
  if (vec.size() > (size_t)N)
    vec.resize(N);
  return vec;
}
