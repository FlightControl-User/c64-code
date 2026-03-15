
#pragma link("c64-tiles.ld")
// #pragma var_model(integer_ssa_mem, pointer_ssa_mem, struct_ma_mem, array_ma_mem)

#include "c64-resources.h"
#include <c64.h>
#include <string.h>

// Runtime-switchable screen base: start at default $0400, switch to $C000 after VIC init
char* conio_screen_base = DEFAULT_SCREEN;
#define CONIO_SCREEN_TEXT conio_screen_base
#include <conio.h>

#include <c64-keyboard.h>

__export char header2[] =kickasm {{

    .struct Tile {tile, ext, start, count, skip, size, width, height, bpp, addr}

    .macro Data(tile, tiledata) {
        // Header
//        .byte sprite.count, sprite.size, sprite.width, sprite.height, sprite.zorder, sprite.fliph, sprite.flipv, sprite.bpp, sprite.collision, sprite.reverse, sprite.loop,0,0,0,0

        // Tile
        .print "tiledata.size = " + tiledata.size()
        .var col = 0
        .var line = ""
        .word tile.addr
        .for(var i=0;i<tiledata.size();i++) {
            .eval line = line.string() + toBinaryString(tiledata.get(i),8) + " "
            .eval col = col + 1
            .if(col == tile.width / 8) {
                .eval col = 0
                .print line 
                .eval line = ""
            }
            .var tilebyte = tiledata.get(i) // ^ 255
            .byte tilebyte
        }
    }
}};

__export char fly[] = kickasm {{{
    .var tile = Tile("road","png",0,59,1,8,8,8,1,$c800)
    // .var pallist = GetPalette(tile)
    .var tiledata = MakeTile(tile)
    // .var pallistdata = MakePalette(tile,pallist)
    .file [name="road.bin", type="bin", segments="road"]
    .segmentdef road
    .segment road
    Data(tile,tiledata)
};}};

char* const ROAD_DATA = (char*)0xc800;
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

void save_machine_state() {
    saved_procport = *PROCPORT;
    saved_cia2_porta = CIA2->PORT_A;
    saved_d011 = *D011;
    saved_d016 = *D016;
    saved_d018 = *D018;
    kickasm(uses zp_backup) {{
        ldx #0
      !:
        lda $0002,x
        sta zp_backup,x
        inx
        cpx #254
        bne !-
    }}
}

void restore_machine_state() {
    *PROCPORT = saved_procport;
    CIA2->PORT_A = saved_cia2_porta;
    *D011 = saved_d011;
    *D016 = saved_d016;
    *D018 = saved_d018;
    kickasm(uses zp_backup) {{
        ldx #0
      !:
        lda zp_backup,x
        sta $0002,x
        inx
        cpx #254
        bne !-
    }}
    irq_enable();    // KERNAL ROM is restored, safe to re-enable IRQs
}

void set_rom_for_kernal() {
    *PROCPORT = (*PROCPORT & (char)(0xff ^ PROCPORT_DDR_MEMORY_MASK)) | PROCPORT_BASIC_KERNEL_IO;
}

void set_ram_with_io() {
    *PROCPORT = (*PROCPORT & (char)(0xff ^ PROCPORT_DDR_MEMORY_MASK)) | PROCPORT_RAM_IO;
}

// Wacht tot een toets ingedrukt én losgelaten is
void wait_keypress() {
    while(!kbhit()) {}
    while(kbhit()) {}
}

// Disable IRQs: must be called before mapping out the KERNAL ROM,
// because the IRQ vector ($FFFE) would point to unmapped memory.
void irq_disable() {
    asm { sei }
}

// Re-enable IRQs: call only after the KERNAL ROM is mapped back in.
void irq_enable() {
    asm { cli }
}


// C64 KERNAL SETNAM ($FFBD)
void cbm_setnam(char* filename) {
    char filename_len = (char)strlen(filename);
    asm {
        lda filename_len
        ldx filename
        ldy filename+1
        jsr $ffbd
    }
}

