/**
 * @name process.c
 * 
 * @description program spawns n processes; each process computes and finds Narcissistic numbers within a ( *              given specified range.
 * 
 * @author Kuanysh Boranbayev
 * @date January 19, 2020
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define PAGESIZE 1024

// structure memory data that each child process carries
struct Process {
    int id;
    unsigned long start;
    unsigned long end;
    long timer;
    char armnums[1000];
};

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
        n /= 10;
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

// a task function that each child process executes
// each child process computes numbers within a specified range to determine Narcissistic numbers
struct Process compute( struct Process *shmem) {
    struct  timeval start, end;
    gettimeofday(&start, NULL);
    
    for (long unsigned j = shmem->start; j <= shmem->end; j++) {
        char armstr[15];
        if (isArmstrongNumber(j, digitCount(j)) == 1) {
            sprintf(armstr, "%ld ", j);
            strncat(shmem->armnums, armstr, sizeof(shmem->armnums));
        }
    }
    gettimeofday(&end, NULL);
    shmem->timer = delay(start, end);
    return *shmem;
}

// writes the results into a file
void write_to_file(struct Process *shmem, char *filename, int n, long time) {
    FILE *fp = fopen(filename, "w");
    long overall_time = 0;
    if (fp == NULL) {
        fprintf (stderr,"fopen\n");
        exit(1);
    }
    
    fprintf(fp, "PID,\tTime (ms),\tFrom,\tTo,\tNarcissistic Numbers\n");
    for (int i = 0; i < n; i++) {
        overall_time += shmem[i].timer;
        fprintf(fp, "%d,\t%ld,\t%ld,\t%ld,\t%s\n", shmem[i].id, shmem[i].timer, shmem[i].start, shmem[i].end, shmem[i].armnums);
    }
    fprintf(fp, "\nParent process time: %ld ms\nSum time of the child processes: %ld ms\n", time, overall_time);
}

// main function execution begins
int main (int argc, char **argv) {
    struct  timeval start, end;
    gettimeofday(&start, NULL);
    long unsigned c;
    int n, power;
    char * filename;

    if (argc != 7) {
        fprintf (stderr,"Usage: -n [number of processes] -c [compute the number] -f [filename to write]\n");
        exit(1);
    }
    
    argc--;
    argv++;
    if (argc > 1 && (strcmp(*argv, "-n") == 0)) {
        if (--argc > 0) {
            argc--;
            argv++;
            n = atoi(*argv);
            printf("-n = %d\n", n);
        }
    }
    argv++;
    if (argc > 1 && (strcmp(*argv, "-c") == 0)) {
        if (--argc > 0) {
            argc--;
            argv++;
            c = atol(*argv);
            power = strlen(*argv);
            printf("-c = %ld power = %d\n", c, power);
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
    
    // Process Code
    pid_t childPID;
    unsigned long p = c / n;
    unsigned long s = 1;
    unsigned long e = p;
    
    struct Process *shmem = malloc(n * sizeof(struct Process));
    shmem = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
   
    for (int i = 0; i < n; i++) {
        childPID = fork();
        
        switch(childPID) {
            case -1:
                fprintf (stderr,"fork()\n");
                exit(1);
            case 0:
                printf("CID =  %d\ts = %ld\t e = %ld\n", getpid(), s, e);
                
                shmem[i].id = getpid();
                shmem[i].start = s;
                shmem[i].end = e;
                
                shmem[i] = compute(&shmem[i]);

                exit(0);
            default:
                printf("PID %d\n", getpid());
                s += p;
                if (e + p > c) {
                    p = c - e;
                }
                e += p;
                
                break;
        }        
    }
    
    // wait for child processes to complete
    for(int i = 0; i < n; i++){
        wait(NULL);
    }
    
    puts("THE END");
    gettimeofday(&end, NULL);
    long time = delay(start, end);
    write_to_file(shmem, filename, n, time);
    return 0;
}
