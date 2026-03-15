
#pragma lib_configure
#pragma lib_export(__varcall, cbm_setnam, cbm_setlfs, cbm_load)

#include "c64_disk.h"
#include <string.h>

void cbm_setnam(char* filename) {
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
    (void)filename;
#endif
}

void cbm_setlfs(char lfn, char device, char sa) {
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

char cbm_load(char* address, char verify) {
    char status;
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
#else
    (void)address;
    (void)verify;
    status = 0;
#endif
    return status;
}

