#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

int main() {
    char *path1 = "/f1";
	char *text = "ThisLine->";
	char read_buffer[1025];
	pthread_t tdi;
	pthread_t tdn;



	assert(tfs_init(NULL) != -1);

	void* writing_func() {
		int f = tfs_open(path1, TFS_O_CREAT | TFS_O_APPEND);
		ssize_t bytes_written;
		char buffer[12];
		assert(f!=-1);
		for(int i = 0; i < 20 ; i++){
			if((bytes_written = tfs_write(f,buffer,strlen(text))) == -1){
				assert(bytes_written != -1);
			}
		}
		tfs_close(f);
		return 0;
	}

	void* reading_func() {
		int f = tfs_open(path1, TFS_O_CREAT | TFS_O_APPEND);
		assert(f!=-1);
		ssize_t bytes_read;
		char buffer[50];
		while((bytes_read = tfs_read(f,buffer,1)) > 0){}
		tfs_close(f);
		return 0;
	}
	

    assert(pthread_create(&tdi, NULL, &writing_func, NULL) != -1);

	for(int i = 0 ; i< 100; i++) {
		pthread_create(&tdn,NULL,reading_func,NULL);
	}

	for(int i = 0 ; i< 100 ; i++){
		pthread_join(tdn,NULL);
	}

	int f;
	ssize_t r;
	f = tfs_open(path1, TFS_O_TRUNC);
    assert(f != -1);

	

    r = tfs_read(f, read_buffer, sizeof(read_buffer) - 1);
    assert(r == (strlen(text)*20));

	/* assert(pthread_create(&tdi, NULL, &reading_func, NULL) == 0); */

    printf("Successful test.\n");

    return 0;
}
