#define _DEFAULT_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>  // For PATH_MAX, TODO: Make this platform independent
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TEMP_FILENAME "temp.log"

int mergedir(const char* src_path, const char* dest_path,
             int (*merge)(const char*, const char*, const char*)) {
    // recursively copy src_path to dest_path, changing or making directories as needed.
    // In case of a conflict call a callback function that merges two files together somehow.

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
            closedir(dfd);
            return -1;
        }

        sprintf(dest_ent, "%s/%s", dest_path, dp->d_name);  // TODO: Hardcoding '/'?

        if (S_ISDIR(stbuf.st_mode)) {
            // src_ent is a directory

            // If dest_ent doesn't exist, make it
            if (mkdir(dest_ent, stbuf.st_mode) == -1) {
                if (errno != EEXIST) {  // If it already exists, then woohoo, less work!
                    perror("mkdir");
                    closedir(dfd);
                    return -1;
                }

                errno = 0;  // Reset errno
            }

            if (mergedir(src_ent, dest_ent, merge) == -1) {  // Recurse
                closedir(dfd);  // but beware that it may fail.
                return -1;  // In that case, fail here as well.
            }

        } else {
            // src_ent is a file
            if (strcasecmp(strrchr(dp->d_name, '.'), ".log"))
                continue;  // If file extension isn't '.log', skip

            int dest_fd;

            // Try to create dest_ent
            if ((dest_fd = open(dest_ent, O_WRONLY | O_CREAT | O_EXCL, stbuf.st_mode)) == -1) {
                if (errno != EEXIST) {  // If it doesn't exist and we get an error, report it
                    perror("open");
                    errno = 0;  // Reset errno
                    closedir(dfd);
                    return -1;
                }

                errno = 0;  // Reset errno

                // If it already exists, don't create it again
                printf("MERGE : %s And %s\n", src_ent, dest_ent);

                char temp_file[PATH_MAX];
                sprintf(temp_file, "%s/%s", dest_path, TEMP_FILENAME);

                merge(src_ent, dest_ent, temp_file);  // Call to provided merging callback function

                if (access(temp_file, F_OK) == -1) {
                    // `temp_file` doesn't exist, merge failed
                    // but don't stop here beacuse of that. Maybe the other files will get merged properly.
                    errno = 0;  // Reset errno

                } else {  //  `temp_file` exists, hence merge was successful
                    if (unlink(dest_ent) == -1) {  // remove previous `dest_ent`
                        perror("unlink");
                        errno = 0;  // Reset errno

                    } else {  // If successful, disguise `temp_file` as new `dest_ent`
                        if (rename(temp_file, dest_ent) == -1) {
                            // previous `dest_ent` doesn't exist here, if we fail to rename, we should stop.
                            // As any successive merges will overwrite `temp_file`, but we must save it.

                            perror("rename");
                            errno = 0;  // Reset errno
                            closedir(dfd);
                            return -1;
                        }
                    }
                }

            } else {  // open(dest_ent) succeeded in creating the file and we don't already have anything in it,
                      // so, copy src_ent to dest_ent.
                int src_fd;

                if ((src_fd = open(src_ent, O_RDONLY)) == -1) {  // Open source file for reading
                    perror("open");
                    errno = 0;  // Reset errno
                    close(dest_fd);
                    closedir(dfd);
                    return -1;
                }

                ssize_t n;
                char buf[BUFSIZ];

                // copy `src_ent` file to `dest_ent` file
                printf(" COPY : %s To %s\n", src_ent, dest_ent);
                while ((n = read(src_fd, buf, BUFSIZ)) > 0)
                    if (write(dest_fd, buf, n) != n) {
                        // `n` bytes were read from `src_fd`, so if all of `n` bytes weren't written to `dest_fd`, some
                        // sort of write error has occurred.
                        fprintf(stderr, "write error on file %s\n", dest_ent);

                        if (errno) {  // if errno is set, report the error
                            perror("write");
                            errno = 0;  // Reset errno
                        }

                        close(dest_fd);
                        close(src_fd);
                        closedir(dfd);
                        return -1;
                    }

                close(src_fd);
                close(dest_fd);
            }
        }

        errno = 0;  // Make sure errno is reset before the next call to `readdir()`.
    }

    if (errno) {  // If errno modified, something went wrong
        perror("readdir");
        errno = 0;  // Reset errno
        return -1;
    }

    closedir(dfd);

    return 0;
}
