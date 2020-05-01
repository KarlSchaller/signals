# Karl Schaller, CIS 3207, Project 3
all: signals-thread signals-process

signals-thread: signals-thread.c
	gcc signals-thread.c -o signals-thread -pthread

signals-process: signals-process.c
	gcc signals-process.c -o signals-process