#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>


// type of token: word, "<" or ">"
enum tokentype
{
    TK_WORD,
    TK_INFILE,
    TK_OUTFILE,
};


// token: start position in line, length, type and next
struct token
{
    char *start;
    size_t len;
    enum tokentype type;
    struct token *next;
};


// tokenizer: number of tokens, head to the list of tokens
struct tokenizer
{
    size_t token_count;
    struct token *head;
};


// skip spaces in command
// ' ', '\t', '\n' are spaces
#define SKIP_SPACES(cur)\
do {\
    while (*cur == ' ' || *cur == '\t' || *cur == '\n') { \
        ++cur; \
    }     \
} while (false)\




// break if it is end of a command
// '\0' is end of command
// no "do while false pattern, because of <<break>>"
#define CHECK_END(cur)\
    if (*cur == '\0') {\
        break;\
    }   \



// intialise token and shift cur, next token is NULL
#define INIT_TOKEN(cur) \
do {\
    token->start = cur++;\
    token->next = NULL;\
} while(false)\



// read token
// if "<" -> type is TK_INFILE
// else if ">" -> type is TK_OUTFILE
// else -> type is TK_WORD
// shift cur to the next char after word/token
// find the length of the word/token
#define READ_TOKEN(cur, token) \
do {\
    switch (*token->start) {\
        case '<':\
            token->type = TK_INFILE;\
            break;\
        case '>':\
            token->type = TK_OUTFILE;\
            break;\
        default:\
            token->type = TK_WORD;\
            while (*cur && *cur != ' ' && *cur != '\t' && *cur != '\n') {\
                ++cur;\
            }\
        }\
        token->len = cur - token->start;\
} while(false)\




// update tokenizer
// if last_token != NULL -> insert after the tail of the list
// else (last_token == NULL) -> insert in the head of the list
// update last token and increment token_count
#define UPDATE_TOKENIZER(tokenizer, token, last_token)\
do {\
    if (last_token) {\
        last_token->next = token;\
    } else {\
        tokenizer->head = token;\
    }\
    last_token = token;\
    ++tokenizer->token_count; \
} while (false)\





// if empty command, do nothing and return (go to the next command)
#define CHECK_EMPTY(tokenizer) \
do {\
    if (tokenizer->token_count == 0 || tokenizer->head == NULL) {\
        tokenizer_free(tokenizer);\
        return; \
    }\
} while (false)\




// syntax check macros
// if wrong syntax, print "Syntax error" and return (go to the next command)
#define CHECK_SYNTAX(tokenizer)\
do {\
    if (!correct_syntax(tokenizer)) {\
        tokenizer_free(tokenizer);\
        printf("Syntax error\n");\
        return;\
    }\
} while (false)\



// skip each "<" or ">" and the next argument
// break, if end of tokens
// no "do while" pattern, because of break
// this macros skips non-argument char*
#define SKIP_FILES(cur)\
    while (cur->type == TK_INFILE || cur->type == TK_OUTFILE) {\
        cur = cur->next;\
        cur = cur->next;\
        if (!cur) {\
            break;\
        }\
    }\
    if (!cur) {\
        break;\
    }\



// create argument
// this macros copies the argument to the "argument" variable, push_backs '\0'
#define CREATE_ARGUMENT(cur, argument) \
do {\
    char* pos = cur->start;\
    memcpy(argument, pos, cur->len); \
    argument[cur->len] = '\0';\
} while(false)\
    



// create char* [] array of arguments from argument_list
// iteration throught the list
// skiping non-argument tokens
// allocate arguments, initialise them, push_back NULL
#define ALLOCATE_ARGUMENTS(argument_list) \
do {\
    size_t index = 0; \
    struct token* cur = tokenizer->head;\
    while (cur) \
    {\
        SKIP_FILES(cur);\
        char* argument = (char*) malloc((cur->len + 1) * sizeof(char));\
        CREATE_ARGUMENT(cur, argument); \
        argument_list[index] = argument; \
        cur = cur->next; \
        index++; \
    }\
    argument_list[index] = NULL;\
} while (false)\





// check if next token is file, if not return false
#define CHECK_NEXT(token) \
do {\
    if (!next_word_is_file(token)) {\
        return false;\
    }\
} while(false)\



// free memory and exit
#define EXIT_FAIL_WITHOUT_CLOSE(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer)\
do {\
    FREE_MEMORY(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);\
    exit(EXIT_FAILURE);\
} while(false)\



