#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>


/**
type of token
1) TK_WORD - an argument
2) TK_INFILE - "<"
3) TK_OUTFILE - ">"
4) TK_PIPE - "|"
**/
enum tokentype
{
    TK_WORD,
    TK_INFILE,
    TK_OUTFILE,
    TK_PIPE,
};


/**
token information
1) start - start position of the token
2) len - number of symbols in the token
3) type - type of the token: TK_WORLD, TK_INFILE, TK_OUTFILE, TK_PIPE
4) next - next token
**/
struct token
{
    char *start;
    size_t len;
    enum tokentype type;
    struct token *next;
};



/**
tokenizer

1) token_count - number of tokens
2) head - pointer to the first token

**/
struct tokenizer
{
    size_t token_count;
    struct token *head;
};




/** skip spaces in command
 ' ', '\t', '\n' are spaces**/
#define SKIP_SPACES(cur)\
do {\
    while (*cur == ' ' || *cur == '\t' || *cur == '\n') { \
        ++cur; \
    }     \
} while (false)\




/** break if it is end of a command
'\0' is end of command
no "do while false pattern, because of <<break>>"**/
#define CHECK_END(cur)\
    if (*cur == '\0') {\
        break;\
    }



/** intialise token and shift cur, next token is NULL**/
#define INIT_TOKEN(cur) \
do {\
    token->start = cur++;\
    token->next = NULL;\
} while(false)\




/**read token**/
#define READ_TOKEN(cur, token) \
do {\
    switch (*token->start) {\
        case '<':\
            token->type = TK_INFILE;\
            break;\
        case '>':\
            token->type = TK_OUTFILE;\
            break;\
        case '|':\
            token->type = TK_PIPE;\
            break;\
        default:\
            token->type = TK_WORD;\
            while (*cur && *cur != ' ' && *cur != '\t' && *cur != '\n') {\
                ++cur;\
            }\
        }\
        token->len = cur - token->start;\
} while(false)\





/**update tokenizer**/
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



/** if empty command, do nothing and return (go to the next command)**/
#define CHECK_EMPTY(tokenizer) \
do {\
    if (tokenizer->token_count == 0 || tokenizer->head == NULL) {\
        tokenizer_free(tokenizer);\
        return; \
    }\
} while (false)\



/**divide work between parent and child**/
#define DIVIDE_WORK(tokenizer, \
        in_file_name, out_file_name,\
        argument_list, number,\
        number_of_pipes, command_number,\
        input_redir,  output_redir, \
        in_fd, out_fd,\
        pipes)\
do {\
    if (command_number == -1) {\
        parent_work(tokenizer, \
        in_file_name, out_file_name,\
        argument_list, number,\
        input_redir,  output_redir, \
        in_fd, out_fd,\
        pipes, number_of_pipes);\
    } else {\
        child_work(tokenizer,\
        in_file_name, out_file_name,\
        argument_list, number,\
        number_of_pipes, command_number,\
        input_redir,  output_redir, \
        in_fd, out_fd,\
        pipes);\
    }\
} while (false);\




/**check double pipe symbol**/
bool check_double_pipe_symbol(struct tokenizer* tokenizer) {
    struct token* cur = tokenizer->head;
    while(cur) {
        // if found double "|" symbols, then it is wrong syntax
        if (cur->next && cur->type == TK_PIPE && (cur->next)->type == TK_PIPE) {
            return false;
        }
        cur = cur->next;
    }
    return true;
}




/**check syntax errors in each command between pipelines**/
bool check_commands_syntax(struct tokenizer* tokenizer) {
    struct token* cur = tokenizer->head;
    // count "<"
    int count_infile = 0;
    // count ">"
    int count_outfile = 0;

    // iterate throuh tokens
    while (cur) {

        // there is no word after "<" or ">"
        if ((cur->type == TK_INFILE || cur->type == TK_OUTFILE)
         && (!cur->next || (cur->next)->type != TK_WORD)) {
            return false;
        }

        // increment "<" and ">" count
        if (cur->type == TK_INFILE) {
            ++count_infile;
        } else if (cur->type == TK_OUTFILE) {
            ++count_outfile;

        // restore "<" and ">" count
        } else if (cur->type == TK_PIPE) {
            count_infile = 0;
            count_outfile = 0;
        }

        // check if there are too many ">" or "<"
        if (count_infile > 1 || count_outfile > 1) {
            return false;
        }

        // shift to the next token
        cur = cur->next;

    }

    // after each "<" and ">" : there is a word
    // there is no
    return true;
}




