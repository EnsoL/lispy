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
enum { LVAL_NUM, LVAL_SEXPR, LVAL_SYM, LVAL_ERR };
/* Create Enumeration of Possible Error Types */
enum { LERR_DIV_BY_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct lval{
	int type;
	long num;

	char* err;
	char* sym;

	unsigned count;
	struct lval** cell;
} lval;

// Creates a new number type lval, converts long -> lval
lval* lval_num(long num){
	lval *lnum = malloc(sizeof(lval));
	lnum->type = LVAL_NUM;
	lnum->num = num;
	return lnum;
}

// Create a new error type lval	
lval* lval_err(char* msg){
	lval *lerr = malloc(sizeof(lval));
	lerr->type = LVAL_ERR;
	lerr->err = malloc(strlen(msg) + 1);
	strcpy(lerr->err, msg);
	return lerr;
}

lval* lval_sym(char* sym){
	lval *lsym = malloc(sizeof(lval));
	lsym->type = LVAL_SYM;
	lsym->sym = malloc(strlen(sym) + 1);
	strcpy(lsym->sym, sym);
	return lsym;
}

lval* lval_sexpr(){
	lval *lsexpr = malloc(sizeof(lval));
	lsexpr->type = LVAL_SEXPR;
	lsexpr->count = 0;
	lsexpr->cell = NULL;
	return lsexpr;
}

void lval_del(lval *v){
	switch(v->type){
		case LVAL_NUM: 
			break;
		case LVAL_ERR:
			free(v->err);
			break;
		case LVAL_SYM:
			free(v->sym);
			break;
		case LVAL_SEXPR:
			for(unsigned i = 0; i < v->count; i++) lval_del(v->cell[i]);
			free(v->cell);
			break;
	}

	free(v);
}

lval* lval_read_num(mpc_ast_t* t){
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("Invalid number.");
}

lval* lval_add(lval* v, lval* l){
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval) * v->count);
	v->cell[v->count - 1] = l;
	return v;
}

lval* lval_read(mpc_ast_t* t){
	if(strstr(t->tag, "number")) return lval_read_num(t);
	if(strstr(t->tag, "sym")) return lval_sym(t->contents);

	lval *x = NULL;
	if(!strcmp(t->tag, ">") || strstr(t->tag, "sexpr")) {
		x= lval_sexpr();
	
		for(unsigned i = 0; i < t->children_num; i++){
			if(!strcmp(t->children[i]->contents, "(")) continue;
			if(!strcmp(t->children[i]->contents, ")")) continue;
			if(!strcmp(t->children[i]->tag, "regex")) continue;
			x = lval_add(x, lval_read(t->children[i]));
		}
	}

	return x;
}

//foreward declaration
void lval_print(lval* val);

void lval_expr_print(lval* v, char open, char close){
	putchar(open);
	for(unsigned i = 0; i < v->count; i++){
		lval_print(v->cell[i]);
		// don't add a space if ya ain't at the end of the expr]
		if((v->count - 1) > i){
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* val){
	switch(val->type){
		case LVAL_NUM: printf("%li", val->num); break;
		case LVAL_ERR:  printf("Error: %s", val->err); break;
		case LVAL_SYM: printf("%s", val->sym); break;
		case LVAL_SEXPR: lval_expr_print(val, '(', ')'); break;
	}
}

void lval_println(lval *val){ lval_print(val); putchar('\n'); }

lval* eval_op(char* op, lval *x, lval *y){
	if(x->type == LVAL_ERR) return x;
	else if(y->type == LVAL_ERR) return y;
	else if(!strcmp(op, "+") | !strcmp(op, "add")) return lval_num(x->num + y->num);
	else if(!strcmp(op, "-") | !strcmp(op, "sub")) return lval_num(x->num - y->num);
	else if(!strcmp(op, "*") | !strcmp(op, "mul")) return lval_num(x->num * y->num);
	else if(!strcmp(op, "/") | !strcmp(op, "div")){ 
		return y->num == 0 ? 
			lval_err(LERR_DIV_BY_ZERO) : 
			lval_num(x->num / y->num);
	}
	else if(!strcmp(op, "%")) lval_num(x->num % y->num);
	else if(!strcmp(op, "^")) return lval_num(pow(x->num, y->num));
	else if(!strcmp(op, "min")) return x->num > y->num ? y : x ;
	else if(!strcmp(op, "max")) return x->num > y->num ? x : y ;
	return lval_err("Invalid operation");
}

// Evaluates expressions, which are either a number or a ( operator/symbol expression+ )
lval* eval(mpc_ast_t* t){
	if(strstr(t->tag, "number")){
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x): lval_err("Invalid number");
	}
	
	// 0 is '(', 1 is an operation, while the last one is ')'
	// the rest are arguments
	char *op = t->children[1]->contents;
	lval *x = eval(t->children[2]);
	
	if((*op) == '-'){
		if(t->children_num == 4) x->num = -x->num;
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
	mpc_parser_t* number  = mpc_new("number");
	mpc_parser_t* symbol  = mpc_new("symbol");
	mpc_parser_t* sexpr      = mpc_new("sexpr");
	mpc_parser_t* expr        = mpc_new("expr");
	mpc_parser_t* lispy       = mpc_new("lispy");

mpca_lang(MPCA_LANG_DEFAULT, // /-?[0-9]+([.,][0-9]+)?/  -  decimal numbers - add later? would be easier with classes, C++
  "                                   																												\
    number	: 	/-?[0-9]+/ ;	   \
    symbol	: 	'+'  |  /add/  |  '-'  |  /sub/  |  '*'  |  /mul/  |  '/'  |  /div/  |  '%'  |  '^'  |  /min/  |  /max/ ;	 \
    sexpr	: 		'(' <expr>* ')';	\
    expr	: 			<number> | <symbol> | <sexpr>;	\
    lispy	: 		/^/ <symbol> <expr>+ /$/; 	   \
 ", number, symbol, sexpr,  expr, lispy);

	puts("Lispy version 0.0.0.5");
	puts("Press Ctrl+C to Exit lispy\n");

	while(1){
		input = readline("> "); //prompt user, and get input
		add_history(input);

		// Attempt to parse
		mpc_result_t r;
		if	(mpc_parse("<stdin>" , input, lispy, &r))	{
			/*
			lval *result  = eval(r.output);
			lval_println(result);
			lval_del(result);
			mpc_ast_delete(r.output);
			*/
			lval *x = lval_read(r.output);
			lval_println(x);
			lval_del(x);
			mpc_ast_delete(r.output);
		} else {
			// On failure, print error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}

	/* Undefine and Deleter parsers */
	mpc_cleanup(5, number, symbol, sexpr, expr, lispy);
	return 0;
}
