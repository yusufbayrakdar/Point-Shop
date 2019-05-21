#include <stdio.h>          /* printf()                 */
#include <stdlib.h>         /* exit(), malloc(), free() */
#include <sys/types.h>      /* key_t, sem_t, pid_t      */
#include <sys/shm.h>        /* shmat(), IPC_RMID        */
#include <errno.h>          /* errno, ECHILD            */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>          /* O_CREAT, O_EXEC          */
#include <unistd.h>         /* usleep(), fork()         */
#include <sys/resource.h>   /* shmget()                 */
#include<string.h>          /* strlen(), memcpy()       */
#define SHSIZE 100          /* Shared memory size       */
typedef int bool;           /* define boolean           */
#define true 1              /* it can be true or false  */
#define false 0             /* it can be true or false  */
char *read_str = NULL;      /* this array is for input.txt so it store input strings and I stored it in shared memory */
int pointing_time;          /* this variable decide box pointing process time */

void write_shm(char *str){/* this function writes given string into shared memory */
    int len=strlen(str); /* get length of the given string */
    int shmid;           /* shared memory id */
    char *shm;          /* shared memory variable */
    key_t key;          /* key of shared memory */
    char *s;            /* place to reach shm */
    key = 9876;         /* processes write into shared memory and read from shared memory must have same key value */
    shmid = shmget(key,SHSIZE, IPC_CREAT | 0666);   /* get shared memory id by creating it*/
    if(shmid<0){                /* if shared memory id is negative then error and exit */
        perror("shmget");
        exit(1);
    }
    shm = shmat(shmid, NULL, 0);    /* attach the shm variable into shared memory */
    if(shm == (char *) -1){         /* if got an error then exit */
        perror("shmat");
        exit(1);
    }
    memcpy(shm, str, len);          /* if no error then copy the given string into the attached shared memory variable */
    s = shm;                        
    s += len;                       /* chage place of s variable accordingly length of the given string */
    *s = 0;
}

void read_shm(){
    int shmid;           /* shared memory id */
    char *shm;          /* shared memory variable */
    key_t key;          /* key of shared memory */
    char *s;            /* place to reach shm */

    key = 9876;             /* processes write into shared memory and read from shared memory must have same key value */
    shmid = shmget(key,SHSIZE, 0666);   /* connect an existing shared memory */
    if(shmid<0){                        /* if got an error exit */
        perror("shmget");
        exit(1);
    }

    shm = shmat(shmid, NULL, 0);        /* attach shm variable to shared memory */

    if(shm == (char *) -1){             /* if got an error then exit */
        perror("shmat");
        exit(1);
    }
    int c=0;
    for(s = shm; *s != 0; s++){         /* store read chars into read_str until got 0 */
      read_str[c]=*s;
      c++;
   }
}


int is_there_pair(char ch,int x,int n){ /* this function takes an char, its index and total number of chars and look for is there any same char with given char if not return 0 if there is/are then return its count */ 
    int counter=0;
    for (int i = x+1; i < n; i++)
    {
        if(read_str[i]==ch)
            counter++;
    }
    return counter;
    
}


