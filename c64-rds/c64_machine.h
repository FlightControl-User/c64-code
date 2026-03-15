#ifndef C64_MACHINE_H
#define C64_MACHINE_H

#include <c64.h>

extern char zp_backup[254];
extern char saved_procport;
extern char saved_cia2_porta;
extern char saved_d011;
extern char saved_d016;
extern char saved_d018;

void machine_save_state();
void restore_machine_state();

#endif
