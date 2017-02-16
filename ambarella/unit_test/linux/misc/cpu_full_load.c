#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


void *sayhello1(void *arg)
{
	unsigned int n = 0;

	while(1) {
		if (n % 25000000 == 0)
			printf("Hello (1): %u\n", n);

		n++;
	};
}

void *sayhello2(void *arg)
{
	unsigned int n = 0;

	while(1) {
		if (n % 25000000 == 0)
			printf("Hello (2): %u\n", n);

		n++;
	};
}

int main(int argc,char *argv[])
{
         int error;
	 pthread_t wf_fid, rf_fid;

         if ((error = pthread_create(&wf_fid ,NULL, sayhello1, NULL))<0)
         {
                  perror("can't create pthread");
                  return 1;
         }

	 sleep(1);

	 if ((error = pthread_create(&rf_fid, NULL, sayhello2, NULL))<0)
         {
                  perror("can't create pthread");
                  return 1;
         }

         while(1);

         return 0;
}




