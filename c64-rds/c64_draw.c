
#pragma lib_configure

#pragma lib_export(__varcall, save_machine_state, restore_machine_state, copy_rom_charset_to_ram, setup_editor_vicii, draw_clear_screen, draw_status_xy, draw_use_editor_screen, clear_editor_screen_tile0, draw_road_test_case)

// #include "graphics/c64_resources.h"
#include "c64_draw.h"
#include <c64.h>
#include <string.h>

char* conio_screen_base = (char*)DEFAULT_SCREEN;

#include <c64-keyboard.h>

char* const CHARSET_SHADOW = (char*)0xc800;
char* const EDITOR_SCREEN = (char*)0xc000;

char zp_backup[254];
char saved_procport;
char saved_cia2_porta;
char saved_d011;
char saved_d016;
char saved_d018;

const unsigned int CHARSET_SIZE = 2048;
const unsigned int ROM_CHARSET_MIXED_OFFSET = 0x800;

const char MAP_WIDTH = 18;
const char MAP_HEIGHT = 13;

const char flat_segment_4x2_table[24] = {
    10, 11, 0xff, 0xff, 12, 13, 4, 5,
    10, 11, 0xff, 0xff, 12, 13, 10, 11,
    6, 7, 0xff, 0xff, 12, 13, 10, 11
};

const char incline_right_one_segment_3x4_table[] = {
    0xff, 14, 0xff, 0xff,
    14, 1, 1, 15,
    12, 13, 15, 0xff,

    1, 15, 0xff, 0xff,
    15, 1, 1, 15,
    12, 13, 15, 1,

    1, 15, 0xff, 0xff,
    15, 1, 1, 16,
    12, 13, 16, 0xff
};

const char incline_right_two_segment_3x4_table[] = {
    0xff, 31, 0xff, 0xff,
    29, 30, 1, 35,
    28, 1, 33, 34,
    12, 13, 32, 0xff,

    0xff, 35, 0xff, 0xff,
    33, 34, 1, 35,
    32, 1, 33, 34,
    12, 13, 32, 0xff,

    0xff, 35, 0xff, 0xff,
    33, 34, 1, 27,
    32, 1, 25, 26,
    12, 13, 24, 0xff
};

const char decline_right_one_segment_1x2_table[] = {
    20, 21,
    20, 21,
    20, 21
};

const char decline_right_two_segment_1x2_table[] = {
    8, 9,
    0xff, 0xff,
    0xff, 0xff,
    2, 3
};

void draw_clear_screen() {
    memset(conio_screen_base, 0x20, 40 * 25);
}

void draw_status_xy(char x, char y, char* text) {
    char* dest = conio_screen_base + (unsigned int)y * 40 + (unsigned int)x;
    while(*text) {
        *dest++ = *text++;
    }
}

void draw_use_editor_screen() {
    conio_screen_base = EDITOR_SCREEN;
}

void save_machine_state() {
    saved_procport = *PROCPORT;
    saved_cia2_porta = CIA2->PORT_A;
    saved_d011 = *D011;
    saved_d016 = *D016;
    saved_d018 = *D018;
#ifndef __INTELLISENSE__
    kickasm(uses zp_backup) {{
        ldx #0
      !:
        lda $0002,x
        sta zp_backup,x
        inx
        cpx #254
        bne !-
    }}
#endif
}

void irq_enable() {
#ifndef __INTELLISENSE__
    asm { cli }
#endif
}

void restore_machine_state() {
    *PROCPORT = saved_procport;
    CIA2->PORT_A = saved_cia2_porta;
    *D011 = saved_d011;
    *D016 = saved_d016;
    *D018 = saved_d018;
#ifndef __INTELLISENSE__
    kickasm(uses zp_backup) {{
        ldx #0
      !:
        lda zp_backup,x
        sta $0002,x
        inx
        cpx #254
        bne !-
    }}
#endif
    irq_enable();
}

void set_ram_with_io() {
    *PROCPORT = (*PROCPORT & (char)(0xff ^ PROCPORT_DDR_MEMORY_MASK)) | PROCPORT_RAM_IO;
}