// close everything, free memory and exit
#define EXIT_FAIL(in_file_name, out_file_name, in_fd, out_fd, input_redir, output_redir, argument_list, tokenizer)\
do {\
    CLOSE(in_fd, out_fd,\
    input_redir, output_redir,\
    in_file_name, out_file_name,\
    argument_list, tokenizer);\
    FREE_MEMORY(in_file_name, out_file_name,\
    input_redir, output_redir,\
    argument_list, tokenizer);\
    exit(EXIT_FAILURE);\
} while(false)\




// free all memory
#define FREE_MEMORY(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer)\
do {\
    tokenizer_free(tokenizer);\
    FREE_ARGUMENTS(argument_list);\
    FREE_FILE_NAMES(in_file_name, out_file_name, input_redir, output_redir);\
} while(false)\




// free memory for file names
#define FREE_FILE_NAMES(in_file_name, out_file_name, input_redir, output_redir)\
do {\
    if (input_redir) {\
        free(in_file_name);\
    }\
    if (output_redir) {\
        free(out_file_name);\
    }\
} while (false)\



// free memory for input file name
#define FREE_INPUT_FILE_NAME(in_file_name, input_redir)\
do {\
    if (input_redir) {\
        free(in_file_name);\
    }\
} while (false)\



// free memory for arguments
#define FREE_ARGUMENTS(argument_list) \
do {\
size_t i = 0;\
while (argument_list[i]) \
    {\
        free(argument_list[i]); \
        i++; \
    }\
} while(false)\



// create file name
// take the file name position
// allocate memory for file name
// initialise file name with token value
// append 0 byte to the end of the file name
#define CREATE_FILE_NAME(cur, file_name)\
do{\
   struct token* file_token = cur->next;\
   char* start_file_pos = file_token->start;\
   file_name = (char*) malloc(file_token->len + 1);\
   memcpy(file_name, start_file_pos, file_token->len); \
   file_name[file_token->len] = '\0';\
} while(false)\



// open file
// if it was needed to do input/output redirection
// then
// iterate through list of commands
// find "<"/">"
// create (allocate + initialise) file_name variable
// call "open" system call
// if it wasn't possible: print "I/O error"
#define OPEN(file_name, tk_type, fd, redir, tokenizer, flags, mode)\
do {\
    if (redir) {\
        struct token* cur = tokenizer->head;\
        while (cur) {\
            if (cur->type == tk_type) {\
                CREATE_FILE_NAME(cur, file_name);\
                fd = open(file_name, flags, mode);\
                if (fd == -1) {\
                    printf("I/O error\n");\
                }\
                break;\
            }\
            cur = cur->next;\
        }\
    }\
} while (false)\




// close standard I/O streams
// if failed, exit
#define CLOSE_STANDARD()\
do {\
    int code = close(STDIN_FILENO);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
    code = close(STDOUT_FILENO);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
}while(false)\





// close input file descriptor
// if there was input redirection
// then
// close input file redirection
// if failure to close, do EXIT_FAIL_WITHOUT_CLOSE
// (free memory and exit)
#define CLOSE_INPUT(in_fd, input_redir, in_file_name, out_file_name, output_redir, argument_list, tokenizer)\
do{\
    if (input_redir) {\
        int code = close(in_fd);\
        if (code == -1) {\
            EXIT_FAIL_WITHOUT_CLOSE(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);\
        }\
    }\
} while(false)\




// close input and output file descriptors
// if there was input/output redirection
// then
// close such descriptors
// if failure, then there are 2 possibilies
// 1) we are running this macros in child process -> no need to free memory
// 2) we are running this macros in parent process -> there is need to free memory
//
// => therefore, based on variable to_free, we either free memory and exit or just exit
#define CLOSE(in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer)\
do {\
    if (input_redir) {\
        int code = close(in_fd);\
        if (code == -1) {\
            EXIT_FAIL_WITHOUT_CLOSE(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);\
        }\
    }\
    if (output_redir) {\
        int code = close(out_fd);\
        if (code == -1) {\
            EXIT_FAIL_WITHOUT_CLOSE(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);\
        }\
    }\
} while(false)\




