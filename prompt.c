#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
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
	/* Creates some parsers */
	mpc_parser_t* number   = mpc_new("number");
	mpc_parser_t* operator  = mpc_new("operator");
	mpc_parser_t* expr         = mpc_new("expr");
	mpc_parser_t* lispy         = mpc_new("lispy");

	/* Defines them */
	mpca_lang(MPCA_LANG_DEFAULT,
	"																									\
		number	   :		/-?[0-9]+/	 												;	\
	    operator    :    	'+'		|	'-'    |  	'/'		|	'*'	 							;	\
	    expr			   :    <number> | '(' <operator> <expr>+ ')'		;   \
	    lispy		   :		/^/ (<operator> <expr>+)? /$/						;	\
		", number, operator, expr, lispy);

	puts("Lispy version 0.0.0.3");
	puts("Press Ctrl+C to Exit lispy\n");

	while(1){
		input = readline("> "); //prompt user, and get input
		add_history(input);

		// Attempt
		mpc_result_t r;
		if	(mpc_parse("<stdin>" , input, lispy, &r))	{
			// On success, print AST
			mpc_ast_print(r.output);
			mpc_ast_delete(r.output);
		} else {
			// On failure, print error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}

	/* Undefine and Deleter parsers */
	mpc_cleanup(4, number, operator, expr, lispy);
	return 0;
}