int main (int argc, char **argv){
    int i;                          /*      loop variables          */
    key_t shmkey;                   /*      shared memory key       */
    int shmid;                      /*      shared memory id        */
    sem_t *sem;                     /*      synch semaphore         *//*shared */
    pid_t pid;                      /*      fork pid                */
    unsigned int n;                 /*      fork count              */
    unsigned int value=2;           /*      semaphore value         */
    char boxes[12];                 /*      box color list          */
    int child_id;                   /*      child process id        */
    int *priority_flag;                       /*      shared variable         *//*shared */
    read_str = malloc(sizeof(char));

    if (argc != 3){                 /*Check the argumants and if they are not appropriate then exit. */
        printf("Unappropriate call, please give me input and output file\n");
        exit(1);
    }  
    FILE *file = fopen ( argv[1], "r" );    /* open input.txt as its given name */
    if ( file != NULL )                     /* if no error then start to read it */
    {
        char line [4];                      /* or other suitable maximum line size */
        int counter=0;
        while ( fgets ( line, sizeof line, file ) != NULL )     /* read a line */
        {
            if(counter==0){                                     /* if it is first line then take tha value as box count */
                n=(int) strtol(line, (char **)NULL, 10);
            }
            else{                                               /* else then store char value in boxes array */
                boxes[counter-1]=line[0];
            }
            counter++;
        }
        fclose ( file );
    }
    else{                               /* if error then give error */
        perror ( argv[1] ); /* why didn't the file open? */
    } 
    write_shm(boxes);                   /* write the given boxes into shared memory */
    

    /* initialize a shared variable in shared memory */
    shmkey = ftok ("/dev/null", 5);       /* valid directory name and a number */
    shmid = shmget (shmkey, sizeof (int), 0644 | IPC_CREAT);    /* create shared memory to syncronization */
    if (shmid < 0){                           /* shared memory error check */
        perror ("shmget\n");
        exit (1);
    }
    priority_flag = (int *) shmat (shmid, NULL, 0);   /* attach priority_flag to shared memory */
    *priority_flag = 0;         /*priority_flag is allocated in shared memory with value 0 */
    *(priority_flag+1) = 0;     /* assign order counter initially 0 */
    *(priority_flag+2) = 0;     /* assign intruder counter initially 0 */
    /* initialize semaphores for shared processes */
    sem = sem_open ("pSem", O_CREAT | O_EXCL, 0644, value); 
    /* name of semaphore is "pSem", semaphore is reached using this name */
    printf ("SEMAPHORES INITIALIZED.\n");
    /* fork child processes */
    for (i = 0; i < n; i++){            /* create a chile until reach given box count */
        pid = fork ();
        if (pid < 0) {
        /* check for error      */
            sem_unlink ("pSem");   
            sem_close(sem);  
            /* unlink prevents the semaphore existing forever */
            /* if a crash occurs during the execution         */
            printf ("Fork error.\n");
        }
        else if (pid == 0){                 /* if process is a child then get process char from the read_str */
            read_shm();                     
            switch (read_str[i])
                {
                case 'R':
                    pointing_time=1;
                    break;
                case 'G':
                    pointing_time=2;
                    break;
                case 'B':
                    pointing_time=2;
                    break;
                case 'Y':
                    pointing_time=1;
                    break;
                case 'P':
                    pointing_time=1;
                    break;
                case 'O':
                    pointing_time=2;
                    break;
                default:
                    pointing_time=0;
                    break;
                }
            break;                  
        }
    }


    /******************************************************/
    /******************   PARENT PROCESS   ****************/
    /******************************************************/
    if (pid != 0){
        if(i==0){ /* give child processes higher priority */
            int which = PRIO_PROCESS;
            int priority = -20;
            setpriority(which, pid, priority);
        }
        /* wait for all children to exit */
        int wait_counter=0;
        while(wait_counter<n){
            wait(NULL);
            wait_counter+=1;
        }

        printf ("Parent: All children have exited.\n");

        /* shared memory detach */
        shmdt(priority_flag);
        shmctl (shmid, IPC_RMID, 0);

        /* cleanup semaphores */
        sem_unlink ("pSem");   
        sem_close(sem);  
        /* unlink prevents the semaphore existing forever */
        /* if a crash occurs during the execution         */
        exit (0);
    }

    /******************************************************/
    /******************   CHILD PROCESS   *****************/
    /******************************************************/
    else{
        int x;
        int priority[13];
        char sorted_colors[7];
        for (int i = 0; i < 12; i++)            /* this loop section gives a sorted list with no repetition for example RRYR list -> RY */
        {
            for (int j = 0; j < 12; j++)
            {
                if (read_str[i]==read_str[j])
                {
                    for (int k = 0; k < 12; k++)
                    {
                        if(sorted_colors[k]=='\0'||sorted_colors[k]==read_str[j]){
                            sorted_colors[k]=read_str[j];
                            break;
                        }
                    }
                    
                    break;
                }
            }
        }
        for (int t = 0; t < 6; t++)         /* this loop search char of child process in sorted colors and assign its index to x variable which is priority of this child */
        {
            if (sorted_colors[t]==read_str[i]){
                x=t;
                break;}
        }
        int order=0;
        for (int j = 0; j < i; j++)         /* this for loop gives child process its char order with in the same char for example given char is R and read chars is RYGRYYR and child pid (i) is 3 then child process knows that its second R */
        {
            if (read_str[j]==read_str[i])
            {   
                order+=1;
            }
        }
        FILE *first_out = fopen(argv[2], "w");      /* open output file with its given name with write operation */
        FILE *out = fopen(argv[2], "a");            /* open output file with its given name with append operation */  
        if (out == NULL)                            /* error check */
        {
            printf("Error opening file!\n");
            exit(1);
        }if (first_out == NULL)                     /* error check */
        {
            printf("Error opening file!\n");
            exit(1);
        }
        bool intruder=true;                         /* this boolean variable is to unable child process to enter into critical section until critical section is completely empty for example first two process enters the critical section and one of them leaves the critical section but the other one is still in the critical section then no other child process can enter the critical section */
        while (1)                                   /* run until all child process leaves critical section */
        {
            if(*(priority_flag+2)==0)               /* *(priority_flag+2) value counts process which is in the critical section, so if this count hits the 0 then turn the intruder variable to true for while *(priority_flag+2) variable is 1 it can be possible any other process can enter the critical section */
                intruder=true;
            if(*(priority_flag+2)==2)               /* when *(priority_flag+2) value hits 2 then turn intruder value to false then while *(priority_flag+2) value is 1 no other process can enter the critical section */
                intruder=false;            
            /* this control is the heart of this code, so firstly let me explain  variables I used in here */  
            /* *priority_flag is an integer value which is attached in to shared memory and I used this variable as priority counter.For example it is initially 0 and until all process with 0 priority is done this variable remains 0 and if all 0 priority processes is done then last process with 0 priority increases this variable, so 1 priority processes can enter the critical section. */
            /* *(priority_flag+1) variable is order counter and it checks the order of the process while processes enter the critical section.By the way let me explain order variable even if process number is 4 order can be 1 in that situation BoxList={RYGBR} process index is 4 but its second R so order is 1 (if it is first R then order is 0) */
            /* *(priority_flag+2) is a counter which counts the processes in the critical section */
            /* So this if firstly checks priority if it fits then checks order then it fits lastly checks intruder boolean so that once critical section is totaly full, no process can enter into critical section until it is fully empty */
            if(x==*priority_flag&&*(priority_flag+1)==order&&intruder){     
                sem_wait (sem);             /* P operation */  
                *(priority_flag+2)+=1;      /* increase critical section process counter */
                printf("%d %c ->\n",i+100,read_str[i]);     /* show process entrance in terminal */
                if (i==0)                                   /* if first process then print output file changeover number */
                fprintf(first_out,"%d\n",(int)strlen(sorted_colors));    
                usleep (pointing_time*1000);                /* wait until first order process to write into outputfile because system call takes a few miliseconds */
                fprintf(out,"%d %c\n",i+100,read_str[i]);   /* print process id and box color output file */
                *(priority_flag+1)+=1;                      /* after writing process call second order process, it means even if it is two-capacity-semaphore second order process cannot enter in critical section unless first order process calls the second one */
                int which_char=is_there_pair(read_str[i],i,n);  /* check the process is the last char with the same priority if it is assign which_char variable to 0 */
                usleep (pointing_time*999000);              /* pointing time */
                if(which_char==0){                          
                    *priority_flag+=1;                      /* if it is the last char with same priority then increase priority_flag value */
                    *(priority_flag+1)=0;                   /* if it is the last char with same priority then assign *(priority_flag+1) value to 0 */
                }
                printf("    <- %d %c\n",i+100,read_str[i]); /* show process leaves critical section in terminal */
                *(priority_flag+2)-=1;      /* decrease critical section process counter */
                sem_post (sem);             /* V operation */
                break;
            }
            usleep(10000);
        }
        
        exit (0);
    }
}
