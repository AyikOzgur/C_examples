#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define N 5
#define THINKING 0
#define HUNGRY 1
#define EATING 2
#define TRUE 1
#define LEFT ( i + N - 1) % N
#define RIGHT ( i + 1 ) % N
#define WITH_SIGNALS 1  //lack of this definition will let compilation without Signals.

pid_t pid[N];
int shmid;
key_t key;
int sem_group;

struct shared_mem
{
    int state[N];
};

union semun 
{
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
    struct seminfo *__buf;
};

void philosopher(int i);
void take_forks(int i);
void put_forks(int i);
void test(int i);
void eat(int i);
void think(int i);
void down(int i);
void up(int i);
void initialize_shared_memory(void);
void initialize_semaphores(void);


struct shared_mem *shared_memory;
union semun semun_var;

int counter=0;

#ifdef WITH_SIGNALS
    char recieved_int = 0;

    void sigint_handler(int status)
    {
        printf("\nParent[%d]: Keyboard Interrupt recieved \n", getpid());
        kill(0,SIGTERM);
    }

    void sigterm_handler(int status)
    {
        printf("Philosopher[%d]: ate %d times \n", getpid(),counter);
        exit(0);
    }
#endif

int main(void)
{
    #ifdef WITH_SIGNALS
        for (int i = 1; i < NSIG; i++)
        {
            signal(i, SIG_IGN);
        }
        signal(SIGCHLD, SIG_DFL);
        signal(SIGINT, sigint_handler);
    #endif



    initialize_shared_memory();

    for(int i=0; i<N; i++)
    {
        shared_memory->state[i]=THINKING;
    }

    initialize_semaphores();


    for(int i=0; i<N; i++)
    {

        pid[i]=fork();
        if(pid[i]==0)
        {
            // Child Process
            #ifdef WITH_SIGNALS
                signal(SIGINT, SIG_IGN);
                signal(SIGTERM, sigterm_handler);
            #endif
            
            philosopher(i);   
            exit(0);
        }
        else if(pid[i]<0)
        {
            printf("Parent[%d]: Problem occured while creating process",getpid());
            shmdt(shared_memory);
            shmctl(shmid,IPC_RMID,NULL);
            exit(1);
        }
        sleep(1);
    }

    #ifdef WITH_SIGNALS
        if (recieved_int == 0)
        {
            printf("Parent[%d]: creation of all child processes is done. \n", getpid());
        }
        else
        {
            printf("Parent[%d]: Process Creation is interrupted \n", getpid());
        }
    #else
        printf("Parent[%d]: creation of all child processes is done. \n", getpid());   
    #endif

    while (wait(NULL) > 0)
    {
    } 
    shmdt(shared_memory);
    shmctl(shmid,IPC_RMID,NULL);
    return 0;
}

void initialize_shared_memory(void)
{
    //Creating shared memory
    key = ftok("shmfile",65);
    if((shmid = shmget(key, sizeof(struct shared_mem), IPC_CREAT|0666))<0)
    {
        perror("Error shmget\n");
        exit(1);
    }
    else
    {
        printf("Memory attached at shmid %d\n", shmid);
    }

    if((shared_memory = shmat(shmid,NULL, 0)) == ((struct shared_mem *) -1))
    {
        perror("error shmat\n");
        exit(1);
    }
    else
    {
        printf("Shmat succeed\n");
    }



}

void initialize_semaphores(void)
{
    if ((sem_group = semget(IPC_PRIVATE, N+1, 0666 | IPC_CREAT) < 0))
    {
        perror("Semaphore initialaziton errror. \n");
        exit(1);
    }
    else
    {
        printf("Semaphore group id : %d \n",sem_group);
    }

    semun_var.array=malloc((N+1)*sizeof(ushort));

    for(int i=0; i<N;i++)
    {
        semun_var.array[i]=0;
    }
    semun_var.array[N]=1;

    if(semctl(sem_group, 0, SETALL, semun_var) < 0)
    {
        perror("semctl"); exit(1);
    }
}



void philosopher(int i) 
{
    while (TRUE) 
    {
        think(i);
        take_forks(i);
        eat(i);
        put_forks(i);
    }
}
void take_forks(int i) 
{
    down(5); //mutex
    shared_memory->state[i] = HUNGRY;
    test(i);
    up(5);   //mutex
    down(i);
}
void put_forks(int i) 
{
    down(5);  //mutex
    shared_memory->state[i] = THINKING;
    test(LEFT);
    test(RIGHT);
    up(5);    //mutex
}
void test(int i) 
{
    if (shared_memory->state[i] == HUNGRY && shared_memory->state[LEFT] != EATING && shared_memory->state[RIGHT] != EATING) 
    {
        shared_memory->state[i] = EATING;
        up(i);
    }
}
void eat(int i)
{
    printf("philosopher[%d] : I m eating\n",i);
    counter++;
    sleep(2);
}

void think(int i)
{
    sleep(1);
}

void down(int i)
{
    struct sembuf my_sem_b = { i, -1, SEM_UNDO};
    if (semop(sem_group, &my_sem_b, 1) < 0) 
    {
        fprintf(stderr, "semaphore failed\n");
    }
}

void up(int i)
{
    struct sembuf my_sem_b = { i, +1, SEM_UNDO};
    if (semop(sem_group, &my_sem_b, 1) < 0) 
    {
        fprintf(stderr, "semaphore failed\n");
    }
}