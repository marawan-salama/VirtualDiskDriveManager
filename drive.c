/* Marawan Salama**/
#include "drive.h"
#include <stdlib.h>
#include <string.h>

Drive* newDrive() {
		Drive* d = malloc(sizeof(Drive));
		if (d == NULL) {
				return NULL; // Allocation failed
		}

		for (int i = 0; i < 16; i++) {
				d->block[i] = malloc(16 * sizeof(unsigned char));
				if (d->block[i] == NULL) {
						// Allocation failed, clean up previously allocated blocks
						for (int j = 0; j < i; j++) {
								free(d->block[j]);
						}
						free(d);
						return NULL;
				}
		}

		// Initialize the root directory block
		d->block[0][0] = 0x01; // Bitmap indicating block usage
		d->block[0][1] = 0x00; // Bitmap for directory entries

		// Initialize the rest of the block
		for (int i = 2; i < 16; i++) {
				d->block[0][i] = 0x00;
		}

		// Initialize other blocks, if necessary
		for (int i = 1; i < 16; i++) {
				memset(d->block[i], 0, 16); // Fill each block with zeros
		}

		return d;
}

void deleteDrive(Drive* d) {
		if (d != NULL) {
				for (int i = 0; i < 16; i++) {
						free(d->block[i]);
				}
				free(d);
		}
}

unsigned char* dump(Drive* d) {
		unsigned char* ret = malloc(256);
		if (ret == NULL) {
				return NULL; // Allocation failed
		}

		for (int i = 0; i < 16; i++) {
				for (int j = 0; j < 16; j++) {
						ret[16*i + j] = d->block[i][j];
				}
		}
		return ret;
}

char* displayDrive(Drive* d) {
		const int RETSIZE = 1024;
		char* temp = malloc(RETSIZE);
		if (temp == NULL) {
				return NULL; // Allocation failed
		}

		char* ret = malloc(RETSIZE);
		if (ret == NULL) {
				free(temp);
				return NULL; // Allocation failed
		}

		ret[0] = '\0';
		strncat(ret, "   ", RETSIZE);
		for (int i = 0; i < 16; i++) {
				sprintf(temp, "  %x", i);
				strncat(ret, temp, RETSIZE);
		}
		strncat(ret, "\n", RETSIZE);

		for (int row = 0; row < 16; row++) {
				sprintf(temp, "%x: ", row);
				strncat(ret, temp, RETSIZE);
				for (int col = 0; col < 16; col++) {
						sprintf(temp, "%3.2x", d->block[row][col]);
						strncat(ret, temp, RETSIZE);
				}
				strncat(ret, "\n", RETSIZE);
		}

		free(temp);
		return ret;
}

int isUsed(Drive* d, int pos) {
		unsigned short bitmap = 0;
		if (pos >= 0 && pos < 16) {
				memcpy(&bitmap, d->block[0], 2);
				unsigned short mask = 1 << pos;
				return (mask & bitmap) != 0;
		}
		return 0; // Invalid position
}