/**check if there are redundant redirections

1) there can't be input redirections in non-first commands
2) there can't be output redirection in no-last commands
**/
bool check_input_output_redundant_redirections(struct tokenizer* tokenizer) {
    struct token* cur = tokenizer->head;

    bool first = true;

    bool found_out_file = false;

    while (cur) {
        // found outfile
        if (cur->type == TK_OUTFILE) {
            found_out_file = true;
        }

        // found stdin in the non-first command -> problem
        if (!first && cur->type == TK_INFILE) {
            return false;
        }

        
        if (cur->type == TK_PIPE) {
            // change to not the first command
            first = false;

            // if found outfile before "|" ->
            // -> found output redirection in the non-last command -> problem
            if (found_out_file) {
                return false;
            }
        }

        // shift
        cur = cur->next;
    }

    // there was no input redirection in non-first command
    // there was no output redirection in non-last command
    return true;
}


/**checking syntax

1) checking double "|" symbols
2) checking each command between pipelines for syntax errors
3) checking: input redirection can be only in first command, output redirection
can be only in second command


**/
#define CHECK_SYNTAX(tokenizer)\
do {\
    if (!check_double_pipe_symbol(tokenizer)) {\
        tokenizer_free(tokenizer);\
        printf("Syntax error\n");\
        return;\
    }\
    if (!check_commands_syntax(tokenizer)) {\
        tokenizer_free(tokenizer);\
        printf("Syntax error\n");\
        return;\
    }\
    if (!check_input_output_redundant_redirections(tokenizer)) {\
        tokenizer_free(tokenizer);\
        printf("Syntax error\n");\
        return;\
    }\
} while (false)\




/**skip files, when creating memory for arguments**/
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


/**creating argument**/
#define CREATE_ARGUMENT(cur, argument) \
do {\
    char* pos = cur->start;\
    memcpy(argument, pos, cur->len); \
    argument[cur->len] = '\0';\
} while(false)\



/**allocating arguments

1) if found "<" or ">" - skip this symbol and the next symbol
2) if found "|" - put NULL - it means: end of this command
3) if found word - put a word, it is an argument
4) if found NULL - put NULL - it means: end of the last command

**/
#define ALLOCATE_ARGUMENTS(argument_list) \
do {\
    int command_number = 0;\
    int index = 0;\
    struct token* cur = tokenizer->head;\
    while (cur) \
    {\
        SKIP_FILES(cur);\
        if (cur->type == TK_PIPE) {\
            argument_list[command_number][index] = NULL;\
            ++command_number;\
            index = 0;\
        } else if (cur->type == TK_WORD) {\
            char* argument = (char*) malloc((cur->len + 1) * sizeof(char));\
            CREATE_ARGUMENT(cur, argument); \
            argument_list[command_number][index] = argument; \
            ++index;\
        }\
        cur = cur->next;\
    }\
    argument_list[command_number][index] = NULL;\
} while (false)\





/**allocating memory for one file name**/
#define CREATE_FILE_NAME(cur, file_name)\
do {\
    struct token* file_token = cur->next;\
    char* start_pos = file_token->start;\
    file_name = (char*) malloc(file_token->len + 1);\
    memcpy(file_name, start_pos, file_token->len);\
    file_name[file_token->len] = '\0';\
} while(false)\



/**allocating memory for file names, if needed**/
#define ALLOCATE_FILE_NAMES(tokenizer, in_file_name, out_file_name)\
do {\
    struct token* cur = tokenizer->head;\
    while (cur) {\
        if (cur->type == TK_INFILE) {\
            CREATE_FILE_NAME(cur, in_file_name);\
        }\
        if (cur->type == TK_OUTFILE) {\
            CREATE_FILE_NAME(cur, out_file_name);\
        }\
        cur = cur->next;\
    }\
\
} while (false)\



