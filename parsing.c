#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* Valid number or error */
enum { LVAL_NUM, LVAL_ERR };

/* Types of errors */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
typedef struct {
	int type;
	long num;
	int err;
} lval; // lisp value

/* Create a new number type lval */
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

lval eval_op(lval x, char* op, lval y) {
	
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }

	if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
	if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
	if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
	if (strcmp(op, "^") == 0) { return lval_num(x.num ^ y.num); }
	
	if (strcmp(op, "/") == 0) {
		// If second operand is zero return error 
		return y.num == 0
		  ? lval_err(LERR_DIV_ZERO)
		  : lval_num(x.num / y.num);
	}
	
	return lval_err(LERR_BAD_OP);
}

lval eval (mpc_ast_t* t) {
	if (strstr(t->tag, "number")) {
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}
	
	char *op = t->children[1]->contents;
	
	lval x = eval(t->children[2]);
	
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

long count_leaves (mpc_ast_t* t) {
	if ((t->children_num == 0) && (strstr(t->tag, "number"))) { return 1; }
	if (t->children_num >= 1) {
		int total = 0;
		for (int i = 0; i < t->children_num; i++) {
			total = total + count_leaves(t->children[i]);
		}
		return total;
	}
	return 0;
}

long count_branches (mpc_ast_t* t) {
	if ((t->children_num == 0) && (strstr(t->tag, "operator"))) { return 1; }
	if (t->children_num >= 1) {
		int total = 0;
		for (int i = 0; i < t->children_num; i++) {
			total = total + count_branches(t->children[i]);
		}
		return total;
	}
	return 0;
}

void lval_print(lval v) {
	switch (v.type) {
		case LVAL_NUM: printf("%li", v.num); break;
		
		case LVAL_ERR:
			// Error Handling
			if (v.err == LERR_DIV_ZERO) {
				printf("Error: Cannot divide by zero");
			}
			if (v.err == LERR_BAD_OP) {
				printf("Error: Invalid Operator");
			}
			if (v.err == LERR_BAD_NUM) {
				printf("Error: Invalid Number!");
			}
		break;
	}
}

// Create a println function
void lval_println(lval v) { lval_print(v); putchar('\n'); }

int main(int argc, char** argv) {
	// Create parsers 
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	
	// Regular expressions to define grammar 
	mpca_lang(MPCA_LANG_DEFAULT,
	  "                                                     \
		number   : /-?[0-9]+/ ;                             \
		operator : '+' | '-' | '*' | '/' ;                  \
		expr     : <number> | '(' <operator> <expr>+ ')' ;  \
		lispy    : /^/ <operator> <expr>+ /$/ ;             \
	  ",
	Number, Operator, Expr, Lispy);
	

  
	puts("v 0.1");
	puts("Use (Ctrl + C) to Exit\n");
	
	while (1) {
		char* input = readline("m1> ");
		
		add_history(input);
		
		// Attempt to Parse the user Input 
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
		  lval result = eval(r.output);
		  lval_println(result);
		  mpc_ast_delete(r.output);
		} else {
		  // Otherwise Print the Error 
		  mpc_err_print(r.error);
		  mpc_err_delete(r.error);
		}
		
		free(input);
	}
	
	// Undefine and Delete parsers 
	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	
	return 0;
}