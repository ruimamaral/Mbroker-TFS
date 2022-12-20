#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_FORMAT "/l%d"
#define MAX_PATH_SIZE 32

char* text = "AAA!";
char* target_path1 = "/f1";
char* link_path0 = "/l0";
char* last_path = "/l20";

int main() {
    assert(tfs_init(NULL) != -1);
	int f;
	ssize_t bytes;
	char buffer[30];
	char *str_ext_file = "BBB!";
    char *path_src = "tests/file_to_copy.txt";



    // Write to symlink and read original file
    {
        f = tfs_open(target_path1, TFS_O_CREAT);
        assert(f != -1);
		bytes = tfs_write(f,text,strlen(text));
		assert(bytes != -1);
        assert(tfs_close(f) != -1);

    }

	// Reads from the Symlink and atempts to unlink a open file(suposed to return -1)
	{
		assert(tfs_sym_link(target_path1, link_path0) != -1);
		f = tfs_open(target_path1,TFS_O_CREAT);
		assert(f != -1);

		assert(tfs_unlink(target_path1) == -1);

		assert(tfs_read(f,buffer,sizeof(buffer)-1) == bytes);
		assert(strcmp(buffer,text) == 0);
		assert(tfs_close(f) != -1);
	}

    // Create multiple symlinks and read through the last one
    {
		for (int i = 1 ; i<= 20 ; i++) {
			char* link_path1 = malloc(sizeof(char)*MAX_PATH_SIZE);
			char* link_path2 = malloc(sizeof(char)*MAX_PATH_SIZE);
			snprintf(link_path1, 5 , PATH_FORMAT, i-1);
			snprintf(link_path2, 5 , PATH_FORMAT, i);

			assert(tfs_sym_link(link_path1,link_path2) != -1);

			f = tfs_open(link_path2,TFS_O_CREAT);
			assert(f != -1);
			assert(tfs_read(f,buffer,sizeof(buffer)-1) == bytes);
			assert(tfs_close(f) != -1);
			assert(strcmp(buffer,text) == 0);
		}
        assert(tfs_copy_from_external_fs(path_src,last_path) != -1);

		f = tfs_open(target_path1,TFS_O_CREAT);
		assert(f != -1);
		assert(tfs_read(f,buffer,sizeof(buffer)-1) == bytes);
		assert(tfs_close(f) != -1);
		assert(strcmp(buffer,str_ext_file) == 0);

    }

    assert(tfs_destroy() != -1);
    printf("Successful test.\n");

    return 0;
}
