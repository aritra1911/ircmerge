#include <stdio.h>
#include <stdlib.h>

int mergedir(const char*, const char*, int (*)(const char*, const char*, const char*));
int mergelog(const char*, const char*, const char*);

int main(int argc, char* argv[]) {
    // argv[0] is the program name,
    // argv[1] to argv[argc - 2] are the source directories, and
    // argv[argc - 1] is the destination directory

    if (argc < 3) {
        fprintf(stderr, "Not enough arguments!\n");
        printf("Usage: %s source_irclogs1 source_irclogs2 ... source_irclogsN destination_irclogs\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i <= argc - 2; i++)
        if (mergedir(argv[i], argv[argc - 1], mergelog) == -1)
            return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
