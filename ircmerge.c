#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BUFFER_LENGTH 1024
#define DATE_BUFFER_LENGTH 20
#define DATE_OFFSET 15
#define DATE_FORMAT "%a %b %d %H:%M:%S %Y"

double compare_headers(const char*, const char*);
time_t to_seconds(const char*, const char*);

int main(int argc, char* argv[]) {
    FILE *src0, *src1, *dest=stdout;
    char buffer0[BUFFER_LENGTH];  // Line or Message Buffer for Source 0 file
    char buffer1[BUFFER_LENGTH];  // Line or Message Buffer for Source 1 file

    if ((src0 = fopen(argv[1], "r")) == NULL) {
        perror(argv[1]);
        return 1;
    }

    if ((src1 = fopen(argv[2], "r")) == NULL) {
        perror(argv[2]);
        fclose(src0);
        return 1;
    }

    fscanf(src0, "%[^\n]", buffer0);
    fscanf(src1, "%[^\n]", buffer1);

    double res = compare_headers(buffer0, buffer1);

    if (res > 0)
        fprintf(dest, "%s started earlier than %s\n", argv[1], argv[2]);
    else if (res < 0)
        fprintf(dest, "%s started earlier than %s\n", argv[2], argv[1]);
    else
        fprintf(dest, "Log files started together\n");

    fclose(src0);
    fclose(src1);

    return 0;
}

double compare_headers(const char* head0, const char* head1) {
    char date_buffer[DATE_BUFFER_LENGTH];
    time_t date0, date1;

    // Get 2 time_t's from the two log headers and return difftime()'s output.
    // If head0 is earlier than head1, difftime() will result in positive, otherwise
    // if head1 is earlier than head0, difftime() will result in negative, and
    // datetime() will equal 0 if head0 == head1.

    strcpy(date_buffer, head0 + DATE_OFFSET);
    date0 = to_seconds(date_buffer, DATE_FORMAT);

    strcpy(date_buffer, head1 + DATE_OFFSET);
    date1 = to_seconds(date_buffer, DATE_FORMAT);

    return difftime(date1, date0);
}

time_t to_seconds(const char* date, const char* format) {
    /* CF: https://www.unix.com/programming/30563-how-compare-dates-c-c.html */
    struct tm storage;

    if (strptime(date, format, &storage)) {
        // SRC: https://stackoverflow.com/questions/24185317/how-to-neatly-initialize-struct-tm-from-ctime
        storage.tm_isdst = -1;  /* Not set by strptime(); tells mktime()
                                   to determine whether daylight saving time
                                   is in effect */
        return mktime(&storage);
    }

    return 0;
}
