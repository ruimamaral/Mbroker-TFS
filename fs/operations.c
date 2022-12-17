#include "operations.h"
#include "config.h"
#include "state.h"
#include "locks.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "betterassert.h"

// Locks functionalities that involve creating new files
static pthread_mutex_t root_lock;

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }
	mutex_init(&root_lock);

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    // TODO: assert that root_inode is the root directory
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

// Move to state.c
int get_symlink_inumber(int inum, inode_t *dir_inode) {
	inode_t *inode = inode_get(inum);

    ALWAYS_ASSERT(dir_inode != NULL,
                  "get_symlink_inumber: invalid directory inode");
    ALWAYS_ASSERT(inode != NULL,
                  "get_symlink_inumber: inode must exist");
	ALWAYS_ASSERT(inode->i_node_type == T_SYMLINK,
			"get_symlink_inumber: inode must represent a symlink");

	int file_inumber; 
	char const *name = (char const*)data_block_get(inode->i_data_block);
	if ((file_inumber = tfs_lookup(name, dir_inode)) == ERROR_VALUE) {
      	return ERROR_VALUE;
   	}
	return file_inumber;
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

	//Lock table if needed (both tables)
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
	mutex_lock(&root_lock);
    int inum = tfs_lookup(name, root_dir_inode);
    size_t offset;

    if (inum >= 0) {
        // The file already exists
        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

		if (inode->i_node_type == T_SYMLINK) {
			// get actual file inode
			if ((inum = get_symlink_inumber(inum, root_dir_inode)) == ERROR_VALUE) {
				mutex_unlock(&root_lock);
      			return ERROR_VALUE;
   			}
			// We already know the inode exists
			inode = inode_get(inum);
		}
		
        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
			mutex_unlock(&root_lock);
            return -1; // no space in inode table
        }

        // Add entry in the root directory
		int valid = add_dir_entry(root_dir_inode, name + 1, inum);
		mutex_unlock(&root_lock);
        if (valid == -1) {
            inode_delete(inum);
			mutex_unlock(&root_lock);
            return -1; // no space in directory
        }

        offset = 0;
    } else {
		mutex_unlock(&root_lock);
        return -1;
    }

	mutex_unlock(&root_lock);

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
	inode_t *link_inode;
	int link_inumber;
	inode_t *dir_inode = inode_get(ROOT_DIR_INUM);
	void *data;

	mutex_lock(&root_lock);

	if (tfs_lookup(link_name, dir_inode) != ERROR_VALUE) {
		fprintf(stderr, "directory lookup error: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
   	}

	if (tfs_lookup(target, dir_inode) == ERROR_VALUE) {
		fprintf(stderr, "directory lookup error: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
   	}
	
	if ((link_inumber = inode_create(T_SYMLINK)) == ERROR_VALUE) {
		fprintf(stderr, "data block cannot be allocated: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
	}
	link_inode = inode_get(link_inumber);
	data = data_block_get(link_inode->i_data_block);
	strcpy(data, target);

	if(add_dir_entry(dir_inode, link_name + 1, link_inumber) == ERROR_VALUE) {
		fprintf(stderr, "add directory entry error: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
   	}
	mutex_unlock(&root_lock);
	return SUCCESS_VALUE;
}

int tfs_link(char const *target, char const *link_name) {
	inode_t *target_inode;
	int target_inumber;
	inode_t *dir_inode = inode_get(ROOT_DIR_INUM);

	mutex_lock(&root_lock);

	if (tfs_lookup(link_name, dir_inode) != ERROR_VALUE) {
		fprintf(stderr, "directory lookup error: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
   	}

	if ((target_inumber = tfs_lookup(target,dir_inode)) == ERROR_VALUE) {
		fprintf(stderr, "directory lookup error: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
   	}

	target_inode = inode_get(target_inumber);

	if(target_inode->i_node_type == T_SYMLINK) {
		fprintf(stderr, "cannot hardlink a symlink: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
	}

	if(add_dir_entry(dir_inode,link_name+1,target_inumber) == ERROR_VALUE) {
		fprintf(stderr, "add directory entry error: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
   	}

	target_inode->i_hard_links++;
	mutex_unlock(&root_lock);
	return SUCCESS_VALUE;
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

	int inumber = file->of_inumber;

    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");
	// Lock after verifying that the inumber is valid
	rwlock_wrlock(&inode_locks[inumber]);

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
				rwlock_unlock(&inode_locks[inumber]);
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

	rwlock_unlock(&inode_locks[inumber]);
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }
	int inumber = file->of_inumber;

    // From the open file table entry, we get the inode
    inode_t const *inode = inode_get(inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");
	// Lock after verifying that the inumber is valid
	rwlock_rdlock(&inode_locks[inumber]);

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }

	rwlock_unlock(&inode_locks[inumber]);
    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    inode_t* dir_inode = inode_get(ROOT_DIR_INUM);
	inode_t* target_inode;
	inode_type type;
	int target_inumber;
	mutex_lock(&root_lock);
	if ((target_inumber = tfs_lookup(target,dir_inode)) == ERROR_VALUE) {
		fprintf(stderr, "directory lookup error: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
   	}

	target_inode = inode_get(target_inumber);
	if( target_inode->i_node_type == T_DIRECTORY) {
		fprintf(stderr, "cannot unlink root directory: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
		return ERROR_VALUE;
	}

	type = target_inode->i_node_type;
	int hardlinks = --target_inode->i_hard_links;

	if (type == T_SYMLINK) {
		inode_delete(target_inumber);
	}
	else{
		if (hardlinks == 0) {
			inode_delete(target_inumber);
		}
	}
	if (clear_dir_entry(dir_inode,target+1) == ERROR_VALUE) {
		fprintf(stderr, "clear directory entry error: %s\n", strerror(errno));
		mutex_unlock(&root_lock);
      	return ERROR_VALUE;
   	}

	mutex_unlock(&root_lock);
	return SUCCESS_VALUE;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
	size_t bytes_read;
	ssize_t bytes_written;
	
	FILE* source_file = fopen(source_path, "r");
	if (!source_file) {
    	fprintf(stderr, "open error: %s\n", strerror(errno));
      	return ERROR_VALUE;
   	}

	int tfs = tfs_open(dest_path, TFS_O_CREAT | TFS_O_TRUNC);
	if (tfs == ERROR_VALUE) {                                    
		fclose(source_file);
    	fprintf(stderr, "open error: %s\n", strerror(errno));
      	return ERROR_VALUE;
   	}

	char buffer[128];
   	memset(buffer,MEMSET_VALUE,sizeof(buffer));
	while ((bytes_read = fread(buffer, sizeof(char),
			sizeof(buffer)/sizeof(char)-1, source_file)) > 0) {

		bytes_written = tfs_write(tfs, buffer,bytes_read );
		if (bytes_written == ERROR_VALUE || bytes_written != bytes_read) {
			tfs_close(tfs);
			fclose(source_file);
			fprintf(stderr, "writing error: %s\n", strerror(errno));
      		return ERROR_VALUE;
		}
	}

	if (tfs_close(tfs) == ERROR_VALUE) {
		fclose(source_file);
    	fprintf(stderr, "close error: %s\n", strerror(errno));
      	return ERROR_VALUE;
   	}

	if (fclose(source_file) == ERROR_VALUE) {
    	fprintf(stderr, "close error: %s\n", strerror(errno));
      	return ERROR_VALUE;
   	}

	return SUCCESS_VALUE;
}
