#pragma lib_configure

#pragma lib_export(__varcall, editor_run)

#include "c64_editor.h"
#include "c64_draw.p"
#include <c64-keyboard.h>

const char editor_tests[112] = {
    0, 1, 2, 3, 4, 4, 4,
    4, 3, 2, 1, 0, 0, 0,
    1, 2, 1, 2, 1, 2, 1,
    3, 4, 3, 4, 3, 4, 3,
    0, 1, 2, 1, 2, 1, 2,
    2, 2, 3, 2, 3, 2, 2,
    0, 1, 2, 3, 2, 1, 0,
    4, 4, 3, 2, 1, 0, 0,
    1, 1, 2, 3, 2, 1, 1,
    0, 0, 1, 2, 1, 0, 0,
    0, 2, 2, 4, 4, 4, 4,
    1, 3, 4, 4, 4, 4, 4,
    4, 2, 2, 0, 0, 0, 0,
    4, 4, 2, 2, 0, 0, 0,
    3, 1, 3, 1, 3, 1, 1,
    4, 4, 4, 4, 4, 4, 2
};

static char editor_current_test;
static char editor_map_left;
static char editor_map_top;

static void editor_wait_key_release(char key) {
    while(keyboard_key_pressed(key) != 0) {}
}

static void editor_draw_current_test() {
    char label[] = "test 00";
    unsigned char idx = (unsigned char)editor_current_test;
    const char* heights = editor_tests + ((unsigned int)idx * 7u);
    unsigned char ones = idx;
    char tens = '0';

    if(ones >= 10u) {
        tens = '1';
        ones -= 10u;
    }

    label[5] = tens;
    label[6] = (char)('0' + ones);

    clear_road_matrix(editor_map_left, editor_map_top);
    draw_road_vertical(editor_map_left, editor_map_top, heights);
    draw_status_xy(0, 2, label);
}

static void editor_layout_init() {
    draw_use_editor_screen();
    clear_editor_screen_tile0();

    draw_status_xy(0, 0, "RDS editor");
    draw_status_xy(0, 1, "H/J prev  K/L next  X exit");

    editor_map_left = (char)((40 - 18) / 2);
    editor_map_top = 4;
    editor_current_test = 0;

    editor_draw_current_test();
}

void editor_run() {
    keyboard_init();
    editor_layout_init();

    while(keyboard_key_pressed(KEY_X) == 0) {
        if(keyboard_key_pressed(KEY_H) != 0 || keyboard_key_pressed(KEY_J) != 0) {
            if(editor_current_test == 0) {
                editor_current_test = 15;
            } else {
                editor_current_test -= 1;
            }
            editor_draw_current_test();
            editor_wait_key_release(KEY_H);
            editor_wait_key_release(KEY_J);
            continue;
        }

        if(keyboard_key_pressed(KEY_K) != 0 || keyboard_key_pressed(KEY_L) != 0) {
            if(editor_current_test == 15) {
                editor_current_test = 0;
            } else {
                editor_current_test += 1;
            }
            editor_draw_current_test();
            editor_wait_key_release(KEY_K);
            editor_wait_key_release(KEY_L);
            continue;
        }
    }

    editor_wait_key_release(KEY_X);
}
