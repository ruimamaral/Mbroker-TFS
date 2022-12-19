#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

int main() {
    char *path1 = "/f1";
	char *text = "ThisLine->";
	char read_buffer[100];
	pthread_t tdi[1000];
	pthread_t tdn[1000];


void* mega_test() {
	int f = tfs_open(path1, TFS_O_CREAT);
	assert(f!=-1);
	ssize_t bytes_read;
	char buffer[50];
	while((bytes_read = tfs_read(f,buffer,sizeof(buffer)) > 0)){

	}
	tfs_close(f);
	return 0;
}
 

	for(int i = 0 ; i< 1000; i++) {
		pthread_create(&tdn[i],NULL,&writing_func,NULL);
		pthread_create(&tdi[i],NULL,&reading_func,NULL);
	}


	for(int i = 0 ; i< 1000 ; i++){
		pthread_join(tdi[i],NULL);
		pthread_join(tdn[i],NULL);
	}



	int fa;
	ssize_t r;

	fa = tfs_open(path1, TFS_O_CREAT);
    assert(fa != -1);

	r = tfs_read(fa, read_buffer, sizeof(read_buffer)-1);
	assert(r == strlen(text)*2);



	/* assert(pthread_create(&tdi, NULL, &reading_func, NULL) == 0); */

    printf("Successful test.\n");

    return 0;
}
