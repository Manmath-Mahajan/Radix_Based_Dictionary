#pragma once

#include <ncurses.h>
#include <string>
#include <vector>
#include <memory>

class UI {
private:
    WINDOW* main_win;
    WINDOW* input_win;
    WINDOW* result_win;
    
    int max_y, max_x;
    std::string input_buffer;
    std::vector<std::string> search_history;
    
    void draw_borders();
    void draw_header();
    void draw_footer(const std::string& status);
    void process_input(int ch);
    
public:
    UI();
    ~UI();
    
    // Main UI loop
    void run();
    
    // Callbacks (to be implemented in main.cpp)
    virtual std::vector<std::string> on_search(const std::string& query) { return {}; }
    virtual bool on_add_word(const std::string& word, const std::string& meaning) { return false; }
    virtual std::string get_word_of_the_day() { return ""; }
    
    // New method for add word dialog
    void show_add_word_dialog() {
        // Create a new window for the dialog
        int height = 8;
        int width = 60;
        WINDOW* dialog = newwin(height, width, 
                              (LINES - height) / 2, 
                              (COLS - width) / 2);
        
        // Enable keypad for special keys
        keypad(dialog, TRUE);
        
        // Draw dialog box
        box(dialog, 0, 0);
        mvwprintw(dialog, 0, 2, " Add New Word ");
        
        // Show instructions
        mvwprintw(dialog, 1, 2, "Enter word and meaning:");
        mvwprintw(dialog, 2, 2, "Word: ");
        mvwprintw(dialog, 4, 2, "Meaning (Ctrl+S to save, Esc to cancel):");
        
        // Input fields
        echo();
        curs_set(1);
        
        // Get word
        char word[50];
        mvwgetnstr(dialog, 2, 8, word, 49);
        
        // Get meaning (multi-line)
        wmove(dialog, 5, 2);
        wclrtoeol(dialog);
        wrefresh(dialog);
        
        char meaning[500];
        mvwgetnstr(dialog, 5, 2, meaning, 499);
        
        // Ask for confirmation
        mvwprintw(dialog, 6, 2, "Save this word? (y/n): ");
        wrefresh(dialog);
        
        int ch = wgetch(dialog);
        if (ch == 'y' || ch == 'Y') {
            if (on_add_word(word, meaning)) {
                mvwprintw(dialog, 7, 2, "Word added successfully!");
            } else {
                mvwprintw(dialog, 7, 2, "Failed to add word!");
            }
            wgetch(dialog);  // Wait for keypress
        }
        
        // Clean up
        delwin(dialog);
        curs_set(1);
        noecho();
        
        // Redraw main UI
        touchwin(stdscr);
        refresh();
    }
};
