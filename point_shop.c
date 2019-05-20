#include <stdio.h>          /* printf()                 */
#include <stdlib.h>         /* exit(), malloc(), free() */
#include <sys/types.h>      /* key_t, sem_t, pid_t      */
#include <sys/shm.h>        /* shmat(), IPC_RMID        */
#include <errno.h>          /* errno, ECHILD            */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>          /* O_CREAT, O_EXEC          */
#include <math.h>
#include <unistd.h>
#include <sys/resource.h>
#include<string.h>
#define SHSIZE 100
char read_str[100];
int pointing_time;

void write_shm(char *str){
   int len=strlen(str);
   int shmid;
    key_t key;
    char *shm;
    char *s;

    key = 9876;
    shmid = shmget(key,SHSIZE, IPC_CREAT | 0666);
    if(shmid<0){
        perror("shmget");
        exit(1);
    }
    shm = shmat(shmid, NULL, 0);
    if(shm == (char *) -1){
        perror("shmat");
        exit(1);
    }
    memcpy(shm, str, len);
    s = shm;
    s += len;
    *s = 0;
}

void read_shm(){
   int shmid;
    key_t key;
    char *shm;
    char *s;

    key = 9876;
    shmid = shmget(key,SHSIZE, 0666);
    if(shmid<0){
        perror("shmget");
        exit(1);
    }

    shm = shmat(shmid, NULL, 0);

    if(shm == (char *) -1){
        perror("shmat");
        exit(1);
    }
    int c=0;
    for(s = shm; *s != 0; s++){
      read_str[c]=*s;
      c++;
   }
}

int priority(char j){
    char ch[1];
        ch[0]=j;
        int x;
        x=strcspn(read_str,ch);
        return x;
}
int p_priority(char ch){
    int priority[13];
    char sorted_colors[7];
    int x;
    for (int i = 0; i < 12; i++)
    {
        int index=0;
        for (int j = 0; j < 12; j++)
        {
            if (read_str[i]==read_str[j])
            {
                index=j;
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
    for (int i = 0; i < 6; i++)
    {
        printf("%c ",sorted_colors[i]);
        if (sorted_colors[i]==ch)
            x=i;
    }
        return x;
}

int is_there_pair(char ch,int x,int n){
    int counter=0;
    for (int i = x+1; i < n; i++)
    {
        if(read_str[i]==ch)
            counter++;
    }
    // printf("%d ",counter);
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

    if (argc != 3){//Check the argumants and if they are not appropriate then exit.
        printf("Unappropriate call, please give me input and output file");
        exit(1);
    }  
    FILE *file = fopen ( argv[1], "r" );
    if ( file != NULL )
    {
        char line [4]; /* or other suitable maximum line size */
        int counter=0;
        while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */
        {
            // fputs ( line, stdout ); /* write the line */
            if(counter==0){
                n=(int) strtol(line, (char **)NULL, 10);
            }
            else{
                boxes[counter-1]=line[0];
            }
            counter++;
        }
        fclose ( file );
    }
    else{
        perror ( argv[1] ); /* why didn't the file open? */
    } 

    /* initialize a shared variable in shared memory */
    shmkey = ftok ("/dev/null", 5);       /* valid directory name and a number */
    shmid = shmget (shmkey, sizeof (int), 0644 | IPC_CREAT);
    if (shmid < 0){                           /* shared memory error check */
        perror ("shmget\n");
        exit (1);
    }
    priority_flag = (int *) shmat (shmid, NULL, 0);   /* attach p to shared memory */
    *priority_flag = 0;
    printf ("priority_flag=%d is allocated in shared memory.\n\n", *priority_flag);
    /* initialize semaphores for shared processes */
    sem = sem_open ("pSem", O_CREAT | O_EXCL, 0644, value); 
    /* name of semaphore is "pSem", semaphore is reached using this name */
    write_shm(boxes);
    printf ("SEMAPHORES INITIALIZED.\n\n");
    /* fork child processes */
    for (i = 0; i < n; i++){
        pid = fork ();
        if (pid < 0) {
        /* check for error      */
            sem_unlink ("pSem");   
            sem_close(sem);  
            /* unlink prevents the semaphore existing forever */
            /* if a crash occurs during the execution         */
            printf ("Fork error.\n");
        }
        else if (pid == 0){
            child_id=i;
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
            break;                  /* child processes */
        }
    }


    /******************************************************/
    /******************   PARENT PROCESS   ****************/
    /******************************************************/
    if (pid != 0){
        /* wait for all children to exit */
        if(i==10){
        int which = PRIO_PROCESS;
        int priority = -20;
        int ret;
        ret = setpriority(which, pid, priority);
        printf("ret :%d",ret);}
        while (pid = waitpid (-1, NULL, 0)){
            if (errno == ECHILD)
                break;
        }

        printf ("\nParent: All children have exited.\n");

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
        for (int i = 0; i < 12; i++)
        {
            int index=0;
            for (int j = 0; j < 12; j++)
            {
                if (read_str[i]==read_str[j])
                {
                    index=j;
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
        for (int t = 0; t < 6; t++)
        {
            if (sorted_colors[t]==read_str[i]){
                // printf("%c %d ",sorted_colors[t],t);
                x=t;
                break;}
        }
        int which = PRIO_PROCESS;
        id_t pid;
        int ret;
        pid = getpid();
        ret = setpriority(which, pid, x*3);
        ret = getpriority(which, pid);
        // sleep(x*2);
        while (1)
        {
            
            if(x<=*priority_flag){
                sem_wait (sem);           /* P operation */  
                // printf("Current priority %d\n",*priority_flag);
                int y=is_there_pair(read_str[i],i,n);
                if(y==0)
                *priority_flag+=1;      
                
                printf("-> Process %d Color %c Prioriy %d y %d\n",i,read_str[i],x,y);
                sleep (pointing_time);
                printf("    <- Process %d\n",i);   
                sem_post (sem);           /* V operation */
                break;
            }
            sleep(1);
        }
        
        exit (0);
    }
}