// failed to open input file
// we are starting with input file => there is no memory for output file
// our job is to:
// 1) free memory for this input file name
// 2) free arguments
// 3) return to proceed to the next command
#define INPUT_OPEN_FAILURE(in_file_name, in_fd, input_redir, argument_list, tokenizer)\
do {\
    if (input_redir && in_fd == -1) {\
        FREE_INPUT_FILE_NAME(in_file_name, input_redir);\
        tokenizer_free(tokenizer);\
        FREE_ARGUMENTS(argument_list);\
        return;\
    }\
} while (false)\




// failed to open output file
// we already processed input file
// =>
// our job is to:
// 1) close input file, if it was opened
// 2) free all memory
// 3) return to proceed to the next command
#define OUTPUT_OPEN_FAILURE(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer)\
do {\
    if (output_redir && out_fd == -1) {\
        CLOSE_INPUT(in_fd, input_redir, in_file_name, out_file_name, output_redir, argument_list, tokenizer);\
        FREE_MEMORY(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);\
        return;\
    }\
} while (false)\



// create child process
// if fork failed, exit with freeing memory ("true" here stands for freeing memory in EXIT_FAIL macros)
#define FORK(pid, in_file_name, out_file_name, in_fd, out_fd, input_redir, output_redir, argument_list, tokenizer) \
do {\
    pid = fork(); \
    if (pid == -1) { \
        EXIT_FAIL(in_file_name, out_file_name,\
        in_fd, out_fd,\
        input_redir, output_redir,\
        argument_list, tokenizer);\
    } \
} while (false)\
    


// division of work between parent and child process
// calling child_work and parent work functions with parameters
// if fork returned 0 => it is child, otherwise it is parent
#define DIVIDE_WORK(parent_work, child_work, child_pid, \
    in_file_name, out_file_name,\
    in_fd,  out_fd,\
    input_redir,  output_redir,\
    argument_list, tokenizer) \
do {\
    if (child_pid == 0) {\
        child_work(in_fd, out_fd, in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);\
    } else {\
        parent_work(child_pid, in_file_name, out_file_name, in_fd, out_fd, input_redir, output_redir, argument_list, tokenizer);\
    }\
} while(false)\




// wait for a child to execute
// if status of child is not 0, ignore
// if waitpid ended with "-1", free memory and exit
// (no need to close files, cause parent does it before waiting child)
#define WAIT_CHILD(child_pid, in_file_name, out_file_name, in_fd, out_fd, input_redir, output_redir, argument_list, tokenizer) \
do\
{\
    int status; \
    int code = waitpid(child_pid, &status, 0); \
    if (code == -1) {\
        EXIT_FAIL(in_file_name, out_file_name,\
        in_fd, out_fd,\
        input_redir, output_redir,\
        argument_list, tokenizer);\
    }\
} while(false)\





// duplicates stdout, to save it, to use it later, if needed
// if failure - closes fd and exit
// we exit without freeing memory, cause parent frees it and child doesn't need to
#define SAVE_STDOUT(saved_stdout_fd, in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer)\
do {\
   if (output_redir) {\
    saved_stdout_fd = dup(STDOUT_FILENO);\
    if (saved_stdout_fd == -1) {\
        EXIT_FAIL(in_file_name, out_file_name,\
        in_fd, out_fd,\
        input_redir, output_redir,\
        argument_list, tokenizer);\
    }\
   }\
}while(false)\





// restores stdout to write in terminal, if needed
// if failure to duplicate or close, exit with failure
#define RESTORE_STDOUT(saved_stdout_fd, in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer)\
do {\
    if (output_redir) {\
        int code = dup2(saved_stdout_fd, 1);\
        if (code == -1) {\
            EXIT_FAIL(in_file_name, out_file_name,\
             in_fd, out_fd,\
             input_redir, output_redir,\
             argument_list, tokenizer);\
        }\
        code = close(saved_stdout_fd);\
        if (code == -1) {\
            EXIT_FAIL(in_file_name, out_file_name,\
             in_fd, out_fd,\
             input_redir, output_redir,\
             argument_list, tokenizer);\
        }\
    }\
} while(false)\





// duplicates stdin and stdout, if failure - closes fd and exit
// we exit without freeing memory, cause parent frees it and child doesn't need to
#define DUP2(in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer)\
do{\
    if (input_redir) {\
        int code = dup2(in_fd, STDIN_FILENO);\
        if (code == -1) {\
            EXIT_FAIL(in_file_name, out_file_name,\
             in_fd, out_fd,\
             input_redir, output_redir,\
             argument_list, tokenizer);\
        }\
    }\
    if (output_redir) {\
        int code = dup2(out_fd, STDOUT_FILENO);\
        if (code == -1) {\
            EXIT_FAIL(in_file_name, out_file_name,\
             in_fd, out_fd,\
             input_redir, output_redir,\
             argument_list, tokenizer);\
        }\
    }\
} while(false)\