void wait_keypress() {
    char* const CIA1_PORT_A = (char*)0xdc00;
    char* const CIA1_PORT_B = (char*)0xdc01;
    *CIA1_PORT_A = 0;
    while(!(~*CIA1_PORT_B)) {}
    while(~*CIA1_PORT_B) {}
}

void irq_disable() {
#ifndef __INTELLISENSE__
    asm { sei }
#endif
}

void copy_rom_charset_to_ram() {
    irq_disable();
    char old_port = *PROCPORT;
    *PROCPORT = (*PROCPORT & (char)(0xff ^ PROCPORT_DDR_MEMORY_MASK)) | PROCPORT_RAM_CHARROM;
    memcpy(CHARSET_SHADOW, CHARGEN + ROM_CHARSET_MIXED_OFFSET, CHARSET_SIZE);
    *PROCPORT = old_port;
    irq_enable();
}

void setup_editor_vicii() {
    irq_disable();
    set_ram_with_io();
    vicSelectGfxBank(EDITOR_SCREEN);
    *D011 = (*D011 & VICII_RST8) | VICII_DEN | VICII_RSEL | VICII_ECM | 3;
    *D016 = VICII_CSEL;
    *D018 = toD018(EDITOR_SCREEN, CHARSET_SHADOW);
    *BORDER_COLOR = 0;
    *BG_COLOR = 1;
}

void draw_tile_block_vertical_table(char map_left, char map_top, char mx, char my, const char* table, unsigned int segment_stride, char segment_column, char block_height, char block_width) {
    unsigned char idx = (unsigned char)segment_column;
    if(idx > 3) {
        idx = 3;
    }

    const char* seg = table;
    switch(idx) {
        case 3:
            seg += segment_stride;
        case 2:
            seg += segment_stride;
        case 1:
            seg += segment_stride;
            break;
        default:
            break;
    }
    char sy = map_top + my;
    char* scr = conio_screen_base + ((unsigned int)(unsigned char)sy) * 40u + (unsigned int)(unsigned char)(map_left + mx);

    char* scr_row = scr;
    const char* seg_row = seg;
    for(char row = 0; row < block_height; row++) {
        for(char col = 0; col < block_width; col++) {
            unsigned char tile = (unsigned char)seg_row[(unsigned char)col];
            if(tile != 0xff) {
                scr_row[(unsigned char)col] = (char)tile;
            }
        }
        scr_row += 40;
        seg_row += (unsigned int)(unsigned char)block_width;
    }
}

void draw_tile_lane_vertical_finish(char map_left, char map_top, char mx, char my) {
    char sy = map_top + my;
    char* scr = conio_screen_base + ((unsigned int)(unsigned char)sy) * 40u + (unsigned int)(unsigned char)(map_left + mx);
    scr[0] = 12;
    scr[1] = 13;
}

void clear_editor_screen_tile0() {
    char* row = conio_screen_base;
    for(char y = 0; y < 25; y++) {
        memset(row, 0, 40u);
        row += 40;
    }
}

void clear_road_matrix(char map_left, char map_top) {
    char* row = conio_screen_base + ((unsigned int)(unsigned char)map_top) * 40u + (unsigned int)(unsigned char)map_left;
    for(char y = 0; y < MAP_HEIGHT; y++) {
        memset(row, 0, (unsigned int)(unsigned char)MAP_WIDTH);
        row += 40;
    }
}

void draw_height_vertical(char map_left, char map_top, char mx, char floor_y, char height) {
    if(height == 0) {
        return;
    }

    if(mx < 0 || mx >= MAP_WIDTH) {
        return;
    }

    char my = floor_y;
    char sy = map_top + my;
    char* scr = conio_screen_base + ((unsigned int)(unsigned char)sy) * 40u + (unsigned int)(unsigned char)(map_left + mx);

    for(char i = 0; i < height; i++) {
        if(my >= 0 && my < MAP_HEIGHT) {
            *scr = 50;
        }

        my -= 1;
        scr -= 40;
    }
}