// C64 KERNAL SETLFS ($FFBA)
void cbm_setlfs(char lfn, char device, char sa) {
    asm {
        lda lfn
        ldx device
        ldy sa
        jsr $ffba
    }
}

// C64 KERNAL LOAD ($FFD5)
char cbm_load(char* address, char verify) {
    char status;
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
    return status;
}

char load_road_bin() {
    cbm_setnam("ROAD.BIN");
    cbm_setlfs(1, 8, 0);
    return cbm_load(ROAD_DATA, 0);
}

void copy_rom_charset_to_ram() {
    irq_disable();    // disable before mapping out KERNAL ROM
    char old_port = *PROCPORT;
    *PROCPORT = (*PROCPORT & (char)(0xff ^ PROCPORT_DDR_MEMORY_MASK)) | PROCPORT_RAM_CHARROM;
    memcpy(CHARSET_SHADOW, CHARGEN + ROM_CHARSET_MIXED_OFFSET, CHARSET_SIZE);
    *PROCPORT = old_port;
    irq_enable();    // KERNAL ROM is mapped back in, safe to re-enable IRQs
}

void setup_editor_vicii() {
    irq_disable();    // disable before mapping out KERNAL ROM
    set_ram_with_io();
    vicSelectGfxBank(EDITOR_SCREEN);
    *D011 = (*D011 & VICII_RST8) | VICII_DEN | VICII_RSEL | VICII_ECM | 3;
    *D016 = VICII_CSEL;
    *D018 = toD018(EDITOR_SCREEN, CHARSET_SHADOW);
    *BORDER_COLOR = 0;
    *BG_COLOR = 1;
}