// execute command
// if falure:
// we need to write "Command not found"
// to write "Command not found" in shell, we need to restore stdout, cause it was duplicated by DUP2
// then we write error
// then we exit in child, cause there is no need for this process to live
#define EXECUTE(saved_stdout_fd, in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer) \
do {\
    char* command = argument_list[0];\
    int code = execvp(command, argument_list); \
    if (code == -1) { \
        RESTORE_STDOUT(saved_stdout_fd, in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer);\
        printf("Command not found\n");\
        EXIT_FAIL_WITHOUT_CLOSE(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);\
    }\
} while (false)\





// next token exists and is not a "<" or ">"
bool next_word_is_file(struct token* cur) {
    // next token
    struct token* next = cur->next;

    // check
    return next && next->type == TK_WORD;
}



// check syntax
bool correct_syntax(struct tokenizer* tokenizer) {
    // count "<"" and ">"
    int count_infile = 0;
    int count_outfile = 0;
    struct token* cur = tokenizer->head;

    // iterate through tokens
    // until token != NULL
    while (cur) {
        // found "<"
        if (cur->type == TK_INFILE) {
            // increment "<" count
            ++count_infile;

            // check next token - it should exist and should be a word
            // otherwise, return false
            CHECK_NEXT(cur);

        // found ">"
        } else if (cur->type == TK_OUTFILE) {
            // increment ">" count
            ++count_outfile;

            // check next token - it should exist and shoukd be a word
            // otherwise, return false
            CHECK_NEXT(cur);
        }

        // shift
        cur = cur->next;
    }

    // if more than one "<" or more than one ">" -> bad command
    if (count_infile > 1 || count_outfile > 1) {
        return false;
    }

    // else
    // <= 1 "<" and <= 1 ">" and after each "<" and each ">" there is a word
    // => no syntax error

    // we return true
    return true;
}




// tokenization process
void tokenizer_init(struct tokenizer *tokenizer, char *line)
{
    // init tokenizer
    tokenizer->token_count = 0;
    tokenizer->head = NULL;

    // init last token
    struct token *last_token = NULL;  

    // init cur 
    char *cur = line;

    // while(true)
    for (;;) {
        // skip spaces
        SKIP_SPACES(cur);

        // check for end of the string
        CHECK_END(cur);

        // allocate token
        struct token *token = (struct token*)malloc(sizeof(struct token));

        // initialise token
        INIT_TOKEN(cur);
        
        // read token
        READ_TOKEN(cur, token);

        // update tokenizer
        UPDATE_TOKENIZER(tokenizer, token, last_token);
    }
}




// free tokens
void tokenizer_free(struct tokenizer *tokenizer)
{
    // current token
    struct token *cur = tokenizer->head;

    // deallocating tokens
    while (cur) {
        struct token *prev = cur;
        cur = cur->next;
        free(prev);
    }
}


// readline from the shell
int get_user_line(char **line, size_t *maxlen)
{
    // new command symbol
    printf("$ ");

    // read line from shell
    return getline(line, maxlen, stdin);
}


// checks if "<" exists
bool exists_inp_red(struct tokenizer *tokenizer) {
    struct token* cur = tokenizer->head;
    // iterate through tokens
    while (cur) {
        if (cur->type == TK_INFILE) {
            return true;
        }

        cur = cur->next;
    }
    return false;
}



// checks if ">" exists
bool exists_out_red(struct tokenizer *tokenizer) {
    struct token* cur = tokenizer->head;
    // iterate through tokens
    while (cur) {
        if (cur->type == TK_OUTFILE) {
            return true;
        }

        cur = cur->next;
    }
    return false;
}



