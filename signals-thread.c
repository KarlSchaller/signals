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

void *handlerfunction1(void *args);
void handler1(int sig);
void *handlerfunction2(void *args);
void handler2(int sig);
void *generatorfunction(void *args);
void *reporterfunction(void *args);
void reporthandler(int sig);

int main(int argc, char **argv, char** envp) {
	
	srand(time(NULL));
	pthread_mutex_init(&sc1lock, NULL);
	pthread_mutex_init(&sc2lock, NULL);
	pthread_mutex_init(&rc1lock, NULL);
	pthread_mutex_init(&rc2lock, NULL);
	//pthread_cond_init(&worknotempty, NULL);
	//pthread_cond_init(&lognotempty, NULL);
	//pthread_cond_init(&worknotfull, NULL);
	//pthread_cond_init(&lognotfull, NULL);
	
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
	while (1) {
		if (rand() % 2 == 0) { // SIGUSR1 signal
			puts("Generator: Sending SIGUSR1");
			if (rand() % 2 == 0)
				pthread_kill(handlerthreads[0], SIGUSR1);
			else
				pthread_kill(handlerthreads[1], SIGUSR1);
			pthread_kill(reporterthread, SIGUSR1);
			puts("Generator: Incrementing SIGUSR1");
			pthread_mutex_lock(&sc1lock);
			sentcount1 += 1;
			pthread_mutex_unlock(&sc1lock);
		}
		else { // SIGUSR2 signal
			puts("Generator: Sending SIGUSR2");
			if (rand() % 2 == 0)
				pthread_kill(handlerthreads[2], SIGUSR2);
			else
				pthread_kill(handlerthreads[3], SIGUSR2);
			pthread_kill(reporterthread, SIGUSR2);
			puts("Generator: Incrementing SIGUSR2");
			pthread_mutex_lock(&sc2lock);
			sentcount2 += 1;
			pthread_mutex_unlock(&sc2lock);
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
	

	struct timeval s1_in, s2_in; //data to hold time of receiving
	int sig; //sigwaitinfo retval holder
	int s1_in_ct, s2_in_ct; //local counters for SIGUSR1 and 2.

	struct timeval inittime, fintime, duration;
	int sig1count = 0, sig2count = 0;
	gettimeofday(&inittime, NULL);
	while(1) {
		puts("Reporter: Pausing");
		if ((sig = sigwaitinfo(&rpset, NULL)) == -1) break; // break on failure so we don't report on a failed signal catch
		if(sig == SIGUSR1) {
			puts("Reporter: Handling");
			sig1count += 1;
		}
		if(sig == SIGUSR2) {
			puts("Reporter: Handling");
			sig2count += 1;
		}
		puts("Reporter: Pausing Done");
		
		if (sig1count + sig2count == 10) {
			puts("Reporter: Reporting");
			gettimeofday(&fintime, NULL);
			timersub(&fintime, &inittime, &duration);
			float mduration = duration.tv_sec*1000 + ((float)duration.tv_usec)/1000;
			puts("================================================");
			printf("SIGUSR1 sent: %d\n", sentcount1);
			printf("SIGUSR1 received: %d\n", receivedcount1);
			printf("SIGUSR1 avg time: %f ms\n", mduration/sig1count);
			printf("SIGUSR2 sent: %d\n", sentcount2);
			printf("SIGUSR2 received: %d\n", receivedcount2);
			printf("SIGUSR2 avg time: %f ms\n", mduration/sig2count);
			puts("================================================");
			sig1count = sig2count = 0;
			gettimeofday(&inittime, NULL);
		}
	}
}