#include <assert.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>


/** atomic flag **/
volatile sig_atomic_t flag = 0;



/** there is no file name in arguments of this program **/
#define NO_FILE_NAME(argc)\
do {\
    if (argc < 2) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)\




/** get name of the directory **/
#define DIR_NAME(dir_name)\
do {\
    char* code = getcwd(dir_name, sizeof(dir_name));\
    if (code == NULL) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)\




/** open file with error checking **/
#define FOPEN(file, file_name, flag)\
do {\
    file = fopen(file_name, flag);\
    if (file == NULL) {\
        exit(EXIT_FAILURE);\
    }\
} while(false)\



/****/
#define FCLOSE(file)\
do {\
    int code = fclose(file);\
    if (code == EOF) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)\



/** fputs with error checking **/
#define FPUTS(chunk, file)\
do {\
    int num = fputs(chunk, file);\
    if (num == EOF) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)\




/** sigaction with error checking **/
#define SIGACTION(signum, new_sa, old_sa)\
do {\
    int code = sigaction(signum, new_sa, old_sa);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)



/** function for renaming with error checking 

rename:
1) counted file_names
cur_count - 1 -> cur_count
cur_count - 2 -> cur_count - 1
...
3 -> 4
2 -> 3
1 -> 2

2) file_name -> file_name.1


**/
#define RENAME(name, cur_count)\
do {\
    for (int i = cur_count - 1; i >= 1; --i) {\
        char* old_name = create_new_name(name, i);\
        char* new_name = create_new_name(name, i + 1);\
        int code = rename(old_name, new_name);\
        if (code == -1) {\
            exit(EXIT_FAILURE);\
        }\
        free(old_name);\
        free(new_name);\
    }\
    char* name_1 = create_new_name(name, 1);\
    int code = rename(name, name_1);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
    free(name_1);\
} while(false)\



/** open directory with error checking **/
#define OPEN_DIR(dir, dir_name)\
do {\
    dir = opendir(dir_name);\
    if (dir == NULL) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)\





/** close directory with error checking **/
#define CLOSE_DIR(dir)\
do {\
    int code = closedir(dir);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
} while(false)\



/** check errno to distniguish between errno and end of reading**/
#define CHECK_ERRNO(errno)\
do {\
    if (errno != 0) {\
        exit(EXIT_FAILURE);\
    }\
} while(false)\




/** find size of a file_name **/
size_t get_name_size(char* name) {
    // counter
    size_t size = 0;
    // find size
    for (char* cur = name; *cur != '\0'; ++cur) {
        ++size;
    }
    return size;
}




/** find size of a count **/
size_t get_count_size(int count) {
    size_t size = 0;
    while (count != 0) {
        count /= 10;
        ++size;
    }
    return size;
}


/** create new file_name **/
char* create_new_name(char* name, int count) {
    // get size of original file name
    size_t size_name = get_name_size(name);

    // get size of count integer
    size_t size_count = get_count_size(count);

    // size of new_name = size of name + size of "." + size of count + '\0' byte
    char* new_name = (char*) malloc(size_name + 1 + size_count + 1);

    // copy name
    memcpy(new_name, name, size_name);

    // add .
    new_name[size_name] = '.';

    // index of last char in new_name
    size_t last = size_name + size_count;

    // copy count to the string
    for (size_t i = 0; i < size_count; ++i) {
        new_name[last] = (count % 10) + '0';
        count /= 10;
        --last;
    }

    // add zero byte
    new_name[size_name + 1 + size_count] = '\0';

    return new_name;
}


