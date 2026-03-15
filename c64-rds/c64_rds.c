#pragma link("c64_rds.ld")

#include "c64_rds.h"
#include <c64-keyboard.h>
#include <string.h>

static void kernal_setnam(char* filename) {
    char filename_len = (char)strlen(filename);
#ifndef __INTELLISENSE__
    asm {
        lda filename_len
        ldx filename
        ldy filename+1
        jsr $ffbd
    }
#else
    (void)filename_len;
#endif
}

static void kernal_setlfs(char lfn, char device, char sa) {
#ifndef __INTELLISENSE__
    asm {
        lda lfn
        ldx device
        ldy sa
        jsr $ffba
    }
#else
    (void)lfn;
    (void)device;
    (void)sa;
#endif
}

static char kernal_load(char* address, char verify) {
    char status = 0;
#ifndef __INTELLISENSE__
    asm {
        ldx address
        ldy address+1
        lda verify
        jsr $ffd5
        bcs error
        lda #$ff
      error:
        sta status
    }
#endif
    return status;
}

char load_road_bin() {
    char* const ROAD_DATA = (char*)0xc800;
    kernal_setnam("ROAD.BIN");
    kernal_setlfs(1, 8, 0);
    return kernal_load(ROAD_DATA, 0);
}

void main() {
    draw_clear_screen();
    draw_status_xy(0, 0, "moving screen...");

load    machine_save_state();
    draw_status_xy(0, 1, "save machine state done...");

    copy_rom_charset_to_ram();
    draw_status_xy(0, 2, "copy charset done...");

    load_road_bin();
    setup_editor_vicii();

    draw_use_editor_screen();
    clear_editor_screen_tile0();
    draw_status_xy(0, 0, "charset geladen");

    const char map_width = 18;
    char map_left = (40 - map_width) / 2;
    char map_top = 1;

    char test0[7] = {0, 1, 2, 3, 4, 4, 4};
    char test1[7] = {4, 3, 2, 1, 0, 0, 0};
    char test2[7] = {1, 2, 1, 2, 1, 2, 1};
    char test3[7] = {3, 4, 3, 4, 3, 4, 3};
    char test4[7] = {0, 1, 2, 1, 2, 1, 2};
    char test5[7] = {2, 2, 3, 2, 3, 2, 2};
    char test6[7] = {0, 1, 2, 3, 2, 1, 0};
    char test7[7] = {4, 4, 3, 2, 1, 0, 0};
    char test8[7] = {1, 1, 2, 3, 2, 1, 1};
    char test9[7] = {0, 0, 1, 2, 1, 0, 0};
    char test10[7] = {0, 2, 2, 4, 4, 4, 4};
    char test11[7] = {1, 3, 4, 4, 4, 4, 4};
    char test12[7] = {4, 2, 2, 0, 0, 0, 0};
    char test13[7] = {4, 4, 2, 2, 0, 0, 0};
    char test14[7] = {3, 1, 3, 1, 3, 1, 1};
    char test15[7] = {4, 4, 4, 4, 4, 4, 2};

    draw_road_test_case(map_left, map_top, test15, 1);
    draw_road_test_case(map_left, map_top, test14, 1);
    draw_road_test_case(map_left, map_top, test0, 1);
    draw_road_test_case(map_left, map_top, test1, 1);
    draw_road_test_case(map_left, map_top, test2, 1);
    draw_road_test_case(map_left, map_top, test3, 1);
    draw_road_test_case(map_left, map_top, test4, 1);
    draw_road_test_case(map_left, map_top, test5, 1);
    draw_road_test_case(map_left, map_top, test6, 1);
    draw_road_test_case(map_left, map_top, test7, 1);
    draw_road_test_case(map_left, map_top, test8, 1);
    draw_road_test_case(map_left, map_top, test9, 1);
    draw_road_test_case(map_left, map_top, test10, 1);
    draw_road_test_case(map_left, map_top, test11, 1);
    draw_road_test_case(map_left, map_top, test12, 1);
    draw_road_test_case(map_left, map_top, test13, 1);

    keyboard_init();

    while(keyboard_key_pressed(KEY_X) == 0) {
        if(keyboard_key_pressed(KEY_L) != 0) {
            while(keyboard_key_pressed(KEY_L) != 0) {}
        }
        if(keyboard_key_pressed(KEY_H) != 0) {
            while(keyboard_key_pressed(KEY_H) != 0) {}
        }
        if(keyboard_key_pressed(KEY_J) != 0) {
            while(keyboard_key_pressed(KEY_J) != 0) {}
        }
        if(keyboard_key_pressed(KEY_K) != 0) {
            while(keyboard_key_pressed(KEY_K) != 0) {}
        }
    }

    restore_machine_state();
}
