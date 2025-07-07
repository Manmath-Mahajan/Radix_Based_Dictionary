#!/bin/bash

# Create package directory
PACKAGE_DIR="DictionaryApp_Mac"
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

# Copy necessary files
cp trie_dict "$PACKAGE_DIR/"
cp get_meaning.py "$PACKAGE_DIR/"
cp -r assets "$PACKAGE_DIR/"

# Create a README file
cat > "$PACKAGE_DIR/README.txt" << 'EOL'
📚 Dictionary App for Mac

How to Run:
1. Open Terminal
2. Navigate to this folder: cd /path/to/DictionaryApp_Mac
3. Make the app executable: chmod +x trie_dict
4. Run: ./trie_dict

Requirements:
- macOS 10.15 or later
- Python 3.6+ (for word meanings)
- Install Python package: pip3 install requests

Note: On first run, allow the app to run in System Preferences → Security & Privacy.
EOL

# Make everything executable
chmod +x "$PACKAGE_DIR/trie_dict"
chmod +x "$PACKAGE_DIR/get_meaning.py"

# Create a ZIP file
zip -r "${PACKAGE_DIR}.zip" "$PACKAGE_DIR"

echo "✅ Package created: ${PACKAGE_DIR}.zip"
echo "📦 Send this ZIP file to your Mac and extract it."
