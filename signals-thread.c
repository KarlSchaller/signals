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
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

pthread_t handlerthreads[4];
pthread_t generatorthreads[3];
pthread_t reporterthread;

int sentcount1 = 0, sentcount2 = 0;
pthread_mutex_t sc1lock, sc2lock;
int receivedcount1 = 0, receivedcount2 = 0;
pthread_mutex_t rc1lock, rc2lock;
int mode = 0;

void *handlerfunction1(void *args);
//void handler1(int sig);
void *handlerfunction2(void *args);
//void handler2(int sig);
void *generatorfunction(void *args);
void *reporterfunction(void *args);
//void reporthandler(int sig);

int main(int argc, char **argv, char** envp) {
	
	if (argc == 2)
		mode = atoi(argv[1]);
	
	srand(time(NULL));
	pthread_mutex_init(&sc1lock, NULL);
	pthread_mutex_init(&sc2lock, NULL);
	pthread_mutex_init(&rc1lock, NULL);
	pthread_mutex_init(&rc2lock, NULL);
	
	// Create handling threads
	puts("Handlers");
	for (int i = 0; i < 2; i++)
		pthread_create(&(handlerthreads[i]), NULL, handlerfunction1, NULL);
	for (int i = 2; i < 4; i++)
		pthread_create(&(handlerthreads[i]), NULL, handlerfunction2, NULL);
	sleep(1);
	
	// Create reporting threads
	puts("Reporter");
	pthread_create(&reporterthread, NULL, reporterfunction, NULL);
	sleep(1);
	
	// Create generating threads
	puts("Generators");
	for (int i = 0; i < 3; i++)
		pthread_create(&(generatorthreads[i]), NULL, generatorfunction, NULL);
	
	// Main thread
	if (mode <= 0)
		sleep(30);
	
	// Join threads
	puts("Joining");
	for (int i = 0; i < 3; i++) {
		if (mode <= 0)
			pthread_cancel(generatorthreads[i]);
		pthread_join(generatorthreads[i], NULL);
	}
	for (int i = 0; i < 4; i++) {
		pthread_cancel(handlerthreads[i]);
		pthread_join(handlerthreads[i], NULL);
	}
	pthread_cancel(reporterthread);
	pthread_join(reporterthread, NULL);
	
	if (mode == 0) { // execute 100K signals mode if default mode
		char *args[] = {argv[0], "100000", NULL};
		execvp(argv[0], args);
	}
	
	exit(0);
}

// 2 threads for handling SIGUSR1
void *handlerfunction1(void *args) {
	puts("Handler: Start");
	sigset_t s1set;
	sigemptyset(&s1set);
	sigaddset(&s1set, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &s1set, NULL);
	
	while(1) {
		puts("Handler: Waiting");
		if (sigwaitinfo(&s1set, NULL) == -1)
			break;
		else { // signal is received
			puts("Handler: Incrementing");
			pthread_mutex_lock(&rc1lock);
			receivedcount1 += 1;
			pthread_mutex_unlock(&rc1lock);
		}
	}
	pthread_exit(NULL);
	return NULL;
}

// 2 threads for handling SIGUSR2
void *handlerfunction2(void *args) {
	puts("Handler: Start");
	sigset_t s2set;
	sigemptyset(&s2set);
	sigaddset(&s2set, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &s2set, NULL);
	
	while(1) {
		puts("Handler: Waiting");
		if (sigwaitinfo(&s2set, NULL) == -1)
			break;
		else { // signal is received
			puts("Handler: Incrementing");
			pthread_mutex_lock(&rc2lock);
			receivedcount2 += 1;
			pthread_mutex_unlock(&rc2lock);
		}
	}
	pthread_exit(NULL);
	return NULL;
}

// 3 threads for generating signals
void *generatorfunction(void *args) {
	puts("Generator: Start");
	while (mode <= 0 || sentcount1 + sentcount2 < mode) {
		if (rand() % 2 == 0) { // SIGUSR1 signal
			puts("Generator: Incrementing SIGUSR1");
			pthread_mutex_lock(&sc1lock);
			sentcount1 += 1;
			pthread_mutex_unlock(&sc1lock);
			puts("Generator: Sending SIGUSR1");
			if (rand() % 2 == 0)
				pthread_kill(handlerthreads[0], SIGUSR1);
			else
				pthread_kill(handlerthreads[1], SIGUSR1);
			pthread_kill(reporterthread, SIGUSR1);
		}
		else { // SIGUSR2 signal
			puts("Generator: Incrementing SIGUSR2");
			pthread_mutex_lock(&sc2lock);
			sentcount2 += 1;
			pthread_mutex_unlock(&sc2lock);
			puts("Generator: Sending SIGUSR2");
			if (rand() % 2 == 0)
				pthread_kill(handlerthreads[2], SIGUSR2);
			else
				pthread_kill(handlerthreads[3], SIGUSR2);
			pthread_kill(reporterthread, SIGUSR2);
		}
		puts("Generator: Waiting");
		usleep(rand()%90001 + 10000);
		puts("Generator: Waiting Done");
	}
}

// Thread for reporting
void *reporterfunction(void *args) {
	puts("Reporter: Start");
	sigset_t rpset;
	sigemptyset(&rpset);
	sigaddset(&rpset, SIGUSR1);
	sigaddset(&rpset, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &rpset, NULL);

	struct timeval inittime1, inittime2, fintime1, fintime2, duration1, duration2;
	int sig1count = 0, sig2count = 0, sig;
	gettimeofday(&inittime1, NULL);
	gettimeofday(&inittime2, NULL);
	while(1) {
		puts("Reporter: Pausing");
		if ((sig = sigwaitinfo(&rpset, NULL)) == -1) break; // break on failure so we don't report on a failed signal catch
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
		
		if (sig1count + sig2count == 10) {
			puts("Reporter: Reporting");
			timersub(&fintime1, &inittime1, &duration1);
			timersub(&fintime2, &inittime2, &duration2);
			float mduration1 = duration1.tv_sec*1000 + ((float)duration1.tv_usec)/1000;
			float mduration2 = duration2.tv_sec*1000 + ((float)duration2.tv_usec)/1000;
			puts("================================================");
			printf("SIGUSR1 sent: %d\n", sentcount1);
			printf("SIGUSR1 received: %d\n", receivedcount1);
			printf("SIGUSR1 avg time: %f ms\n", mduration1/sig1count);
			printf("SIGUSR2 sent: %d\n", sentcount2);
			printf("SIGUSR2 received: %d\n", receivedcount2);
			printf("SIGUSR2 avg time: %f ms\n", mduration2/sig2count);
			puts("================================================");
			sig1count = sig2count = 0;
			inittime1 = fintime1;
			inittime2 = fintime2;
		}
	}
}