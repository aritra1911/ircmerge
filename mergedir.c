#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

int mergedir(const char* src_path, const char* dest_path) {
    // recursively copy src_path to dest_path, changing or making directories as needed.
    // TODO: In case of a conflict call a callback function that merges two files together somehow.

    DIR* dfd;
    struct dirent* dp;
    struct stat stbuf;
    char path[NAME_MAX+1];

    errno = 0;  // Initialize errno
    if (!(dfd = opendir(src_path))) {
        perror("opendir");
        return -1;  // Notify caller that something went wrong!
    }

    errno = 0;
    while (dp = readdir(dfd)) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;  // Skip self & parent directories

        sprintf(path, "%s/%s", src_path, dp->d_name);  // TODO: Hardcoding '/'?

        // path holds absolute path to dp
        if (stat(path, &stbuf) == -1) {
            perror("stat");
            return -1;
        }

        if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
            printf("[D] %s\n", path);  // Is a directory
        else
            printf("[F] %s\n", path);  // Is a file

    }

    if (errno) {  // If errno modified, something went wrong
        perror("readdir");
        return -1;
    }

    return 0;
}

// Testing only, TODO: remove
int main(int argc, char* argv[]) {
    if(mergedir(argv[1], argv[2]) == -1)
        return 1;
    return 0;
}
