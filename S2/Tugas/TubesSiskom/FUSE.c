#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>

char dir_list[256][256];
int curr_dir_idx = -1;

char files_list[256][256];
int curr_file_idx = -1;

char *files_content[256];
int curr_file_content_idx = -1;

void add_dir(const char *dir_name)
{
    curr_dir_idx++;
    strcpy(dir_list[curr_dir_idx], dir_name);
}

int is_dir(const char *path)
{
    path++;

    for (int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++)
        if (strcmp(path, dir_list[curr_idx]) == 0)
            return 1;

    return 0;
}

void add_file(const char *filename)
{
    curr_file_idx++;
    strcpy(files_list[curr_file_idx], filename);

    curr_file_content_idx++;
    files_content[curr_file_content_idx] = (char *)malloc(256);
    strcpy(files_content[curr_file_content_idx], "");
}


int is_file(const char *path)
{
    path++;

    for (int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++)
        if (strcmp(path, files_list[curr_idx]) == 0)
            return 1;

    return 0;
}

int get_file_index(const char *path)
{
    path++;

    for (int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++)
        if (strcmp(path, files_list[curr_idx]) == 0)
            return curr_idx;

    return -1;
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    path++;
    add_file(path);

    return 0;
}

void write_to_file(const char *path, const char *new_content)
{
    int file_idx = get_file_index(path);

    if (file_idx == -1)
        return;

    strcpy(files_content[file_idx], new_content);
}

static int do_getattr(const char *path, struct stat *st)
{
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);

    if (strcmp(path, "/") == 0 || is_dir(path) == 1)
    {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    }
    else if (is_file(path) == 1)
    {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 1024;
    }
    else
    {
        return -ENOENT;
    }

    return 0;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    if (strcmp(path, "/") == 0)
    {
        for (int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++)
            filler(buffer, dir_list[curr_idx], NULL, 0);

        for (int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++)
            filler(buffer, files_list[curr_idx], NULL, 0);
    }

    return 0;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int file_idx = get_file_index(path);

    if (file_idx == -1)
        return -ENOENT;

    char *content = files_content[file_idx];
    size_t content_size = strlen(content);

    if (offset >= content_size)
    {
        return 0;
    }

    size_t bytes_to_read = (size < content_size - offset) ? size : (content_size - offset);
    memcpy(buffer, content + offset, bytes_to_read);

    return bytes_to_read;
}

static int do_mkdir(const char *path, mode_t mode)
{
    if (is_dir(path)) {
        // Directory already exists
        return -EEXIST;
    }

    add_dir(path);

    return 0;
}



static int do_mknod(const char *path, mode_t mode, dev_t rdev)
{
    path++;
    add_file(path);

    return 0;
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info)
{
    int file_idx = get_file_index(path);

    if (file_idx == -1)
        return -ENOENT;

    char *content = files_content[file_idx];
    size_t content_size = strlen(content);

    if (offset + size > content_size)
    {
        content_size = offset + size;
        content = realloc(content, content_size);
        if (!content)
        {
            return -ENOMEM;
        }
        strcpy(files_content[file_idx], content);
    }

    memcpy(content + offset, buffer, size);

    return size;
}

static struct fuse_operations operations = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .read = do_read,
    .mkdir = do_mkdir,
    .mknod = do_mknod,
    .write = do_write,
    .create = do_create,
};

int main(int argc, char *argv[])
{

    fuse_main(argc, argv, &operations, NULL);

    return 0;
}