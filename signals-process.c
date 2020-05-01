/*
============================================================================
Name        : Karl Schaller
Date        : 04/29/2020
Course      : CIS3207
Homework    : Assignment 4 Signals
 ============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>

pid_t handlerprocesses[4];
pid_t generatorprocesses[3];
pid_t reporterprocess;

int countersid;
int *countersptr; // sentcount1, sentcount2, receivedcount1, receivedcount2
int mode = 0;
FILE *fp;

void handlerfunction1();
//void handler1(int sig);
void handlerfunction2();
//void handler2(int sig);
void generatorfunction();
void reporterfunction();
//void reporthandler(int sig);

int main(int argc, char **argv, char** envp) {
	
	fp = fopen("runs-process", "a");
	if (argc == 2) {
		mode = atoi(argv[1]);
		puts("PART B");
		fprintf(fp, "PART B\n");
	}
	else {
		puts("PART A");
		fprintf(fp, "PART A\n");
	}
	
	if ((countersid = shmget(IPC_PRIVATE, 4*sizeof(int), IPC_CREAT | 0666)) > 0)
		perror("shmget error");
	if ((countersptr = shmat(countersid, NULL, 0)) == (int *) -1)
		perror("shmat error");
	countersptr[0] = countersptr[1] = countersptr[2] = countersptr[3] = 0;
	
	srand(time(NULL));
	
	// Create handling processes
	puts("Handlers");
	for (int i = 0; i < 2; i++) {
		if ((handlerprocesses[i] = fork()) == 0)
			handlerfunction1();
	}
	for (int i = 2; i < 4; i++) {
		if ((handlerprocesses[i] = fork()) == 0)
			handlerfunction2();
	}
	sleep(1);
	
	// Create reporting processes
	puts("Reporter");
	if ((reporterprocess = fork()) == 0)
		reporterfunction();
	sleep(1);
	
	// Create generating processes
	puts("Generators");
	for (int i = 0; i < 3; i++) {
		if ((generatorprocesses[i] = fork()) == 0)
			generatorfunction();
	}
	
	// Main processes
	if (mode <= 0)
		sleep(30);
	
	// Terminate children
	puts("Joining");
	for (int i = 0; i < 3; i++) {
		if (mode <= 0)
			kill(generatorprocesses[i], SIGKILL);
		else {
			wait(NULL);
			sleep(1);
		}
	}
	for (int i = 0; i < 4; i++)
		kill(handlerprocesses[i], SIGKILL);
	kill(reporterprocess, SIGKILL);
	
	fclose(fp);
	
	if (mode == 0) { // execute 100K signals mode if default mode
		char *args[] = {argv[0], "100000", NULL};
		execvp(argv[0], args);
	}
	exit(0);
}

// 2 threads for handling SIGUSR1
void handlerfunction1() {
	puts("Handler: Start");
	sigset_t s1set;
	sigemptyset(&s1set);
	sigaddset(&s1set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &s1set, NULL);
	
	while(1) {
		puts("Handler: Waiting");
		if (sigwaitinfo(&s1set, NULL) == -1)
			break;
		else { // signal is received
			puts("Handler: Incrementing");
			countersptr[2] += 1;
		}
	}
	exit(1);
}

// 2 threads for handling SIGUSR2
void handlerfunction2() {
	puts("Handler: Start");
	sigset_t s2set;
	sigemptyset(&s2set);
	sigaddset(&s2set, SIGUSR2);
	sigprocmask(SIG_BLOCK, &s2set, NULL);
	
	while(1) {
		puts("Handler: Waiting");
		if (sigwaitinfo(&s2set, NULL) == -1)
			break;
		else { // signal is received
			puts("Handler: Incrementing");
			countersptr[3] += 1;
		}
	}
	exit(1);
}

// 3 threads for generating signals
void generatorfunction() {
	puts("Generator: Start");
	while (mode <= 0 || countersptr[0] + countersptr[1] < mode) {
		if (rand() % 2 == 0) { // SIGUSR1 signal
			puts("Generator: Incrementing SIGUSR1");
			countersptr[0] += 1;
			puts("Generator: Sending SIGUSR1");
			if (rand() % 2 == 0)
				kill(handlerprocesses[0], SIGUSR1);
			else
				kill(handlerprocesses[1], SIGUSR1);
			kill(reporterprocess, SIGUSR1);
		}
		else { // SIGUSR2 signal
			puts("Generator: Incrementing SIGUSR2");
			countersptr[1] += 1;
			puts("Generator: Sending SIGUSR2");
			if (rand() % 2 == 0)
				kill(handlerprocesses[2], SIGUSR2);
			else
				kill(handlerprocesses[3], SIGUSR2);
			kill(reporterprocess, SIGUSR2);
		}
		puts("Generator: Waiting");
		usleep(rand()%90001 + 10000);
		puts("Generator: Waiting Done");
	}
}

// Thread for reporting
void reporterfunction() {
	puts("Reporter: Start");
	sigset_t rpset;
	sigemptyset(&rpset);
	sigaddset(&rpset, SIGUSR1);
	sigaddset(&rpset, SIGUSR2);
	sigprocmask(SIG_BLOCK, &rpset, NULL);
	
	struct timeval inittime1, inittime2, fintime1, fintime2, duration1, duration2;
	int sig1count = 0, sig2count = 0, sig;
	gettimeofday(&inittime1, NULL);
	gettimeofday(&inittime2, NULL);
	while(1) {
		puts("Reporter: Pausing");
		if ((sig = sigwaitinfo(&rpset, NULL)) == -1) break; // break on failure
		if(sig == SIGUSR1) {
			puts("Reporter: Handling");
			sig1count += 1;
			gettimeofday(&fintime1, NULL);
		}
		if(sig == SIGUSR2) {
			puts("Reporter: Handling");
			sig2count += 1;
			gettimeofday(&fintime2, NULL);
		}
		puts("Reporter: Pausing Done");
		
		if (sig1count + sig2count >= 10) {
			puts("Reporter: Reporting");
			timersub(&fintime1, &inittime1, &duration1);
			timersub(&fintime2, &inittime2, &duration2);
			float mduration1 = duration1.tv_sec*1000 + ((float)duration1.tv_usec)/1000;
			float mduration2 = duration2.tv_sec*1000 + ((float)duration2.tv_usec)/1000;
			time_t systime = time(NULL);
			puts("================================================");
			printf("%s", ctime(&systime));
			printf("SIGUSR1 sent: %d\n", countersptr[0]);
			printf("SIGUSR1 received: %d\n", countersptr[2]);
			printf("SIGUSR1 avg time: %f ms\n", mduration1/sig1count);
			printf("SIGUSR2 sent: %d\n", countersptr[1]);
			printf("SIGUSR2 received: %d\n", countersptr[3]);
			printf("SIGUSR2 avg time: %f ms\n", mduration2/sig2count);
			puts("================================================");
			fprintf(fp, "================================================\n");
			fprintf(fp, "%s", ctime(&systime));
			fprintf(fp, "SIGUSR1 sent: %d\n", countersptr[0]);
			fprintf(fp, "SIGUSR1 received: %d\n", countersptr[2]);
			fprintf(fp, "SIGUSR1 avg time: %f ms\n", mduration1/sig1count);
			fprintf(fp, "SIGUSR2 sent: %d\n", countersptr[1]);
			fprintf(fp, "SIGUSR2 received: %d\n", countersptr[3]);
			fprintf(fp, "SIGUSR2 avg time: %f ms\n", mduration2/sig2count);
			fprintf(fp, "================================================\n");
			sig1count = sig2count = 0;
			inittime1 = fintime1;
			inittime2 = fintime2;
		}
	}
}