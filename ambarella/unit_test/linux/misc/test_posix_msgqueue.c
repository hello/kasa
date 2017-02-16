
/*
    POSIX message Queue example,  demostrating how to use POSIX message queue for more real-time IPC in Linux.
    it's supported by Linux since 2.6.6, and supports asynchronous notification.
    Tests have proven that POSIX message Queue message passing latency is around 110 microseconds even under
    CPU intensive situation.

    please "man mq_overview"  to know more...

    2013-2-20  Louis
*/


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>


#define USE_SIG_THREAD_TO_ASYNC_NOTIFY

/* Threads */
static void *MainThread (void *);
static void *AnotherThread (void *);

/* Defines */
#define MAIN_QNAME "/MainQueue100"
#define MAX_SIZE  128


pthread_mutex_t wait_mutex;

int mq_test(void)
{
	 pthread_t mainThread, anotherThread;

	 printf ("Creating threads .. \n");
	 pthread_create (&mainThread,    NULL, &MainThread,    NULL);

	 pthread_create (&anotherThread, NULL, &AnotherThread, NULL);

	 pthread_mutex_init (&wait_mutex, NULL);

	 pthread_join (mainThread,    NULL);
	 pthread_join (anotherThread, NULL);

  return 0;
}


char receivebuffer[MAX_SIZE+1];
static void notifySetup(mqd_t *mqdp);


static void threadFunc (union sigval sv)
{

	 mqd_t *mqdp;

	struct timeval time_current;
	 int bytes_read;
	long int time_remote, time_diff;

	//get message queue handler
	mqdp = sv.sival_ptr;

	//set up notify thread again before retrieving data
	notifySetup(mqdp);

	while (( bytes_read = mq_receive(*mqdp, receivebuffer, MAX_SIZE, NULL)) >= 0) {
		gettimeofday(&time_current, NULL);
		time_remote = atoll(receivebuffer);
		time_diff =  (time_current.tv_sec * 1000000L + time_current.tv_usec) - time_remote;

		fprintf(stderr, "time_diff is %ld\n",  time_diff);
	}

	if (errno != EAGAIN ) {
		perror("mq_receive");
		exit(1);
	}

	pthread_exit(NULL);

}


static void notifySetup(mqd_t *mqdp)
{
	struct sigevent sev;
	sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
	sev.sigev_notify_function = threadFunc;
	sev.sigev_notify_attributes = NULL;
	/* Could be pointer to pthread_attr_t structure */
	sev.sigev_value.sival_ptr = mqdp; /* Argument to threadFunc() */
	if (mq_notify(*mqdp, &sev) == -1) {
		perror("mq_notify");
		exit(1);
	}

}



/* Main thread .. Waiting for messages */
static void *MainThread (void *args)
{

	mqd_t queue_handle;
	char buffer[MAX_SIZE+1];


	struct mq_attr msgq_attr;
	struct mq_attr attr;



	attr.mq_flags = 0;
	attr.mq_maxmsg = 200;
	attr.mq_msgsize = MAX_SIZE;
	attr.mq_curmsgs = 0;

	printf ("[MainThread] Inside main thread \n");



	// Let the other thread wait till I am ready!
	pthread_mutex_lock (&wait_mutex);

	// Clear the buffer
	memset (buffer, 0, sizeof(buffer));


	// Detach the thread
	//pthread_detach (pthread_self());

	// unlink the queue if it exisits - debug
	mq_unlink (MAIN_QNAME);


	printf ("[MainThread]Opening MQ \n");

	//create POSIX message queue with attributes
	queue_handle=  mq_open(MAIN_QNAME, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG, &attr);

	 if (queue_handle == -1)
	 {
		  perror ("[MainThread] Error Opening MQ: ");
		  return NULL;
	 }


	 printf("[MainThread] init done . \n");
	 pthread_mutex_unlock (&wait_mutex);

#ifndef USE_SIG_THREAD_TO_ASYNC_NOTIFY
	{
		struct timeval time_current;
		 int bytes_read;
		long int time_remote, time_diff;

		while (( bytes_read = mq_receive(queue_handle, receivebuffer, MAX_SIZE, NULL)) >= 0) {
			gettimeofday(&time_current, NULL);
			time_remote = atoll(receivebuffer);
			time_diff =  (time_current.tv_sec * 1000000L + time_current.tv_usec) - time_remote;

			fprintf(stderr, "time_diff is %ld\n",  time_diff);

		}
	}

#endif

	// Get the MQ attributes
	mq_getattr(queue_handle, &msgq_attr);
	printf("[MainThread] Queue \"%s\":\n"
      "\t- stores at most %ld messages\n"
      "\t- size no bigger than %ld bytes each\n"
      "\t- currently holds %ld messages\n",
       MAIN_QNAME,
       msgq_attr.mq_maxmsg,
       msgq_attr.mq_msgsize,
       msgq_attr.mq_curmsgs);


	notifySetup(&queue_handle);
	pause();

	mq_close (queue_handle);

	return NULL;
}





#define MAX_SEND_BUFFER 70
static void *AnotherThread (void *args)
{

	 mqd_t queue_handle;
	 char buffer[MAX_SEND_BUFFER];
	 unsigned int msgprio = 1;
	 struct timeval cur_time;


	 int count;

	 sleep(1);
	 printf ("[AnotherThread] Inside Another thread \n");

	 pthread_mutex_lock (&wait_mutex);

	 queue_handle=  mq_open(MAIN_QNAME, O_RDWR);
	 if (queue_handle == -1)
	 {

		  perror ("[AnotherThread] Error Opening MQ:");
		  return NULL;
	 }

	 for (count = 0; count < 10000000; count++)
	 {


		gettimeofday(&cur_time, NULL);
		snprintf (buffer, MAX_SEND_BUFFER, "%ld",  (long int)(cur_time.tv_sec *1000000L + cur_time.tv_usec)   );

		if (0 != mq_send (queue_handle, buffer, strlen(buffer)+1, msgprio))
		{

			perror ("[AnotherThread] Sending:");
			mq_close (queue_handle);
			pthread_mutex_unlock (&wait_mutex);

			return NULL;
		}

		usleep(10*1000);

	 }

	 pthread_mutex_unlock (&wait_mutex);
	 mq_close (queue_handle);

	 return NULL;
}


int main (void)
{
	 return mq_test();
}



