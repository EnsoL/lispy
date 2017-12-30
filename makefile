prompt: prompt.c mpc.c mpc.h
	gcc -std=c99 -Wall mpc.c prompt.c -ledit -lm -o prompt
