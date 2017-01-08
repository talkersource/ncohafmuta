#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/telnet.h>

#include "constants.h"
#include "protos.h"

/*--------------------------------------------------------------*/
/* Define this if you're debugging the IAC telnet options       */
/* otherwise, comment out                                       */
/*--------------------------------------------------------------*/
/* #define IAC_DEBUG */

unsigned char off_seq[4]       = {IAC, WILL, TELOPT_ECHO, '\0'};
unsigned char off_seq_cr[5]    = {IAC, WILL, TELOPT_ECHO, '\n',0};
unsigned char on_seq[4]        = {IAC, WONT, TELOPT_ECHO, '\0'};
unsigned char on_seq_cr[5]     = {IAC, WONT, TELOPT_ECHO, '\n', 0};
/*
unsigned char on_seq2[5]       = {IAC, DO,   TELOPT_ECHO, '\n', 0};
*/
unsigned char eor_seq[4]       = {IAC, WILL, TELOPT_EOR, '\0'};
unsigned char eor_seq2[3]      = {IAC, EOR, '\0'};
unsigned char go_ahead_str[3]  = {IAC, GA, '\0'};
/* unsigned char neg_ttype[]  = {IAC, SB, TELOPT_TTYPE,   IAC, SE, 0}; */
/* unsigned char neg_tuid[]   = {IAC, SB, TELOPT_TUID,    IAC, SE, 0}; */
/* unsigned char neg_tlocl[]  = {IAC, SB, TELOPT_TTYLOC,  IAC, SE, 0}; */

/* Telnet option definitions if the OS doesn't define them */
#if !defined(TELOPT_NEW_ENVIRON)
#define TELOPT_NEW_ENVIRON      39
#define IS                      0
#define SEND                    1
#define INFO                    2
#define VAR                     0
#define VALUE                   1
#define ESC                     2
#define USERVAR                 3
#endif

/*----------------------------------------*/
/* send telnet control iac echo-off       */
/*----------------------------------------*/
void telnet_echo_off(int user)
{
 if (ustr[user].passhid == 1)
	write_raw(user, off_seq, 4);
}

/*----------------------------------------*/
/* send telnet control iac echo-on        */
/*----------------------------------------*/
void telnet_echo_on(int user)
{
 if (ustr[user].passhid == 1) {
	if (ustr[user].promptseq > 0)
	    write_raw(user, on_seq, 4);
	else
	    write_raw(user, on_seq_cr, 5);
  }
}

/*----------------------------------------*/
/* send telnet control iac send user id   */
/*----------------------------------------*/
void telnet_ask_tuid(int user)
{
 /*  write_raw(user, neg_tuid, 6); */
}

/*----------------------------------------*/
/* send telnet control iac will eor       */
/*----------------------------------------*/
void telnet_ask_eor(int user)
{
    write_raw(user, eor_seq, 4);
}

/*----------------------------------------*/
/* send telnet control iac eor            */
/*----------------------------------------*/
void telnet_write_eor(int user)
{
	if (ustr[user].promptseq==1) {
		write_raw(user, eor_seq2, 3);
#if defined(IAC_DEBUG)
		print_to_syslog("writing EOR for prompts\n");
#endif
	}
	else if (ustr[user].promptseq==0) {
		write_raw(user, go_ahead_str, 3);
#if defined(IAC_DEBUG)
		print_to_syslog("writing GA for prompts\n");
#endif
	}
}
    
    
/*------------------------------------------------------------*/
/* this section designed to process telnet commands           */
/* NOTE: to get here, we have alread read an IAC command      */
/*------------------------------------------------------------*/
void do_telnet_commands(int user)
{
 unsigned char inpchar[2];
 char t_buff[132];

 t_buff[0]=0;
 
 if (S_READ(ustr[user].sock, inpchar, 1) != 1)
    return;

 switch(inpchar[0])
   {
    case BREAK:
    case IP:     user_quit(user); break;
 
    case DONT:   proc_dont(user); break;
    case DO:     proc_do(user);   break;
    case WONT:   proc_wont(user); break;
    case WILL:   proc_will(user); break;

    case SB:     break;
    case GA:     break;
    case EL:     break;
    case EC:     break;
 
    case AYT:    write_hilite(user,"\n[Yes]\n");
                 break;
    
    case AO:     break;
    case DM:     break;
    case SE:     break;
    case EOR:    break;
    
    case 123:    break;   /* no idea what this command is */
    case 0:      break;   /* no idea what this command is */
    
                 
    default:
#if defined(IAC_DEBUG)
	sprintf(t_buff,"NOTE: IAC %d not recognized (user:%s)\n",(int)inpchar[0], ustr[user].name);
	print_to_syslog(t_buff);
#endif
       break;
   }
#if defined(IAC_DEBUG)
	sprintf(t_buff,"NOTE: got main IAC %d (user:%s)\n",(int)inpchar[0],ustr[user].name);
	print_to_syslog(t_buff);
#endif
}
    