void draw_tile_block_from_table(char map_left, char map_top, char mx, char my, const char* table, unsigned int segment_stride, char segment_column, char block_height, char block_width) {
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
    char sx = map_left + mx;
    char sy = map_top + my;
    char* scr = conio_screen_base + ((unsigned int)(unsigned char)sy) * 40u + (unsigned int)(unsigned char)sx;

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


void draw_lane_finish_12_13(char map_left, char map_top, char mx, char my) {
    char sx = map_left + mx;
    char sy = map_top + my;
    char* scr = conio_screen_base + ((unsigned int)(unsigned char)sy) * 40u + (unsigned int)(unsigned char)sx;
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

void clear_road_matrix_tile0(char map_left, char map_top) {
    char* row = conio_screen_base + ((unsigned int)(unsigned char)map_top) * 40u + (unsigned int)(unsigned char)map_left;
    for(char y = 0; y < MAP_HEIGHT; y++) {
        memset(row, 0, (unsigned int)(unsigned char)MAP_WIDTH);
        row += 40;
    }
}

void draw_height_indicator_column(char map_left, char map_top, char mx, char floor_y, char height) {
    if(height == 0) {
        return;
    }

    if(mx < 0 || mx >= MAP_WIDTH) {
        return;
    }

    char sx = map_left + mx;
    char my = floor_y;
    char sy = map_top + my;
    char* scr = conio_screen_base + ((unsigned int)(unsigned char)sy) * 40u + (unsigned int)(unsigned char)sx;

    for(char i = 0; i < height; i++) {
        if(my >= 0 && my < MAP_HEIGHT) {
            *scr = 50;
        }

        my -= 1;
        scr -= 40;
    }
}

char draw_road_3x6_from_heights(char map_left, char map_top, const char* heights7) {
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


    // Align initial projection using the last height in the 6-segment block.
    char base_y = (char)(4 - heights7[6]);
    char base_x = 10;

    // Right-most height column (h6): up to 4 indicators from floor upwards.
    // draw_height_indicator_column(map_left, map_top, base_x + 8, base_y + 4, heights7[6]);

    for(char count = 0; count < 6; count++) {
        signed char delta = deltas[(unsigned char)count];
        char mx_left = base_x;
        char my_left = base_y;
        char mx_mid = base_x + 2;
        char my_mid = base_y + 1;
        char mx_right = base_x + 4;
        char my_right = base_y + 2;

        if(count == 0) {
            draw_lane_finish_12_13(map_left, map_top, mx_left + 2, my_left);
            draw_lane_finish_12_13(map_left, map_top, mx_mid + 2, my_mid);
            draw_lane_finish_12_13(map_left, map_top, mx_right + 2, my_right);
        }

        switch(delta) {
            case 0:
                draw_tile_block_from_table(map_left, map_top, mx_left, my_left, flat_segment_4x2_table, 8u, 2, 2, 4);
                draw_tile_block_from_table(map_left, map_top, mx_mid, my_mid, flat_segment_4x2_table, 8u, 1, 2, 4);
                draw_tile_block_from_table(map_left, map_top, mx_right, my_right, flat_segment_4x2_table, 8u, 0, 2, 4);
                base_y += 1;
                break;

            case 1:
                draw_tile_block_from_table(map_left, map_top, mx_left, my_left, incline_right_one_segment_3x4_table, 12u, 0, 3, 4);
                draw_tile_block_from_table(map_left, map_top, mx_mid, my_mid, incline_right_one_segment_3x4_table, 12u, 1, 3, 4);
                draw_tile_block_from_table(map_left, map_top, mx_right, my_right, incline_right_one_segment_3x4_table, 12u, 2, 3, 4);
                base_y += 2;
                break;

            case -2:
                draw_tile_block_from_table(map_left, map_top, mx_left, my_left - 1, decline_right_two_segment_1x2_table, 2u, 0, 1, 2);
                draw_tile_block_from_table(map_left, map_top, mx_right + 2, my_right, decline_right_two_segment_1x2_table, 2u, 3, 1, 2);
                base_y -= 1;
                break;

            case -1:
                draw_tile_block_from_table(map_left, map_top, mx_left, my_left, decline_right_one_segment_1x2_table, 2u, 0, 1, 2);
                draw_tile_block_from_table(map_left, map_top, mx_mid, my_mid, decline_right_one_segment_1x2_table, 2u, 1, 1, 2);
                draw_tile_block_from_table(map_left, map_top, mx_right, my_right, decline_right_one_segment_1x2_table, 2u, 2, 1, 2);
                base_y += 0;
                break;

            case 2:
                draw_tile_block_from_table(map_left, map_top, mx_left, my_left, incline_right_two_segment_3x4_table, 16u, 0, 4, 4);
                draw_tile_block_from_table(map_left, map_top, mx_mid, my_mid, incline_right_two_segment_3x4_table, 16u, 1, 4, 4);
                draw_tile_block_from_table(map_left, map_top, mx_right, my_right, incline_right_two_segment_3x4_table, 16u, 2, 4, 4);
                base_y += 3;
                break;

            default:
                return 0;
        }

        base_x -= 2;

        // After each segment draw next height column (h5..h0).
        // For -2 decline, only (height-1) indicators fit from the floor.
        char next_height = heights7[(unsigned char)(5 - count)];
        if(delta == -2) {
            if(next_height > 0) {
                next_height -= 1;
            }
        }
        draw_height_indicator_column(map_left, map_top, base_x + 8, count + 7, next_height);
    }

    return 1;
}
 
void draw_road_test_case(char map_left, char map_top, const char* heights7, char wait_after) {
    clear_road_matrix_tile0(map_left, map_top);
    draw_road_3x6_from_heights(map_left, map_top, heights7);
    if(wait_after != 0) {
        wait_keypress();
    }
}

void main()
{
    clrscr();
    cputsxy(0, 0, "moving screen...");

    save_machine_state();
    cputsxy(0, 1, "save machine state done...");

    copy_rom_charset_to_ram();
    cputsxy(0, 2, "copy charset done...");

    load_road_bin();
    setup_editor_vicii();

    conio_screen_base = EDITOR_SCREEN;
    gotoxy(0, 0);
    clear_editor_screen_tile0();
    cputsxy(0, 0, "charset geladen");

    char map_left = (40 - MAP_WIDTH) / 2;
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

