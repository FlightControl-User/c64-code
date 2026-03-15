#ifndef C64_DISK_H
#define C64_DISK_H

void cbm_setnam(char* filename);
void cbm_setlfs(char lfn, char device, char sa);
char cbm_load(char* address, char verify);

#endif