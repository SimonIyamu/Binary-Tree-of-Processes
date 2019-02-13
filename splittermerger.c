#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include "dataTypes.h"

void findIntervals(int*, int*, int, float);

/* Arguments: ./spilttermerger datafile pattern start end height s_flag results_fd leaf_statistics_fd sm_statistics_fd root_pid a b z */
int main(int argc,char *argv[]){
    if(argc!=14){
        printf("Wrong ammount of arguments. Given %d. Expected 8\n",argc);
        exit(0);
    }

    int start = atoi(argv[3]), end = atoi(argv[4]),m, height = atoi(argv[5]), results_left_pipe[2], \
        results_right_pipe[2],leaf_stats_left_pipe[2],leaf_stats_right_pipe[2],sm_stats_left_pipe[2],sm_stats_right_pipe[2];
    /* Parent Pipes' File Descriptors for results , stats of leafs and stats of splittermergers respectively : */
    int results_fd = atoi(argv[7]), leaf_stats_fd = atoi(argv[8]),sm_stats_fd = atoi(argv[9]); 
    /* This splitter merger has leafs in the interval [a,b] as successors. ss is the middle of that interval. */
    int a = atoi(argv[11]), b = atoi(argv[12]), ss;
    float z = atof(argv[13]);
    bool flag = atoi(argv[6]);
    char startstring[INT_LENGTH], endstring[INT_LENGTH], mstring[INT_LENGTH],left_res_fd[INT_LENGTH],\
        right_res_fd[INT_LENGTH], astring[INT_LENGTH], bstring[INT_LENGTH],ssstring[INT_LENGTH],left_sm_stats_fd[INT_LENGTH],\
        right_sm_stats_fd[INT_LENGTH],left_leaf_stats_fd[INT_LENGTH],right_leaf_stats_fd[INT_LENGTH],heightstring[INT_LENGTH];
    pid_t left_child,right_child;
    clock_t clockstart = clock();
  
    /* m is the position where the given file will be split */ 
    if(!flag)
        m = start + (end - start)/2;
    else{
        if(!a && !b){ /*a==0 and b==0 is given to first splitter merger by the root*/
            a=0;
            b=pow(2,height)-1;
        }
        /* If -s is given, m will be computed at splitter-mergers of height 1 */
        if(height==1){
            findIntervals(&start,&m,a,z);
            findIntervals(&m,&end,b,z);
        }
        ss = a + (b-a)/2;
    }
    
    
    /* results_left_pipe and results_right_pipe are used to send records, from the left and the right child respectively ,to this sm node */
    /* stats pipes do the same but for statistics(cpu time) */
    if((pipe(results_left_pipe)==-1) || (pipe(leaf_stats_left_pipe)==-1 || 
        (pipe(results_right_pipe)== -1) || (pipe(leaf_stats_right_pipe)==-1))){
            perror("Error in pipe call\n");
            exit(1);
    }

    //if(height>1)
    if(pipe(sm_stats_left_pipe)==-1 || (pipe(sm_stats_right_pipe)== -1)){
            perror("Error in pipe call\n");
            exit(1);
    }

    /* In order to pass integers as arguments to another executable, conversions to strings is required */
    sprintf(mstring,"%d",m);
    sprintf(left_res_fd,"%d",results_left_pipe[1]);
    sprintf(right_res_fd,"%d",results_right_pipe[1]);
    sprintf(left_leaf_stats_fd,"%d",leaf_stats_left_pipe[1]);  
    sprintf(right_leaf_stats_fd,"%d",leaf_stats_right_pipe[1]);
    sprintf(left_sm_stats_fd,"%d",sm_stats_left_pipe[1]);
    sprintf(right_sm_stats_fd,"%d",sm_stats_right_pipe[1]);
    sprintf(astring,"%d",a);
    sprintf(bstring,"%d",b);
    sprintf(startstring,"%d",start);
    sprintf(endstring,"%d",end);
    sprintf(ssstring,"%d",ss);
    sprintf(heightstring,"%d",height-1); /* height - 1 will be passed to both child processes */

    switch(left_child = fork()){
        case -1:
            perror("Failed to fork\n");
            exit(1);
        case 0: /* left child node */
            close(results_right_pipe[1]);
            close(leaf_stats_right_pipe[1]);
            close(sm_stats_right_pipe[1]);
            if(height==1){
                execl("./searcher","searcher",argv[1], argv[2], startstring, mstring,\
                    left_res_fd, left_leaf_stats_fd, argv[10],NULL); /* Will search through [0,m) */
            }
            else{
                execl("./splittermerger","splittermerger", argv[1], argv[2], startstring, mstring, heightstring, argv[6],\
                    left_res_fd, left_leaf_stats_fd, left_sm_stats_fd, argv[10], astring, ssstring, argv[13], argv[14], NULL);
            }
            printf("Error in execl\n");
            exit(1);
        default:
            break;
    }

    switch(right_child = fork()){
        case -1:
            perror("Failed to fork\n");
            exit(1);
        case 0: /* right child node */
            close(results_left_pipe[1]);
            close(leaf_stats_left_pipe[1]);
            close(sm_stats_left_pipe[1]);
            if(height==1){
                execl("./searcher","searcher",argv[1],argv[2],mstring,endstring,\
                    right_res_fd, right_leaf_stats_fd, argv[10], NULL); /*Will search through [m,end)*/
            }
            else{
                sprintf(ssstring,"%d",ss+1);
                execl("./splittermerger","splittermerger",argv[1],argv[2],mstring,endstring, heightstring, argv[6],\
                     right_res_fd, right_leaf_stats_fd, right_sm_stats_fd, argv[10], ssstring, bstring, argv[13], argv[14], NULL);
            }
            printf("Error in execl");
            exit(1);
        default:
            break;
    }

    /* Close the input file descriptor of following pipes, because this node only reads from them */
    close(results_left_pipe[1]);
    close(results_right_pipe[1]);
    close(leaf_stats_left_pipe[1]);
    close(leaf_stats_right_pipe[1]);
    close(sm_stats_left_pipe[1]);
    close(sm_stats_right_pipe[1]);

    /* Read the results from the child processes and write them to this node's parent process */
    char inbuf[RECORD_SIZE];
    /*
    while(read(results_left_pipe[0], inbuf, RECORD_SIZE))
        write(results_fd,inbuf,RECORD_SIZE);
    while(read(results_right_pipe[0], inbuf, RECORD_SIZE))
        write(results_fd,inbuf,RECORD_SIZE);
    */
    /* Reading the results alternately is faster: */
    bool fin1=false,fin2=false;
    while(!fin1 || !fin2){ 
        if(!fin1)
            if(read(results_left_pipe[0], inbuf, RECORD_SIZE))
                write(results_fd,inbuf,RECORD_SIZE);
            else
                fin1 = true;
        if(!fin2)
            if(read(results_right_pipe[0], inbuf, RECORD_SIZE))
                write(results_fd,inbuf,RECORD_SIZE);
            else
                fin2 = true;
    }

    /* Do the same for statistics */
    char inbuf2[FLOAT_LENGTH];
    while(read(leaf_stats_left_pipe[0], inbuf2, FLOAT_LENGTH))
        write(leaf_stats_fd,inbuf2,FLOAT_LENGTH);
    while(read(leaf_stats_right_pipe[0], inbuf2, FLOAT_LENGTH))
        write(leaf_stats_fd,inbuf2,FLOAT_LENGTH);

    while(read(sm_stats_left_pipe[0], inbuf2, FLOAT_LENGTH)){
        write(sm_stats_fd,inbuf2,FLOAT_LENGTH);
    }
    while(read(sm_stats_right_pipe[0], inbuf2, FLOAT_LENGTH))
        write(sm_stats_fd,inbuf2,FLOAT_LENGTH);


    clock_t clockend = clock();
    float seconds = (float)(clockend- clockstart) / CLOCKS_PER_SEC;
    char secondsstring[FLOAT_LENGTH];
    sprintf(secondsstring, "%f", seconds);
    write(sm_stats_fd, secondsstring, FLOAT_LENGTH);

    close(results_fd);
    close(leaf_stats_fd);
    close(sm_stats_fd);

    return 0;
}


/*
    If -s is given as argument, this function is called to determine the intervals of leaf node i.

    i will be searching through records in [start,end).

    start = k \frac{\sum_{j=0}^{i}j}{\sum_{n=1}^{2^h}n}  = z * \sum_{j=0}^{i}j
    end = k \frac{\sum_{j=0}^{i+1}j}{\sum_{n=1}^{2^h}n}  = z * \sum_{j=0}^{i+1}j
*/ 
void findIntervals(int *start,int *end, int i, float z){
    int j,num_sum = 0;  /* num_sum is the sum in the numerator */
    for(j=0; j<=i ; j++){
        num_sum += j;
    }

    *start = z * num_sum;
    *end = z * (num_sum + (i+1));
}