/**free memory for the filenames, if needed**/
#define FREE_FILE_NAMES(in_file_name, out_file_name, input_redir, output_redir)\
do {\
    if (input_redir) {\
        free(in_file_name);\
    }\
    if (output_redir) {\
        free(out_file_name);\
    }\
} while (false)\



/**free memory for arguments

1) iterate through all the count arguments in argument list
2) if it is not NULL, free the argument memory

**/
#define FREE_ARGUMENTS(argument_list, number) \
do {\
for (int i = 0; i < number; ++i) {\
    int j = 0;\
    while (argument_list[i][j]) {\
        free(argument_list[i][j]);\
        ++j;\
    }\
}\
} while(false)\




/**free memory for tokenizer, arguments and file names**/
#define FREE_MEMORY(tokenizer,\
 argument_list, number,\
 in_file_name, out_file_name,\
 input_redir, output_redir)\
do {\
    tokenizer_free(tokenizer);\
    FREE_ARGUMENTS(argument_list, number);\
    FREE_FILE_NAMES(in_file_name, out_file_name, input_redir, output_redir);\
} while (false)\



/**open file for input/output redirection in the first and the last commands**/
#define OPEN(redir, file_name, fd, flags, mode)\
do {\
    if (redir) {\
        fd = open(file_name, flags, mode);\
        if (fd == -1) {\
            printf("I/O error\n");\
        }\
    }\
} while (false)\


/**close with checking error**/
#define CLOSE(fd)\
do {\
    int code = close(fd);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
} while(false)\



/**handling failure after openning file

1) free memory
2) if in_fd is already > 0 and there is input redirection,
we need to close all input file

**/
#define OPEN_FAILURE(tokenizer,\
     argument_list, number,\
     redir, fd,\
     in_file_name, out_file_name,\
     input_redir, output_redir,\
     in_fd)\
do {\
    if (redir && fd == -1) {\
        FREE_MEMORY(tokenizer,\
         argument_list, number,\
         in_file_name, out_file_name,\
         input_redir, output_redir);\
         if (input_redir && in_fd > 0) {\
            CLOSE(in_fd);\
         }\
         return;\
    }\
} while (false)\



/**close files, if they were open**/
#define CLOSE_FILES(input_redir, output_redir, in_fd, out_fd)\
do {\
    if (input_redir) {\
        CLOSE(in_fd);\
    }\
    if (output_redir) {\
        CLOSE(out_fd);\
    }\
\
} while (false)\



/**close one file**/
#define CLOSE_FILE(redir, fd)\
do {\
    if (redir) {\
        CLOSE(fd);\
    }\
\
} while (false)\



/**close pipes**/
#define CLOSE_PIPES(pipes, to_close)\
do {\
    for (int i = 0; i < to_close; ++i) {\
        CLOSE(pipes[i][0]);\
        CLOSE(pipes[i][1]);\
    }\
} while (false)



/**close all pipe ends, except needed for the process**/
#define CLOSE_REDUNDANT_PIPES(pipes, to_close, j, type1, k, type2)\
do {\
    for (int i = 0; i < to_close; ++i) {\
        if (((i != j) || (type1 != 0)) && ((i != k) || (type2 != 0))) {\
            CLOSE(pipes[i][0]);\
        }\
        if (((i != j) || (type1 != 1)) && ((i != k) || (type2 != 1))) {\
            CLOSE(pipes[i][1]);\
        }\
    }\
} while (false)\





