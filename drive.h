/* Marawan Salama**/
#ifndef DRIVE_H
#define DRIVE_H

typedef struct {
		unsigned char* block[16];
} Drive;

Drive* newDrive();
void deleteDrive(Drive* d);
unsigned char* dump(Drive* d);
char* displayDrive(Drive*);
int isUsed(Drive* d, int pos);

#endif // DRIVE_H
