#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BUFFER_LENGTH      1024
#define DATE_BUFFER_LENGTH 42
#define DATE_OFFSET        15
#define DATE_FORMAT        "%a %b %d %H:%M:%S %Y"

int skip_logblock(FILE*, char* buffer);
int copy_logblock(FILE*, FILE*, char* buffer);
void copy_log(FILE*, FILE*, char* buffer);
double compare_headers(const char*, const char*);
time_t to_seconds(const char*, const char*);

int mergelog(const char* src0_logfile, const char* src1_logfile, const char* dest_logfile) {
    FILE *src0, *src1, *dest;
    char buffer0[BUFFER_LENGTH];  // Line or Message Buffer for Source 0 file
    char buffer1[BUFFER_LENGTH];  // Line or Message Buffer for Source 1 file

    if ((src0 = fopen(src0_logfile, "r")) == NULL) {
        perror(src0_logfile);
        errno = 0;  // Reset errno
        return -1;
    }

    if ((src1 = fopen(src1_logfile, "r")) == NULL) {
        perror(src1_logfile);
        errno = 0;  // Reset errno
        fclose(src0);
        return -1;
    }

    if ((dest = fopen(dest_logfile, "w")) == NULL) {
        perror(dest_logfile);
        errno = 0;  // Reset errno
        fclose(src1);
        fclose(src0);
        return -1;
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

        } else {
            /* If we're here, this means this log block exists on both the
               source files. So, we shall copy this one from the src1 to dest
               and skip this log block of src0 */
            fprintf(dest, "%s\n", buffer1);

            /* We shall report the user about this fact too */
            /* TODO: Does the user want to know the position and date and time
               details of that block? Could be a `--verbose' feature */
            printf("Similar log blocks found, copying only once!\n");

            /* Notice that we have an option of whether to copy src0 and skip
             * src1 or copy src1 and skip src0. I have assumed that src1 is a
             * more updated or recent version, but that might not be the case
             * always.
             * TODO : Should we count the size of a log block, compare and
             * always copy the longer one while skipping the shorter one?
             */

            /* Skip a block on src0 */
            if (!skip_logblock(src0, buffer0)) {
                /* If that was the last block, then we'll copy over rest of src1
                 * and exit */
                copy_log(src1, dest, buffer1);
                break;
            }

            /* Here we copy the log block from src1 to dest after skipping the
             * same log block belonging to src0 */
            if (!copy_logblock(src1, dest, buffer1)) {
                /* src1 EOFed so we now copy over the rest of src0 and exit */

                /* Don't forget to copy the `Log opened' line */
                fprintf(dest, "%s\n", buffer0);
                copy_log(src0, dest, buffer0);
                break;
            }
        }
    }

    fclose(src0);
    fclose(src1);
    fclose(dest);

    return 0;
}

int skip_logblock(FILE* src, char* buffer) {
    /* Moves the FILE pointer to the start of the next log block. We return 0 if
       we encounter an EOF in the process, otherwise we return 1. */

    while (fscanf(src, "%[^\n]", buffer) != EOF) {
        getc(src);  // Get the '\n' at the EOL and throw it away

        if (!strncmp(buffer, "--- L", 5)) {
            /* Sometimes when irssi quits abnormally, it missies the line
             * stating the `Log closed` and time information. In such a case, we
             * may find messages contained between two `Log opened' statements,
             * or an abrupt EOF. The above condition checks that we've
             * encountered either a `Log opened' or a `Log closed', so we'll now
             * take action accordingly.
             */
            if (!strncmp(buffer, "--- Log c", 9)) {
                /* Since we've encounted a `Log closed' statement we'll try to
                 * read the next line which should say `Log opened'. */
                if (fscanf(src, "%[^\n]", buffer) == EOF) {
                    /* but return 0 on EOF */
                    return 0;
                }
                getc(src);  // Get the '\n' at the EOL and throw it away
            }
            /* Our aim is to keep `buffer` pointed at the next `Log opened'
             * block, so we really don't have to check that since if it's not
             * `Log closed', it must be `Log opened'.
             */
            return 1;
        }
    }

    /* We encountered an EOF while looking for a `Log closed' */
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