/**close redundant

closes redundand files and pipes for a child process

a) files
1) for non-first: input file, if needed
2) for non-last: output file, if needed


b) pipes
1) for the first: everything, except first writing pipe
2) for the last: everything, except last reading pipe
3) for others (i-th one): everything, except (i-1)-th reading and i-th writing

**/
void close_redundant(bool input_redir, bool output_redir,
 int in_fd, int out_fd,
 int number, int number_of_pipes,
 int command_number, int pipes[][2]) {

    // close input file for the non-first command, if needed
    // there is no input redirection for the non-first commands
    if (command_number > 0) {
        CLOSE_FILE(input_redir, in_fd);
    }

    // close output file for the non-last command, if needed
    // there is no input redirection for the non-last commands
    if (command_number + 1 != number) {
        CLOSE_FILE(output_redir, out_fd);
    }


    // if there are pipes, let's close redundant ones
    if (number_of_pipes > 0) {
        // if it is the first command
        // we need to have first pipe for writing only
        if (command_number == 0) {
            CLOSE_REDUNDANT_PIPES(pipes, number_of_pipes, 0, 1, 0, 1);
        }


        // if it is the last command
        // we need to have last pipe for reading only
        else if (command_number + 1 == number) {
            int last = number_of_pipes - 1;
            CLOSE_REDUNDANT_PIPES(pipes, number_of_pipes, last, 0, last, 0);
        }


        // if it is not the first and not the last command
        // suppose, it is i-th command, then we need to mantain open
        // (i-1)-th reading and i-th writing pipe
        else {
            CLOSE_REDUNDANT_PIPES(pipes, number_of_pipes,
             command_number - 1, 0,
             command_number, 1);
        }
    }
}

/**dup2 with checking error**/
#define DUP2(old_fd, new_fd)\
do {\
    int code = dup2(old_fd, new_fd);\
    if (code == -1) {\
        exit(EXIT_FAILURE);\
    }\
} while(false)\



/**dup2 and close with checking errors**/
#define DUP2_AND_CLOSE(old_fd, new_fd)\
do {\
    DUP2(old_fd, new_fd);\
    CLOSE(old_fd);\
} while(false);\




void redirect(int number,
int number_of_pipes, int command_number,
bool input_redir, bool output_redir, 
int in_fd, int out_fd,
int pipes[][2]) {
    
    // there is no pipes
    if (number == 1) {
        if (input_redir) {
            // redirect input file to stdin
            DUP2_AND_CLOSE(in_fd, STDIN_FILENO)
        }
        if (output_redir) {
            // redirect output file to stdout
            DUP2_AND_CLOSE(out_fd, STDOUT_FILENO);
        }
    }

    // there are pipes
    if (number_of_pipes > 0) {

        // it is the first command
        if (command_number == 0) {
            if (input_redir) {
                // redirect input file to stdin
                DUP2_AND_CLOSE(in_fd, STDIN_FILENO);
            }

            // redirect writing pipe to the second command
            DUP2_AND_CLOSE(pipes[0][1], STDOUT_FILENO);
        }

        // it is the last command
        else if (command_number + 1 == number) {
            if (output_redir) {
                //DUP2_AND_CLOSE output file to stdout
                DUP2_AND_CLOSE(out_fd, STDOUT_FILENO);
            }

            int last = number_of_pipes - 1;
            // redirect reading pipe to the pre-last command
            DUP2_AND_CLOSE(pipes[last][0], STDIN_FILENO);
        }

        // it is not the first command or the last command
        else {
            // redirect previous pipe reading to stdin
            DUP2_AND_CLOSE(pipes[command_number - 1][0], STDIN_FILENO);

            // redirect this pipe writing to stdout
            DUP2_AND_CLOSE(pipes[command_number][1], STDOUT_FILENO);
        }
    }
}



/**create pipes

if failure:

1) free memory
2) close input and output files
3) close previously opened pipes
4) exit with failure

**/
#define CREATE_PIPES(pipes, number_of_pipes)\
do {\
    for (size_t i = 0; i < number_of_pipes; ++i) {\
        int code = pipe(pipes[i]);\
        if (code == -1) {\
            exit(EXIT_FAILURE);\
        }\
    }\
} while (false)\




/**fork processes N times

On each iteration i

If this is the main process (parent_pid):

1) create another i-th child of the main process
2) if failed => exit both in child and in parent, when exiting in parent, all childs will exit too
3) save command number as i for a child - this is index - "custom pid"
to associate each process with a command

**/
#define FORK_N_TIMES(parent_pid, number, command_number)\
do{\
    for (int i = 0; i < number; ++i) {\
        pid_t current_pid = getpid();\
        if (current_pid == parent_pid) {\
            pid_t cur_child_pid = fork();\
            if (cur_child_pid == -1) {\
                exit(EXIT_FAILURE);\
            } else {\
                if (cur_child_pid == 0) {\
                    command_number = i;\
                }\
            }\
        }\
    }\
} while (false)\


