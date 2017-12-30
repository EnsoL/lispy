#include <stdio.h>
#include <stdlib.h>	
#define INPUT_SIZE 2048

#ifdef _WIN32
#include <string.h>

static char buffer[INPUT_SIZE];

char* readline(char* prompt){
	fputs(prompt, stdout);
	fgets(buffer, INPUT_SIZE, stdin);

	char *cpy = malloc(strlen(buffer) + 1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = '\0';

	return cpy;
}

void add_history(char* unused){}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

int main(int argc, char** argv){
	char *input;

	puts("Lispy version 0.0.0.2");
	puts("Press Ctrl+C to Exit lispy\n");

	while(1){
		input = readline("> "); //prompt user, and get input
		add_history(input);

		printf("echo: %s\n", input); //echo input

		free(input);
	}

	return 0;
}
