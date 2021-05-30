#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BUFFER_LENGTH      1024
#define DATE_BUFFER_LENGTH 42
#define DATE_OFFSET        15
#define DATE_FORMAT        "%a %b %d %H:%M:%S %Y"

typedef struct {
    const char* filename;
    FILE* file;
    char buffer[BUFFER_LENGTH];
    long int line;  /* current line number */

    /* I have no idea how long a log file can be,
     * so `long int' seems like a safe way to go
     */
} log_t;

int scan_log(log_t*);
int skip_logblock(log_t*);
int copy_logblock(log_t*, FILE*);
void copy_log(log_t*, FILE*);
int compare_headers(const char*, const char*, double*);
time_t to_seconds(const char*, const char*);

int mergelog(const char* src0_logfile, const char* src1_logfile, const char* dest_logfile) {
    log_t src0, src1;
    FILE *dest;
    double res;  // Contain time difference of log headers

    src0.filename = src0_logfile;
    src1.filename = src1_logfile;

    if ((src0.file = fopen(src0.filename, "r")) == NULL) {
        perror(src0.filename);
        errno = 0;  // Reset errno
        return -1;
    }

    if ((src1.file = fopen(src1.filename, "r")) == NULL) {
        perror(src1.filename);
        errno = 0;  // Reset errno
        fclose(src0.file);
        return -1;
    }

    if ((dest = fopen(dest_logfile, "w")) == NULL) {
        perror(dest_logfile);
        errno = 0;  // Reset errno
        fclose(src1.file);
        fclose(src0.file);
        return -1;
    }

    src0.line = 0;
    src1.line = 0;

    /* line = 0 means we haven't read any lines yet!
     * `scan_log()' takes care of incrementing the value as we read lines
     */

    // Get the log headers containing starting dates & time
    scan_log(&src0);
    scan_log(&src1);

    while (1) {
        /* Looks like I've got log files where a log block starts arbitrarily
         * without a `Log opened' statement, but not within a log block, i.e. we
         * get conversation hapenning right after a `Log closed', skipping the
         * `Log open' statement. Not sure if irssi is to blame here, or if I had
         * actually attempted a merge before with a buggy version of this
         * program. But still it's good to have a check for this and perhaps
         * TODO: Do not bail out here, since the situation is clear. So either
         * skip this block or assume the date and time information from the
         * `Log closed' line to be the date and time of current log block.
         */
        if (compare_headers(src0.buffer, src1.buffer, &res) != 0) {
            /* Whoops! The function got bad headers to compare.
               Bail out, this file is clearly corrupted. */
            fprintf(stderr, "ERROR : Failed to compare log headers:\n"
                            "            %s:%li and %s:%li\n"
                            "        Bailing out!\n",
                    src0.filename, src0.line, src1.filename, src1.line);
            return -1;
        }

        if (res > 0) {
            fprintf(dest, "%s\n", src0.buffer);  // TODO: Should this be taken care of by copy_log[block]()?

            if (!copy_logblock(&src0, dest)) {       // EOF in copy_logblock()
                fprintf(dest, "%s\n", src1.buffer);  // Proceed to print the pending header from src1
                copy_log(&src1, dest);               // And then the rest of src1
                break;  // exit
            }

        } else if (res < 0) {
            fprintf(dest, "%s\n", src1.buffer);

            if (!copy_logblock(&src1, dest)) {       // EOF in copy_logblock()
                fprintf(dest, "%s\n", src0.buffer);  // Proceed to print the pending header from src0
                copy_log(&src0, dest);               // And then the rest of src0
                break;  // exit
            }

        } else {
            /* If we're here, this means this log block exists on both the
               source files. So, we shall copy this one from the src1 to dest
               and skip this log block of src0 */
            fprintf(dest, "%s\n", src1.buffer);

            /* We shall warn the user about this fact too */
            fprintf(stderr, " WARN : Similar log blocks found:\n"
                            "            %s:%li and %s:%li\n"
                            "        copying only once!\n",
                    src0.filename, src0.line, src1.filename, src1.line);

            /* Notice that we have an option of whether to copy src0 and skip
             * src1 or copy src1 and skip src0. I have assumed that src1 is a
             * more updated or recent version, but that might not be the case
             * always.
             * TODO : Should we count the size of a log block, compare and
             * always copy the longer one while skipping the shorter one?
             */

            /* Skip a block on src0 */
            if ( !skip_logblock(&src0) ) {
                /* If that was the last block, then we'll copy over rest of src1
                 * and exit */
                copy_log(&src1, dest);
                break;
            }

            /* Here we copy the log block from src1 to dest after skipping the
               same log block belonging to src0 */
            if ( !copy_logblock(&src1, dest) ) {
                /* src1 EOFed so we now copy over the rest of src0 and exit */

                /* Don't forget to copy the `Log opened' line */
                fprintf(dest, "%s\n", src0.buffer);
                copy_log(&src0, dest);
                break;
            }
        }
    }

    fclose(src0.file);
    fclose(src1.file);
    fclose(dest);

    return 0;
}

int scan_log(log_t* log) {
    /* Get the next line from the log file */

    int retval = fscanf(log->file, "%[^\n]", log->buffer);

    if (retval != EOF) {
        /* Get the '\n' at the EOL and throw it away */
        getc(log->file);

        log->line++;
    }

    return retval;
}

int skip_logblock(log_t* log) {
    /* Move the FILE pointer to the start of the next log block. We return 0 if
       we encounter an EOF in the process, otherwise we return 1. */

    while (scan_log(log) != EOF) {

        if (!strncmp(log->buffer, "--- L", 5)) {
            /* Sometimes when irssi quits abnormally, it missies the line
             * stating the `Log closed` and time information. In such a case, we
             * may find messages contained between two `Log opened' statements,
             * or an abrupt EOF. The above condition checks that we've
             * encountered either a `Log opened' or a `Log closed', so we'll now
             * take action accordingly.
             */
            if (!strncmp(log->buffer, "--- Log c", 9)) {
                /* Since we've encounted a `Log closed' statement we'll try to
                 * read the next line which should say `Log opened'. */
                if (scan_log(log) == EOF) {
                    /* but return 0 on EOF */
                    return 0;
                }
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

int copy_logblock(log_t* log, FILE* dest) {
    int rets;  // Used to determine which failing condition caused the while() loop below to break

    while ( (rets = scan_log(log)) != EOF ) {
        if (!strncmp(log->buffer, "--- L", 5)) break;  // Log closed or opened again
        fprintf(dest, "%s\n", log->buffer);
    }

    if (rets == EOF) return 0;

    if ( !strncmp(log->buffer, "--- Log c", 9) ) {  // If 'Log closed',
        fprintf(dest, "%s\n", log->buffer);  // write it out, and
        /* get the next line which should say 'Log opened' */
        if (scan_log(log) == EOF)
            return 0;               // but return 0 on EOF

    }  // `buffer` should hold in it the next log header with date & time.

    return 1;
}

void copy_log(log_t* src, FILE* dest) {
    while (scan_log(src) != EOF) {
        fprintf(dest, "%s\n", src->buffer);
    }
}

int compare_headers(const char* head0, const char* head1, double* diff) {
    if (strncmp(head0, "--- Log o", 9) || strncmp(head1, "--- Log o", 9)) {
        /* Quick check to see if we were actually passed the correct statement
         * containing the `Log opened' date and time information. So if any of
         * the above conditions evaluate to non-zero, we're definitely reading
         * the wrong line and hence it'll be impossible to extract any useful
         * date and time information in order to compare them. */
        return -1;
    }

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

    *diff = difftime(date1, date0);

    return 0;
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