/**wait children

1) if wait returns > 0 => there are children to wait
2) if wait returns -1 => there is either no children or error
3) if errno is set to ECHILD, there are no children
4) if errno is not set to ECHILD, it is error => parent finishes
and exits all children

**/
#define WAIT()\
do {\
    int status;\
    while (wait(&status) > 0) {}\
    if (errno != ECHILD) {\
        exit(EXIT_FAILURE);\
    }\
    return;\
} while (false)\



/**save stdout to print errors after execvp**/
#define SAVE_STDOUT()\
do {\
   if (output_redir) {\
    saved_stdout_fd = dup(STDOUT_FILENO);\
    if (saved_stdout_fd == -1) {\
        exit(EXIT_FAILURE);\
    }\
   }\
}while(false)\



#define RESTORE_STDOUT(saved_stdout_fd)\
do {\
    if (output_redir) {\
        DUP2_AND_CLOSE(saved_stdout_fd, STDOUT_FILENO);\
    }\
} while (false)



/**run execvp**/
#define EXECUTE(command_number, argument_list, saved_stdout_fd)\
do {\
    char* command = argument_list[command_number][0];\
    int code = execvp(command, argument_list[command_number]);\
    if (code == -1) {\
        RESTORE_STDOUT(saved_stdout_fd);\
        printf("Command not found\n");\
        exit(EXIT_FAILURE);\
    }\
} while (false)\




/**checks if there is input redirection in the first command**/
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




/**checks if there is output redirection in the last command**/
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



/**finding number of commands**/
int find_number_of_commands(struct tokenizer* tokenizer) {
    // at least one command exists
    int count = 1;

    // iterate through tokens
    struct token* cur = tokenizer->head;

    while(cur) {
        if (cur->type == TK_PIPE) {
            ++count;
        }
        cur = cur->next;
    }

    // return number of commands
    return count;
}




/**tokenization process**/
void tokenizer_init(struct tokenizer *tokenizer, char *line)
{
    // init tokenizer
    tokenizer->token_count = 0;
    tokenizer->head = NULL;

    char *cur = line;

    // init last token
    struct token *last_token = NULL;

    // while(true)
    for (;;) {

        // skip spaces
        SKIP_SPACES(cur);

        // check for end of the string
        CHECK_END(cur);

        // init token
        struct token *token = (struct token*)malloc(sizeof(struct token));
        INIT_TOKEN(cur);

        // read token
        READ_TOKEN(cur, token);

        // update tokenizer
        UPDATE_TOKENIZER(tokenizer, token, last_token);
    }
}





/**free memory for tokenizer**/
void tokenizer_free(struct tokenizer *tokenizer)
{
    // head of the tokenizer - the first token
    struct token *cur = tokenizer->head;

    // iteration through all tokens
    while (cur) {
        struct token *prev = cur;
        cur = cur->next;
        free(prev);
    }
}





/**read command line**/
int get_user_line(char **line, size_t *maxlen)
{
    // print "$" as a start symbol of a command line
    printf("$ ");

    // read line from the shell
    return getline(line, maxlen, stdin);
}


/**parent work

parent:

1) frees argument
2) close files
3) close pipes
4) waits for children

**/
#define parent_work(tokenizer,\
in_file_name,  out_file_name,\
argument_list, number,\
input_redir, output_redir, \
in_fd,  out_fd,\
pipes, number_of_pipes) {\
    FREE_MEMORY(tokenizer,\
     argument_list, number,\
      in_file_name, out_file_name,\
       input_redir, output_redir);\
    CLOSE_FILES(input_redir, output_redir, in_fd, out_fd);\
    CLOSE_PIPES(pipes, number_of_pipes);\
    WAIT();\
}


