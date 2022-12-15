#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>

int main() {
    char *path1 = "/f1";

    /* Tests scenarios where tfs_copy_from_external_fs is expected to
     * fail */

    assert(tfs_init(NULL) != -1);

    int f1 = tfs_open(path1, TFS_O_CREAT);
    assert(f1 != -1);
    assert(tfs_close(f1) != -1);

    // Scenario 1: dest path does not exist
	assert(tfs_copy_from_external_fs(path1,"/unexistent") == -1);

    printf("Successful test.\n");

    return 0;
}