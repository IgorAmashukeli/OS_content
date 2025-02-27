#include <asm-generic/errno.h>
#include <assert.h>
#include <limits.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>



/**no k was mentioned in the arguments**/
#define NO_K(argc)\
do {\
    if (argc < 2) {\
        exit(EXIT_FAILURE);\
    }\
} while (false);\




/**create pipe**/
#define CREATE_PIPE(pipe_fds)\
do {\
    int code = pipe(pipe_fds);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)




/**close pipe end**/
#define CLOSE_PIPE_END(pipe_fd)\
do {\
    int code = close(pipe_fd);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)\




/**open directory**/
#define OPEN_DIR(dir_name, dir)\
do {\
    dir = opendir(dir_name);\
    if (dir == NULL) {\
        err(1, "opendir");\
    }\
} while(false)




/**close directory**/
#define CLOSE_DIR(dir)\
do {\
    int code = closedir(dir);\
    if (code == -1) {\
        err(1, "closedir");\
    }\
} while(false)\




/**read directory**/
#define READ_DIR(entry, dir)\
do {\
    entry = readdir(dir);\
    if (entry == NULL && errno != 0) {\
        err(1, "readdir");\
    }\
} while (false)\



/**read from string **/
#define READ(fd, buffer, size)\
do {\
    int code = read(fd, buffer, size);\
    if (code == -1) {\
        err(1, "read");\
    }\
} while(false)\



/**add slash after old path or not

it depends on whether it is current directory or not
**/
#define SLASH_ADDING_CHOICE(slash_adding)\
do {\
    slash_adding = true;\
    if (strcmp(path, ".") == 0) {\
        slash_adding = false;\
    }\
} while (false)\




/** write buffer to fd **/
#define WRITE(fd, buffer, size)\
do {\
    int code = write(fd, buffer, size);\
    if (code == -1) {\
        err(1, "write");\
    }\
} while(false)\




/**stat function with error checking**/
#define STAT(file_name, stat_pntr)\
do {\
    int code = stat(file_name, stat_pntr);\
    if (code == -1) {\
        err(1, "stat");\
    }\
} while (false)\




/**check end of directory

there is no "do while(false)", because of break

**/
#define CHECK_END_OF_DIR(entry)\
    if (entry == NULL) {\
        break;\
    }\




/**fork with error checking**/
#define FORK(pid)\
do {\
    pid = fork();\
    if (pid < 0) {\
        err(1, "fork");\
    }\
} while (false)\






/**execvp with error checking**/
#define EXECV(file_name)\
do {\
    char* arguments[2];\
    arguments[0] = file_name;\
    arguments[1] = NULL;\
    execv(file_name, arguments);\
    err(1, "execvp");\
} while (false)\




/**waitpid with error checking**/
#define WAITPID(pid)\
do {\
    int status;\
    int code = waitpid(pid, &status, 0);\
    if (code == -1) {\
        err(1, "waitpid");\
    }\
} while (false)\



/**wait with error checking**/
#define WAIT()\
do {\
    int status;\
    while (wait(&status) > 0) {}\
    if (errno != ECHILD) {\
        exit(EXIT_FAILURE);\
    }\
} while (false)\





/**calculate size of a string**/
size_t get_size(char* k_string) {
    size_t size = 0;

    for (char* cur = k_string; *cur != '\0'; ++cur) {
        ++size;
    }
    return size;
}




/**convert string to integer**/
int convert(char* k_string) {
    // size of "k"
    size_t size = get_size(k_string);

    int number = 0;
    int base = 1;
    for (size_t i = 0; i < size; ++i) {
        // reversed index
        int j = size - i - 1;
        // take the digit from string
        char digit = k_string[j] - '0';

        // found non-digit element
        if (digit < 0 || digit >= 10) {
            exit(EXIT_FAILURE);
        }

        // add up to the number
        number += digit * base;

        // change base
        base *= 10;
    }

    return number;
}



/**concatenate path**/
void concatenate_path(char* new_path, char* old_path, char* entry, bool add_slash) {
    // size of old path
    size_t old_path_size = get_size(old_path);

    // if it is original directory, we don't need '.'
    if (old_path[0] == '.') {
        old_path_size = 0;
    }

    // size of entry name
    size_t entry_size = get_size(entry);

    // copy old path
    memcpy(new_path, old_path, old_path_size);
    // add "/" delimiter

    // add slash if needed
    if (add_slash) {
        memcpy(new_path + old_path_size, "/", 1);
    }
    

    // copy entry name
    memcpy(new_path + old_path_size + add_slash, entry, entry_size);
    new_path[old_path_size + add_slash + entry_size] = '\0';

}




