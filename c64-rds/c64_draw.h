#ifndef C64_DRAW_H
#define C64_DRAW_H

extern char* conio_screen_base;
extern char* const EDITOR_SCREEN;
extern const char MAP_WIDTH;

void copy_rom_charset_to_ram();
void setup_editor_vicii();

void draw_clear_screen();
void draw_status_xy(char x, char y, char* text);
void draw_use_editor_screen();

void clear_editor_screen_tile0();
void clear_road_matrix(char map_left, char map_top);
char draw_road_vertical(char map_left, char map_top, const char* heights7);

#endif