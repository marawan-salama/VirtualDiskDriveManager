/* Marawan Salama**/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOCK_SIZE 16
#define TOTAL_BLOCKS 16
#define TOTAL_DISK_SIZE (BLOCK_SIZE * TOTAL_BLOCKS)

typedef struct {
	unsigned char bitmap;
	unsigned char names[8];
	unsigned char block_pointers[8];
} Directory;

typedef struct {
	unsigned char size;
	unsigned char block_pointers[15];
} FileDescriptor;

int disk_fd = -1;

// Function Prototypes
void initialize_disk(const char *diskfile);
void read_block(int block, void *buf);
void write_block(int block, const void *buf);
void handle_import(const char *filepath, const char *tfs_path);
void handle_ls(const char *tfs_path);
void handle_display();
void handle_open(const char *diskfile);
void handle_create(const char *diskfile);
void handle_exit();
void handle_mkdir(const char *tfs_path);
void handle_rm(const char *tfs_path);
void handle_export(const char *tfs_path, const char *local_path);


int main(int argc, char *argv[]) {
	char *diskfile = argc > 1 ? argv[1] : "temp";
	disk_fd = open(diskfile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (disk_fd < 0) {
		perror("Error opening disk file");
		return 1;
	}

	initialize_disk(diskfile);

	char command[256];
	while (1) {
		printf("TFS> ");
		if (!fgets(command, sizeof(command), stdin)) {
			break; // Exit on EOF
		}

		// Remove newline character
		command[strcspn(command, "\n")] = 0;

		char cmd[256], arg1[256], arg2[256];
		int numArgs = sscanf(command, "%s %s %s", cmd, arg1, arg2);

		if (numArgs == 1) {
			if (strcmp(cmd, "display") == 0) {
				handle_display();
			} else if (strcmp(cmd, "exit") == 0) {
				handle_exit();
				break; // Exit the loop after closing resources
			} else {
				printf("Unknown or incomplete command.\n");
			}
		} else if (numArgs == 2) {
			if (strcmp(cmd, "open") == 0) {
				handle_open(arg1);
			} else if (strcmp(cmd, "create") == 0) {
				handle_create(arg1);
			} else if (strcmp(cmd, "ls") == 0) {
				handle_ls(arg1);
			} else if (strcmp(cmd, "mkdir") == 0) {
				handle_mkdir(arg1);
			} else {
				printf("Unknown command.\n");
			}
		} else if (numArgs == 3) {
			if (strcmp(cmd, "import") == 0) {
				handle_import(arg1, arg2);
			} else {
				printf("Unknown command or incorrect arguments.\n");
			}
		} else {
			printf("Invalid command format.\n");
		}
	}

	close(disk_fd);
	return 0;
}

void initialize_disk(const char *diskfile) {
	if (lseek(disk_fd, 0, SEEK_END) != TOTAL_DISK_SIZE) {
		ftruncate(disk_fd, TOTAL_DISK_SIZE);
	}

	Directory root;
	pread(disk_fd, &root, sizeof(root), 0);
	if (root.bitmap != 0x01) {
		memset(&root, 0, sizeof(root));
		root.bitmap = 0x01;
		pwrite(disk_fd, &root, sizeof(root), 0);
	}
}

void read_block(int block, void *buf) {
	pread(disk_fd, buf, BLOCK_SIZE, block * BLOCK_SIZE);
}

void write_block(int block, const void *buf) {
	pread(disk_fd, buf, BLOCK_SIZE, block * BLOCK_SIZE);
}


int find_file_in_vfs(const char *tfs_path) {
		if (strlen(tfs_path) != 2 || tfs_path[0] != '/') {
				return -1; // Invalid path format
		}

		char fileChar = tfs_path[1];
		Directory rootDir;
		read_block(0, &rootDir); // Assuming block 0 is the root directory

		for (int i = 0; i < 8; i++) {
				if ((rootDir.bitmap & (1 << i)) && rootDir.names[i] == fileChar) {
						return rootDir.block_pointers[i]; // Return the block number
				}
		}

		return -1; // File not found
}


int find_file_or_directory_in_vfs(const char *tfs_path) {
		if (strlen(tfs_path) != 2 || tfs_path[0] != '/') {
				return -1; // Invalid path format
		}

		char entryChar = tfs_path[1];
		Directory rootDir;
		read_block(0, &rootDir);

		for (int i = 0; i < 8; i++) {
				if ((rootDir.bitmap & (1 << i)) && rootDir.names[i] == entryChar) {
						return i; // Return the slot number
				}
		}

		return -1; // Entry not found
}


int is_directory_empty(int slotNum) {
		Directory dir;
		read_block(slotNum, &dir); // Assuming each directory has its own block

		for (int i = 0; i < 8; i++) {
				if (dir.bitmap & (1 << i)) {
						return 0; // Directory is not empty
				}
		}

		return 1; // Directory is empty
}

int is_directory(const char *tfs_path) {
		if (strlen(tfs_path) != 2 || tfs_path[0] != '/') {
				return 0; // Invalid path format or not a directory
		}

		char entryChar = tfs_path[1];
		Directory rootDir;
		read_block(0, &rootDir); // Assuming block 0 is the root directory

		for (int i = 0; i < 8; i++) {
				if ((rootDir.bitmap & (1 << i)) && rootDir.names[i] == entryChar) {
						return 1;
				}
		}

		return 0; // Entry not found or not a directory
}










void handle_import(const char *filepath, const char *tfs_path) {
	char filename = tfs_path[1];
	Directory root;
	read_block(0, &root);

	for (int i = 0; i < 8; i++) {
		if (root.names[i] == filename) {
			printf("File %s already exists in TFS\n", tfs_path);
			return;
		}
	}

	for (int i = 0; i < 8; i++) {
		if ((root.bitmap & (1 << i)) == 0) {
			root.bitmap |= (1 << i);
			root.names[i] = filename;
			root.block_pointers[i] = 0;
			write_block(0, &root);
			printf("Imported %s to %s\n", filepath, tfs_path);
			return;
		}
	}

	printf("No space in root directory to import %s\n", filepath);
}

void handle_ls(const char *tfs_path) {
	Directory root;
	read_block(0, &root);

	for (int i = 0; i < 8; i++) {
		if (root.bitmap & (1 << i)) {
			printf("%c ", root.names[i]);
		}
	}
	printf("\n");
}

void handle_display() {
		unsigned char block[BLOCK_SIZE];

		printf("Displaying the raw contents of the virtual disk:\n");
		for (int i = 0; i < TOTAL_BLOCKS; i++) {
				// Read the content of each block
				read_block(i, block);

				// Display the block number and its contents
				printf("Block %d: ", i);
				for (int j = 0; j < BLOCK_SIZE; j++) {
						printf("%02x ", block[j]);
				}
				printf("\n");
		}
}


void handle_open(const char *diskfile) {
	int new_fd = open(diskfile, O_RDWR);
	if (new_fd < 0) {
		perror("Error opening disk file");
		return;
	}

	if (lseek(new_fd, 0, SEEK_END) != TOTAL_DISK_SIZE) {
		printf("Error: Invalid disk file size\n");
		close(new_fd);
		return;
	}

	if (disk_fd >= 0) {
		close(disk_fd);
	}
	disk_fd = new_fd;
	printf("Opened disk file %s\n", diskfile);
}

void handle_create(const char* filename) {
	// Check if filename is valid (for simplicity, assume single-character filenames)
	if (strlen(filename) != 1) {
			printf("Invalid filename. Only single-character filenames are supported.\n");
			return;
	}

	char file_char = filename[0];

	// Load the root directory (assuming it's already loaded into a variable `rootDir`)
	Directory rootDir;
	read_block(0, &rootDir);  // Assuming block 0 is the root directory

	// Check if file already exists
	for (int i = 0; i < 8; i++) {
			if ((rootDir.bitmap & (1 << i)) && rootDir.names[i] == file_char) {
					printf("File %s already exists in VFS\n", filename);
					return;
			}
	}

	// Find a free slot in the root directory
	int free_slot = -1;
	for (int i = 0; i < 8; i++) {
			if (!(rootDir.bitmap & (1 << i))) {
					free_slot = i;
					break;
			}
	}

	if (free_slot == -1) {
			printf("No space in root directory to create file %s\n", filename);
			return;
	}

	// Update the root directory with the new file
	rootDir.bitmap |= (1 << free_slot);
	rootDir.names[free_slot] = file_char;
	rootDir.block_pointers[free_slot] = 0; // Assuming files are empty initially

	// Write the updated root directory back to the VFS
	write_block(0, &rootDir);

	printf("Created file %s in VFS\n", filename);
}

void handle_exit() {
	if (disk_fd >= 0) {
		close(disk_fd);
	}
	exit(0);
}

void handle_mkdir(const char *tfs_path) {
	// Check for valid path format
		if (strlen(tfs_path) != 2 || tfs_path[0] != '/') {
				printf("Invalid path format.\n");
				return;
		}

		// If the path format is valid, continue with creating the directory
		char dirname = tfs_path[1];

		// Read the root directory
		Directory root;
		read_block(0, &root); // Assuming block 0 is the root

		int free_block = -1;
		for (int i = 0; i < 8; i++) {
				if ((root.bitmap & (1 << i)) && root.names[i] == dirname) {
						printf("Directory %s already exists in TFS\n", tfs_path);
						return;
				}
				if (free_block == -1 && !(root.bitmap & (1 << i))) {
						free_block = i;
				}
		}

		if (free_block == -1) {
				printf("No space in root directory.\n");
				return;
		}

		// Update the root directory with new directory info
		root.bitmap |= (1 << free_block);
		root.names[free_block] = dirname;
		write_block(0, &root); // Assuming 'write_block' function is correctly defined

		printf("Created directory %s\n", tfs_path);
}


void handle_export(const char *tfs_path, const char *local_path) {
		// Find the file in the virtual disk
		int blockNum = find_file_in_vfs(tfs_path); // Implement this function
		if (blockNum == -1) {
				printf("File not found in virtual file system.\n");
				return;
		}

		// Read the file content from the virtual disk
		unsigned char buffer[BLOCK_SIZE];
		read_block(blockNum, buffer);

		// Write the content to a file in the real file system
		int fd = open(local_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (fd < 0) {
				perror("Error creating file in real file system");
				return;
		}

		if (write(fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
				perror("Error writing to file in real file system");
				close(fd);
				return;
		}

		close(fd);
		printf("File exported to %s\n", local_path);
}



void handle_rm(const char *tfs_path) {
		// Check if the path is valid and find the file or directory
		int slotNum = find_file_or_directory_in_vfs(tfs_path); // Implement this function
		if (slotNum == -1) {
				printf("File or directory not found in virtual file system.\n");
				return;
		}

		// Load the root directory
		Directory rootDir;
		read_block(0, &rootDir);

		// Check if it's a directory and if it's empty
		if (is_directory(tfs_path) && !is_directory_empty(slotNum)) { // Implement these functions
				printf("Directory is not empty.\n");
				return;
		}

		// Update the root directory bitmap to remove the file or directory
		rootDir.bitmap &= ~(1 << slotNum);
		write_block(0, &rootDir);

		printf("%s removed from virtual file system.\n", tfs_path);
}
