#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BUFFER_LENGTH      1024
#define DATE_BUFFER_LENGTH 42
#define DATE_OFFSET        15
#define DATE_FORMAT        "%a %b %d %H:%M:%S %Y"

int copy_logblock(FILE*, FILE*, char* buffer);
void copy_log(FILE*, FILE*, char* buffer);
double compare_headers(const char*, const char*);
time_t to_seconds(const char*, const char*);

int main(int argc, char* argv[]) {
    FILE *src0, *src1, *dest=stdout;
    char buffer0[BUFFER_LENGTH];  // Line or Message Buffer for Source 0 file
    char buffer1[BUFFER_LENGTH];  // Line or Message Buffer for Source 1 file

    if (argc != 3) {
        fprintf(stderr, "Usage: %s first_log_file second_log_file\n", argv[0]);
        return 1;
    }

    if ((src0 = fopen(argv[1], "r")) == NULL) {
        perror(argv[1]);
        return 1;
    }

    if ((src1 = fopen(argv[2], "r")) == NULL) {
        perror(argv[2]);
        fclose(src0);
        return 1;
    }

    // Get the log headers containing starting dates & time
    fscanf(src0, "%[^\n]", buffer0);
    getc(src0);  // Get the '\n' at the EOL and throw it away
    fscanf(src1, "%[^\n]", buffer1);
    getc(src1);  // Get the '\n' at the EOL and throw it away

    while (1) {
        double res = compare_headers(buffer0, buffer1);

        if (res > 0) {
            fprintf(dest, "%s\n", buffer0);  // TODO: Should this be taken care of by copy_log[block]()?

            if (!copy_logblock(src0, dest, buffer0)) {  // EOF in copy_logblock()
                fprintf(dest, "%s\n", buffer1);  // Proceed to print the pending header from src1
                copy_log(src1, dest, buffer1);  // And then the rest of src1
                break;  // exit
            }

        } else if (res < 0) {
            fprintf(dest, "%s\n", buffer1);

            if (!copy_logblock(src1, dest, buffer1)) {  // EOF in copy_logblock()
                fprintf(dest, "%s\n", buffer0);  // Proceed to print the pending header from src0
                copy_log(src0, dest, buffer0);  // And then the rest of src0
                break;  // exit
            }

        } else
            fprintf(stderr, "Log files started together\n");
    }

    fclose(src0);
    fclose(src1);

    return 0;
}

int copy_logblock(FILE* src, FILE* dest, char* buffer) {
    int rets;  // Used to determine which failing condition caused the while() loop below to break

    while ((rets = fscanf(src, "%[^\n]", buffer)) != EOF) {
        getc(src);  // Get the '\n' at the EOL and throw it away
        if (!strncmp(buffer, "--- L", 5)) break;  // Log closed or opened again
        fprintf(dest, "%s\n", buffer);
    }

    if (rets == EOF) return 0;

    if (!strncmp(buffer, "--- Log c", 9)) {  // If 'Log closed',
        fprintf(dest, "%s\n", buffer);  // write it out, and get the next line which should say
        if (fscanf(src, "%[^\n]", buffer) == EOF)  // 'Log opened'
            return 0;                              // but return 0 on EOF

        getc(src);  // Get the '\n' at the EOL and throw it away

    }  // `buffer` should hold in it the next log header with date & time.

    return 1;
}

void copy_log(FILE* src, FILE* dest, char* buffer) {
    while (fscanf(src, "%[^\n]", buffer) != EOF) {
        getc(src);  // Get the '\n' at the EOL and throw it away
        fprintf(dest, "%s\n", buffer);
    }
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
