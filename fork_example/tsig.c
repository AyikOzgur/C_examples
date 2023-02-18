#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define NUM_CHILD 10
#define WITH_SIGNALS 1  //lack of this definition will let compilation without Signals.

pid_t pid[NUM_CHILD];
int child_finished = 0;


#ifdef WITH_SIGNALS
    char recieved_int = 0;

    void sigint_handler(int status)
    {
        printf("\nParent[%d]: Keyboard Interrupt recieved \n", getpid());
        recieved_int++;
    }

    void sigterm_handler(int status)
    {
        printf("Child[%d]: received SIGTERM signal, terminating \n", getpid());
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

    for (int i = 0; i < NUM_CHILD; i++)
    {
        #ifdef WITH_SIGNALS
            if (recieved_int == 1)
            {
                for (int j = 0; j < i; j++)
                {
                    kill(pid[j], SIGTERM);
                }
                break;
            }
        #endif

        pid[i] = fork();

        if (pid[i] == 0)
        {
            // Child Process

            #ifdef WITH_SIGNALS
                signal(SIGINT, SIG_IGN);
                signal(SIGTERM, sigterm_handler);
            #endif

            printf("Child[%d]: created by parent [%d] \n", getpid(), getppid());
            sleep(sleep(10));
            printf("Child[%d]: execution is done \n", getpid());
            exit(0);
        }

        else if (pid[i] < 0)
        {
            // Parent Process when child couldnot created properly
            for (int j = 0; j < i; j++)
            {
                kill(pid[j], SIGTERM);
            }
            printf("Parent[%d]: child process could not be created properly  \n ", getpid());
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
        child_finished++;
    }

    printf("Parent[%d]: There are no more child processes. Exit code is recieved from %d child processes. \n", getpid(), child_finished);

    #ifdef WITH_SIGNALS
        for (int i = 1; i < NSIG; i++)
        {
            signal(i, SIG_DFL);
        }
    #endif

    return 0;
    
}
