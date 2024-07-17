main:main.c
	CC=gcc
	$(CC) main.c -o main -Wall -Wextra -pedantic -std=c99
	./main