/**get number after file_name and "."

if such doesn't exist => return 0

**/
int read_count_from_entry_name(char* entry_name, char* file_name) {

    // file_name size
    size_t file_name_size = get_name_size(file_name);

    // entry_name size
    size_t entry_name_size = get_name_size(entry_name);

    // it is not "file_name" or "file_name.<count>"
    // => return 0 (it is not "file_name.<count>")
    if (entry_name_size < file_name_size) {
        return 0;
    }

    // it is either the "file_name" or something different
    // => return 0 (it is not "file_name.<count>")
    if (entry_name_size == file_name_size) {
        return 0;
    }

    // first file_name_size characters in entry name are different from file_name
    // => return 0 (it is not "file_name.<count>")
    for (size_t i = 0; i < file_name_size; ++i) {
        if (entry_name[i] != file_name[i]) {
            return 0;
        }
    }

    // entry_name is bigger, but there is no '.' after file name
    // => return 0 (it is not "file_name.<count>")
    if (entry_name[file_name_size] != '.') {
        return 0;
    }

    // entry_name has '.' after file_name, but nothing after
    // => return 0 (it is not "file_name.<count>")
    if (entry_name_size == file_name_size + 1) {
        return 0;
    }

    // size after '.'
    size_t after_dot_size = entry_name_size - (file_name_size + 1);
    // last character
    size_t last = entry_name_size - 1;

    // read characters after '.'
    // number
    int number = 0;
    // bases : 1, 10, 100...
    int base = 1;

    // iterate through characters after '.'
    for (size_t i = 0; i < after_dot_size; ++i) {
        // last current character
        char chr = entry_name[last];
        // get digit from character
        int digit = chr - '0';
        // found non-digit value => it is not "file_name.<count>"
        if (digit < 0 || digit > 10) {
            return 0;
        }

        // add up digits to the number
        number += digit * base;

        // decrease last position
        --last;

        // change base value
        base *= 10;
    }

    return number;
}


/** get count of minimum number, not found in directory file_names**/
int get_count(char* dir_name, char* file_name) {
    // open directory
    DIR* dir;
    OPEN_DIR(dir, dir_name);

    // max found value in directory
    int max_found = 0;

    // structure with information about directory
    struct dirent* strct;

    // errno should be set to 0 to distinguish error and end of directory after
    errno = 0;
    

    // iterate through directory
    while ((strct = readdir(dir)) != NULL) {
        // found regular file
        if (strct->d_type == DT_REG) {
            char* name = strct->d_name;
            int found = read_count_from_entry_name(name, file_name);
            if (found > max_found) {
                max_found = found;
            }
        }
    }

    CHECK_ERRNO(errno);

    // next number of file
    int min_not_found = max_found + 1;

    // close directory
    CLOSE_DIR(dir);

    return min_not_found;
}



/** function to rotate logs **/
FILE* rotate(FILE* file, char* file_name, char* dir_name) {
    // get min count in directory, for which there is no file
    int count = get_count(dir_name, file_name);
    // rename file
    RENAME(file_name, count);
    
    // close previous file
    FCLOSE(file);

    // open file with the same file_name again
    FILE* new_file;
    FOPEN(new_file, file_name, "w");
    
    return new_file;
}


/** signal handler **/
void signal_handler(int num) {
    flag = 1;
}


void write_to_the_file(FILE* file, char* file_name, char* dir_name) {
    // reading buffer
    char chunk[BUFSIZ];

    // errno should be set to 0 to distinguish error and end of the stdin
    errno = 0;

    // read until there is eof or error
    while (true) {

        // read from stdin
        char* code = fgets(chunk, sizeof(chunk), stdin);
        
        
        // SIGHUP was sent
        if (flag == 1) {
            // change flag to 0 again
            flag = 0;

            // rotate
            file = rotate(file, file_name, dir_name);
            
        }

        if (code == NULL) {
            break;
        }

        // write to the file
        FPUTS(chunk, file);
    }

    CHECK_ERRNO(errno);

    // close file
    FCLOSE(file);
}


int main(int argc, char**argv) {
    // sigaction to put a signal handler on SIGHUP
    struct sigaction sa;
    sa.sa_handler = &signal_handler;
    
    // fgets uses read function
    // read function will get EINTR, if not set this flag:
    sa.sa_flags = SA_RESTART;
    SIGACTION(SIGHUP, &sa, NULL);

    // find file_name
    NO_FILE_NAME(argc);
    char* file_name = argv[1];

    // open file
    FILE* file;
    FOPEN(file, file_name, "w");

    // find dir_name
    char dir_name[PATH_MAX];
    DIR_NAME(dir_name);

    // write to the file
    write_to_the_file(file, file_name, dir_name);
}