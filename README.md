# RTOS _assignment

## Description

The goal of this assignment is building one application file with 3 periodic threads and an aperiodic thread, and building a custom kernel module that allows to write on the kernel log.
The last task of the assignment is the implementing of a semaphore with a priority ceiling access protocol.
I created two application: the first follows from trask <1> to <5> ("application.c"), the second is exactly the same but with the implementation of the mutex ("application_withMutex.c").

Here is the tasks of the assignment:

* Design an application with 3 threads J1, J2, J3, whose periods are 300ms, 500ms, and 800ms, plus an aperiodic thread J4 in background which is triggered by J2.

* The threads shall just "waste time," as we did in the exercise with threads.

* Design a simple driver with only open, close, and write system calls.

* During its execution, every task:  
	
	1. opens the special file associated with the driver;  
	2. writes to the driver its identifier plus open square brackets (i.e., [1, [2, [3, or [4)  
	3. close the special files  
	4. performs operations (i.e., wasting time)  
	5. performs (i)(ii) and (iii) again to write to the driver its identifier, but with closed square brackets (i.e., 1], 2], 3] or 4]).  

* The write system call writes on the kernel log the string received from the thread. A typical output of the system, when reading the kernel log, can be the following [11][2[11]2][3[11]3][4]. This sequence clearly shows that some threads can be preempted by other threads (if this does not happen, try to increase the computational time of longer tasks).

* Finally, modify the code of all tasks to use semaphores. Every thread now protects all its operations (i) to (v) with a semaphore, which prevents other tasks from preempting. Specifically, use semaphores with a priority ceiling access protocol.  

## How to run
First of all, you need to have root privileges so open a terminal window and type:
```console
$ sudo su
```
Then from inside the `rtos_assignment` folder, you need to compile the kernel module using the Makefile, so simply type:
```console
$ make
```

To install the module, type:
```console
$ insmod module.ko
```

and to check if it was correctly installed, type:
```console
$ /sbin/lsmod
```

and in the list you should find a module called `module`. 

Read the major number associated to the driver (number next to `module`) by typing:
```console
$ cat /proc/devices
```

and create the special file by typing:
```console
$ mknod /dev/module c <majornumber> 0
```

Finally, compile the scheduler by typing:
```console
$ g++ -lpthread application.c -o application
```

and run it with:
```console
$ ./application
```

To read the kernel log, use the command:
```console
$ dmesg
```
