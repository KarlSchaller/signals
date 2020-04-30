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

pthread_t handlerthreads[4];
pthread_t generatorthreads[3];
pthread_t reporterthread;

int sentcount1 = 0, sentcount2 = 0;
pthread_mutex_t sc1lock, sc2lock;
int receivedcount1 = 0, receivedcount2 = 0;
pthread_mutex_t rc1lock, rc2lock;
time_t inittime;
int sig1count, sig2count;

void *handlerfunction1(void *args);
void handler1(int sig);
void *handlerfunction2(void *args);
void handler2(int sig);
void *generatorfunction(void *args);
void *reporterfunction(void *args);
void reporthandler(int sig);

int main(int argc, char **argv, char** envp) {
	
	pthread_mutex_init(&sc1lock, NULL);
	pthread_mutex_init(&sc2lock, NULL);
	pthread_mutex_init(&rc1lock, NULL);
	pthread_mutex_init(&rc2lock, NULL);
	//pthread_cond_init(&worknotempty, NULL);
	//pthread_cond_init(&lognotempty, NULL);
	//pthread_cond_init(&worknotfull, NULL);
	//pthread_cond_init(&lognotfull, NULL);
	
	// Create handling threads
	for (int i = 0; i < 2; i++)
		pthread_create(&(handlerthreads[i]), NULL, handlerfunction1, NULL);
	for (int i = 3; i < 4; i++)
		pthread_create(&(handlerthreads[i]), NULL, handlerfunction2, NULL);
	puts("Handlers");
	
	// Create generating threads
	for (int i = 0; i < 3; i++)
		pthread_create(&(generatorthreads[i]), NULL, generatorfunction, NULL);
	puts("Generators");
	
	// Create reporting threads
	pthread_create(&reporterthread, NULL, reporterfunction, NULL);
	puts("Reporter");
	
	// Main thread
	sleep(30);
	
	// Join threads
	for (int i = 0; i < 4; i++)
		pthread_join(handlerthreads[i], NULL);
	for (int i = 0; i < 3; i++)
		pthread_join(generatorthreads[i], NULL);
	pthread_join(reporterthread, NULL);
	puts("Joined");
	
	exit(0);
}

// 2 threads for handling SIGUSR1
void *handlerfunction1(void *args) {
	if (signal(SIGUSR1, handler1) == SIG_ERR)
		perror("Signal Error");
	while(1)
		pause();
}
void handler1(int sig) {
	pthread_mutex_lock(&rc1lock);
	receivedcount1 += 1;
	pthread_mutex_unlock(&rc1lock);
}

// 2 threads for handling SIGUSR2
void *handlerfunction2(void *args) {
	if (signal(SIGUSR2, handler2) == SIG_ERR)
		perror("Signal Error");
	while(1)
		pause();
}
void handler2(int sig) {
	pthread_mutex_lock(&rc2lock);
	receivedcount2 += 1;
	pthread_mutex_unlock(&rc2lock);
}

// 3 threads for generating signals
void *generatorfunction(void *args) {
	while (1) {
		if (rand() % 2 == 0) { // SIGUSR1 signal
			pthread_mutex_lock(&sc1lock);
			sentcount1 += 1;
			pthread_mutex_unlock(&sc1lock);
			if (rand() % 2 == 0)
				pthread_kill(handlerthreads[0], SIGUSR1);
			else
				pthread_kill(handlerthreads[1], SIGUSR1);
		}
		else { // SIGUSR2 signal
			pthread_mutex_lock(&sc2lock);
			sentcount2 += 1;
			pthread_mutex_unlock(&sc2lock);
			if (rand() % 2 == 0)
				pthread_kill(handlerthreads[2], SIGUSR2);
			else
				pthread_kill(handlerthreads[3], SIGUSR2);
		}
		usleep(rand()%101 + 10);
	}
}

// Thread for reporting
void *reporterfunction(void *args) {
	if (signal(SIGUSR1, reporthandler) == SIG_ERR)
		perror("Signal Error");
	if (signal(SIGUSR2, reporthandler) == SIG_ERR)
		perror("Signal Error");
	inittime = time();
	while(1)
		pause();
}
void reporthandler(int sig) {
	if (sig == SIGUSR1)
		sig1count += 1;
	else
		sig2count += 1;
	if (sig1count + sig2count == 10) {
		time_t duration = time() - inittime;
		printf("Average time between SIGUSR1: %f", ((float)sig1count)/duration);
		printf("Average time between SIGUSR2: %f", ((float)sig2count)/duration);
		sig1count = sig2count = 0;
		inittime = time();
	}
}