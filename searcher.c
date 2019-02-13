#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "dataTypes.h"

/* Arguments: ./searcher datafile pattern start end results_fd stats_fd root_id*/
int main(int argc,char *argv[]){
    if(argc!=8){
        printf("Wrong ammount of arguments\n");
        exit(0);
    }

    FILE *fpb;
    Record rec;
    clock_t clockstart = clock();
    /* Parent Pipes' File Descriptors:*/
    int results_fd = atoi(argv[5]), stats_fd = atoi(argv[6]);
    int start = atoi(argv[3]), end = atoi(argv[4]),i;
    char *data_file = argv[1], *pattern = argv[2], recstring[RECORD_SIZE];
    pid_t root_id = atoi(argv[7]);
    //printf("I will be searching through [%d,%d). There are  %d nodes\n",start,end,end-start);
   
    fpb = fopen (data_file,"rb");
    if (fpb==NULL) {
        printf("Cannot open binary file\n");
        return 1;
    }

    for (i=0; i<end ; i++) {
        fread(&rec, sizeof(rec), 1, fpb);
        if(i<start)  /* Ignore records that are out of range [start,end) */
            continue;

        sprintf(recstring,"%ld %s %s  %s %d %s %s %-9.2f",\
                rec.custid, rec.LastName, rec.FirstName, \
                rec.Street, rec.HouseID, rec.City, rec.postcode, \
                rec.amount);
        if(strstr(recstring,pattern)){
            if(write(results_fd,recstring,RECORD_SIZE)==-1){
                printf("Error in write\n");
                exit(1);
            }
        }
    }
    fclose (fpb);
    close(results_fd);

    clock_t clockend = clock();
    float seconds = (float)(clockend-clockstart) / CLOCKS_PER_SEC;
    char secondsstring[20];
    sprintf(secondsstring, "%f",seconds);
    write(stats_fd, secondsstring, 20);
    close(stats_fd);

    kill(root_id, SIGUSR2); 
    return 0;
}
