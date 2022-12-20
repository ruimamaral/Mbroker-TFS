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
#define MAX_OPEN_FILES 3000

char *text = "BBB!";
char *target = "/target";
char *path_src = "tests/file_to_copy.txt";


tfs_params tfs_test_params() {
    tfs_params params = {
        .max_inode_count = 300,
        .max_block_count = 1024,
        .max_open_files_count = MAX_OPEN_FILES,
        .block_size = 1000000,
    };
    return params;
}


void* hardlinks_func(void* j ) { 
	char name[7];
	int number = *(int*)j;
	snprintf(name,sizeof(name),PATH_FORMAT,number);
	for (int i = 0 ; i < 50 ; i++){
		assert(tfs_sym_link(target,name) != -1);
		assert(tfs_unlink(name) != -1);
		assert(tfs_link(target,name) != -1);
		assert(tfs_unlink(name) != -1);
	}
	return 0;
}


int main() {
	printf("Test takes a while...\n");
	tfs_params params = tfs_test_params();
	assert(tfs_init(&params) != -1);
	pthread_t tdn[100];
	int par[100];

	int f;
	ssize_t r;
	char buffer[20];
	f = tfs_copy_from_external_fs(path_src, target);
    assert(f != -1);
	assert((f = tfs_open(target,TFS_O_CREAT)) != -1 );
	assert( (r = tfs_read(f,buffer,sizeof(buffer)-1)) == strlen(text) );
	assert((tfs_close(f)) != -1);

	for(int i = 0 ; i < 100; i++){
		par[i]=i;
		pthread_create(&tdn[i],NULL,&hardlinks_func,(void*)&par[i]);
	}

	for(int i = 0 ; i< 100; i++){
		pthread_join(tdn[i],NULL);
	}

    printf("Successful test.\n");

    return 0;
}
