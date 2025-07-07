#include "../include/ui.hpp"
#include <algorithm>
#include <cstring>

UI::UI() : input_buffer("") {
    // Initialize ncurses
    initscr();
    raw();  // Changed from cbreak() to raw() to handle special characters better
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    nonl();         // Tell curses not to do NL->CR/NL on output
    intrflush(stdscr, FALSE);  // Prevent flush on interrupt
    
    // Enable colors if supported
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_CYAN, COLOR_BLACK);
    }
    
    getmaxyx(stdscr, max_y, max_x);
    
    // Create windows
    main_win = newwin(max_y - 2, max_x, 0, 0);
    input_win = newwin(1, max_x - 10, max_y - 1, 2);
    
    // Enable keypad for special keys
    keypad(main_win, TRUE);
    keypad(input_win, TRUE);
    
    // Enable scrolling
    scrollok(main_win, TRUE);
    
    // Draw initial UI
    refresh();
    draw_borders();
    draw_header();
    draw_footer("Type to search. Press F1 for help.");
    
    // Show word of the day (moved to run() method)
    
    wrefresh(main_win);
}

UI::~UI() {
    delwin(main_win);
    delwin(input_win);
    endwin();
}

void UI::draw_borders() {
    // Draw border around main window
    box(stdscr, 0, 0);
    
    // Draw title
    std::string title = " Dictionary App ";
    mvprintw(0, (max_x - title.length()) / 2, "%s", title.c_str());
    
    // Draw input prompt
    mvprintw(max_y - 1, 0, ">");
    
    refresh();
}

void UI::draw_header() {
    // Clear header area
    for (int i = 1; i < 3; i++) {
        move(i, 1);
        clrtoeol();
    }
    
    // Draw header content
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(1, 2, "Search, Add, or type 'help' for commands");
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    // Draw a separator line
    attron(COLOR_PAIR(2));
    mvhline(2, 1, '-', max_x - 2);
    attroff(COLOR_PAIR(2));
    
    refresh();
}

void UI::draw_footer(const std::string& status) {
    // Clear footer area
    move(max_y - 1, 1);
    clrtoeol();
    
    // Draw status message
    attron(COLOR_PAIR(2));
    mvprintw(max_y - 1, 2, "%s", status.c_str());
    attroff(COLOR_PAIR(2));
    
    // Draw input prompt
    move(max_y - 1, 0);
    refresh();
}

