#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#if defined(SOL_SYS) || defined(__linux__)
#include <string.h>        /* for strcpy(),strcmp(),memcpy(), etc */
#else
#include <strings.h>
#endif

#include "constants.h"
#include "protos.h"

extern int down_time;
extern int num_of_users;
extern int atmos_on;
extern int signl;
extern int treboot;
extern char mess[ARR_SIZE+25];

void init_signals()
{

/* Damn zombie kids..get rid of them when they cry for help */
signal(SIGCHLD, zombie_killer); /* does win32 like us? */
              
/*------------------------------------------------*/
/* the following signal ignores may be desireable */
/* to comment out for debugging                   */
/*------------------------------------------------*/
           
signal(SIGTERM,handle_sig);
signal(SIGUSR1,handle_sig); /* does win32 like us? */
signal(SIGSEGV,handle_sig);

signal(SIGILL,handle_sig);
signal(SIGINT,SIG_IGN);
signal(SIGABRT,SIG_IGN);
signal(SIGFPE,SIG_IGN);

#if !defined(WIN32) || defined(__CYGWIN32__)
  /* we don't care about SIGPIPE, we notice it in select() and write() */
  signal(SIGPIPE, SIG_IGN);
  signal(SIGBUS,handle_sig);
#if !defined(__CYGWIN32__)
  signal(SIGIOT,SIG_IGN);
#endif
  signal(SIGTSTP,SIG_IGN);
  signal(SIGCONT,SIG_IGN);
  signal(SIGHUP,SIG_IGN);
  signal(SIGQUIT,SIG_IGN);
#if !defined(__CYGWIN32__)
  signal(SIGURG,SIG_IGN);  
#endif
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTTOU,SIG_IGN);
#if !defined(__linux__)
signal(SIGEMT,SIG_IGN); /* does win32 like us? */
#endif
#endif

}

/*----------------------------------------------------------------------*/
/* Function to clean up the zombied child process from a fork           */
/*                                                                      */
/* Zombie'd child processes occur when the the child finishes before    */
/* the parent process. The kernel still keeps some of the information   */
/* about the child in case the parent might need it. To be able to get  */
/* this info, the parent calls waitpid(). When this happens, the kernel */
/* can discard the information. Zombies dont take up any resources,     */
/* other than a process table entry. If the parent terminates without   */
/* calling waitpid(), the child is adopted by init, which handles the   */
/* work necessary to cleanup after the child.                           */
/*----------------------------------------------------------------------*/
void zombie_killer(int sig)
{
int status, child_val, i=0;
pid_t child_pid=-1;
pid_t get_pid;

#if defined(ZOMBIE_DEBUG)
 write_log(DEBUGLOG,YESTIME,"ZOMBIE: Calling zombie killer..\n");
#endif

/* (void *) casts to avoid warnings on systems that mis-declare */
/* the argument type. */
while ( (get_pid = waitpid(
        -1,             /* Wait for any child */
        (void *) &status,
        WNOHANG         /* Don't block waiting */
       )) > 0) {
           if (get_pid <= 0) break;
           else child_pid = get_pid;
          }

/* No child to clean up, or error */
if (child_pid <= 0) {
        if (child_pid == -1) {
#if defined(ZOMBIE_DEBUG)
        write_log(DEBUGLOG,YESTIME,"ZOMBIE: waitpid() error! %s\n",get_error());
#endif   
        }
        signal(SIGCHLD, zombie_killer);
        return;
        }
else {
#if defined(ZOMBIE_DEBUG)
        write_log(DEBUGLOG,YESTIME,"ZOMBIE: waitpid() returned pid %u\n",(unsigned int)child_pid);
#endif
        }
        
        
/* Reset handler                    */
/* Doing this before the waitpid()  */
/* can lead to an infinite loop     */
signal(SIGCHLD, zombie_killer);

/* Negative child_val indicates some error    */
/* Zero child_val indicates no data available */
         
    /*  
     * We now have the info in 'status' and can manipulate it using
     * the macros in wait.h.
     */
    if (WIFEXITED(status))                /* did child exit normally? */
    {
        child_val = WEXITSTATUS(status); /* get child's exit status */
#if defined(ZOMBIE_DEBUG)
        write_log(DEBUGLOG,YESTIME,"ZOMBIE: Child exited normally with status %d\n", child_val);
#endif

     
        /* Find the user who had this child and let them do remote whos again */
        for (i=0;i<MAX_USERS;++i) {
                if (ustr[i].rwho==child_pid) {
                ustr[i].rwho=1;
                break;   
                } /* end of if */
        } /* end of for */
    }

}       

/*** switching function ***/
void sigcall(int sig)
{
               /*-------------------------*/
               /* process timed events    */
               /*-------------------------*/

              /*--------------------------------------*/
              /* check for out of date board messages */
              /*--------------------------------------*/
              check_mess(0);

              if (down_time > 0)
                {
                 check_shut();
                }

              if (num_of_users)
                {
                 check_idle();
                }

              if (atmos_on)
                {
                 atmospherics();
                }

                tot_user_check(0);
                 
#if !defined(WIN32) || defined(__CYGWIN32__)
reset_alarm();   
#endif
              
signl = 1;
}

/*** reset alarm - first called from add_user ***/
void reset_alarm()
{
signal(SIGALRM, sigcall);
alarm( MAX_ATIME );
}

/**** START OF SIGNAL FUNCTIONS ****/
void handle_sig(int sig)
{
                 
switch(sig) {
        case SIGTERM:
                shutdown_error(log_error(10));
        case SIGSEGV:
                if (REBOOT_A_CRASH==1)
                 treboot=1;
                shutdown_error(log_error(11));
        case SIGBUS: 
                if (REBOOT_A_CRASH==1)
                 treboot=1;
                shutdown_error(log_error(12));
        case SIGUSR1:
                shutdown_error(log_error(13));
        case SIGILL:
                if (REBOOT_A_CRASH==1)
                 treboot=1;
                shutdown_error(log_error(15));
  }
}

