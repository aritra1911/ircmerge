#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BUFFER_LENGTH 1024

time_t date_to_time_t(const char* date, const char* format) {
    /* CF: https://www.unix.com/programming/30563-how-compare-dates-c-c.html */
    struct tm storage; // = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    if (strptime(date, format, &storage))
        return mktime(&storage);

    return 0;
}

int main(int argc, char* argv[]) {
    FILE *src1, *src2, *dest=stdout;
    char buffer[BUFFER_LENGTH];  // Line or Message Buffer
    char date_buffer[20];

    if ((src1 = fopen(argv[1], "r")) == NULL) {
        perror(argv[1]);
        return 1;
    }

    if ((src2 = fopen(argv[2], "r")) == NULL) {
        perror(argv[2]);
        fclose(src1);
        return 1;
    }

    fscanf(src1, "%[^\n]", buffer);
    printf(buffer);
    strcpy(date_buffer, buffer + 15);
    time_t date = date_to_time_t(date_buffer, "%a %b %d %H:%M:%S %Y");
    strftime(date_buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&date));
    printf(date_buffer);

    fclose(src1);
    fclose(src2);

    return 0;
}