void UI::process_input(int ch) {
    switch (ch) {
        case KEY_BACKSPACE:
        case 127:  // Backspace
        case 8:    // Backspace (alternative)
            if (!input_buffer.empty()) {
                input_buffer.pop_back();
                wclear(input_win);
                wprintw(input_win, "%s", input_buffer.c_str());
                wrefresh(input_win);
            }
            break;
            
        case '\n':  // Enter key
        case '\r':  // Also handle carriage return (for Windows compatibility)
            if (!input_buffer.empty()) {
                // Process command
                std::string cmd = input_buffer;
                input_buffer.clear();
                wclear(input_win);
                wrefresh(input_win);
                
                // Add to history
                search_history.push_back(cmd);
                if (search_history.size() > 100) {
                    search_history.erase(search_history.begin());
                }
                
                // Process command
                if (cmd == "help") {
                    // Show help
                    wprintw(main_win, "\nCommands:\n");
                    wprintw(main_win, "  /q       - Quit\n");
                    wprintw(main_win, "  /h       - Show history\n");
                    wprintw(main_win, "  /c       - Clear screen\n");
                    wprintw(main_win, "  /a or /add - Add a new word (interactive)\n");
                    wprintw(main_win, "  word     - Search for a word\n\n");
                    wprintw(main_win, "Keyboard Shortcuts:\n");
                    wprintw(main_win, "  F1       - Show this help\n");
                    wprintw(main_win, "  Ctrl+L   - Clear screen\n");
                    wprintw(main_win, "  Esc      - Exit\n\n");
                } else if (cmd == "/q") {
                    // Quit
                    endwin();
                    exit(0);
                } else if (cmd == "/c") {
                    // Clear screen
                    wclear(main_win);
                    draw_header();
                } else if (cmd == "/h") {
                    // Show history
                    wprintw(main_win, "\nSearch History:\n");
                    int count = 0;
                    for (auto it = search_history.rbegin(); it != search_history.rend() && count < 10; ++it) {
                        if (!it->empty() && (*it)[0] != '/') {  // Skip commands
                            wprintw(main_win, "  %s\n", it->c_str());
                            count++;
                        }
                    }
                    wprintw(main_win, "\n");
                } else if (cmd == "/add" || cmd == "/a") {
                    // Show add word dialog
                    show_add_word_dialog();
                    // Redraw main window
                    wclear(main_win);
                    draw_header();
                    wrefresh(main_win);
                } else {
                    // Search for word
                    wprintw(main_win, "\n> %s\n", cmd.c_str());
                    auto results = on_search(cmd);
                    
                    if (results.empty()) {
                        wprintw(main_win, "  No results found.\n");
                    } else {
                        for (const auto& line : results) {
                            wprintw(main_win, "  %s\n", line.c_str());
                        }
                    }
                }
                
                wrefresh(main_win);
                draw_footer("Type to search. Press F1 for help.");
            }
            break;
            
        case KEY_F(1):
            // Show help
            wprintw(main_win, "\nKeyboard Shortcuts:\n");
            wprintw(main_win, "  F1       - Show this help\n");
            wprintw(main_win, "  Ctrl+L   - Clear screen\n");
            wprintw(main_win, "  Ctrl+U   - Clear input\n");
            wprintw(main_win, "  Up/Down  - Navigate history\n");
            wprintw(main_win, "  Esc      - Exit\n\n");
            wrefresh(main_win);
            break;
            
        case 12:  // Ctrl+L
            wclear(main_win);
            draw_header();
            wrefresh(main_win);
            break;
            
        case 21:  // Ctrl+U
            input_buffer.clear();
            wclear(input_win);
            wrefresh(input_win);
            break;
            
        case 27:  // ESC key
            endwin();
            exit(0);
            
        case KEY_UP:
            // Navigate history (simplified)
            if (!search_history.empty()) {
                static size_t hist_pos = search_history.size();
                if (hist_pos > 0) {
                    hist_pos--;
                    input_buffer = search_history[hist_pos];
                    wclear(input_win);
                    wprintw(input_win, "%s", input_buffer.c_str());
                    wrefresh(input_win);
                }
            }
            break;
            
        case KEY_DOWN:
            // Navigate history down (simplified)
            if (!search_history.empty()) {
                static size_t hist_pos = 0;
                if (hist_pos < search_history.size() - 1) {
                    hist_pos++;
                    input_buffer = search_history[hist_pos];
                } else {
                    input_buffer.clear();
                }
                wclear(input_win);
                wprintw(input_win, "%s", input_buffer.c_str());
                wrefresh(input_win);
            }
            break;
            
        default:
            // Add character to input buffer
            if (isprint(ch)) {
                input_buffer += ch;
                wprintw(input_win, "%c", ch);
                wrefresh(input_win);
            }
            break;
    }
}

void UI::run() {
    // Show word of the day after initialization
    std::string wod = get_word_of_the_day();
    if (!wod.empty()) {
        wprintw(main_win, "\n  Word of the Day: ");
        wattron(main_win, COLOR_PAIR(1) | A_BOLD);
        wprintw(main_win, "%s", wod.c_str());
        wattroff(main_win, COLOR_PAIR(1) | A_BOLD);
        wprintw(main_win, "\n\n");
        wrefresh(main_win);
    }
    
    // Main input loop
    int ch;
    while ((ch = wgetch(input_win)) != 'q') {
        process_input(ch);
    }
}
