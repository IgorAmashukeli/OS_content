#include <stdio.h>
#include <stdbool.h>

bool check_key(char* buf) {

    int prev = 122;

    for(int i = 0; buf[i] != '\0'; ++i) {
        if (buf[i] == prev) {
            return true;
        }
        prev ^= buf[i];
    }

    return false;

}


int main()
{
    char buf[1024];
    fgets(buf, sizeof(buf)-1, stdin);
    if (check_key(buf)) {
        printf("OK\n");
        return 0;
    } else {
        printf("NOT OK\n");
        return 1;
    }
}