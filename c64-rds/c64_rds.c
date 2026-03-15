#pragma link("c64_rds.ld")

#include "c64_rds.h"

char load_road_bin() {
    char* const ROAD_DATA = (char*)0xc800;
    cbm_setnam("ROAD.BIN");
    cbm_setlfs(1, 8, 0);
    return cbm_load(ROAD_DATA, 0);
}

void main() {
    draw_clear_screen();
    draw_status_xy(0, 0, "moving screen...");

    machine_save_state();
    draw_status_xy(0, 1, "save machine state done...");

    copy_rom_charset_to_ram();
    draw_status_xy(0, 2, "copy charset done...");

    load_road_bin();
    setup_editor_vicii();

    editor_run();

    restore_machine_state();
}
