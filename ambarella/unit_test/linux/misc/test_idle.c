
/*
    Simple utility to know how much CPU use it has
    2013-06-08  Louis
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int get_total_and_idle (int * total_time, int * idle_time)
{
    char line_buffer[256];
    char * argv[11];
    FILE * fp;
//    int usr, system, nice, idle, wait, irq, srq, zero, total;
    int total;
    char * saveptr;
    int i;
    char * token;
    char * start;
    const char * delimiter =" ";

    if (!total_time || !idle_time)
        return -1;

    fp = fopen("/proc/stat", "r");
    if(!fp) {
        printf("open /proc/stat failed \n");
        return -1;
   }
    if (!fgets(line_buffer, 255, fp)) {
        printf("read cpu line failed \n");
        return -1;
   }

      //copy, before strtok_r will modify str
      start=line_buffer;
      total = 0;
      for (i =0 ; i < 11; i++) {
        token =  strtok_r(start, delimiter,&saveptr);
        if (token == NULL) {
          argv[i] = NULL;
          //     printf("argv[%d] is %s\n", i,  argv[i]);
          break;
        }
        else {
          argv[i] = token;
      //     printf("argv[%d] is %s\n", i,  token);
        }
        total+=atoi(argv[i]);
        start = NULL; //for the next token search
      }
  //   printf("argv[0] %s, argv[1] %s, argv[2] %s, argv[3] %s \n", argv[0], argv[1], argv[2], argv[3]);

        fclose(fp);

       *total_time = total;
       *idle_time = atoi(argv[4]);


    return 0;

}


int  main()
{
    int total_time, idle_time;
    int total_time_old =0;
    int idle_time_old = 0;
    int diff_total, diff_idle;
    int cpu_usage;

       while(1) {
            get_total_and_idle(&total_time, &idle_time);
            diff_total= total_time - total_time_old;
            diff_idle  = idle_time - idle_time_old;
            cpu_usage = (1000* (diff_total - diff_idle)/ diff_total + 5)/10;
            total_time_old = total_time;
            idle_time_old = idle_time;
            printf ("CPU Use %d, idle is %d \n", cpu_usage,  100 - cpu_usage);
            sleep(1);
        }

    return 0;
}


