/* Copyright 1990 by John Ockerbloom.
   Derivative for talker by Cygnus (cygnus@ncohafmuta.com), 1996

  Permission to copy this software, to redistribute it, and to use it
  for any purpose is granted, subject to the following restrictions and
  understandings.

  1. Any copy made of this software must include this copyright notice
  in full.

  2. Users of this software agree to make their best efforts (a) to
  return to the above-mentioned authors any improvements or extensions
  that they make, so that these may be included in future releases; and
  (b) to inform the authors of noteworthy uses of this software.

  3. All materials developed as a consequence of the use of this
  software shall duly acknowledge such use, in accordance with the usual
  standards of acknowledging credit in academic research.

  4. The authors have made no warrantee or representation that the
  operation of this software will be error-free, and the authors are
  under no obligation to provide any services, by way of maintenance,
  update, or otherwise.

  5. In conjunction with products arising from the use of this material,
  there shall be no use of the name of the authors, of Carnegie-Mellon
  University, nor of any adaptation thereof in any advertising,
  promotional, or sales literature without prior written consent from
  the authors and Carnegie-Mellon University in each case.

*/

/* Last changed: Mar 15th 1999 */
/* Version 1.2 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <varargs.h>
#include <time.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

/*** CONFIGURATION SECTION ***/

/* REPLACE THIS with the host the robot will be connecting to */
char *host = "ncohafmuta.com";

/* REPLACE THIS with the talker's main login port */
int   port = 5000;

/* BOT_NAME and ROOT_ID must be all lowercase */
/* REPLACE THIS with the bot's name */
#define BOT_NAME      "spokes"

/* REPLACE THIS with the bot's login sequence. The default here is */
/* bot_username <carriage-return> bot_password                     */
#define CONNMSG       "\n spokes \n maple2\0"

/* REPLACE THIS with the username of the bot's owner */
#define ROOT_ID       "cygnus"

/* REPLACE THIS with the command the bot will use to disconnect */
#define DCONMSG       ".q\0"

/* REPLACE THESE with .desc messages the bot will change to */
#define IDLEMSG       "sits silently on his dais."
#define TELLMSG       "tells a tale to someone nearby."
#define LISTENMSG     "hears a story by"
#define SHUTMSG       "is asleep. Come back later."

/* REPLACE THIS with the name of the talker's main room */
#define MAIN_ROOM     "front_gate"

/* REPLACE THIS with the bot room set in the talker's constants.h */
#define HOME_ROOM     "pond"

/* REPALCE THIS with the directions the bot will give to get to him */
/* Last %s is the bot's home room */
#define UNKNOWNDIRS "by typing \".go meadow\", then \".go %s\" "

/* REPLACE THIS with the file you want the bot to log his recorded */
/* conversations to */
#define LOG_FILE      "log.log"

/* REPLACE THIS with the file you want the bot to log his normal debug */
/* stuff to */
#define BOTLOG_FILE      "botlog"

/* Should be set same as in the talker's constants.h */
#define NAME_LEN	21
#define MAX_USERS	40

/* the directory the bot's sotries will go in. dont change normally */
#define STORYDIR      "Stories"

/* dont change */
#define DIRECTORY     "."

/*-----------------------------------------------------------------*/
/* The number of characters minus the length of the players's name */
/* are before where the input string starts                        */
/*                                                                 */
/* i.e. if a comm format in constants.h is "%s tells you: %s"      */
/*      then the offset is 12, because there is 12 characters      */
/*      minus the length of the player before the actual message   */
/*-----------------------------------------------------------------*/
#define SAY_OFFSET	7
#define TELL_OFFSET	13
#define SHOUT_OFFSET	9
#define SEMOTE_OFFSET	5
#define SHEMOTE_OFFSET	3
#define EMOTE_OFFSET	1

/*** END OF CONFIGURATION SECTION ***/

/*** FUTURE USE ***/
/*---------------------------------------------------------*/
/* Current communications formats                          */
/* Set these to the same communications syntax you have    */
/* in your talkers constants.h                             */
/*---------------------------------------------------------*/
/* NORMAL TALK */
#define SAY_FORM     "[%s]: %50c"

/* PRIVATE TELLS */
#define TELL_FORM    "%s tells you: \"%s\""

/* SEMOTES */
#define SEMOTE_FORM  "--> %s %s"

/* SHOUTS */
#define SHOUT_FORM   "%s shouts \"%s\""

/* SHEMOTES */
#define SHEMOTE_FORM "& %s %s"

/*** END OF FUTURE USE ***/

/* Some maximum quantities */

#define MAXSTRLEN  1024
#define MAXKIDS    25
#define MAXLEVELS  512
#define MAXLINES   255
#define FORMATMAX  64
#define TOKSIZ     1024
#define MSGSIZ     512
#define BIGBUF     4096

/* The storystate structure gives us info about where we are in the story */

struct storystate {
  char title[MAXSTRLEN];        /* Title of the story                   */
  char author[MAXSTRLEN];       /* Author of current paragraph          */
  char paragraphid[MAXLEVELS];  /* ID of current paragraph              */
  int   writing;                /* Nonzero if we're in writing mode     */
  int   editing;                /* Nonzero if we're editing this para.  */
  int   numkids;                /* Number of children of this para.     */
  int   numlines;               /* Number of lines in this para.        */
  char *kids[MAXKIDS];          /* Names of authors of various children */
  char *paragraph[MAXLINES];    /* Text of current paragraph            */
                                /* Maybe more will come later!          */
};

struct {
	char name[NAME_LEN];
	char say_name[NAME_LEN];
	char room[NAME_LEN];
	int vis;
	int logon;
} ustr[MAX_USERS];
 
/* Volume of request to StoryBot */

#define WHISPER 0
#define SAY     1
#define EMOTE   2
#define SHOUT   3
#define SHEMOTE 4
#define SEMOTE  5

/* Misc constants */

#define YES 1
#define NO 0

/* For matching strings from users that are not part of story handling */
/* Results from star matcher */
char res1[BUFSIZ], res2[BUFSIZ], res3[BUFSIZ], res4[BUFSIZ];
char res5[BUFSIZ], res6[BUFSIZ], res7[BUFSIZ], res8[BUFSIZ];
char *result[] = { res1, res2, res3, res4, res5, res6, res7, res8 };
# define MATCH(D,P)     smatch ((D),(P),result)

void load_user();
void add_user();
void read_next(char *author, struct storystate *story);
void read_previous(struct storystate *story);
void read_paragraph(struct storystate *story);
void get_paragraph(struct storystate *story);
void read_titles(char *bitmatch, int mode);
void give_help(char *playername, char *lowername, struct storystate *story);
void wlog(char *str);
void robot();
void quit_robot();
void sync_bot();
void reboot_robot();
void clear_all_users();
void clear_user(int user);
void listen_to_people();
void remove_first(char *inpstr);
void strtolower(char *str);
void midcpy(char *strf, char *strt, int fr, int to);
void crash_n_burn(char *string);
void handle_page(char *pageline);
int handle_action(char *playername, char *lowername, char *command, int volume);
int smatch (register char *dat, register char *pat, register char **res);
int find_free_slot();
int connect_robot();
int charsavail();
int get_user_num(char *name);
int handle_page2(char *pageline, char *playername, char *lowername);
int setup_directory(struct storystate *story);
int save_para(struct storystate *story, char *playername, char *lowername);
char *getline();
char *getinput();
char *resolve_title();
struct storystate *start_story(), *finish_story(), 
                  *handle_command(), *handle_writing(), *abort_writing();

/* A couple of global vars (Bad code style! Bad code style!) */

int FORMATTED;           /* If YES, paras are formatted to 64 lines */
int bs;                  /* bot's connection socket */
int logging=0;		 /* logging flag */
char directions[256];
char mess[9000];
FILE *log_fp;		 /* file pointer for log file */


/* The main program spawns the child process, sets up the socket */
/* to the talker and then calls robot().                         */
int main()
{
  int fd;

   /*-------------------------------------------------*/
   /* Redirect stdin, stdout, and stderr to /dev/null */
   /*-------------------------------------------------*/
    fd = open("/dev/null",O_RDWR);
    if (fd < 0) {
      perror("Unable to open /dev/null");
      exit (-1); 
      }
        
    close(0); 
    close(1); 
    close(2);
   
    if (fd != 0)
    {
     dup2(fd,0);    
     close(fd);  
    }  
        
    dup2(0,1);
    dup2(0,2);

  switch(fork())
     {
        case -1:    wlog("FORK 1 FAILED");
                    exit(1);

#if defined(__OpenBSD__)
        case 0:     setpgrp(0,0);
#else
        case 0:     setpgrp();
#endif
                    break;
   
        default:    sleep(1); 
                    exit(0);
      }

  switch(fork())
     {
        case -1:    wlog("FORK 2 FAILED");
                    exit(2);
    
        case 0:     break;  /* child becomes server */
   
        default:    exit(0);
      }

    clear_all_users();

    wlog("Bot starting...");

    bs = connect_robot();
    if (bs < 0) {
      sprintf(mess,"Connect failed to %s %d, exiting!",host,port);
      wlog(mess);
      exit(-1);
     }

    signal(SIGTERM, quit_robot);
    sprintf(mess,"Bot started with PID %d", getpid());
    wlog(mess);
    robot();
    sprintf(mess, "Bot broke out of main robot loop, exiting!");
    wlog(mess);
    quit_robot();

return 0;
}

/* This connect_robot function sets up a socket for the bot to the talker */
int connect_robot()
{
 struct sockaddr_in sin;
 struct hostent *hp;
 int cs;

 bzero((char *) &sin, sizeof(sin));
 sin.sin_port = htons(port);
 
 /* Handle numeric or host name addresses */
  if (isdigit (*host))
  { sin.sin_addr.s_addr = inet_addr (host);
    sin.sin_family = AF_INET;
  }
  else
  { if ((hp = gethostbyname(host)) == 0) return (-1);

    bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
  }
  
  cs = socket(AF_INET, SOCK_STREAM, 0);
  if (cs < 0) return -1;
   
  if (connect(cs,(struct sockaddr *) &sin, sizeof(sin)) < 0) return -1;
  
  return cs;
}

/* We are exiting now */
void quit_robot()
{
 time_t tm;

 shutdown(bs, 2);
 close(bs);
 
 /* Close log file if open */
 fclose(log_fp);

 time(&tm);
 sprintf(mess,"QUIT: %s is quitting at %s",BOT_NAME,ctime(&tm));
 mess[strlen(mess)-1]=0;
 wlog(mess);
 exit(0);
}

/* We are rebooting now */
void reboot_robot()
{
 int fd;
 time_t tm;
 char *args[]={ "./restart",NULL };

 shutdown(bs, 2);
 close(bs);
 
 time(&tm);
 sprintf(mess,"REBOOT: %s is rebooting at %s",BOT_NAME,ctime(&tm));
 wlog(mess);

 /* Sleep for some time to make sure talker is up so we can login */
 sleep(10);
        /* If someone has changed the binary or the config filename while
           this prog has been running this won't work */
        fd = open( "/dev/null", O_RDONLY );
        if (fd != 0) {
            dup2( fd, 0 );
            close(fd);
        }
        fd = open( "/dev/null", O_WRONLY );
        if (fd != 1) {
            dup2( fd, 1 );
            close(fd);
        }
        fd = open( "/dev/tty", O_WRONLY ); 
        if (fd != 2) {
            dup2( fd, 2 );
            close(fd);
        }
        execvp("./restart",args);
        /* If we get this far it hasn't worked */
        close(0);
        close(1);
        close(2);
 time(&tm);
 sprintf(mess,"REBOOT: Reboot failed at %s",ctime(&tm));
 wlog(mess);

}

