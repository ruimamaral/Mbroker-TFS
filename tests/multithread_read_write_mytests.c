#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

char *path1 = "/f1";
char *text = "test";
char read_buffer[1000];
pthread_t tdn[1000];





void* writing_func() {
	int f = tfs_open(path1, TFS_O_CREAT | TFS_O_APPEND);
	ssize_t bytes_written;
	assert(f!=-1);
	for(int i = 0; i < 2 ; i++){
		if((bytes_written = tfs_write(f,text,strlen(text))) == -1){
			assert(bytes_written != -1);
		}
	}
	tfs_close(f);
	return 0;
}

int main() {
	assert(tfs_init(NULL) != -1);


	for(int i = 0 ; i< 100; i++) {
		pthread_create(&tdn[i],NULL,&writing_func,NULL);
	}

	for(int i = 0 ; i< 100 ; i++){
		pthread_join(tdn[i],NULL);
	}

	int fa;
	ssize_t r;

	fa = tfs_open(path1, TFS_O_CREAT);
    assert(fa != -1);

	r = tfs_read(fa, read_buffer, sizeof(read_buffer)-1);
	assert(r == strlen(text)*2*100);



	/* assert(pthread_create(&tdi, NULL, &reading_func, NULL) == 0); */

    printf("Successful test.\n");

    return 0;
}
