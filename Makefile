CC = gcc
SDIR = src
ODIR = obj

mtte : mtte.c
	$(CC) mtte.c -o mtte -std=c99 -Wall -pedantic