/* Wait for input on the socket */
char *getinput()
{
 long len;
 static char buf[BUFSIZ], rbuf[4];
 register char *s=buf, *tail=buf+MSGSIZ+1;

 /* No input waiting */
  if (!charsavail (bs)) return (NULL);
 
  /* Read one line, save printing chars only */
  while ((len = read (bs, rbuf, 1)) > 0)
  { if (*rbuf == '\n')                  break;
    if (isprint (*rbuf) && s < tail)    *s++ = *rbuf;
  }
  *s = '\0';

  /* Check for error */ 
  if (len < 0)
  { 
    sprintf(mess, "Error %ld reading from talker", len);
    wlog(mess);
    quit_robot();
  }

  return (s = buf);
}

/* Check for input available from socket */
int charsavail(int fd)
{
 long n;
 long retc;

   if ((retc = ioctl (fd, FIONREAD, &n)))
     {
      sprintf(mess, "Ioctl returns %ld, n=%ld.", retc, n);
      wlog(mess);
      quit_robot();
     }
  
  return ((int) n);

}

/* Send string down the socket */
void sendchat (fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char *fmt;
int a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
  int len;
  char buf[BIGBUF];
 
  if (!fmt) {
     wlog("Null fmt in sendchat");
     quit_robot();
    }

  sprintf (buf, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
  /* strcat (buf, "\n"); */
  len = strlen (buf);

  if (write(bs, buf, len) != len)
    {
     sprintf(mess,"Write failed: %s",buf);
     wlog(mess);
     quit_robot();
    }

}

void wlog(char *str)
{
 char timebuf[30];
 char filename[256];
 char mess2[9030];
 time_t tm;
 FILE *fp;

 strcpy(filename,BOTLOG_FILE);
 fp=fopen(filename,"a");

 time(&tm);
 strcpy(timebuf, ctime(&tm));
 timebuf[strlen(timebuf)-6]=0;
 sprintf(mess2,"%s: %s\n",timebuf,str);
 fputs(mess2,fp);
 fclose(fp);
}


/* The robot routine takes care of the initial setup required for startup */
void robot()
{

  /* Set bots directions */
  sprintf(directions,UNKNOWNDIRS,HOME_ROOM);

  sendchat("%s\n\n", CONNMSG);
  sync_bot();
  sendchat(";has just rebooted.\n");
  sendchat(".desc %s\n", IDLEMSG);
  sendchat(".set home %s\n", HOME_ROOM);
  sendchat(".home\n");
  sendchat(".shout I'm baaaaack!\n");
  sendchat("_who\n");
  listen_to_people();
  sendchat(".go\n");
  sendchat("%s\n", DCONMSG);           /* should never reach here */
  return;
}

/* The listen_to_people routine is the main loop of the program.
   It keeps track of the goings-on in the room, and processes 
   the various requests from people.  

   StoryBot can understand both say and whisper, though most
   commands need to be said out loud, and StoryBot will sometimes
   say or whisper different messages depending on their type.           */

void listen_to_people()
{
   char *inbuf;                    /* Raw buffer of what came in */
   char *playername;               /* Name of player associated w/ last cmd */
   char *lowername;                /* Name of player associated w/ last cmd */
   char *command;                  /* Points to command player gave         */
   char command2[100];             /* Points to command player gave         */
   int volume;                     /* Was cmd a SAY or a WHISPER?           */
   int chance2;
   struct storystate *ourstory;    /* Ptr to main story structure           */
   char c;
   int u,vis;
   char room[NAME_LEN];

   ourstory = NULL;
   FORMATTED = NO;
   playername = (char *) malloc(MAXSTRLEN * sizeof(char));
   lowername = (char *) malloc(MAXSTRLEN * sizeof(char));
   
   for (;;) {
        /* Get input from talker */
        inbuf = getinput();

        if (inbuf == NULL) {
        /* wlog("Couldn't read any input from getinput"); */
        /* quit_robot(); */
        sleep(1);
        continue;
        }

	if (!strncmp(inbuf,"+++++ command:",14)) {
	  remove_first(inbuf);
	  remove_first(inbuf);
	  sscanf(inbuf,"%s ",command2);
	  command2[strlen(command2)-1]=0;
	  if (!strcmp(command2,"who")) {
		remove_first(inbuf);
		u=find_free_slot();
		/* get name */
		sscanf(inbuf,"%s ",ustr[u].say_name);
		strcpy(ustr[u].name,ustr[u].say_name);
		strtolower(ustr[u].name);
		/* get room */
		remove_first(inbuf);
		sscanf(inbuf,"%s ",ustr[u].room);
		/* get vis status */
		remove_first(inbuf);
		sscanf(inbuf,"%d",&ustr[u].vis);
        sprintf(mess, "Read in user %s (%s) (%d)", ustr[u].say_name,ustr[u].room,ustr[u].vis);
        wlog(mess);
	    } /* end of _who command line read */
	}
     else if ( (sscanf(inbuf, "+++++ came in:%s", playername)) == 1) {
        sprintf(mess, "%s recorded as arrived.", playername);
        wlog(mess);
        strcpy(lowername,playername);
        strtolower(lowername);
        if (ourstory && ourstory->numkids > 0)
        sendchat(".tell %s Hello, %s.\n.t %s I'm in the middle of telling a story.\n",
               playername, playername, playername);
        else if (ourstory && ourstory->writing)
        sendchat(".tell %s Hello, %s.\n.t %s I'm busy listening to %s's story.\n",
               playername, playername, playername, ourstory->author);
        else
        sendchat(".tell %s Hello, %s.\n.t %s I'm glad to have some company.\n",
               playername, playername, playername);

        if ((ourstory == NULL) || !(ourstory->writing))
          sendchat(".tell %s Say 'help' to see commands I recognize.\n",playername);
     }
     else if ( (sscanf(inbuf, "+++++ logon:%s %s %d", playername,room,&vis)) == 3) {
	/*
	if (!load_user(playername))
	  add_user(playername);
	*/
        strcpy(lowername,playername);
        strtolower(lowername);

	u=find_free_slot();

	strcpy(ustr[u].name,lowername);
	strcpy(ustr[u].say_name,playername);
	strcpy(ustr[u].room,room);
	ustr[u].vis=vis;
	ustr[u].logon=1;

	vis=0;
	room[0]=0;
        sprintf(mess, "%s recorded as logged on.", playername);
        wlog(mess);
        chance2 = rand() % 3;
        if (chance2==1) sendchat(".tell %s Hiya %s!\n",playername,playername);
        else if (chance2==2) sendchat(".tell %s Hola %s!\n",playername,playername);
        else sendchat(".tell %s Howdy %s!\n",playername,playername);
     }
     else if ( (sscanf(inbuf, "+++++ logoff:%s", playername)) == 1) {
	u=get_user_num(playername);

       if (ourstory && ourstory->writing &&
           !strcmp(ustr[u].name, ourstory->author)) {
         sendchat(";has finished listening to %s's story.\n",ourstory->author); 
         ourstory = finish_story(ourstory);
         sendchat(".desc %s\n", IDLEMSG);
       }
        sprintf(mess, "%s recorded as logged off.", ustr[u].say_name);
        wlog(mess);

	/* Clear structure */
	clear_user(u);
     }
     else if ( (sscanf(inbuf, "+++++ left:%s", playername)) == 1) {
	u=get_user_num(playername);

       sprintf(mess, "%s recorded as left.", ustr[u].say_name);
       wlog(mess);
       if (ourstory && ourstory->writing &&
           !strcmp(ustr[u].name, ourstory->author)) {
         sendchat(";has finished listening to %s's story.\n",ourstory->author); 
         ourstory = finish_story(ourstory);
         sendchat(".desc %s\n", IDLEMSG);
       }
     } /* end of else if */
     else if (!strcmp(inbuf, "+++++ SHUTDOWN")) {
        sendchat(".desc %s\n", SHUTMSG); /* shut down bot! */
        sendchat("%s\n", DCONMSG);
        wlog("Got talker shutdown message.");
        quit_robot();
     } 
     else if (!strcmp(inbuf, "+++++ REBOOT")) {
        sendchat(".desc %s\n", SHUTMSG); /* reboot bot! */
        sendchat("%s\n", DCONMSG);
        wlog("Got talker reboot message.");
        reboot_robot();
     }
     else if (!strcmp(inbuf, "+++++ QUIT")) {
        wlog("Got talker quit message.");
        quit_robot();
     }
     else if (sscanf(inbuf, "%s says %c", playername, &c) == 2) {
	u=get_user_num(playername);
        strcpy(lowername,playername);
        strtolower(lowername);

         command = inbuf+strlen(playername)+SAY_OFFSET;
	 if (logging==1) {
	  if (strcmp(ustr[u].name,BOT_NAME)) {
		fputs(inbuf,log_fp);
		fputs("\n",log_fp);
		}
	  }
         if (command[strlen(command)-1]=='\"') command[strlen(command)-1]='\0';

         sprintf(mess,"SAY: \"%s\" \"%s\"",playername,command);
         wlog(mess);

         volume = SAY;
     if (ourstory && ourstory->writing) {
       ourstory = handle_command(playername, lowername, command, volume, ourstory);
       }
     else {
       if (handle_action(playername, lowername, command, volume) == 1)
        ourstory = handle_command(playername, lowername, command, volume, ourstory);
       else { }
      }
     }
     else if (sscanf(inbuf, "%s tells you: %c", playername, &c) == 2) {
        strcpy(lowername,playername);
        strtolower(lowername);
         command = inbuf+strlen(playername)+TELL_OFFSET;

         if (command[strlen(command)-1]=='\"') command[strlen(command)-1]='\0';

         sprintf(mess,"TELL: \"%s\" \"%s\"",playername,command);
         wlog(mess);

         volume = WHISPER;
     if (ourstory && ourstory->writing) {
         if (strcmp(lowername,ourstory->author)) {
          sendchat(".tell %s I can't talk to you now, I'm busy listening to %s's story.\n", lowername, ourstory->author);
          continue;
         } /* end of if strcmp */
       ourstory = handle_command(playername, lowername, command, volume, ourstory);
       }
     else {
       if (handle_action(playername, lowername, command, volume) == 1)
        ourstory = handle_command(playername, lowername, command, volume, ourstory);
       else { }
      }
     }

     else if (sscanf(inbuf, "%s shouts %c", playername, &c) == 2) {
        strcpy(lowername,playername);
        strtolower(lowername);
         command = inbuf+strlen(playername)+SHOUT_OFFSET;
         if (command[strlen(command)-1]=='\"') command[strlen(command)-1]='\0';

         sprintf(mess,"SHOUT: \"%s\" \"%s\"",playername,command);
         wlog(mess);

         volume = SHOUT;
       handle_action(playername, lowername, command, volume);
     }

     else if (sscanf(inbuf, "--> %s ", playername)) {
        strcpy(lowername,playername);
        strtolower(lowername);
         command = inbuf+strlen(playername)+SEMOTE_OFFSET;
         if (command[strlen(command)-1]=='\"') command[strlen(command)-1]='\0';

         sprintf(mess,"SEMOTE: \"%s\" \"%s\"",playername,command);
         wlog(mess);

             volume = SEMOTE;
             handle_action(playername, lowername, command, volume);
         }

     else if (sscanf(inbuf, "& %s ", playername)) {
          if (strstr(inbuf,"Spokes") || strstr(inbuf,"spokes")) {
        strcpy(lowername,playername);
        strtolower(lowername);
         command = inbuf+strlen(playername)+SHEMOTE_OFFSET;
         if (command[strlen(command)-1]=='\"') command[strlen(command)-1]='\0';

         sprintf(mess,"SHEMOTE: \"%s\" \"%s\"",playername,command);
         wlog(mess);

             volume = SHEMOTE;
             handle_action(playername, lowername, command, volume);
             }
         }

     else if (sscanf(inbuf, "%s ", playername)) {
        strcpy(lowername,playername);
        strtolower(lowername);
             command = inbuf+strlen(playername)+EMOTE_OFFSET;
             volume = EMOTE;

	 if (logging==1) {
	  if (strcmp(lowername,BOT_NAME)) {
		fputs(inbuf,log_fp);
		fputs("\n",log_fp);
		}
	  }

             handle_action(playername, lowername, command, volume);
         }

   }
}

/* This function handle all commands sent to the bot that */
/* aren't related to story functions */
int handle_action(char *playername, char *lowername, char *command, int volume)
{
char command2[800];
char comstr[50];
char namestr[22];
int chance=0;
int u;

comstr[0]=0;
namestr[0]=0;

if (!strcmp(lowername,BOT_NAME)) goto ENDING;

u=get_user_num(playername);

/* Copy the original command line to another buffer so we can */
/* lowercase it */
strcpy(command2,command);
strtolower(command2);

/* SAY part */
if (volume==SAY) {
 if (!strcmp(command2,BOT_NAME)) {
   sendchat("What do you want, %s? ;-)\n",playername);
   goto ENDING;
   }
 if (strstr(command2,BOT_NAME)) {
   if (strstr (command2, "fuck you" ) ||
       strstr (command2, "screw you" )) {
       sendchat(".tell %s Sorry, I don't do requests.\n",playername);
       sendchat(".shemote sings to %s \"Na na na na, na na na na, hey hey hey, goodbye!\"\n",playername);
        sendchat(".kill %s\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "how are you" ) ||
            strstr (command2, "how are ya" ) ||
            strstr (command2, "how are u" ) ||
            strstr (command2, "how be you" ) ||
            strstr (command2, "how be ya" ) ||
            strstr (command2, "how goes it" )) {
        sendchat(".say I'm good %s, how are you?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "what is up" ) ||
            strstr (command2, "what's up" )) {
        sendchat(".say An adverb, %s.\n",playername);
        sendchat(".emote rolls his eyes\n");
        goto ENDING;
      }
   else if (strstr (command2, "what is goin down" ) ||
            strstr (command2, "what's goin down" )) {
        sendchat(".say No %s, what's goin up!?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "que pasa" ) ||
            strstr (command2, "what is happenin" ) ||
            strstr (command2, "what are you doin" ) ||
            strstr (command2, "whatcha doin" ) ||
            strstr (command2, "what's happenin" )) {
        sendchat(".say Not much %s, you?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "you suck" ) ||
            strstr (command2, "you blow" ) ||
            strstr (command2, "you stink" ) ||
            strstr (command2, "you bite" )) {
        sendchat(".say Yeah? And you smell bad!\n");
        goto ENDING;
      }
   else if (strstr (command2, "shut up" ) ||
            strstr (command2, "shuddup" ) ||
            strstr (command2, "hush" ) ||
            strstr (command2, "be quiet" )) {
        sendchat(".say And who's gonna make me? You? HAH!\n");
        goto ENDING;
      }
  } /* end of check for bots name */

   if (strstr (command2, "i'm good" ) ||
            strstr (command2, "i am good" ) ||
            strstr (command2, "im good" ) ||
            strstr (command2, "i'm ok" ) ||
            strstr (command2, "i am ok" ) ||
            strstr (command2, "im ok" ) ||
            strstr (command2, "i'm well" ) ||
            strstr (command2, "i am well" ) ||
            strstr (command2, "im well" ) ||
            strstr (command2, "nuthin" ) ||
            strstr (command2, "not too much" ) ||
            strstr (command2, "not a lot" ) ||
            strstr (command2, "nothing" ) ||
            strstr (command2, "i'm fine" ) ||
            strstr (command2, "i am fine" ) ||
            strstr (command2, "im fine" ) ||
            strstr (command2, "i'm so-so" ) ||
            strstr (command2, "i am so-so" ) ||
            strstr (command2, "im so-so" ) ||
            strstr (command2, "i'm alright" ) ||
            strstr (command2, "i am alright" ) ||
            strstr (command2, "im alright" )) {
        chance = rand() % 2;
        if (chance==1) sendchat(".emote nods\n");
        else sendchat(".say I see\n");
        goto ENDING;
      }

   else if (strstr (command2, "lag" )) {
        chance = rand() % 3;
        if (chance==1)
         sendchat(".say Where?? WHERE!?!?\n");
        else if (chance==2)
         sendchat(".say NO!!! Anything but that!!\n");
	else
         sendchat(".say Really %s? Sucks bein' you!\n",playername);

        sendchat(".emote hides\n");
        goto ENDING;
      }

   else if (strstr (command2, "vax" ) ||
	    strstr (command2, "v a x" ) ||
	    strstr (command2, "v-a-x" ) ||
	    strstr (command2, "v_a_x" ) ||
	    strstr (command2, "v m s" ) ||
	    strstr (command2, "v-m-s" ) ||
	    strstr (command2, "v_m_s" ) ||
	    strstr (command2, "vms" )) {
        sendchat(".say Such language cannot go unpunished!\n");
        sendchat(".kill %s\n",playername);
	goto ENDING;
	}

if (strstr(command2,BOT_NAME)) {
    if (strstr (command2, "hi" ) ||
            strstr (command2, "hello" ) ||
            strstr (command2, "yo" ) ||
            MATCH (command2, "re *" )) {
	if (!ustr[u].logon) {
        chance = rand() % 3;
        if (chance==1) sendchat(".say Hiya %s!\n",playername);
        else if (chance==2) sendchat(".say Hola %s!\n",playername);
        else sendchat(".say Howdy %s!\n",playername);
	}
	else ustr[u].logon=0;

        goto ENDING;
      }
    else if (strstr (command2, "bye" ) ||
            strstr (command2, "ciao" ) ||
            strstr (command2, "cya" ) ||
            strstr (command2, "adios" )) {
        chance = rand() % 3;
        if (chance==1) sendchat(".say Bye %s!\n",playername);
        else if (chance==2) sendchat(".say Adios %s!\n",playername);
        else sendchat(".say Ciao, %s!\n",playername);
        goto ENDING;
      }

    }

 return 1;
}  /* end of SAY part */

/* WHISPER part */
if (volume==WHISPER) {
     if (MATCH (command2, "fuck you*" ) ||
         MATCH (command2, "screw you*" )) {
       sendchat(".tell %s Sorry, I don't do requests.\n",playername);
       sendchat(".shemote sings to %s \"Na na na na, na na na na, hey hey hey, goodbye!\"\n",playername);
       sendchat(".kill %s\n",playername);
       goto ENDING;
      }
   else if (strstr (command2, "how are you" ) ||
            strstr (command2, "how be you" ) ||
            strstr (command2, "how are ya" ) ||
            strstr (command2, "how are u" ) ||
            strstr (command2, "how be ya" ) ||
            strstr (command2, "how goes it" )) {
        sendchat(".tell %s I'm good, how are you?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "what is up" ) ||
            strstr (command2, "what's up" )) {
        sendchat(".tell %s An adverb, %s.\n",playername,playername);
        sendchat(".semote %s rolls his eyes\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "what is goin down" ) ||
            strstr (command2, "what's goin down" )) {
        sendchat(".tell %s No, what's goin up!?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "que pasa" ) ||
            strstr (command2, "what is happenin" ) ||
            strstr (command2, "what are you doin" ) ||
            strstr (command2, "whatcha doin" ) ||
            strstr (command2, "what's happenin" )) {
        sendchat(".tell %s Not much, you?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "you suck" ) ||
            strstr (command2, "you blow" ) ||
            strstr (command2, "you stink" ) ||
            strstr (command2, "you bite" )) {
        sendchat(".tell Yeah? And you smell bad!\n");
        goto ENDING;
      }
   else if (strstr (command2, "shut up" ) ||
            strstr (command2, "shuddup" ) ||
            strstr (command2, "hush" ) ||
            strstr (command2, "be quiet" )) {
        sendchat(".tell And who's gonna make me? You? HAH!\n");
        goto ENDING;
      }
   else if (strstr (command2, "i'm good" ) ||
            strstr (command2, "i am good" ) ||
            strstr (command2, "im good" ) ||
            strstr (command2, "i'm ok" ) ||
            strstr (command2, "i am ok" ) ||
            strstr (command2, "im ok" ) ||
            strstr (command2, "i'm well" ) ||
            strstr (command2, "i am well" ) ||
            strstr (command2, "im well" ) ||
            strstr (command2, "nuthin" ) ||
            strstr (command2, "not too much" ) ||
            strstr (command2, "not a lot" ) ||
            strstr (command2, "nothing" ) ||
            strstr (command2, "i'm fine" ) ||
            strstr (command2, "i am fine" ) ||
            strstr (command2, "im fine" ) ||
            strstr (command2, "i'm so-so" ) ||
            strstr (command2, "i am so-so" ) ||
            strstr (command2, "im so-so" ) ||
            strstr (command2, "i'm alright" ) ||
            strstr (command2, "i am alright" ) ||
            strstr (command2, "im alright" )) {
        chance = rand() % 2;
        if (chance==1) sendchat(".semote %s nods\n",playername);
        else sendchat(".tell %s I see\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "lag" )) {
        chance = rand() % 3;
        if (chance==1)
         sendchat(".tell %s Where?? WHERE!?!?\n",playername);
        else if (chance==2)
         sendchat(".tell %s NO!!! Anything but that!!\n",playername);
	else
         sendchat(".say Really %s? Sucks bein' you!\n",playername);

        sendchat(".semote %s hides\n",playername);
        goto ENDING;
      }
   else if (MATCH (command2, "re *" ) ||
            MATCH (command2, "hello *" ) ||
            MATCH (command2, "yo *" ) ||
            MATCH (command2, "hi*" )) {
	if (!ustr[u].logon) {
        chance = rand() % 3;
        if (chance==1) sendchat(".tell %s Hiya %s!\n",playername,playername);
        else if (chance==2) sendchat(".tell %s Hola %s!\n",playername,playername);
        else sendchat(".tell %s Howdy %s!\n",playername,playername);
	}
	else ustr[u].logon=0;
       goto ENDING;
      }
   else if (MATCH (command2, "bye *" ) ||
            MATCH (command2, "ciao *" ) ||
            MATCH (command2, "cya *" ) ||
            MATCH (command2, "adios *" )) {
        chance = rand() % 3;
        if (chance==1) sendchat(".tell %s Bye %s!\n",playername,playername);
        else if (chance==2) sendchat(".tell %s Adios %s!\n",playername,playername);
        else sendchat(".tell %s Ciao, %s!\n",playername,playername);
       goto ENDING;
      }
   else if (strstr (command2, "vax" ) ||
	    strstr (command2, "v a x" ) ||
	    strstr (command2, "v-a-x" ) ||
	    strstr (command2, "v_a_x" ) ||
	    strstr (command2, "v m s" ) ||
	    strstr (command2, "v-m-s" ) ||
	    strstr (command2, "v_m_s" ) ||
	    strstr (command2, "vms" )) {
        sendchat(".tell %s Such language cannot go unpunished!\n",playername);
        sendchat(".kill %s\n",playername);
	goto ENDING;
	}

  return 1;
}  /* end of WHISPER part */



/* SHOUT part */
else if (volume==SHOUT) {
  if (strstr(command2, BOT_NAME)) {
     if (strstr (command2, "fuck you*" ) ||
         strstr (command2, "screw you*" )) {
       sendchat(".shout Sorry %s, I don't do requests.\n",playername);
       sendchat(".shemote sings to %s \"Na na na na, na na na na, hey hey hey, goodbye!\"\n",playername);
       sendchat(".kill %s\n",playername);
       goto ENDING;
      }
   else if (strstr (command2, "how are you" ) ||
            strstr (command2, "how be you" ) ||
            strstr (command2, "how are ya" ) ||
            strstr (command2, "how are u" ) ||
            strstr (command2, "how be ya" ) ||
            strstr (command2, "how goes it" )) {
        sendchat(".shout I'm good %s, how are you?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "what is up" ) ||
            strstr (command2, "what's up" )) {
        sendchat(".shout An adverb, %s.\n",playername);
        sendchat(".shemote rolls his eyes\n");
        goto ENDING;
      }
   else if (strstr (command2, "what is goin down" ) ||
            strstr (command2, "what's goin down" )) {
        sendchat(".shout No %s, what's goin up!?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "que pasa" ) ||
            strstr (command2, "what is happenin" ) ||
            strstr (command2, "what are you doin" ) ||
            strstr (command2, "whatcha doin" ) ||
            strstr (command2, "what's happenin" )) {
        sendchat(".shout Not much %s, you?\n",playername);
        goto ENDING;
      }
   else if (strstr (command2, "you suck" ) ||
            strstr (command2, "you blow" ) ||
            strstr (command2, "you stink" ) ||
            strstr (command2, "you bite" )) {
        sendchat(".shout Yeah? And you smell bad!\n");
        goto ENDING;
      }
   else if (strstr (command2, "shut up" ) ||
            strstr (command2, "shuddup" ) ||
            strstr (command2, "hush" ) ||
            strstr (command2, "be quiet" )) {
        sendchat(".shout And who's gonna make me? You? HAH!\n");
        goto ENDING;
      }
   else if (strstr (command2, "i'm good" ) ||
            strstr (command2, "i am good" ) ||
            strstr (command2, "im good" ) ||
            strstr (command2, "i'm ok" ) ||
            strstr (command2, "i am ok" ) ||
            strstr (command2, "im ok" ) ||
            strstr (command2, "i'm well" ) ||
            strstr (command2, "i am well" ) ||
            strstr (command2, "im well" ) ||
            strstr (command2, "i'm fine" ) ||
            strstr (command2, "nuthin" ) ||
            strstr (command2, "not too much" ) ||
            strstr (command2, "not a lot" ) ||
            strstr (command2, "nothing" ) ||
            strstr (command2, "i am fine" ) ||
            strstr (command2, "im fine" ) ||
            strstr (command2, "i'm so-so" ) ||
            strstr (command2, "i am so-so" ) ||
            strstr (command2, "im so-so" ) ||
            strstr (command2, "i'm alright" ) ||
            strstr (command2, "im alright" ) ||
            strstr (command2, "i am alright" )) {
        chance = rand() % 2;
        if (chance==1) sendchat(".shemote nods\n");
        else sendchat(".shout I see\n");
        goto ENDING;
      }
   else if (MATCH (command2, "re *" ) ||
           strstr (command2, "hello" ) ||
           strstr (command2, "yo" ) ||
           strstr (command2, "hi" )) {
	if (!ustr[u].logon) {
        chance = rand() % 3;
        if (chance==1) sendchat(".shout Hiya %s!\n",playername);
        else if (chance==2) sendchat(".shout Hola %s!\n",playername);
        else sendchat(".shout Howdy %s!\n",playername);
	}
	else ustr[u].logon=0;

      goto ENDING;
   }
   else if (strstr (command2, "bye" ) ||
           strstr (command2, "ciao" ) ||
           strstr (command2, "cya" ) ||
           strstr (command2, "adios" )) {
        chance = rand() % 3;
        if (chance==1) sendchat(".shout Bye %s!\n",playername);
        else if (chance==2) sendchat(".shout Adios %s!\n",playername);
        else sendchat(".shout Ciao, %s!\n",playername);
      goto ENDING;
   }
 } /* end of if name check */

   if (strstr (command2, "lag" )) {
        chance = rand() % 3;
        if (chance==1)
         sendchat(".shout Where?? WHERE!?!?\n");
        else if (chance==2)
         sendchat(".shout NO!!! Anything but that!!\n");
	else 
         sendchat(".say Really %s? Sucks bein' you!\n",playername);

        sendchat(".shemote hides\n");
        goto ENDING;
      }
   else if (strstr (command2, "vax" ) ||
	    strstr (command2, "v a x" ) ||
	    strstr (command2, "v-a-x" ) ||
	    strstr (command2, "v_a_x" ) ||
	    strstr (command2, "v m s" ) ||
	    strstr (command2, "v-m-s" ) ||
	    strstr (command2, "v_m_s" ) ||
	    strstr (command2, "vms" )) {
        sendchat(".shout Such language cannot go unpunished!\n");
        sendchat(".kill %s\n",playername);
	goto ENDING;
	}

 return 1;
} /* end of SHOUT */
  

/* EMOTE part */
else if (volume==EMOTE) {
 if (strstr(command2,BOT_NAME)) {
  if (MATCH (command2, "kicks *" )) {
       sendchat("; kicks %s back!\n",playername);
      goto ENDING;
      }
  if (MATCH (command2, "smacks *" )) {
       sendchat("; smacks %s back!\n",playername);
      goto ENDING;
      }
  if (MATCH (command2, "pounds *" )) {
       sendchat("; gets up and thumbs his probiscus at %s\n",playername);
      goto ENDING;
      }
  else if (MATCH (command2, "licks *" )) {
       sendchat("; ewwwwwsss, %s cooties!\n",playername);
      goto ENDING;
      }
  else if (MATCH (command2, "kisses *" )) {
       sendchat("; blushes and kisses %s back on the cheek.\n",playername);
      goto ENDING;
      }
  else if (MATCH (command2, "hug* *" )) {
       sendchat("; huggles back\n");
      goto ENDING;
      }
  else if (MATCH (command2, "pokes *" )) {
       sendchat("; pokies %s in some choice spots.\n",playername);
      goto ENDING;
      }
  }  /* end of name check */

  if (strstr (command2, "laughs" ) ||
           strstr (command2, "laffs" ) ||
           strstr (command2, "chuckles" ) ||
           strstr (command2, "chortles" ) ||
           strstr (command2, "snickers" )) {
       sendchat("; chuckles a bit.\n");
       goto ENDING;
      }
  if (strstr (command2, "giggles" ) ||
      strstr (command2, "gigglz" )) {
      sendchat("; pinches %s's cheeks, you're so cute!\n",playername);
      goto ENDING;
     }
  if (strstr (command2, "vax" ) ||
	    strstr (command2, "v a x" ) ||
	    strstr (command2, "v-a-x" ) ||
	    strstr (command2, "v_a_x" ) ||
	    strstr (command2, "v m s" ) ||
	    strstr (command2, "v-m-s" ) ||
	    strstr (command2, "v_m_s" ) ||
	    strstr (command2, "vms" )) {
        sendchat(".say Such language cannot go unpunished!\n");
        sendchat(".kill %s\n",playername);
	goto ENDING;
	}

  return 1;
}  /* end of EMOTE */

/* SEMOTE or SHEMOTE part */
else if ((volume==SEMOTE) || (volume==SHEMOTE)) {
  if (volume==SEMOTE) {
    sprintf(comstr,".semote %s",playername);
    strcpy(namestr,"you");
    }
  else if (volume==SHEMOTE) {
    strcpy(comstr,".shemote");
    strcpy(namestr,playername);
    }

  if (strstr (command2, "kicks" )) {
       sendchat("%s kicks %s back!\n",comstr,namestr);
      goto ENDING;
      }
  if (strstr (command2, "smacks" )) {
       sendchat("%s smacks %s back!\n",comstr,namestr);
      goto ENDING;
      }
  if (strstr (command2, "pounds" )) {
       sendchat("%s gets up and thumbs his probiscus at %s\n",comstr,namestr);
      goto ENDING;
      }
  else if (strstr (command2, "licks" )) {
      sendchat("%s ewwwwwsss, cooties!\n",comstr);
      goto ENDING;
      }
  else if (strstr (command2, " pokes" )) {
      sendchat("%s pokies %s in some choice spots.\n",comstr,namestr);
      goto ENDING;
      }
  else if (strstr (command2, "kisses" )) {
      sendchat("%s blushes and kisses %s back on the cheek.\n",comstr,namestr);
      goto ENDING;
      }
  else if (strstr (command2, "hug" )) {
      sendchat("%s huggles %s back.\n",comstr,namestr);
      goto ENDING;
      }
  else if (strstr (command2, "laughs" ) ||
           strstr (command2, "laffs" ) ||
           strstr (command2, "chuckles" ) ||
           strstr (command2, "chortles" ) ||
           strstr (command2, "snickers" )) {
       sendchat("%s chuckles a bit.\n",comstr);
       goto ENDING;
      }
  else if (strstr (command2, "giggles" ) ||
      strstr (command2, "gigglz" )) {
      sendchat("%s pinches your cheeks, you're so cute!\n",comstr);
      goto ENDING;
     }
  else if (strstr (command2, "pokes" )) {
       sendchat("%s pokes %s in the ribs.\n",comstr,namestr);
      goto ENDING;
      }

  return 1;
 }  /* end of SEMOTE and SHEMOTE */


else {
     return 1;
     }

ENDING:
return 0;

}


/*****************************************************************
 * smatch: Given a data string and a pattern containing one or
 * more embedded stars (*) (which match any number of characters)
 * return true if the match succeeds, and set res[i] to the
 * characters matched by the 'i'th *.
 *****************************************************************/

int smatch (register char *dat, register char *pat, register char **res)
{
  register char *star = 0, *starend = 0, *resp = 0;
  int nres = 0;

  while (1)
  { if (*pat == '*')
    { star = ++pat; 			     /* Pattern after * */
      starend = dat; 			     /* Data after * match */
      resp = res[nres++]; 		     /* Result string */
      *resp = '\0'; 			     /* Initially null */
    }
    else if (*dat == *pat) 		     /* Characters match */
    { if (*pat == '\0') 		     /* Pattern matches */
	return (1);
      pat++; 				     /* Try next position */
      dat++;
    }
    else
    { if (*dat == '\0') 		     /* Pattern fails - no more */
	return (0); 			     /* data */
      if (star == 0) 			     /* Pattern fails - no * to */
	return (0); 			     /* adjust */
      pat = star; 			     /* Restart pattern after * */
      *resp++ = *starend; 		     /* Copy character to result */
      *resp = '\0'; 			     /* null terminate */
      dat = ++starend; 			     /* Rescan after copied char */
    }
  }
}


/* The handle_command routine calls the appropriate functions for
   the thing just said, and the context given.  If what was said
   doesn't make sense as a command, we ignore it if it was said
   (assuming it was meant for someone else in the room).          */

struct storystate *handle_command(playername, lowername, command, volume, story)
char *playername, *lowername, *command;
int volume;
struct storystate *story;
{
  char buffer[MAXSTRLEN];
  char buffer2[MAXSTRLEN];
  char dirbuf[256];
  char titlebuf[256];
  char author[22];
  int i,u,found=0;
  struct dirent *dp;
  FILE *fp;
  FILE *tfp;
  DIR *dirp;
  time_t tm;

  u=get_user_num(lowername);

  if (!strcasecmp(command, "help")) 
    give_help(playername, lowername, story);
  else if (story && story->writing) {
    if (strcmp(lowername, story->author) && strcmp(lowername,ROOT_ID))
    { 
      if (volume == WHISPER) 
        sendchat(".tell %s Sorry, I'm busy listening to %s's story just now...\n",
                playername, story->author);
    }
    else {
      story = handle_writing(playername, lowername, command, story);
    }
  }
  else {
    if (volume == WHISPER) {
       if (!handle_page2(command,playername,lowername))
       sendchat(".tell %s I can't hear you! :-P\n", playername);
       }
    else if (!strncasecmp(command, "delete ", 7)) {
      if (story && story->numkids > 0) {
        sendchat(".tell %s I'm already in the middle of reading a story, %s.\n",
               playername, playername);
        sendchat(".tell %s Say 'stop' to abort reading this story first.\n",
               playername);
      }
      else {
         /* Quick way to separate a spaced title into one array */
         /* since sscanf reads up to a space                    */
         strcpy(buffer,"");
         for (i=0;i<strlen(command);++i) {
          if (i<7) continue;
          else if (i==7) buffer[0]=command[i];
          else buffer[i-7]=command[i];
          }
          buffer[i-7]=0;
          i=0;
          sprintf(buffer2,"%s",STORYDIR);
          dirp=opendir((char *)buffer2);

          if (dirp == NULL) {
            sendchat(".tell %s Can't open the story directory to delete your story\n",playername);
            sendchat(".tell %s Please tell %s about this.\n",playername,ROOT_ID);
            wlog("d1: Can't open story directory");
            story = NULL;
            goto END;
            }
          while (((dp = readdir(dirp)) != NULL) && (!found))
           {
            sprintf(dirbuf,"%s",dp->d_name);
            if (!strcmp(dirbuf,buffer)) found=1;
            else dirbuf[0]=0;
           }
         (void) closedir(dirp);

        /* this if should never prove true if user typed it in full */
         if (!found) {
            sendchat(".tell %s I can't find that story to delete! Sorry.\n",playername);
            sendchat(".tell %s Make sure you type the title in full with proper capitalization\n",playername);
            wlog("d2: Can't find story to delete");
            story = NULL;
            goto END;
         }

         found=0;

      /* Check if user wanting to delete is author or root */

      sprintf(buffer2, "%s/%s/a.idx", STORYDIR, dirbuf);
      if ((tfp = fopen(buffer2, "r")) != NULL) {
        if (fgets(author, MAXSTRLEN, tfp) != NULL) {
          author[strlen(author)-1] = '\0';
          }
        else wlog("d3: Can't get author for story delete");
        }
      else wlog("d4: Can't open story index file for delete");

      fclose(tfp);
      if (!strcmp(lowername,author) || !strcmp(lowername,ROOT_ID)) { }
      else {
          sendchat(".tell %s You must be the author to delete this story!\n",playername);
          wlog("d5: No permission to delete story.");
          story = NULL;
          goto END;
         }
      
      /* Remove all files in directory */
          sprintf(buffer2,"%s/%s",STORYDIR,dirbuf);
          dirp=opendir((char *)buffer2);

          if (dirp == NULL) {
            sendchat(".tell %s Can't open the story's directory to delete your story\n",playername);
            sendchat(".tell %s Please tell %s about this.\n",playername,ROOT_ID);
            wlog("d6: Can't open the story's directory.");
            story = NULL;
            goto END;
            }
          while (((dp = readdir(dirp)) != NULL) && (!found))
           {
            sprintf(titlebuf,"%s/%s",buffer2,dp->d_name);
            if (dp->d_name[0]!='.') {
              remove(titlebuf);
              }
            titlebuf[0]=0;
           }

          (void) closedir(dirp);

          /* Remove empty directory */
          sprintf(dirbuf,"%s/%s",STORYDIR,buffer);
          if (rmdir(dirbuf) == -1) {
            sendchat(".tell %s Could not remove empty directory..\n",playername);
            sendchat(".tell %s Please tell %s about this.\n",playername,ROOT_ID);
            wlog("d7: Can't remove empty directory.");
            story = NULL;
            goto END;
          }

          sendchat(".tell %s Ok, I deleted that story *sigh*\n",playername);
          sendchat(".emote deletes a story for %s.\n",playername);
          sendchat(".say I'm tellin' ya, that was a really good story!\n");
          wlog("Story deleted");
       } /* end of if not in story in else */
     }  /* end of delete command */
    else if (!strncasecmp(command, "execute ", 8)) {
	if (strcmp(ustr[u].name,ROOT_ID)) {
    	  if (volume == WHISPER)
	   sendchat(".say Only %s can tell me to do that! :-P\n",ROOT_ID);
	  else
	   sendchat(".tell %s Only %s can tell me to do that! :-P\n",ustr[u].name,ROOT_ID);
	  goto END;
	  }
	remove_first(command);
	sendchat(".tell %s Ok, executing \"%s\"\n",ROOT_ID,command);
	sendchat("%s\n",command);
	goto END;
	}
    else if (!strncasecmp(command, "logging ", 8)) {
	if (strcmp(ustr[u].name,ROOT_ID)) {
    	  if (volume == WHISPER)
	   sendchat(".say Only %s can tell me to do that! :-P\n",ROOT_ID);
	  else
	   sendchat(".tell %s Only %s can tell me to do that! :-P\n",ustr[u].name,ROOT_ID);
	  goto END;
	  }
	remove_first(command);
	if (!strcmp(command,"on")) {
	strcpy(buffer2, LOG_FILE);
	if (!(log_fp = fopen(buffer2, "a"))) {
	  if (volume == WHISPER)
	   sendchat(".tell %s I can't open my notepad, %s!\n",ustr[u].name,ustr[u].say_name);
	  else
	   sendchat(".say I can't open my notepad!\n");
	  goto END;
	  }
	if (volume == WHISPER) {
	 sendchat(".sem %s gets out his notepad and felt tip pen.\n",ustr[u].name);
	 sendchat(".tell %s Ok, i'm ready!\n",ustr[u].name);
	}
	else {
	 sendchat(".emote gets out his notepad and felt tip pen.\n");
	 sendchat(".say Ok, i'm ready!\n");
	 }
	logging=1;
	time(&tm);
	sprintf(buffer,"*** Log started on %s",ctime(&tm));
	fputs(buffer,log_fp);
	goto END;
	}
	else if (!strcmp(command,"off")) {
	time(&tm);
	sprintf(buffer,"*** Log stopped on %s",ctime(&tm));
	fputs(buffer,log_fp);
	fclose(log_fp);
	if (volume == WHISPER) {
	 sendchat(".sem %s closes his notepad and caps his pen.\n",ustr[u].name);
	}
	else {
	 sendchat(".emote closes his notepad and caps his pen.\n");
	 }
	logging=0;
	goto END;
	}
	else if (!strcmp(command,"read")) {
	strcpy(buffer2, LOG_FILE);

	/* we could fflush the log file here to write all data to disk */
	/* before we read it, but then we'd still be logging and the   */
	/* bot would log the log he just read back to us. Instead,     */
	/* we'll just close the file, read the log, and open it back   */
	/* up again.				-Cygnus		       */
	if (logging==1) fflush(log_fp);

	if (!(fp = fopen(buffer2, "r"))) {
	  if (volume == WHISPER)
	   sendchat(".tell %s I can't open my notepad, %s!\n",ustr[u].name,ustr[u].say_name);
	  else
	   sendchat(".say I can't open my notepad!\n");

	  goto END;
	  }

	if (volume == WHISPER) {
	 sendchat(".sem %s starts from the beginning.\n",ustr[u].name);
	}
	else {
	 sendchat(".emote starts from the beginning.\n");
	 }

	while (fgets(buffer,1000,fp) != NULL) {
	buffer[strlen(buffer)-1]=0; /* strip nl */
	if (volume == WHISPER)
		sendchat(".tell %s %s\n",ustr[u].name,buffer);
	else
		sendchat(".say %s\n",buffer);
	} /* end of while */
	fclose(fp);
	buffer[0]=0;

	/* reopen log file for continued logging */
	if (logging==1) {
	  log_fp = fopen(buffer2, "a");
	  }

	goto END;
	}
	else if (!strcmp(command,"clear")) {
	remove(LOG_FILE);
	if (volume == WHISPER)
	 sendchat(".semote %s rips up the log and throws it in the garbage\n",ustr[u].name);
	else
	 sendchat(".emote rips up the log and throws it in the garbage\n");
	goto END;
	}
	else {
	if (volume == WHISPER)
	 sendchat(".tell %s I don't understand what you want me to do!\n",ustr[u].name);
	else
	 sendchat(".say I don't understand what you want me to do!\n");	 	
	goto END;
	}
    } /* end of logging command */

    else if (!strncasecmp(command, "list ", 5)) {
      if (story && story->numkids > 0)  {
        sendchat(".tell %s Sorry, I'm in the middle of a story just now.\n",
                playername);
        sendchat(".tell %s Say 'stop' if you want me to read a different one.\n",
               playername);
        }
      else {
		read_titles(command+5,1);
      }
    } /* end of else if */
    else if (!strcasecmp(command, "list")) {
      if (story && story->numkids > 0)  {
        sendchat(".tell %s Sorry, I'm in the middle of a story just now.\n",
                playername);
        sendchat(".tell %s Say 'stop' if you want me to read a different one.\n",
               playername);
        }
      else {
		read_titles(NULL,0);
      }
    } /* end of else if */
    else if (!strncasecmp(command, "read ", 5)) {
      if (story && story->numkids > 0) {
        sendchat(".tell %s I'm already in the middle of reading a story, %s.\n",
               playername, playername);
        sendchat(".tell %s Say 'stop' if you want me to read a different one.\n",
               playername);
      }
      else {
        if (resolve_title(command+5, buffer, YES) == NULL)
          sendchat(".tell %s I don't know which story you mean!\n", playername);
        else {
          sprintf(mess, "%s reads %s", playername, buffer);
          wlog(mess);
          story = start_story(buffer, story);
          sendchat(".desc %s\n", TELLMSG);
          sprintf(buffer, "%s/%s/a.idx", STORYDIR, story->title);
          if ((fp = fopen(buffer, "r")) == NULL) {
            sendchat(".say I can't find that story! Sorry.\n");
            story = NULL;
          }
          else {
            fclose(fp);
            get_paragraph(story);
            read_paragraph(story);
          }
        }
      }
    }
    else if (!strncasecmp(command, "write ", 6)) {
      if (story && story->numkids > 0) {
        sendchat(".tell %s I'm already in the middle of reading a story, %s.\n",
               playername, playername);
        sendchat(".tell %s Say 'stop' if you want to tell me a new one.\n",
               playername);
      }
      else if (!isalpha(command[6]) && !isdigit(command[6]))
        sendchat(".tell %s Titles must start with a letter or digit.\n",
                playername);
      else {
        if (islower(command[6]))
          command[6] = toupper(command[6]);
        if (resolve_title(command+6, buffer, NO) == NULL) {
          story = start_story(command+6, story);
          sendchat(".desc %s %s.\n", LISTENMSG, playername);
          sendchat(";turns his attention to %s and their story.\n", playername);
          strcpy(story->author, lowername);
          story->writing = YES;
          story->editing = NO;
        }
        else 
          sendchat(".tell %s That title is already taken.  Try a different one.\n",
                 playername);
      }
    }
    else if (!strcasecmp(command, "reread")) {
      if (story) 
        read_paragraph(story);
      else
        sendchat(".tell %s I'm not reading any story at the moment.\n", 
               playername);
    }
    else if (!strcasecmp(command, "next") || !strcasecmp(command, "n")) {
      if (story && story->numkids > 0) 
        read_next(NULL, story);
      else
        sendchat(".tell %s There is no next paragraph.\n", playername);
    }
    else if (!strncasecmp(command, "next ", 5)) {
      if (story && story->numkids > 0) 
        read_next(command+5, story);
      else
        sendchat(".tell %s There is no next paragraph.\n", playername);
    }
    else if (!strncasecmp(command, "n ", 2)) {
      if (story && story->numkids > 0) 
        read_next(command+2, story);
      else
        sendchat(".tell %s There is no next paragraph.\n", playername);
    }
    else if (!strcasecmp(command, "back") || !strcasecmp(command, "b")) {
      if (story && strlen(story->paragraphid) > 1)
        read_previous(story);
      else
        sendchat(".tell %s There is no previous paragraph.\n", playername);
    }
    else if (!strcasecmp(command, "stop")) {
      if (story) {
        story = finish_story(story);
        sendchat(";puts aside the story he was reading.\n");
        sendchat(".desc %s\n", IDLEMSG);
      }
      else
        sendchat(".tell %s But I'm not reading any story now!\n", playername);
    }
    else if (!strcasecmp(command, "format")) {
      if (FORMATTED) {
        FORMATTED = NO;
        sendchat(".say Paragraph formatting turned off.\n");
      }
      else {
        FORMATTED = YES;
        sendchat(".say Paragraph formatting turned on.\n");
      }
    }
    else if (!strcasecmp(command, "add")) {
      for (i = 0; i < story->numkids && strcmp(story->kids[i], playername); i++)
        ;
      if (i < story->numkids)
        sendchat(".tell %s You already have a branch of this paragraph.\n",
                playername);
      else {
        strcpy(buffer, story->paragraphid);
        sprintf(buffer+strlen(buffer), "%c", 'a' + story->numkids);
        
        for (i = 0; i < story->numlines; i++)
          free(story->paragraph[i]);
        for (i = 0; i < story->numkids; i++)
          free(story->kids[i]);
        story->numlines = 0;
        story->numkids = 0;
        strcpy(story->paragraphid, buffer);
        strcpy(story->author, lowername);
        sendchat(".desc %s %s.\n", LISTENMSG, playername);
        sendchat(";turns his attention to %s and their story addition.\n",
               playername);
        story->writing = YES;
        story->editing = NO;
      }
    }
    else if (!strcasecmp(command, "edit")) {
      if (strcmp(lowername, story->author)
	&& strcmp(lowername,ROOT_ID))
      {
        sendchat(".tell %s You can't edit someone else's paragraph.\n", 
                playername);
        sendchat(".tell %s If you want to write a different version of this\n",
               playername);
        sendchat(".tell %s paragraph, go 'back' and then 'add'.\n",
               playername);
      }
      else {
        sendchat(".desc %s %s.\n", LISTENMSG, playername);
        sendchat(".tell %s Starting edit at end of paragraph.\n", playername);
        sendchat(".tell %s To start the paragraph over instead, say 'clear'.\n",
               playername);
        sendchat(";turns his attention to %s for a paragraph edit.\n",
                playername);
        story->writing = YES;
        story->editing = YES;
      }
    }
  }

END:
  return(story);
}  

/* Handle_writing handles all the things an author can do while writing. */

struct storystate *handle_writing(playername, lowername, command, story)
char *playername, *lowername, *command;
struct storystate *story;
{
  int i;
  char buffer[MAXSTRLEN];

  if (strlen(command) == 0) {
    if (story->numlines == 0) {           /* Author is done */
      sendchat(".tell %s Nice listening to you.\n", playername);
      story = abort_writing(story);
      wlog("Done with abort writing1.");
    }
    else {                                /* Save this paragraph */
      /* Foist, create a directory if we need one */
      wlog("Setting up new directory..");
      if (!setup_directory(story)) {
        sendchat(".tell %s Sorry, I'm unable to start saving your story!\n",
               playername);
        story = abort_writing(story);
       wlog("Done with abort writing2.");
      }
      else if (!(save_para(story, playername, lowername))) {
        sendchat(".tell %s This just isn't my day...\n", playername);
        story = abort_writing(story);
      wlog("Done with abort writing3.");
      }
      /* If we're just "editing", we're done. */
      else if (story->editing) {
        sendchat(".tell %s Paragraph edited. Thank you for making corrections.\n", 
		playername);
        sendchat(";has finished listening to %s's paragraph.\n", playername);
        story->writing = NO;
        if (story->numkids == 0) 
          sendchat(".desc %s\n", IDLEMSG);
        else 
          sendchat(".desc %s\n", TELLMSG);
      }
      /* If not, start a new para! */
      else {
        sendchat(";logs %s's paragraph.\n", playername);
        sprintf(buffer, "%s%c", story->paragraphid, 'a' + story->numkids);
	strcat(buffer,"\0"); 
       /* Above line ASSUMES ASCII */
        for (i = 0; i < story->numlines; i++)
          free(story->paragraph[i]);
        for (i = 0; i < story->numkids; i++)
          free(story->kids[i]);
        story->numlines = 0;
        story->numkids = 0;
        strcpy(story->paragraphid, buffer);
      }
    }
  }
  else if (!strcasecmp(command, "delete")) {
    if (story->numlines == 0)
      sendchat(".tell %s There are no lines in this paragraph to delete.\n",
             playername);
    else {
      sendchat(".tell %s Deleting: %s\n", 
             playername, story->paragraph[--(story->numlines)]);
      free(story->paragraph[story->numlines]);
    }
  }
  else if (!strcasecmp(command, "abort")) {
    sendchat(".tell %s Paragraph aborted.  Nice listening to you.\n", playername);
    story = abort_writing(story);
      wlog("Done with abort writing4.");
  }
  else if (!strcasecmp(command, "format")) {
    sendchat(".tell %s 'Format' commands have no effect now.\n", playername);
    sendchat(".tell %s (Formatting is always turned off while I'm writing.)\n",
            playername);
  }
  else if (!strcasecmp(command, "clear")) {
    if (story->numlines == 0) {
      sendchat(".tell %s The paragraph is already empty.\n", playername);
      sendchat(".tell %s If you wish to quit, say 'abort'.\n", playername);
    }
    else {
      sendchat(".tell %s Clearing entire paragraph contents.\n", playername);
      for (i = 0; i < story->numlines; i++)
        free(story->paragraph[i]);
      story->numlines = 0;
    }
  }
  else if (!strcasecmp(command, "reread")) {
    for (i = 0; i < story->numlines; i++)
      sendchat(";: %s\n", story->paragraph[i]);
    sendchat(";pauses.\n");
  }
  else {                           /* This is the next line of the story */
    if (story->numlines + 2 == MAXLINES) {
      sendchat(".tell %s Your paragraph is too big.  Please end it.\n",
              playername);
      sendchat(".tell %s (An empty say (with just .say) ends a paragraph.)\n",
              playername);
    }
    else {
      story->paragraph[story->numlines] =
                       (char *) malloc((strlen(command)+1)*sizeof(char));
      strcpy(story->paragraph[(story->numlines)++], command);
    }
  }
  return(story);
}

/* setup_directory makes sure that if this is a new story, a story
   directory exists.  Returns nonzero on success.                      */

int setup_directory(struct storystate *story)
{
  char buffer[MAXSTRLEN];

  sprintf(buffer, "%s/%s", STORYDIR, story->title);
  if (strlen(story->paragraphid) > 1 || story->editing)
    return 1;
  if (mkdir(buffer, 0700) == -1)
    return 0;
  else
    return 1;
}

/* save_para attempts to save a paragraph of the story, and then update
    the appropriate index files.  Returns positive if successful.        */

int save_para(struct storystate *story, char *playername, char *lowername)
{
  char buffer[MAXSTRLEN];
  FILE *fp;
  int i;

  sprintf(buffer, "%s/%s/%s.txt", STORYDIR, story->title, story->paragraphid);
  if ((fp = fopen(buffer, "w")) == NULL) {
    sendchat(".tell %s Sorry, I can't save this paragraph!\n",playername);
    wlog("Paragraph save error");
    return 0;
  }
  for (i = 0; i < story->numlines; i++)
    fprintf(fp, "%s\n", story->paragraph[i]);
  fclose(fp);
  sprintf(mess, "%s saves paragraph in %s", playername, story->title);
  wlog(mess);
  if (!(story->editing)) {
    sprintf(buffer, "%s/%s/%s.idx", STORYDIR, story->title, story->paragraphid);
    if ((fp = fopen(buffer, "w")) == NULL) {
      sendchat(".tell %s I can't save this paragraph! (index creation problem)\n",
              playername);
      wlog("Index creation error");
      return 0;
    }
    fprintf(fp, "%s\n", lowername);
    fclose(fp);
    if (strlen(story->paragraphid) > 1) {     /* Update previous index file */
      sprintf(buffer, "%s/%s/", STORYDIR, story->title);
      strncat(buffer, story->paragraphid, strlen(story->paragraphid)-1);
      strcat(buffer, ".idx");
      if ((fp = fopen(buffer, "a")) == NULL) {
        sendchat(".tell %s I can't save this paragraph! (index update problem)\n",
                playername);
        wlog("Index update error");
        return 0;
      }
      fprintf(fp, "%s\n", lowername);
      fclose(fp);
      sprintf(mess, "Index file for %s updated.", story->title);
      wlog(mess);
    }
    else {                                    /* Make new directory file */
	/*
      sprintf(buffer, "/bin/ls %s > %s", STORYDIR, DIRECTORY);
      system(buffer);
      wlog("New directory created.");
	*/
    }
  }
  return 1;
}

/* Abort_writing cleans up gracefully after we abort an edit */

struct storystate *abort_writing(story)
struct storystate *story;
{
  story->writing = NO;
  wlog("In abort_writing");
  if (story->editing) {
    sendchat(";has finished listening to %s's revision.\n", story->author); 
    get_paragraph(story);
      wlog("Story revision completed.");
    if (story->numkids)
      sendchat(".desc %s\n", TELLMSG);
  }
  else {
    sendchat(";has finished listening to %s's story.\n", story->author); 
    story = finish_story(NULL);
    sendchat(".desc %s\n", IDLEMSG);
    wlog("New story or addition completed.");
  }
  wlog("Leaving abort_writing");
  return story;
}

/* Give_help gives appropriate help for different story states. */

void give_help(char *playername, char *lowername, struct storystate *story)
{
  sendchat("; passes %s a note.\n", playername);
  sendchat(".tell %s I am now ", playername); 
  if (story && story->writing) {
    if (strcmp(lowername, story->author) && strcmp(lowername,ROOT_ID)) {
      sendchat("listening to %s's story.\n", story->author);
      sendchat(".tell %s I can't be of much help until they finish.\n",playername);
    }
    else {
      sendchat("listening to your story.\n");
      sendchat(".tell %s I take anything you say as part of the story, except: \n", playername);
      sendchat(".tell %s reread - read back the paragraph so far.\n",playername);
      sendchat(".tell %s (paragraph formatting will be OFF in this case)\n",playername);
      sendchat(".tell %s delete - delete last line in paragraph.\n",playername);
      sendchat(".tell %s clear - delete entire paragraph.\n", playername);
      sendchat(".tell %s abort - end this writing session.\n",playername);
      sendchat(".tell %s help - give context-sensitive help\n",playername);
      sendchat(".tell %s <blank line> (just .say)- end this paragraph.\n",playername);
      sendchat(".tell %s (this saves the paragraph if anything's in it,\n", playername);
      sendchat(".tell %s and ends this writing session if it's empty.\n", 
              playername);
      sendchat(".tell %s Thus, 2 blank lines save the previous para. and quit.)\n", 
              playername);
    }
  }
  else if (story == NULL || !(story->numkids)) { 
    sendchat("ready to tell a story or hear one of yours.\n");
    sendchat(".tell %s Useful selection commands are:\n", playername);
    sendchat(".tell %s read <story title> - start reading a story\n",playername);
    sendchat(".tell %s write <story title> - start recording your story\n", playername);
    sendchat(".tell %s delete <story title> - delete a story of yours\n",playername);
    sendchat(".tell %s list - list the stories I know\n", playername);
    sendchat(".tell %s list <author> - list the stories I know by <author>\n", playername);
    sendchat(".tell %s format - turn paragraph formatting %s\n",
            playername, (FORMATTED ? "off" : "on"));
    sendchat(".tell %s help - give context-sensitive help\n", playername);
 if (!strcmp(lowername,ROOT_ID)) {
    sendchat(".tell %s execute <command> - run a talker command\n",playername);
    sendchat(".tell %s logging [on|off|read|clear] - log conversation i see to a file \"%s\" for later reading, etc..\n",playername,LOG_FILE);
    }
    sendchat(".tell %s For more documentation, read 'Using %s'.\n", 
            playername,BOT_NAME);
  }
  else {
    sendchat("in the middle of telling a story.\n");
    sendchat(".tell %s Useful reading commands are:\n", playername);
    sendchat(".tell %s n or next - read the next paragraph\n",playername);
    sendchat(".tell %s n or next <authorname> - read the author's next paragraph\n", playername);
    sendchat(".tell %s b or back - read the previous paragraph\n",playername);
    sendchat(".tell %s reread - read the current paragraph again\n",playername);
    sendchat(".tell %s stop - don't read any more of this story\n",playername);
    sendchat(".tell %s format - turn paragraph formatting %s\n",
            playername, (FORMATTED ? "off" : "on"));
    sendchat(".tell %s add - add a branch or extension to this story\n", 
            playername);
    sendchat(".tell %s edit - edit current paragraph (if yours)\n",playername);
    sendchat(".tell %s (Add and edit halt my storytelling until you finish\n", playername);
    sendchat(".t %s adding your part; it's polite to check with other people\n", 
            playername);
    sendchat(".t %s in the room before you do this.)\n", playername);
    sendchat(".t %s help - give context-sensitive help\n", playername);
  }
}

/* read_titles simply reads the directory file, reporting the names
    and authors of the stories it finds there.                          */

void read_titles(char *bitmatch, int mode)
{
  char *upperbit = 0;
  char buffer[MAXSTRLEN], titlebuf[MAXSTRLEN], author[MAXSTRLEN]; 
  char buffer2[30], chunk1[4], chunk2[3];
  char lsdir[256];
  static int breakup=0;
  static int abreakup=0;
  int storynum=0, total_stories=0;
  struct stat fileinfo;
  FILE *tfp;
  struct dirent *dp;
  DIR *dirp;

  if (mode==1) abreakup+=15;
  else breakup+=15;

	sprintf(lsdir,"%s/%s",DIRECTORY,STORYDIR);
	dirp=opendir((char *)lsdir);

  if (dirp == NULL) {
    sendchat(".say I'm sorry, but I can't get to my story directory!\n");
    sendchat(".say Cygnus should know about this-- this shouldn't happen.\n");
    return;
  }
  else {
   if (mode==1) {
    if (!strlen(bitmatch)) {
      sendchat(".say Whose storylist do you want to see?\n");
      abreakup=0;
      return;
      }
    upperbit=bitmatch;
    strtolower(bitmatch);
    /* get number of stories by author first */
    while ((dp = readdir(dirp)) != NULL) {
	sprintf(titlebuf,"%s",dp->d_name);
	if (titlebuf[0]=='.') continue;
      /* titlebuf[strlen(titlebuf)-1] = '\0'; */
      sprintf(buffer, "%s/%s/a.idx", STORYDIR, titlebuf);
      if ((tfp = fopen(buffer, "r")) != NULL) {
        if (fgets(author, MAXSTRLEN, tfp) != NULL) {
          author[strlen(author)-1] = '\0';
	  if (!strcmp(bitmatch,author))
		total_stories++;
        } /* end of if fgets author */
        fclose(tfp);
      } /* end of if fopen */
    } /* end of while */
    if (!total_stories) {
      sendchat(".say I don't know any stories by ^%s^!\n", upperbit);
      abreakup=0;
      return;
     }
   } /* end of if bitmatch */
   else {
    while ((dp = readdir(dirp)) != NULL) {
	sprintf(titlebuf,"%s",dp->d_name);
	if (titlebuf[0]=='.') continue;
		total_stories++;
    }
   } /* end of bitmatch else */
  } /* end of directory read else */
  (void)closedir(dirp);

	sprintf(lsdir,"%s/%s",DIRECTORY,STORYDIR);
	dirp=opendir((char *)lsdir);

  if (dirp == NULL) {
    sendchat(".say I'm sorry, but I can't get to my story directory!\n");
    sendchat(".say Cygnus should know about this-- this shouldn't happen.\n");
  }
  else {
   if (mode==1) {
    if (total_stories==1)
    sendchat(".say I know ^HG%d^ story by ^HM%s^\n",total_stories,
upperbit);
    else
    sendchat(".say I know ^HG%d^ stories by ^HM%s^, here are ^%d^ through ^%d^:\n",total_stories, upperbit, 
		abreakup-14, ( total_stories < abreakup ? total_stories : abreakup ) );

    while ((dp = readdir(dirp)) != NULL) {
	sprintf(titlebuf,"%s",dp->d_name);
	if (titlebuf[0]=='.') continue;
      /* titlebuf[strlen(titlebuf)-1] = '\0'; */
      sprintf(buffer, "%s/%s/a.idx", STORYDIR, titlebuf);
      if ((tfp = fopen(buffer, "r")) != NULL) {
	/* Get last changed info */
      sprintf(buffer2, "%s/%s", STORYDIR, titlebuf);
      stat(buffer2,&fileinfo);
      sprintf(buffer2,"%s",ctime(&fileinfo.st_mtime));
      midcpy(buffer2,chunk1,4,6);
      midcpy(buffer2,chunk2,8,9);
      if (chunk2[0]==' ') midcpy(chunk2,chunk2,1,1);

        if (fgets(author, MAXSTRLEN, tfp) != NULL) {
          author[strlen(author)-1] = '\0';
          if (!strcmp(bitmatch,author)) {
		if (++storynum >= abreakup-14  &&  storynum <= abreakup )
		 sendchat(".say %s (^HM%s^)  [^HG%s-%s^]\n", titlebuf,author,chunk1,chunk2);
		chunk1[0]=0; chunk2[0]=0; buffer2[0]=0;
          }
        } /* end of if fgets author */
        fclose(tfp);
      } /* end of if fopen */
    } /* end of while */
   }
   else {
    if (total_stories==1)
    sendchat(".say I know ^HG%d^ story.\n",total_stories);
    else
    sendchat(".say I know ^HG%d^ stories, here are ^%d^ through ^%d^:\n",total_stories, 
		breakup-14, ( total_stories < breakup ? total_stories : breakup ) );

    while ((dp = readdir(dirp)) != NULL) {
	sprintf(titlebuf,"%s",dp->d_name);
	if (titlebuf[0]=='.') continue;
      /* titlebuf[strlen(titlebuf)-1] = '\0'; */
      sprintf(buffer, "%s/%s/a.idx", STORYDIR, titlebuf);
      if ((tfp = fopen(buffer, "r")) != NULL) {
	/* Get last changed info */
      sprintf(buffer2, "%s/%s", STORYDIR, titlebuf);
      stat(buffer2,&fileinfo);
      sprintf(buffer2,"%s",ctime(&fileinfo.st_mtime));
      midcpy(buffer2,chunk1,4,6);
      midcpy(buffer2,chunk2,8,9);
      if (chunk2[0]==' ') midcpy(chunk2,chunk2,1,1);

        if (fgets(author, MAXSTRLEN, tfp) != NULL) {
          author[strlen(author)-1] = '\0';
			if (++storynum >= breakup-14  &&  storynum <= breakup )
			 sendchat(".say %s (^HM%s^)  [^HG%s-%s^]\n", titlebuf,author,chunk1,chunk2);
			chunk1[0]=0; chunk2[0]=0; buffer2[0]=0;

        } /* end of if fgets author */
        fclose(tfp);
      } /* end of if fopen */
    } /* end of while */
   } /* end of bitmatch else */
   if (mode==1) {
	if (abreakup >= total_stories) {
		sendchat(".say '^LIST %s^' to start over at the beginning of the list...\n", bitmatch);
		abreakup=0;
	} else {
		sendchat(".say '^LIST %s^' for more story titles...\n", bitmatch);
	}
     }
   else {
	if (breakup >= total_stories) {
		sendchat(".say ^LIST^ to start over at the beginning of the list...\n");
		breakup=0;
	} else {
		sendchat(".say ^LIST^ for more story titles...\n");
	}
     }
  } /* end of main else */
  (void)closedir(dirp);

}

/* resolve_title read the directory file, looking for a story which
   is uniquely identified by the prefix the user gave (if acceptprefix
   if YES), or by the exact title (if acceptprefix is NO).
   If no such story exists, or if the prefix is ambiguous, we return NULL.
   (If, however, the prefix exactly matches a title, we return that.) */

char *resolve_title(prefix, buffer, acceptprefix)
char *prefix, *buffer;
int acceptprefix;
{
  char inbuf[MAXSTRLEN];
  char tmp[256];
  int storymatch;
  int exactmatch;
  struct dirent *dp;
  DIR *dirp;

  exactmatch = NO;
  if (strlen(prefix) == 0)
    return NULL;
  storymatch = 0;
  sprintf(tmp,"%s/%s",DIRECTORY,STORYDIR);
	dirp=opendir((char *)tmp);

  if (dirp == NULL) {
    sendchat(".say I'm sorry, but I can't get to my story directory!\n");
    sendchat(".say Cygnus should know about this-- this shouldn't happen.\n");
  }
  else {
    while ((dp = readdir(dirp)) != NULL && !exactmatch) {
	sprintf(inbuf,"%s",dp->d_name);
	if (inbuf[0]=='.') continue;
      /* inbuf[strlen(inbuf)-1] = '\0'; */
      if (!strncasecmp(prefix, inbuf, strlen(prefix))) {
        if (acceptprefix)
          storymatch++;
        strcpy(buffer, inbuf);
        if (!strcasecmp(buffer, prefix))
          exactmatch = YES;
      } /* end of sub if */
    } /* end of while */
  } /* end of else */
  (void)closedir(dirp);
  if (storymatch == 1 || exactmatch)
    return buffer;
  else
    return(NULL);
}

/* start_story initializes a new story structure.            */

struct storystate *start_story(title, story)
char *title;
struct storystate *story;
{
  int i;
  finish_story(story);
  story = (struct storystate *) malloc(sizeof(struct storystate));
  strcpy(story->title, title);
  strcpy(story->author, "");         /* Will be reset in a little while. */
  strcpy(story->paragraphid, "a");
  for (i = 0; i < MAXLINES; i++)
    story->paragraph[i] = NULL;
  story->writing = 0;                /* Will be reset if we're writing   */
  story->numkids = 0;
  story->numlines = 0;
  return story;
}

/* Get_paragraph reads in the necessary information about the paragraph
   designated by story->paragraphid, and updates the structure appropriately.

   The format for a .idx file is:
     Author of current paragraph
     Author of child paragraph a (if any)
     Author of child paragraph b (if any)
       ...
*/

void get_paragraph(struct storystate *story)
{
  FILE *fp;
  char filename[MAXSTRLEN], inbuf[MAXSTRLEN];
  int i;

  sprintf(filename, "%s/%s/%s.idx", STORYDIR, story->title, story->paragraphid);
  if ((fp = fopen(filename, "r")) == NULL) {
    sendchat(".say Hey! I can't retrieve index file for next paragraph!\n");
    crash_n_burn("idx lookup failure %s!"); 
  }
  else {
    for (i = 0; i < story->numkids; i++)
      free(story->kids[i]);
    story->numkids = 0;
    fgets(inbuf, MAXSTRLEN, fp);
    inbuf[strlen(inbuf)-1] = '\0';
    if (strcmp(story->author, inbuf)) {
      sendchat(".say (%s wrote this part of the story.)\n", inbuf);
      strcpy(story->author, inbuf);
    }
    for (i = 0; fgets(inbuf, MAXSTRLEN, fp) != NULL; i++) {
      inbuf[strlen(inbuf)-1] = '\0';
      story->kids[i] = (char *) malloc((strlen(inbuf)+1) * sizeof(char));
      strcpy(story->kids[i], inbuf);
    }
    story->kids[i] = NULL;
    story->numkids = i;
  }
  fclose(fp);
  if (story->numkids == 0) 
    sendchat(".desc %s\n", IDLEMSG);

  sprintf(filename, "%s/%s/%s.txt", STORYDIR, story->title, story->paragraphid);
  if ((fp = fopen(filename, "r")) == NULL) {
    sendchat(".say Hey! I can't retrieve text file for next paragraph!\n");
    crash_n_burn("txt lookup failure %s!"); 
  }
  else {
    for (i = 0; i < story->numlines; i++)
      free(story->paragraph[i]);
    for (i = 0; fgets(inbuf, MAXSTRLEN, fp) != NULL; i++) {
      inbuf[strlen(inbuf)-1] = '\0';
      story->paragraph[i] = (char *) malloc((strlen(inbuf)+1) * sizeof(char));
      strcpy(story->paragraph[i], inbuf);
    }
    story->paragraph[i] = NULL;
    story->numlines = i;
  }
  fclose(fp);
}

/* read_next(author, story) goes on to the 'next' paragraph by 
   that author.  If author is NULL, it goes on to the next
   paragraph by the same author as the current paragraph, or
   if there is no such paragraph, to the first paragraph
   which was written to follow this one.

   If the author doesn't match, we pretend we never heard the cmd       */

void read_next(char *author, struct storystate *story)
{
  int i;
  char c = '\0';
  int exactmatch;

  if (author == NULL) {
    for (i = 0; i < story->numkids; i++) {
      if (!strcasecmp(story->author, story->kids[i]))
        c = 'a' + i;                               /* ASSUMES ASCII */
    }
    if (c == '\0')
        c = 'a';
  }
  else {
    exactmatch = 0;
    for (i = 0; i < story->numkids && !exactmatch; i++) {
      if (!strncasecmp(author, story->kids[i], strlen(author)))
        c = 'a' + i;                               /* ASSUMES ASCII */
      if (!strcasecmp(author, story->kids[i]))
        exactmatch = YES;
    }
  }
  if (c != '\0') {
	/*** NULL ***/
    sprintf(story->paragraphid+strlen(story->paragraphid), "%c", c);
    get_paragraph(story);
    read_paragraph(story);
  }
}

/* read_previous(story) backs up a paragraph.  We've already
   ascertained that there is a paragraph to back up onto.     */

void read_previous(struct storystate *story)
{
  story->paragraphid[strlen(story->paragraphid)-1] = '\0';
  get_paragraph(story);
  read_paragraph(story);
}


/* read_paragraph reads the paragraph currently in memory. */
/* Formatting, if asked for, is applied.                   */

void read_paragraph(struct storystate *story)
{
  int i, j;
  int col;
  int tokenlen;
  char *token;

  col = 0;	
  if (!(story->writing) && FORMATTED) {
    for (i = 0; i < story->numlines; i++) {
      token = story->paragraph[i];
      while (*token != '\0') {
        if (isspace(*token))
          token++;
        else {
          if (!col)
            sendchat(";: ");
          tokenlen = 0;
          while (token[tokenlen] != '\0' && !isspace(token[tokenlen]))
            tokenlen++;
          if ((col + tokenlen) > FORMATMAX) {
            col = 0;
            sendchat("\n;: ");
          }
          for (j = 0; j < tokenlen; j++) {
           /* putchar(token[j]); */
           sprintf(mess,"%c",token[j]);
           sendchat(mess);
           }
          /* putchar(' '); */
          strcpy(mess," ");
          sendchat(mess);
          col += (tokenlen + 1);
          token += tokenlen;
        }
      }
    }
    sendchat("\n");
  }
  else {
    for (i = 0; i < story->numlines; i++) 
        sendchat(";: %s\n", story->paragraph[i]); 
  }
  if (story->numkids > 0) {
    if (story->numkids > 1 || strcasecmp(story->author, story->kids[0])) {
      sendchat(".say Following paragraphs by: ");
      for (i = 0; i < story->numkids; i++) {
        sendchat("%s", story->kids[i]); 
        if (i == story->numkids - 2)
          sendchat(" and ");
        else if (i < story->numkids - 2)
          sendchat(", ");
      }
      sendchat("\n"); 
    }
    sendchat(";pauses.\n");
  }
  else
    sendchat(";has finished the story.\n");
}

/* Finishstory frees up the story data structure */

struct storystate *finish_story(story)
struct storystate *story;
{
  int i;

  if (story != NULL) {
    for (i = 0; i < story->numlines; i++) {
      if (story->paragraph[i] != NULL)
        free(story->paragraph[i]);
    }
    for (i = 0; i < story->numkids; i++) {
      if (story->kids[i] != NULL)
        free(story->kids[i]);
    }
    free(story);
  }
  return NULL;
}

/* sync_bot sends a garbage message and waits for the HUH? to     */
/* come back. This is useful for synchronizing dialogue.          */

void sync_bot()
{

}

/* handle_page2 takes a paging line, and pages back a message giving
    StoryBot's location.                                           */

int handle_page2(char *pageline, char *playername, char *lowername)
{
  char playername2[MAXSTRLEN];
  int chance;

  strcpy(playername2,playername);

  if ((!strcmp(pageline, ""))  ||
      (!strcmp(pageline, "directions")) ||
      (!strcmp(pageline, "Directions")) ||
      (!strcmp(pageline, "Where are you?")) ||
      (!strcmp(pageline, "Where are you")) ||
      (!strcmp(pageline, "where are you?")) ||
      (!strcmp(pageline, "where are you")) ||
      (!strcmp(pageline, "Where")) ||
      (!strcmp(pageline, "where")) ) {
      sendchat(".tell %s Hello, %s! You can find me from the %s %s\n",
              playername2, playername2, MAIN_ROOM, directions);
     return 1;
    }
  else if (!strcmp(pageline,"shutdown") && !strcmp(lowername,ROOT_ID)) {
     sendchat(".tell %s Shutting down..\n",playername2);
     chance = rand() % 2;
     if (chance==1)
     sendchat(".shout See ya all! %s told me to go home..the nerve!\n",playername2);
     else
     sendchat(".shemote moons everyone!\n");

     sendchat(".shemote spins around in a daze, singing 'Daisy'.\n");
     sendchat(".shemote sings more and more slowly, till he finally shuts down.\n");
     sendchat("%s\n", DCONMSG);
     quit_robot();
    }
  else if (!strcmp(pageline,"reboot") && !strcmp(lowername,ROOT_ID)) {
     sendchat(".tell %s Rebooting..\n",playername2);
     sendchat(".shout See ya all in a bit! %s told me to reboot\n",playername2);
     sendchat(".shemote spins around in a daze, singing 'Daisy'.\n");
     sendchat(".shemote sings more and more slowly, till he finally shuts down.\n");
     sendchat("%s\n", DCONMSG);
     reboot_robot();
    }
 else {
  return 0;
  }

return 1;
}

void handle_page(char *pageline)
{
  char player[MAXSTRLEN];
  char c;

  player[0] = '\0';

  if ((sscanf(pageline, "%s", player) == 1)  ||
      (sscanf(pageline, "%s tells you%c", player,  &c) == 2)) {
    sprintf(mess,"Paged by %s", player);
    wlog(mess);
    if (strcasecmp(player, "Gardener")) {
      sendchat(".tell %s Hello, %s! You can find me from the %s %s\n",
              player, player, MAIN_ROOM, directions);
    }
  }
}


/* Getline takes a line from specified input and stuffs it into inbuf. */
/* Inbuf is NULL if EOF reached.                                       */

char *getline(inbuf, fp)
char *inbuf;
FILE *fp;
{
  char c;
  int i = 0;

  while ((c = getc(fp)) != '\n' &&  c != EOF && i < MAXSTRLEN - 2)
    inbuf[i++] = c;
  inbuf[i] = '\0';
  if (c == EOF)
    inbuf = NULL;
  else if ((strncmp(inbuf, "ifejf", 5)) ||
           (sscanf(inbuf, "%*s fakes you --%c", &c) ==1)) 
    /* handle_page(inbuf); */
  return(inbuf);

return(inbuf);
}

/* crash_n_burn handles panics.  We hope this never gets called.  */
void crash_n_burn(char *string)
{
   sendchat(".desc %s\n", SHUTMSG);
   sendchat(".go\n");
   sendchat(";suddenly starts to panic!\n");
   sendchat(";spins around in a daze, singing 'Daisy'.\n");
   sendchat(";sings more and more slowly, till it finally shuts down.\n");
   sendchat(";is broken.  Cygnus should hear about this.\n");
   sendchat("%s\n", DCONMSG);
   wlog(string);
   quit_robot();
}


/*** convert string to lower case ***/
void strtolower(str)
char *str;
{
while(*str)
  {
    *str=tolower(*str);
    str++;
  }
}

void midcpy(char *strf, char *strt, int fr, int to)
{
int f;
for (f=fr;f<=to;++f)
  {
   if (!strf[f])
     {
      strt[f-fr]='\0';
      return;
     }
   strt[f-fr]=strf[f];
  }
strt[f-fr]='\0';
}

/*** removes first word at front of string and moves rest down ***/
void remove_first(inpstr)
char *inpstr;
{
int newpos,oldpos;

newpos=0;  oldpos=0;
/* find first word */
while(inpstr[oldpos]==' ') {
        if (!inpstr[oldpos]) { inpstr[0]=0;  return; }
        oldpos++;
        }
/* find end of first word */
while(inpstr[oldpos]!=' ') {
        if (!inpstr[oldpos]) { inpstr[0]=0;  return; }
        oldpos++;
        }
/* find second word */
while(inpstr[oldpos]==' ') {
        if (!inpstr[oldpos]) { inpstr[0]=0;  return; }
        oldpos++;
        }
while(inpstr[oldpos]!=0)
        inpstr[newpos++]=inpstr[oldpos++];
inpstr[newpos]='\0';
}


void load_user()
{
}

void add_user()
{
}

void clear_all_users()
{
int u;

for (u=0;u<MAX_USERS;++u) {
	ustr[u].name[0]=0;
	ustr[u].say_name[0]=0;
	ustr[u].room[0]=0;
	ustr[u].vis=0;
	ustr[u].logon=0;
	}
}

void clear_user(int user)
{
ustr[user].name[0]=0;
ustr[user].say_name[0]=0;
ustr[user].room[0]=0;
ustr[user].vis=0;
ustr[user].logon=0;
}

int get_user_num(char *name)
{
int u;

strtolower(name);

for (u=0;u<MAX_USERS;++u) {
        if (!strcmp(ustr[u].name,name))
        return u;
	}

return(-1);

}

int find_free_slot()
{
int u;

   for (u=0;u<MAX_USERS;++u)
     {
      if (!strlen(ustr[u].name)) {
        return u;
	}
     }
  return -1;

}