char draw_road_vertical(char map_left, char map_top, const char* heights7) {
    signed char deltas[6];

    for(char count = 0; count < 6; count++) {
        char idx = (char)(5 - count);
        char height0 = heights7[(unsigned char)idx];
        char height1 = heights7[(unsigned char)(idx + 1)];
        if(height0 > 4 || height1 > 4) {
            return 0;
        }
        signed char delta = (signed char)height1 - (signed char)height0;
        deltas[(unsigned char)count] = delta;
        if(delta != -2 && delta != -1 && delta != 0 && delta != 1 && delta != 2) {
            return 0;
        }
    }

    char base_y = (char)(4 - heights7[6]);
    char base_x = 10;

    for(char count = 0; count < 6; count++) {
        signed char delta = deltas[(unsigned char)count];
        char mx_left = base_x;
        char my_left = base_y;
        char mx_mid = base_x + 2;
        char my_mid = base_y + 1;
        char mx_right = base_x + 4;
        char my_right = base_y + 2;

        if(count == 0) {
            draw_tile_lane_vertical_finish(map_left, map_top, mx_left + 2, my_left);
            draw_tile_lane_vertical_finish(map_left, map_top, mx_mid + 2, my_mid);
            draw_tile_lane_vertical_finish(map_left, map_top, mx_right + 2, my_right);
        }

        switch(delta) {
            case 0:
                draw_tile_block_vertical_table(map_left, map_top, mx_left, my_left, flat_segment_4x2_table, 8u, 2, 2, 4);
                draw_tile_block_vertical_table(map_left, map_top, mx_mid, my_mid, flat_segment_4x2_table, 8u, 1, 2, 4);
                draw_tile_block_vertical_table(map_left, map_top, mx_right, my_right, flat_segment_4x2_table, 8u, 0, 2, 4);
                base_y += 1;
                break;

            case 1:
                draw_tile_block_vertical_table(map_left, map_top, mx_left, my_left, incline_right_one_segment_3x4_table, 12u, 0, 3, 4);
                draw_tile_block_vertical_table(map_left, map_top, mx_mid, my_mid, incline_right_one_segment_3x4_table, 12u, 1, 3, 4);
                draw_tile_block_vertical_table(map_left, map_top, mx_right, my_right, incline_right_one_segment_3x4_table, 12u, 2, 3, 4);
                base_y += 2;
                break;

            case -2:
                draw_tile_block_vertical_table(map_left, map_top, mx_left, my_left - 1, decline_right_two_segment_1x2_table, 2u, 0, 1, 2);
                draw_tile_block_vertical_table(map_left, map_top, mx_right + 2, my_right, decline_right_two_segment_1x2_table, 2u, 3, 1, 2);
                base_y -= 1;
                break;

            case -1:
                draw_tile_block_vertical_table(map_left, map_top, mx_left, my_left, decline_right_one_segment_1x2_table, 2u, 0, 1, 2);
                draw_tile_block_vertical_table(map_left, map_top, mx_mid, my_mid, decline_right_one_segment_1x2_table, 2u, 1, 1, 2);
                draw_tile_block_vertical_table(map_left, map_top, mx_right, my_right, decline_right_one_segment_1x2_table, 2u, 2, 1, 2);
                break;

            case 2:
                draw_tile_block_vertical_table(map_left, map_top, mx_left, my_left, incline_right_two_segment_3x4_table, 16u, 0, 4, 4);
                draw_tile_block_vertical_table(map_left, map_top, mx_mid, my_mid, incline_right_two_segment_3x4_table, 16u, 1, 4, 4);
                draw_tile_block_vertical_table(map_left, map_top, mx_right, my_right, incline_right_two_segment_3x4_table, 16u, 2, 4, 4);
                base_y += 3;
                break;

            default:
                return 0;
        }

        base_x -= 2;

        char next_height = heights7[(unsigned char)(5 - count)];
        if(delta == -2) {
            if(next_height > 0) {
                next_height -= 1;
            }
        }
        draw_height_vertical(map_left, map_top, base_x + 8, count + 7, next_height);
    }

    return 1;
}

void draw_road_test_case(char map_left, char map_top, const char* heights7, char wait_after) {
    clear_road_matrix(map_left, map_top);
    draw_road_vertical(map_left, map_top, heights7);
    if(wait_after != 0) {
        wait_keypress();
    }
}