// child:
// 1) saves stdout for future usage
// 2) duplicates stdin and/or stdout from in_fd and out_fd
// 3) closes in_fd and/or out_fd if needed
// 4) executes command
void child_work(int in_fd, int out_fd,
 char* in_file_name, char* out_file_name,
 bool input_redir, bool output_redir,
  char* argument_list[], struct tokenizer* tokenizer) {

    // saves stdout in saved_stdout_fd
    int saved_stdout_fd;
    SAVE_STDOUT(saved_stdout_fd, in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer);

    // duplicate stdin and stdout in in_fd and out_fd
    DUP2(in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer);

    // close redundant descriptors (false means - no need to free memory)
    CLOSE(in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer);

    // execute command
    EXECUTE(saved_stdout_fd, in_fd, out_fd, input_redir, output_redir, in_file_name, out_file_name, argument_list, tokenizer);
}


// parent:
// 1) closes in_fd and/or out_fd if needed
// 2) waites for a child to end it's work
void parent_work(int child_pid,
 char* in_file_name, char* out_file_name,
   int in_fd, int out_fd,
    bool input_redir, bool output_redir,
     char* argument_list[], struct tokenizer* tokenizer) {

    // close all files in parent (true means no need to free memory)
    CLOSE(in_fd, out_fd,
     input_redir, output_redir,
     in_file_name, out_file_name,
     argument_list, tokenizer);
    
    // wait for a child
    WAIT_CHILD(child_pid, in_file_name, out_file_name,
     in_fd, out_fd,
     input_redir, output_redir,
     argument_list, tokenizer);
}


// execute command from the shell
// 1) checks if command is empty
// 2) checks if there are syntax errors
// 3) allocates memory for arguments for execvp
// 4) checks if there are "<" and ">" in a command
// 5) tries to open input file, if needed
// 6) tries to open output file, if needed
// 7) tries to fork
// 8) divides work between parent and child
// 9) frees all memory
// (parent closes all files, if needed, child closes (and even does exit - therefore closes))
// => no need to close them in this fuction
void exec_command(struct tokenizer *tokenizer) {

    // empty command is ignored
    CHECK_EMPTY(tokenizer);

    // check syntax errors
    CHECK_SYNTAX(tokenizer);

    // allocate arguments
    char* argument_list[tokenizer->token_count + 1];
    ALLOCATE_ARGUMENTS(argument_list);


    // if there is input or output redirection
    bool input_redir = exists_inp_red(tokenizer);
    bool output_redir = exists_out_red(tokenizer);

    // names of input and output files
    char* in_file_name;
    char* out_file_name;

    // input and output file descriptors
    int in_fd = 0;
    int out_fd = 0;

    // open input file: 
    // flag is O_RDONLY - we need to read only, if there is no such file
    // we don't create file => mode can be ignored
    OPEN(in_file_name, TK_INFILE, in_fd, input_redir, tokenizer, O_RDONLY, 0);


    // open input file failure handling
    INPUT_OPEN_FAILURE(in_file_name, in_fd, input_redir, argument_list, tokenizer);

    // open output file:
    // flag is O_WRONLY and O_CREAT - we need to write, if there is no such
    // file, we create it => mode is not ignored =>
    // => we put mode to be
    // user : rw
    // group : r
    // others: r
    // (we need to write in a file and maybe read from it)
    OPEN(out_file_name, TK_OUTFILE, out_fd, output_redir, tokenizer, O_WRONLY | O_CREAT | O_TRUNC,
     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);


    // open output file failure
    OUTPUT_OPEN_FAILURE(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);

    // fork
    int pid;
    FORK(pid, in_file_name, out_file_name, in_fd, out_fd, input_redir, output_redir, argument_list, tokenizer);
    

    // divide work between parent and child
    DIVIDE_WORK(parent_work, child_work, pid, 
    in_file_name, out_file_name,
    in_fd,  out_fd,
    input_redir,  output_redir,
    argument_list, tokenizer);


    // free all memory
    FREE_MEMORY(in_file_name, out_file_name, input_redir, output_redir, argument_list, tokenizer);
}



// main function: does tokinization and executes each command
int main()
{
    // line buffer
    char *line = NULL;

    // size of line
    size_t maxlen = 0;

    // disable stdin buffereing
    setbuf(stdin, NULL);

    // number of bytes readed
    ssize_t len = 0;

    // tokenizer to tokize input commands
    struct tokenizer tokenizer;

    // read lines from shell
    while ((len = get_user_line(&line, &maxlen)) > 0) {

        // tokenize input commands
        tokenizer_init(&tokenizer, line);

        // execute command from the shell
        exec_command(&tokenizer);
    }

    // free line buffer, if not NULL
    if (line) {
        free(line);
    }

    // move cursor to the next line
    printf("\n");

    return 0;
}