/**init pipe "semaphore"

init pipe "semaphore" <=> write k byte to writing end of pipe

**/
void semaphore_init(int pipe_fds[2], int k) {
    char buffer[k];

    // pipe buffer
    WRITE(pipe_fds[1], buffer, k);
}




/**up pipe "semaphore

up pipe "semaphore" <=> write 1 byte to writing end of pipe

"**/
void semaphore_up(int pipe_fds[2]) {
    char buffer[1];

    // pipe write
    WRITE(pipe_fds[1], buffer, 1);
}




/**down pipe "semaphore"

down pipe "semaphore" <=> read 1 byte to reading end of pipe

**/
void semaphore_down(int pipe_fds[2]) {
    char buffer[1];

    // pipe read
    READ(pipe_fds[0], buffer, 1);
}





/**execute command from file_name

run user's process in child and wait for it in parent

**/
void runWatcher(char* file_name, int pipe_fds[2], int k) {
    // pids of main make process - parent and watcher son process
    pid_t pid_parent_son;
    FORK(pid_parent_son);

    // watcher
    if (pid_parent_son == 0) {
        // close reading pipe
        // watcher only uses writing end of pipe 
        CLOSE_PIPE_END(pipe_fds[0]);


        // fork to create process for user's executable file
        pid_t pid_son_grand_son;
        FORK(pid_son_grand_son);

        // grandson executes
        if (pid_son_grand_son == 0) {

            // close redundant ends of pipe
            // grandson doesn't need any end of pipe
            // watcher closed reading one
            // now grandson is closing writing one
            CLOSE_PIPE_END(pipe_fds[1]);
            
            // execute file
            EXECV(file_name);

        // son-watcher waites for grandson to execute
        } else {
            // wait for grandson
            WAITPID(pid_son_grand_son);


            // up semaphore
            semaphore_up(pipe_fds);

            // watcher ended his job
            exit(EXIT_SUCCESS);
        }
        
    }
    
}




void traverse_and_run(char* dir_name, char* path, int pipe_fds[2], int k) {

    /**traverse subdirectories**/

    // open current directory
    DIR* dir;
    OPEN_DIR(path, dir);

    // traverse sudirectories -> "run all recursive processes make"
    while(true) {

        // set errno to 0 to distinguish between error and end of directory
        errno = 0;

        // structure, that describes current directory
        struct dirent* entry; 
        
        // read directory
        READ_DIR(entry, dir);

        // check if it is end of directory
        CHECK_END_OF_DIR(entry);


        // found subdirectory, traverse to run recursivly
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {

            // create path for subdirectory
            char new_path[PATH_MAX];

            // to add or not to add slash between
            bool slash_adding;
            SLASH_ADDING_CHOICE(slash_adding);

            concatenate_path(new_path, path, entry->d_name, slash_adding);

            pid_t recursive_son;
            FORK(recursive_son);

            if (recursive_son == 0) {
                traverse_and_run(entry->d_name, new_path, pipe_fds, k);
            }
        }

    }
    // close directory
    CLOSE_DIR(dir);

    /**loop for processes in current directory**/

    // open current directory again to run processes in this directory
    OPEN_DIR(path, dir);


    // run all processes
    while(true) {

        // set errno to 0 to distinguish between error and end of directory
        errno = 0;

        // structure, that describes current directory
        struct dirent* entry; 
        
        // read directory
        READ_DIR(entry, dir);

        // check if it is end of directory
        CHECK_END_OF_DIR(entry);


        if (entry->d_type == DT_REG) {
            // create file_name as relative path from initial directory
            char file_name[PATH_MAX];

            // to add or not to add slash between
            bool slash_adding;
            SLASH_ADDING_CHOICE(slash_adding);

            concatenate_path(file_name, path, entry->d_name, slash_adding);

            // structure with information about file
            struct stat sb;
            STAT(file_name, &sb);


            // check if file is executable
            if (sb.st_mode & S_IXUSR) {
                // down semaphore
                semaphore_down(pipe_fds);

                // run watcher
                runWatcher(file_name, pipe_fds, k);
            }
        }
    }

    CLOSE_PIPE_END(pipe_fds[0]);
    CLOSE_PIPE_END(pipe_fds[1]);
    WAIT();

    CLOSE_DIR(dir);
    
    exit(EXIT_SUCCESS);
}





int main(int argc, char* argv[]) {
    // if no k in argument found
    NO_K(argc);

    // string, that represents k
    char* k_string = argv[1];

    // convert string to integer  k
    int k = convert(k_string);

    // create pipes
    int pipe_fds[2];
    CREATE_PIPE(pipe_fds);

    // init pipe as semaphore
    semaphore_init(pipe_fds, k);

    // absolute path from current directory

    // traverse current directory
    traverse_and_run(".", ".", pipe_fds, k);

    return 0;
}

