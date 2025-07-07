from __future__ import annotations

import json
import sys
import textwrap
from typing import List, Tuple

try:
    import requests  # type: ignore
except ImportError:  # pragma: no cover
    print(
        "[ERROR] Python package 'requests' not found. Install via 'pip install requests'",
        file=sys.stderr,
    )
    sys.exit(2)

API_URL = "https://api.dictionaryapi.dev/api/v2/entries/en/{}"

def fetch_meanings(word: str) -> List[Tuple[str, str, str]] | None:
    """Return list of (partOfSpeech, definition, example) tuples."""

    try:
        resp = requests.get(API_URL.format(word), timeout=5)
    except requests.RequestException as exc:  # pragma: no cover
        print(f"[ERROR] Network error: {exc}", file=sys.stderr)
        return None

    if resp.status_code != 200:
        return None

    try:
        data = resp.json()
    except json.JSONDecodeError:  # pragma: no cover
        return None

    if not isinstance(data, list) or not data:
        return None

    entry = data[0]
    meanings = []
    for meaning in entry.get("meanings", []):
        pos = meaning.get("partOfSpeech", "")
        for definition_block in meaning.get("definitions", []):
            definition = definition_block.get("definition", "").strip()
            example = definition_block.get("example", "").strip()
            if definition:
                meanings.append((pos, definition, example))
    return meanings or None


def print_meanings(word: str, meanings: List[Tuple[str, str, str]]) -> None:
    """Pretty-print the meanings to stdout."""

    wrap = textwrap.TextWrapper(width=80, subsequent_indent=" " * 4)
    print(f"\033[1;32mDefinitions for '{word}':\033[0m")
    for i, (pos, definition, example) in enumerate(meanings, 1):
        pos_str = f" ({pos})" if pos else ""
        print(f"{i}. {pos_str} " + wrap.fill(definition))
        if example:
            print(" " * 4 + "e.g., " + wrap.fill(example))


def main() -> None:  # pragma: no cover
    if len(sys.argv) < 2:
        print("Usage: get_meaning.py <word>", file=sys.stderr)
        sys.exit(1)

    word = sys.argv[1].strip()
    if not word:
        sys.exit(1)

    meanings = fetch_meanings(word)
    if not meanings:
        print(f"No definition found for '{word}'.", file=sys.stderr)
        sys.exit(1)

    print_meanings(word, meanings)


if __name__ == "__main__":  # pragma: no cover
    main()
