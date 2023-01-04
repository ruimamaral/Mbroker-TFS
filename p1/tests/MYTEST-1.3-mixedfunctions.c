#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>


#define FILE_COUNT 5
#define MAX_PATH_SIZE 32
#define MAX_OPEN_FILES 3000


char *path1 = "/f1";
char *path2 = "/f2";
char *target = "/target";
char *link_name = "/l1";
char *text = "test";
pthread_t tdn[3];





tfs_params tfs_test_params() {
    tfs_params params = {
        .max_inode_count = 600,
        .max_block_count = 1024,
        .max_open_files_count = MAX_OPEN_FILES,
        .block_size = 2000000,
    };
    return params;
}

//Continously writes in a file
void* writing_func() {
	for(int j = 0 ; j < 100 ; j++){
		int f = tfs_open(path1, TFS_O_CREAT | TFS_O_TRUNC);
		assert(f!=-1);
		assert(tfs_close(f) != -1);
		assert(tfs_unlink(path1) != -1);
		
	}
	return 0;
}

// Opens previously created file,reads its contents and then closes the file
void* reading_func() {
	char buffer[10];
	ssize_t bytes_read;
	for(int j = 0 ; j < 100 ; j++){
		int f = tfs_open(target, TFS_O_CREAT );
		assert(f!=-1);
		bytes_read = tfs_read(f,buffer,sizeof(buffer)-1);
		assert(bytes_read == strlen(text));
		assert(tfs_close(f) != -1);
		
	}
	return 0;
}

// Creates a symlink to a previously created file,reads its contents and then unlinks the symbolic link
void* symlinks_func() {
	char buffer[10];
	ssize_t bytes_read;
	int f;
	for(int j = 0 ; j < 100 ; j++){
		assert(tfs_sym_link(target,link_name) != -1);
		assert((f = tfs_open(link_name, TFS_O_CREAT)) != -1);
		bytes_read = tfs_read(f,buffer,sizeof(buffer)-1);
		assert(bytes_read == strlen(text));
		assert(tfs_close(f) != -1);
		assert(tfs_unlink(link_name) != -1);
	}
	return 0;
}

int main() {
	tfs_params params = tfs_test_params();
	assert(tfs_init(&params) != -1);
	int f;
	ssize_t bytes;


	f = tfs_open(target, TFS_O_CREAT);
	assert(f != -1);
	bytes = tfs_write(f,text,strlen(text));
	assert(bytes  == strlen(text));
	assert(tfs_close(f) != -1);


	pthread_create(&tdn[0],NULL,&writing_func,NULL);
	pthread_create(&tdn[2],NULL,&symlinks_func,NULL); 
	pthread_create(&tdn[1],NULL,&reading_func,NULL);

	for(int i = 0 ; i< 3 ; i++){
		pthread_join(tdn[i],NULL);
	}


	/* assert(pthread_create(&tdi, NULL, &reading_func, NULL) == 0); */

    printf("Successful test.\n");

    return 0;
}
