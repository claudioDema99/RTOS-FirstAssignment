/*

Claudio Demaria - S5433737

compile with: g++ -lpthread application_withMutex.c -o application_withMutex

This is the application file with 3 periodic threads (J1, J2, J3) and an aperiodic thread (J4) in background which is triggered by J2.
In this code I have directly implemented the first 5 tasks of the assignment without the last one.
The 6th task is due to implement a semaphore with a priority ceiling access protocol: I have implemented it in the "application_withMutex.c" file.

TASKS:

<1> Design an application with 3 threads J1, J2, J3, whose periods are 300ms, 500ms, and 800ms, plus an aperiodic thread J4 in background which is triggered by J2.

<2> The threads shall just "waste time," as we did in the exercise with threads.

<3> Design a simple driver with only open, close, and write system calls.

<4> During its execution, every task 

	(i) opens the special file associated with the driver;

	(ii) writes to the driver its identifier plus open square brackets (i.e., [1, [2, [3, or [4)

	(iii) close the special files

	(iv) performs operations (i.e., wasting time)

	(v) performs (i)(ii) and (iii) again to write to the driver its identifier, but with closed square brackets (i.e., 1], 2], 3] or 4]).

<5> The write system call writes on the kernel log the string received from the thread. A typical output of the system, when reading the kernel log, can be the following [11][2[11]2][3[11]3][4]. This sequence clearly shows that some threads can be preempted by other threads (if this does not happen, try to increase the computational time of longer tasks).

*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

//code of periodic tasks
void task1_code( );
void task2_code( );
void task3_code( );

//code of aperiodic tasks
void task4_code( );

//characteristic function of the thread, only for timing and synchronization
//periodic tasks
void *task1( void *);
void *task2( void *);
void *task3( void *);

//aperiodic tasks
void *task4( void *);

// initialization of mutexes and conditions (only for aperiodic scheduling)
pthread_mutex_t mutex_task_4 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_task_4 = PTHREAD_COND_INITIALIZER;

#define INNERLOOP 1000
#define OUTERLOOP 2000

#define NPERIODICTASKS 3
#define NAPERIODICTASKS 1
#define NTASKS NPERIODICTASKS + NAPERIODICTASKS

long int periods[NTASKS];
struct timespec next_arrival_time[NTASKS];
double WCET[NTASKS];
pthread_attr_t attributes[NTASKS];
pthread_t thread_id[NTASKS];
struct sched_param parameters[NTASKS];
int missed_deadlines[NTASKS];

int
main()
{
  	// set task periods in nanoseconds
	//the first task has period 300 millisecond
	//the second task has period 500 millisecond
	//the third task has period 800 millisecond
	//you can already order them according to their priority; 
	//if not, you will need to sort them
  	periods[0]= 300000000; //in nanoseconds
  	periods[1]= 500000000; //in nanoseconds
  	periods[2]= 800000000; //in nanoseconds

  	//for aperiodic tasks we set the period equals to 0
  	periods[3]= 0;

	//this is not strictly necessary, but it is convenient to assign a name to the maximum and the minimum priority in the system. We call them priomin and priomax.
  	struct sched_param priomax;
  	priomax.sched_priority=sched_get_priority_max(SCHED_FIFO);
  	struct sched_param priomin;
  	priomin.sched_priority=sched_get_priority_min(SCHED_FIFO);

	// set the maximum priority to the current thread (you are required to be superuser)
    	if (getuid() == 0)
        	pthread_setschedparam(pthread_self(), SCHED_FIFO, &priomax);
	
	// open the special file
    	int fd;
    	if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    	{
        	perror("Error opening file");
        	return -1;
    	}

	// this is the string to be written to the special file
	char str[100];
	
	// execute all tasks in standalone modality in order to measure execution times
    	// Use the computed values to update the worst case execution time of each task.
  	int i;    		
    	for (i =0; i < NTASKS; i++)
    	{
		// initialize time_1 and time_2 required to read the clock
	        struct timespec time_1, time_2;
	        clock_gettime(CLOCK_REALTIME, &time_1);

		//we should execute each task more than one for computing the WCET
		//periodic tasks
 	     	if (i==0)
			task1_code();
      		if (i==1)
			task2_code();
      		if (i==2)
			task3_code();
      		
      		//aperiodic tasks
      		if (i==3)
			task4_code();

		clock_gettime(CLOCK_REALTIME, &time_2);

        	// compute the Worst Case Execution Time (in a real case, we should repeat this many times under different conditions, in order to have reliable values
        	WCET[i] = 1000000000 * (time_2.tv_sec - time_1.tv_sec) + (time_2.tv_nsec - time_1.tv_nsec);

        	// write the WCET in the special file
        	sprintf(str, "Worst Case Execution Time %d=%f", i, WCET[i]);
        	if (write(fd, str, strlen(str) + 1) != strlen(str) + 1)
        	{
            		perror("Error writing file");
            		return -1;
        	}
    	}

	// compute U
    	double U = WCET[0] / periods[0] + WCET[1] / periods[1] + WCET[2] / periods[2];

    	// compute Ulub
    	double Ulub = NPERIODICTASKS * (pow(2.0, (1.0 / NPERIODICTASKS)) - 1);

    	// check the sufficient conditions: if they are not satisfied, exit
    	if (U > Ulub)
    	{
        	sprintf(str, "U=%lf Ulub=%lf Non schedulable Task Set", U, Ulub);
        	if (write(fd, str, strlen(str) + 1) != strlen(str) + 1)
        	{
        	    perror("Error writing file");
        	    return -1;
        	}
        	return (-1);
    	}
    	// write the results in the special file
    	sprintf(str, "U=%lf Ulub=%lf Schedulable Task Set", U, Ulub);
    	if (write(fd, str, strlen(str) + 1) != strlen(str) + 1)
    	{
    	    perror("Error writing file");
    	    return -1;
    	}

    	// close the special file
    	close(fd);
    	sleep(5);

  	// set the minimum priority to the current thread: this is now required because we will assign higher priorities to periodic threads to be soon created
	//pthread_setschedparam
  	if (getuid() == 0)
    		pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin);

  	// set the attributes of each task, including scheduling policy and priority
  	for (i =0; i < NPERIODICTASKS; i++)
    	{
		//initializa the attribute structure of task i
      		pthread_attr_init(&(attributes[i]));

		//set the attributes to tell the kernel that the priorities and policies are explicitly chosen, not inherited from the main thread (pthread_attr_setinheritsched) 
      		pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);
      
		// set the attributes to set the SCHED_FIFO policy (pthread_attr_setschedpolicy)
		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

		//properly set the parameters to assign the priority inversely proportional to the period
      		parameters[i].sched_priority = priomin.sched_priority + NTASKS - i;

		//set the attributes and the parameters of the current thread (pthread_attr_setschedparam)
      		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    	}

 	// aperiodic tasks
  	for (int i = NPERIODICTASKS; i < NTASKS; i++)
    	{
      		pthread_attr_init(&(attributes[i]));
      		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

      		//set minimum priority (background scheduling)
      		parameters[i].sched_priority = 0;
      		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    	}

	//declare the variable to contain the return values of pthread_create	
  	int iret[NTASKS];

	//declare variables to read the current time
	struct timespec time_1;
	clock_gettime(CLOCK_REALTIME, &time_1);

  	// set the next arrival time for each task. This is not the beginning of the first period, but the end of the first period and beginning of the next one. 
  	for (i = 0; i < NPERIODICTASKS; i++)
    	{
		long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
		//then we compute the end of the first period and beginning of the next one
		next_arrival_time[i].tv_nsec = next_arrival_nanoseconds % 1000000000;
		next_arrival_time[i].tv_sec = time_1.tv_sec + next_arrival_nanoseconds / 1000000000;
       		missed_deadlines[i] = 0;
    	}

	// create all threads(pthread_create)
  	iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), task1, NULL);
  	iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task2, NULL);
  	iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task3, NULL);
   	iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task4, NULL);

  	// join all threads (pthread_join)
  	pthread_join( thread_id[0], NULL);
  	pthread_join( thread_id[1], NULL);
  	pthread_join( thread_id[2], NULL);

  	// set the next arrival time for each task. This is not the beginning of the first period, but the end of the first period and beginning of the next one. 
  	for (i = 0; i < NTASKS; i++)
    	{
      		printf ("\nMissed Deadlines Task %d=%d", i, missed_deadlines[i]);
		fflush(stdout);
    	}
    	
  	exit(0);
}

// application specific task_1 code
int task1_code()
{
    // strings to write and strings length
    const char *str_i;
    const char *str_f;
    int len_i, len_f;

    // open the special file
    int fd;
    if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // write on the special file the id of the current task
    str_i = " [1 ";
    len_i = strlen(str_i) + 1;
    if (write(fd, str_i, len_i) != len_i)
    {
        perror("write failed");
        return -1;
    }
    // close the special file
    close(fd);

    // this double loop is only required to waste time
    int i, j, i_j;
    for (i = 0; i < OUTERLOOP; i++)
    {
        for (j = 0; j < INNERLOOP; j++)
        {
            i_j = i * j * 0.5;
        }
    }

    /// open the special file
    if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // write on the special file the id of the current task
    str_f = " 1] ";
    len_f = strlen(str_f) + 1;
    if (write(fd, str_f, len_f) != len_f)
    {
        perror("write failed");
        return -1;
    }
    // close the special file
    close(fd);

    return 0;
}

// thread code for task_1 (used only for temporization)
void *task1(void *ptr)
{
    // set thread affinity, that is the processor on which threads shall run
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(0, &cset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

    // execute the task one hundred times... it should be an infinite loop (too dangerous)
    int i = 0;
    for (i = 0; i < 100; i++)
    {
        // execute application specific code
        if (task1_code())
        {
            printf("task1_code failed\n");
            fflush(stdout);
            return NULL;
        }
        
        // sleep until the end of the current period (which is also the start of the new one
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[0], NULL);

        // the thread is ready and can compute the end of the current period for the next iteration
        long int next_arrival_nanoseconds = next_arrival_time[0].tv_nsec + periods[0];
        next_arrival_time[0].tv_nsec = next_arrival_nanoseconds % 1000000000;
        next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec + next_arrival_nanoseconds / 1000000000;
    }
}

int task2_code()
{
    // strings to write and strings lenght
    const char *str_i;
    const char *str_f;
    int len_i, len_f;

    // open the special file
    int fd;
    if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // write on the special file the id of the current task
    str_i = " [2 ";
    len_i = strlen(str_i) + 1;
    if (write(fd, str_i, len_i) != len_i)
    {
        perror("write failed");
        return -1;
    }
    // close the special file
    close(fd);

    // this double loop is only required to waste time
    int i, j, i_j;
    for (i = 0; i < OUTERLOOP; i++)
    {
        for (j = 0; j < INNERLOOP; j++)
        {
            i_j = i * j * 0.5;
        }
    }

    /// open the special file
    if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // The aperiodic thread J4 in background is triggered by J2.
    // When the variable i_j = 777000 (at same point, it takes that particular value..), then aperiodic task 4 must be executed
    if (i_j == 777000)
    {
        const char *str_4 = ":ex(4)";
        int len_4 = strlen(str_4) + 1;
        if (write(fd, str_4, len_4) != len_4)
        {
            perror("write failed");
            return -1;
        }
        pthread_cond_signal(&cond_task_4);
    }

    // write on the special file the id of the current task
    str_f = " 2] ";
    len_f = strlen(str_f) + 1;
    if (write(fd, str_f, len_f) != len_f)
    {
        perror("write failed");
        return -1;
    }
    // close the special file
    close(fd);

    return 0;
}

void *task2(void *ptr)
{
    // set thread affinity, that is the processor on which threads shall run
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(0, &cset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

    int i = 0;
    for (i = 0; i < 60; i++)
    {
        // same as task 1
        if (task2_code())
        {
            printf("task2_code failed\n");
            fflush(stdout);
            return NULL;
        }

        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);
        long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
        next_arrival_time[1].tv_nsec = next_arrival_nanoseconds % 1000000000;
        next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec + next_arrival_nanoseconds / 1000000000;
    }
}

int task3_code()
{
    // strings to write and strings length
    const char *str_i;
    const char *str_f;
    int len_i, len_f;

    // open the special file
    int fd;
    if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // write on the special file the id of the current task
    str_i = " [3 ";
    len_i = strlen(str_i) + 1;
    if (write(fd, str_i, len_i) != len_i)
    {
        perror("write failed");
        return -1;
    }
    // close the special file
    close(fd);

    // this double loop is only required to waste time
    int i, j, i_j;
    for (i = 0; i < OUTERLOOP; i++)
    {
        for (j = 0; j < INNERLOOP; j++)
        {
            i_j = i * j * 0.5;
        }
    }

    // open the special file
    if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // write on the special file the id of the current task
    str_f = " 3] ";
    len_f = strlen(str_f) + 1;
    if (write(fd, str_f, len_f) != len_f)
    {
        perror("write failed");
        return -1;
    }
    // close the special file
    close(fd);

    return 0;
}

void *task3(void *ptr)
{
    // set thread affinity, that is the processor on which threads shall run
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(0, &cset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

    int i = 0;
    for (i = 0; i < 40; i++)
    {
        // same as task 1
        if (task3_code())
        {
            printf("task3_code failed\n");
            fflush(stdout);
            return NULL;
        }
        
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);
        long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
        next_arrival_time[2].tv_nsec = next_arrival_nanoseconds % 1000000000;
        next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec + next_arrival_nanoseconds / 1000000000;
    }
}

int task4_code()
{
    // strings to write and strings length
    const char *str_i;
    const char *str_f;
    int len_i, len_f;

    // open the special file
    int fd;
    if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // write on the special file the id of the current task
    str_i = " [4 ";
    len_i = strlen(str_i) + 1;
    if (write(fd, str_i, len_i) != len_i)
    {
        perror("write failed");
        return -1;
    }
    // close the special file
    close(fd);

    // this double loop is only required to waste time
    int i, j, i_j;
    for (i = 0; i < OUTERLOOP; i++)
    {
        for (j = 0; j < INNERLOOP; j++)
        {
            i_j = i * j * 0.5;
        }
    }

    // open the special file
    if ((fd = open("/dev/my_device", O_RDWR)) == -1)
    {
        perror("open failed");
        return -1;
    }
    // write on the special file the id of the current task
    str_f = " 4] ";
    len_f = strlen(str_f) + 1;
    if (write(fd, str_f, len_f) != len_f)
    {
        perror("write failed");
        return -1;
    }
    // close the special file
    close(fd);

    return 0;
}

void *task4(void *ptr)
{
    // set thread affinity, that is the processor on which threads shall run
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(0, &cset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

    // add an infinite loop
    while (1)
    {
        // wait for the proper condition to be signaled
        pthread_cond_wait(&cond_task_4, &mutex_task_4);

        // execute the task code
        if (task4_code())
        {
            printf("task4_code failed\n");
            fflush(stdout);
            return NULL;
        }
    }
}
