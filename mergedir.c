#include <stdio.h>
#include <errno.h>
#include <dirent.h>

int mergedir(const char* src_path, const char* dest_path) {
    // recursively copy src_path to dest_path, changing or making directories as needed.
    // TODO: In case of a conflict call a callback function that merges two files together somehow.

    DIR* dfd;
    struct dirent* dp;

    errno = 0;  // Initialize errno
    if (!(dfd = opendir(src_path))) {
        perror("opendir");
        return -1;  // Notify caller that something went wrong!
    }

    errno = 0;
    while (dp = readdir(dfd)) {
        printf("%s\n", dp->d_name);
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
