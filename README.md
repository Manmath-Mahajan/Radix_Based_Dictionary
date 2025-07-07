# Trie-Based Dictionary

A high-performance, memory-efficient dictionary implementation using a Radix Tree (compressed trie) data structure. This C++ application provides fast word lookups, prefix searches, and spell checking capabilities.

## Features

- 📖 **Dictionary Operations**: Add, search, and remove words
- 🔍 **Prefix Search**: Find all words with a given prefix
- ✨ **Spell Checking**: Get suggestions for misspelled words
- ⚡ **Fast Lookups**: O(k) time complexity for search operations, where k is the length of the key
- 📊 **Statistics**: Track dictionary metrics like word count and memory usage
- 🔖 **Bookmarks**: Save and manage frequently looked-up words

## Prerequisites

- C++17 or later
- CMake (for building)
- Make

## Building the Project

```bash
# Clone the repository
git clone <repository-url>
cd trie_based_dictionary

# Build the project
make
```

## Usage

Run the dictionary application:

```bash
./trie_dict
```

### Available Commands

- `add <word>`: Add a word to the dictionary
- `search <word>`: Search for a word
- `prefix <prefix>`: Find all words with the given prefix
- `suggest <word>`: Get spelling suggestions
- `bookmark`: Manage bookmarks
- `stats`: Show dictionary statistics
- `exit`: Exit the application

## Project Structure

```
├── assets/               # Dictionary files and resources
├── benchmarks/           # Performance benchmarks
├── include/              # Header files
│   └── radix_tree.hpp    # Radix Tree implementation
├── src/                  # Source files
│   ├── main.cpp          # Main application
│   ├── radix_tree.cpp    # Radix Tree implementation
│   └── spellchecker.cpp  # Spell checking functionality
├── Makefile              # Build configuration
└── README.md             # This file
```

## Performance

The Radix Tree implementation provides:
- O(k) search, insert, and delete operations
- Memory efficiency through path compression
- Fast prefix-based searches

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