void proc_dont(int user)
{
 unsigned char inpchar[2];
 char t_buff[132];

 t_buff[0]=0;
    
 if (S_READ(ustr[user].sock, inpchar, 1) != 1)
    return;  
    
 switch(inpchar[0])
   {

    case ECHO:   break;
 
    default:
#if defined(IAC_DEBUG)
	sprintf(t_buff,"NOTE: IAC DONT %d not recognized (user:%s)\n",(int)inpchar[0], ustr[user].name);
	print_to_syslog(t_buff);
#endif
       break;
   }
#if defined(IAC_DEBUG)
        sprintf(t_buff,"NOTE: got IAC DONT %d (user:%s)\n",(int)inpchar[0], ustr[user].name);
	print_to_syslog(t_buff);
#endif
}   
 
void proc_do(int user)
{
 unsigned char inpchar[2];
 char t_buff[132];

 t_buff[0]=0;
    
 if (S_READ(ustr[user].sock, inpchar, 1) != 1)
    return;  
    
 switch(inpchar[0])
   {
    case ECHO:		break;
 
    case TELOPT_TM:	will_time_mark(user);	break;
    case TELOPT_EOR:	ustr[user].promptseq=2;	break;

    default:
#if defined(IAC_DEBUG)
        sprintf(t_buff,"NOTE: IAC DO %d not recognized (user:%s)\n",(int)inpchar[0], ustr[user].name);
	print_to_syslog(t_buff);
#endif
       break;
   }
#if defined(IAC_DEBUG)
        sprintf(t_buff,"NOTE: got IAC DO %d (user:%s)\n",(int)inpchar[0],ustr[user].name);
	print_to_syslog(t_buff);
#endif
}
    
void proc_wont(int user)
{
 unsigned char inpchar[2];
 char t_buff[132];

 t_buff[0]=0;
    
 if (S_READ(ustr[user].sock, inpchar, 1) != 1)
    return;  
    
 switch(inpchar[0])
   {
    case ECHO:   break; 
 
    default:
#if defined(IAC_DEBUG)
       sprintf(t_buff,"NOTE: IAC WONT %d not recognized (user:%s)\n",(int)inpchar[0], ustr[user].name);
       print_to_syslog(t_buff);
#endif
       break;
   }
#if defined(IAC_DEBUG)
        sprintf(t_buff,"NOTE: got IAC WONT %d (user:%s)\n",(int)inpchar[0], ustr[user].name);
	print_to_syslog(t_buff);
#endif
}

void proc_will(int user)
{   
 unsigned char inpchar[2];
 char t_buff[132];

 t_buff[0]=0;
    
 if (S_READ(ustr[user].sock, inpchar, 1) != 1)
    return;  

 switch(inpchar[0])
   {
    case ECHO:   break; 
    
    default:
#if defined(IAC_DEBUG)
        sprintf(t_buff,"NOTE: IAC WILL %d not recognized (user:%s)\n",(int)inpchar[0], ustr[user].name);
	print_to_syslog(t_buff);
#endif
       break;
   }
#if defined(IAC_DEBUG)
        sprintf(t_buff,"NOTE: got IAC WILL %d (user:%s)\n",(int)inpchar[0], ustr[user].name);
	print_to_syslog(t_buff);
#endif
}   

