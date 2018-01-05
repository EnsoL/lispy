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

long eval_op(char* op, long x, long y){
	if(!strcmp(op, "+") | !strcmp(op, "add")) return x + y;
	if(!strcmp(op, "-") | !strcmp(op, "sub")) return x - y;
	if(!strcmp(op, "*") | !strcmp(op, "mul")) return x * y;
	if(!strcmp(op, "/") | !strcmp(op, "div")) return x / y;
	if(!strcmp(op, "%")) return x % y;
	if(!strcmp(op, "^")) return pow(x, y);
	if(!strcmp(op, "min")) return x > y ? y : x ;
	if(!strcmp(op, "max")) return x > y ? x : y ;

	return 0;
}

// Evaluates expressions, which are either a number or a ( operator expression+ )
long eval(mpc_ast_t* t){
	if(strstr(t->tag, "number")) return atoi(t->contents);  // atoi converts *char to long
	
	// 0 is '(', 1 is an operation, while the last one is ')'
	// the rest are arguments
	char *op = t->children[1]->contents;
	long x = eval(t->children[2]);
	
	if((*op) == '-'){
		if(t->children_num == 4) return -x;
	}

	for(int i = 3; strstr(t->children[i]->tag, "expr"); i++){
		x = eval_op(op, x, eval(t->children[i]));
	}

	return x;
}

long numberOfNodes(mpc_ast_t* t){
	if(t->children_num == 0) return 1;
	else if(t->children_num >= 0) {
		long total = 1;
		for(long i = 0; i < t->children_num; i++){
			total += numberOfNodes(t->children[i]);		
		}
		return total;
	}
	else return 0;
}

int main(int argc, char** argv){
	char *input;
	/* Creates some parsers */
	mpc_parser_t* number   = mpc_new("number");
	mpc_parser_t* operator  = mpc_new("operator");
	mpc_parser_t* expr         = mpc_new("expr");
	mpc_parser_t* lispy         = mpc_new("lispy");

mpca_lang(MPCA_LANG_DEFAULT, // /-?[0-9]+([.,][0-9]+)?/  -  decimal numbers 
  "                                                     																										\
    number   : /-?[0-9]+/ ;                             																	     	\
    operator  : '+' | \"add\" | '-' | \"sub\" | '*' | \"mul\" | '/' | \"div\" | '%' | '^' | \"min\" | \"max\";			\
    expr         : <number> | '(' <operator> <expr>+ ')' ;  																			\
    lispy        : /^/ <operator> <expr>+ /$/ ;             																				\
 ", number, operator, expr, lispy);

	puts("Lispy version 0.0.0.3");
	puts("Press Ctrl+C to Exit lispy\n");

	while(1){
		input = readline("> "); //prompt user, and get input
		add_history(input);

		// Attempt to parse
		mpc_result_t r;
		if	(mpc_parse("<stdin>" , input, lispy, &r))	{
			long result  = eval(r.output);
			printf("%ld \n", result);
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
