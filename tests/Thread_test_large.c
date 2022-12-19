#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_FORMAT "/f%d"
#define FILE_COUNT 5
#define MAX_PATH_SIZE 32

char *str_ext_file = "BBB!";
char *path_src = "tests/file_to_copy.txt";
char buffer[40];
char* names[50];


void* mega_test(void* name ) {
	int f;
	assert(tfs_copy_from_external_fs(path_src,(char*)name) != -1);
	assert((f = tfs_open((char*)name,TFS_O_CREAT)) != -1);
	tfs_read(f, buffer, sizeof(buffer) - 1);
	printf("going to close %s\n", (char*)name);
	assert(tfs_close(f) != -1);
	tfs_unlink(name);
	return 0;
}

int main() {
    assert(tfs_init(NULL) != -1);
	pthread_t tdn[400];
	for(int i = 0 ; i < 50 ; i++){
		char *name = malloc(sizeof(char)*MAX_PATH_SIZE);
		snprintf(name, 5 , PATH_FORMAT, i);
		names[i] = name;
	}

	for(int i = 0 ; i< 50; i++) {

		for(int j = 0; j < 3 ; j++){
			pthread_create(&tdn[3*i+j],NULL,&mega_test,(void*)names[i]);
		}
	}	

	for(int i = 0 ; i< 150 ; i++){
		pthread_join(tdn[i],NULL);
	}



	/* assert(pthread_create(&tdi, NULL, &reading_func, NULL) == 0); */

    printf("Successful test.\n");

    return 0;
}
