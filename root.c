#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "dataTypes.h"

static int sigcount = 0;
void sighandler(int);

int main(int argc,char *argv[]){
    FILE *fpb;
    int height,numOfRecords, i, results_pipe[2],leaf_stats_pipe[2],sm_stats_pipe[2];
    char *data_file=NULL, *pattern=NULL, *heightstring=NULL, rootid[INT_LENGTH], zstring[INT_LENGTH],\
        startstring[INT_LENGTH],start[]="0",end[INT_LENGTH],sstring[1],res_fd[INT_LENGTH],leaf_stats_fd[INT_LENGTH],sm_stats_fd[INT_LENGTH];
    float z;
    long lSize;
    bool s=false;
    pid_t sm0,sort_id;
    clock_t clockstart = clock();
    //printf("Im ROOT\n");

    /* Handling command line arguments */
    if(argc==7 || argc==8){
        int i;
        for(i=1 ; i<argc ; i++){
            if(!strcmp(argv[i],"-h")){
                height = atoi(argv[i+1]);
                heightstring=argv[i+1];
            }
            if(!strcmp(argv[i],"-d"))
                data_file = argv[i+1];
            if(!strcmp(argv[i],"-p"))
                pattern = argv[i+1];
            if(!strcmp(argv[i],"-s"))
                s = true;
        }
    }
    else{
        printf("Wrong ammount of arguments\n");
        return(1);
    }
    if(height > 5 || height < 1){
        printf("Illegal height value.\n");
        return(1);
    }

    signal(SIGUSR2, sighandler);

    fpb = fopen (data_file,"rb");
    if (fpb==NULL) {
        printf("Cannot open binary file\n");
        return 1;
    }

    /* Find the number of records */
    fseek (fpb , 0 , SEEK_END);
    lSize = ftell (fpb);
    rewind (fpb);
    numOfRecords = (int) lSize/sizeof(Record);
    fclose(fpb);

    /* Intialize pipes */
    if((pipe(results_pipe)==-1) || (pipe(leaf_stats_pipe) == -1) || (pipe(sm_stats_pipe) == -1)){
        perror("Error in pipe call\n");
        exit(1);
    }

    /* In the following block we compute z = k / (sum of n) where n = 1,2,..,2^h-1 and k is numOfRecords.
       This will be used by splitter-mergers of height 1 to compute the interval of
       the records that will be searched by each node.
    */
    if(s){
        int n,den_sum = 0; /* den_sum is the sum in the denominator */
        for(n=1; n <= pow(2,height); n++)
            den_sum += n; 
        z = numOfRecords / (float)den_sum;
        sprintf(zstring,"%f",z);
    }
        

    /* In order to pass integers as arguments to another executable, conversions to strings is required */
    sprintf(end,"%d",numOfRecords);
    sprintf(sstring,"%d",s);
    sprintf(res_fd,"%d",results_pipe[1]);
    sprintf(leaf_stats_fd,"%d",leaf_stats_pipe[1]);
    sprintf(sm_stats_fd,"%d",sm_stats_pipe[1]);
    sprintf(rootid,"%d",getpid());

    switch(sm0 = fork()){
        case -1:
            perror("Failed to fork\n");
            exit(1);
        case 0: //child sm0 node
            execl("./splittermerger","splittermerger",data_file,pattern,start,end,heightstring,sstring,\
                res_fd,leaf_stats_fd,sm_stats_fd,rootid,"0","0", zstring,NULL);
            printf("Error in execl\n");
            exit(1);
        default: //root node
            close(results_pipe[1]);
            close(leaf_stats_pipe[1]);
            close(sm_stats_pipe[1]);
            break;
    }

    FILE *f = fopen("output_records.txt","w");
    if(f==NULL){
        printf("Error in opening file!\n");
        exit(1);
    }

    /* Read results and write them on a file */
    char inbuf[RECORD_SIZE];
    while(read(results_pipe[0], inbuf, RECORD_SIZE)){
        fprintf(f,"%s\n",inbuf);
        //printf("%s\n",inbuf);
    }
    fclose(f);
  
    /* Sort the file by using sort utility */
    switch(sort_id = fork()){
        case -1:
            perror("Failed to fork\n");
            exit(1);
        case 0: //child node
            execl("/usr/bin/sort","sort","./output_records.txt", NULL);
            printf("Error in execl\n");
            exit(1);
        default: //root node
            break;
    }
    waitpid(sort_id,NULL,0);

 
    /* Searchers stats */ 
    float min=INFINITY, max=0, sum=0,seconds;
    while(read(leaf_stats_pipe[0], inbuf, 20)){
        seconds = atof(inbuf);
        if( seconds < min )
            min = seconds;
        if( seconds > max)
            max = seconds;
        sum += seconds;
    }
    printf("Leaf Nodes Statistics:    Min: %f    Max: %f    Avg: %f\n",min,max,sum/pow(2,height));

    /* Splitter-mergers stats */
    min=INFINITY, max=0, sum=0;
    while(read(sm_stats_pipe[0], inbuf, FLOAT_LENGTH)){
        seconds = atof(inbuf);
        if( seconds < min )
            min = seconds;
        if( seconds > max)
            max = seconds;
        sum += seconds;
    }
    printf("Splitter-merger Nodes Statistics:    Min: %f    Max: %f    Avg: %f\n",min,max,sum/pow(2,height));

    clock_t clockend = clock();
    seconds = (float) (clockend - clockstart) / CLOCKS_PER_SEC;
    printf("Turnaround Time: %f\n",seconds);
    printf("Ammount of terminated leaf nodes: %d\n",sigcount);
    //printf("ROOT OUT\n");
    return 0;
}

void sighandler(int signum){
    sigcount++;
}
