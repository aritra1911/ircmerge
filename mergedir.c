#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>  // For PATH_MAX, TODO: Make this platform independent
#include <dirent.h>
#include <sys/stat.h>

int mergedir(const char* src_path, const char* dest_path) {
    // recursively copy src_path to dest_path, changing or making directories as needed.
    // TODO: In case of a conflict call a callback function that merges two files together somehow.

    DIR* dfd;
    struct dirent* dp;
    struct stat stbuf;
    char src_ent[PATH_MAX];  // Absolute path to entity belonging to src_path
    char dest_ent[PATH_MAX];  // Absolute path to entity belonging to dest_path

    errno = 0;  // Initialize errno
    if (!(dfd = opendir(src_path))) {
        perror("opendir");
        errno = 0;  // Reset errno
        return -1;  // Notify caller that something went wrong!
    }

    errno = 0;  // Reset errno
    while ((dp = readdir(dfd))) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;  // Skip self & parent directories

        sprintf(src_ent, "%s/%s", src_path, dp->d_name);  // TODO: Hardcoding '/'?

        // src_ent holds absolute path to current directory entity
        if (stat(src_ent, &stbuf) == -1) {
            perror("stat");  // It'll be very weird if stat() fails here
            errno = 0;  // Reset errno
            return -1;
        }

        if (S_ISDIR(stbuf.st_mode)) {
            // src_ent is a directory
            sprintf(dest_ent, "%s/%s", dest_path, dp->d_name);  // TODO: Hardcoding '/'?

            // If dest_ent doesn't exist, make it
            if (mkdir(dest_ent, stbuf.st_mode) == -1) {
                if (errno != EEXIST) {  // If it already exists, then woohoo, less work!
                    perror("mkdir");
                    return -1;
                }

                errno = 0;  // Reset errno
            }

            if (mergedir(src_ent, dest_ent) == -1)  // Recurse
                return -1;  // but beware that it may fail. In that case, fail here as well.

        } else
            printf("[F] %s\n", src_ent);  // Is a file

    }

    if (errno) {  // If errno modified, something went wrong
        perror("readdir");
        errno = 0;  // Reset errno
        return -1;
    }

    closedir(dfd);

    return 0;
}

// Testing only, TODO: remove
int main(int argc, char* argv[]) {
    if(mergedir(argv[1], argv[2]) == -1)
        return 1;
    return 0;
}
