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

/* Create Enumeration of Possible lval Types */
enum { LVAL_NUM, LVAL_ERR };
/* Create Enumeration of Possible Error Types */
enum { LERR_DIV_BY_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct{
	int type;
	long num;
	int err;
} lval;
// Creates a new number type lval, converts long -> lval
lval lval_num(long num){
	lval lnum;
	lnum.type = LVAL_NUM;
	lnum.num = num;
	return lnum;
}
// Create a new error type lval	
lval lval_err(int error){
	lval lerr;
	lerr.type = LVAL_ERR;
	lerr.err = error;
	return lerr;
}

void lval_print(lval val){
	switch(val.type){
		case LVAL_NUM: printf("%li", val.num); break;
		case LVAL_ERR: switch(val.err) {
			case LERR_DIV_BY_ZERO: printf("Error: Illegal Operation - Division by Zero"); break;
			case LERR_BAD_OP: printf("Error: Invalid Operator"); break;
			case LERR_BAD_NUM: printf("Error: Invalid Number"); break;
		} break;
	}
}

void lval_println(lval val){ lval_print(val); putchar('\n'); }

lval eval_op(char* op, lval x, lval y){
	if(x.type == LVAL_ERR) return x;
	else if(y.type == LVAL_ERR) return y;
	else if(!strcmp(op, "+") | !strcmp(op, "add")) return lval_num(x.num + y.num);
	else if(!strcmp(op, "-") | !strcmp(op, "sub")) return lval_num(x.num - y.num);
	else if(!strcmp(op, "*") | !strcmp(op, "mul")) return lval_num(x.num * y.num);
	else if(!strcmp(op, "/") | !strcmp(op, "div")){ 
		return y.num == 0 ? 
			lval_err(LERR_DIV_BY_ZERO) : 
			lval_num(x.num / y.num);
	}
	else if(!strcmp(op, "%")) lval_num(x.num % y.num);
	else if(!strcmp(op, "^")) return lval_num(pow(x.num, y.num));
	else if(!strcmp(op, "min")) return x.num > y.num ? y : x ;
	else if(!strcmp(op, "max")) return x.num > y.num ? x : y ;
	return lval_err(LERR_BAD_OP);
}

// Evaluates expressions, which are either a number or a ( operator expression+ )
lval eval(mpc_ast_t* t){
	if(strstr(t->tag, "number")){
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x): lval_err(LERR_BAD_NUM);
	}
	
	// 0 is '(', 1 is an operation, while the last one is ')'
	// the rest are arguments
	char *op = t->children[1]->contents;
	lval x = eval(t->children[2]);
	
	if((*op) == '-'){
		if(t->children_num == 4) return x.num = -x.num;
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
  "                                   																												\
    number	: 	/-?[0-9]+/ ;	   \
    operator	: 	'+'  |  /add/  |  '-'  |  /sub/  |  '*'  |  /mul/  |  '/'  |  /div/  |  '%'  |  '^'  |  /min/  |  /max/ ;	 \
    expr	: 			<number> | '(' <operator> <expr>+ ')';	\
    lispy	: 		/^/ <operator> <expr>+ /$/; 	   \
 ", number, operator, expr, lispy);

	puts("Lispy version 0.0.0.3");
	puts("Press Ctrl+C to Exit lispy\n");

	while(1){
		input = readline("> "); //prompt user, and get input
		add_history(input);

		// Attempt to parse
		mpc_result_t r;
		if	(mpc_parse("<stdin>" , input, lispy, &r))	{
			lval result  = eval(r.output);
			lval_println(result);
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
