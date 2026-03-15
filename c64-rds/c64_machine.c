#pragma lib_configure

#pragma lib_export(__varcall, machine_save_state, restore_machine_state)

#include "c64_machine.h"
#include <6502.h>
#include <c64.h>

char zp_backup[254];
char saved_procport;
char saved_cia2_porta;
char saved_d011;
char saved_d016;
char saved_d018;

void machine_save_state() {
    SEI();
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
    CLI();
}

void restore_machine_state() {
    SEI();
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
    CLI();
}