/**child work:

1) close redundant files
2) save stdout in special fd
3) redirect (dup2 + close) files/pipes
4) execute (with restoring saved stdout)

**/
#define child_work(tokenizer,\
in_file_name, out_file_name,\
argument_list, number,\
 number_of_pipes, command_number,\
 input_redir,  output_redir, \
 in_fd,  out_fd,\
 pipes) {\
    close_redundant(input_redir, output_redir,\
     in_fd, out_fd,\
     number, number_of_pipes,\
     command_number, pipes);\
    int saved_stdout_fd;\
    SAVE_STDOUT();\
    redirect(number, number_of_pipes,\
     command_number,\
     input_redir, output_redir,\
     in_fd,  out_fd,\
     pipes);\
    EXECUTE(command_number, argument_list, saved_stdout_fd);\
}



/**execute command**/
void exec_command(struct tokenizer* tokenizer) {
    // empty commands are ignored
    CHECK_EMPTY(tokenizer);
    // check syntax
    CHECK_SYNTAX(tokenizer);

    // find number of commands between pipelines
    int number = find_number_of_commands(tokenizer);

    // allocating memory for the arguments in the commands
    char* argument_list[number][tokenizer->token_count + 1];
    ALLOCATE_ARGUMENTS(argument_list);

    // if there is input redirection in the first command
    bool input_redir = exists_inp_red(tokenizer);

    // if there is output redirection in the last command
    bool output_redir = exists_out_red(tokenizer);


    // allocating memory for the filenames in the commands, if needed
    char* in_file_name;
    char* out_file_name;
    ALLOCATE_FILE_NAMES(tokenizer, in_file_name, out_file_name);

    // file descriptor of an input file in the first command
    int in_fd = 0;

    // file descriptor of an output file in the last command
    int out_fd = 0;

    // open input file for read-only operations 
    OPEN(input_redir,
     in_file_name,
     in_fd,
     O_RDONLY,
     0);
    
    // handle failure after opening input file
    OPEN_FAILURE(tokenizer,
     argument_list, number,
     input_redir, in_fd,
     in_file_name, out_file_name,
     input_redir, output_redir,
     in_fd);

    // open output file for write-only operations
    // create, if it doesn't exist
    // set O_TRUNC to overwrite
    // create file with rw-r--r-- rights
    OPEN(output_redir,
     out_file_name,
     out_fd,
     O_WRONLY | O_CREAT | O_TRUNC,
     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    // handling failure after openning output file
    OPEN_FAILURE(tokenizer,
     argument_list, number,
     output_redir, out_fd,
     in_file_name, out_file_name,
     input_redir, output_redir,
     in_fd);

    // number of pipes equals number of commands - 1
    int number_of_pipes = number - 1;

    // array of pipes
    // if number = 1 => number of pipes is 0 => we need to make array at least size of 1
    int reserve = number_of_pipes;
    if (number_of_pipes == 0) {
        reserve = 1;
    }
    int pipes[reserve][2];

    // create array of pipes
    CREATE_PIPES(pipes, number_of_pipes);

    // remember the pid of the parent process
    pid_t parent_pid = getpid();

    // custom process number
    // -1 - parent process
    // 0...number-1 - child process
    // i-th process executes i-th command
    int command_number = -1;

    // create number child processes of this process
    FORK_N_TIMES(parent_pid, number, command_number);

    // divide work between parent and children
    DIVIDE_WORK(tokenizer, 
        in_file_name, out_file_name,
        argument_list, number,
        number_of_pipes, command_number,
        input_redir,  output_redir, 
        in_fd, out_fd,
        pipes);
}


int main()
{
    // line - the command to read
    char *line = NULL;

    // max length of the command - for "readline" function
    size_t maxlen = 0;

    // disable stdin buffering
    setbuf(stdin, NULL);

    // number of bytes read
    ssize_t len = 0;

    // tokenizer
    struct tokenizer tokenizer;

    while ((len = get_user_line(&line, &maxlen)) > 0) {

        tokenizer_init(&tokenizer, line);

        exec_command(&tokenizer);
    }

    // free line, if it is null
    if (line) {
        free(line);
    }

    // go to the next line
    printf("\n");

    return 0;
}
