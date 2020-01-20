/**
 * @name thread.c
 * 
 * @description program spawns n threads; each thread computes and finds Narcissistic numbers within a ( * *              given specified range.
 * 
 * @author Kuanysh Boranbayev
 * @date January 19, 2020
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

// structure memory data that each thread carries
struct Thread {
    int id;
    unsigned long start;
    unsigned long end;
    long timer;
    char armnums[1000];
    struct timeval t;
};

// global variables serve as shared memory variables among threads
int nt; 
struct Thread *shmem;
pthread_mutex_t lock;

// measures the latency of the execution by substracting start time of the execution from the end time
long delay (struct timeval t1, struct timeval t2)
{
	long d;

	d = (t2.tv_sec - t1.tv_sec) * 1000;
	d += ((t2.tv_usec - t1.tv_usec + 500) / 1000);
	return(d);
}

// multiplies a digit n by itself p times
long int powerOf(long int n, int p) {
    long int result = 1;
    for (int i = 1; i <= p; i++) {
        result *= n;
    }
    return result;
}

// multiplies each digit by the total number of digits, and sums all of them together
long int sumElement(long int* ar, int p) {
    long int result = 0;
    
    for (int i = 0; i < p; i++ ) {
        result += powerOf(ar[i], p);
    }
    return result;
}

// counts the number of digits of the number
int digitCount(long long n) {
    int count = 0;
    while (n != 0) {
        n /= 10;     // n = n/10
        ++count;
    }
    return count;
}

// checks a number for Narcissistic characteristics
int isArmstrongNumber(long n, int power) {
    long original = n, result = 0;
    int remainder;
    
    while (original != 0) {
        remainder = original % 10;
        result += powerOf(remainder, power);
        original /= 10;
    }
    
    if (result == n) {
        return 1;
    }
    return 0;
}

// a task function that each spawned thread executes
// each spawned thread computes numbers within a specified range to determine Narcissistic numbers
void * compute(void* arg) {
    struct  timeval start, end;
    gettimeofday(&start, NULL);
    struct Thread *t = (struct Thread *)arg;
    
    int id = pthread_self();
    
    for (unsigned long j = t->start; j <= t->end; j++) {
        char armstr[15];
        if (isArmstrongNumber(j, digitCount(j)) == 1) {
            sprintf(armstr, "%ld ", j);
            strncat(t->armnums, armstr, sizeof(t->armnums));
        }
    }
    
    gettimeofday(&end, NULL);
    t->timer = delay(start, end);
    
    pthread_mutex_lock(&lock);
    
    shmem[t->id] = *t;
    shmem[t->id].id = id;
    
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

// writes the results into a file
void write_to_file(struct Thread *shmem, char *filename, int n, long time) {
    FILE *fp = fopen(filename, "w");
    long overall_time = 0;
    if (fp == NULL) {
        fprintf (stderr,"fopen\n");
        exit(1);
    }
    
    fprintf(fp, "TID,\tTime (ms),\tFrom,\tTo,\tNarcissistic Numbers\n");
    for (int i = 0; i < n; i++) {
        overall_time += shmem[i].timer;
        fprintf(fp, "%d,\t%ld,\t%ld,\t%ld,\t%s\n", shmem[i].id, shmem[i].timer, shmem[i].start, shmem[i].end, shmem[i].armnums);
    }
    fprintf(fp, "\nParent process time: %ld ms\nSum time of the spawned threads: %ld ms\n", time, overall_time);
}

// main function execution begins
int main (int argc, char **argv) {
    struct  timeval start, end;
    gettimeofday(&start, NULL);
    long unsigned c;
    char * filename;
    
    if (argc != 7) {
        fprintf (stderr,"Usage: -n [number of threads] -c [compute the number] -f [filename to write]\n");
        exit(1);
    }
    
    argc--;
    argv++;
    if (argc > 1 && (strcmp(*argv, "-n") == 0)) {
        if (--argc > 0) {
            argc--;
            argv++;
            nt = atoi(*argv);
            printf("-n = %d\n", nt);
        }
    }
    argv++;
    if (argc > 1 && (strcmp(*argv, "-c") == 0)) {
        if (--argc > 0) {
            argc--;
            argv++;
            c = atol(*argv);
            printf("-c = %ld\n", c);
        }
    }
    argv++;
    if (argc > 1 && (strcmp(*argv, "-f") == 0)) {
        if (--argc > 0) {
            argc--;
            argv++;
            filename = *argv;
            printf("-f = %s\n", filename);
        }
    }
    
    // Thread Code
    int rc, p = c / nt;
    long unsigned int s = 1;
    long unsigned int e = p;
    
    shmem = malloc(nt * sizeof(struct Thread));
    
    pthread_t thr[nt];
    
    pthread_mutex_init(&lock, NULL);
    
    for (int i = 0; i < nt; i++) { 
        shmem[i].id = i;
        shmem[i].start = s;
        shmem[i].end = e;
        shmem[i].t = start; 
        
        if (( rc = pthread_create(&thr[i], NULL, compute, &shmem[i]))) {
            fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
            return EXIT_FAILURE;
        }
        
        printf("TID = %ld\ts = %ld\t e = %ld\n", thr[i], s, e);
        
        s += p;
        if (e + p > c) {
            p = c - e;
        }
        e += p;
    }
    
    /* block until all threads complete */
    for (int i = 0; i < nt; ++i) {
        pthread_join(thr[i], NULL);
    }
    
    puts("THE END");
    gettimeofday(&end, NULL);
    long time = delay(start, end);
    write_to_file(shmem, filename, nt, time);
    return 0;
}
