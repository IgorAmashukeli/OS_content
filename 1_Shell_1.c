#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


// exit
#define EXIT_FAIL\
    FREE_ARGUMENTS\
    tokenizer_free(tokenizer);\
    exit(EXIT_FAILURE);\


// create child process
// if fork failed, free memory and return from exec_command function
#define FORK \
    int pid = fork(); \
    if (pid == -1) { \
        EXIT_FAIL\
    } \


// division of work between parent and child process
#define DIVIDE_WORK(parent_work, child_work) \
    if (pid == 0) {\
        child_work(tokenizer, argument_list);\
    } else {\
        parent_work(tokenizer, argument_list, pid);\
    }


// wait for a child to execute
// if waitpid ended with -1, free memory exit with EXIT_FAILURE
// if status of child is not 0, ignore
#define WAIT_CHILD \
    int status; \
    int code = waitpid(child_pid, &status, 0); \
    if (code == -1) {\
        EXIT_FAIL\
    }


// if empty command, do nothing - exit in child process
#define CHECK_EMPTY \
    if (tokenizer->token_count == 0 || tokenizer->head == NULL) {\
        return; \
    }



#define CREATE_ARGUMENT \
    char* pos = cur->start;\
    char* argument = (char*) malloc((cur->len + 1) * sizeof(char));\
    memcpy(argument, pos, cur->len); \
    argument[cur->len] = '\0';\


// create char* [] array of arguments from list
#define ALLOCATE_ARGUMENTS \
    char* argument_list[tokenizer->token_count + 1]; \
    size_t index = 0; \
    struct token* cur = tokenizer->head;\
    while (index != tokenizer->token_count) \
    {\
        CREATE_ARGUMENT \
        argument_list[index] = argument; \
        cur = cur->next; \
        index++; \
    }\
    argument_list[index] = NULL;
    

// free arguments
#define FREE_ARGUMENTS \
size_t i = 0;\
while (i != tokenizer->token_count) \
    {\
        free(argument_list[i]); \
        i++; \
    }



// execute program
#define EXECUTE \
    char* command = argument_list[0];\
    int code = execvp(command, argument_list); \
    if (code == -1) { \
        printf("Command not found\n");\
        EXIT_FAIL\
    }



// struct token: start char, size of token, next token
struct token
{
    char *start;
    size_t len;
    struct token *next;
};

// struct tokenuzer: number of tokens, head of the token list
struct tokenizer
{
    size_t token_count;
    struct token *head;
};


// tokenize string (char array to a list of tokens)
void tokenizer_init(struct tokenizer *tokenizer, char *line)
{

    // create empty tokenizer
    tokenizer->token_count = 0;
    tokenizer->head = NULL;

    // current char
    char *cur = line;

    // last token is NULL
    struct token *last_token = NULL;

    // while(true)
    for (;;) {

        // skip all space characters
        while (*cur == ' ' || *cur == '\t' || *cur == '\n') {
            ++cur;
        }

        // found last null "\0" character -> break
        if (*cur == '\0') {
            break;
        }

        // pointer to the start of the word
        char *start = cur;

        // iterate to the position after the end of the word
        while (*cur && *cur != ' ' && *cur != '\t' && *cur != '\n') {
            ++cur;
        }

        // allocate memory for token
        struct token *token = (struct token*)malloc(sizeof(struct token));

        // start char of token is pointer to the start of the word
        token->start = start;

        // length of token is number of increments in while loop
        token->len = cur - start;

        // next token is NULL, cause so far it is last token
        token->next = NULL;

        // if there were tokens before, insert in tail
        if (last_token) {
            last_token->next = token;

        // otherwise, head of all tokens is this first token
        } else {
            tokenizer->head = token;
        }

        // explicitly save token in last_token variable
        last_token = token;

        // increment number of tokens in tokenizer structure
        ++tokenizer->token_count;
    }
}

// free tokenizer list
void tokenizer_free(struct tokenizer *tokenizer)
{
    // take tokenizer head
    struct token *cur = tokenizer->head;

    // iterate through tokens
    while (cur) {
        // save cur in prev
        struct token *prev = cur;
        // shift cur
        cur = cur->next;
        // free prev
        free(prev);
    }
}

// read user's line of text
int get_user_line(char **line, size_t *maxlen)
{
    // command line
    printf("$ ");

    // get line
    return getline(line, maxlen, stdin);
}


void child_work(struct tokenizer* tokenizer, char* argument_list[]) {
    // change child process to run the command
    // stdin and stdout remain the same
    EXECUTE
}

void parent_work(struct tokenizer* tokenizer, char* argument_list[], int child_pid) {
    // parent waits for a child to end the work
    WAIT_CHILD
}

// execute command
void exec_command(struct tokenizer* tokenizer) {
    // empty command is ignored
    CHECK_EMPTY

    // malloc arguments
    ALLOCATE_ARGUMENTS

    // create child process to execute command
    // stdin and stdout are inherited by child process
    FORK

    // do child and parent work
    DIVIDE_WORK(parent_work, child_work)

    // free arguments
    FREE_ARGUMENTS
}


int main()
{
    // current line
    char *line = NULL;

    // max length of a command line to pass to getline function
    size_t maxlen = 0;

    // disable stdin buffering
    setbuf(stdin, NULL); 

    // length of the command
    ssize_t len = 0;

    // tokenizer to read from command line
    struct tokenizer tokenizer;

    // reading line until EOF
    while ((len = get_user_line(&line, &maxlen)) > 0) {

        // tokenize from line to tokenizer
        tokenizer_init(&tokenizer, line);

        // Your code goes here

        exec_command(&tokenizer);

        // free tokenizer
        tokenizer_free(&tokenizer);
    }

    //free pointer to the line
    if (line) {
        free(line);
    }

    // go to the next line
    printf("\n");

    // while loop ended -> found EOF -> return 0
    return 0;
}