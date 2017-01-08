/*----------------------------------------------------------------------*/
/* Now Come on Over Here And Fuck Me Up The Ass - Ncohafmuta V 1.2.1    */
/*----------------------------------------------------------------------*/
/*  This code is a collection of software that originally started       */
/*  as a system called:                                                 */
/*                       IFORMS V 1.0                                   */
/*            Interactive FORum Multiplexor Software - (C) Deep         */
/*                 Last update 25/9/94                                  */
/*                                                                      */
/* As a result of extensive changes, it can no longer be considered     */
/* the same code                                                        */
/*                     -Cygnus (Anthony J. Biacco - Ncohafmuta Ent.)    */
/*                                                                      */
/* Legal note:  This code may NOT be freely distributed.  Doing so may  */
/*              be in violation of the US Munitions laws which cover    */
/*              exportation of encoding technology.                     */
/*----------------------------------------------------------------------*/

 /* SPECIAL PINWHEELS USER EXCEPTIONS IN THE CODE */
 /* llo doesn't get blinking */
	
 /* Major things to do:
                intertalker connectivity
                investigate using a database
                investigate using threads or async IO for connections
                investigate creating an email gateway
                multiline mail, board writes
                add command initialization from a file
                create standards for diffent term type 
                allocate structure dynamically
                break code up into multiple C files
                port code to Win95 and WinNT (in progress)
 */
 
/*--------------------------------------------------------------*/
/* NOTE: For AIX users the getrlimit is not supported by the OS */
/*                                                              */
/*   For sun users the cpp is not ansi standard so preprocessor */
/*   strings are not supported properly                         */
/*                                                              */
/*--------------------------------------------------------------*/
 
/*--------------------------------------------------------------*/
/* the debug switch make it easier to run this under a debugger */
/* it prevents deamonization and hardcodes the config directory */
/* to testfig when set to 1                                     */
/*--------------------------------------------------------------*/

#define DEBUG 0

#if defined(__sun__)
 /* SunOS of some sort */
  #if defined(__svr4__)
  /* Solaris, i.e. SunOS 5.x */
   #define SOL_SYS
 #else
  /* __sun__ && !__svr4__ (SunOS 4.x presumeably) */
  #define SUN4_SYS
 #endif /* __svr4__ */
#endif /* __sun__ */

#include <stdio.h>
#include <stdlib.h>  

#if defined(WIN32) && !defined(__CYGWIN32__)
#include <winsock.h>
#include <winnt.h>
#include <process.h>
#include <io.h>
#define EINTR WSAEINTR
#define EMFILE WSAEMFILE
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#define ENETUNREACH WSAENETUNREACH
#define ETIMEDOUT WSAETIMEDOUT
#define ECONNREFUSED WSAECONNREFUSED
#define EIO WSAEIO /* EIO is used in ident code, but what */
                   /* function is it an errno for         */
#define MAXHOSTNAMELEN 32
#define errno WSAGetLastError()
#define strerror(errnum) winerrstr(errnum)
#else
#include <sys/utsname.h>
#include <sys/socket.h>    /* for socket(), bind(), etc.. */
#include <netinet/in.h>    /* for sockaddr_in structure   */
#include <netdb.h>
#include <fcntl.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>
#endif

#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(NeXT)
#include <sys/dir.h>
#define dirent direct
#endif /* NeXT */

#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

/*
#if defined(__linux__)
#include <sys/un.h>
#endif
*/

#if defined(SOL_SYS)
#include <string.h>
#else
#include <strings.h>
#endif

#if defined(_AIX) || defined(AIX)
#include <sys/select.h>
#endif

/* Figure out OS's socket non-blocking option */
#if defined(FNONBLOCK)                     /* SYSV,AIX,SOLARIS,IRIX,HP-UX */
# define NBLOCK_CMD FNONBLOCK
#else
# if defined(O_NDELAY)                     /* BSD,LINUX,SOLARIS,IRIX */
#  define NBLOCK_CMD O_NDELAY
# else
#  if defined(FNDELAY)                     /* BSD,LINUX,SOLARIS,IRIX */
#   define NBLOCK_CMD FNDELAY
#  else
#   if defined(FNBIO)                      /* SYSV */
#    define NBLOCK_CMD FNBIO
#   else
#    if defined(FNONBIO)                   /* ? */
#     define NBLOCK_CMD FNONBIO
#    else
#     if defined(FNONBLK)                  /* IRIX */
#      define NBLOCK_CMD FNONBLK
#     else
#      define NBLOCK_CMD 0
#     endif
#    endif
#   endif
#  endif
# endif
#endif

/* If the OS doesn't define a max number of file      */
/* descriptors, define a normal number for select()   */
#if !defined(FD_SETSIZE)
#define FD_SETSIZE 256
#endif

/* BSD 4.2 and maybe some others need these defined */
#if !defined(FD_ZERO)
#define fd_set int
#define FD_ZERO(p)       (*p = 0)
#define FD_SET(n,p)      (*p |= (1<<(n)))
#define FD_CLR(n,p)      (*p &= ~(1<<(n)))
#define FD_ISSET(n,p)    (*p & (1<<(n)))
#endif

/* If not defined, i.e. unix system, define it so we cut */
/* down on ifdefs between win32 and *nix		 */
#if !defined(INVALID_SOCKET)
#define INVALID_SOCKET -1
#endif

/* glibc changes the type for arg3 of accept() and other socket calls */
/* from int to socklen_t, we want to eliminate some warnings	      */
#if !defined(__GLIBC__) || (__GLIBC__ < 2)
#define socklen_t	int
#endif

/* Definitions for authuser info from a remote identd */
struct timeval timeout={10, 0};
unsigned short auth_tcpport = 113;
int            auth_rtimeout = 5;

/*--------------------------------------------------------*/
/* Talker-related include files                           */
/*--------------------------------------------------------*/
#define _DEFINING_CONSTANTS
#include "authuser.h"
#include "text.h"
#include "constants.h"

/*---------------------------------------------------------*/
/* port definitions                                        */
/*---------------------------------------------------------*/

struct {
         int  total_connections_allowed;
         int  users;
         int  wizes;
         int  who;
         int  www;
         int  interconnect;
         int  cypherconnect;
        } range = 
          {  MAX_USERS + MAX_WHO_CONNECTS + MAX_WWW_CONNECTS + MAX_INTERCONNECTS + MAX_CYPHERCONNECTS,
             NUM_USERS,
             NUM_USERS + NUM_WIZES,
             NUM_USERS + NUM_WIZES + MAX_WHO_CONNECTS,
             NUM_USERS + NUM_WIZES + MAX_WHO_CONNECTS + MAX_WWW_CONNECTS,
             NUM_USERS + NUM_WIZES + MAX_WHO_CONNECTS + MAX_WWW_CONNECTS + MAX_INTERCONNECTS,
             NUM_USERS + NUM_WIZES + MAX_WHO_CONNECTS + MAX_WWW_CONNECTS + MAX_INTERCONNECTS + MAX_CYPHERCONNECTS
           };
           
int PORT;                  /* main login port for incoming   */

int listen_sock[4];        /* 32 bit listening sockets       */
fd_set    readmask;	   /* bitmap read set                */
int inter;                 /* inter talker connections       */
int cypher;                /* caller id port                 */

/*-----------------------------------------------*/
/* wiz only flag                                 */
/*-----------------------------------------------*/
#define WIZ_ONLY -4

/*--------------------------------------------------------*/
/* terminal control switch settings                       */
/*--------------------------------------------------------*/
#define NORM      0
#define BOLD      1

/* Initialize functions we need too            */
/* Almost all, including all command functions */
#include "protos.h"

/** Far too many bloody global declarations **/
int last_user;

/*** START OF COMMAND STRUCTURES ***/

/* User command list structure */
struct {
	char command[32];            /* command name                      */
	int  su_com;                 /* authority level                   */
	int  jump_vector;            /* the number to use for the command */
	int  type;                   /* the type of command for help */
                                     /* socials get a type of NONE */
	char cabbr[2];               /* the default abbr. for the command */
       } sys[] = {
		  {".abbrs",    1,      117,    INFO,""},
		  {".afk",      0,      75,     SETS,""},
                  {".auto",     3,      186,    MISC,""},
		  {".bafk",     0,      99,     SETS,""},
		  {".bop",      1,      172,    NONE,""},
		  {".bubble",   1,      121,    MISC,""},
		  {".call",     1,      124,    COMM,""},
		  {".creply",   1,      125,    COMM,"?"},
		  {".cbuff",    1,      42,     MISC,"*"},
		  {".chicken",  1,      213,    NONE,""},
                  {".chuckle",  1,      192,    NONE,""},
                  {".cough",    1,      193,    NONE,""},
		  {".cls",      0,      84,     MISC,""},
		  {".cmail",    1,      47,     MAIL,""},
		  {".csent",    1,      142,    MAIL,""},
                  {".dance",    1,      194,    NONE,""},
		  {".desc",     0,      37,     SETS,""},
                  {".doh",      1,      195,    NONE,""},
		  {".emote",    1,      11,     COMM,";"},
		  {".entpro",   0,      105,    SETS,""},
		  {".entermsg", 1,      116,    SETS,""},
		  {".examine",  0,      107,    INFO,""},
		  {".exitmsg",  1,      158,    SETS,""},
		  {".faq",      0,      106,    INFO,""},
		  {".fight",    1,      85,     MISC,""},
		  {".find",     1,      123,    INFO,""},
                  {".flirt",    1,      196,    NONE,""},
		  {".fmail",    1,      133,    MAIL,""},
		  {".go",       0,      7,      MOOV,""},
                  {".goose",    1,      197,    NONE,""},
                  {".grab",     1,      198,    NONE,""},
		  {".gripe",    1,      156,    MESG,""},
                  {".growl",    1,      199,    NONE,""},
		  {".guru",     1,      188,    MISC,""},
		  {".help",     0,      25,     INFO,""},
		  {".heartells",1,      62,     SETS,""},
                  {".hiss",     1,      200,    NONE,""},
		  {".home",     1,      159,    MOOV,""},
		  {".hug",      1,      164,    NONE,""},
		  {".ignore",   0,      5,      SETS,""},
		  {".igtells",  1,      61,     SETS,""},
                  {".insult",   1,      201,    NONE,""},
		  {".invite",   1,      10,     MOOV,""},
                  {".kick",     1,      202,    NONE,""},
		  {".kiss",     1,      170,    NONE,""},
		  {".last",     1,      119,    INFO,""},
		  {".laugh",    1,      165,    NONE,""},
		  {".lick",     1,      175,    NONE,""},
		  {".listen",   0,      4,      SETS,""},
		  {".log",      0,      137,    INFO,""},
                  {".lol",      1,      203,    NONE,""},
		  {".look",     0,      6,      MISC,"@"},
		  {".macros",   1,      44,     SETS,""},
		  {".map",      0,      126,    MISC,""},
		  {".mutter",   1,      129,    COMM,"#"},
		  {".nerf",     1,      160,    MISC,""},
		  {".news",     0,      27,     INFO,""},
		  {".noogie",   1,      214,    NONE,""},
		  {".password", 0,      67,     MISC,""},
		  {".poke",     1,      166,    NONE,""},
		  {".quit",     0,      0,      MISC,""},
		  {".quote",    1,      136,    SETS,""},
		  {".read",     0,      15,     MESG,""},
		  {".reload",   1,      162,    MISC,""},
		  {".review",   1,      24,     MISC,"<"},
		  {".rmail",    1,      45,     MAIL,""},
		  {".rooms",    1,      12,     INFO,""},
		  {".rsent",    1,      143,    MAIL,""},
		  {".rules",    0,      212,    INFO,""},
		  {".say",      0,      90,     COMM,"\""},
		  {".schedule", 0,      148,    INFO,""},
		  {".semote",   1,      69,     COMM,"/"},
		  {".set",      0,      79,     SETS,""},
		  {".shout",    1,      2,      COMM,"!"},
                  {".shake",    1,      204,    NONE,""},
                  {".shove",    1,      205,    NONE,""},
		  {".sing",     1,      151,    MISC,"%"},
                  {".slap",     1,      206,    NONE,""},
		  {".smail",    1,      46,     MAIL,""},
		  {".smile",    1,      176,    NONE,""},
		  {".smirk",    1,      174,    NONE,""},
		  {".socials",  1,      163,    COMM,""},
		  {".sos",      0,      104,    MISC,""},
		  {".sthink",   1,      122,    COMM,""},
		  {".suggest",  1,      130,    MESG,""},
		  {".suicide",  0,      182,    MISC,""},
		  {".swho",     0,      118,    INFO,""},
		  {".tell",     0,      3,      COMM,","},
		  {".tackle",   1,      173,    NONE,""},
		  {".talker",   1,      120,    MISC,""},
		  {".think",    1,      103,    COMM,""},
		  {".thwap",    1,      171,    NONE,""},
		  {".tickle",   1,      169,    NONE,""},
		  {".time",     0,      76,     MISC,""},
		  {".to",       1,      183,    COMM,"$"},
		  {".version",  0,      111,    MISC,""},
		  {".who",      0,      1,      INFO,"{"},
		  {".wedgie",   1,      43,     NONE,""},
                  {".whine",    1,      207,    NONE,""},
                  {".wink",     1,      208,    NONE,""},
		  {".wlist",    0,      147,    INFO,""},
                  {".woohoo",   1,      209,    NONE,""},
		  {".write",    1,      14,     MESG,""},
		  {".alert",    1,      146,    SETS,""},
		  {".fail",     1,      127,    SETS,""},
		  {".femote",   1,      113,    COMM,""},
		  {".friends",  1,      191,    INFO,""},
		  {".ftell",    1,      161,    COMM,""},
		  {".gag",      1,      145,    SETS,""},
		  {".greet",    1,      40,     MISC,""},
		  {".guess",    1,      218,    MISC,""},
		  {".hangman",  1,      219,    MISC,""},
		  {".knock",    1,      13,     MOOV,""},
		  {".meter",    1,      91,     MISC,""},
		  {".preview",  1,      66,     INFO,""},
		  {".private",  1,      8,      SETS,""},
		  {".public",   1,      9,      SETS,""},
		  {".search",   1,      23,     MESG,""},
		  {".shemote",  1,      112,    COMM,"&"},
		  {".shthink",  1,      181,    COMM,""},
		  {".succ",     1,      128,    SETS,""},
		  {".topic",    1,      18,     MISC,""},
		  {".ttt",      1,      217,    MISC,""},
		  {".ustat",    1,      78,     INFO,""},
		  {".vote",     1,      168,    MISC,""},
		  {".with",     1,      102,    INFO,""},
		  {".wizards",  0,      101,    INFO,""},
		  {".beep",     1,      74,     COMM,""},
		  {".echo",     1,      36,     COMM,""},
		  {".follow",   1,      72,     MOOV,""},
		  {".ptell",    1,      71,     COMM,""},
		  {".ranks",    1,      58,     MISC,""},
		  {".show",     2,      108,    MISC,"\'"},
		  {".anchor",   2,      135,    MOOV,""},
		  {".arrest",   2,      41,     MOOV,""},
		  {".unarrest", 2,      190,    MOOV,""},
		  {".memory",   2,      177,    INFO,""},
                  {".gagcomm",  2,      184,    COMM,""},
		  {".muzzle",   2,      51,     COMM,""},
		  {".system",   2,      28,     INFO,""},
		  {".unmuzzle", 2,      52,     COMM,""},
		  {".wipe",     2,      16,     MESG,""},
		  {".bcast",    2,      26,     COMM,""},
		  {".bring",    2,      54,     MOOV,""},
		  {".clist",    2,      167,    INFO,""},
		  {".hide",     2,      56,     SETS,""},
		  {".monitor",  2,      70,     SETS,""},
		  {".move",     2,      29,     MOOV,""},
		  {".picture",  1,      65,     MISC,""},
		  {".cline",    3,      109,    MISC,""},
		  {".wiztell",    2,    82,     COMM,">"},
		  {".demote",     3,    50,     SETS,""},
		  {".descroom",   3,    155,    SETS,""},
		  {".finger",     3,    140,    INFO,""},
		  {".force",      3,    153,    SETS,""},
                  {".frog",       2,    185,    COMM,""},
		  {".grant",      3,    216,    SETS,""},
		  {".whois",      2,    141,    INFO,""},
		  {".kill",       2,    21,     INFO,""},
		  {".nslookup",   2,    139,    INFO,""},
		  {".permission", 3,    68,     SETS,""},
		  {".promote",    3,    49,     SETS,""},
		  {".realuser",   3,    138,    INFO,""},
		  {".samesite",   2,    144,    INFO,""},
		  {".site",       2,    80,     INFO,""},
		  {".swipe",      3,    134,    MESG,""},
		  {".addatmos",   3,    178,    SETS,""},
		  {".delatmos",   3,    179,    SETS,""},
		  {".listatmos",  3,    180,    SETS,""},
		  {".atmos",      3,    35,     SETS,""},
		  {".bannew",     3,    149,    BANS,""},
		  {".banname",    3,    210,    BANS,""},
		  {".expire",     3,    152,    SETS,""},
		  {".nuke",       3,    83,     MISC,""},
		  {".pcreate",    3,    211,    MISC,""},
		  {".restrict",   3,    59,     BANS,""},
		  {".resolve",    3,    86,     MISC,""},
		  {".revoke",     3,    215,    SETS,""},
		  {".unbannew",   3,    150,    BANS,""},
		  {".unrestrict", 3,    60,     BANS,""},
		  {".users",      3,    77,     MISC,""},
		  {".wlog",       1,    189,    MESG,""},
		  {".wnote",      2,    97,     MESG,""},
		  {".xcomm",      3,    100,    COMM,""},
		  {".suname",     3,    114,    MISC,""},
		  {".supass",     3,    115,    MISC,""},
		  {".allow_new",  4,    38,     BANS,""},
		  {".bbcast",     4,    110,    COMM,""},
		  {".close",      4,    30,     MISC,""},
		  {".gwipe",      4,    157,    MESG,""},
		  {".open",       4,    31,     MISC,""},
		  {".quota",      4,    95,     SETS,""},
		  {".readlog",    3,    154,    INFO,""},
		  {".reinit",     4,    73,     MISC,""},
		  {".shutdown",   4,    22,     MISC,""},
		  {".wwipe",      4,    98,     MESG,""},
		  /*-----------------------------------------------*/
		  /* this item marks the end of the list, do not   */
		  /* remove it                                     */
		  /*-----------------------------------------------*/
		  {"<EOL>",       -1,   -1,     -1,     ""}
		};

/* Bot command list structure */
struct {
	char command[32];            /* command name                      */
	int  jump_vector;            /* the number to use for the command */
	int  type;                   /* the type of command for help */
                                     /* socials get a type of NONE */
       } botsys[] = {
		  {"_who",    0,    INFO},
		  /*-----------------------------------------------*/
		  /* this item marks the end of the list, do not   */
		  /* remove it                                     */
		  /*-----------------------------------------------*/
		  {"<EOL>",	-1,	-1}
		};

/*** END OF COMMAND STRUCTURES ***/

/*** GLOBAL DELCARATIONS ***/

/** Big Letter array map **/
int biglet[26][5][5] =
     {{{0,1,1,1,0},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1}},
      {{1,1,1,1,0},{1,0,0,0,1},{1,1,1,1,0},{1,0,0,0,1},{1,1,1,1,0}},
      {{0,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{0,1,1,1,1}},
      {{1,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,0}},
      {{1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,0},{1,1,1,1,1}},
      {{1,1,1,1,1},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,0},{1,0,0,0,0}},
      {{0,1,1,1,0},{1,0,0,0,0},{1,0,1,1,0},{1,0,0,0,1},{0,1,1,1,0}},
      {{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1}},
      {{0,1,1,1,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,1,1,1,0}},
      {{0,0,0,0,1},{0,0,0,0,1},{0,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}},
      {{1,0,0,0,1},{1,0,0,1,0},{1,0,1,0,0},{1,0,0,1,0},{1,0,0,0,1}},
      {{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,1}},
      {{1,0,0,0,1},{1,1,0,1,1},{1,0,1,0,1},{1,0,0,0,1},{1,0,0,0,1}},
      {{1,0,0,0,1},{1,1,0,0,1},{1,0,1,0,1},{1,0,0,1,1},{1,0,0,0,1}},
      {{0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}},
      {{1,1,1,1,0},{1,0,0,0,1},{1,1,1,1,0},{1,0,0,0,0},{1,0,0,0,0}},
      {{0,1,1,1,0},{1,0,0,0,1},{1,0,1,0,1},{1,0,0,1,1},{0,1,1,1,0}},
      {{1,1,1,1,0},{1,0,0,0,1},{1,1,1,1,0},{1,0,0,1,0},{1,0,0,0,1}},
      {{0,1,1,1,1},{1,0,0,0,0},{0,1,1,1,0},{0,0,0,0,1},{1,1,1,1,0}},
      {{1,1,1,1,1},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0}},
      {{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1}},
      {{1,0,0,0,1},{1,0,0,0,1},{0,1,0,1,0},{0,1,0,1,0},{0,0,1,0,0}},
      {{1,0,0,0,1},{1,0,0,0,1},{1,0,1,0,1},{1,1,0,1,1},{1,0,0,0,1}},
      {{1,0,0,0,1},{0,1,0,1,0},{0,0,1,0,0},{0,1,0,1,0},{1,0,0,0,1}},
      {{1,0,0,0,1},{0,1,0,1,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0}},
      {{1,1,1,1,1},{0,0,0,1,0},{0,0,1,0,0},{0,1,0,0,0},{1,1,1,1,1}}};

char *syserror="Sorry - a system error has occured";

char area_nochange[MAX_AREAS]; 
char mess[ARR_SIZE+25];    /* functions use mess to send output   */ 
char t_mess[ARR_SIZE+25];  /* functions use t_mess as a buffer    */ 
char l_mess[ARR_SIZE+25];  /* log functions use l_mess as buffer  */ 
char conv[MAX_AREAS][NUM_LINES][MAX_LINE_LEN+1]; /* stores lines of conversation in room */
char bt_conv[NUM_LINES][MAX_LINE_LEN+1]; /* stores lines of conversation in wiztell buffer */
char sh_conv[NUM_LINES][MAX_LINE_LEN+1]; /* store review shouts */
char datadir[255];	/* config directory                       */
char thishost[101];	/* FQDN were running on                   */
char thisos[101];       /* operating system were running on       */
char thisprog[255];	/* the binary the program is run as	  */
char web_opts[11][64];	/* web configuration options from file    */

time_t start_time;	/* startup time                           */

int NUM_AREAS;		/* number of areas defined in config file */
int num_of_users=0;	/* total number of users online           */
int MESS_LIFE=0;	/* message lifetime in days               */

int noprompt;		/* talker waiting for user input?         */
int signl;
int atmos_on;		/* all room atmospherics on?              */
int syslog_on;		/* are we logging system stuff to a file  */
int allow_new;		/* can new users be created?              */
int average_tells;	/* average tells                          */

int tells;		/* tells in defined time period           */
int commands;		/* total commands in defined time period  */
int says;		/* says in defined time period            */
int says_running;
int tells_running;
int commands_running;

int shutd= -1;		/* talker waiting for confirm on a shutdown?    */
int down_time=0;	/* countdown to an auto shutdown or auto reboot */
int delete_sent=0;

int sys_access=1;	/* is the system open for user connections?      */
int wiz_access=1;	/* is the system open for wizard connections?    */
int who_access=1;	/* is the system open for external who listings? */
int www_access=1;	/* is the system open for external web requests? */

int checked=0;		/* see if messages have been checked at midnight */
int bt_count;		/* wiztell count in the buffer                   */
int sh_count;		/* shout count in the buffer                     */
int treboot=0;		/* is an auto-reboot started?                    */
int autopromote = AUTO_PROMOTE; /* allowing users to promote themselves? */
int autonuke    = NUKE_NOSET;	/* nuking users on quit without a desc.? */
int autoexpire  = AUTO_EXPIRE;	/* expiring users at midnight on auto?   */
int bot         = -5;           /* this will hold the bots user number   */
int new_room;			/* room new users log into               */

/*** END OF GLOBAL DELCARATIONS ***/

/*** START OF MISC STRUCTURES ***/

/* Terminal definitions for future implementation */
struct {
	char name[10];
	char hion[10];
	char hioff[10];
	char cls[20];
	} terms[] = {
		{"xterm", "\033[1m", "\033[m", "\033[H\033[2J"},
		{"vt220", "\033[1m", "\033[m", "\033[H\033[J"},
		{"vt100", "\033[1m", "\033[m", "50\033[;H\0332J"},
		{"vt102", "\033[1m", "\033[m", "50\033["},
		{"ansi", "\033[1m", "\033[0m", "50\033[;H\0332J"},
		{"wyse-30", "\033G4", "\033G0", ""},
		{"tvi912", "\033l", "\033m", "\032"},
		{"sun", "\033[1m", "\033[m", "\014"},
   		{"adm", "\033)", "\033(", "1\032"},
 	  	{"hp2392", "\033&dB", "\033&d@", "\033H\033J"},
   		{"", "", "", ""}
		};

/*** END OF MISC STRUCTURES ***/

/*** START OF FUNCTIONS ***/
/****************************************************************************
     Main function - 
     Sets up network data, signals, accepts user input and acts as 
     the switching center for speach output.
*****************************************************************************/ 
int main(int argc, char **argv)
{
struct sockaddr_in acc_addr; /* this is the socket for accepting */

int       as=0;             /* as = accept socket - 32 bit */
socklen_t       size;
int       user;
int       com_num;
int       new_user;	     /* new user's index number */
int       imotd=0;	     /* random MOTD placer */
int       i=0;
int       fd;                /* file desc. for stdin,out,err    */

char      port;
char      filename[1024];
char      inpstr[ARR_SIZE];
char	  wcd1[FILE_NAME_LEN+10];


unsigned  long ip_address;   /* connector's net address actually  */
#if defined(WIN32) && !defined(__CYGWIN32__)
unsigned  long arg = 1;      /* for a WIN32 nonblocking set       */
#endif

struct timeval sel_timeout;  /* how much time to give select() */

#if defined(WIN32) && !defined(__CYGWIN32__)
WSADATA wsaData; /* WinSock details */
DWORD dwThreadId, dwThrdParam = 1;
DWORD alarm_thread(LPDWORD lpdwParam);
OSVERSIONINFO myinfo;
unsigned short wVersionRequested = MAKEWORD(1, 1); /* v1.1 requested */
int err; /* error status */
#else
struct utsname uname_info;
#endif

/* It's showtime!!!!! */
#if defined(WIN32) && !defined(__CYGWIN32__)
sprintf(mess,"WIN%s - WIN32 VERSION -",VERSION);
SetConsoleTitle(mess);
puts(mess);
#else
puts(VERSION);
#endif

puts("Copyrighted software                      ");
puts("                                          ");

#if defined(WIN32) && !defined(__CYGWIN32__)
    /* Need to include library: wsock32.lib for Windows Sockets */
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
      printf("Error %i %s on WSAStartup\n", errno, strerror(errno));
      WSACleanup();
      exit(1);
    }

     /* Check the WinSock version information */
     if ( (LOBYTE(wsaData.wVersion) != LOBYTE(wVersionRequested)) ||
        (HIBYTE(wsaData.wVersion) != HIBYTE(wVersionRequested)) ) {
      printf("WinSock version %d found doesn't live up to WinSock version %d needed\n",wsaData.wHighVersion,wVersionRequested);
      WSACleanup();
      exit(1);
    }

     /* Print WinSock info */
     printf("%s\n%s\n",wsaData.szDescription,wsaData.szSystemStatus);
     printf("Max sockets program can open: %d\n",wsaData.iMaxSockets);
#endif                          /* WIN32 */


if (DEBUG)
  {
   puts("NOTE: Running in debug mode, using testfig for configuration info.");
   strcpy(datadir,"testfig");
  }
 else if (argc==1)       /* check comline input */
    {
     puts("NOTE: Running with config data directory");
     strcpy(datadir,"config");
    }
 else
    {
     strcpy(datadir,argv[1]);
    }

/* Copy the program binary name to memory */
strcpy(thisprog,argv[0]);

/* Leave a buffer of 3 fds for open files */
if (range.total_connections_allowed > FD_SETSIZE-3) {
 printf("*** Max connections defined higher than number of file ***\n");
 printf("*** descriptors allowed by host machine..lower them    ***\n");
 printf("Connections tried to allocate: %d\n",
        range.total_connections_allowed);
 printf("Connections allowed to allocate: %d\n",
        FD_SETSIZE-3);
#if defined(WIN32) && !defined(__CYGWIN32__)
	WSACleanup();
#endif
 exit(0);
 }

/* SET TALKER TIME TO THE TIMEZONE WE WANT */
/* Get the current working directory so we can reference */
/* to the absolute path of the TZ info                   */
getcwd(wcd1,FILE_NAME_LEN);
strcat(wcd1,"/tzinfo");

if (!strcmp(TZONE,"localtime")) {
#if defined(__FreeBSD__) || defined(__NetBSD__)
putenv("TZ=:/etc/localtime");
#else
putenv("TZ=localtime");      
#endif  
}
else {
sprintf(mess,"TZ=:%s/%s",wcd1,TZONE);
putenv(mess);
}
#if !defined(__CYGWIN32__)
tzset();
#endif

printf("Checking abbreviation count..\n");
abbrcount();

/* read system data */
printf("Reading area data from dir. %s ...\n",datadir);
read_init_data();

/* read exempt users */
printf("Reading user exempt data from file  %s ...\n",EXEMFILE);
read_exem_data();

/* read banned names */
printf("Reading banned user names from file  %s ...\n",NBANFILE);
read_nban_data();

/*---------------------*/
/* Initialize sockets  */
/*---------------------*/
printf("Initializing sockets...\n");
make_sockets();

/* Initialize functions */
init_user_struct();
init_area_struct();
init_misc_struct();

/* Reset meter variables */
tells         = 0;
commands      = 1;
says          = 0;
says_running  = 0;
tells_running = 0;

puts("Resetting login numbers...");
system_stats.quota              = MAX_NEW_PER_DAY;
system_stats.logins_today       = 0;
system_stats.logins_since_start = 0;
system_stats.new_since_start    = 0;
system_stats.new_users_today    = 0;
system_stats.tot_users          = 0;
system_stats.tot_expired        = 0;
system_stats.cache_hits         = 0;
system_stats.cache_misses       = 0;

puts("Checking for out of date messages...");
check_mess(1);

puts("Counting users in user directory...");
tot_user_check(1);

puts("Counting messages...");
messcount();

/* Clear fights */
reset_chal(0,"");

puts("** System running **");

/*----------------------------*/
/* dissociate from tty device */
/*----------------------------*/
if (!DEBUG) 
  {

   /*-------------------------------------------------*/
   /* Redirect stdin, stdout, and stderr to /dev/null */
   /*-------------------------------------------------*/
    fd = open("/dev/null",O_RDWR);
    if (fd < 0) {
      perror("Unable to open /dev/null");
      exit (-1);
      }

    CLOSE(0);
    CLOSE(1);
    CLOSE(2);

    if (fd != 0)
    {
     dup2(fd,0);
     CLOSE(fd);
    }

    dup2(0,1);
    dup2(0,2);

#if !defined(WIN32) || defined(__CYGWIN32__)

   /*-------------------------------------------------------*/
   /* Fork the process away from the foreground space       */
   /*-------------------------------------------------------*/  
   switch(fork()) 
      {
        case -1:    print_to_syslog("FORK 1 FAILED!\n"); 
	            exit(1);
#if defined(__OpenBSD__)
        case 0:     setpgrp(0,0);
#else
        case 0:     setpgrp();
#endif
		    break;
        default:    sleep(1);
                    exit(0);  /* kill parent */
      }

   /*-------------------------------------------------------*/
   /* Fork the process away from the tty terminal           */
   /*-------------------------------------------------------*/  
   switch(fork()) 
      {
        case -1:    print_to_syslog("FORK 2 FAILED!\n"); 
	            exit(2);
        case 0:     break;  /* child becomes server */
        default:    sleep(1);
                    exit(0);  /* kill parent */
      }

	/* Get OS version */
	uname(&uname_info);
	strcpy(thisos,uname_info.sysname);
	strcat(thisos," ");
	strcat(thisos,uname_info.release);

	/*------------------------------------------------*/
	/* set up alarm, signals and signal handlers      */
	/*------------------------------------------------*/
	reset_alarm();
#else

/* Start timer thread */
hThread = CreateThread(
        NULL, /* no security attributes */
        0,    /* use default stack size */
        (LPTHREAD_START_ROUTINE) alarm_thread, /* thread function */
        &dwThrdParam, /* argument to thread function   */
        0,            /* use default creation flags    */
        &dwThreadId); /* returns the thread identifier */

/* Check the return value for success. */
if (hThread == NULL) {
   WSACleanup();
   printf("Can't create timer thread! Exiting!\n");
   exit(7);
   }

/* Get Windows version */
myinfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
GetVersionEx(&myinfo);
sprintf(thisos,"Windows v%d.%d",myinfo.dwMajorVersion,myinfo.dwMinorVersion);

#endif

/* Log startup */
sysud(1,0);

  } /* end of if !debug */

/* Get local hostname */
 if (gethostname(thishost,100) != 0) {
    printf("\n   Cannot get local hostname!\n");
    strcpy(thishost,"");
   }
#if !defined(WIN32) || defined(__CYGWIN32__)
mess[0]=0;
if (!getdomainname(mess,80)) {
  if (strcmp(mess,"(none)") && strlen(mess)) {
  /* we found a domain name */
  if (strstr(thishost,".")) {
   /* we already have a domain on the end, skip this */
   }
  else {
   /* we only have a hostname, add the domain we found */
    if (strlen(thishost))
     strcat(thishost,".");
    strcat(thishost,mess);
    if (thishost[strlen(thishost)-1]=='.')
	midcpy(thishost,thishost,0,strlen(thishost)-2);
   }
  }
  else {
  /* we didn't find a domain */
  if (strstr(thishost,".")) {
   /* we already have a domain on the end, skip this */
   }
  else {
   /* we only have a hostname, add the default domain from constants.h */
   if (strlen(thishost))
    strcat(thishost,".");
   strcat(thishost,DEF_DOMAIN);
   }
  }
} /* end of if getdomainname */
else {
   /* getdomainname failed, put the default on the end */
   if (strlen(thishost))
    strcat(thishost,".");
   strcat(thishost,DEF_DOMAIN);
}

mess[0]=0;
#endif

sprintf(mess,
"echo \"SYSTEM_NAME: %s\nPID: %u\nSTATUS: UP\nROOT_ID: %s\nVERSION: %s\nHOSTNAME: %s\nMAIN_PORT: %d\nWIZ_PORT: %d\nWHO_PORT: %d\nWWW_PORT: %d\nSYSTEM_EMAIL: %s\nNUM_USERS: %ld\nNEWUSER_STATUS: %d\nTHEME: %s\nEIGHTTPLUS: %d\nEND:\n\" > .tinfo",
SYSTEM_NAME,(unsigned int)getpid(),ROOT_ID,VERSION,thishost,PORT,
PORT+WIZ_OFFSET,PORT+WHO_OFFSET,PORT+WWW_OFFSET,SYSTEM_EMAIL,
system_stats.tot_users,allow_new,TTHEME,EIGHTTPLUS);
system(mess);
system("mail tinfo@ncohafmuta.com < .tinfo");

/* Initalize signal handlers                           */  
init_signals();

/*-----------------------------------------------------*/
/* clear the btell and shout buffers                   */
/*-----------------------------------------------------*/

cbtbuff();
cshbuff();

/*--------------------------*/
/**** Main program loop *****/
/*--------------------------*/

LOOP_FOREVER
   {
	noprompt=0;  
	signl=0;
	
	/*-------------------------------------------*/
	/* Set up bitmap readmask by clearing it out */
	/*-------------------------------------------*/
	FD_ZERO(&readmask);

	/*----------------------------------*/
	/* Set up timeout for select()      */
	/* what is really a good number???? */
	/*----------------------------------*/
	sel_timeout.tv_sec  = 0;   /* number of seconds      */
	sel_timeout.tv_usec = 0;   /* number of microseconds */

        /* Is there a who port socket in use? */
        /* If so, add it to our mask          */
	for (user = 0; user < MAX_WHO_CONNECTS; ++user)
	  {
            if (whoport[user].sock != -1)
             FD_SET(whoport[user].sock,&readmask);
	  }
	user=0;

        /* Is there a www port socket in use? */
        /* If so, add it to our mask          */
	for (user = 0; user < MAX_WWW_CONNECTS; ++user)
	  {
            if (wwwport[user].sock != -1)
             FD_SET(wwwport[user].sock,&readmask);
	  }
	user=0;

	for (user = 0; user < MAX_USERS; ++user) 
	  {
	   /* A user slot is defined, but there is no */
	   /* user filling the slot yet               */
           /* if (ustr[user].area == -1 && !ustr[user].logging_in) */
           /* continue; */

	   /* If a connection exists, add the online users socket */
	   /* to the read mask set                                */
	   if (ustr[user].sock != -1)
             FD_SET(ustr[user].sock,&readmask);
          }

       for (i=0;i<4;++i) {
	/* 1 = wiz 2 = who 3 = www */
	if (i==1) { if (WIZ_OFFSET==0) continue; }
	else if (i==2) { if (WHO_OFFSET==0) continue; }
	else if (i==3) { if (WWW_OFFSET==0) continue; }
	/* Add the listening sockets to the read mask set */
	FD_SET(listen_sock[i],&readmask);
        }

	/*--------------------------------------------------------------*/
	/* Wait for input on the ports                                  */
	/*                                                              */
        /* We declare the args as (void*) because HP/UX for example has */
        /* a select() prototype declaring the args as (int*) rather     */
        /* than (fd_set*), POSIX or no POSIX. By casting to (void*) we  */
        /* avoid compiletime warnings about these args                  */
	/*--------------------------------------------------------------*/

#if defined(WIN32) && !defined(__CYGWIN32__)
	if (select(FD_SETSIZE, &readmask, 0, 0, 0) == SOCKET_ERROR) {
#else
	if (select(FD_SETSIZE, (void *) &readmask, (void *) 0, (void *) 0, 0) == -1) {
#endif
	 if (errno != EINTR) {
	 	sprintf(mess,"Select failed: %s\n",strerror(errno));
		print_to_syslog(mess);
		shutdown_error(9);
	   }
	 else continue;
	}

	if (signl) continue; 

        port = ' ';

	/*---------------------------------------*/
	/* Check for connection to who socket    */
	/*---------------------------------------*/
	if (WHO_OFFSET != 0) {
	if (FD_ISSET(listen_sock[2],&readmask)) 
	  {
           size=sizeof(acc_addr);
           if ( ( as = accept(listen_sock[2], (struct sockaddr *)&acc_addr, &size) ) == INVALID_SOCKET )
             {
	       /* we can not open a new file descriptor */
               FD_CLR(listen_sock[2],&readmask);
	       sprintf(mess,"ERROR -> create who socket : %d %s\n",errno,strerror(errno));
	       print_to_syslog(mess);
	      if (errno==EINTR || errno==EAGAIN)
		continue;
	      else
               shutdown_error(1);
             }
            else
             {
              port='3';
              /* Set socket to non-blocking */
#if defined(WIN32) && !defined(__CYGWIN32__)
	      if (ioctlsocket(as, FIONBIO, &arg) == -1) {
#else
              if (fcntl(as,F_SETFL,NBLOCK_CMD)== -1) {
#endif
	       sprintf(mess,"ERROR -> set who socket to non-blocking : %s\n",strerror(errno));
	       print_to_syslog(mess);
               shutdown_error(2);
              }

             /*---------------------------------*/
	     /* Get who port connect and log it */
	     /*---------------------------------*/

              ip_address = acc_addr.sin_addr.s_addr;
              if ( (new_user = find_free_slot(port) ) == -1 ) {
	         while (CLOSE(as) == -1 && errno == EINTR)
			; /* empty while */
                 continue;
                }

              whoport[new_user].sock=as;

              if (log_misc_connect(new_user,ip_address,1) == -1) {
                 free_sock(new_user,port);
                 continue;
                }
              if (who_access) {
		/* Send out who list to person or remote who connected to who port */
		external_who(whoport[new_user].sock);
		}
              else {
	       strcpy(mess,WHO_CLOSED);
	       write_it(whoport[new_user].sock,mess);
               }
              free_sock(new_user,port);
              continue;
             } /* end of accept else */
           } /* end of FD_ISSET */
	} /* end of WHO_OFFSET if */

	/*--------------------------------------------*/
	/* Check for connection to mini www socket    */
	/*--------------------------------------------*/
	if (WWW_OFFSET != 0) {
	if (FD_ISSET(listen_sock[3], &readmask)) 
	  {
           size=sizeof(acc_addr);
           if ( ( as = accept(listen_sock[3], (struct sockaddr *)&acc_addr, &size) ) == INVALID_SOCKET )
             {
	       /* we can not open a new file descriptor */
               FD_CLR(listen_sock[3],&readmask);
	       sprintf(mess,"ERROR -> create www socket : %d %s\n",errno,strerror(errno));
	       print_to_syslog(mess);
	      if (errno==EINTR || errno==EAGAIN)
		continue;
	      else
               shutdown_error(3);
             }
            else
             {
              port='4';

              /* Set socket to non-blocking */
#if defined(WIN32) && !defined(__CYGWIN32__)
              if (ioctlsocket(as, FIONBIO, &arg) == -1) {
#else
              if (fcntl(as,F_SETFL,NBLOCK_CMD)== -1) {
#endif
	       sprintf(mess,"ERROR -> set www socket to non-blocking : %s\n",strerror(errno));
	       print_to_syslog(mess);
               shutdown_error(4);
              }

             /*---------------------------------*/
	     /* Get www port connect and log it */
	     /*---------------------------------*/

              ip_address = acc_addr.sin_addr.s_addr;
              if ( (new_user = find_free_slot(port) ) == -1 ) {
	         while (CLOSE(as) == -1 && errno == EINTR)
			; /* empty while */
                 continue;
                }

              wwwport[new_user].sock=as;

              if (log_misc_connect(new_user,ip_address,2) == -1) {
                 free_sock(new_user,port);
                 continue;
                }
              if (!www_access) {
	       strcpy(mess,WWW_CLOSED);
	       write_it(wwwport[new_user].sock,mess);
               free_sock(new_user,port);
               continue;
               }
             } /* end of accept else */
           } /* end of FD_ISSET */
	} /* end of WWW_OFFSET if */

	/*---------------------------------------*/
	/* Check for connection to listen socket */
	/*---------------------------------------*/

	if (FD_ISSET(listen_sock[0],&readmask) || FD_ISSET(listen_sock[1],&readmask)) 
	  {
           if ( FD_ISSET(listen_sock[0],&readmask))
             {
               size=sizeof(acc_addr);
               if ( ( as = accept(listen_sock[0], (struct sockaddr *)&acc_addr,&size) ) == INVALID_SOCKET )
                 {
	       /* we can not open a new file descriptor */
               FD_CLR(listen_sock[0],&readmask);
	       sprintf(mess,"ERROR -> accept can't create user socket : %d %s\n",errno,strerror(errno));
	       print_to_syslog(mess);
		  if (errno==EINTR || errno==EAGAIN)
		   continue;
		  else
                   shutdown_error(5);
                }
              port = '1';
            }

	if (WIZ_OFFSET != 0) {        
           if ( FD_ISSET(listen_sock[1],&readmask))
             {
               size=sizeof(acc_addr);
               if ( ( as = accept(listen_sock[1], (struct sockaddr *)&acc_addr,&size) ) == INVALID_SOCKET )
                 {
	       /* we can not open a new file descriptor */
               FD_CLR(listen_sock[1],&readmask);
	       sprintf(mess,"ERROR -> accept can't create wiz socket : %d %s\n",errno,strerror(errno));
	       print_to_syslog(mess);
		  if (errno==EINTR || errno==EAGAIN)
		   continue;
		  else
                   shutdown_error(6);
                 }
               port = '2';
             }
	} /* end of WIZ_OFFSET if */
             
              /* Set socket to non-blocking */
#if defined(WIN32) && !defined(__CYGWIN32__)
              if (ioctlsocket(as, FIONBIO, &arg) == -1) {
#else
              if (fcntl(as,F_SETFL,NBLOCK_CMD)== -1) {
#endif
	       sprintf(mess,"ERROR -> set user or wiz socket to non-blocking : %s\n",strerror(errno));
	       print_to_syslog(mess);
               shutdown_error(7);
              }

	   
	   /*----------------------------------------------*/
	   /* No system access is allowed                  */
	   /*----------------------------------------------*/

           if (port == '1') {
	   if (!sys_access) 
	     {
	       strcpy(mess,SYS_CLOSED);
	       S_WRITE(as,mess,strlen(mess));
	         while (CLOSE(as) == -1 && errno == EINTR)
			; /* empty while */
	       continue;
	     }
           }
          else if (port == '2') {
	   if (!wiz_access) 
	     {
	       strcpy(mess,WIZ_CLOSED);
	       S_WRITE(as,mess,strlen(mess));
	         while (CLOSE(as) == -1 && errno == EINTR)
			; /* empty while */
	       continue;
	     }
           }
			
            if ( (new_user = find_free_slot(port) ) == -1 ) 
              {                      
		sprintf(mess,"%s",SYSFULLFILE);
		cat_to_sock(mess,as);
	         while (CLOSE(as) == -1 && errno == EINTR)
			; /* empty while */
		continue;
	      }
	     
	     ustr[new_user].sock           = as;		
	     ustr[new_user].last_input     = time(0);
	     ustr[new_user].logging_in     = 3;
             ustr[new_user].attleft        = 3;
	     ustr[new_user].warning_given  = 0;
	     ustr[new_user].afk            = 0;
	     ustr[new_user].lockafk        = 0;
	     ustr[new_user].attach_port    = port;

             if (checked)
              write_str(new_user,"System in check or in progress of booting, please wait..");

/*--------------------------------------------------*/
/* Randomize motds, by the NUM_MOTDS variable       */
/*  First motd is motd0, then motd1, and so on      */
/* Ditto on the wiz port with wizmotd0 and wizmotd1 */
/*--------------------------------------------------*/
 
     imotd = rand() % NUM_MOTDS;

	if (ustr[new_user].attach_port=='1')
		sprintf(filename, "%s/motd%d", LIBDIR, imotd);
	else if (ustr[new_user].attach_port=='2')
		sprintf(filename,"%s/wizmotd%d", LIBDIR, imotd);

             cat( filename, new_user, -1);
             sprintf(mess,TOTAL_USERS_MESS,system_stats.tot_users);
             write_str(new_user,mess);
             write_str(new_user,"@@");

             /*------------------------*/
	     /* get internet site info */
	     /*------------------------*/

  ustr[new_user].site[0]=0;
  ustr[new_user].net_name[0]=0;

  ip_address = acc_addr.sin_addr.s_addr;

  /* Get ip address of new user..we do this no matter what */
  resolve_add(new_user,ip_address,1);

  /* If global to resolve address, resolve address to hostname */
  if (resolve_names >= 1) resolve_add(new_user,ip_address,2);
  else strcpy(ustr[new_user].net_name,SYS_RES_OFF);

  /* Send connection info to staff who ask to get it */
  sprintf(mess," [INCOMING LOGIN from %s (%s) on line %d (%c)]",ustr[new_user].net_name,ustr[new_user].site,ustr[new_user].sock,port);
  writeall_str(mess, -2, new_user, 0, new_user, BOLD, NONE, 0);
  mess[0]=0;

	     /*---------------------------*/
	     /* Check for restricted site */
	     /*---------------------------*/
	     
             if (check_restriction(new_user, ANY) == 1) 
               {
                sprintf(mess,"%s: Connection attempt, RESTRICTed site %s:%s\n",get_time(0,0),ustr[new_user].site,
		ustr[new_user].net_name);
		print_to_syslog(mess);
                user_quit(new_user);
		mess[0]=0;
                continue;
               }     

		telnet_echo_on(user);
             write_str_nr(new_user,SYS_LOGIN);
	     /* ask if we can send EORs for clients like TF */
	     telnet_ask_eor(new_user);

	   } /** end of last FD_ISSET **/

	/** cycle through user list, getting input and taking actions	**/
	/** based on their status					**/
	for (user=0; user < MAX_USERS; ++user) {

		strcpy(inpstr,get_input(user,ustr[user].attach_port,0));
		if (!strcmp(inpstr,"_someerr")) continue;

		ustr[user].last_input    = time(0);  /* ie now        */
		ustr[user].warning_given = 0;        /* reset warning */
		
		/*---------------------------------*/
		/* user wakes up from afk or bafk  */
		/*---------------------------------*/
		
		if (ustr[user].afk)
		  {
                    if (ustr[user].lockafk) {
                        strtolower(inpstr);
                        st_crypt(inpstr);
                        if (strcmp(ustr[user].password,inpstr)) {
                          telnet_echo_on(user);
                          write_str(user,"Password incorrect.");
                          write_str_nr(user,"Enter password: ");
                          telnet_echo_off(user);
			  telnet_write_eor(user);
                          continue;
                          }
                        else { telnet_echo_on(user); }
                       }
		    if (!ustr[user].vis)
		     sprintf(mess,AFK_BACK,INVIS_ACTION_LABEL);
		    else
		     sprintf(mess,AFK_BACK,ustr[user].say_name);
                    writeall_str(mess,1,user,0,user,NORM,AFK_TYPE,0);
		    alert_check(user,1);
                    if (ustr[user].lockafk==0) {
                      if (ustr[user].afk==1)
                       sprintf(mess,AFK_BACK3, "AFK");
                      else if (ustr[user].afk==2)
                       sprintf(mess,AFK_BACK3, "BAFK");
                      write_str(user,mess);
                     }
                    ustr[user].afk = 0;
                    ustr[user].afkmsg[0] = 0;
                    if (ustr[user].lockafk) {
                      write_str(user,AFK_BACK2);
                      ustr[user].lockafk = 0;
                      continue;
                      }
		  }
		  
		/*-------------------------------*/
		/* see if user is logging in     */
		/*-------------------------------*/
		if (ustr[user].logging_in) 
		  { 
		   login(user,inpstr);  
		   continue; 
		  }

		/*-------------------------------*/
		/* see if user is reading a file */
		/*-------------------------------*/
		if (ustr[user].file_posn) 
		  {
		   if (inpstr[0] == 'q' || inpstr[0] == 'Q') 
		     {
		      ustr[user].file_posn=0;  
		      ustr[user].line_count=0; 
		      ustr[user].number_lines=0; 
		      ustr[user].numbering=0;
                      continue;
		     }
		     
		   cat(ustr[user].page_file,user, 0); 
		   continue;
		  }


              /*---------------------------------------*/
              /*  See if user is entering a profile    */
              /*---------------------------------------*/
              if (ustr[user].pro_enter) {
               if (ustr[user].pro_enter > PRO_LINES) strtolower(inpstr);
               enter_pro(user,inpstr);  continue;
               }

              /*---------------------------------------*/
              /*  See if user is adding a talker       */
              /*---------------------------------------*/
              if (ustr[user].t_ent) {
                 talker(user,inpstr);  continue;
                 }

              /*---------------------------------------*/
              /*  See if user is entering a vote topic */
              /*---------------------------------------*/
              if (ustr[user].vote_enter) {
               if (ustr[user].vote_enter > VOTE_LINES) strtolower(inpstr);
               enter_votedesc(user,inpstr);  continue;
               }

              /*---------------------------------------*/
              /*  See if user is writing a room desc   */
              /*---------------------------------------*/
              if (ustr[user].roomd_enter) {
               if (ustr[user].roomd_enter > ROOM_DESC_LINES) strtolower(inpstr);
               descroom(user,inpstr);  continue;  
               }

              /* make sure users can't send bot commands */
               if (strstr(inpstr,"+++++")) continue;

	      /* did the user enter a macro of theirs? */
	       if (check_macro(user,inpstr) == -1) continue;

               /*----------------------------------------------*/
               /* if user did nothing, return                  */
               /*----------------------------------------------*/
               
	       if (!inpstr[0] || nospeech(inpstr)) continue; 

	       /*------------------------*/
	       /* deal with any commands */
	       /*------------------------*/
	       com_num=get_com_num(user,inpstr);
		
		if ((com_num == -1) && 
                    (inpstr[0] == '.' || !strcmp(ustr[user].name,BOT_ID))) 
		  {
		   write_str(user,SYNTAX_ERROR);
		   continue;
		  }
		  
		if (com_num != -1) 
		  {
		   last_user=user;
                   if ((!strcmp(ustr[user].name,BOT_ID) || !strcmp(ustr[user].name,ROOT_ID)) && inpstr[0]=='_')
                    bot_com(com_num,user,inpstr);
                   else
		    exec_com(com_num,user,inpstr);
			
		   last_user= -1;
		   continue;
		  }

		/*--------------------------------------------*/
		/* see if input is answer to clear_mail query */
		/*--------------------------------------------*/
		if (ustr[user].clrmail==user && inpstr[0]!='y') 
		  {
		   ustr[user].clrmail= -1; 
		   continue;
		  }
		  
		if (ustr[user].clrmail==user && inpstr[0]=='y') 
		  {
                     if (delete_sent) {
                        clear_sent(user, "");
                        ustr[user].clrmail= -1;
                        continue;
                        }
                     else {
		        clear_mail(user, "");
		        ustr[user].clrmail= -1;
		        continue;
                        }
		  } 

		/*------------------------------------------*/
		/* see if input is answer to shutdown query */
		/*------------------------------------------*/
		
		if (shutd==user && inpstr[0]!='y') 
		  {
		    shutd= -1;  
		    continue;
		   }
		   
		if (shutd==user && inpstr[0]=='y') 
		  shutdown_d(user,"");

		/*-----------------------------------------------------*/
		/* send speech to speaker & everyone else in same area */
		/*-----------------------------------------------------*/
		commands++;
		say(user, inpstr, 0);
	      } /* end of MAIN USER FOR */

	user=0;

	/** cycle through web server users **/
	for (user=0; user < MAX_WWW_CONNECTS; ++user) {

		strcpy(inpstr,get_input(user,'4',1));
		if (!strcmp(inpstr,"_someerr")) continue;
		parse_input(user,inpstr);
		continue;

	   } /* end of web server users for */

	user=0;

if (resolve_names==2) {
	for (user = 0; user < MAX_USERS; ++user) 
	  {
		if (ustr[user].needs_hostname==1)
		 cache_lookup(user);
	  } /* end of ongoing user ip cache lookup */
} /* end of resolve_names via site cache if */

	} /* end while LOOP_FOREVER */
	
    shutdown_error(8);

}

/*------------------------------------------------------------------------*/
/*                       Utility Functions Follow                         */
/*------------------------------------------------------------------------*/

/* Read user input from a socket */
char *get_input(int user, char port, int mode)
{
int len=0;
int complete_line;
int buff_size;
int yeah=0,serr=0;

char *dest;
char *err="_someerr";

static char astring[ARR_SIZE];
unsigned  char inpchar[2];   /* read() data from socket into this */

        int  char_buffer_size = 0;
        char char_buffer[MAX_CHAR_BUFF];

                /* if (ustr[user].area== -1 && !ustr[user].logging_in) */
                /* continue; */
if (mode==0) {
                if (ustr[user].sock == -1) return err;
}
else if (mode==1) {
                if (wwwport[user].sock == -1) return err;
}

                /* see if any data on socket else continue */
if (mode==0) {
                if (!FD_ISSET(ustr[user].sock,&readmask)) return err;
}
else if (mode==1) {
                if (!FD_ISSET(wwwport[user].sock,&readmask)) return err;
}
                  
               /*--------------------------------------------*/
               /* reset the user input space                 */
               /*--------------------------------------------*/
               
                 astring[0]  = 0;
                 inpchar[0]  = 0;
                 dest        = astring;

                 /*-----------------------------------------*/
                 /* see if the user is gone or has input    */
                 /*-----------------------------------------*/

/* a win32 two-step */
#if defined(WIN32) && !defined(__CYGWIN32__)
 yeah=1;
 serr=SOCKET_ERROR;
#else
 yeah=0;
#endif

if (yeah==1) {
	if (mode==0) {
                 len = S_READ(ustr[user].sock, inpchar, 1);
	}
	else if (mode==1) {
                 len = S_READ(wwwport[user].sock, inpchar, 1);
	}

                 if (!len || len==serr) {
			if (mode==0)
		                    user_quit(user);
			else if (mode==1)
				    free_sock(user,port);

		                    return err;
                	}

  } /* end of if yeah */
else {
	if (mode==0) {
                 if ( !( len = S_READ(ustr[user].sock, inpchar, 1) ) ) {
                    user_quit(user);
                    return err;
                   }
	}
	else if (mode==1) {
                 if ( !( len = S_READ(wwwport[user].sock, inpchar, 1) ) ) {
		    free_sock(user,port);
                    return err;
                   }
	}
  } /* end of else yeah */
              
yeah=0;

                 /*-------------------------------------------*/
                 /* if there is input pending, read it        */
                 /*  (stopping on <cr>, <EOS>, or <EOF>)      */
                 /*-------------------------------------------*/
                 complete_line = 0;

if (mode==0) {
                 while ((inpchar[0] != 0)  &&
                        (len != EOF)       &&
                        (len != -1)        &&
                        (complete_line ==0 )  )
                   {
                    /*----------------------------------------------*/
                    /* process input                                */
                    /*----------------------------------------------*/
                    switch (inpchar[0])
                     {
                       case IAC:     do_telnet_commands(user);
                                     break;

                       case '\001':  user_quit(user); break;    /* soh  */
                       case '\002':  user_quit(user); break;    /* stx */
                       case '\003':  user_quit(user); break;    /* etx */
                       case '\004':  user_quit(user); break;
                       case '\005':  user_quit(user); break;    /* enq  */
                       case '\006':  user_quit(user); break;    /* ack */

                       case 127:                             /* delete */
                       case '\010':  ustr[user].char_buffer_size--;
                                     if (ustr[user].char_buffer_size < 0)
                                       {
                                        ustr[user].char_buffer_size = 0;
                                       }
                                      else
                                       {
                                        write_str_nr(user, " \b");
                                       }
                                     break;


                       case '\013':  user_quit(user); break;    /* enq */
                       case '\014':  user_quit(user); break;    /* enq */
                       case '\015':  break;

                       case '\016':  user_quit(user); break;    /* enq */
                       case '\017':  user_quit(user); break;    /* enq */

                       case '\020':  user_quit(user); break;    /* dle */
                       case '\021':  user_quit(user); break;    /* dc1 */
                       case '\022':  user_quit(user); break;    /* dc2 */
                       case '\023':  user_quit(user); break;    /* dc3 */
                       case '\024':  user_quit(user); break;    /* dc4 */
                       case '\025':  user_quit(user); break;    /* nak */
                       case '\026':  user_quit(user); break;    /* syn */
                       case '\027':  user_quit(user); break;    /* etb */

                       case '\030':  user_quit(user); break;    /* can */
                       case '\031':  user_quit(user); break;    /* em  */
                       case '\032':  user_quit(user); break;    /* sub */
                       case '\033':  ; break;                   /* esc */
                       case '\034':  user_quit(user); break;    /* fs  */
                       case '\035':  user_quit(user); break;    /* gs  */
                       case '\036':  user_quit(user); break;    /* rs  */
                       case '\037':  user_quit(user); break;    /* us  */

                       default:
                           ustr[user].char_buffer[ustr[user].char_buffer_size++] = inpchar[0];
                           break;
                     } /* end of switch */

	if (ustr[user].logging_in && ustr[user].promptseq==2) {
		ustr[user].promptseq=1;
		telnet_write_eor(user);
		}

                    if (inpchar[0] == '\012')
                        {
                         complete_line = 1;
                         ustr[user].char_buffer[ustr[user].char_buffer_size++] = 0;
                        }
                      else
                        {
                         if (ustr[user].char_buffer_size > (MAX_CHAR_BUFF-4) )
                           {
                  ustr[user].char_buffer[ustr[user].char_buffer_size++] = '\n';
                  ustr[user].char_buffer[ustr[user].char_buffer_size++] = 0;
                            complete_line = 1;
                           }
                        }

               inpchar[0]=0;

                    if (complete_line == 0)
                      {
                       len = S_READ(ustr[user].sock, inpchar, 1);
                      }
                   } /* end of while */

                /*--------------------------------------*/
                /* terminate the line                   */
                /*--------------------------------------*/

                ustr[user].char_buffer[ustr[user].char_buffer_size] = 0;

                /*------------------------------------------------*/
                /* check for complete line (terminated by \n)     */
                /*------------------------------------------------*/

                if (!complete_line)
                 {
    /*-----------------------------------------------------*/
    /* need to support char mode, no local echo, some time */
    /*-----------------------------------------------------*/
    /* write_str_nr(user,ustr[user].char_buffer[ustr[user].char_buffer_size]); */
    /*-----------------------------------------------------*/
                   return err;
                  }

                /*--------------------------------------------*/
                /* copy the user buffer to the input string   */
                /*--------------------------------------------*/

                strcpy(astring, ustr[user].char_buffer);
                buff_size = strlen(astring);
                ustr[user].char_buffer_size = 0;

                if ((astring[0] == '\012') && (ustr[user].logging_in)
		    && (ustr[user].logging_in < 11)) return err;

                /*----------------------------------------------------*/
                /* some nice users were doing some things that would  */
                /* intentionally kill the system.  This should trap   */
                /* that and report such incidents.                    */
                /*----------------------------------------------------*/
                
                if (buff_size > 8000)
                  {
                    sprintf(mess,"HACK flood from site %21.21s possibly as %12s\n",
                                  ustr[user].site,
                                  ustr[user].say_name);
                    print_to_syslog(mess);
                    
                    writeall_str(mess, WIZ_ONLY, -1, 0, -1, BOLD, NONE, 0); 
                    
                    if (ustr[user].logging_in)
                      {
                        write_str(user,"----------------------------------------------------------------");
                        write_str(user,"Notice:  You are attempting to use this computer system in a way");
                        write_str(user,"         which is considered a crime under United States federal");
                        write_str(user,"         access laws.  All attempts illegally access this site are ");
                        write_str(user,"         logged.  Repeat violators of this offense will be ");
                        write_str(user,"         prosecuted to the fullest extent of the law.");
                        write_str(user,"----------------------------------------------------------------");
                        
                        /*-----------------------------------------*/
                        /* during logins, auto restrict the site   */
                        /*-----------------------------------------*/
                    
                        auto_restrict(user);    

                        user_quit(user);
                       }
                      else
                       {
                        if (ustr[user].locked == 0)
                          {
                           write_str(user,"Notice: Buffer data has been lost. ");
                           write_str(user,"        Further lose of data will result in connection termination.");
                           ustr[user].locked = 1;
                          }
                         else
                          {
                           write_str(user,"Notice: Connection terminated due to loss of data.\n");
                           user_quit(user);
                          }

                       }

                    return err;
                  } /* end of if buff size */

} /* end of if mode USER */
else if (mode==1) {
                 while ((inpchar[0] != 0)  &&
                        (len != EOF)       &&
                        (len != -1)        &&
                        (complete_line ==0 )  )
                   {
                    /*----------------------------------------------*/
                    /* process input                                */
                    /*----------------------------------------------*/
                    switch (inpchar[0])
                     {
                       case IAC:     break;

                       case '\001':  free_sock(user,port); break;
                       case '\002':  free_sock(user,port); break;
                       case '\003':  free_sock(user,port); break;
                       case '\004':  free_sock(user,port); break;
                       case '\005':  free_sock(user,port); break;
                       case '\006':  free_sock(user,port); break;

                       case 127:                             /* delete */
                       case '\010':  char_buffer_size--;
                                     if (char_buffer_size < 0 )
                                       {
                                        char_buffer_size = 0;
                                       }
                                     break;


                       case '\013':  free_sock(user,port); break;
                       case '\014':  free_sock(user,port); break;

                       case '\016':  free_sock(user,port); break;
                       case '\017':  free_sock(user,port); break;

                       case '\020':  free_sock(user,port); break;
                       case '\021':  free_sock(user,port); break;
                       case '\022':  free_sock(user,port); break;
                       case '\023':  free_sock(user,port); break;
                       case '\024':  free_sock(user,port); break;
                       case '\025':  free_sock(user,port); break;
                       case '\026':  free_sock(user,port); break;
                       case '\027':  free_sock(user,port); break;

                       case '\030':  free_sock(user,port); break;
                       case '\031':  free_sock(user,port); break;
                       case '\032':  free_sock(user,port); break;
                       case '\033':  ; break;               
                       case '\034':  free_sock(user,port); break;
                       case '\035':  free_sock(user,port); break;
                       case '\036':  free_sock(user,port); break;
                       case '\037':  free_sock(user,port); break;

                       default:
                           char_buffer[char_buffer_size++] = inpchar[0];
                           break;
                     } /* end of switch */

                    if (inpchar[0] == '\012' || inpchar[0] == '\015')
                        {
                         complete_line = 1;
                         char_buffer[char_buffer_size++] = 0;
                        }
                      else
                        {
                         if (char_buffer_size > (MAX_CHAR_BUFF-4) )
                           {
                  char_buffer[char_buffer_size++] = '\n';
                  char_buffer[char_buffer_size++] = 0;
                            complete_line = 1;
                           }
                        }

               inpchar[0]=0;

                    if (complete_line == 0)
                      {
                       len = S_READ(wwwport[user].sock, inpchar, 1);
                      }
                   } /* end of while */

                /*--------------------------------------*/
                /* terminate the line                   */
                /*--------------------------------------*/

                char_buffer[char_buffer_size] = 0;

                /*------------------------------------------------*/
                /* check for complete line (terminated by \n)     */
                /*------------------------------------------------*/

                if (!complete_line)
                 {
    /*-----------------------------------------------------*/
    /* need to support char mode, no local echo, some time */
    /*-----------------------------------------------------*/
    /* write_str_nr(user,char_buffer[char_buffer_size]); */
    /*-----------------------------------------------------*/
                   return err;
                  }

                /*--------------------------------------------*/
                /* copy the user buffer to the input string   */
                /*--------------------------------------------*/

                strcpy(astring, char_buffer);
                buff_size = strlen(astring);
                char_buffer_size = 0;

                if (astring[0] == '\012') return err;

                /*----------------------------------------------------*/
                /* some nice users were doing some things that would  */
                /* intentionally kill the system.  This should trap   */
                /* that and report such incidents.                    */
                /*----------------------------------------------------*/
                
                if (buff_size > 8000)
                  {
                    sprintf(mess,"HACK flood from site %21.21s\n",
                                  wwwport[user].site);
                    print_to_syslog(mess);
                    
		    free_sock(user,port);

                    writeall_str(mess, WIZ_ONLY, -1, 0, -1, BOLD, NONE, 0); 
                    
                    return err;
                  } /* end of if buff size */

} /* end of if mode 1 */
                 
                /*-------------------------------------*/
                /* terminate the string                */
                /*-------------------------------------*/
                
		/* misc. operations */
		terminate(user, astring);

return astring;
}

/*----------------------------------*/
/* Normal speech                    */
/*----------------------------------*/
void say(int user, char *inpstr, int mode)
{
  int z=0,gravoked=0;
  int area = ustr[user].area;

if (!mode) {
 /* Check if command was revoked from user - UNDER CONSTRUCTION */
 for (z=0;z<MAX_GRAVOKES;++z) {
        if (!isrevoke(ustr[user].revokes[z])) continue;
        if (strip_com(ustr[user].revokes[z])==sys[get_com_num(user,".say")].jump_vector) { gravoked=1; break; }
   }
 if (gravoked==1) {
    write_str(user,NOT_WORTHY);
    gravoked=0; z=0;
    return;
    }
} /* end of !mode */
 
  if (!strlen(inpstr) && strcmp(astr[area].name,BOT_ROOM))
    {
      write_str(user," [Default blank say action is a review]");
      review(user);
      return;
    }
    

  says++;
  if (!strcmp(inpstr,"quit") || !strcmp(inpstr,"q") || 
      !strcmp(inpstr,"/quit") || !strcmp(inpstr,"QUIT")) {
      write_str(user,"The command to leave is --> .quit");
      return;
      }
  if ((!strcmp(inpstr,"help") && strcmp(astr[area].name,BOT_ROOM)) || 
      !strcmp(inpstr,"/help") ||
      !strcmp(inpstr,"HELP")) {
      write_str(user,HELP_HELP);
      return;
     }

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

  sprintf(mess,VIS_SAYS,ustr[user].say_name,inpstr);
  write_str(user,mess);	
		
  if (!ustr[user].vis)
    sprintf(mess,INVIS_SAYS,INVIS_TALK_LABEL,inpstr);   
			                        
  writeall_str(mess,1,user,0,user,NORM,SAY_TYPE,0);

/*--------------------------------*/
/* store say to the review buffer */
/*--------------------------------*/
  strncpy(conv[area][astr[area].conv_line],mess,MAX_LINE_LEN);
  astr[area].conv_line=(++astr[area].conv_line)%NUM_LINES;	
}


/* Reset all user auto-forward limits to nill               */
/* Also check abbreviations to see if any have been deleted */
/* added, or changed                                        */
void reset_userfors(int startup)
{
int c=0;
int i=0;
int b=1;
int a=0;
int j=0;
int fixed=0;
int matched=0;
int add=0;
int changed=0;
int ret=0;
char small_buffer[FILE_NAME_LEN];
char filerid[FILE_NAME_LEN];
char z_mess[ARR_SIZE];
struct dirent *dp;
DIR *dirp;
 
 sprintf(z_mess,"%s",USERDIR);
 strncpy(filerid,z_mess,FILE_NAME_LEN);
 
 dirp=opendir((char *)filerid);

 if (dirp == NULL)
   {  
      sprintf(z_mess,"SYSTEM: Directory information not found for reset_userfors\n");
	if (!startup)
         print_to_syslog(z_mess);
	else
	 printf("%s",z_mess);
      return;
   }

 while ((dp = readdir(dirp)) != NULL) 
   { 

    sprintf(small_buffer,"%s",dp->d_name);
        if (small_buffer[0] == '.')
         continue;
        else {
         strtolower(small_buffer);
         ret=read_user(small_buffer);
         if (ret==0) {
          sprintf(z_mess,"Can't open userfile %s.  Permission problem probably\n",small_buffer);
          if (!startup)
           print_to_syslog(z_mess);
          else
           printf("%s",z_mess);
          continue;
         }
         else if (ret==-1) {
          sprintf(z_mess,"Can't open userfile %s.  0 length file!  Removed.  Continuing..\n",small_buffer);
          if (!startup)
           print_to_syslog(z_mess);
          else
           printf("%s",z_mess);
          continue;
         }
 
         t_ustr.automsgs = 0;
	 /* Check to make sure granted/revoked commands still exist */
	 for (j=0;j<MAX_GRAVOKES;++j) {
		if (strlen(t_ustr.revokes[j])) {
		  for (i=0;sys[i].jump_vector != -1;++i) {
			if (strip_com(t_ustr.revokes[j]) == sys[i].jump_vector) {
				changed=1; break;
				}
		     }
		if (!changed) t_ustr.revokes[j][0]=0;
		changed=0; i=0;
		}
	    }
	 j=0;
	 i=0;
	 changed=0;

          /* Check if abbreviations have changed */
         for (j=0;j<NUM_ABBRS;++j) {
         if (strlen(t_ustr.custAbbrs[j].abbr) > 1) {
          t_ustr.custAbbrs[j].abbr[0]=0;
          t_ustr.custAbbrs[j].com[0]=0;
          }
         else if (t_ustr.custAbbrs[j].com[0]!='.') {
          t_ustr.custAbbrs[j].abbr[0]=0;
          t_ustr.custAbbrs[j].com[0]=0;
          }
         } /* end of for */

    /*-----------------------------------------------------------------*/
    /* PART 1 - Check if number of abbreviations defined has decreased */
    /*-----------------------------------------------------------------*/
    i = NUM_ABBRS;
    if (strlen(t_ustr.custAbbrs[i].com) > 1 ) {
       i=0;
        while (strlen(t_ustr.custAbbrs[i].com)) {
          for (c=0; sys[c].su_com != -1 ;++c) {
             if (!strcmp(t_ustr.custAbbrs[i].com,sys[c].command)) {
                if (!strlen(sys[c].cabbr)) {
while (strlen(t_ustr.custAbbrs[i+b].com) > 0) {
 strcpy(t_ustr.custAbbrs[i+a].com,t_ustr.custAbbrs[i+b].com);
 strcpy(t_ustr.custAbbrs[i+a].abbr,t_ustr.custAbbrs[i+b].abbr);
 a++;
 b++;
 }
 
/* Make sure last lingering copy is cleared out */
t_ustr.custAbbrs[i+a].com[0]=0;
t_ustr.custAbbrs[i+a].abbr[0]=0;
a=0;
b=1;
                  fixed=1;
                  changed=1;
                  break;
                  }
                else { fixed=0; break; }
               }
            } /* end of for */
          c=0;
          if (!fixed) i++;
         } /* end of while */
      } /* end of main if */

 c=0;
 i=0;
 fixed=0;

    /*---------------------------------------------------------*/
    /* PART 2 - Check if these were any abbreviation additions */
    /*---------------------------------------------------------*/
    for (i=0;i<NUM_ABBRS;++i) {
        if (strlen(t_ustr.custAbbrs[i].com) <= 1) add++;
       }
    /* Get number of current abbreviations */
    i = NUM_ABBRS - add;

    for (c=0; sys[c].su_com != -1 ;++c) {
       if (strlen(sys[c].cabbr) > 0) {
          for (a=0;a<i;++a) {
             if (!strcmp(sys[c].command,t_ustr.custAbbrs[a].com)) {
               matched=1;
               break;
               }
             } /* end of abbr for */
          if (!matched) {
            strcpy(t_ustr.custAbbrs[i].com,sys[c].command);
            strcpy(t_ustr.custAbbrs[i].abbr,sys[c].cabbr);
            changed=1;
            i++;
            }
          matched=0;
          a=0;
         } /* end of sys abbr if */
      } /* end of command for */

i=0;
b=1;
c=0;
a=0;
fixed=0;
matched=0;
add=0;

    /* Check if admin or coder changed the name of a command */
    for (i=0;i<NUM_ABBRS;++i) {
     for (c=0; sys[c].su_com != -1 ;++c) {
        if (!strcmp(t_ustr.custAbbrs[i].abbr,sys[c].cabbr) ) {
          if (strcmp(t_ustr.custAbbrs[i].com,sys[c].command)) {
             strcpy(t_ustr.custAbbrs[i].com,sys[c].command);
             changed=1;
            } /* end of sub strcmp if */
          } /* end of main strcmp if */
       } /* end of sys[] for */
      } /* end of num_abbrs for */

i=0;
a=0;
        write_user(small_buffer);
        } /* end of else */
   }

if (changed)
 print_to_syslog("Re-evaluated user abbreviations\n");
 
 (void) closedir(dirp);

}

/*---------------------------------------*/
/* Get a temporary file for data storage */
/*---------------------------------------*/
char *get_temp_file()
{
static char tempname[FILE_NAME_LEN];

sprintf(tempname,"junk/temp_%d.%d",rand()%500,rand()%500);
return tempname;
}

/*----------------------------------------------------*/
/* Return error description for error code for WIN32  */
/* since we can't find function like strerror         */
/*----------------------------------------------------*/
#if defined(WIN32) && !defined(__CYGWIN32__)
char *winerrstr(int type)
{
static char errdesc[80];

switch(type) {
  case WSAEINTR: strcpy(errdesc,"Interrupted system call"); break;
  case WSAEBADF: strcpy(errdesc,"Bad file number"); break;
  case WSAEACCES: strcpy(errdesc,"Permission denied"); break;
  case WSAEFAULT: strcpy(errdesc,"Bad Address"); break;
  case WSAEINVAL: strcpy(errdesc,"Invalid argument"); break;
  case WSAEMFILE: strcpy(errdesc,"Too many open files"); break;
  case WSAEWOULDBLOCK: strcpy(errdesc,"Operation would block"); break;
  case WSAEINPROGRESS: strcpy(errdesc,"Operation now in progress"); break;
  case WSAEALREADY: strcpy(errdesc,"Operation already in progress"); break;
  case WSAENOTSOCK: strcpy(errdesc,"Socket operation on nonsocket"); break;
  case WSAEDESTADDRREQ: strcpy(errdesc,"Destination address required"); break;
  case WSAEMSGSIZE: strcpy(errdesc,"Message too long"); break;
  case WSAEPROTOTYPE: strcpy(errdesc,"Protocol wrong type for socket"); break;
  case WSAENOPROTOOPT: strcpy(errdesc,"Protocol not available"); break;
  case WSAEPROTONOSUPPORT: strcpy(errdesc,"Protocol not supported"); break;
  case WSAESOCKTNOSUPPORT: strcpy(errdesc,"Socket type not supported"); break;
  case WSAEOPNOTSUPP: strcpy(errdesc,"Operation not supported on socket"); break;
  case WSAEPFNOSUPPORT: strcpy(errdesc,"Protocol family not supported"); break;
  case WSAEAFNOSUPPORT: strcpy(errdesc,"Address family not supported by protocol family"); break;
  case WSAEADDRINUSE: strcpy(errdesc,"Address already in use"); break;
  case WSAEADDRNOTAVAIL: strcpy(errdesc,"Can't assign requested address"); break;
  case WSAENETDOWN: strcpy(errdesc,"Network is down"); break;
  case WSAENETUNREACH: strcpy(errdesc,"Network in unreachable"); break;
  case WSAENETRESET: strcpy(errdesc,"Network dropped connection on reset"); break;
  case WSAECONNABORTED: strcpy(errdesc,"Software caused connection abort"); break;
  case WSAECONNRESET: strcpy(errdesc,"Connection reset by peer"); break;
  case WSAENOBUFS: strcpy(errdesc,"No buffer space available"); break;
  case WSAEISCONN: strcpy(errdesc,"Socket is already connected"); break;
  case WSAENOTCONN: strcpy(errdesc,"Socket is not connected"); break;
  case WSAESHUTDOWN: strcpy(errdesc,"Can't send after socket shutdown"); break;
  case WSAETOOMANYREFS: strcpy(errdesc,"Too many references can't splice"); break;
  case WSAETIMEDOUT: strcpy(errdesc,"Connection timed out"); break;
  case WSAECONNREFUSED: strcpy(errdesc,"Connection refused"); break;
  case WSAELOOP: strcpy(errdesc,"Too many levels of symbolic links"); break;
  case WSAENAMETOOLONG: strcpy(errdesc,"File name too long"); break;
  case WSAEHOSTDOWN: strcpy(errdesc,"Host is down"); break;
  case WSAEHOSTUNREACH: strcpy(errdesc,"No route to host"); break;
  case WSAENOTEMPTY: strcpy(errdesc,"Directory not empty"); break;
  case WSAEPROCLIM: strcpy(errdesc,"Too many processes"); break;
  case WSAEUSERS: strcpy(errdesc,"Too many users"); break;
  case WSAEDQUOT: strcpy(errdesc,"Disk quota exceeded"); break;
  case WSAESTALE: strcpy(errdesc,"Stale NFS file handle"); break;
  case WSAEREMOTE: strcpy(errdesc,"Too many levels of remote in path");break;
  case WSASYSNOTREADY: strcpy(errdesc,"Network subsystem is unusable"); break;
  case WSAVERNOTSUPPORTED: strcpy(errdesc,"WinSock DLL cannot support this application"); break;
  case WSANOTINITIALISED: strcpy(errdesc,"WinSock not initialized"); break;
  case WSAEDISCON: strcpy(errdesc,"Disconnect"); break;
  case WSAHOST_NOT_FOUND: strcpy(errdesc,"Host not found"); break;
  case WSATRY_AGAIN: strcpy(errdesc,"Nonauthoritative host not found"); break;
  case WSANO_RECOVERY: strcpy(errdesc,"Nonrecoverable error"); break;
  case WSANO_DATA: strcpy(errdesc,"Valid name, no data record of requested type"); break;
  default: strcpy(errdesc,"Unknown error case"); break;
 }

return errdesc;
}
#endif

/*------------------------------------------*/
/* Remove all files from the junk directory */
/*------------------------------------------*/
void remove_junk(int startup)
{
char small_buff[64];
char filerid[FILE_NAME_LEN];
char filename[FILE_NAME_LEN];
struct dirent *dp;
DIR  *dirp;
 
 strcpy(t_mess,"junk");
 strncpy(filerid,t_mess,FILE_NAME_LEN);

 dirp=opendir((char *)filerid);

 if (dirp == NULL)
   {
    strcpy(t_mess,"SYSTEM: Directory information not found for remove_junk()\n");
	if (!startup)
	print_to_syslog(t_mess);
	else
	printf("%s",t_mess);
    return;
   }
   
 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
       if (small_buff[0] != '.')
        {
         sprintf(filename,"%s/%s",filerid,small_buff);
         remove(filename);
        }
    filename[0]=0;
    small_buff[0]=0;
   }

 (void) closedir(dirp);
}

/*** Auto-restrict a site that is hacking on login ***/
void auto_restrict(int user)
{
char timestr[30];
char filename[FILE_NAME_LEN];
time_t tm;
FILE *fp;

time(&tm);
                        sprintf(t_mess,"%s/%s", RESTRICT_DIR, ustr[user].site);
                        strncpy(filename, t_mess, FILE_NAME_LEN);

                        if (!(fp=fopen(filename,"w"))) 
                          {
                           return;
                          }

                        sprintf(timestr,"%ld\n",(unsigned long)tm);
                        fputs(timestr,fp);
                        FCLOSE(fp);

                        /* Add set reason to reason file */
                        sprintf(t_mess,"%s/%s.r", RESTRICT_DIR,ustr[user].site);
                        strncpy(filename, t_mess, FILE_NAME_LEN);

                        if (!(fp=fopen(filename,"w"))) 
                          {
                           return;
                          }
                        fputs("Your site is denied access for hacking\n",fp);
                        FCLOSE(fp);

                        /* Add set comment to comment file */
                        sprintf(t_mess,"%s/%s.c",RESTRICT_DIR,ustr[user].site);
                        strncpy(filename, t_mess, FILE_NAME_LEN);

                        if (!(fp=fopen(filename,"w"))) 
                          {
                           return;
                          }
                        sprintf(mess,"Site denied access for hacking, possibly user %s\n",ustr[user].say_name);
                        fputs(mess,fp);
                        FCLOSE(fp);

}


/** quote function for fortunes **/
void quotes(int user)
{
char line[257];
FILE *pp;

if (!ustr[user].quote) { return; }

write_str(user,"+---- Quote for this login --------------(type .quote to turn these off)----+");
 sprintf(t_mess,"%s -s 2> /dev/null",FORTPROG);
 if (!(pp=popen(t_mess,"r"))) {
	write_str(user,"No quote.");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	line[strlen(line)-1]=0;
	write_str(user,line);
      } /* end of while */
pclose(pp);
}

/*--------------------------------------*/
/* Now we check who we have to alert    */
/* of our login.                        */
/*--------------------------------------*/
void alert_check(int user, int mode)
{
int u=0;
int i=0;
char check[50];

for (u=0;u<MAX_USERS;++u) {

          if (ustr[u].area!= -1 && u !=user) {

                        for (i=0; i<MAX_ALERT; ++i) {
                        strcpy(check,ustr[u].friends[i]);
                        strtolower(check);
                        if (!strcmp(ustr[user].name,check)) 
                           {
			    if (mode==0) {
                              if (!user_wants_message(u,BEEPS))
                               sprintf(mess,"^HR.ALERT!^ %s has logged on.",ustr[user].say_name);
                              else
                               sprintf(mess,"^HR.ALERT!^ %s has logged on.\07\07",ustr[user].say_name);
                              write_str(u,mess);
                           break;
                           } /* end of if mode 0 */
			    else if ((mode==1) && (ustr[u].area!=ustr[user].area)) {
                              if (!user_wants_message(u,BEEPS))
                               sprintf(mess,"^HM.ALERT!^ %s has come back from being ^HYAFK^",ustr[user].say_name);
                              else
                               sprintf(mess,"^HM.ALERT!^ %s has come back from being ^HYAFK^\07\07",ustr[user].say_name);
                              write_str(u,mess);
                           break;
                           } /* end of if mode 1 */
			 } /* end of strcmp if */
                        } /* end of sub-for */
                  i=0; continue;
              }    /* end of if */                
   }   /* end of for */

}

/*------------------------*/
/* Check for gagged user  */
/*------------------------*/
int gag_check(int user, int user2, int mode)
{
int b=0;
char check[50];

      for (b=0; b<MAX_GAG; ++b) {
        strcpy(check,ustr[user2].gagged[b]);
        strtolower(check);
        if (!strcmp(ustr[user].name,check)) {
           if (!mode)
            write_str(user,IS_GAGGED);

           check[0]=0;
           return 0;
          }
        check[0]=0;
        }

return 1;
}

/*----------------------------------------*/
/* Check for gagged user that isnt online */
/*----------------------------------------*/
int gag_check2(int user, char *name)
{
int b=0;
char check[50];

if (!read_user(name)) return 2;

      for (b=0; b<MAX_GAG; ++b) {
        strcpy(check,t_ustr.gagged[b]);
        strtolower(check);
        if (!strcmp(ustr[user].name,check)) {
           write_str(user,IS_GAGGED);
           check[0]=0;
           return 0;
          }
        check[0]=0;
        }

return 1;
}


/*------------------------------------------------------*/
/* Compare NUM_ABBRS to abbr count in sys[] on startup  */
/*------------------------------------------------------*/
void abbrcount()
{
int c=0;
int count=0;

    for (c=0; sys[c].su_com != -1; ++c) {
      if (strlen(sys[c].cabbr) > 0) count++;
    }

    if (count != NUM_ABBRS) {
      printf("\nAbbreviation count in sys[] structure does not match that of NUM_ABBRS\n");
      printf("Aborting startup!\n");
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
      exit(0);
      }
}


/*-----------------------------------------*/
/* Set up the default abbrevation keys     */
/*-----------------------------------------*/
void initabbrs(int user)
{
int c=0;
int i;
	
	for (i=0;i<NUM_ABBRS;++i)
	{
		ustr[user].custAbbrs[i].abbr[0] = 0;
		ustr[user].custAbbrs[i].com[0] = 0;
	}

        i=0;

        for (i=0;i<NUM_ABBRS;++i)
        {

         REDO:
          if (strlen(sys[c].cabbr) > 0) {
            strcpy(ustr[user].custAbbrs[i].com,sys[c].command);
            strcpy(ustr[user].custAbbrs[i].abbr,sys[c].cabbr);
            c++;
           }
          else {
            c++;
            goto REDO;
           }

        }	

}


/*-------------------------------------------------------------------*/
/* Copy abbreviation commands and marks under a blank field into the */
/* field so it isn't blank anymore                                   */
/*-------------------------------------------------------------------*/
void copy_abbrs(int user, int ref)
{
int a=0;
int b=1;

while (strlen(ustr[user].custAbbrs[ref+b].com) > 0) {
 strcpy(ustr[user].custAbbrs[ref+a].com,ustr[user].custAbbrs[ref+b].com);
 strcpy(ustr[user].custAbbrs[ref+a].abbr,ustr[user].custAbbrs[ref+b].abbr);
 a++;
 b++;
 }

/* Make sure last lingering copy is cleared out */
ustr[user].custAbbrs[ref+a].com[0]=0;
ustr[user].custAbbrs[ref+a].abbr[0]=0;
}

/*-----------------------------------------------------------------*/
/* Add new abbreviation fields to the end of the users abbr struct */
/*-----------------------------------------------------------------*/
void add_abbrs(int user, int ref, int num)
{

strcpy(ustr[user].custAbbrs[num].com,sys[ref].command);
strcpy(ustr[user].custAbbrs[num].com,sys[ref].cabbr);

}


/* Get the abbreviation number for the emote command */
int get_emote(int user)
{
int i=0;

for (i=0;i<NUM_ABBRS;++i) {
    if (!strcmp(ustr[user].custAbbrs[i].com,".emote")) {
      return i;
     }
    else continue;
   }

return -1;
}


/*** put string terminate char. at first char < 32 ***/
void terminate(int user, char *str)
{
int u;
int bell = 7;
int tab  = 9;

/*----------------------------------------------------------------*/
/* only allow cntl-g from users rank > 5                          */
/*----------------------------------------------------------------*/

if (ustr[user].super < WIZ_LEVEL) bell = tab;

for (u = 0; u<ARR_SIZE; ++u)  
  {
   if ((*(str+u) < 32 &&       /* terminate line on first control char */
       *(str+u) != bell &&     /* except for bell                      */     
       *(str+u) != tab) ||     /* and tab                              */
       *(str+u) > 126  )       /* special chars over 126               */
     {
      *(str+u)=0; 
      u=ARR_SIZE;
     }
  }
}


/*** convert string to lower case ***/
void strtolower(char *str)
{
while(*str) 
  { 
    *str=tolower((int)*str);
    str++; 
  }
}



/*** check for empty string ***/
int nospeech(char *str)
{
while(*str) 
  {  
    if (*str > ' ')  
       return 0;  
    str++;  
  }
return 1;
}

/*--------------------------------------------------------------------*/
/* This function converts minutes into days hours minutes for output  */
/*--------------------------------------------------------------------*/
char *converttime(long mins)
{
int d,h,m;
static char convstr[35];

d=(int)mins/1440;
m=(int)mins%1440;
h=m/60;
m%=60;

sprintf(convstr,"%d day%s %d hour%s %d minut%s",
        d,d == 1 ? "," : "s,",
        h,h == 1 ? "," : "s,",
        m,m == 1 ? "e" : "es");
return convstr;
}

/* Get time in a certain way and return it as a string */
/* mode 0 is to get rid of the year string and the carriage return */
/* mode 1 is to get rid of just the carriage return */
char *get_time(time_t ref,int mode)
{
time_t tm;
static char mrtime[30];

if ((int)ref==0) {
   time(&tm);
   strcpy(mrtime,ctime(&tm));
  }
else {
   strcpy(mrtime,ctime(&ref));
  }

if (mode==0)
   mrtime[strlen(mrtime)-6]=0; /* get rid of newline and year */
else if (mode==1)
   mrtime[strlen(mrtime)-1]=0; /* get rid of newline */

   return mrtime;
}

char *get_error(void)
{
static char errstr[256];

sprintf(errstr,"(err: %d,%s)",errno,strerror(errno));
return errstr;
}

/* Count colors in a string for correct line formatting		*/
/* if mode 0 just count the codes				*/
/* if mode 1 count space that will be used my replacing codes	*/
int count_color(char *str, int mode)
{
int found=0,i=0,count=0;
int left=strlen(str);

	for(i=0; i<left; i++) {
        if (str[i]==' ') {
                continue;
                }
        if (str[i]=='@') { 
                i++;
                if (str[i]=='@') {
		 if (!mode) count+=2;
		 else count+=4;
		 found=0;
                 continue;
                }
                else { i--;
                       continue;
                     }
                }
        if (str[i]=='^') {
                if (found) {
                        found=0;
			 if (!mode) count++;
			 else count+=4;
                        continue;
                        }
                else {
                        found=1;
			i++;
                 if (str[i]=='H') {
                    i++;
                     if (i == left) {
			 if (!mode) count++;
			 else count+=4;
                         found=0;
                            break;
                           }
                     if (str[i]=='R') {
			if (!mode) count+=3;
			else count+=7;
                       continue;
                      }
                     else if (str[i]=='G') {
			if (!mode) count+=3;
			else count+=7;
                       continue;
                       }
                     else if (str[i]=='Y') {
			if (!mode) count+=3;
			else count+=7;
                       continue;
                       }
                     else if (str[i]=='B') {
			if (!mode) count+=3;
			else count+=7;
                       continue;
                       }
                     else if (str[i]=='M') {
			if (!mode) count+=3;
			else count+=7;
                       continue;
                       }
                     else if (str[i]=='C') {
			if (!mode) count+=3;
			else count+=7;
                       continue;
                       }
                     else if (str[i]=='W') {
			if (!mode) count+=3;
			else count+=7;
                       continue;
                       }
                     else {
				i--;
			 if (!mode) count++;
			 else count+=4;
			  }
                   }
                 else if (str[i]=='L') {
                    i++;
                     if (i == left) {
			 if (!mode) count++;
			 else count+=4;
                         found=0;
                            break;
                           }
                     if (str[i]=='R') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                      }
                     else if (str[i]=='G') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                       }
                     else if (str[i]=='Y') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                       }
                     else if (str[i]=='B') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                       }
                     else if (str[i]=='M') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                       }
                     else if (str[i]=='C') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                       }
                     else if (str[i]=='W') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                       }
                     else {
				i--;
			 if (!mode) count++;
			 else count+=4;
                          }
                   }
                 else if (str[i]=='B') {
                    i++;
                     if (i == left) {
			 if (!mode) count++;
			 else count+=4;
                         found=0;
                            break;
                           }
                     if (str[i]=='L') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                      }
		     else {
			i--; 
			 if (!mode) count++;
			 else count+=4;
			}
                   }
                 else if (str[i]=='U') {
                    i++;
                     if (i == left) {
			 if (!mode) count++;
			 else count+=4;
                         found=0;
                            break;
                           }
                     if (str[i]=='L') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                      }
		     else {
			i--;
			 if (!mode) count++;
			 else count+=4;
			}
                   }
                 else if (str[i]=='R') {
                    i++;
                     if (i == left) {
			 if (!mode) count++;
			 else count+=4;
                         found=0;
                            break;
                           }
                     if (str[i]=='V') {
			if (!mode) count+=3;
			else count+=7;
                        continue;
                      }
		     else {
			i--;
			 if (!mode) count++;
			 else count+=4;
			}
                    }
                 else {
			 if (!mode) count++;
			 else count+=4;
                      }
		} /* end of found else */
            } /* end if if ^ */
        } /* end of for */
found=0;

	return count;
}


/* Generate a random password */
char *generate_password(void)
{
int i=0;
static char pass[12];

pass[0]=0;

i = rand() % 32767;
sprintf(pass,"pin%d",i);

return pass;
}

/* Delete a user that was in tracking but now is a new user */
void delete_verify(int user)
{
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
char junk[12];
char name2[NAME_LEN+1];
long timenum;
FILE *fp;
FILE *fp2;

strcpy(filename,VERIFILE);
if (!(fp=fopen(filename,"r"))) return;

strcpy(filename2,get_temp_file());

if (!(fp2=fopen(filename2,"w"))) {
  fclose(fp);
  sprintf(l_mess,"%s: Could not open tempfile for writing in delete_verify! %s",get_time(0,0),get_error());
  write_log(l_mess,ERROR_L,NEWLINE);
  return;
  }

while (!feof(fp)) {
 fscanf(fp,"%s %s %ld\n",name2,junk,&timenum);
 if (strcmp(name2,ustr[user].login_name))
  {
   fprintf(fp2,"%s %s %ld\n",name2,junk,timenum);
  }
 } /* end of while */

fclose(fp);
fclose(fp2);

remove(filename);
if (!file_count_lines(filename2))
 remove(filename2);
else
 rename(filename2,filename);

}

/* Check to see if user is an email verifying user */
int check_verify(int user, int mode)
{
int found=0;
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
char name2[NAME_LEN+1];
char junk[12];
unsigned long diff=0;
unsigned long timenum;
time_t tm;
FILE *fp;
FILE *fp2;

strcpy(filename,VERIFILE);
if (!(fp=fopen(filename,"r"))) return 0;

if (mode==0) {
while (!feof(fp)) {
 fscanf(fp,"%s %s %ld\n",name2,ustr[user].login_pass,&timenum);
 if (!strcmp(name2,ustr[user].login_name)) {
  found=1;
  break;
  } /* end of if */
 } /* end of while */

fclose(fp);

 if (found==1) { return 1; }
 else {
  ustr[user].login_pass[0]=0;
  return 0;
  }
} /* end of mode if */
else if (mode==1) {
time(&tm);

strcpy(filename2,get_temp_file());

if (!(fp2=fopen(filename2,"w"))) {
  fclose(fp);
  sprintf(l_mess,"%s: Could not open tempfile for writing in check_verify! %s",get_time(0,0),get_error());
  write_log(l_mess,ERROR_L,NEWLINE);
  return 0;
  }

while (!feof(fp)) {
 fscanf(fp,"%s %s %ld\n",name2,junk,&timenum);
 diff = tm - timenum;
 if (diff >= (3600*24)) { continue; }
 else { fprintf(fp2,"%s %s %ld\n",name2,junk,(unsigned long)timenum); }
 } /* end of while */

fclose(fp);
fclose(fp2);
remove(filename);

if (!file_count_lines(filename2))
 remove(filename2);
else
 rename(filename2,filename);

 return 1;
} /* end of else if */

return 0;
}

/* Write user email verification info to tracking file */
int write_verifile(int user, char *epass)
{
char filename[FILE_NAME_LEN];
unsigned long timenum;
time_t tm;
FILE *fp;

strcpy(filename,VERIFILE);
if (!(fp=fopen(filename,"a"))) {
  print_to_syslog("Could not open file to append to in write_verifile!\n");
  return -1;
  }

time(&tm);
timenum = tm;

sprintf(mess,"%s %s %ld\n",ustr[user].login_name,epass,(unsigned long)timenum);
fputs(mess,fp);
fclose(fp);
return 1;

}

/* E-Mail the user their username and the random passsord to emailadd */
int mail_verify(int user, char *epass, char *emailadd)
{
int nosubject=0;
int sendmail=0;
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
char line[301];
FILE *fp;
FILE *pp;

line[0]=0;
strcpy(filename,VERIEMAIL);

if (!(fp=fopen(filename,"r"))) {
  print_to_syslog("Could not open VERIEMAIL file!\n");
  return -1;
  }

/*---------------------------------------------------*/
/* write email message                               */
/*---------------------------------------------------*/

if (strstr(MAILPROG,"sendmail")) {
  sprintf(t_mess,"%s",MAILPROG);
  sendmail=1;
  }
else {
  sprintf(t_mess,"%s %s",MAILPROG,emailadd);
  if (strstr(MAILPROG,"-s"))
	nosubject=0;
  else
	nosubject=1;
  }  
strncpy(filename2,t_mess,FILE_NAME_LEN);

/* Open pipe to sendmail program */
if (!(pp=popen(filename2,"w"))) 
  {
   sprintf(mess,"%s : fmail message cannot be written\n", syserror);
   fclose(fp);
   write_str(user,mess);
   return -1;
  }

if (sendmail) {
fprintf(pp,"From: %s <%s>\n",SYSTEM_NAME,SYSTEM_EMAIL);
fprintf(pp,"To: %s <%s>\n",ustr[user].login_name,emailadd);
fprintf(pp,"Subject: %s new account info\n\n",SYSTEM_NAME);
}
else if (nosubject) {
fprintf(pp,"%s new account info\n",SYSTEM_NAME);
}

fgets(line,300,fp);
strcpy(line,check_var(line,SYS_VAR,SYSTEM_NAME));
strcpy(line,check_var(line,USER_VAR,ustr[user].login_name));

while (!feof(fp)) {
   fputs(line,pp);
   fgets(line,300,fp);
   strcpy(line,check_var(line,SYS_VAR,SYSTEM_NAME));
   strcpy(line,check_var(line,USER_VAR,ustr[user].login_name));
  } /* end of while */
fclose(fp);

fputs("\n",pp);
sprintf(mess," Name/Login name: %s\n",ustr[user].login_name);
fputs(mess,pp);
sprintf(mess," Password       : %s\n",epass);
fputs(mess,pp);
fputs("\n\n",pp);

/* Copy the AGREEFILE to the end of the mail since we wont */
/* ask for it when they login for their new account        */
strcpy(filename,AGREEFILE);

if (!(fp=fopen(filename,"r"))) {
  print_to_syslog("Could not open AGREEFILE file!\n");
  }
else {
  fgets(line,300,fp);

  while (!feof(fp)) {
     fputs(line,pp);
     fgets(line,300,fp);
    } /* end of while */
  fclose(fp);
  } /* end of else */

fputs(".\n",pp);

pclose(pp);

/* Write to log */
sprintf(l_mess,"%s: EMAILVER %s %s:%s:%s",get_time(0,0),ustr[user].login_name,
             ustr[user].site,ustr[user].net_name,emailadd);
write_log(l_mess,VEMAIL_L,NEWLINE);

  return 1;

}

/* Convert userdata file from old version to new version */
/* mode 0 - convert already standardized file		 */
/* mode 1 - convert old non-standardized file		 */
int convert_file(FILE *f, char *filename, int mode)
{
char junk[20];
char buf[1001];
char tempfile[FILE_NAME_LEN];
FILE *rfp;
FILE *wfp;

fclose(f);
if (!(rfp=fopen(filename,"r"))) {
	sprintf(mess,"%s: Can't open userfile %s for converting!\n",get_time(0,0),filename);
	print_to_syslog(mess);
	return 0;
	}
strcpy(tempfile,get_temp_file());
if (!(wfp=fopen(tempfile,"w"))) {
	sprintf(mess,"%s: Can't open tempfile to be used for converting!\n",get_time(0,0));
	print_to_syslog(mess);
	return 0;
	}

if (mode==0) {
	/* first line is version, discard */
	rbuf(junk,20);
	/* write current version to file */
	fputs(UDATA_VERSION,wfp);
	fputs("\n",wfp);

	while (fgets((char *)buf,1000,rfp) != NULL) {
	buf[strlen(buf)-1]=0; /* get rid of nl */

	if (!strncmp(buf,"--ENDVER",8)) {
		/* any new structs I have should get written here */
		remove_first(buf);
		if (!strcmp(buf,"121.ver")) {
		fputs("0\n",wfp); /* hangman wins */
		fputs("0\n",wfp); /* hangman losses */
		fputs("No idea\n",wfp); /* ICQ number */
		fputs("NA\n",wfp); /* miscstr1 */
		fputs("NA\n",wfp); /* miscstr2 */
		fputs("NA\n",wfp); /* miscstr3 */
		fputs("NA\n",wfp); /* miscstr4 */
		fputs("1 0 0 0 0\n",wfp); /* pause_login, miscnum2-5 */
		}
		/* tack on new marker for this version */
		sprintf(mess,"--ENDVER %s\n",UDATA_VERSION);
		fputs(mess,wfp);
		/* continue on looking for user-added structs */
	  } /* end of if ENDVER */
	else {
		fputs(buf,wfp);
		fputs("\n",wfp);
	  } /* end of else */

	} /* end of while */
	fclose(rfp);
	fclose(wfp);
	remove(filename);
	rename(tempfile,filename);
  } /* end of if mode 0 */
else if (mode==1) {
	fputs(UDATA_VERSION,wfp);
	fputs("\n",wfp);

	while (fgets((char *)buf,1000,rfp) != NULL) {
	buf[strlen(buf)-1]=0; /* get rid of nl */

	if (!strcmp(buf,"..End revokes..")) {
		fputs(buf,wfp);
		fputs("\n",wfp);
		/* any new structs I have should get written here */
		fputs("0\n",wfp); /* hangman wins */
		fputs("0\n",wfp); /* hangman losses */
		fputs("No idea\n",wfp); /* ICQ number */
		fputs("NA\n",wfp); /* miscstr1 */
		fputs("NA\n",wfp); /* miscstr2 */
		fputs("NA\n",wfp); /* miscstr3 */
		fputs("NA\n",wfp); /* miscstr4 */
		fputs("1 0 0 0 0\n",wfp); /* pause_login, miscnum2-5 */
		/* tack on marker */
		sprintf(mess,"--ENDVER %s\n",UDATA_VERSION);
		fputs(mess,wfp);
		/* continue on looking for user-added structs */
	  } /* end of if revoke */
	else {
		fputs(buf,wfp);
		fputs("\n",wfp);
	  } /* end of else */

	} /* end of while */
	fclose(rfp);
	fclose(wfp);
	remove(filename);
	rename(tempfile,filename);
  } /* end of else if mode if 1 */

return 1;
}


/* is this command revoked */
int isrevoke(char *str)
{
char junkmain[NAME_LEN];
char junk[2];

if (!strlen(str)) {
	/* no command in this slot */
	return 0;
	}

junk[0]=0;
junkmain[0]=0;
strcpy(junkmain,str);
remove_first(junkmain);
sscanf(junkmain,"%s ",junk);
if (!strcmp(junk,"-")) return 1;
else return 0;
}

/* is this command granted */
int isgrant(char *str)
{
char junkmain[NAME_LEN];
char junk[2];

if (!strlen(str)) {
	/* no command in this slot */
	return 0;
	}

junk[0]=0;
junkmain[0]=0;
strcpy(junkmain,str);
remove_first(junkmain);
sscanf(junkmain,"%s ",junk);
if (!strcmp(junk,"+")) return 1;
else return 0;
}

/* strip command number from gravoke struct */
int strip_com(char *str)
{
char junkstr[4];
int junk=0;

junkstr[0]=0;
sscanf(str,"%s ",junkstr);
junk=atoi(junkstr);
return junk;
}

/* strip level to make from gravoke struct */
int strip_level(char *str)
{
char junkstr[4];
char junkmain[NAME_LEN];
int junk=0;

junkstr[0]=0;
junkmain[0]=0;
strcpy(junkmain,str);
remove_first(junkmain); /* com num */
remove_first(junkmain); /* the + sign */
sscanf(junkmain,"%s",junkstr);
junk=atoi(junkstr);
return junk;
}

/* list global revoke (mode 0) or grant (mode 1) permissions */
void listall_gravokes(int user, int mode)
{
int aa=0,ii=0,found2=0,t=0;
char name2[ARR_SIZE];
char small_buff2[64];
char filerid2[FILE_NAME_LEN];
struct dirent *dp2;
DIR  *dirp2;

if (mode==0)
write_str(user," Global revoke list");
else if (mode==1)
write_str(user," Global grant list");

write_str(user,"+-------------------------------------------------+");

  sprintf(t_mess,"%s",USERDIR);
        
  strncpy(filerid2,t_mess,FILE_NAME_LEN);
    
  dirp2=opendir((char *)filerid2);
       
 if (dirp2 == NULL)
   {write_str(user,"Directory information not found.");
    write_str(user,"+-------------------------------------------------+");
    return;
   }

 while ((dp2 = readdir(dirp2)) != NULL)   
   {
    sprintf(small_buff2,"%s",dp2->d_name);
       if (small_buff2[0]=='.') continue;

	found2=0; aa=0; ii=0;
        read_user(small_buff2);

		/* first see if user has at least 1 to print their name */
                for (aa=0; aa < MAX_GRAVOKES; ++aa) {
		if (mode==0) {
                  if (!isrevoke(t_ustr.revokes[aa])) continue;
		  }
		else if (mode==1) {
                  if (!isgrant(t_ustr.revokes[aa])) continue;
		  }
                  found2=1; break;
                } /* end of for */
		if (!found2) continue;

sprintf(name2,"%-18s",t_ustr.say_name);
		/* print out all permissions */
                for (aa=0; aa < MAX_GRAVOKES; ++aa) {
		if (mode==0) {
                  if (!isrevoke(t_ustr.revokes[aa])) continue;
		  }
		else if (mode==1) {
                  if (!isgrant(t_ustr.revokes[aa])) continue;
		  }
		  if (ii>0) strcat(name2,",");
		  for (t=0;sys[t].jump_vector != -1;++t) {
			if (strip_com(t_ustr.revokes[aa]) == sys[t].jump_vector) {
				  strcat(name2,sys[t].command);
				break;
				}
		     }
			t=0;
		  ii=1;
                } /* end of for */
write_str(user,name2);
continue;
} /* end of while */
(void) closedir(dirp2);
write_str(user,"+-------------------------------------------------+");

}


/* free a misc. sockets structures */
void free_sock(int user, char port)
{

if (port=='3') {
	         while (CLOSE(whoport[user].sock) == -1 && errno == EINTR)
			; /* empty while */
                 FD_CLR(whoport[user].sock,&readmask);

   whoport[user].sock=-1; 
   whoport[user].site[0]=0;
   whoport[user].net_name[0]=0;
  }
else if (port=='4') {
	         while (CLOSE(wwwport[user].sock) == -1 && errno == EINTR)
			; /* empty while */
                 FD_CLR(wwwport[user].sock,&readmask);
   wwwport[user].sock=-1;
   wwwport[user].method=-1;
   wwwport[user].req_length=0;
   wwwport[user].keypair[0]=0;
   wwwport[user].file[0]=0;
   wwwport[user].site[0]=0;
   wwwport[user].net_name[0]=0;
  }

}


/* Count number of users in our user storage directory */
void tot_user_check(int mode)
{
char small_buffer[FILE_NAME_LEN];
char filerid[FILE_NAME_LEN];
char z_mess[ARR_SIZE];
struct dirent *dp;
DIR *dirp;

 sprintf(z_mess,"%s",USERDIR);
 strncpy(filerid,z_mess,FILE_NAME_LEN);
 
 dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {  
    if (mode==1) {
      sprintf(z_mess,"\nSYSTEM: Directory information not found for tot_user_check");
      perror(z_mess);
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
      exit(0);
      }
    else {
      sprintf(z_mess,"SYSTEM: Directory information not found for tot_user_check\n");
      print_to_syslog(z_mess);
      return;
     }
   }

system_stats.tot_users = 0;
   
 while ((dp = readdir(dirp)) != NULL) 
   { 

    sprintf(small_buffer,"%s",dp->d_name);
        if (small_buffer[0] == '.')
         continue;
        else
         system_stats.tot_users++;

   }
 
 (void) closedir(dirp);
}


/*** count no. of messages (counts no. of newlines in message files) ***/
void messcount()
{
char filename[FILE_NAME_LEN];
int a;

for(a=0;a<NUM_AREAS;++a) {
	astr[a].mess_num=0;
	sprintf(t_mess,"%s/board%d",MESSDIR,a);
	strncpy(filename,t_mess,FILE_NAME_LEN);
	
        astr[a].mess_num = file_count_lines(filename);
	}
}


/*------------------------------------------------*/
/* Check the .who input string for a rwho request */
/*------------------------------------------------*/
int check_rwho(int user, char *inpstr)
{
int portnum;
int type=1;  /* 1 - Iforms or Ncohafmuta  0 - Nuts or other */
int i=0;
char addy[64];
char info[100];
char filename[FILE_NAME_LEN];
FILE *fp;

if (inpstr[0]=='@') {
  midcpy(inpstr,inpstr,1,ARR_SIZE);
  if (!strlen(inpstr)) {
   strcpy(filename,get_temp_file());
   if (!(fp=fopen(filename,"w"))) {
     write_str(user,"Can't create file for list of Rwho sites!");
     return 1;
     }
    fputs("Who listings are available from these talkers:\n",fp);
    fputs("^Ncohafmuta Talkers:^\n",fp);
    fputs("  after   - After Dark              aot      - Aotearoa\n",fp);
    fputs("  blue    - The Blue Note           dark     - Dark Tower\n",fp);
    fputs("  emerald - The Emerald Isle        flirt    - Flirt Town\n",fp);
    fputs("  globe   - The Globe               grease   - Grease\n",fp);
    fputs("  invas   - Invasions               jour     - Journeys\n",fp);
    fputs("  jungle  - The Jungle              mega     - Mega MoviePlex\n",fp);
    fputs("  merlin  - Merlin\'s Hideaway       north    - North Woods\n",fp);
    fputs("  path    - Path Of The Unicorn     pw       - PinWHeeLs\n",fp);
    fputs("  scu     - StateOfConfusionUni     sommer   - Sommerland\n",fp);
    fputs("  spell   - Spellbinder             wjjm     - The WJJM\n",fp);
    fputs("  xonia   - Xonia\n",fp);
    fputs("^Iforms or other type talkers:^\n",fp);
    fputs("  beach   - Davenport Beach         breck    - Brecktown\n",fp);
    fputs("  cactus  - Cactus                  chakrams - Chakrams and Scrolls\n",fp);
    fputs("  light   - LightHouse              ocean    - Oceanhome\n",fp);
    fputs("  quest   - Vision Quest            trek     - Enterprise\n",fp);
    fputs("  village - The Village\n",fp);
    fclose(fp);
    cat(filename,user,0);
    return 1;
   }
  if (!strcmp(inpstr,"beach")) {
    strcpy(info,"Davenport Beach (nowaksg.chem.nd.edu 3371)");
    strcpy(addy,"129.74.80.116");
    portnum=3372;
    }
  else if (!strcmp(inpstr,"quest")) {
    strcpy(info,"Vision Quest (talker.com 6000)");
    strcpy(addy,"208.220.34.34");
    portnum=6001;
    }
  else if (!strcmp(inpstr,"spell")) {
    strcpy(info,"Spellbinder (talker.com 9999)");
    strcpy(addy,"208.220.34.34");
    portnum=9990;
    }
  else if (!strcmp(inpstr,"trek")) {
    strcpy(info,"Enterprise (linex1.linex.com 5000)");
    strcpy(addy,"199.4.98.11");
    portnum=5001;
    }
  else if (!strcmp(inpstr,"cactus")) {
    strcpy(info,"Cactus (cactus.ecpi.com 5000)");
    strcpy(addy,"208.21.246.2");
    portnum=5001;
    }
  else if (!strcmp(inpstr,"blue")) {
    strcpy(info,"The Blue Note (ncohafmuta.com 4000)");
    strcpy(addy,"206.245.154.72");
    portnum=3998;
    }
  else if (!strcmp(inpstr,"breck")) {
    strcpy(info,"BreckTown (talker.com 5000)");
    strcpy(addy,"208.220.34.34");
    portnum=5001;
    }
  else if (!strcmp(inpstr,"light")) {
    strcpy(info,"LightHouse (mookie.com 6969)");
    strcpy(addy,"209.27.13.114");
    portnum=6970;
    type=0;
    }
  else if (!strcmp(inpstr,"globe")) {
    strcpy(info,"The Globe (talker.com 9050)");
    strcpy(addy,"208.220.34.34");
    portnum=9051;
    }
  else if (!strcmp(inpstr,"grease")) {
    strcpy(info,"Grease (talker.com 9500)");
    strcpy(addy,"208.220.34.34");
    portnum=9510;
    }
  else if (!strcmp(inpstr,"jour")) {
    strcpy(info,"Journeys (talker.com 9000)");
    strcpy(addy,"208.220.34.34");
    portnum=8999;
    }
  else if (!strcmp(inpstr,"jungle")) {
    strcpy(info,"The Jungle (talker.com 3050)");
    strcpy(addy,"208.220.34.34");
    portnum=3051;
    }
  else if (!strcmp(inpstr,"pw")) {
    strcpy(info,"PinWHeeLs (ncohafmuta.com 5000)");
    strcpy(addy,"206.245.154.72");
    portnum=5001;
    }
  else if (!strcmp(inpstr,"scu")) {
    strcpy(info,"State of Conf. Uni. (talker.com 3500)");
    strcpy(addy,"208.220.34.34");
    portnum=3501;
    }
  else if (!strcmp(inpstr,"sommer")) {
    strcpy(info,"Sommerland (talker.com 5555)");
    strcpy(addy,"208.220.34.34");
    portnum=5560;
    }
  else if (!strcmp(inpstr,"village")) {
    strcpy(info,"The Village (village.pb.net 5000)");
    strcpy(addy,"204.117.211.4");
    portnum=5555;
    }
  else if (!strcmp(inpstr,"wjjm")) {
    strcpy(info,"The WJJM (ncohafmuta.com 6666)");
    strcpy(addy,"206.245.154.72");
    portnum=6668;
    }
  else if (!strcmp(inpstr,"after")) {
    strcpy(info,"After Dark (barney.gonzaga.edu 8000)");
    strcpy(addy,"147.222.2.1");
    portnum=8001;
    }
  else if (!strcmp(inpstr,"aot")) {
    strcpy(info,"Aotearoa (ncohafmuta.com 4444)");
    strcpy(addy,"206.245.154.72");
    portnum=4445;
    }
  else if (!strcmp(inpstr,"chakrams")) {
    strcpy(info,"Chakrams and Scrolls (talker.com 8000)");
    strcpy(addy,"208.220.34.34");
    portnum=8001;
    }
  else if (!strcmp(inpstr,"dark")) {
    strcpy(info,"Dark Tower (talker.com 5432)");
    strcpy(addy,"208.220.34.34");
    portnum=5433;
    }
  else if (!strcmp(inpstr,"flirt")) {
    strcpy(info,"Flirt Town (talker.com 3200)");
    strcpy(addy,"208.220.34.34");
    portnum=3201;
    }
  else if (!strcmp(inpstr,"invas")) {
    strcpy(info,"Invasions (talker.com 9786)");
    strcpy(addy,"208.220.34.34");
    portnum=9788;
    }
  else if (!strcmp(inpstr,"mega")) {
    strcpy(info,"Mega MoviePlex (talker.com 7000)");
    strcpy(addy,"208.220.34.34");
    portnum=7002;
    }
  else if (!strcmp(inpstr,"merlin")) {
    strcpy(info,"Merlin\'s Hideaway (talker.com 9876)");
    strcpy(addy,"208.220.34.34");
    portnum=9873;
    }
  else if (!strcmp(inpstr,"north")) {
    strcpy(info,"North Woods (ncohafmuta.com 6000)");
    strcpy(addy,"206.245.154.72");
    portnum=6001;
    }
  else if (!strcmp(inpstr,"ocean")) {
    strcpy(info,"Oceanhome (talker.com 4000)");
    strcpy(addy,"208.220.34.34");
    portnum=4005;
    }
  else if (!strcmp(inpstr,"path")) {
    strcpy(info,"The Path Of The Unicorn (talker.com 4444)");
    strcpy(addy,"208.220.34.34");
    portnum=4446;
    }
  else if (!strcmp(inpstr,"emerald")) {
    strcpy(info,"The Emerald Isle (ncohafmuta.com 8000)");
    strcpy(addy,"206.245.154.72");
    portnum=8002;
    }
  else if (!strcmp(inpstr,"xonia")) {
    strcpy(info,"Xonia (ncohafmuta.com 3000)");
    strcpy(addy,"206.245.154.72");
    portnum=3001;
    }
  else if (!strcmp(inpstr,"test")) {
    strcpy(info,"Test (www.atomic.org 5001)");
    strcpy(addy,"209.42.10.17");
    portnum=5001;
    }
  else {
   strcpy(filename,get_temp_file());
   if (!(fp=fopen(filename,"w"))) {
     write_str(user,"Can't create file for list of Rwho sites!");
     return 1;
     }
    fputs("Who listings are available from these talkers:\n",fp);
    fputs("^Ncohafmuta Talkers:^\n",fp);
    fputs("  after   - After Dark              aot      - Aotearoa\n",fp);
    fputs("  blue    - The Blue Note           dark     - Dark Tower\n",fp);
    fputs("  emerald - The Emerald Isle        flirt    - Flirt Town\n",fp);
    fputs("  globe   - The Globe               grease   - Grease\n",fp);
    fputs("  invas   - Invasions               jour     - Journeys\n",fp);
    fputs("  jungle  - The Jungle              mega     - Mega MoviePlex\n",fp);
    fputs("  merlin  - Merlin\'s Hideaway       north    - North Woods\n",fp);
    fputs("  path    - Path Of The Unicorn     pw       - PinWHeeLs\n",fp);
    fputs("  scu     - StateOfConfusionUni     sommer   - Sommerland\n",fp);
    fputs("  spell   - Spellbinder             wjjm     - The WJJM\n",fp);
    fputs("  xonia   - Xonia\n",fp);
    fputs("^Iforms or other type talkers:^\n",fp);
    fputs("  beach   - Davenport Beach         breck    - Brecktown\n",fp);
    fputs("  cactus  - Cactus                  chakrams - Chakrams and Scrolls\n",fp);
    fputs("  light   - LightHouse              ocean    - Oceanhome\n",fp);
    fputs("  quest   - Vision Quest            trek     - Enterprise\n",fp);
    fputs("  village - The Village\n",fp);
    fclose(fp);
    cat(filename,user,0);
    return 1;
   }

if (ustr[user].rwho > 1) {
  write_str(user,"You already have a remote who connection opened. Wait for it to finish or timeout first.");
  return 1;
  }

/* Here is where we break the remove who connection off so we dont hang the talker */
/* if the connection fails. We do this with fork() to create a child process       */
/* Things inherited by the child: process credentials, environment, memory, stack, */
/* open file descriptors, close-on-exec flags, signal handling, nice value, sched- */
/* uler class, process group ID, session ID, cwd, root dir, umask, reources limits */
/* , controlling terminal.                                                         */
/* Things NOT inherited by the child: process ID, different parent process ID, own */
/* copy of file descriptors and dir. streams, process, data, text, memory locks,   */
/* process times, pending signals init'd to the empty set, timers created by       */
/* timer_create, async input/output operations, resource utilizations are set to 0 */

   switch(ustr[user].rwho = fork())
	{
	 case -1:	print_to_syslog("Rwho fork failed, case -1!\n");
			write_str(user,"^HR Rwho fork failed, Notify a staff member.^");
			break;
	 case 0:	/* We're the child..lets run along now */
			get_rwho(user,addy,portnum,info,type);
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
			/* Close all user sockets */
			for (i=0;i<MAX_USERS;++i) {
			if (ustr[i].sock != -1) {
        		while (CLOSE(ustr[i].sock) == -1 && errno == EINTR)
				; /* empty while */
                	FD_CLR(ustr[i].sock,&readmask);
			}
			}
			_exit(0);
	 default:	sprintf(mess,"%s: Child spawned with PID of %d\n",get_time(0,0),ustr[user].rwho);
			print_to_syslog(mess);
			break;
	}

   return 1;
 } /* end of if @ */
else return 0;
}

/*-----------------------------------*/
/* Go get the who listing as a child */
/*-----------------------------------*/
void get_rwho(int user, char *host, int port, char *info, int type)
{
	struct	sockaddr_in	raddr;
	struct	hostent		*hp;
	int			fd;
	int			red;
	int			i=0;
	int			flag=0;
	int			point = 0;
	int			done = 0;
	int			linenum = 0;
	int			size=sizeof(struct sockaddr_in);
	char			timebuf[30];
	char			buffer[FILE_NAME_LEN];
	char			*p;
	char			rbuf;
	time_t			tm;

/* Close all the listening sockets */
for (i=0;i<4;++i) {
 FD_CLR(listen_sock[i],&readmask);
 CLOSE(listen_sock[i]);
 }

i=0;

/* Zero out memory for address */
#if defined(HAVE_BZERO)
 bzero((char *)&raddr, size);
#else
 memset((char *)&raddr, 0, size);
#endif  

	p = host;
	while(*p != '\0' && (*p == '.' || isdigit((int)*p)))
		p++;

	/* not all digits or dots */
	if(*p != '\0') {
		if((hp = gethostbyname(host)) == (struct hostent *)0) {
			sprintf(buffer,"^HRUnknown hostname %s, can't get rwho list^",host);
			write_str(user,buffer);
			time(&tm);
			strcpy(timebuf,ctime(&tm));
			timebuf[strlen(timebuf)-6]=0;
			sprintf(buffer,"%s: Unknown hostname %s, can't get rwho list\n",timebuf,host);
			print_to_syslog(buffer);
			return;
		}

		(void)bcopy(hp->h_addr,(char *)&raddr.sin_addr,hp->h_length);
	}
	else {
		unsigned long	f;

		if((f = inet_addr(host)) == -1L) {
			sprintf(buffer,"^HRUnknown ip address %s, can't get rwho list^",host);
			write_str(user,buffer);
			time(&tm);
			strcpy(timebuf,ctime(&tm));
			timebuf[strlen(timebuf)-6]=0;
			sprintf(buffer,"%s: Unknown ip address %s, can't get rwho list\n",timebuf,host);
			print_to_syslog(buffer);
			return;
			}
		(void)bcopy((char *)&f,(char *)&raddr.sin_addr,sizeof(f));
	}

	raddr.sin_port = htons(port);
	raddr.sin_family = AF_INET;

	signal(SIGALRM,SIG_IGN);


	if ((fd = socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET) {
		write_str(user,"^HRRwho socket cannot be made!^");

		time(&tm);
		strcpy(timebuf,ctime(&tm));
		timebuf[strlen(timebuf)-6]=0;
		sprintf(buffer,"%s: Rwho socket failed for %s %d : %s\n",timebuf,host,port,strerror(errno));
		print_to_syslog(buffer);
#if !defined(WIN32) || defined(__CYGWIN32__)
		reset_alarm();
#endif
		return;
	}

        sprintf(buffer,"^Getting listing from %s..^",info);
	write_str(user,buffer);
	write_str(user,"^This MAY take time. You may play through the wait^");
        buffer[0]=0;

#if defined(WIN32) && !defined(__CYGWIN32__)
	if (connect(fd, (struct sockaddr *)&raddr, sizeof(raddr)) == SOCKET_ERROR) {
#else
	if (connect(fd, (struct sockaddr *)&raddr, sizeof(raddr)) == -1) {
#endif
	   if (errno == ECONNREFUSED)
		write_str(user,"^HRConnection refused to talker!  Talker may be down. Try again in a few minutes.^");
	   else if (errno == ETIMEDOUT)
		write_str(user,"^HRConnection timed out to talker!  Talker or internet route may be down. Try again in a few minutes.^");
	   else if (errno == ENETUNREACH)
		write_str(user,"^HRNetwork remote talker is on is unreachable! Try later.^");
	   else if (errno != EINPROGRESS)
		write_str(user,"^HRUnknown problem. Try later.^");

		time(&tm);
		strcpy(timebuf,ctime(&tm));
		timebuf[strlen(timebuf)-6]=0;

		/* Uncomment if you want connection errors logged
		sprintf(buffer,"%s: Connection failed to %s %d : %s\n",timebuf,host,port,strerror(errno));
		print_to_syslog(buffer);
		*/

		CLOSE(fd);
#if !defined(WIN32) || defined(__CYGWIN32__)
		reset_alarm();
#endif
		return;
	}

	while((red = S_READ(fd ,&rbuf, 1)) > 0) {
		flag=0;
		if ((unsigned char)rbuf==255) continue;


                switch(rbuf) {
		case '\n': buffer[point]   = 0;
			   point = 0;
			   break;
		case '\r': flag=1;
			   break;
		case '\t': buffer[point] = ' ';
			   point++;
			   break;
		default: buffer[point] = rbuf;
			 point++;
			 break;
		}
		if (flag) continue;

		/* If complete line, check for header or trailer */
                /* strip is found */
	if (!done) {
		if (!point) {
		 if (type==0) {
			if (linenum < 2) { linenum++; continue; }
		  }
		 else if (type==1) {
			if (linenum < 4) { linenum++; continue; }
		  }

		  if (strstr(buffer,"Total of")) {
			write_str(user,buffer);
			buffer[0]=0;
			done = 1;
		    }
                  else {
			write_str(user,buffer);
			buffer[0]=0;
		    }
		  linenum++;
                  continue;
		 } /* end of if point */
		else continue;
	  } /* end of if done */

	} /* end of while */

        /* Close socket and files and reset vars */
	CLOSE(fd);
	done = 0;
	type = 1;
	linenum = 0;
	point = 0;
	buffer[0]=0;

#if !defined(WIN32) || defined(__CYGWIN32__)
	reset_alarm();
#endif

        /* Cat out rwho listing file to user
	if (!cat(filename,user,0)) {
	  write_str(user,"^HRCannot show Rwho listing!  Notify a staff member.^");
	}
	*/

	write_str(user,"");

return;
}

/*------------------------------------------------------------------*/
/* This function is called after the MOTD is shown.  It allows      */
/* the connector to login to the system as an old user or create a  */
/* new user.                                                        */
/*------------------------------------------------------------------*/
void login(int user, char *inpstr)
{
  char         name[ARR_SIZE];
  char         passwd[ARR_SIZE];
  char         email[ARR_SIZE];
  char         lowemail[71];
  char         email_pass[NAME_LEN];
  char         tempname[NAME_LEN+1];
  char         z_mess[100];
  char         buf[30];
  char         filename[FILE_NAME_LEN];
  int          f=0;
  int          su;
  time_t       tm;
  time_t       tm_then;

  passwd[0]=0;  
  email_pass[0]=0;
  lowemail[0]=0;

  /*----------------------------------------------------------*/
  /* if this is the second time the password has been entered */
  /*----------------------------------------------------------*/
  
  if (ustr[user].logging_in==1)  
    goto CHECK_PASS;

  /*------------------------------------------------------------*/
  /* If we're getting an email address for account verification */
  /*------------------------------------------------------------*/
  if (ustr[user].logging_in==5) {
    email[0]=0;
    sscanf(inpstr,"%s",email);
    email[70]=0;
    strcpy(lowemail,email);
    strtolower(lowemail);

    if (!strcmp(lowemail,"quit")) {
      ustr[user].logging_in=3;
      return;
     }
    if ((!strstr(lowemail,"@") && !strstr(lowemail,".")) || strpbrk(lowemail,";/[]\\") ||
        strstr(lowemail,"whitehouse.gov")) {
      write_str(user,"Invalid email address.");
      write_str_nr(user,EMAIL_VERIFY);
	telnet_write_eor(user);
      return;
      }
    /* Generate password */
    strcpy(email_pass,generate_password());

    /* Email user with password */
    if (mail_verify(user,email_pass,email) == -1) {
      write_str(user,"Mail message could not be sent. Try later.");
      email_pass[0]=0;
      user_quit(user);
      return;
      }

    /* Encrypt password writing time, user, and password to VERIFILE */
    st_crypt(email_pass);
    write_verifile(user,email_pass);
    email_pass[0]=0;

    /* Tell them their username and a password was mailed to them */
    write_str(user,"");
    sprintf(mess,"Your username and password has been emailed to %s",email);
    write_str(user,mess);
    write_str(user,"");
    write_str(user,"Come back when you have received it to activate your account");
    write_str(user,"Thank you for stopping by!");

    user_quit(user);
    return;
    }

  /*------------------------------------------------------------*/
  /* If we're getting a password for account email verification */
  /*------------------------------------------------------------*/
  if (ustr[user].logging_in==10) {

     telnet_echo_on(user);
     sscanf(inpstr,"%s",passwd);
     strtolower(passwd);
     st_crypt(passwd);                                    
      
     if (strcmp(ustr[user].login_pass,passwd)) 
       {
         write_str(user,NON_VERIFY);
         ustr[user].login_pass[0]=0;
         user_quit(user);
         return;
       }

    delete_verify(user);
    goto EMAIL2_PASS;
   }

  /* If user is coming from a login info prompt */
if (ustr[user].pause_login==1) {
  if (ustr[user].logging_in==11) {
     add_user(user);
     add_user2(user,0);
     return;
    }

  if (ustr[user].logging_in==12) {
     add_user(user);
     add_user2(user,1);
     return;
    }
}

  /*-------------------------------------*/
  /* get login name                      */
  /*-------------------------------------*/

  if (ustr[user].logging_in==3) 
    {
      name[0]=0;
      sscanf(inpstr,"%s",name);
	strtolower(name);
	reset_user_struct(user);
        if (!strcmp(name,"quit")) {
                write_str(user,IF_QUIT_LOGIN);
                user_quit(user);  return;
                }      
        if (!strcmp(name,"who")) {
             if (LONGLOGIN_WHO) {
                t_who(user,"",0);
                }
             else {
                write_str(user,"");
                newwho(user);
                write_str(user,"");
               }
		telnet_echo_on(user);
                write_str_nr(user,SYS_LOGIN);
		telnet_write_eor(user);
                return;
                }      
      if (name[0]<32 || !strlen(name)) 
        {
	 write_str_nr(user,SYS_LOGIN);
	 telnet_write_eor(user);
	 return;
	}
	
      if (strlen(name)<3) 
        {
        write_str(user,SYS_NAME_SHORT);
        write_str(user," ");
        write_str(user," Note: Your telnet client might not support line mode of operation.");
        write_str(user,"       If this is true, you might not be able to use this service.");
        write_str(user," ");
        attempts(user);
        return;
	}
	
     if (strlen(name)>NAME_LEN-1) 
       {
	write_str(user,SYS_NAME_LONG);
	attempts(user);  
	return;
       }
	
	/* see if only letters in login */
     for (f=0; f<strlen(name); ++f) 
       {
         if (!isalpha((int)name[f]) || name[f]<'A' || name[f] >'z') 
           {
	     write_str(user,ONLY_LET);
	     attempts(user);  
	     return;
	   }
       }

     /* Check to see if name is not allowed */
     strcpy(tempname,name);
     if (check_nban(tempname,ustr[user].site) == 1) {
       write_str(user,IS_BANNED);
       attempts(user);
       return;
       }


     strcpy(ustr[user].login_name,tempname);

     /* Check to see if this user is returning with a password   */
     /* we emailed him. If so, check their site against new-bans */
     /* If they pass ask them for the password                   */
     if (check_verify(user,0) == 1) {
	     if (check_restriction(user, NEW) == 1)
		{
                sprintf(mess,"%s: Creation attempt (%s), BANNEWed site %s:%s\n",get_time(0,0),ustr[user].login_name,ustr[user].site,
		ustr[user].net_name);
		print_to_syslog(mess);
                ustr[user].login_pass[0]=0;
                ustr[user].password[0]=0;
		attempts(user);
		return;
		}
        sprintf(filename,"%s",VERINSTRUCT2);
        /* Print out the welcome back and password instructions file */
        /* to the user */
     cat(filename,user,0);
     
     write_str_nr(user,PASEMAIL_VERIFY); 
	telnet_write_eor(user);
     ustr[user].logging_in=10;
     return;

       }

     if (allow_new==1) {
       /* See if user already exists */
       if (!check_for_user(ustr[user].login_name)) {
        /* First check if new users are banned from this site */
        if (check_restriction(user, NEW) == 1)
         {
                sprintf(mess,"%s: Creation attempt (%s), BANNEWed site %s:%s\n",get_time(0,0),ustr[user].login_name,ustr[user].site,
		ustr[user].net_name);
		print_to_syslog(mess);
          attempts(user);
          return;
         }
        sprintf(filename,"%s",VERIINSTRUCT); 
        /* Print out the email verification instructions file */
        /* to the user */
        cat(filename,user,0);
     
        write_str_nr(user,EMAIL_VERIFY); 
	telnet_write_eor(user);
        ustr[user].logging_in=5;
        return;
        }
       }

     ustr[user].logging_in=2;
     strtolower(ustr[user].login_name);
 
   /* See if we need to hide the password */    
   t_ustr.passhid = 0;
   read_user(ustr[user].login_name);
   ustr[user].passhid = t_ustr.passhid;  

     write_str_nr(user,SYS_PASSWD_PROMPT);
	telnet_echo_off(user);  
	telnet_write_eor(user);
     return;
   }

  /*-------------------------------------*/
  /* get first password                  */
  /*-------------------------------------*/

  if (ustr[user].logging_in==2) 
    {
      passwd[0]=0;
      telnet_echo_on(user);
      sscanf(inpstr,"%s",passwd);
      
      if (passwd[0]<32 || !strlen(passwd) || (strlen(passwd) < 3))
        {
	if (ustr[user].promptseq==0 && ustr[user].passhid==1) write_str(user,"");
          write_str(user,SYS_PASSWD_INVA);
	  write_str_nr(user,SYS_PASSWD_PROMPT);
	  telnet_echo_off(user);
	  telnet_write_eor(user);
	  return;
        }
        
      if (strlen(passwd)>NAME_LEN-1) 
        {
	if (ustr[user].promptseq==0 && ustr[user].passhid==1) write_str(user,"");
	  write_str(user,SYS_PASSWD_LONG);
	  write_str_nr(user,SYS_PASSWD_PROMPT);  
          telnet_echo_off(user);
	  telnet_write_eor(user);
	  return;
	}
     }

  /*-------------------------------------------------------------*/
  /* convert name & passwd to lowercase and encrypt the password */
  /*-------------------------------------------------------------*/
  
  strtolower(ustr[user].login_name);
  strtolower(passwd);
   
  if (strcmp(ustr[user].login_name, passwd) ==0)
    {
	if (ustr[user].promptseq==0 && ustr[user].passhid==1) write_str(user,"");
       write_str(user,PASS_NO_NAME1);         
       write_str(user,PASS_NO_NAME2);         
       attempts(user);                                        
       return;                                                
    }
    
  st_crypt(passwd);                                  
  strcpy(ustr[user].login_pass,passwd);              


  /*-------------------------------------------------------------------------*/
  /* check for user and login info                                           */
  /*-------------------------------------------------------------------------*/

  if (read_to_user(ustr[user].login_name,user) )
    {   
                                                               
     /*---------------------------------------------*/         
     /* The file exists, so the user has an account */        
     /*---------------------------------------------*/        
     su = t_ustr.super;                                       

     if ( strcmp(ustr[user].login_pass,ustr[user].password) )        
       {
        time(&tm);
	if (ustr[user].promptseq==0 && ustr[user].passhid==1) write_str(user,"");
        write_str(user,PASS_NOT_RIGHT);
	sprintf(buf,"%s",ctime(&tm));
        buf[strlen(buf)-6]=0; /* get rid of nl and year */
        sprintf(z_mess,"%s: wrong passwd on %s from %s\n",
                buf, ustr[user].login_name, ustr[user].site);
        ustr[user].area = -1;
        print_to_syslog(z_mess);

        attempts(user);
        return;
       }
      else
       {
     telnet_echo_on(user);
     time(&tm);
     tm_then=((time_t) ustr[user].rawtime);

if (ustr[user].super < MIN_HIDE_LEVEL)
  {
    ustr[user].vis=1;
  }

write_str(user,"+---------------------------------------------------------------------------+");
if (ustr[user].vis)
  {write_str_nr(user,"Status [^HYVisible,^ ");}
  else
  {write_str_nr(user,"Status [^HRInVisible,^ ");}
  
if (ustr[user].shout)
  {write_str_nr(user,"^HYUnmuzzled,^ ");}
  else
  {write_str_nr(user,"^HRMuzzled,^ ");}

if (ustr[user].suspended)
  {write_str_nr(user,"^HRXcommed,^ ");}
  else
  {write_str_nr(user,"^HYUNxcommed,^ ");}

if (ustr[user].frog)
  {write_str_nr(user,"^HRFrogged,^ ");}
  else
  {write_str_nr(user,"^HYUNfrogged,^ ");}

if (ustr[user].anchor)
  {write_str_nr(user,"^HRAnchored,^ ");}
  else
  {write_str_nr(user,"^HYUNanchored,^ ");}
 
if (ustr[user].gagcomm)
  {write_str(user,"^HRGagcommed^]");}
  else
  {write_str(user,"^HYUNgagcommed^]");}

if (astr[ustr[user].area].private)
  {
   ustr[user].area=INIT_ROOM;
   write_str(user,IS_PRIVATE);
  }
write_str(user,"");
sprintf(z_mess,"Welcome to ^%s^ %s %s",SYSTEM_NAME,ranks[ustr[user].super],ustr[user].say_name);
write_str(user,z_mess);  
write_str(user,"");

write_str(user," Last login from..");
sprintf(z_mess,"  ^%s^ (^HG%s^)",ustr[user].last_name,ustr[user].last_site);
write_str(user,z_mess);
sprintf(z_mess,"  %s ago.",converttime((long)((tm-tm_then)/60)));
write_str(user,z_mess);
write_str(user,"");
write_str(user," This login from..");
sprintf(z_mess,"  ^%s^ (^HG%s^)",ustr[user].net_name,ustr[user].site);
write_str(user,z_mess);
write_str(user,"");
quotes(user);
write_str(user,"+---------------------------------------------------------------------------+");   
        strcpy(ustr[user].last_date, ctime(&tm));
         ustr[user].last_date[24]=0;
        strcpy(ustr[user].last_site, ustr[user].site);
        strcpy(ustr[user].last_name, ustr[user].net_name);
        ustr[user].rawtime = tm;
        ustr[user].logging_in=11;
	if (how_many_users(ustr[user].name) > 1) {
         write_str(user,ALREADY_ON);
	 if (!quit_multiples(user)) {
		user_quit(user);
		return;
		}
	 }
	if (ustr[user].pause_login==1) {
         write_str_nr(user,"--- Press <ENTER> to complete login ---");
	 telnet_write_eor(user);
	}
	else {
	 add_user(user);
	 add_user2(user,0);
	 }
        return;
       }
    }
   else
    {
     /*---------------------------------------------*/
     /* The file does not exists, so the user has   */      
     /* no previous account                         */      
     /*---------------------------------------------*/      
     if (check_restriction(user, NEW) == 1)
       {
                sprintf(mess,"%s: Creation attempt (%s), BANNEWed site %s:%s\n",get_time(0,0),ustr[user].login_name,ustr[user].site,
		ustr[user].net_name);
		print_to_syslog(mess);
        attempts(user);
        return;
       }

     if (!allow_new)
        {
          write_str(user,NO_NEW_LOGIN);
  	  attempts(user);
	  return;
         }
     
     if (system_stats.quota > 0 && system_stats.new_users_today >= system_stats.quota)
       {
         write_str(user,"=====================================================");
         write_str(user,"We are currently using a maximum quota for new users.");
         write_str(user,"The limit for today has been reached.");
         write_str(user,"This will be reset at midnight.");
         write_str(user,"=====================================================");
         attempts(user);                                   
	 return;                                             
       }                                                   

         
     write_str(user,NEW_USER_MESS); 

     write_str(user,"");
     write_str(user,"");

     sprintf(filename,"%s",AGREEFILE); 
      /* Print out the terms of agreement file to the user */
     cat(filename,user,0);

     write_str(user,"");
     write_str(user,"");
     
     write_str_nr(user,PASS_VERIFY); 
     telnet_write_eor(user);
     telnet_echo_off(user);
     strcpy(ustr[user].login_pass,passwd);                   
     ustr[user].logging_in=1;                              
     return;                                                
    }                                                         
                                                        
                                                             
  /*------------------------------------------------------------------------------*/
  /* For new users, double check the password to make sure they entered it right  */
  /* and save the new account if allowed                                          */
  /*------------------------------------------------------------------------------*/

  CHECK_PASS:
     telnet_echo_on(user);
     sscanf(inpstr,"%s",passwd);
     strtolower(passwd);
     st_crypt(passwd);                                    
      
     if (strcmp(ustr[user].login_pass,passwd)) 
       {
         write_str(user,NON_VERIFY);
         ustr[user].login_pass[0]=0;
         attempts(user);  
         return;
       }

     EMAIL2_PASS:
     write_str(user," ");
     telnet_echo_on(user);   
     write_str(user,NEW_CREATE);   

        time(&tm);                                            
	sprintf(buf,"%s",ctime(&tm));
        buf[strlen(buf)-6]=0; /* get rid of nl and year */

     sprintf(z_mess,"%s: NEW USER created - %s\n",buf,ustr[user].login_name);
     print_to_syslog(z_mess);
     strcpy(ustr[user].creation, ctime(&tm));
     ustr[user].creation[24]=0;
     strcpy(ustr[user].password,passwd);   
     init_user(user);
     copy_from_user(user);                        
     write_user(ustr[user].login_name);            
     ustr[user].logging_in=12;
     if (ustr[user].pause_login==1) {
      write_str_nr(user,"--- Press <ENTER> to complete login ---");
      telnet_write_eor(user);
     }
     else {
      add_user(user);
      add_user2(user,1);
      }
     return;
}


/*-----------------------------------------------------*/
/*   check to see if user has had max login attempts   */
/*-----------------------------------------------------*/
void attempts(int user)
{
if (!--ustr[user].attleft) {
	write_str(user,ATTEMPT_MESS);
	user_quit(user); 
	return;
	}

reset_user_struct(user);
ustr[user].logging_in=3;
telnet_echo_on(user);
write_str_nr(user,SYS_LOGIN);
telnet_write_eor(user);
}
	

/*------------------------------------------------------*/
/* write time system went up or down to syslog file     */
/*------------------------------------------------------*/
void sysud(int ud, int user)
{
char filename[FILE_NAME_LEN];
FILE *tfp;

/* if ud=1 system is coming up, else if 0, going down */

if (!syslog_on) return;

if (ud) puts("Logging startup...");

/* write to file */
if (ud)  {
  time(&start_time);
  sprintf(mess,"%s: BOOT on port %d using datadir: %s\n",get_time(start_time,0),PORT,datadir);
  }
else {
      if (user==-1) {
      if (treboot)
       sprintf(mess,"%s: REBOOT by the talker\n",get_time(0,0));
      else
       sprintf(mess,"%s: SHUTDOWN by the talker\n",get_time(0,0));
      }
      else {
      if (treboot)
       sprintf(mess,"%s: REBOOT by %s\n",get_time(0,0),ustr[user].say_name);
      else
       sprintf(mess,"%s: SHUTDOWN by %s\n",get_time(0,0),ustr[user].say_name);
      }
     }

print_to_syslog(mess);

sprintf(filename,"%s.pid",thisprog);

if (ud) {
  if (!(tfp=fopen(filename,"w"))) return;

  fprintf(tfp,"%u",(unsigned int)getpid());
  fclose(tfp);
  }
else {
  remove(filename);
  }

}

/*-------------------------------------------------------------------------*/
/* locate a free position in the array to place an incoming user.  check   */
/* both the wizard and user allocations to place the user.                 */
/*-------------------------------------------------------------------------*/
int find_free_slot(char port)
{
int u;

/*-------------------------------------------------*/
/* check for full system                           */
/*-------------------------------------------------*/

if ( (port == '1' && num_of_users >= NUM_USERS) || 
     (port == '2' && num_of_users >= MAX_USERS) )
  {
   return -1;
  }

/*-------------------------------------------------*/
/* find a free slot                                */
/*-------------------------------------------------*/

if (port=='3') {
   for (u=0;u<MAX_WHO_CONNECTS;++u)
     {
      if (strlen(whoport[u].site) < 2)
        return u;
     }
   return -1;
  }
else if (port=='4') {
   for (u=0;u<MAX_WWW_CONNECTS;++u)
     {
      if (strlen(wwwport[u].site) < 2)
        return u;
     }
   return -1;
  }
else {
   for (u=0;u<MAX_USERS;++u) 
     {
      if (ustr[u].area== -1 && !ustr[u].logging_in) 
        return u;
     }
  return -1;
  }

}

/* Check if user is already online..if so, terminate old user */
int quit_multiples(int user)
{
int u,i=0;
int reload1=0;

	for (u=0;u<MAX_USERS;++u) {
	  if (!strcmp(ustr[u].name,ustr[user].name) && 
	      ustr[u].area!= -1 &&
	      u != user ) 
	    {
	      write_str(u,TERMIN);
              reload1=1;
              /* User was hung, but we're gonna save the following into their */
	      /* new login: tell buffer and .call user */

		for (i=0; i < NUM_LINES+1; ++i)
		 strcpy(ustr[user].conv[i],ustr[u].conv[i]);

		ustr[user].conv_count = ustr[u].conv_count;
		strcpy(ustr[user].phone_user,ustr[u].phone_user);
		user_quit(u); 
	    } /* end of if */
	} /* end of for */

/* reset user structure */
if (reload1) {
	if (!read_to_user(ustr[user].login_name,user)) return 0;
	/* this is a cheesy way to do this, but it works	*/
	/* the mutter string here is used to tell add_user()	*/
	/* to replace room with RE-LOGIN in the user's online	*/
	/* announcement						*/
	strcpy(ustr[user].mutter,"RE-LOGIN");
	}
return 1;
}


/*-----------------------------------------------------------*/
/* Initialize a user for the system or set up data for a     */
/* new user if he can get on.                                */
/*-----------------------------------------------------------*/
void add_user(int user)
{
int v;
char room[32];
time_t tm;
#if defined(WIN32) && !defined(__CYGWIN32__)
unsigned long arg = 1;
#endif

 if (ustr[user].attach_port == '2' && ustr[user].super < WIZ_LEVEL)
   {
     write_str(user,NO_WIZ_ENTRY);
     user_quit(user);
     return;
   }
 
 ustr[user].locked=           0;
 
 ustr[user].clrmail=          -1;
 ustr[user].time=             time(0);
 ustr[user].invite=           -1;
 ustr[user].last_input=       time(0);
 ustr[user].logging_in=       0;
 ustr[user].file_posn=        0;
 ustr[user].pro_enter=        0;
 ustr[user].t_ent=            0;
 ustr[user].t_num=            0;
 ustr[user].t_name[0]=        0;
 ustr[user].t_host[0]=        0;
 ustr[user].t_ip[0]=          0;
 ustr[user].t_port[0]=        0;
 ustr[user].roomd_enter=      0;
 ustr[user].vote_enter=       0;
 ustr[user].warning_given=    0;
 ustr[user].needs_hostname=   0;
 
 if (strcmp(ustr[user].mutter,"RE-LOGIN")) ustr[user].conv_count=       0;
 ustr[user].cat_mode =        0;
 ustr[user].numbering =       0;

 if (strcmp(ustr[user].mutter,"RE-LOGIN")) ustr[user].phone_user[0] =   0;
 ustr[user].real_id[0] =      0;
 ustr[user].afkmsg[0] =       0;
 ustr[user].rwho=             1;
 ustr[user].tempsuper=        0;

if (strcmp(ustr[user].mutter,"RE-LOGIN")) {
 for (v=0; v<NUM_LINES; v++)
     ustr[user].conv[v][0]=0;
}

num_of_users++;

if (!strcmp(ustr[user].name,BOT_ID)) bot=user;

#if defined(WIN32) && !defined(__CYGWIN32__)
ioctlsocket(ustr[user].sock, FIONBIO, &arg);
#else
fcntl(ustr[user].sock, F_SETFL, NBLOCK_CMD); /* set socket to non-blocking */
#endif

/* send room details to user */
time(&tm);
strcpy(ustr[user].last_date,  ctime(&tm));
 ustr[user].last_date[24]=0;
strcpy(ustr[user].last_site, ustr[user].site);
strcpy(ustr[user].last_name, ustr[user].net_name);
ustr[user].rawtime = tm;

copy_from_user(user);                          
write_user(ustr[user].login_name);        

look(user,""); 
alert_check(user,0);

/*----------------------------------*/
/* If user is new, tell all wizzes  */
/*----------------------------------*/
if (ustr[user].numcoms==0) {

sprintf(mess,NEW_USER_TO_WIZ,ustr[user].say_name,ustr[user].net_name,ustr[user].site);
   writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, NONE, 0);
   write_str(user,NEW_HELP);

   if (autopromote == 1)
    write_str(user,NEW_HELP2);

   mess[0]=0;
   }

check_mail(user);

/* If user logging in again from a hung connection, make the spot where the */
/* room name usually goes, say "RE-LOGIN" instead                           */
if (strcmp(ustr[user].mutter,"RE-LOGIN")) {
if (astr[ustr[user].area].hidden)
    sprintf(room," ? ");
else
    sprintf(room,"%s",astr[ustr[user].area].name);
}
else {
 strcpy(room,"RE-LOGIN");
 }
 ustr[user].mutter[0] =       0;

/* send message to other users and to file */
if (ustr[user].super >= WIZ_LEVEL) 
  {
   sprintf(mess, ANNOUNCEMENT_HI, ustr[user].say_name, ustr[user].desc,room);
   writeall_str(mess, 0, user, 0, user, NORM, LOGIO, 0);
  }
else 
  {
   sprintf(mess, ANNOUNCEMENT_LO, ustr[user].say_name, ustr[user].desc,room);
   writeall_str(mess, 0, user, 0, user, NORM, LOGIO, 0);
  }

/* stick signon in file */
syssign(user,1);
}

/* Misc add user stuff */
void add_user2(int user, int mode)
{

if (mode==0) {
     system_stats.logins_today++;
     system_stats.logins_since_start++;
     ustr[user].logging_in=0;
     }
else if (mode==1) {
     system_stats.logins_today++;    
     system_stats.logins_since_start++;
     system_stats.new_since_start++;
     system_stats.new_users_today++;
     ustr[user].logging_in=0;
     }

}


/** page a file out to a socket **/
int cat_to_sock(char *filename, int accept_sock)
{
int n;
char line[257];
FILE *fp;

if (!(fp=fopen(filename,"rb"))) 
  {
   print_to_syslog("Can't open binary file!\n");
   return 0;
  }

line[0]=0;

/*
fread(line, 1, 257, fp);

while(!feof(fp)) {
	        S_WRITE(accept_sock,line,strlen(line));

  fread(line, 1, 256, fp);
*/

while ((n = fread(line, 1, sizeof line,  fp)) != 0) {
   S_WRITE(accept_sock,line,n);
   } /* end of feof */

fclose(fp);

return 1;
}

/*** page a file out to user ***/
int cat(char *filename, int user, int line_num)
{
int num_chars=0,lines=0,retval=1;
FILE *fp;
int max_lines = 25;
int line_pos = 0;
int i = 0;
char leader[17];

if (!(fp=fopen(filename,"r"))) 
  {
   ustr[user].file_posn  = 0;  
   ustr[user].line_count = 0;
   return 0;
  }

if (line_num == 1)
  ustr[user].number_lines = 1;

  
/* jump to reading posn in file */
if (line_num != -1) 
  {
    fseek(fp,ustr[user].file_posn,0);
    max_lines = ustr[user].rows;
    line_pos = ustr[user].line_count;
  }
 else
  {
    max_lines = 999;
    line_num = 0;
  }
  
if (max_lines < 5 || max_lines > 999)
   max_lines = 25;

/* loop until end of file or end of page reached */
mess[0]=0;
fgets(mess, sizeof(mess)-25, fp);
		   strcpy(mess,check_var(mess,SYS_VAR,SYSTEM_NAME));
		   strcpy(mess,check_var(mess,USER_VAR,ustr[user].say_name));

if (!ustr[user].cols) ustr[user].cols=80;

while(!feof(fp) && lines < max_lines) 
  {
   line_pos++;
  
   i = strlen(mess);
   lines      += i / ustr[user].cols + 1;
   num_chars  += i;
   
   if (ustr[user].number_lines) {
    if (ustr[user].numbering > 0)
     sprintf(leader,"%-3d ",ustr[user].numbering);
    else
     sprintf(leader,"%-3d ",line_pos);
    }
    else {
          leader[0]=0;
         }

   mess[i-1] = 0;      /* remove linefeed */
   sprintf(t_mess,"%s%s",leader, mess);
    
   write_str(user,t_mess);
   if (ustr[user].numbering > 0) ustr[user].numbering++;
   fgets(mess, sizeof(mess)-25, fp);
		   strcpy(mess,check_var(mess,SYS_VAR,SYSTEM_NAME));
		   strcpy(mess,check_var(mess,USER_VAR,ustr[user].say_name));
  }
  
if (user== -1) goto SKIP;

if (feof(fp)) 
  {
   ustr[user].number_lines = 0;
   ustr[user].file_posn    = 0;  
   ustr[user].line_count   = 0;
   ustr[user].numbering    = 0;
   noprompt=0;  
   retval=2;
  }
else  
  {
   /* store file position and file name */
   ustr[user].file_posn += num_chars;
   ustr[user].line_count = line_pos;
   strcpy(ustr[user].page_file,filename);
   write_str_nr(user,CONF_PROMPT);
	telnet_write_eor(user);
   noprompt=1;
  }
  
SKIP:
  FCLOSE(fp);
  return retval;
}


/*** get user number using name ***/
int get_user_num(char *i_name, int user)
{
int u;
int found = 0, last = -1;

strtolower(i_name);

t_mess[0] = 0;

for (u=0; u<MAX_USERS; ++u) 
	if ( !strcmp(ustr[u].name,i_name) && ustr[u].area != -1) 
	return u;
	
for (u=0; u<MAX_USERS; ++u)
  {
   if (instr2(0, ustr[u].name, i_name, 0) != -1)
    { 
      strcat(t_mess, ustr[u].say_name);
      strcat(t_mess, " ");
      found++;
      last= u;
    }
  }

if (found == 0) return -1;

if (found >1) 
  {
   sprintf(mess, NOT_UNIQUE, t_mess);
   write_str(user,mess);
   return -1;
  }
 else
  return last;
}

/*** get user number using name ***/
int get_user_num_exact(char *i_name, int user)
{
int u;

strtolower(i_name);
t_mess[0] = 0;

for (u=0;u<MAX_USERS;++u) 
	if (!strcmp(ustr[u].name,i_name) && ustr[u].area != -1) 
	return u;

return -1;
}

/*** get user number using name ***/
int how_many_users(char *name)
{
int u;
int num=0;

strtolower(name);

for (u=0;u<MAX_USERS;++u) 
	if (!strcmp(ustr[u].name,name) && ustr[u].area != -1) num++;
	
return num;
}



/*** removes first word at front of string and moves rest down ***/
void remove_first(char *inpstr)
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



/*** sends output to all users if area==0                       ***/
/*** else only users in same area                               ***/
/*----------------------------------------------------------------*/
/* str  - what to print                                           */
/* area - -1 = login, -2 = pre-login, -5 = wizards only,          */
/*         0 and above - any room                                 */
/* user - the one who did it                                      */
/* send_to_user = 0 all on system 1 = in room                     */
/* who_did = user                                                 */
/* mode = normal - 0, bold = 1                                    */
/* type = message type                                            */
/*----------------------------------------------------------------*/

void writeall_str(char *str, int area, int user, int send_to_user, int who_did, int mode, int type, int sw)
{
int u,i=0,z=0;
int gagged=0,gravoked=0;
char str2[ARR_SIZE];

str2[0]=0;

if (!ustr[who_did].vis || 
     type == ECHOM     || 
     type == BCAST     || 
     type == KILL      || 
     type == MOVE      || 
     type == PICTURE   ||
     type == GREET)
  {
   strcpy(str2,"<");
   strcat(str2,ustr[who_did].say_name);
   strcat(str2,"> ");
  }
  
strcat(str2,str);

/* if (str[0]!=' ') str[0]=toupper(str[0]); */

/*---------------------------------------*/
/* added for btell                       */
/*---------------------------------------*/

 if (area == WIZ_ONLY)
   {
     for (u=0;u<MAX_USERS;++u)
       {
        if (!user_wants_message(u,type)) continue;

/* WRITE CODE TO CHECK THIS FOR ONLY TYPE OF WIZT */
if (type == WIZT) {
/* Check if command was revoked from user */
 for (z=0;z<MAX_GRAVOKES;++z) {
        if (!isrevoke(ustr[u].revokes[z])) continue;
        if (strip_com(ustr[u].revokes[z])==get_com_num_plain(".wiztell")) {
                gravoked=1; break;
        }
   }
if (gravoked==1) { gravoked=0; continue; }
gravoked=0;
z=0;

/* Check if command was granted to user */
 if (ustr[u].super < WIZ_LEVEL && !ustr[u].logging_in && u != user && ustr[u].area!=-1) {
  for (z=0;z<MAX_GRAVOKES;++z) {
                print_to_syslog("looking for is granted!\n");
        if (!isgrant(ustr[u].revokes[z])) continue; 
                print_to_syslog("past is grant!\n");
        if (strip_com(ustr[u].revokes[z])==get_com_num_plain(".wiztell")) {
                print_to_syslog("granted!\n");
                gravoked=1; break;
          }
  } /* end of for */
 } /* end of if lower level */
} /* end of if WIZT */
        
	if (((ustr[u].super >= WIZ_LEVEL) || (gravoked==1)) && !ustr[u].logging_in && u != user && ustr[u].area!=-1)  
	  {
	   if (mode == BOLD)
	     {
	       write_hilite(u,str);
	     }
	    else
	     {
	      write_str(u,str);
	     }
          } /* end of if wiz level */
        } /* end of for */
    return;
   } /* end of if area */
   
/*---------------------------------------*/
/* normal write to all users             */
/*---------------------------------------*/
   
for (u=0; u<MAX_USERS; ++u) {
        if ((ustr[u].logging_in==11) || (ustr[u].logging_in==12)) continue;

	if ((!send_to_user && user==u) ||  ustr[u].area== -1) continue;
	    
	if (!user_wants_message(u,type)) continue;
    
        if (!strcmp(ustr[user].mutter,ustr[u].say_name)) continue;

        /* An incoming connection message for monitoring people */
        if (area == -2) {
           if (ustr[u].monitor > 1) {
            if (mode == BOLD)
             write_hilite(u,str);
            else
             write_str(u,str);
            }
           continue;
          }

        /* A normal login */
        if (type == LOGIO)
           {
                write_str(u,str);
                continue;
           }

        /* See if u has user gagged */
        for (i=0; i<NUM_IGN_FLAGS; ++i) {
           if (type==gagged_types[i]) {
             if (!gag_check(user,u,1)) { gagged=1; break; }
             }
          }
        if (gagged) {
          gagged=0; i=0;
          continue;
          }
        i=0;

        if (ustr[u].area==ustr[user].area || !area)  
	  { 
	    if((ustr[u].monitor==1) || (ustr[u].monitor==3))
	      {
	       if (mode == BOLD)
	         {
	          write_hilite(u,str2);
	         }
	        else if (mode == BEEPS)
	         {
                  if (user_wants_message(u,BEEPS))
                   strcat(str2,"\07");
	          write_str(u,str2);
	         }
	        else
	         {
	          write_str(u,str2);
	         }
	      }
	    else
	      {
	       if (mode == BOLD)
	         {
	          write_hilite(u,str);
	         }
	        else if (mode == BEEPS)
	         {
                  if (user_wants_message(u,BEEPS))
                   strcat(str,"\07");
	          write_str(u,str);
	         }
	        else
	         {
	          write_str(u,str);
	         }
	      }
	  }
	}
}


/*** Handle macros... if any ***/
/* returning 0 is an error, but the command will continue through the */
/* main loop, returning -1 is an error and the command is discarded   */
int check_macro(int user, char *inpstr)
{
int a=0;
int arg1=0;
int arg2=0;
int arg3=0;
int i=0;
int j=0;
int found=0;
int noarg=0;
char line[ARR_SIZE];
char outstr[ARR_SIZE];
char tp[3];
char argu[3][ARR_SIZE];

tp[0]=0;

sscanf(inpstr,"%s ",line);
for (i=0;i<NUM_MACROS;++i) {
   if (!strcmp(ustr[user].Macros[i].name,line)) {
     found=1;
     break;
    }
  }
if (!found) return 0;
found=0;

outstr[0]=0;
/* strcpy(outstr,line); */
remove_first(inpstr);
if (!strlen(inpstr)) noarg=1;
else {
 argu[0][0]=0;
 argu[1][0]=0;
 argu[2][0]=0;
 argu[3][0]=0;  /* last input */

 /* scan arguments into seperate arrays */
 while (strlen(inpstr)) {
   if (a<3) {
    sscanf(inpstr,"%s ",argu[a]);
    argu[a][80]=0;
    }
   else {
    strcpy(line,inpstr);
    break;
    }
   if (a<3) line[0]=0;
   remove_first(inpstr); 
   a++;
  } /* end of while */
 a=0;
} /* end of else */


/* go through the input string character by character */
for (j=0;j<strlen(ustr[user].Macros[i].body);++j) {
  if (ustr[user].Macros[i].body[j]=='$') {
     j++;
     if (ustr[user].Macros[i].body[j]=='1') {
        if (noarg) {
          write_str(user,"Macro was made to accept an argument. None was given.");
          return -1;
          }
        /* only set if coming from a lower argument substition */
        if (a<1 && strlen(argu[0])) a=1;
        arg1++;
         /* only can use same substitution 3 times in same macro */
        if (arg1<=3)
         strcat(outstr,argu[0]);
        else { }
        continue;
       }
     else if (ustr[user].Macros[i].body[j]=='2') {
        if (noarg) {
          write_str(user,"Macro was made to accept an argument. None was given.");
          return -1;
          }
        /* only set if coming from a lower argument substition */
        if (a<2 && strlen(argu[1])) a=2;
        arg2++;
         /* only can use same substitution 3 times in same macro */
        if (arg2<=3)
         strcat(outstr,argu[1]);
        else { }
        continue;     
       }
     else if (ustr[user].Macros[i].body[j]=='3') {
        if (noarg) {
          write_str(user,"Macro was made to accept an argument. None was given.");
          return -1;
          }
        /* only set if coming from a lower argument substition */
        if (a<3 && strlen(argu[2])) a=3;
        arg3++;
         /* only can use same substitution 3 times in same macro */
        if (arg3<=3)
         strcat(outstr,argu[2]);
        else { }
        continue;     
       }
     else {
        /* $ is not a substitution, just a $ */
        j--;
        tp[0]=ustr[user].Macros[i].body[j];
        tp[1]=0;
        strcat(outstr,tp);
        continue;
       }
    } /* end of if $ */
  else {
   /* just a normal character */
   tp[0]=ustr[user].Macros[i].body[j];
   tp[1]=0;
   strcat(outstr,tp);
   tp[0]=0;
   continue;
  } /* end of else */
 } /* end of for */
j=0;

/* copy extra arguments onto end depending  */
/* on how many substitions were given       */
if (!a && !noarg) {
  /* none given to match macro, copy whole inpstr to end */
  strcat(outstr," ");
  strcat(outstr,argu[0]);
  if (strlen(argu[1])) strcat(outstr," ");
  strcat(outstr,argu[1]);
  if (strlen(argu[2])) strcat(outstr," ");
  strcat(outstr,argu[2]);
  if (strlen(line)) strcat(outstr," ");
  j = ARR_SIZE - (strlen(outstr) + 1);
  line[j]=0;
  strcat(outstr,line);
  }
else if (a==1 && !noarg) {
  /* 1 given to match macro, copy last 3 to end */
  if (strlen(argu[1])) strcat(outstr," ");
  strcat(outstr,argu[1]);
  if (strlen(argu[2])) strcat(outstr," ");
  strcat(outstr,argu[2]);
  if (strlen(line)) strcat(outstr," ");
  j = ARR_SIZE - (strlen(outstr) + 1);
  line[j]=0;
  strcat(outstr,line);
  }
else if (a==2 && !noarg) {
  /* 2 given to match macro, copy last 2 to end */
  if (strlen(argu[2])) strcat(outstr," ");
  strcat(outstr,argu[2]);
  if (strlen(line)) strcat(outstr," ");
  j = ARR_SIZE - (strlen(outstr) + 1);
  line[j]=0;
  strcat(outstr,line);
  }
else if (a==3 && !noarg) {
  /* 3 given to match macro, copy last 1 to end */
  if (strlen(line)) strcat(outstr," ");
  j = ARR_SIZE - (strlen(outstr) + 1);
  line[j]=0;
  strcat(outstr,line);
  }

/* copy final processed string back to main input string */
strcpy(inpstr,outstr);

 argu[0][0]=0;
 argu[1][0]=0;
 argu[2][0]=0;
 argu[3][0]=0;  /* last input */
 line[0]=0; outstr[0]=0; tp[0]=0;
 arg1=0; arg2=0; arg3=0; a=0; i=0; j=0; found=0; noarg=0;

return 1;
}


/*---------------------------------------------------------------*/
/* this is a simple token parser. I, Cygnus, have made it more   */
/* complex (of course). No, really, it's cool. It takes abbrevia */
/* -tions or/and the first command on and checks to see if it is */
/* a valid command. And NOW since command structure abbreviation */
/* defaults are copied to a new user, it looks at them instead   */
/*---------------------------------------------------------------*/
int get_com_num(int user, char *inpstr)
{
char comstr[ARR_SIZE];
char tstr[ARR_SIZE+25];
int f=0;
int i=0;
int found=0;

if (ustr[user].white_space)
 {
  while(inpstr[0] == ' ') inpstr++;
 }

comstr[0]=inpstr[0];
comstr[1]=0;

/* Bot commands start with a _ */
if (!strcmp(ustr[user].name,BOT_ID) ||
    !strcmp(ustr[user].name,ROOT_ID)) {
  if (!strcmp(comstr,"_")) {
    sscanf(inpstr,"%s",comstr); 
    for (f=0; sys[f].su_com != -1; ++f)
      if (!instr2(0,botsys[f].command,comstr,0) && strlen(comstr)>1) return f;
    return -1;
   }
 } /* end of if bot */

if (!strcmp(comstr,"."))
 {
   sscanf(inpstr,"%s",comstr);
 }
 else
 {
   if (ustr[user].abbrs)
     {

     /* check user abbreviations first */
                        for (found=0, i=0;i<NUM_ABBRS && found == 0;++i)
                        {
                              if (comstr[0] == ustr[user].custAbbrs[i].abbr[0])
                                {
                                        strcpy(comstr, ustr[user].custAbbrs[i].com);
                                        found = 1;
                                }
                        }
                        if (!found)
                        {
                                return -1;
                        }
 
      tstr[0] = 0;
      inpstr[0] = ' ';
      strcpy(tstr,comstr);
      strcat(tstr,inpstr);
      strcpy(inpstr,tstr);
    }
 }

for (f=0; sys[f].su_com != -1; ++f)
	if (!instr2(0,sys[f].command,comstr,0) && strlen(comstr)>1) return f;
return -1;
}

int get_com_num_plain(char *inpstr)
{
int f=0;
  
for (f=0; sys[f].su_com != -1; ++f)
        if (!instr2(0,sys[f].command,inpstr,0) && strlen(inpstr)>1) return sys[f].jump_vector;

return -1;
}     

int get_rank(char *inpstr)
{
int f=0;
  
for (f=0; sys[f].su_com != -1; ++f)
        if (!instr2(0,sys[f].command,inpstr,0) && strlen(inpstr)>1) return sys[f].su_com;

return 0;
}     

/*---------------------------------------------------------*/
/* keep an audit trail of user logins and logoffs          */
/* in the system log file                                  */
/*---------------------------------------------------------*/
void syssign(int user, int onoff)
{
time_t tm;
char stm[30],other_user[256];
char filename[80];
FILE *fp;

/* write to file */
time(&tm);
strcpy(stm,ctime(&tm));
stm[strlen(stm)-6]=0; /* get rid of nl and year */
sprintf(mess,"%s:%1d: %s %s:%s:sock#%d\n",stm,onoff,ustr[user].name,ustr[user].site,ustr[user].net_name,ustr[user].sock);
print_to_syslog(mess);

 if (onoff==1) {

    sprintf(mess,"+++++ logon:%s %s %d", ustr[user].say_name,
		astr[ustr[user].area].name, ustr[user].vis);
    write_bot(mess);

  /* If user coming in in bot room, tell bot that user is here */
  if (!strcmp(astr[ustr[user].area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ came in:%s", ustr[user].say_name);
    write_bot(mess);
    }

  sprintf(filename,"%s",LASTLOGS);
  if (!(fp=fopen(filename,"a"))) {
       write_str(user,"Can't add you to lastlogs file.");
       return;
       }
  strcpy(other_user,ustr[user].say_name);
  strcat(other_user," ");
  strcat(other_user,ustr[user].desc);
  strcpy(other_user, strip_color(other_user));  
  sprintf(t_mess,"%s  %s\n",stm,other_user);
  other_user[0]=0;
  fputs(t_mess,fp);
  fclose(fp);

  midcpy(stm,stm,11,12);
  if (!strcmp(stm,"00"))
   logstat[0].logins++;
  else if (!strcmp(stm,"01"))
   logstat[1].logins++;
  else if (!strcmp(stm,"02"))
   logstat[2].logins++;
  else if (!strcmp(stm,"03"))
   logstat[3].logins++;
  else if (!strcmp(stm,"04"))
   logstat[4].logins++;
  else if (!strcmp(stm,"05"))
   logstat[5].logins++;
  else if (!strcmp(stm,"06"))
   logstat[6].logins++;
  else if (!strcmp(stm,"07"))
   logstat[7].logins++;
  else if (!strcmp(stm,"08"))
   logstat[8].logins++;
  else if (!strcmp(stm,"09"))
   logstat[9].logins++;
  else if (!strcmp(stm,"10"))
   logstat[10].logins++;
  else if (!strcmp(stm,"11"))
   logstat[11].logins++;
  else if (!strcmp(stm,"12"))
   logstat[12].logins++;
  else if (!strcmp(stm,"13"))
   logstat[13].logins++;
  else if (!strcmp(stm,"14"))
   logstat[14].logins++;
  else if (!strcmp(stm,"15"))
   logstat[15].logins++;
  else if (!strcmp(stm,"16"))
   logstat[16].logins++;
  else if (!strcmp(stm,"17"))
   logstat[17].logins++;
  else if (!strcmp(stm,"18"))
   logstat[18].logins++;
  else if (!strcmp(stm,"19"))
   logstat[19].logins++;
  else if (!strcmp(stm,"20"))
   logstat[20].logins++;
  else if (!strcmp(stm,"21"))
   logstat[21].logins++;
  else if (!strcmp(stm,"22"))
   logstat[22].logins++;
  else if (!strcmp(stm,"23"))
   logstat[23].logins++;

 }
 else {
  /* If user leaving from bot room, tell bot that user left */
  if (!strcmp(astr[ustr[user].area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ left:%s", ustr[user].say_name);
    write_bot(mess);
    }
 }

}


/*---------------------------------------------------------*/
/* keep an audit trail of user logins to the who and www   */
/* ports in the system log file                            */
/*---------------------------------------------------------*/
int log_misc_connect(int user, unsigned long addr, int type)
{
char stm[30];
static char buf[256];
static char namebuf[256];
struct hostent *he;
time_t tm;

/* write to file */
time(&tm);
strcpy(stm,ctime(&tm));
stm[strlen(stm)-6]=0; /* get rid of nl at end */

/* Resolve sock address to hostname, if cant copy failed message */
 he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
 if (he && he->h_name)
    strcpy(namebuf, he->h_name);
 else
    strcpy(namebuf, SYS_LOOK_FAILED);

/* Resolve to ip */
addr = ntohl(addr);
sprintf(buf,"%ld.%ld.%ld.%ld", (addr >> 24) & 0xff, (addr >> 16) & 0xff,
         (addr >> 8) & 0xff, addr & 0xff);

if (type==1) {
  sprintf(mess,"%s: WHO port connect %s:%s\n",stm,buf,namebuf);
  strcpy(whoport[user].site,buf);
  strcpy(whoport[user].net_name,namebuf);
  }
else if (type==2) {
  sprintf(mess,"%s: WWW port connect %s:%s\n",stm,buf,namebuf);
  strcpy(wwwport[user].site,buf);
  strcpy(wwwport[user].net_name,namebuf);
  }

print_to_syslog(mess);

if (type==1) {
  if (check_misc_restrict(whoport[user].sock,buf,namebuf) == 1) {
   sprintf(mess,"%s: WHO Connection attempt, RESTRICTed site %s:%s\n",get_time(0,0),
   buf,namebuf);
   print_to_syslog(mess);
   return -1;
   }
  else
   return 0;
  }
else if (type==2) {
  if (check_misc_restrict(wwwport[user].sock,buf,namebuf) == 1) {
   sprintf(mess,"%s: WWW Connection attempt, RESTRICTed site %s:%s\n",get_time(0,0),
   buf,namebuf);
   print_to_syslog(mess);
   return -1;
   }
  else
   return 0;
  }
return 1;
}


/*** log runtime errors in LOGFILE ***/
void logerror(char *s)
{
char line[ARR_SIZE];

s[500]=0;
sprintf(line,"ERROR: %s\n",s);
print_to_syslog(line);
}


/* mid copy copies chunk from string strf to string strt */
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


/*----------------------------------------------------------*/
/* given two string: ss and sf, and a position in ss,       */
/* determine if sf is present in ss                         */
/* return position if it is present, or -1 if it is not     */
/*                                                          */
/* note: this is obscure, but functional code               */
/*----------------------------------------------------------*/
int instr2(int pos, char *ss, char *sf, int mode)
{
int f;
int g;
int nofirst=0;

for (f=pos; *(ss+f); ++f) 
  {
	for (g=0;;++g) 
                {
		if (*(sf+g)=='\0' && g>0) 
                  {
                  return f;
                  }

		if (*(sf+g)!=*(ss+f+g)) 
                  {
                   if (mode==1 && f==0)
                    nofirst=1;
                   break;
                  }
		} 
    if (mode==1 && nofirst==1) break;
   } 
return -1;
}



/*** Finds number or users in given area ***/
int find_num_in_area(int area)
{
int u,num=0;
for (u=0;u<MAX_USERS;++u) 
	if (ustr[u].area==area) ++num;
return num;
}


/*** COMMAND FUNCTIONS ***/

/*** Call command function or execute command directly ***/
void exec_com(int com_num, int user, char *inpstr)
{
int z=0,gravoked=0;
char temp_mess[ARR_SIZE+65];
char filename[FILE_NAME_LEN];
FILE *fp;

/* See if user is suspended from using all commands */
if (ustr[user].suspended)
  {
    return;
  }

/* Check if command was granted to user - UNDER CONSTRUCTION */
for (z=0;z<MAX_GRAVOKES;++z) {
	if (!isgrant(ustr[user].revokes[z])) continue;
	if (strip_com(ustr[user].revokes[z])==sys[com_num].jump_vector) {
		gravoked=1; break;
	  }
  }
if (gravoked==1) {
gravoked=0;
ustr[user].tempsuper=strip_level(ustr[user].revokes[z]);
} /* end of granted if */
else {
 /* See if user has the required rank for this command */
 if (ustr[user].super < sys[com_num].su_com) 
   {
    write_str(user,NOT_WORTHY);
    return;
   }

 /* Check if command was revoked from user - UNDER CONSTRUCTION */
 for (z=0;z<MAX_GRAVOKES;++z) {
	if (!isrevoke(ustr[user].revokes[z])) continue;
	if (strip_com(ustr[user].revokes[z])==sys[com_num].jump_vector) { gravoked=1; break; }
   }
 if (gravoked==1) {
    write_str(user,NOT_WORTHY);
    gravoked=0; z=0;
    return;
    }

ustr[user].tempsuper=ustr[user].super;
} /* end of not granted else */

sprintf(filename,"%s",LASTFILE);
if (!(fp=fopen(filename,"w"))) {
   sprintf(temp_mess," %s couldn't write lastcommand to file",syserror);
   btell(user,temp_mess); 
   sprintf(temp_mess,"Couldn't write last command from %s, to file",ustr[user].say_name);
   logerror(temp_mess);
   FCLOSE(fp);
   }

sprintf(temp_mess,"%s: Command < %s > from %s\n",get_time(0,0),inpstr,ustr[user].say_name);
fputs(temp_mess,fp);
FCLOSE(fp);
temp_mess[0]=0;

if (ustr[user].numcoms==10000000) ustr[user].numcoms=1;
ustr[user].numcoms++;
commands++;
  
remove_first(inpstr);  /* get rid of commmand word */

switch(sys[com_num].jump_vector) {
	case 0 : user_quit(user); break;
	case 1 : t_who(user,inpstr,0); break;
	case 2 : shout(user,inpstr); break;
	case 3 : tell_usr(user,inpstr,0); break;
	case 4 : user_listen(user,inpstr); break;
	case 5 : user_ignore(user,inpstr); break;
	case 6 : look(user,inpstr);  break;
	case 7 : go(user,inpstr,0);  break;
	case 8 : room_access(user,1);  break; /* private */
	case 9 : room_access(user,0);  break; /* public */
	case 10: invite_user(user,inpstr);  break;
	case 11: emote(user,inpstr);  break;
	case 12: rooms(user,inpstr);  break;
	case 13: go(user,inpstr,1);  break;  /* knock */
	case 14: write_board(user,inpstr,0);  break; /* normal write */
	case 15: read_board(user,0,inpstr);  break;  /* normal read  */
	case 16: wipe_board(user,inpstr,0);  break;
	case 18: set_topic(user,inpstr);  break;
	case 21: kill_user(user,inpstr);  break;
	case 22: shutdown_d(user,inpstr);  break;
	case 23: search_boards(user,inpstr);  break;
	case 24: review(user);  break;
	case 25: help(user,inpstr);  break;
	case 26: broadcast(user,inpstr);  break;
	case 27: if (!cat(NEWSFILE,user,0)) 
			write_str(user,NO_NEWS);
                        write_str(user,"Ok");
		 break;

	case 28: system_status(user); break;
	case 29: move(user,inpstr);  break;
	case 30: system_access(user,inpstr,0);
	         break;  /* close */

	case 31: system_access(user,inpstr,1);
	         break;  /* open */

	case 35: toggle_atmos(user,inpstr); break;
	case 36: echo(user,inpstr);  break;
	case 37: set_desc(user,inpstr);  break;
	case 38: toggle_allow(user,inpstr); break;
        case 40: greet(user,inpstr); break;
	case 41: arrest(user,inpstr,0); break;
	case 42: cbuff2(user,inpstr); break;
        case 43: socials(user,inpstr,32); break; /* wedgie */
	case 44: macros(user,inpstr); break;
        case 45: read_mail(user,inpstr); break;
        case 46: send_mail(user,inpstr,0); break;
        case 47: ustr[user].clrmail= -1;
                 clear_mail(user, inpstr); 
                 break;
                 
	case 49: promote(user,inpstr); break;
	case 50: demote(user,inpstr); break;
	case 51: muzzle(user,inpstr,0); break;
	case 52: unmuzzle(user,inpstr); break;
	case 54: bring(user,inpstr); break;
	case 56: hide(user,inpstr); break;
	case 58: display_ranks(user); break;
	case 59: restrict(user,inpstr,ANY); break;
	case 60: unrestrict(user,inpstr,ANY); break;
	case 61: igtells(user); break;
	case 62: heartells(user); break;
	case 65: picture(user,inpstr); break;                 
	case 66: preview(user,inpstr); break;                
	case 67: password(user,inpstr); break;              
	case 68: permission_u(user,inpstr); break;             
	case 69: semote(user,inpstr); break;               
	case 70: tog_monitor(user); break;                     
	case 71: ptell(user,inpstr); break;                
	case 72: follow(user,inpstr); break;                
	case 73: read_init_data(); 
	         messcount();
	         write_str(user,"<ok>"); 
	         break;                  
	case 74: beep_u(user,inpstr); break;              
	case 75: set_afk(user,inpstr);  break;
	case 76: systime(user,inpstr);  break;
	case 77: print_users(user,inpstr);  break;
	case 78: usr_stat(user,inpstr,0);  break;
	case 79: set(user,inpstr);  break;
	case 80: swho(user,inpstr); break;
	case 82: btell(user,inpstr); break;
	case 83: nuke(user,inpstr,0); break;
	case 84: cls(user); break;
	case 85: fight_another(user,inpstr); break;
	case 86: resolve_names_set(user); break;
	case 90: say(user,inpstr,1); break;
	case 91: meter(user, inpstr); break;
	case 95: set_quota(user, inpstr); break;
	case 96: command_disabled(user); break;
	case 97: write_board(user,inpstr,1);  break; /* wiz_note */
	case 98: wipe_board(user,inpstr,1);  break;  /* wiz wipe */
	case 99: set_bafk(user,inpstr);  break;  
        case 100: xcomm(user,inpstr);  break;  
	case 101: t_who(user,inpstr,1); break;
	case 102: t_who(user,inpstr,2); break;
        case 103: think(user,inpstr); break;
        case 104: sos(user,inpstr); break;
        case 105: enter_pro(user,inpstr); break;
        case 106: if (!cat(FAQFILE,user,0))
                        write_str(user,NO_FAQ);
                        write_str(user,"<ok>");
                        break;
	case 107: usr_stat(user,inpstr,1); break;
        case 108: show(user,inpstr); break;
        case 109: cline(user,inpstr); break;
        case 110: bbcast(user,inpstr); break;
        case 111: version(user); break;
        case 112: shemote(user,inpstr); break; 
        case 113: femote(user,inpstr); break;
        case 114: suname(user,inpstr);  break;
        case 115: supass(user,inpstr);  break;
        case 116: enterm(user,inpstr);  break;
        case 117: abbrev(user,inpstr);  break;
        case 118: newwho(user);  break;
        case 119: last_u(user,inpstr);  break;
        case 120: talker(user,inpstr);  break;
        case 121: bubble(user);  break;
        case 122: sthink(user,inpstr);  break;
        case 123: where(user,inpstr);  break;
        case 124: call(user,inpstr);  break;
        case 125: creply(user,inpstr);  break;
        case 126: if (!cat(MAPFILE,user,0))
                        write_str(user,NO_MAP);
                        write_str(user,"Ok");
                  break;
        case 127: failm(user,inpstr);  break;
        case 128: succm(user,inpstr);  break;
        case 129: mutter(user,inpstr);  break;
        case 130: write_board(user,inpstr,3); break; /* suggestions */
        case 131: break;
        case 132: break;
        case 133: fmail(user,inpstr);  break;
        case 134: swipe(user,inpstr);  break;
        case 135: anchor_user(user,inpstr); break;
        case 136: quote_op(user,inpstr);  break;
        case 137: list_last(user,inpstr);  break;
        case 138: real_user(user,inpstr);  break;
        case 139: pukoolsn(user,inpstr);  break;
        case 140: regnif(user,inpstr);  break;
        case 141: siohw(user,inpstr);  break;        
        case 142: ustr[user].clrmail= -1;
                  clear_sent(user,inpstr);  break;
        case 143: read_sent(user,inpstr);  break;
        case 144: same_site(user,inpstr);  break;
        case 145: gag(user,inpstr);  break;
        case 146: alert(user,inpstr);  break;
        case 147: if (!cat(WIZFILE,user,0))   
                        write_str(user,NO_WIZLIST);
                        write_str(user,"Ok");
                  break;
        case 148: schedule(user);  break;
        case 149: restrict(user,inpstr,NEW); break;
        case 150: unrestrict(user,inpstr,NEW); break;
        case 151: sing(user,inpstr); break;
        case 152: show_expire(user,inpstr); break;
        case 153: force_user(user,inpstr); break;
        case 154: readlog(user,inpstr); break;
        case 155: descroom(user,inpstr); break;
        case 156: write_board(user,inpstr,2);  break; /* gripe_note */
        case 157: wipe_board(user,inpstr,2);  break;  /* gripe wipe */
        case 158: exitm(user,inpstr);  break;
        case 159: home_user(user);  break;
        case 160: nerf(user,inpstr);  break;
        case 161: frtell(user,inpstr); break;
        case 162: reload(user);  break;
        case 163: list_socs(user);  break;
        case 164: socials(user,inpstr,1);  break;  /* hug */
        case 165: socials(user,inpstr,2);  break;  /* laugh */
        case 166: socials(user,inpstr,3);  break;  /* poke */
        case 167: clist(user,inpstr);  break;
        case 168: vote(user,inpstr);  break;
        case 169: socials(user,inpstr,4);  break;  /* tickle */
        case 170: socials(user,inpstr,5);  break;  /* kiss */
        case 171: socials(user,inpstr,6);  break;  /* thwap */
        case 172: socials(user,inpstr,7);  break;  /* bop */
        case 173: socials(user,inpstr,8);  break;  /* tackle */
        case 174: socials(user,inpstr,9);  break;  /* smirk */
        case 175: socials(user,inpstr,10);  break;  /* lick */
        case 176: socials(user,inpstr,11);  break;  /* smile */
        case 177: memcheck(user);  break;
        case 178: add_atmos(user,inpstr);  break;
        case 179: del_atmos(user,inpstr);  break;
        case 180: list_atmos(user);  break;
        case 181: shout_think(user,inpstr);  break;
        case 182: suicide_user(user,inpstr);  break;
        case 183: say_to_user(user,inpstr);  break;
        case 184: gag_comm(user,inpstr,0);  break;
        case 185: frog_user(user,inpstr);  break;
        case 186: auto_com(user,inpstr);  break;
        case 187: break;
        case 188: eight_ball(user,inpstr);  break;
        case 189: warning(user,inpstr);  break;
        case 190: arrest(user,inpstr,1);  break;
        case 191: t_who(user,inpstr,3);  break;
        case 192: socials(user,inpstr,12); break; /* chuckle */
        case 193: socials(user,inpstr,13); break; /* cough */
        case 194: socials(user,inpstr,14); break; /* dance */
        case 195: socials(user,inpstr,15); break; /* doh */
        case 196: socials(user,inpstr,16); break; /* flirt */
        case 197: socials(user,inpstr,17); break; /* goose */
        case 198: socials(user,inpstr,18); break; /* grab */
        case 199: socials(user,inpstr,19); break; /* growl */
        case 200: socials(user,inpstr,20); break; /* hiss */
        case 201: socials(user,inpstr,21); break; /* insult */
        case 202: socials(user,inpstr,22); break; /* kick */
        case 203: socials(user,inpstr,23); break; /* lol */
        case 204: socials(user,inpstr,24); break; /* shake */
        case 205: socials(user,inpstr,25); break; /* shove */
        case 206: socials(user,inpstr,26); break; /* slap */
        case 207: socials(user,inpstr,27); break; /* whine */
        case 208: socials(user,inpstr,28); break; /* wink */
        case 209: socials(user,inpstr,29); break; /* woohoo */
        case 210: banname(user,inpstr); break;
        case 211: player_create(user,inpstr); break;
        case 212: if (!cat(RULESFILE,user,0))   
                        write_str(user,NO_RULESFILE);
                        write_str(user,"Ok");
                  break;
        case 213: socials(user,inpstr,30); break; /* chicken */
        case 214: socials(user,inpstr,31); break; /* noogie */
	case 215: revoke_com(user,inpstr); break;
	case 216: grant_com(user,inpstr); break;
	case 217: ttt_cmd(user,inpstr); break;
	case 218: guess_hangman(user,inpstr);  break;
	case 219: play_hangman(user,inpstr);  break;
        default: break;
	}
}

/*** Call command function or execute command directly ***/
void bot_com(int com_num, int user, char *inpstr)
{
char temp_mess[ARR_SIZE+41];
char filename[FILE_NAME_LEN];
FILE *fp;

/* see if su command */
if (ustr[user].suspended)
  {
    return;
  }
  
sprintf(filename,"%s",LASTFILE);
if (!(fp=fopen(filename,"w"))) {
   sprintf(temp_mess," %s couldn't write bot lastcommand to file",syserror);
   btell(user,temp_mess); 
   sprintf(temp_mess,"Couldn't write last command from bot %s, to file",ustr[user].say_name);
   logerror(temp_mess);
   FCLOSE(fp);
   }

sprintf(temp_mess,"Command < %s > from %s\n",inpstr,ustr[user].say_name);
fputs(temp_mess,fp);
FCLOSE(fp);
temp_mess[0]=0;

if (ustr[user].numcoms==10000000) ustr[user].numcoms=1;
ustr[user].numcoms++;
  
remove_first(inpstr);  /* get rid of commmand word */

switch(botsys[com_num].jump_vector) {
	case 0 : get_whoinfo(user,inpstr); break;
        default: break;
     }
}


/*** closes socket & does relevant output to other users & files ***/
void user_quit(int user)
{
int u,area=ustr[user].area;
int min;
int v=0;
char nuke_name[NAME_LEN+1];
time_t tm;

ustr[user].char_buffer[0]   = 0;
ustr[user].char_buffer_size = 0;

/* see is user has quit before he logged in */
if (ustr[user].logging_in) {
       min=(int)((time(0) - ustr[user].last_input)/60);
       if (min >= LOGIN_TIMEOUT)
        sprintf(mess," [LOGIN TIMED OUT from %s (%s)]",ustr[user].net_name,ustr[user].site);
       else
        sprintf(mess," [LOGIN ABORTED from %s (%s)]",ustr[user].net_name,ustr[user].site);
        writeall_str(mess, -2, user, 0, user, BOLD, NONE, 0);
        mess[0]=0;
        min=0;
   if (!strcmp(ustr[user].desc,DEF_DESC) && (ustr[user].super==0)) {
        write_str(user,"");
        write_str(user,"Description not set. GONNA HAFTA NUKE YOU. Sorry.");
        write_str(user,"");
        strtolower(ustr[user].login_name);
        remove_exem_data(ustr[user].login_name);
        remove_user(ustr[user].login_name);
        }
	ustr[user].logging_in=0;

	/* Close socket and clear from readmask */
        while (CLOSE(ustr[user].sock) == -1 && errno == EINTR)
		; /* empty while */
        FD_CLR(ustr[user].sock,&readmask);
        ustr[user].sock= -1;

        ustr[user].name[0]=0;
        ustr[user].say_name[0]=0;
        ustr[user].mutter[0]=0;
        ustr[user].phone_user[0]=0;
        ustr[user].creation[0]=0;
        ustr[user].homepage[0]=0;
        ustr[user].area= -1;
        ustr[user].conv_count = 0;
        ustr[user].numcoms = 0;
        ustr[user].mail_num = 0;
        ustr[user].numbering = 0;
        ustr[user].rows = 24;
        ustr[user].cols = 256;
        ustr[user].abbrs        = 1;
        ustr[user].times_on     = 0;
        ustr[user].aver         = 0;
        ustr[user].white_space  = 1;
        ustr[user].totl         = 0;
        ustr[user].autor        = 0;
        ustr[user].autof        = 0;
        ustr[user].automsgs     = 0;
        ustr[user].gagcomm      = 0;
        ustr[user].semail       = 0;
        ustr[user].quote        = 1;
        ustr[user].hilite       = 0;
        ustr[user].new_mail     = 0;
        ustr[user].color        = COLOR_DEFAULT;
        ustr[user].friend_num   = 0;
        ustr[user].revokes_num  = 0;
        ustr[user].gag_num      = 0;
        ustr[user].home_room[0] = 0;
        ustr[user].nerf_shots   = 5;
        ustr[user].nerf_energy  = 10;
        ustr[user].nerf_kills   = 0;
        ustr[user].nerf_killed  = 0;
        ustr[user].passhid      = 0;
        ustr[user].pbreak       = 0;
        ustr[user].beeps        = 0;
        ustr[user].mail_warn    = 0;
        ustr[user].muz_time     = 0;
        ustr[user].xco_time     = 0;
        ustr[user].gag_time     = 0;
        ustr[user].frog         = 0;
        ustr[user].frog_time    = 0;
        ustr[user].anchor       = 0;
        ustr[user].anchor_time  = 0;
        ustr[user].promote      = 0;
        ustr[user].afkmsg[0]    = 0;
        ustr[user].pro_enter    = 0;
        ustr[user].roomd_enter  = 0;
        ustr[user].vote_enter   = 0;
        ustr[user].t_ent        = 0;
        ustr[user].t_num        = 0;
        ustr[user].t_name[0]    = 0;
        ustr[user].t_host[0]    = 0;
        ustr[user].t_ip[0]      = 0;
        ustr[user].t_port[0]    = 0;
        ustr[user].help         = 0;
        ustr[user].who          = 0;
        ustr[user].webpic[0]    = 0;
        ustr[user].rwho         = 1;
        ustr[user].tempsuper    = 0;
	ustr[user].promptseq	= 0;
        ustr[user].needs_hostname = 0;
        ustr[user].ttt_kills    = 0;
        ustr[user].ttt_killed   = 0;
	ustr[user].ttt_board    = 0;
	ustr[user].ttt_opponent = -3;
	ustr[user].ttt_playing  = 0;
        ustr[user].hang_wins    = 0;
        ustr[user].hang_losses  = 0;
	ustr[user].hang_stage=-1;
	ustr[user].hang_word[0]='\0';
	ustr[user].hang_word_show[0]='\0';
	ustr[user].hang_guess[0]='\0';
	ustr[user].icq[0]        = 0; 
	ustr[user].miscstr1[0]   = 0;
	ustr[user].miscstr2[0]   = 0;
	ustr[user].miscstr3[0]   = 0;
	ustr[user].miscstr4[0]   = 0;
	ustr[user].pause_login   = 0;
	ustr[user].miscnum2      = 0;
	ustr[user].miscnum3      = 0;
	ustr[user].miscnum4      = 0;
	ustr[user].miscnum5      = 0;
        listen_all(user);

   for (v=0; v<MAX_ALERT; v++)
     {
      ustr[user].friends[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_GAG; v++)
     {
      ustr[user].gagged[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_GRAVOKES; v++)
     {
      ustr[user].revokes[v][0]=0;
     }
   v=0;
   for (v=0; v<NUM_LINES; v++)
     {
      ustr[user].conv[v][0]=0;
     }

	return;
	}

write_str(user,BYE_MESS); 

time(&tm);
strcpy(ustr[user].last_date,  ctime(&tm));
  ustr[user].last_date[24]=0;
strcpy(ustr[user].last_site, ustr[user].site);
strcpy(ustr[user].last_name, ustr[user].net_name);
ustr[user].rawtime = tm;

min=(tm-ustr[user].time)/60;

sprintf(mess,"%s %s@@",
             ustr[user].say_name,  ustr[user].desc);
write_str(user,mess);

sprintf(mess,"From site %s - %s",
             ustr[user].last_site, ustr[user].last_name);
write_str(user,mess);
                                      
sprintf(mess,USE_MESS,ustr[user].last_date,converttime((long)min));
write_str(user,mess);

/* kind of a fudge factor always have at least one */
if (ustr[user].aver==0) ustr[user].aver = 1;  

ustr[user].times_on++;

ustr[user].totl += (long)min;

ustr[user].aver = (int)(ustr[user].totl / (long)ustr[user].times_on);

/* Fix a user that has quit or has been killed while stuck in editing */
if (ustr[user].pro_enter) {
  free(ustr[user].pro_start);
  ustr[user].pro_end='\0';
  strcpy(ustr[user].flags,ustr[user].mutter);
  }
else if (ustr[user].vote_enter) {
  free(ustr[user].vote_start);
  ustr[user].vote_end='\0';
  strcpy(ustr[user].flags,ustr[user].mutter);
  }
else if (ustr[user].roomd_enter) {
  free(ustr[user].roomd_start);
  ustr[user].roomd_end='\0';
  strcpy(ustr[user].flags,ustr[user].mutter);
  }

copy_from_user(user);
write_user(ustr[user].name);

/* send message to other users & conv file */
if (ustr[user].super >= WIZ_LEVEL) 
  {
   sprintf(mess,SYS_DEPART_HI,ustr[user].say_name,ustr[user].desc);
   writeall_str(mess, 0, user, 0, user, NORM, LOGIO, 0);
  }
else 
  {
   sprintf(mess,SYS_DEPART_LO,ustr[user].say_name,ustr[user].desc);
   writeall_str(mess, 0, user, 0, user, NORM, LOGIO, 0);
  }

if (astr[area].private && (find_num_in_area(area) <= PRINUM))
   {
    strcpy(mess,NOW_PUBLIC);
    writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
    astr[area].private=0;
    cbuff(user);
   }


/* store signoff in log file & set some vars. */
num_of_users--;
syssign(user,0);

sprintf(mess,"+++++ logoff:%s",ustr[user].say_name);
write_bot(mess);

ustr[user].area       = -1;
ustr[user].logging_in = 0;
if (autonuke) strcpy(nuke_name,ustr[user].name);
ustr[user].name[0]       = 0;
ustr[user].say_name[0]   = 0;
ustr[user].mutter[0]     = 0;
ustr[user].phone_user[0] = 0;
ustr[user].creation[0]= 0;
ustr[user].homepage[0]= 0;
ustr[user].conv_count = 0;
ustr[user].numcoms    = 0;
ustr[user].mail_num   = 0;
ustr[user].numbering  = 0;
ustr[user].rows       = 24;
ustr[user].cols       = 256;
ustr[user].abbrs      = 1;
ustr[user].white_space= 1;
ustr[user].times_on   = 0;
ustr[user].aver       = 0;
ustr[user].totl       = 0;
ustr[user].autor      = 0;
ustr[user].autof      = 0;
ustr[user].automsgs   = 0;
ustr[user].gagcomm    = 0;
ustr[user].semail     = 0;
ustr[user].quote      = 1;
for (u=0; u<NUM_LINES; u++)
   {
    ustr[user].conv[u][0]=0;
   }
u=0;
for (u=0; u<NUM_MACROS; u++)
   {
    ustr[user].Macros[u].name[0]=0;
    ustr[user].Macros[u].body[0]=0;
   }

ustr[user].hilite       = 0;
ustr[user].new_mail     = 0;
ustr[user].color        = COLOR_DEFAULT;
ustr[user].friend_num   = 0;
ustr[user].revokes_num  = 0;
ustr[user].gag_num      = 0;
ustr[user].home_room[0] = 0;
ustr[user].nerf_shots   = 5;
ustr[user].nerf_energy  = 10;
ustr[user].nerf_kills   = 0;
ustr[user].nerf_killed  = 0;
ustr[user].passhid      = 0;
ustr[user].pbreak       = 0;
ustr[user].beeps        = 0;
ustr[user].mail_warn    = 0;
ustr[user].muz_time     = 0;
ustr[user].xco_time     = 0;
ustr[user].gag_time     = 0;
ustr[user].frog         = 0;
ustr[user].frog_time    = 0;
ustr[user].anchor       = 0;
ustr[user].anchor_time  = 0;
ustr[user].promote      = 0;
ustr[user].afkmsg[0]    = 0;
ustr[user].pro_enter    = 0;
ustr[user].roomd_enter  = 0;
ustr[user].vote_enter   = 0;
ustr[user].t_ent        = 0;
ustr[user].t_num        = 0;
ustr[user].t_name[0]    = 0;
ustr[user].t_host[0]    = 0;
ustr[user].t_ip[0]      = 0;
ustr[user].t_port[0]    = 0;
ustr[user].help         = 0;
ustr[user].who          = 0;
ustr[user].webpic[0]    = 0;
ustr[user].tempsuper    = 0;
ustr[user].promptseq    = 0;
ustr[user].needs_hostname = 0;
ustr[user].ttt_kills    = 0;
ustr[user].ttt_killed   = 0;
ustr[user].ttt_board    = 0;
if (ustr[user].ttt_opponent != -3)
        ttt_abort(user); 
ustr[user].ttt_opponent = -3;
ustr[user].ttt_playing  = 0;
ustr[user].hang_wins    = 0;
ustr[user].hang_losses  = 0;
ustr[user].hang_stage=-1;
ustr[user].hang_word[0]='\0';
ustr[user].hang_word_show[0]='\0';
ustr[user].hang_guess[0]='\0';
ustr[user].icq[0]        = 0; 
ustr[user].miscstr1[0]   = 0;
ustr[user].miscstr2[0]   = 0;
ustr[user].miscstr3[0]   = 0;
ustr[user].miscstr4[0]   = 0;
ustr[user].pause_login   = 0;
ustr[user].miscnum2      = 0;
ustr[user].miscnum3      = 0;
ustr[user].miscnum4      = 0;
ustr[user].miscnum5      = 0;
listen_all(user);

   for (v=0; v<MAX_ALERT; v++)
     {
      ustr[user].friends[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_GAG; v++)
     {
      ustr[user].gagged[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_GRAVOKES; v++)
     {
      ustr[user].revokes[v][0]=0;
     }
   v=0;
   for (v=0; v<NUM_LINES; v++)
     {
      ustr[user].conv[v][0]=0;
     }

if (user == fight.first_user || user == fight.second_user) reset_chal(0,"");

 if (autonuke) {
   if (!strcmp(ustr[user].desc,DEF_DESC) && (ustr[user].super==0)) {
        write_str(user,"");
        write_str(user,"Description not set. GONNA HAFTA NUKE YOU. Sorry.");
        write_str(user,"");
        strtolower(nuke_name);
        remove_exem_data(nuke_name);
        remove_user(nuke_name);

        sprintf(mess,"%s %s has been AUTO-NUKED",STAFF_PREFIX,nuke_name);
        mess[6]=toupper((int)mess[6]);
        writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);
      }
   else ustr[user].super=0;
   }

/* If bot logging off, clear bot id */
if (user==bot) {
  write_bot("+++++ QUIT");
  bot=-5;
  }

if (ustr[user].rwho > 1) {
        write_str(user,"");
        write_str(user,"You're in the middle of a remote who. Your connection will not close until the task is done. Please wait.");
   }

 /* Close user's connection socket */
  while (CLOSE(ustr[user].sock) == -1 && errno == EINTR)
	; /* empty while */
  FD_CLR(ustr[user].sock,&readmask);

  ustr[user].sock= -1;
  ustr[user].rwho         = 1;

}


/*** prints who is on the system to requesting user ***/
void t_who(int user, char *inpstr, int mode)
{
FILE   *fp=NULL;
int    s,u,v,min,idl,invis=0,count=0;
int    z=0;
int    vi=' ';
int    with,num=0;
char   *super=RANKS;
char   ud[100],un[100],an[NAME_LEN],und[200],rank[50];
char   temp[256];
char   check[NAME_LEN+1];
char   i_buff[5];
char   filename[FILE_NAME_LEN];
time_t tm;

/*------------------------------------------------------------*/
/* Check if inpstr length is greater than the max name length */
/*------------------------------------------------------------*/
if (strlen(inpstr) > NAME_LEN) {
   write_str(user,"Name too long.");
   return;
   }

if (check_rwho(user,inpstr) == 1) return;

if (mode==3) {
  if (ustr[user].friend_num==0) {
     write_str(user,"You have no friends!");
     return;
     }
  }  

/*-------------------------------------------------------*/
/* process the with command                              */
/*-------------------------------------------------------*/
with = user;
if (mode == 2)
  {
    if(strlen(inpstr) == 0) 
      {
       with = ustr[user].area;
      }
     else
      {
       sscanf(inpstr,"%s ",temp);
       strtolower(temp);

       if ((u=get_user_num(temp,user))== -1) 
         {
           not_signed_on(user,temp);  
           return;
         }
       with = ustr[u].area;
      }
  }

time(&tm);

if (ustr[user].pbreak) {
   strcpy(filename,get_temp_file());
   if (!(fp=fopen(filename,"w"))) {
     write_str(user,"Can't create file for paged who listing!");
     return;
     }
   }

/* display current time */
if (mode==1)
 sprintf(mess,WWHO_COLOR,ctime(&tm));
else if (mode==3)
 sprintf(mess,FWHO_COLOR,ctime(&tm));
else
 sprintf(mess,WHO_COLOR,ctime(&tm));

if (ustr[user].pbreak) {
   strcat(mess,"\n");
   fputs(mess,fp);
   }
else write_str(user,mess);

/* Give Display format */
if (ustr[user].who==0 || ustr[user].who==3) { /* OURS */
sprintf(mess,"^LCRoom^             ");
strcat(mess,"^HGTime^ "); 
strcat(mess,"^HRStat^ "); 
strcat(mess,"^Idl^   ");
strcat(mess,"^HMName^/^HBDescription^");
if (ustr[user].pbreak) {
   strcat(mess,"\n");
   fputs(mess,fp);
   }
else write_str(user,mess);
} /* end of if */
else if (ustr[user].who==2) { /* IFORMS */
sprintf(mess,"^LCRoom^              ");
strcat(mess,"^HMName^/^HBDescription^                           ");
strcat(mess,"^HGTime^-"); 
strcat(mess,"^HRStat^-"); 
strcat(mess,"^Idle^");
if (ustr[user].pbreak) {
   strcat(mess,"\n");
   fputs(mess,fp);
   }
else write_str(user,mess);
} /* end of if */


if (mode!=3) {
for (v=0;v<NUM_AREAS;++v) {
  for (u=0;u<MAX_USERS;++u) {
	if ((ustr[u].area!= -1) && (ustr[u].area == v) && (!ustr[u].logging_in))
	  {
		if (!ustr[u].vis && ustr[user].tempsuper < MIN_HIDE_LEVEL)
	          {
	            invis++;
	            continue; 
	          }
	   
       	        min=(tm-ustr[u].time)/60;
		idl=(tm-ustr[u].last_input)/60;

		if (ustr[user].who==1)
		sprintf(un," %s",ustr[u].say_name);
		else
		strcpy(un,ustr[u].say_name);

                if (ustr[u].afk==0)
		 strcpy(ud,ustr[u].desc);
		else if ((strlen(ustr[u].afkmsg) > 1) && (ustr[u].afk>=1))
		 strcpy(ud,ustr[u].afkmsg);
                else if (!strlen(ustr[u].afkmsg) && (ustr[u].afk>=1))
                 strcpy(ud,ustr[u].desc);

		strcpy(und,un);
		strcat(und," ");
		strcat(und,ud);
		
		if (!astr[ustr[u].area].hidden)
		  {
		    strcpy(an,astr[ustr[u].area].name);
		  }
		 else
		  { 
		    if ((ustr[user].tempsuper >= ROOMVIS_LEVEL) && SHOW_HIDDEN)
		      {
		        strcpy(an, "<");
		        strcat(an, astr[ustr[u].area].name);
		        strcat(an, ">");
		      }
		     else
		      {
		       strcpy(an, "        ");
		      }
		  }
		  
		
		if (ustr[user].tempsuper >= WHO_LEVEL) 
		   { 
		    s=super[ustr[u].super];
		   }
		  else
		   s=' ';
		   
		if (ustr[u].afk == 1) {
		   if (ustr[u].lockafk)
		    strcpy(i_buff,"^HYLAFK^");
		   else
		    strcpy(i_buff,"^HYAFK^ ");
                  }
		 else if (ustr[u].afk == 2) {
		      strcpy(i_buff,"^HRBAFK^");
                     }
                 else if (ustr[u].pro_enter) {
		      strcpy(i_buff,"^HGPROF^");
                     }
                 else if (ustr[u].vote_enter) {
		      strcpy(i_buff,"^HGVOTE^");
                     }
                 else if (ustr[u].roomd_enter) {
		      strcpy(i_buff,"^HGDESC^");
                     }
		 else {
		if ((ustr[user].who==1) && (ustr[user].tempsuper >= WHO_LEVEL))
		 strcpy(i_buff,"");
		else {
                  if (idl < 3)
                    strcpy(i_buff,"Actv");
                  else if (idl >= 3 && idl < 60)
                    strcpy(i_buff,"Awke");
                  else if (idl >= 60 && idl < 180)
		    strcpy(i_buff,"Idle"); 
                  else if (idl >= 180)
                    strcpy(i_buff,"Coma");
		  }
                    } 

	if (ustr[user].who==1) {
	if (ustr[user].tempsuper >= WHO_LEVEL)
	strcpy(rank,ranks[ustr[u].super]);
	else {
	strcpy(rank,"  ");
	strcat(rank,i_buff);
	strcat(rank,"  ");
	strcpy(i_buff,"");
	}
	}

	if (ustr[user].who==1) {
		if (!ustr[u].vis)
		 und[0]='*';
	}
	else {
                if (!ustr[u].vis) vi='_';
                else vi=' ';
	}

count=DESC_LEN+count_color(und,0);
/*
sprintf(mess,"len without mod        : %d\nlen of colors          : %d\nlen of replacing colors: %d\ncount                  : %d\n",strlen(und),count_color(und,0),count_color(und,1),count);
write_str(user,mess);
*/
if (ustr[user].who==1)
        sprintf(mess,"%-*s :%-*s:%-*s:%-3.3d m %s",count,und,RANK_LEN,rank,ROOM_LEN,an,min,i_buff);
else if (ustr[user].who==2)
        sprintf(mess,"%-*s %c%c%-*s %-5.5d %s %3.3d",ROOM_LEN,an,s,vi,count,und,min,i_buff,idl);
else
        sprintf(mess,"%-*s %-5.5d %s %3.3d %c%c%s",ROOM_LEN,an,min,i_buff,idl,s,vi,und);

count=0;
		
		strncpy(temp,mess,256);
		strtolower(temp);
		
		if (!strlen(inpstr) ||
		     instr2(0,mess,inpstr,0)!= -1 ||
		     instr2(0,temp,inpstr,0)!= -1 ||
		     mode > 0) 
		 {
		   if ((mode == 1 && ustr[u].tempsuper >= WIZ_LEVEL) || 
		        mode == 0 ||
		       (mode == 2 && ustr[u].area == with))
		     {
		      mess[0]=toupper((int)mess[0]);
			if (ustr[user].pbreak) {
			   strcat(mess,"\n");
  			   fputs(mess,fp);
  			 }
		      else write_str(user,mess);
                      num++;
		     }  /* inpstr check */
                 }  /* mode check */
	   }  /* if user in area check */
	}  /* user for */
 }  /* area for */
}  /* end of mode if */

/* Friends check */
if (mode==3) {
for (z=0; z<MAX_ALERT; ++z) {
  strcpy(check,ustr[user].friends[z]);
  strtolower(check);
for (v=0;v<NUM_AREAS;++v) {
  for (u=0;u<MAX_USERS;++u) {
	if ((ustr[u].area!= -1) && (ustr[u].area == v) && (!ustr[u].logging_in))
	  {
            if (!strcmp(ustr[u].name,check)) {
		if (!ustr[u].vis && ustr[user].tempsuper < MIN_HIDE_LEVEL)
	          {
	            invis++;
	            continue; 
	          }
	   
       	        min=(tm-ustr[u].time)/60;
		idl=(tm-ustr[u].last_input)/60;

		if (ustr[user].who==1)
		sprintf(un," %s",ustr[u].say_name);
		else
		strcpy(un,ustr[u].say_name);

                if (ustr[u].afk==0)
		 strcpy(ud,ustr[u].desc);
		else if ((strlen(ustr[u].afkmsg) > 1) && (ustr[u].afk>=1))
		 strcpy(ud,ustr[u].afkmsg);
                else if (!strlen(ustr[u].afkmsg) && (ustr[u].afk>=1))
                 strcpy(ud,ustr[u].desc);

		strcpy(und,un);
		strcat(und," ");
		strcat(und,ud);
		
		if (!astr[ustr[u].area].hidden)
		  {
		    strcpy(an,astr[ustr[u].area].name);
		  }
		 else
		  { 
		    if ((ustr[user].tempsuper >= ROOMVIS_LEVEL) && SHOW_HIDDEN)
		      {
		        strcpy(an, "<");
		        strcat(an, astr[ustr[u].area].name);
		        strcat(an, ">");
		      }
		     else
		      {
		       strcpy(an, "        ");
		      }
		  }
		  
		
		if (ustr[user].tempsuper >= WHO_LEVEL) 
		   { 
		    s=super[ustr[u].super];
		   }
		  else
		   s=' ';
		   
		if (ustr[u].afk == 1) {
		   if (ustr[u].lockafk)
		    strcpy(i_buff,"^HYLAFK^");
		   else
		    strcpy(i_buff,"^HYAFK^ ");
                  }
		 else if (ustr[u].afk == 2) {
		      strcpy(i_buff,"^HRBAFK^");
                     }
                 else if (ustr[u].pro_enter) {
		      strcpy(i_buff,"^HGPROF^");
                     }
                 else if (ustr[u].vote_enter) {
		      strcpy(i_buff,"^HGVOTE^");
                     }
                 else if (ustr[u].roomd_enter) {
		      strcpy(i_buff,"^HGDESC^");
                     }
		 else {
		if ((ustr[user].who==1) && (ustr[user].tempsuper >= WHO_LEVEL))
		  strcpy(i_buff,"");
		else {
                  if (idl < 3)
                    strcpy(i_buff,"Actv");
                  else if (idl >= 3 && idl < 60)
                    strcpy(i_buff,"Awke");
                  else if (idl >= 60 && idl < 180)
		    strcpy(i_buff,"Idle"); 
                  else if (idl >= 180)
                    strcpy(i_buff,"Coma");
		  }
                    } 

	if (ustr[user].who==1) {
	if (ustr[user].tempsuper >= WHO_LEVEL)
	strcpy(rank,ranks[ustr[u].super]);
	else {
	strcpy(rank,"  ");
	strcat(rank,i_buff);
	strcat(rank,"  ");
	strcpy(i_buff,"");
	}
	}

	if (ustr[user].who==1) {
		if (!ustr[u].vis)
		 und[0]='*';
	}
	else {
                if (!ustr[u].vis) vi='_';
                else vi=' ';
	}

count=DESC_LEN+count_color(und,0);
if (ustr[user].who==1)
        sprintf(mess,"%-*s :%-*s:%-*s:%-3.3d min %s",count,und,RANK_LEN,rank,ROOM_LEN,an,min,i_buff);
else if (ustr[user].who==2)
        sprintf(mess,"%-*s %c%c%-*s %-5.5d %s %3.3d",ROOM_LEN,an,s,vi,count,und,min,i_buff,idl);
else
        sprintf(mess,"%-*s %-5.5d %s %3.3d %c%c%s",ROOM_LEN,an,min,i_buff,idl,s,vi,und);

count=0;
                 
		      mess[0]=toupper((int)mess[0]);
			if (ustr[user].pbreak) {
			   strcat(mess,"\n");
  			   fputs(mess,fp);
  			 }
		      else write_str(user,mess);
                      num++;
              }  /* end of if found */
	   }  /* if user in area check */
	}  /* user for */
  }  /* area for */
 }  /* end of friends list for */
}  /* end of mode if */


if (invis) {
sprintf(mess,SHADOW_COLOR,invis == 1 ? "is" : "are",invis,invis == 1 ? " " : "s");

	if (ustr[user].pbreak) {
	   strcat(mess,"\n");
	   fputs(mess,fp);
  	 }
	else write_str(user,mess);

	}

if (ustr[user].pbreak) {
   fputs("\n",fp);
   }	
else write_str(user," ");

 if (mode==1)
  sprintf(mess,WUSERCNT_COLOR,num,num == 1 ? "" : "s");
 else if (mode==3)
  sprintf(mess,FUSERCNT_COLOR,num,num == 1 ? "" : "s");
 else
  sprintf(mess,USERCNT_COLOR,num_of_users,num_of_users == 1 ? "" : "s");

	if (ustr[user].pbreak) {
	   strcat(mess,"\n");
	   fputs(mess,fp);
           fputs("\n",fp);
  	 }
  else {
  write_str(user,mess);
  write_str(user," ");
  }

	if (ustr[user].pbreak) {
	   fclose(fp);
           cat(filename,user,0);
  	 }

}


/*** prints who is on the system to requesting user ***/
void external_who(int as)
{
int    s,u,vi,min,idl,invis=0;
char   ud[100],un[100],an[NAME_LEN],und[200];
char   temp[256];
char   i_buff[5];
time_t tm;

/*-------------------------------------------------------------------------*/
/* write out title block                                                   */
/*-------------------------------------------------------------------------*/
if (EXT_WHO1) S_WRITE(as,EXT_WHO1, strlen(EXT_WHO1) );
if (EXT_WHO2) S_WRITE(as,EXT_WHO2, strlen(EXT_WHO2) );
if (EXT_WHO3) {
  sprintf(mess,EXT_WHO3,PORT+WHO_OFFSET);
  S_WRITE(as,mess, strlen(mess) );        
  }
if (EXT_WHO4) S_WRITE(as,EXT_WHO4, strlen(EXT_WHO4) );

/* display current time */
time(&tm);
sprintf(mess,WHO_PLAIN,ctime(&tm));
strcat(mess,"\n\r");
S_WRITE(as, mess, strlen(mess));

/* Give Display format */
sprintf(mess,"Room             Time Stat Idl   Name/Description\n");
S_WRITE(as, mess, strlen(mess));

/* display user list */
for (u=0;u<MAX_USERS;++u) {
	if ((ustr[u].area!=-1) && (!ustr[u].logging_in))
	  {
		if (!ustr[u].vis)
	          { 
	            invis++;  
	            continue; 
	          }
			 
		min=(tm-ustr[u].time)/60;
		idl=(tm-ustr[u].last_input)/60;

		strcpy(un,ustr[u].say_name);
    
                if (ustr[u].afk==0)
                 strcpy(ud,ustr[u].desc);
                else if ((strlen(ustr[u].afkmsg) > 1) && (ustr[u].afk>=1))
                 strcpy(ud,ustr[u].afkmsg);
                else if (!strlen(ustr[u].afkmsg) && (ustr[u].afk>=1))
                 strcpy(ud,ustr[u].desc);
 		
		strcpy(und,un);
		strcat(und," ");		
		strcat(und,ud);
		
		if (!astr[ustr[u].area].hidden)
		  {
		    strcpy(an,astr[ustr[u].area].name);
		  }
		 else
		  { 
		    strcpy(an, "        ");
		  }
		  
		s=' ';
		   
		if (ustr[u].afk == 1) {
                   strcpy(i_buff,"AFK ");
                  }
		 else if (ustr[u].afk == 2) {
                      strcpy(i_buff,"BAFK");
                     }
                 else if (ustr[u].pro_enter) {
                      strcpy(i_buff,"PROF");
                     }
                 else if (ustr[u].vote_enter) {
                      strcpy(i_buff,"VOTE");
                     }
                 else if (ustr[u].roomd_enter) {
                      strcpy(i_buff,"DESC");
                     }
		 else {
                  if (idl < 3)
                    strcpy(i_buff,"Actv");
                  else if (idl >= 3 && idl < 60)
                    strcpy(i_buff,"Awke");
                  else if (idl >= 60 && idl < 180)
		    strcpy(i_buff,"Idle"); 
                  else if (idl >= 180)
                    strcpy(i_buff,"Coma");
                    } 

		if (!ustr[u].vis) vi='_';
                else vi=' ';

	sprintf(mess,"%-*s %-5.5d %s %3.3d %c%c%s\n",ROOM_LEN,an,min,i_buff,idl,s,vi,und);
	        
		strncpy(temp,mess,256);
		strtolower(temp);
		
		mess[0]=toupper((int)mess[0]);
                strcpy(mess, strip_color(mess));
		S_WRITE(as, mess, strlen(mess));
	       }
	}
	
if (invis) 
  {
   sprintf(mess,SHADOW_PLAIN,invis == 1 ? "is" : "are",invis,invis == 1 ? " " : "s");
   S_WRITE(as, mess, strlen(mess));
  }

S_WRITE(as, "\n\n", 1);
sprintf(mess,USERCNT_PLAIN,num_of_users,num_of_users == 1 ? "" : "s");
S_WRITE(as, mess, strlen(mess) );
S_WRITE(as, "\n\n\n", 2);
}

/*** prints who is on the system to requesting user ***/
void swho(int user, char *inpstr)
{
int u;
int found = 0;

/* Give Display format */
if ( !strlen(inpstr) )
  {
   write_str(user,"----------------------------------------------------------------");
   write_str(user,"User          IP address    User#:Sock  Hostname");
   write_str(user,"----------------------------------------------------------------");
  }
else strtolower(inpstr);

/* display who port connections */
for (u=0; u<MAX_WHO_CONNECTS; ++u) 
  {
   if (strlen(whoport[u].site) > 1) {
	   sprintf(mess, SYS_SITE_LINE, " [who port] ", 
	                                whoport[u].site, 
	                                u, whoport[u].sock,
		                        whoport[u].net_name );

	   if (!strlen(inpstr) || instr2(0,mess,inpstr,0) != -1) 
	     {
	       mess[0]=toupper((int)mess[0]);
	       write_str(user,mess);
               found=1;
             }
     } /* end of if */
  } /* end of for */
u=0;

/* display www port connections */
for (u=0; u<MAX_WWW_CONNECTS; ++u) 
  {
   if (strlen(wwwport[u].site) > 1) {
	   sprintf(mess, SYS_SITE_LINE, " [www port] ", 
	                                wwwport[u].site, 
	                                u, wwwport[u].sock,
		                        wwwport[u].net_name );

	   if (!strlen(inpstr) || instr2(0,mess,inpstr,0) != -1) 
	     {
	       mess[0]=toupper((int)mess[0]);
	       write_str(user,mess);
               found=1;
             }
     } /* end of if */
  } /* end of for */
u=0;
/* display user list */
for (u=0; u<MAX_USERS; ++u) 
  {
    if (!strcmp(ustr[u].name,inpstr)) found = 2;

    if (ustr[u].area != -1)  
      {
	sprintf(mess, SYS_SITE_LINE, ustr[u].name,
	                             ustr[u].site,
	                             u, ustr[u].sock,
		                     ustr[u].net_name );

        if (strlen(inpstr) && (found==2)) {
	   mess[0]=toupper((int)mess[0]);
	   write_str(user,mess);
           write_str(user," ");
           return;
           }
	else if (!strlen(inpstr) || instr2(0,mess,inpstr,0) != -1) 
	  {
	   mess[0]=toupper((int)mess[0]);
	   write_str(user,mess);
           found=1;
          }
       } /* end of area if */
      else if (ustr[u].logging_in) 
         {
	   sprintf(mess, SYS_SITE_LINE, " [login] ", 
	                                ustr[u].site, 
	                                u, ustr[u].sock,
		                        ustr[u].net_name );

	   if (!strlen(inpstr) || instr2(0,mess,inpstr,0) != -1) 
	     {
	       mess[0]=toupper((int)mess[0]);
	       write_str(user,mess);
               found=1;
             }
	  } /* end of else if */
  } /* end of for */

if (!found) {
/* plug security hole */
if (check_fname(inpstr,user))
  {                                     
   write_str(user,"Illegal name.");
   return;
  }

   if (!check_for_user(inpstr)) {
     write_str(user,NO_USER_STR);
     return;
     }
   read_user(inpstr);
   sprintf(mess,"%s, in from %s %s",t_ustr.say_name,t_ustr.last_site,t_ustr.last_name);
   write_str(user,mess);
   }

write_str(user," ");
}


/*** shout sends speech to all users regardless of area ***/
void shout(int user, char *inpstr)
{
int pos = sh_count%NUM_LINES;
int f; 

if (!ustr[user].shout) 
  {
   write_str(user,NO_SHOUT);
   return;
  }
  
if (!strlen(inpstr)) 
  {
   write_str(user,"Review shouts:"); 
    
    for (f=0;f<NUM_LINES;++f) 
      {
        if ( strlen( sh_conv[pos] ) )
         {
	  write_str(user,sh_conv[pos]);  
	 }
	pos = ++pos % NUM_LINES;
      }

    write_str(user,"<Done>");  
    return; 
  }
  
if (ustr[user].frog) {
   strcpy(inpstr,FROG_TALK);
   say(user,inpstr,0);
   return;
   }

sprintf(mess,USER_SHOUTS,ustr[user].say_name,inpstr);

if (!ustr[user].vis)
	sprintf(mess,INVIS_SHOUTS,INVIS_TALK_LABEL,inpstr);

/** Store the shout in the buffer **/
strncpy(sh_conv[sh_count],mess,MAX_LINE_LEN);
sh_count = ( ++sh_count ) % NUM_LINES;
	
writeall_str(mess, 0, user, 0, user, NORM, SHOUT, 0);
sprintf(mess,YOU_SHOUT,inpstr);
write_str(user,mess);

}


/*** tells another user something without anyone else hearing ***/
void tell_usr(int user, char *inpstr, int mode)
{
int point=0,count=0,i=0,lastspace=0,lastcomma=0,gotchar=0;
int point2=0,multi=0;
int multilistnums[MAX_MULTIS];
char multilist[MAX_MULTIS][ARR_SIZE];
char multiliststr[ARR_SIZE];
char other_user[ARR_SIZE];
int u=-1;
char prefix[25];
int pos = ustr[user].conv_count % NUM_LINES;
int f;

for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; multilistnums[i]=-1; }
multiliststr[0]=0;
i=0;

if (!strlen(inpstr)) 
  {
    write_str(user,"Review tells:"); 
    
    for (f=0;f<NUM_LINES;++f) 
      {
        if ( strlen( ustr[user].conv[pos] ) )
         {
	  write_str(user,ustr[user].conv[pos]);  
	 }
	pos = ++pos % NUM_LINES;
      }

    write_str(user,"<Done>");  
    return;
  }

if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

sscanf(inpstr,"%s ",other_user);
if (!strcmp(other_user,"-f")) {
	other_user[0]=0;
	for (i=0;i<MAX_ALERT;++i) {
	 if (strlen(ustr[user].friends[i])) {
	  strcpy(multilist[count],ustr[user].friends[i]);
	  count++;
	  if (count==MAX_MULTIS) break;
	  }
	}
	if (!count) {
		write_str(user,"You dont have any friends!");
		return;
	}
	i=0;
	remove_first(inpstr);
  }
else {
other_user[0]=0;

for (i=0;i<strlen(inpstr);++i) {
	if (inpstr[i]==' ') {
		if (lastspace && !gotchar) { point++; point2++; continue; }
		if (!gotchar) { point++; point2++; }
		lastspace=1;
		continue;
	  } /* end of if space */
	else if (inpstr[i]==',') {
		if (!gotchar) {
			lastcomma=1;
			point++;
			point2++;
			continue;
		}
		else { 
		if (count <= MAX_MULTIS-1) {
		midcpy(inpstr,multilist[count],point,point2-1);
		count++;
		}
		point=i+1;
		point2=point;
		gotchar=0;
		lastcomma=1;
		continue;
		}

	} /* end of if comma */
	if ((inpstr[i-1]==' ') && (gotchar)) {
		if (count <= MAX_MULTIS-1) {
		midcpy(inpstr,multilist[count],point,point2-1);
		count++;
		}
		break;
	}
	gotchar=1;
	lastcomma=0;
	lastspace=0;
	point2++;
} /* end of for */
midcpy(inpstr,multiliststr,i,ARR_SIZE);

if (!strlen(multiliststr)) {
	/* no message string, copy last user */
	midcpy(inpstr,multilist[count],point,point2);
	count++;
	strcpy(inpstr,"");
	}
else {
	strcpy(inpstr,multiliststr);
	multiliststr[0]=0;
     }
} /* end of friend else */

i=0;
point=0;
point2=0;
gotchar=0;

tells++;

if (count>1) multi=1;

/* go into loop and check users */
for (i=0;i<count;++i) {

strcpy(other_user,multilist[i]);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   if (!multi) {
   write_str(user,"Illegal name.");
   return;
   }
   else continue;
  }

strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   if (!read_user(other_user)) {
      write_str(user,NO_USER_STR);
      if (!multi) return;
      else continue;
      }
   not_signed_on(user,other_user);
if (user_wants_message(user,FAILS)) {
   sprintf(mess,"%s",t_ustr.fail);
   write_str(user,mess);
   }
      if (!multi) return;
      else continue;
  }

if (!gag_check(user,u,0)) {
      if (!multi) return;
      else continue;
  }

if (ustr[u].pro_enter || ustr[u].vote_enter || ustr[u].roomd_enter) {
    write_str(user,IS_ENTERING);
      if (!multi) return;
      else continue;
    }
  
if (ustr[u].afk)
  {
    if (ustr[u].afk == 1) {
      if (!strlen(ustr[u].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[u].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }
     else {
      if (!strlen(ustr[u].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[u].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }

    write_str(user,t_mess);
  }
  
if (ustr[u].igtell && ustr[user].tempsuper<WIZ_LEVEL) 
  {
   sprintf(mess,"%s is ignoring tells",ustr[u].say_name);
   write_str(user,mess);
   if (user_wants_message(user,FAILS)) write_str(user,ustr[u].fail);
      if (!multi) return;
      else continue;
  }

/* check if this user is already in the list */
/* we're gonna reuse some ints here          */
for (point2=0;point2<MAX_MULTIS;++point2) {
	if (multilistnums[point2]==u) { gotchar=1; break; }
   }
point2=0;
if (gotchar) {
  gotchar=0;
  continue;
  }

/* it's ok to send the tell to this user, add them to the multistr */
/* add this user to the list for our next loop */
multilistnums[point]=u;
point++;
} /* end of user for */  
i=0;

/* no multilistnums, must be all bad users */
if (!point) {
	return;
  }

/* loop to compose the messages and print to the users */
for (i=0;i<point;++i) {
u=multilistnums[i];

count=0;
point2=0;
multiliststr[0]=0;
/* make multi string to send to this user */
for (point2=0;point2<point;++point2) {
/* dont send recipients name to themselves */
if (u==multilistnums[point2]) continue;
else count++;
if (count>0)
 strcat(multiliststr,",");
/* add their name to the output string */
if (!ustr[multilistnums[point2]].vis)
 strcat(multiliststr,INVIS_ACTION_LABEL);
else
 strcat(multiliststr,ustr[multilistnums[point2]].say_name);
}

if ((ustr[u].monitor==1) || (ustr[u].monitor==3))
  {
    strcpy(prefix,"<");
    strcat(prefix,ustr[user].say_name);
    strcat(prefix,"> ");
  }
 else
  {
   prefix[0]=0;
  }

/* write to user being told */
if (!strlen(inpstr) && (mode==0)) {
if (ustr[user].vis) 
  {
    sprintf(mess,"You sense that %s is looking for you in the %s",
ustr[user].say_name,astr[ustr[user].area].name);
  }
 else
  {
  if ((ustr[u].monitor==1) || (ustr[u].monitor==3))
    {
      sprintf(prefix,"? %s",ustr[user].say_name);
    }
   else
    {
     strcpy(prefix,INVIS_TALK_LABEL);
    }
   sprintf(mess,"You sense that %s is looking for you.",prefix);
  }

}
else {
if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

if (ustr[user].vis) 
  {
   if (!multi)
    sprintf(mess,VIS_TELLS,ustr[user].say_name,inpstr);
   else
    sprintf(mess,VIS_TELLS_M,ustr[user].say_name,multiliststr,inpstr);
  }
 else
  {
  if ((ustr[u].monitor==1) || (ustr[u].monitor==3))
    {
      sprintf(prefix,"? %s",ustr[user].say_name);
      if (!multi)
       sprintf(mess,VIS_TELLS,prefix,inpstr);
      else
       sprintf(mess,VIS_TELLS_M,prefix,multiliststr,inpstr);
    }
   else
    {
     if (!multi)
      sprintf(mess,INVIS_TELLS,INVIS_TALK_LABEL,inpstr);
     else
      sprintf(mess,INVIS_TELLS_M,INVIS_TALK_LABEL,multiliststr,inpstr);
    }
  }
}

/* if a .beep (mode 1) and .set beeps are on add an extra beep */
/* if mode 0, a normal tell, if beep listening add beep        */
if (ustr[u].beeps) {
 if (mode==1) { }
 else {
 if (user_wants_message(u,BEEPS))
  strcat(mess,"\07");
 }
}

/* if a .beep (mode 1) add beep listening add the beep message */
/* else non beep message                                       */
if (mode==1) {
  if (user_wants_message(u,BEEPS)) {
   strcat(mess," \07 *Beep*\07");
   strcat(inpstr,"  *Beep*");
   }
  else {
   strcat(mess,"  *Beep*");
   strcat(inpstr,"  *Beep*");
   }
 }

/*-----------------------------------*/
/* store the tell in the rev buffer  */
/*-----------------------------------*/
/* moved because of multi tells */
strncpy(ustr[u].conv[ustr[u].conv_count],mess,MAX_LINE_LEN);
ustr[u].conv_count = ( ++ustr[u].conv_count ) % NUM_LINES;

if (ustr[u].hilite==2)
 write_str(u,mess);
else {
 strcpy(mess, strip_color(mess));
 write_hilite(u,mess);
 }

} /* end of message compisition for loop */

if (multi) {
point2=0;
multiliststr[0]=0;
/* make multi string to send to this user */
for (point2=0;point2<point;++point2) {
/* dont send recipients name to themselves */
if (point2>0)
 strcat(multiliststr,",");
/* add their name to the output string */
if (!ustr[multilistnums[point2]].vis)
 strcat(multiliststr,INVIS_ACTION_LABEL);
else
 strcat(multiliststr,ustr[multilistnums[point2]].say_name);
}
} /* end of if multi */

/* write to teller */
if (strlen(inpstr)) {
 if (!multi)
  sprintf(mess,VIS_FROMTELLS,ustr[u].say_name,inpstr);
 else
  sprintf(mess,VIS_FROMTELLS,multiliststr,inpstr);
}
else {
 if (!multi)
  sprintf(mess,"Telepathic message sent to %s.",ustr[u].say_name);
 else
  sprintf(mess,"Multi-Telepathic message sent to %s.",multiliststr);
}

write_str(user,mess);
if (!multi) {
if (user_wants_message(user,SUCCS) && strlen(inpstr) && strlen(ustr[u].succ))
write_str(user,ustr[u].succ);
}

strncpy(ustr[user].conv[ustr[user].conv_count],mess,MAX_LINE_LEN);
ustr[user].conv_count = ( ++ustr[user].conv_count ) % NUM_LINES;

i=0;
for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; }
multiliststr[0]=0;

return;
}

/* friend tell */
void frtell(int user, char *inpstr)
{
char str[ARR_SIZE+4];

strcpy(str,"-f ");
strcat(str,inpstr);
tell_usr(user,str,0);
}

/* friend emote */
void femote(int user, char *inpstr)
{
char str[ARR_SIZE+4];

strcpy(str,"-f ");
strcat(str,inpstr);
semote(user,str);
}

/*** beep a user ***/
void beep_u(int user, char *inpstr)
{
if (!strlen(inpstr)) 
  {
    write_str(user,"Beep who?");  
    return;
  }

tell_usr(user,inpstr,1);
}


/*** not signed on - subsid func ***/
void not_signed_on(int user, char *name)
{
sprintf(mess,"%s is not signed on",name);
mess[0]=toupper((int)mess[0]);
write_str(user,mess);
return;
}


/*** look decribes the surrounding scene **/
void look(int user, char *inpstr)
{
int f, i, u, area=0, new_area, found=0;
int occupied=0;
int spys=0;
char text[8];
char filename[FILE_NAME_LEN];

i = rand() % NUM_COLORS;

if (!strlen(inpstr))
 area=ustr[user].area;
else {
  if (strlen(inpstr) > 20) {
      write_str(user,"Room name is too long");  return;
      }
  for (new_area=0; new_area < NUM_AREAS; ++new_area)
       {
        if (! instr2(0, astr[new_area].name, inpstr, 0) )
          {
            found = TRUE;
            area = new_area;
           if (astr[new_area].hidden && ustr[user].tempsuper < GRIPE_LEVEL)
             {
              write_str(user,"^Secured room, peeking not allowed.^");
              return;
             }
            break;
          }
       } /* end of for */
 if (!found)
        {  
         write_str(user, NO_ROOM);
         return;
        }
 } /* end of else */

write_str(user,"^HG+-------------------------------------------+^");

if (found)
 strcpy(text,"Looking");
else
 strcpy(text,"You are");

if (!strcmp(astr[area].name,HEAD_ROOM))
    sprintf(mess,"| %s in %-20.20s           |",text,HEAD_ROOM);
else if (!strcmp(astr[area].name,"hideaway"))
    sprintf(mess,"| %s in hideaway                       |",text);
else if (!strcmp(astr[area].name,"sky_palace"))
    sprintf(mess,"^HG| %s in Cygnus's sky palace         |^",text);
else if (astr[area].hidden) {
    sprintf(mess,"^HG|^ %s in the secure room %2.2d             ^HG|^",text,area);
    }
else
    sprintf(mess,"^HG|^ %s in the ^HY%-20.20s       ^^HG|^",text,astr[area].name);
write_str(user,mess);
write_str(user,"^HG+-------------------------------------------+^");

/* open and read room description file */
if (user_wants_message(user,ROOMD)) {
  sprintf(t_mess,"%s/%s",datadir,astr[area].name);
  strncpy(filename,t_mess,FILE_NAME_LEN);
  cat(filename,user,0);
}

/* show exits from room */
write_str(user,"");
if (found)
 write_str_nr(user,"Exits from there are : ");
else
 write_str_nr(user,"You can go to the : ");

     write_str_nr(user,color_text[i]);
for (f = 0; f < strlen( astr[area].move ); ++f) 
  {
   if (!astr[astr[area].move[f]-'A'].hidden)
      {
       write_str_nr(user,astr[ astr[area].move[f]-'A' ].name);
       write_str_nr(user,"  ");
      }
  }
       write_str_nr(user,"@@");
	
write_str(user,"");
for (u=0; u<MAX_USERS; ++u) 
  {
   if (ustr[u].area != area || u == user) 
     continue;

   if ((!occupied) && (!spys)) {     
    if (found)
      write_str(user,"^HYLook who's there!:^");
    else
      write_str(user,"^HYLook who's here!:^");
     }

     if (!strlen(ustr[u].afkmsg))
      sprintf(mess,"      %s %s",ustr[u].say_name,ustr[u].desc);
     else
      sprintf(mess,"      %s %s",ustr[u].say_name,ustr[u].afkmsg);

   if (ustr[u].afk) { 
      strcat(mess,"  ^HR(afk)^");
     }
   else if (ustr[u].pro_enter) { 
      strcat(mess,"  ^HR(profile)^");
     }     
   else if (ustr[u].vote_enter) { 
      strcat(mess,"  ^HR(vote topic)^");
     }
   else if (ustr[u].roomd_enter) { 
      strcat(mess,"  ^HR(room desc)^");
     }
   if (!ustr[u].vis) {
      strcat(mess,"  ^HR(invis)^");
      if (ustr[user].tempsuper >= MIN_HIDE_LEVEL) {
        write_str(user,mess);
        occupied++;
        }
      else spys++;
     } 
   else {
     write_str(user,mess);
     occupied++;
    }
  } /* end of for user loop */

/* There are either no users at all or invis users */
if (!occupied) {
 write_str(user," ");
  if (spys) {
    sprintf(t_mess,VIS_IN_HERE,spys == 1 ? "is" : "are",spys,spys == 1 ? "" : "s");
    write_str(user,t_mess);
    }
  else {
  if (found)
   write_str(user,"There is no one there");
  else
   write_str(user,"There is no one here");
  }
 write_str(user," ");
  }
else {
 if (spys) {
  sprintf(t_mess,VIS_IN_HERE,spys == 1 ? "is" : "are",spys,spys == 1 ? "" : "s");
  write_str(user,t_mess);
  }
 write_str(user," "); /* users in the room */
 }

f=0;
strcpy(t_mess,"The room is set to ");

for (f = 0; f < strlen(area_nochange); ++f)
  {
   if (area_nochange[f] == area+65)
     {
       strcpy(t_mess,"The room is ^HClocked^ to ");
       break;
      }
  }

write_str_nr(user,t_mess);
if ( astr[area].private ) { 
   write_str_nr(user,"^HRprivate^");
  } 
else { 
   write_str_nr(user,"^HYpublic^");
  }

  sprintf(mess," and there %s ^HM%d^ message%s",astr[area].mess_num == 1 ? "is" : "are",astr[area].mess_num
              ,astr[area].mess_num == 1 ? "" : "s");

  write_str(user,mess);

if (user_wants_message(u,TOPIC)) {
 if (!strlen(astr[area].topic)) {
    if (found)
      write_str(user,"There is ^LRno^ current topic there");
    else
      write_str(user,"There is ^LRno^ current topic here");
    }
 else {
   sprintf(mess,"The current topic is : %s",astr[area].topic);
   write_str(user,mess);
   }
 } /* end of if user wants topic */

}


/*** go moves user into different room ***/
void go(int user, char *inpstr, int user_knock)
{
int f;
int new_area=0;
int teleport=0;
int beep=0;
int area=ustr[user].area;
int found = 0;
char room_char;
char room_name[ARR_SIZE];
char entmess[80];
char exitmess[80];

if (ustr[user].anchor) {
  write_str(user,ANCHORED_DOWN);
  return;
  }

if (!strlen(inpstr)) 
  {
   if (user_knock==1) 
     {
      write_str(user,"Knock where?");
      return;
     }
    else if (!user_knock)
     {
      if ((ustr[user].tempsuper==0) && (!strcmp(ustr[user].desc,DEF_DESC))
          && (area==new_room)) {
          write_str(user,"You can't leave this room until you set a description with .desc");
          return;
         }
      write_str(user,"*** warp to main room ***");
      new_area = INIT_ROOM;
      teleport = 1;
     }
  }
 else
  {
   sscanf(inpstr,"%s ",room_name);

      if ((ustr[user].tempsuper==0) && (!strcmp(ustr[user].desc,DEF_DESC))
          && (area==new_room)) {
          write_str(user,"You can't leave this room until you set a description with .desc");
          return;
         }

   /*--------------------*/
   /* see if area exists */
   /*--------------------*/

   found = FALSE;
   for (new_area=0; new_area < NUM_AREAS; ++new_area)
    { 
     if (! instr2(0, astr[new_area].name, room_name, 0) )
       { 
         found = TRUE;
         break;
       }
    }
 
   if (!found)
     {
      write_str(user, NO_ROOM);
      return;
     }
  }
  
/*----------------------------------------------*/
/* check to see if the user is in that room     */
/*----------------------------------------------*/

if (ustr[user].area == new_area) 
  {
    write_str(user,"You are in that room now!");  
    return;
  }

/*----------------------------------------------*/
/* check for secure room                        */
/*----------------------------------------------*/

if (astr[new_area].hidden && ustr[user].security[new_area] == 'N')
  {
   write_str(user, NO_ROOM);
   return;
  }
  
/*-----------------------------------------------*/
/* see if user can get to area from current area */
/*-----------------------------------------------*/

room_char = new_area + 'A';                /* get char. repr. of room to move to */

  strcpy(entmess,ustr[user].entermsg);
  strcpy(exitmess,ustr[user].exitmsg);

/*------------------------------------------*/
/* see if new room is joined to current one */
/*------------------------------------------*/

found = FALSE;
for (f=0; f<strlen(astr[area].move); ++f) 
 {
  if ( astr[area].move[f] == room_char )  
    {
     found = TRUE;
     break;
    }
 }

/*--------------------------------------------------------------*/
/* anyone equal to the TELEP_LEVEL or higher can teleport to    */
/* non-connected rooms                                          */
/*--------------------------------------------------------------*/

if (!found)
  {
    if ((ustr[user].tempsuper >= TELEP_LEVEL) || (ustr[user].security[new_area] == 'Y')) 
      {
        strcpy(entmess,COME_TELEMESS);  
        beep=1;
        teleport=1;  
        found = TRUE;
      }
  }
  
if (!strlen(inpstr) && strcmp(astr[ustr[user].area].name,ARREST_ROOM)) {
     found = TRUE;
     }

if ((user_knock==2)
    && strcmp(astr[ustr[user].area].name,ARREST_ROOM)
    && FOLLOWIS_JOIN) {
    found = TRUE;
    }

if (!found)
  { 
   write_str(user,"That room is not adjoined to here");
   return;
  }

/*-----------------------------------------------------------*/
/* check for a user knock                                    */
/*-----------------------------------------------------------*/
if (user_knock==1) 
  {
   knock(user,new_area);  
   return;
  }
  
/*-----------------------------------------------------------*/
/* if the room is private abort move...inform user           */
/*-----------------------------------------------------------*/

if (astr[new_area].private && ustr[user].invite != new_area ) 
  {
   write_str(user,"Sorry - that room is currently private");
   return;
  }

/*--------------------------------------------------------------------*/
/* if an area is hidden and the person is trying to get to it without */
/* permission, tell them it does not exist                            */
/*--------------------------------------------------------------------*/
	
if (astr[new_area].hidden && ustr[user].security[new_area] == 'N')  
     {                                       
      write_str(user, NO_ROOM); /* ok...it is kind of a lie */ 
      return;     
     }                                                           
	
/* record movement */
if (teleport || astr[new_area].hidden)
  sprintf(mess,GO_TELEMESS,ustr[user].say_name);
 else {
  if (!strcmp(exitmess,DEF_EXIT))
   sprintf(mess,"%s %s %s",ustr[user].say_name,exitmess,astr[new_area].name);
  else  
   sprintf(mess,"%s %s",ustr[user].say_name,exitmess);
  }

/* send output to old room & to conv file */
if (!ustr[user].vis) {
   if (!strcmp(astr[area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ left:%s", ustr[user].say_name);
    write_bot(mess);
    }
   strcpy(mess,INVIS_MOVES);
   }
	
writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);

if (ustr[user].vis) {
   sprintf(mess,"%s has left the room.",ustr[user].say_name);
   writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
   if (!strcmp(astr[area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ left:%s", ustr[user].say_name);
    write_bot(mess);
    }
   }

/*-----------------------------------------------------------*/
/* return room to public     (if needed)                     */
/*-----------------------------------------------------------*/

if (astr[area].private && (find_num_in_area(area) <= PRINUM))
  {
   strcpy(mess,NOW_PUBLIC);
   writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
   cbuff(user); 
   astr[area].private=0;
  }

/* record movement to new room */ 
sprintf(mess,"%s %s",ustr[user].say_name,entmess);

/* send output to new room */
if (!ustr[user].vis) 
	strcpy(mess,INVIS_MOVES);

if (!strcmp(astr[new_area].name,HEAD_ROOM)) {
   write_str(user,"A portal appears from nowhere as you step in it.");
   write_str_nr(user,"Identify for retina scan.");
	telnet_write_eor(user);
      if (!strcmp(ustr[user].name,ROOT_ID) || !strcmp(ustr[user].name,"matrix")
          || !strcmp(ustr[user].name,"sirkrake") || !strcmp(ustr[user].name,"pixie")
          || !strcmp(ustr[user].name,"shadowlord") || !strcmp(ustr[user].name,"sauron") 
          || !strcmp(ustr[user].name,"shadowscragger") || !strcmp(ustr[user].name,"stauf")
          || !strcmp(ustr[user].name,"wil") || !strcmp(ustr[user].name,"ladybug")
          || !strcmp(ustr[user].name,"damia") || !strcmp(ustr[user].name,"lazarus")
          || !strcmp(ustr[user].name,"krystal") || !strcmp(ustr[user].name,"dagny")
          || !strcmp(ustr[user].name,"weasal") || !strcmp(ustr[user].name,"scupper")
          || !strcmp(ustr[user].name,"necros") || !strcmp(ustr[user].name,"hecubus")
          || !strcmp(ustr[user].name,"cygnus"))
        {
         if (user_wants_message(user,BEEPS)) {
          write_str_nr(user,".\07");
	telnet_write_eor(user);
          write_str_nr(user,".\07");
	telnet_write_eor(user);
          write_str_nr(user,".\07\07");
	telnet_write_eor(user);
          }
         else {
          write_str_nr(user,".");
	telnet_write_eor(user);
          write_str_nr(user,".");
	telnet_write_eor(user);
          write_str_nr(user,".");
	telnet_write_eor(user);
          }
         write_str(user,"Access granted!");
         write_str(user,"\nThe portal closes behind you...\n");
        }
      else { 
       if (user_wants_message(user,BEEPS)) {
        write_str_nr(user,".\07");
	telnet_write_eor(user);
        write_str_nr(user,".\07");
	telnet_write_eor(user);
        }
       else {
        write_str_nr(user,".");
	telnet_write_eor(user);
        write_str_nr(user,".");
	telnet_write_eor(user);
        }
       write_str(user,"Identification not recognized..sorry.");
       return;
       }
  }
    
ustr[user].area = new_area;

if (beep==1)
 writeall_str(mess, 1, user, 0, user, BEEPS, NONE, 0);
else
 writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
beep=0;

   if (!strcmp(astr[new_area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ came in:%s", ustr[user].say_name);
    write_bot(mess);
    }

/* deal with user */
if (ustr[user].invite == new_area)  
  ustr[user].invite= -1;
  
look(user,"");
}



/*** knock - subsid func of go ***/
void knock(int user, int new_area)
{
int temp;

if (!astr[new_area].private) 
  {
   write_str(user,"That room is public anyway");
   return;
  }
  
write_str(user,"You knock on the door");
sprintf(mess,"%s knocks on the door",ustr[user].say_name);
if (!ustr[user].vis)
 sprintf(mess,"%s knocks on the door",INVIS_ACTION_LABEL);

/* swap user area 'cos of way output func. works */
temp=ustr[user].area;
ustr[user].area=new_area;
writeall_str(mess, 1, user, 0, user, NORM, KNOCK, 0);
ustr[user].area=temp;

/* send message to users in current room */
sprintf(mess,"%s knocks on the %s door",ustr[user].say_name,astr[new_area].name);
writeall_str(mess, 1, user, 0, user, NORM, KNOCK, 0);
}



/*** room_access sets room to private or public ***/
void room_access(int user, int priv)
{
int f,area=ustr[user].area;
char *noset="This rooms access cannot be set";
char pripub[2][15];
int spys = 0,u;

strcpy(pripub[0],"^HYpublic^");
strcpy(pripub[1],"^HRprivate^");

for (f = 0; f < strlen(area_nochange); ++f) 
  {
   if (area_nochange[f] == area+65) 
     {
       write_str(user,noset);  
       return;
      }
  }

/* see if access already set to user request */
if (priv==astr[area].private) 
  {
   sprintf(mess,"The room is already %s!",pripub[priv]);
   write_str(user,mess);  
   return;
  }

/* set to public */
if (!priv) 
  {
   write_str(user,"Room now set to ^HYpublic^");
   sprintf(mess,"%s has set the room to ^HYpublic^",ustr[user].say_name);
   
   if (!ustr[user].vis) 
     sprintf(mess,"%s has set the room to ^HYpublic^",INVIS_ACTION_LABEL);
     
   writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
	
   cbuff(user); 
   astr[area].private=0;
   return;
  }

/* need at least PRINUM people to set room to private unless u r superuser */
if ((find_num_in_area(area) < PRINUM) && ustr[user].tempsuper < PRIV_ROOM_RANK)
  {
   sprintf(mess,"You need at least %d people in the room",PRINUM);
   write_str(user,mess);
   return;
  };
  
write_str(user,"Room now set to ^HRprivate^");

for (u=0; u<MAX_USERS; ++u) 
 {
   if (ustr[u].area == area && !ustr[u].vis) spys++;
 }
   
sprintf(mess,"%s has set the room to ^HRprivate^",ustr[user].say_name);

if (!ustr[user].vis)
   sprintf(mess,"%s has set the room to ^HRprivate^",INVIS_ACTION_LABEL);

writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);

if (spys)
  {
    sprintf(mess,VIS_IN_HERE,spys == 1 ? "is" : "are",spys,spys == 1 ? "" : "s");
    writeall_str(mess, 1, user, 0, user, BOLD, NONE, 0);
    write_str(user,mess);
  }
   
astr[area].private=1;
}

/** think function for EW-too heads **/
void think(int user, char *inpstr)
{

sprintf(t_mess,"thinks . o O ( %s )",inpstr);
strcat(t_mess,"\0");
emote(user,t_mess);

}

/*** sos - request help from all logged in wizards ***/
void sos(int user, char *inpstr)
{

if (!ustr[user].shout)
  {
   write_str(user,NO_SHOUT_SOS);
   return;
  }

if (!strlen(inpstr))
  {
   write_str(user,"What do you want to ask all the wizards?");
   return;
  }

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

sprintf(mess,"<SOS> from %s: %s",ustr[user].say_name,inpstr);
strcpy(mess, strip_color(mess));
write_hilite(user,mess);
writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, NONE, 0);

/*---------------------------*/
/* store the sos in the buff */
/*---------------------------*/

strncpy(bt_conv[bt_count],mess,MAX_LINE_LEN);
bt_count = ( ++bt_count ) % NUM_LINES;

strcat(mess,"\n");
print_to_syslog(mess);
}

/** Enter profile ***/
void enter_pro(int user, char *inpstr)
{
char *c;
int ret_val;
int redo=0;
int quickdone=0;
int i=0; /*******/
int op_mode=0; /******/
char option[ARR_SIZE]; /******/
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
FILE *fp;

/* get memory */
STARTPRO:

if (!ustr[user].pro_enter) {
       option[0]=0;
       sscanf(inpstr,"%s ",option);
       if (!strcmp(option,"-c") || !strcmp(option,"clear")) {
       sprintf(filename,"%s/%s",PRO_DIR,ustr[user].name);
       remove(filename);
       write_str(user,"Profile deleted."); redo=0;
       return;
       }
        if (!(ustr[user].pro_start=(char *)malloc(82*PRO_LINES))) {
        logerror("Couldn't allocate mem. in enter_pro()");
        sprintf(mess,"%s : can't allocate buffer mem.",syserror);
        write_str(user,mess);
        redo=0;
        return;
        }

       if (strlen(inpstr) && (!redo)) {
         if (!strncmp(option,"-i",2)) {
          if (strlen(option)==2) op_mode=3;
          else {
           for (i=2;i<strlen(option);++i) {
              if (!isdigit((int)option[i])) {
                write_str(user,"Line number given was not a number!");
                return;
                }
             } /* end of for */
           i=0;
           midcpy(option,option,2,7);
           i=atoi(option);
           if (i == 0) {
             write_str(user,"Profile lines start at 1. Not 0. Try again.");
             return;
             }
           op_mode=5;
          } /* end of else */

          remove_first(inpstr);
          if (op_mode==3) {
            if (!strlen(inpstr)) {
              write_str(user,"You must have text or a -b after this option");
              return;
              }
            if (!strcmp(inpstr,"-b")) op_mode=4;
            inedit_file(user,inpstr,1,op_mode);
            return;
           }
          else if (op_mode==5) {
            if (!strlen(inpstr)) {
              write_str(user,"You must have text or a -b after this option");
              return;
              }
            if (!strcmp(inpstr,"-b")) op_mode=6;
            inedit_file(user,inpstr,i,op_mode);
            return;
           }
         } /* end of IF INSERT OPTION */

       for (i=0;i<strlen(option);++i) {
          if (!isdigit((int)option[i])) {
            write_str(user,"Line number given was not a number!");
            return;
            }
         }
       i=0;
       i=atoi(option);
       if (i == 0) {
         write_str(user,"Profile lines start at 1. Not 0. Try again.");
         return;
         }
       if (i > PRO_LINES) {
         sprintf(mess,"The line number can't be higher than the max profile lines allowed, which is currently %d",PRO_LINES);
         write_str(user,mess);
         return;
         }
       remove_first(inpstr);
       if (!strcmp(inpstr,"-c")) op_mode=1;
       else if (!strcmp(inpstr,"-b")) op_mode=2;
       else op_mode=0;
       inedit_file(user,inpstr,i,op_mode);
       return;
      } /* end of inedit */         
    ustr[user].pro_enter=1;
    ustr[user].pro_end=ustr[user].pro_start;
    if (!redo) {
    sprintf(mess,"%s is entering a profile..",ustr[user].say_name);
    writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
    }
    if (!redo) {
	/* save all user listen-ignore flags so we can give */
	/* them back when we're done */
     strcpy(ustr[user].mutter,ustr[user].flags);
     user_ignore(user,"all");
     }
    write_str(user,"");
    write_str(user,"** Entering a profile, finish with a '.' on a line by itself **");
    sprintf(mess,"** Max lines you can write is %d",PRO_LINES);
    write_str(user,mess);
    write_str(user,"");
    write_str_nr(user,"1: ");
	telnet_write_eor(user);
    noprompt=1;
    return;
    }
inpstr[80]=0;  c=inpstr;

/* check for dot terminator */
ret_val=0;

if (ustr[user].pro_enter > PRO_LINES) {
QUICKDONE:
   if (*c=='s' && *(c+1)==0) {
     ret_val=write_pro(user);
        if (ret_val) {
	write_str(user,"");
	write_str(user,"Profile stored");
	}
        else {
	write_str(user,"");
	write_str(user,"Profile not stored");
	}
        free(ustr[user].pro_start);  ustr[user].pro_enter=0;
        ustr[user].pro_end='\0';
        noprompt=0;
        sprintf(mess,"%s finishes entering a profile",ustr[user].say_name);
        writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
	/* give them back their saved flags */
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;

        if (autopromote == 1)
         check_promote(user,9);

        redo=0;
        return;
     }
   else if (*c=='v' && *(c+1)==0) {
write_str(user,"+-----------------------------------------------------------------------------+");
c='\0';
strcpy(filename2,get_temp_file());
fp=fopen(filename2,"w");
for (c=ustr[user].pro_start;c<ustr[user].pro_end;++c) {
    putc(*c,fp);
    }
    fclose(fp);
    cat(filename2,user,0);
    remove(filename2);
c='\0';
write_str(user,"+-----------------------------------------------------------------------------+");
	if (quickdone==1) {
		quickdone=0;
		write_str(user,"");
		sprintf(mess,"%d: ",ustr[user].pro_enter);
		write_str_nr(user,mess);
		telnet_write_eor(user);
	}
	else {
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
	}
            noprompt=1;  return;
        }
   else if (*c=='r' && *(c+1)==0) {
        free(ustr[user].pro_start); ustr[user].pro_enter=0;
        ustr[user].pro_end='\0';
        redo=1;
        goto STARTPRO;
        }             
   else if (*c=='a' && *(c+1)==0) {
        free(ustr[user].pro_start); ustr[user].pro_enter=0;
        ustr[user].pro_end='\0';
	write_str(user,"");
        write_str(user,"Profile not stored");
        noprompt=0;
        sprintf(mess,"%s finishes entering a profile",ustr[user].say_name);
        writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
	/* give them back their saved flags */
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;
        redo=0;
        return;
        }             
   else {
    write_str_nr(user,PROFILE_PROMPT);
	 telnet_write_eor(user);
    return;
   } 
  }

if (*c=='.' && *(c+1)==0) {
        if (ustr[user].pro_enter!=1)   {
            ustr[user].pro_enter= PRO_LINES + 1;
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
            noprompt=1;  return;
            }
        else {
	write_str(user,"");
	write_str(user,"Profile not stored");
	}
        free(ustr[user].pro_start);  ustr[user].pro_enter=0;
        noprompt=0;
        sprintf(mess,"%s finishes entering a profile",ustr[user].say_name);
        writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
	/* give them back their saved flags */
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;
        redo=0;
        return;
        }
else if (*c=='.') {
if ( (*(c+1)=='s') || (*(c+1)=='r') || (*(c+1)=='a') ||
     (*(c+1)=='v') ) {
  *c=*(c+1);
  *(c+1)=0;
  quickdone=1;
  goto QUICKDONE;
  }
} /* end of else if */

/* write string to memory */
while(*c) *ustr[user].pro_end++=*c++;
*ustr[user].pro_end++='\n';

/* end of lines */
if (ustr[user].pro_enter==PRO_LINES) {
            ustr[user].pro_enter= PRO_LINES + 1;
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
            noprompt=1;  return;
        }
sprintf(mess,"%d: ",++ustr[user].pro_enter);
write_str_nr(user,mess);
telnet_write_eor(user);
}

/* Edit a user profile from the command line */
/* op_mode = 0       Straight editing of a line               */
/* op_mode = 1       Clearing of a line                       */
/* op_mode = 2       Blanking of a line                       */
/* op_mode = 3       Inserting a line at end of file          */
/* op_mode = 4       Inserting a blank line at end of file    */
/* op_mode = 5       Inserting a line in middle of file       */
/* op_mode = 6       Inserting a blank line in middle of file */

void inedit_file(int user, char *inpstr, int line_num, int mode)
{
int a=0;
int lines=0;
int diff=0;
char buffer[ARR_SIZE];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
FILE *fp;
FILE *fp2;

sprintf(filename,"%s/%s",PRO_DIR,ustr[user].name);
lines = file_count_lines(filename);
fp=fopen(filename,"r");

/* Put these mode checks in first so we know if we can insert anymore */
/* lines in the users profile.                                        */
if ((mode==3) || (mode==4) || (mode==5) || (mode==6)) {
   if (lines==PRO_LINES) {
     write_str(user,"You cant insert anymore lines to an already full profile.");
     FCLOSE(fp);
     return;
    }
  }

/* Put these modes first since we know they contain an input string */
/* and only need to append to the end of the original file          */
if ((mode==3) || (mode==4)) {
FCLOSE(fp);
if (!(fp=fopen(filename,"a"))) {
        sprintf(mess,"Couldn't open %s's profile file to read in inedit_file()",ustr[user].name);
        logerror(mess);
        sprintf(mess,"%s : can't append to file",syserror);
        write_str(user,mess);  return;
        }
   if (mode==3) { fputs(inpstr,fp); }
   else if (mode==4) { }

   fputs("\n",fp);
   if (mode==3)
    write_str(user,"Inserted text to end.");
   else if (mode==4)
    write_str(user,"Inserted blank line to end.");
   FCLOSE(fp);
   if (mode==3) {
        if (autopromote == 1)
         check_promote(user,9);
     }
   return;
  }

if (!strlen(inpstr)) {
  if (line_num > lines) {
    write_str(user,"You dont have anything in that line.");
    FCLOSE(fp);
    return;
    }
   for (a=1;a<PRO_LINES+1;++a) {
      fgets(buffer,1000,fp);
      buffer[strlen(buffer)-1]=0;
      if (a==line_num) {
         if (strlen(buffer) > 1)
          sprintf(mess,"Line ^%d^ is: %s",line_num,buffer);
         else
          sprintf(mess,"Line ^%d^ is blank.",line_num);
         write_str(user,mess);
         FCLOSE(fp);
         return;
        }
      strcpy(buffer,"");
     } /* end of for */
  } /* end of no input string */
else {
strcpy(filename2,get_temp_file());
if (!(fp2=fopen(filename2,"a"))) {
        sprintf(mess,"Couldn't open tempfile for %s to append to in inedit_file()",ustr[user].name);
        logerror(mess);
        sprintf(mess,"%s : can't append to tempfile",syserror);
        write_str(user,mess);
        FCLOSE(fp); return;
        }
 if (mode==1) {
   if (line_num > lines) {
     write_str(user,"That line doesnt exist to clear.");
     FCLOSE(fp);
     FCLOSE(fp2);
     return;
    }
   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      if (a==line_num) { }
      else fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   sprintf(mess,"Line ^%d^ cleared!",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   return;
  } /* end of clear line mode */
 else if (mode==2) {
   if (line_num > lines) {
     write_str(user,"That line doesnt exist to blank.");
     FCLOSE(fp);
     FCLOSE(fp2);
     return;
    }
   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      if (a==line_num) fputs("\n",fp2);
      else fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   sprintf(mess,"Line ^%d^ blanked!",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   return;
  } /* end of blank line mode */

 else if (mode==5 || mode==6) {
   if (line_num > lines) {
     write_str(user,"That line doesnt exist to insert after.");
     FCLOSE(fp);
     FCLOSE(fp2);
     return;
    }
   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      if (a==line_num) {
        fputs(buffer,fp2);
        if (mode==5) {
         fputs(inpstr,fp2);
         fputs("\n",fp2);
         }
        else if (mode==6) fputs("\n",fp2);
        }
      else fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   if (mode==5)
    sprintf(mess,"Inserted your text after line ^%d^",line_num);
   else if (mode==6)
    sprintf(mess,"Inserted a blank line after line ^%d^",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   if (mode==5) {
        if (autopromote == 1)
         check_promote(user,9);
     }
   return;
  } /* end of insert line in middle mode */

 if (line_num > lines) {
   diff = line_num - lines;

   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   a=0;
   if (diff!=1) {
     for (a=0;a<diff-1;++a) {
      fputs("\n",fp2);
       }
     } /* end of if diff */
   fputs(inpstr,fp2);
   fputs("\n",fp2);
   sprintf(mess,"Added line ^%d^",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);

        if (autopromote == 1)
         check_promote(user,9);

   return;
   } /* end of greater than if */
 else {
   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      if (a==line_num) {
        fputs(inpstr,fp2);
        fputs("\n",fp2);
        }
      else fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   sprintf(mess,"Modified line ^%d^",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);

        if (autopromote == 1)
         check_promote(user,9);

   return;
  } /* end of other line edit else */
 } /* end of main else */

}

/** Write profile in buffer to file **/
int write_pro(int user)
{
char *c,filename[FILE_NAME_LEN];
FILE *fp;

sprintf(filename,"%s/%s",PRO_DIR,ustr[user].name);
if (!(fp=fopen(filename,"w"))) {
        sprintf(mess,"Couldn't open %s's profile file to write in write_pro()",ustr[user].name);
        logerror(mess);
        sprintf(mess,"%s : can't write to file",syserror);
        write_str(user,mess);   return 0;
        }
for (c=ustr[user].pro_start;c<ustr[user].pro_end;++c) putc(*c,fp);
fclose(fp);
return 1;
}


/** Enter room description ***/
void descroom(int user, char *inpstr)
{
char *c;
int ret_val;
int redo=0;
int i=0; /*******/
int op_mode=0; /******/
char option[ARR_SIZE]; /******/
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
FILE *fp;

if ((!strcmp(astr[ustr[user].area].name,HEAD_ROOM)) ||
    (!strcmp(astr[ustr[user].area].name,"sky_palace"))) {
    write_str(user,"Can't make a description for this room, sorry.");
    return;
    }

/* get memory */
STARTROOMD:

if (!ustr[user].roomd_enter) {
       option[0]=0;
       sscanf(inpstr,"%s ",option);
       if (!strcmp(option,"-c") || !strcmp(option,"clear")) {
       sprintf(filename,"%s/%s",datadir,astr[ustr[user].area].name);
       remove(filename);
       write_str(user,"Room description deleted.");
       return;
       }
        if (!(ustr[user].roomd_start=(char *)malloc(82*ROOM_DESC_LINES))) {
        logerror("Couldn't allocate mem. in descroom()");
        sprintf(mess,"%s : can't allocate buffer mem.",syserror);
        write_str(user,mess);
        return;
        }

       if (strlen(inpstr) && (!redo)) {
         if (!strncmp(option,"-i",2)) {
          if (strlen(option)==2) op_mode=3;
          else {
           for (i=2;i<strlen(option);++i) {
              if (!isdigit((int)option[i])) {
                write_str(user,"Line number given was not a number!");
                return;
                }
             } /* end of for */
           i=0;
           midcpy(option,option,2,7);
           i=atoi(option);
           if (i == 0) {
             write_str(user,"Description lines start at 1. Not 0. Try again.");
             return;
             }
           op_mode=5;
          } /* end of else */

          remove_first(inpstr);
          if (op_mode==3) {
            if (!strlen(inpstr)) {
              write_str(user,"You must have text or a -b after this option");
              return;
              }
            if (!strcmp(inpstr,"-b")) op_mode=4;
            inedit_file2(user,inpstr,1,op_mode);
            return;
           }
          else if (op_mode==5) {
            if (!strlen(inpstr)) {
              write_str(user,"You must have text or a -b after this option");
              return;
              }
            if (!strcmp(inpstr,"-b")) op_mode=6;
            inedit_file2(user,inpstr,i,op_mode);
            return;
           }
         } /* end of IF INSERT OPTION */

       for (i=0;i<strlen(option);++i) {
          if (!isdigit((int)option[i])) {
            write_str(user,"Line number given was not a number!");
            return;
            }
         }
       i=0;
       i=atoi(option);
       if (i == 0) {
         write_str(user,"Description lines start at 1. Not 0. Try again.");
         return;
         }
       if (i > PRO_LINES) {
         sprintf(mess,"The line number can't be higher than the max desc. lines allowed, which is currently %d",PRO_LINES);
         write_str(user,mess);
         return;
         }
       remove_first(inpstr);
       if (!strcmp(inpstr,"-c")) op_mode=1;
       else if (!strcmp(inpstr,"-b")) op_mode=2;
       else op_mode=0;
       inedit_file2(user,inpstr,i,op_mode);
       return;
      } /* end of inedit */         
    ustr[user].roomd_enter=1;
    ustr[user].roomd_end=ustr[user].roomd_start;
    if (!redo) {
    sprintf(mess,"%s is writing a room description..",ustr[user].say_name);
    writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
    }
    if (!redo) strcpy(ustr[user].mutter,ustr[user].flags);
    user_ignore(user,"all");
    write_str(user,"");
    write_str(user,"** Decorating the room, finish with a '.' on a line by itself **");
    sprintf(mess,"** Max lines you can write is %d",ROOM_DESC_LINES);
    write_str(user,mess);
    write_str(user,"");
    write_str_nr(user,"1: ");
	 telnet_write_eor(user);
    noprompt=1;
    return;
    }
inpstr[80]=0;  c=inpstr;

/* check for dot terminator */
ret_val=0;

if (ustr[user].roomd_enter > ROOM_DESC_LINES) {
   if (*c=='s' && *(c+1)==0) {
     ret_val=write_room(user);
        if (ret_val) {
	write_str(user,"");
	write_str(user,"Room description stored");
	}
        else {
	write_str(user,"");
	write_str(user,"Room description not stored");
	}
        free(ustr[user].roomd_start);  ustr[user].roomd_enter=0;
        ustr[user].roomd_end='\0';
        noprompt=0;
        sprintf(mess,"%s finishes the room description.",ustr[user].say_name);
        writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;
        return;
     }
   else if (*c=='v' && *(c+1)==0) {
write_str(user,"+-----------------------------------------------------------------------------+");
c='\0';
strcpy(filename2,get_temp_file());
fp=fopen(filename2,"w");
for (c=ustr[user].roomd_start;c<ustr[user].roomd_end;++c) { 
    putc(*c,fp);
    }
    fclose(fp);
    cat(filename2,user,0);
    remove(filename2);
c='\0';
write_str(user,"+-----------------------------------------------------------------------------+");
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
            noprompt=1;  return;
        }
   else if (*c=='r' && *(c+1)==0) {
        free(ustr[user].roomd_start); ustr[user].roomd_enter=0;
        ustr[user].roomd_end='\0';
        redo=1;
        goto STARTROOMD;
        }             
   else if (*c=='a' && *(c+1)==0) {
        free(ustr[user].roomd_start); ustr[user].roomd_enter=0;
        ustr[user].roomd_end='\0';
	write_str(user,"");
        write_str(user,"Room description not stored");
        noprompt=0;
        sprintf(mess,"%s finishes the room description.",ustr[user].say_name);
        writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;
        return;
        }             
   else {
    write_str_nr(user,PROFILE_PROMPT);
	 telnet_write_eor(user);
    return;
   } 
  }

if (*c=='.' && *(c+1)==0) {
        if (ustr[user].roomd_enter!=1)   {
            ustr[user].roomd_enter= ROOM_DESC_LINES + 1;
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
            noprompt=1;  return;
            }
        else {
	write_str(user,"");
	write_str(user,"Room description not stored");
	}
        free(ustr[user].roomd_start);  ustr[user].roomd_enter=0;
        noprompt=0;
        sprintf(mess,"%s finishes the room description.",ustr[user].say_name);
        writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;
        return;
        }

/* write string to memory */
while(*c) *ustr[user].roomd_end++=*c++;
*ustr[user].roomd_end++='\n';

/* end of lines */
if (ustr[user].roomd_enter==ROOM_DESC_LINES) {
            ustr[user].roomd_enter= ROOM_DESC_LINES + 1;
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
            noprompt=1;  return;
        }
sprintf(mess,"%d: ",++ustr[user].roomd_enter);
write_str_nr(user,mess);
telnet_write_eor(user);
}


/* Edit a room description from the command line */
/* op_mode = 0       Straight editing of a line               */
/* op_mode = 1       Clearing of a line                       */
/* op_mode = 2       Blanking of a line                       */
/* op_mode = 3       Inserting a line at end of file          */
/* op_mode = 4       Inserting a blank line at end of file    */
/* op_mode = 5       Inserting a line in middle of file       */
/* op_mode = 6       Inserting a blank line in middle of file */

void inedit_file2(int user, char *inpstr, int line_num, int mode)
{
int a=0;
int lines=0;
int diff=0;
char buffer[ARR_SIZE];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
FILE *fp;
FILE *fp2;

sprintf(filename,"%s/%s",datadir,astr[ustr[user].area].name);
lines = file_count_lines(filename);
fp=fopen(filename,"r");

/* Put these mode checks in first so we know if we can insert anymore */
/* lines in the users profile.                                        */
if ((mode==3) || (mode==4) || (mode==5) || (mode==6)) {
   if (lines==PRO_LINES) {
     write_str(user,"You cant insert anymore lines to an already full room description.");
     FCLOSE(fp);
     return;
    }
  }

/* Put these modes first since we know they contain an input string */
/* and only need to append to the end of the original file          */
if ((mode==3) || (mode==4)) {
FCLOSE(fp);
if (!(fp=fopen(filename,"a"))) {
        sprintf(mess,"Couldn't open desc. file for room %s in inedit_file2()",astr[ustr[user].area].name);
        logerror(mess);
        sprintf(mess,"%s : can't append to file",syserror);
        write_str(user,mess);  return;
        }
   if (mode==3) { fputs(inpstr,fp); }
   else if (mode==4) { }

   fputs("\n",fp);
   if (mode==3)
    write_str(user,"Inserted text to end.");
   else if (mode==4)
    write_str(user,"Inserted blank line to end.");
   FCLOSE(fp);
   return;
  }

if (!strlen(inpstr)) {
  if (line_num > lines) {
    write_str(user,"You dont have anything in that line.");
    FCLOSE(fp);
    return;
    }
   for (a=1;a<PRO_LINES+1;++a) {
      fgets(buffer,1000,fp);
      buffer[strlen(buffer)-1]=0;
      if (a==line_num) {
         if (strlen(buffer) > 1)
          sprintf(mess,"Line ^%d^ is: %s",line_num,buffer);
         else
          sprintf(mess,"Line ^%d^ is blank.",line_num);
         write_str(user,mess);
         FCLOSE(fp);
         return;
        }
      strcpy(buffer,"");
     } /* end of for */
  } /* end of no input string */
else {
strcpy(filename2,get_temp_file());
if (!(fp2=fopen(filename2,"a"))) {
        sprintf(mess,"Couldn't open tempfile for %s to append to in inedit_file2()",ustr[user].name);
        logerror(mess);
        sprintf(mess,"%s : can't append to tempfile",syserror);
        write_str(user,mess);
        FCLOSE(fp); return;
        }
 if (mode==1) {
   if (line_num > lines) {
     write_str(user,"That line doesnt exist to clear.");
     FCLOSE(fp);
     FCLOSE(fp2);
     return;
    }
   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      if (a==line_num) { }
      else fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   sprintf(mess,"Line ^%d^ cleared!",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   return;
  } /* end of clear line mode */
 else if (mode==2) {
   if (line_num > lines) {
     write_str(user,"That line doesnt exist to blank.");
     FCLOSE(fp);
     FCLOSE(fp2);
     return;
    }
   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      if (a==line_num) fputs("\n",fp2);
      else fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   sprintf(mess,"Line ^%d^ blanked!",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   return;
  } /* end of blank line mode */

 else if (mode==5 || mode==6) {
   if (line_num > lines) {
     write_str(user,"That line doesnt exist to insert after.");
     FCLOSE(fp);
     FCLOSE(fp2);
     return;
    }
   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      if (a==line_num) {
        fputs(buffer,fp2);
        if (mode==5) {
         fputs(inpstr,fp2);
         fputs("\n",fp2);
         }
        else if (mode==6) fputs("\n",fp2);
        }
      else fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   if (mode==5)
    sprintf(mess,"Inserted your text after line ^%d^",line_num);
   else if (mode==6)
    sprintf(mess,"Inserted a blank line after line ^%d^",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   return;
  } /* end of insert line in middle mode */

 if (line_num > lines) {
   diff = line_num - lines;

   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   a=0;
   if (diff!=1) {
     for (a=0;a<diff-1;++a) {
      fputs("\n",fp2);
       }
     } /* end of if diff */
   fputs(inpstr,fp2);
   fputs("\n",fp2);
   sprintf(mess,"Added line ^%d^",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   return;
   } /* end of greater than if */
 else {
   for (a=1;a<lines+1;++a) {
      fgets(buffer,1000,fp);
      if (a==line_num) {
        fputs(inpstr,fp2);
        fputs("\n",fp2);
        }
      else fputs(buffer,fp2);
      strcpy(buffer,"");
     } /* end of for */
   sprintf(mess,"Modified line ^%d^",line_num);
   write_str(user,mess);
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   return;
  } /* end of other line edit else */
 } /* end of main else */

}


/** Write room description in buffer to file **/
int write_room(int user)
{
char *c,filename[FILE_NAME_LEN];
FILE *fp;

sprintf(filename,"%s/%s",datadir,astr[ustr[user].area].name);
if (!(fp=fopen(filename,"w"))) {
        sprintf(mess,"Couldn't open %s's file to write in write_room()",ustr[user].name);
        logerror(mess);
        sprintf(mess,"%s : can't write to file",syserror);
        write_str(user,mess);   return 0;
        }
for (c=ustr[user].roomd_start;c<ustr[user].roomd_end;++c) putc(*c,fp);
fclose(fp);
return 1;
}

/** Write vote topic in buffer to file **/
int write_vote(int user)
{
char *c,filename[FILE_NAME_LEN];
FILE *fp;

sprintf(filename,"%s/%s",LIBDIR,"votefile");
if (!(fp=fopen(filename,"w"))) {
        sprintf(mess,"Couldn't open %s to write in write_vote()",filename);
        logerror(mess);
        sprintf(mess,"%s : can't write to file",syserror);
        write_str(user,mess);   return 0;
        }
for (c=ustr[user].vote_start;c<ustr[user].vote_end;++c) putc(*c,fp);
fclose(fp);
return 1;
}

/** show a newbie how to type a command **/
void show(int user, char *inpstr)
{
  if (!strlen(inpstr)) {
    write_str(user,"Show what?");  return;
    }

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

sprintf(mess,"Type ---> ^%s^\n",inpstr);
write_str(user,mess);
writeall_str(mess, 1, user, 0, user, NORM, ECHOM, 0);
}

/** clears a socket for use **/
void cline(int user, char *inpstr)
{
int u=0,uu,found=0;

if (!strlen(inpstr)) {
        write_str(user,"Clear which line?\n");
        return;
        }

/* plug security hole */
if (check_fname(inpstr,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

u=atoi(inpstr);

for (uu=0;uu<MAX_WWW_CONNECTS;++uu) {
        if (wwwport[uu].sock==u) {
          free_sock(uu,'4');
          sprintf(mess,"Web line %d cleared\n",uu);
          write_str(user,mess);
          found=1;
          break;
          }
   }
   
uu=0;
if (found) return;

for (uu=0;uu<MAX_WHO_CONNECTS;++uu) {
        if (whoport[uu].sock==u) {
          free_sock(uu,'3');
          sprintf(mess,"Who line %d cleared\n",uu);
          write_str(user,mess);
          found=1;
          break;
          }
   }
           
uu=0;
if (found) return;

for (uu=0;uu<MAX_USERS;++uu) {
	if (ustr[uu].sock==u && ustr[uu].logging_in) {
        user_quit(uu);
        sprintf(mess,"Line %d cleared\n",uu);
        write_str(user,mess);
	found=1; break;
        }
	else if (ustr[uu].sock==u && !ustr[uu].logging_in) {
        sprintf(mess,"Line %d is in use\n",uu);
        write_str(user,mess);
	found=1; break;
	}
   } /* end of for */
if (!found) {
	write_str(user,"Line not found!");
  }

}

/** blinking broadcast **/
void bbcast(int user, char *inpstr)
{
if (!strlen(inpstr)) {
  write_str(user,"Broadcast what?");
  return;
  }
sprintf(mess,"^BL *** [ %s ] ***^",inpstr);
writeall_str(mess, 0, user, 1, user, BEEPS, BCAST, 0);
print_to_syslog("BLINKING BROADCAST\n");
}

/** version command **/
void version(int user)
{
int i=0;
struct stat fileinfo2;
time_t lastmod;

write_str(user,VERSION);

/* See when the program was last updated */
stat(thisprog, &fileinfo2);
lastmod = fileinfo2.st_ctime;
sprintf(mess,"Updated: %s",ctime(&lastmod));
mess[strlen(mess)-1]=0;
write_str(user,mess);

sprintf(mess,"Hostname & OS: %s (%s)",thishost,thisos);
write_str(user,mess);

if ((TELEP_LEVEL==0) && (FOLLOWIS_JOIN==1))
 write_str(user,"Teleportation is NOT restricted.");
else if ((TELEP_LEVEL==0) && (FOLLOWIS_JOIN==0))
 write_str(user,"Teleportation is loosely restricted.");
else if ((TELEP_LEVEL > 0) && (FOLLOWIS_JOIN==0)) {
 sprintf(mess,"Teleportation is reserved for ranks >= %d",TELEP_LEVEL);
 write_str(user,mess);
 }
else if ((TELEP_LEVEL > 0) && (FOLLOWIS_JOIN==1)) {
 sprintf(mess,"Teleportation is reserved for ranks >= %d (except .follow)",TELEP_LEVEL);
 write_str(user,mess);
 }

if (allow_new > 1)
 write_str(user,"Registration is NOT needed for new players.");
else if (allow_new==1)
 write_str(user,"Email verification IS required for new players.");
else
 write_str(user,"New players are not allowed at this time.");

i=DAYS_OFF;
if ((i < 1) || (i > 1))
 sprintf(mess,"Expiration grace period for users is %d days.",i);
else
 sprintf(mess,"Expiration grace period for users is %d day.",i);
write_str(user,mess);

if ((autoexpire==1) || (autoexpire==2)) {
i=TIME_TO_GO;
if (i > 1)
 sprintf(mess,"Expiration warning period for users is %d days.",i);
else if (i==1)
 sprintf(mess,"Expiration warning period for users is %d day.",i);
else
 strcpy(mess,"Expiration warning period for users is DISABLED");
write_str(user,mess);
}
else {
 write_str(user,"Expiration warning period for users is DISABLED");
}

write_str(user,"An administrative hierarchy exists.");
if (resolve_names==2)
 write_str(user,"Resolver is                 configured for site-wide cache.");
else if (resolve_names==1)
 write_str(user,"Resolver is                 configured for talker-wide cache.");
else
 write_str(user,"Resolver is                 disabled.");
write_str(user,"The robot interface is      enabled.");
if (down_time == 1)
 sprintf(mess,"Auto-Shutdown/Reboot is     enabled.  (%d min. left)",down_time);
else if (down_time > 0)
 sprintf(mess,"Auto-Shutdown/Reboot is     enabled.  (%d mins. left)",down_time);
else
 sprintf(mess,"Auto-Shutdown/Reboot is     disabled.");
write_str(user,mess);
sprintf(mess,"Max users                   : %3d       Max atmospheres p/room : %-3d",MAX_USERS,MAX_ATMOS);
write_str(user,mess);
sprintf(mess,"Max areas                   : %3d       Max length of atmoses  : %-3d",MAX_AREAS,ATMOS_LEN);
write_str(user,mess);
sprintf(mess,"Max name length for players : %3d       Max mail forwards a day: %-3d",NAME_LEN,MAX_AUTOFORS);
write_str(user,mess);
sprintf(mess,"Max description length      : %3d       Max mailfile size      : %-6d",DESC_LEN,MAX_MAILSIZE);
write_str(user,mess);
sprintf(mess,"Max macro length            : %3d       Max sent-mailfile size : %-6d",MACRO_LEN,MAX_SMAILSIZE);
write_str(user,mess);
sprintf(mess,"Max profile lines           : %3d",PRO_LINES);
write_str(user,mess);
sprintf(mess,"Max room description lines  : %3d",ROOM_DESC_LINES);
write_str(user,mess);
sprintf(mess,"Max entermessage length     : %3d",MAX_ENTERM-2);
write_str(user,mess);
sprintf(mess,"Max exitmessage length      : %3d",MAX_EXITM-2);
write_str(user,mess);
sprintf(mess,"Max success and fail lengths: %3d",MAX_ENTERM-2);
write_str(user,mess);
sprintf(mess,"Max topic length            : %3d",TOPIC_LEN);
write_str(user,mess);
}

/** shout and emote together **/
void shemote(int user, char *inpstr)
{
int pos = sh_count%NUM_LINES;
int f; 
char comstr[ARR_SIZE];

if (!ustr[user].shout) 
  {
   write_str(user,NO_SHOUT);
   return;
  }
  
if (!strlen(inpstr)) 
  {
   write_str(user,"Review shouts:"); 
    
    for (f=0;f<NUM_LINES;++f) 
      {
        if ( strlen( sh_conv[pos] ) )
         {
	  write_str(user,sh_conv[pos]);  
	 }
	pos = ++pos % NUM_LINES;
      }

    write_str(user,"<Done>");  
    return; 
  }


if (ustr[user].frog) {
   strcpy(inpstr,FROG_EMOTE);
   emote(user,inpstr);
   return;
   }

if (ustr[user].vis) {
   comstr[0]=inpstr[0];
   comstr[1]=0;
   if (comstr[0] == '\'') {
        inpstr[0] = ' ';
        while(inpstr[0] == ' ') inpstr++;
        sprintf(mess,"& %s\'%s ",ustr[user].say_name,inpstr);
        }
    else
      sprintf(mess,"& %s %s ",ustr[user].say_name,inpstr);
    }
  else {
   comstr[0]=inpstr[0];
   comstr[1]=0;
   if (comstr[0] == '\'') {
        inpstr[0] = ' ';
        while(inpstr[0] == ' ') inpstr++;
        sprintf(mess,"& %s\'%s ",INVIS_ACTION_LABEL,inpstr);
       }
    else
       sprintf(mess,"& %s %s ",INVIS_ACTION_LABEL,inpstr);
    }

/** Store the shemote in the buffer **/
strncpy(sh_conv[sh_count],mess,MAX_LINE_LEN);
sh_count = ( ++sh_count ) % NUM_LINES;

writeall_str(mess, 0, user, 0, user, NORM, SHOUT, 0);
write_str(user,mess);
}


/** superusers changing user_name **/
void suname(int user, char *inpstr)
{
int f,u;
char other_user[ARR_SIZE],newname[ARR_SIZE],lowername[NAME_LEN+1];
char filename[80];
char filename2[80];


if (!strlen(inpstr)) {
   write_str(user,"Whose name do you want to change?");
   return;
   }

sscanf(inpstr,"%s ",other_user);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

strtolower(other_user);
CHECK_NAME(other_user);

if (!read_user(other_user))
  {
   write_str(user,NO_USER_STR);
   return;
  }

 if ((t_ustr.super >= ustr[user].tempsuper) && strcmp(ustr[user].name,ROOT_ID))
  {
    strcpy(t_mess,"You cannot rename a user of same or higher rank.");
    write_str(user,t_mess);
    return;
  }
  
remove_first(inpstr);  
   
sscanf(inpstr,"%s",newname);

 if (newname[0]<32 || strlen(newname)< 3) 
    {
     write_str(user,"Invalid name given [must be at least 3 letters].");  
     return;
    }

 if (strstr(newname,"^")) {
    write_str(user,"Name cannot have color or hilites in it.");
    return;
    }

  if (strlen(newname)>NAME_LEN-1) 
    {
     write_str(user,"Name too long");  
     return;
    }

	/* see if only letters in login */
     for (f=0; f<strlen(newname); ++f) 
       {
         if (!isalpha((int)newname[f]) || newname[f]<'A' || newname[f] >'z') 
           {
	     write_str(user,"Name can only contain letters.");
	     return;
	   }
       }
        

  /*----------------------------------------------------------------*/
  /* copy capitalized name to a temp array and convert to lowercase */
  /*----------------------------------------------------------------*/
  
  strcpy(lowername,newname);
  strtolower(lowername);
 
 if (!strcmp(other_user, lowername))
    {
        write_str(user,"\nNew name cannot be the login name. \nName not changed."); 
        return;  
    }

if (check_for_user(lowername))
    {
      write_str(user,"Sorry, that name is already used.");
      return;
    }
  
  sprintf(mess,"changed %s\'s name to %s",other_user,newname);
  btell(user,mess); 
  strcpy(t_ustr.name,lowername);   
  strcpy(t_ustr.login_name,lowername);   
  strcpy(t_ustr.say_name,newname);   
  write_user(lowername);
  if ((u=get_user_num_exact(other_user,user)) != -1) 
    {
     strcpy(ustr[u].name,lowername);
     strcpy(ustr[u].login_name,lowername);
     sprintf(mess,"^*^ ^HY%s^ has had their name changed to ^HY%s^ ^*^",ustr[u].say_name,newname);
     writeall_str(mess, 0, user, 1, user, NORM, BCAST, 0);
     strcpy(ustr[u].say_name,newname);
    }

/* change exempt file */
/* first arguement is to check against names in file for user */
/* second is the new name we're changing to */
change_exem_data(other_user,newname);

   /* move user's INBOX mail file to new name */
  sprintf(filename,"%s/%s",MAILDIR,other_user);
  sprintf(filename2,"%s/%s",MAILDIR,lowername);
  rename(filename,filename2);

   /* move user's SENT mail file to new name */
  sprintf(filename,"%s/%s.sent",MAILDIR,other_user);
  sprintf(filename2,"%s/%s.sent",MAILDIR,lowername);
  rename(filename,filename2);

   /* move user's EMAIL file to new name */
  sprintf(filename,"%s/%s.email",MAILDIR,other_user);
  sprintf(filename2,"%s/%s.email",MAILDIR,lowername);
  rename(filename,filename2);

   /* move user's PROFILE file to new name */
  sprintf(filename,"%s/%s",PRO_DIR,other_user);
  sprintf(filename2,"%s/%s",PRO_DIR,lowername);
  rename(filename,filename2);

   /* move user's WARNING file to new name */
  sprintf(filename,"%s/%s",WLOGDIR,other_user);
  sprintf(filename2,"%s/%s",WLOGDIR,lowername);
  rename(filename,filename2);

  nuke(user,other_user,0);
  sprintf(mess,"%s Had the name ^%s^, but is renamed now to ^%s^ by ^HR%s^"
              ,lowername,other_user,newname,ustr[user].say_name);
  warning(user,mess);
  newname[0]=0;
  lowername[0]=0;
}

/** superusers changing user's passwords **/
void supass(int user, char *inpstr)
{
int u;
char other_user[ARR_SIZE],newpass[ARR_SIZE];


if (!strlen(inpstr)) {
   write_str(user,"Whose password do you want to change?");
   return;
   }

sscanf(inpstr,"%s ",other_user);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

strtolower(other_user);
CHECK_NAME(other_user);

if (!read_user(other_user))
  {
   write_str(user,NO_USER_STR);
   return;
  }

 if ((t_ustr.super >= ustr[user].tempsuper) && strcmp(ustr[user].name,ROOT_ID))
  {
    strcpy(t_mess,"You cannot repassword a user of same or higher rank.");
    write_str(user,t_mess);
    return;
  }
  
remove_first(inpstr);  
   
sscanf(inpstr,"%s",newpass);

 if (newpass[0]<32 || strlen(newpass)< 3) 
    {
     write_str(user,SYS_PASSWD_INVA);  
     return;
    }

 if (strstr(newpass,"^")) {
    write_str(user,"Password cannot have color or hilites in it.");
    return;
    }
        
  if (strlen(newpass)>NAME_LEN-1) 
    {
     write_str(user,SYS_PASSWD_LONG);  
     return;
    }

  /*-------------------------------------------------------------*/
  /* convert password to lowercase and encrypt the password      */
  /*-------------------------------------------------------------*/
  
  strtolower(newpass);  
 
 if (!strcmp(other_user, newpass))
    {
        write_str(user,"\nPassword cannot be the login name. \nPassword not changed."); 
        return;  
    }
  
  sprintf(mess,"Password for %s changed to %s",t_ustr.say_name,newpass);
  write_str(user,mess); 
  
  st_crypt(newpass);                                   
  strcpy(t_ustr.password,newpass);   
  strcpy(t_ustr.login_pass,newpass);   
  write_user(other_user);
  if ((u=get_user_num_exact(other_user,user)) != -1) 
    {
     strcpy(ustr[u].password,newpass);
     strcpy(ustr[u].login_pass,newpass);
    }
  newpass[0]=0; 
} 

/** entermessage so instead of "walks in" if one chooses **/
void enterm(int user, char *inpstr)
{

if (!strlen(inpstr)) { 
   sprintf(mess,"^HYYour entermessage is:^ %s",ustr[user].entermsg);
   write_str(user,mess);
   return; 
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(ustr[user].entermsg,DEF_ENTER);
    copy_from_user(user);
    write_user(ustr[user].name);
    write_str(user,"Entermsg set to default.");
    return;
    }
if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(ustr[user].entermsg,inpstr);
strcat(ustr[user].entermsg,"@@");
copy_from_user(user);
write_user(ustr[user].name);
sprintf(mess,"^HYNew entermsg:^ %s",ustr[user].entermsg);
write_str(user,mess);

}

/** exitmessage so instead of "goes to the" if one chooses **/
void exitm(int user, char *inpstr)
{

if (!strlen(inpstr)) { 
   sprintf(mess,"^HYYour exitmessage is:^ %s",ustr[user].exitmsg);
   write_str(user,mess);
   return; 
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(ustr[user].exitmsg,DEF_EXIT);
    copy_from_user(user);
    write_user(ustr[user].name);
    write_str(user,"Exitmsg set to default.");
    return;
    }
if (strlen(inpstr) > MAX_EXITM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(ustr[user].exitmsg,inpstr);
strcat(ustr[user].exitmsg,"@@");
copy_from_user(user);
write_user(ustr[user].name);
sprintf(mess,"^HYNew exitmsg:^ %s",ustr[user].exitmsg);
write_str(user,mess);

}

/** list abbreviations **/
void abbrev(int user, char *inpstr)
{
int i=0;
int c=0;
int num=0;
int found=0;
char abbrcom[ARR_SIZE];

if (!strlen(inpstr)) {
 write_str(user,"Command           Yours  Default");
 write_str(user,"-------------     -----  -------");

  for (i=0;i<NUM_ABBRS;++i) {

     for (c=0; sys[c].su_com != -1; ++c) {
      if (!strcmp(sys[c].command,ustr[user].custAbbrs[i].com)) {
     sprintf(mess,"%-12s <-->   %s      %s", ustr[user].custAbbrs[i].com,
	ustr[user].custAbbrs[i].abbr,sys[c].cabbr);
     write_str(user,mess);
     break;
     }
    } /* end of c for */

   } /* end of i for */

  write_str(user,"Ok.");
  return;
 }

/* User wants the defaults back */
if (!strcmp(inpstr,"-d")) {
    write_str(user,"Resetting abbreviations to defaults..");
    initabbrs(user);
    write_str(user,"Ok.");
    return;
    }

sscanf(inpstr,"%s ",abbrcom);
strtolower(abbrcom);
i=0;

/* Make sure command name is one that is available for abbreviation */
 for (i=0;i<NUM_ABBRS;++i) {
  if (!strcmp(abbrcom,ustr[user].custAbbrs[i].com)) {
     found=1;
     num=i;
     break;
     }
  }

 if (!found) {
   write_str(user,"That command is not available for abbreviation.");
   return;
   }

 remove_first(inpstr);

 if (!strlen(inpstr)) {
    sprintf(mess,"Your abbreviation for %s is %s",ustr[user].custAbbrs[num].com,ustr[user].custAbbrs[num].abbr);
    write_str(user,mess);
    return;
    }

 if (strlen(inpstr) > 1) {
   write_str(user,"Abbreviation too long.");
   return;
   }

/* Check for letters, numbers, and a period */
 if (inpstr[0] == '.' || isalnum((int)inpstr[0])) {
   write_str(user,"You can't use that key for a abbreviation.");
   return;
   }

i=0;
found=0;

/* Check if abbreviation is being used by another command already */
 for (i=0;i<NUM_ABBRS;++i) {
       if (!strcmp(ustr[user].custAbbrs[i].abbr,inpstr)) {
        found=1;
        break;
       }
     }

 if (found) {
    write_str(user,"That abbreviation is already being used by another command.");
    return;
    }
       
 if (inpstr[0]=='\\')
     strcpy(ustr[user].custAbbrs[num].abbr,"\\");
 else if (inpstr[0]=='\'')
     strcpy(ustr[user].custAbbrs[num].abbr,"\'");
 else if (inpstr[0]=='\"')
     strcpy(ustr[user].custAbbrs[num].abbr,"\"");
 else if (inpstr[0]=='\?')
     strcpy(ustr[user].custAbbrs[num].abbr,"\?");
 else
     strcpy(ustr[user].custAbbrs[num].abbr,inpstr);
 
/*
 strcpy(ustr[user].custAbbrs[num].abbr,inpstr);
*/

 copy_from_user(user);
 write_user(ustr[user].name);
 write_str(user,"Abbreviation changed.");
}


/** short who with new mud form **/
void newwho(int user)
{
int u,v,idl,invis=0;
int len;
char un[NAME_LEN+2];
char amess[1000];
char bmess[1000];
char cmess[600];
char dmess[600];
char emess[200];
char fmess[200];
time_t tm;

time(&tm);
strcpy(amess,"");
strcpy(bmess,"");
strcpy(cmess,"");
strcpy(dmess,"");
strcpy(emess,"");
strcpy(fmess,"");

for (v=0;v<NUM_AREAS;++v) 
     {
       for (u=0;u<MAX_USERS;++u) {
	if ((ustr[u].area!= -1) && (ustr[u].area == v) && (!ustr[u].logging_in))
	        {
	   	  if (!ustr[u].vis && ustr[user].tempsuper < MIN_HIDE_LEVEL)
	             { 
	              invis++;  
	              continue; 
	              }
			 
		 idl=(tm-ustr[u].last_input)/60;
		
		 strcpy(un,ustr[u].say_name);
                 strcat(un," ");

                 if (idl < 3) {  
                    strcat(amess,un);
                    } 
                 if (idl >= 3 && idl < 60) {
                    strcat(bmess,un);
                    }
                 if (idl >= 60 && idl < 180) {
                    strcat(cmess,un);
                    }
                 if (idl >= 180) {
                    strcat(dmess,un);
                    }
                 }
        }
     }
 if (strlen(amess) >= 3) {
    len=strlen(amess);
    amess[len]=0;
    sprintf(mess,"  Active: %s",amess);
    write_str(user,mess);
    }
 if (strlen(bmess) >= 3) {
    len=strlen(bmess);
    bmess[len]=0;
    sprintf(mess,"   Awake: %s",bmess);
    write_str(user,mess);
    }
 if (strlen(cmess) >= 3) {
    len=strlen(cmess);
    cmess[len]=0;
    sprintf(mess,"    Idle: %s",cmess);
    write_str(user,mess);
    }
 if (strlen(dmess) >= 3) {
    len=strlen(dmess);
    dmess[len]=0;
    sprintf(mess,"Comatose: %s",dmess);
    write_str(user,mess);
    }

sprintf(emess,SHORT_WHO,num_of_users,num_of_users == 1 ? "" : "s",num_of_users == 1 ? "is" : "are");

if (invis) {
   sprintf(fmess,"(%d invis)",invis);
   strcat(emess,fmess);
   } 
write_str(user,emess);

}


/** check last login for specified user **/
void last_u(int user, char *inpstr)
{
int on_now=0;
int u;
char other_user[ARR_SIZE],ldate[20];
time_t tm;
time_t tm_then;

if (!strlen(inpstr)) {
   write_str(user,"Who do you want to see the lastlog of?");
   return;
   } 

sscanf(inpstr,"%s",other_user);

strtolower(other_user);
CHECK_NAME(other_user);

   if ((u = get_user_num(other_user,user)) != -1) {
      if (!strcmp(ustr[u].name,other_user))
       on_now = 1;
      }

if (!on_now) {
if (!read_user(other_user))
   {
    write_str(user,NO_USER_STR);
    return;
   }
midcpy(t_ustr.last_date,ldate,0,15);
write_str(user,"");
sprintf(mess,"^LR-->^    %s was last here on %s",t_ustr.say_name,ldate);
write_str(user,mess);
time(&tm);
tm_then=((time_t) t_ustr.rawtime);
sprintf(mess,"^LR-->^    That was %s ago.",converttime((long)((tm-tm_then)/60)));
write_str(user,mess);
write_str(user,"");
}
else { 
  write_str(user,"");
  sprintf(mess,"^LR-->^    %s is online right now!!    ^LR<--^",ustr[u].say_name);
  write_str(user,mess);
  write_str(user,"");
 }
}

/*** search for specific talker in the talker list, add a talker ***/
/*** modify a talker, or delete a talker                         ***/
void talker(int user, char *inpstr)
{
int occured=0;
int i=0;
int num=0;
int num2=0;
int alpha=0;
int lasthost=0;
char filename[FILE_NAME_LEN],filename2[FILE_NAME_LEN];
char line[ARR_SIZE],line2[ARR_SIZE];
FILE *fp;
FILE *fp2;

if (!ustr[user].t_ent) {
 ustr[user].t_num        = 0;
 ustr[user].t_name[0]    = 0;
 ustr[user].t_host[0]    = 0;
 ustr[user].t_ip[0]      = 0;
 ustr[user].t_port[0]    = 0;

 if (!strlen(inpstr)) 
   {
	sprintf(t_mess,"%s",TALKERLIST);
	strncpy(filename,t_mess,FILE_NAME_LEN);
        if (!check_for_file(filename)) {
           write_str(user,NO_TLIST);
           return;
          }
        write_str(user,"    Name                    Hostname                       Number Address  Port");
        write_str(user,"    ----------------------- ------------------------------ --------------- ----");
        cat(filename,user,1);
   }
 else {
	sprintf(t_mess,"%s",TALKERLIST);
	strncpy(filename,t_mess,FILE_NAME_LEN);

 if (!strcmp(inpstr,"-a")) {
   START:
    ustr[user].t_ent=1;
    write_str_nr(user,"Enter the name of the talker: ");
	 telnet_write_eor(user);
    noprompt=1; return;
   } /* end of -a if */
 else if (!strncmp(inpstr,"-d",2)) {
   if (ustr[user].tempsuper < WIZ_LEVEL) {
     write_str(user,NOT_WORTHY);
     return;
     }
    remove_first(inpstr);
   if (!strlen(inpstr) || (strlen(inpstr)>3)) {
     write_str(user,"You must enter the number of the talker you wish to delete.");
     return;
    }
   num=atoi(inpstr);
   num2=file_count_lines(filename);
   if ((num==0) || (num>num2)) {
     write_str(user,"That is not a valid number.");
     return;
    }
   if (!(fp=fopen(filename,"r"))) { 
     write_str(user,"Can't open talker list! May not exist");
     return;
    }
   strcpy(filename2,get_temp_file());
   if (!(fp2=fopen(filename2,"a"))) { 
     write_str(user,"Can't open temp file for writing!");
     logerror("Can't open temp file for writing in talker()");
     return;
    }
   strcpy(line,"");
   line[0]=0;
   while (fgets(line,80,fp)!=NULL) {
    line[strlen(line)-1]=0;
    i++;
    if (i==num) { strcpy(line,""); line[0]=0; continue; }
    else {
     fputs(line,fp2); fputs("\n",fp2);
     strcpy(line,""); line[0]=0;
     }
   }
   i=0;
   write_str(user,"Talker entry deleted.");
   FCLOSE(fp);
   FCLOSE(fp2);
   remove(filename);
   rename(filename2,filename);
   ustr[user].t_num        = 0;
   ustr[user].t_name[0]    = 0;
   ustr[user].t_host[0]    = 0;
   ustr[user].t_ip[0]      = 0;
   ustr[user].t_port[0]    = 0;
   return;
  } /* end of -d if */
 else if (isdigit((int)inpstr[0]) && (strlen(inpstr)<4)) {
   if (ustr[user].tempsuper < WIZ_LEVEL) {
     write_str(user,NOT_WORTHY);
     return;
     }
   num=atoi(inpstr);
   num2=file_count_lines(filename);
   if ((num==0) || (num>num2)) {
     write_str(user,"That is not a valid number.");
     return;
    }
   write_str(user,"Modifying talker info..");
   ustr[user].t_num=num;
   goto START;
   } /* end of modify if */
 else {
	strtolower(inpstr);
        /* look through list */
	if (!(fp=fopen(filename,"r"))) { 
           write_str(user,"Can't open talker list! May not exist");
           return;
           }
        strcpy(line,""); line[0]=0;
	while(fgets(line,80,fp)!=NULL) {
                num++;
	        line[strlen(line)-1]=0;
                strcpy(line2,line);
                strtolower(line2);
		if (instr2(0,line2,inpstr,0)== -1) goto NEXT;
                if (!occured) {
        write_str(user,"    Name                    Hostname                       Number Address  Port");
        write_str(user,"    ----------------------- ------------------------------ --------------- ----");
                 }
                 if (ustr[user].tempsuper >= LINENUM_LEVEL) {
		  sprintf(mess,"%-3d ",num);
		  write_str_nr(user,mess);
                  }
		  write_str(user,line);	
		  ++occured;
		NEXT:
                strcpy(line,""); line[0]=0;
		continue;
		}
	FCLOSE(fp);

   if (!occured) write_str(user,"No occurences found");
   else {
        write_str(user," ");
	sprintf(mess,"%d occurence%s found",occured,occured == 1 ? "" : "s");
	write_str(user,mess);
	}
   num=0;
   } /* end of search else */
  } /* end of strlen inpstr else */
 } /* end of if t_ent */
else {
    if ((strlen(inpstr) < 5) && ustr[user].t_ent>0 && ustr[user].t_ent<4) {
      if (ustr[user].t_ent==1) {
       write_str(user,"Name too short.");
       write_str(user,"Enter the name of the talker: ");
       noprompt=1; return;
       }
      else if (ustr[user].t_ent==2) {
       write_str(user,"Hostname too short.");
       write_str(user,"Use \"unknown\" if you dont know the name address");
       write_str(user,"Enter the hostname address (w/o port): ");
       noprompt=1; return;
       }
      else if (ustr[user].t_ent==3) {
        write_str(user,"Address too short.");
        write_str(user,"Use \"unknown\" if you dont know the numeric address");
        write_str(user,"Enter the talker numeric address (w/o port): ");
        noprompt=1; return;
       }
     } /* end of if strlen inpstr */
   if (ustr[user].t_ent==1) {
      inpstr[23]=0;
      strcpy(ustr[user].t_name,inpstr);
      if (isalpha((int)ustr[user].t_name[0])) ustr[user].t_name[0]=toupper((int)ustr[user].t_name[0]);
      write_str(user,"Enter hostname address (w/o port): ");
      ustr[user].t_ent=2;
      noprompt=1; return;
     }
   else if (ustr[user].t_ent==2) {
      inpstr[30]=0;
      strcpy(ustr[user].t_host,inpstr);
      write_str(user,"Enter the talker numeric address (w/o port): ");
      ustr[user].t_ent=3;
      noprompt=1; return;
     }
   else if (ustr[user].t_ent==3) {
      inpstr[15]=0;
      strcpy(ustr[user].t_ip,inpstr);
      write_str(user,"Enter the talker port number: ");
      ustr[user].t_ent=4;
      noprompt=1; return;
     }
   else if (ustr[user].t_ent==4) {
      inpstr[5]=0;
      strcpy(ustr[user].t_port,inpstr);
      sprintf(t_mess,"%s",TALKERLIST);
      strncpy(filename,t_mess,FILE_NAME_LEN);
      if (!(fp=fopen(filename,"r"))) {
        fp=fopen(filename,"a");
        sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
        ustr[user].t_ent=0;
        write_str(user,"Talker added.");
        ustr[user].t_num        = 0;
        ustr[user].t_name[0]    = 0;
        ustr[user].t_host[0]    = 0;
        ustr[user].t_ip[0]      = 0;
        ustr[user].t_port[0]    = 0;
        fputs(mess,fp);
        FCLOSE(fp);
        return;
       }
      else {
       if (isalpha((int)ustr[user].t_name[0]) ||
           isdigit((int)ustr[user].t_name[0])) alpha=1;
       else alpha=2;

       strcpy(filename2,get_temp_file());
   if (!(fp2=fopen(filename2,"a"))) { 
     write_str(user,"Can't open temp file for writing!");
     logerror("Can't open temp file for writing in talker()");
        alpha=0;
        ustr[user].t_ent        = 0;
        ustr[user].t_num        = 0;
        ustr[user].t_name[0]    = 0;
        ustr[user].t_host[0]    = 0;
        ustr[user].t_ip[0]      = 0;
        ustr[user].t_port[0]    = 0;
     FCLOSE(fp);
     return;
    }
   if (ustr[user].t_num) {
    strcpy(line,""); line[0]=0;
    while (fgets(line,80,fp)!=NULL) {
     line[strlen(line)-1]=0;
     i++;
     if (i==ustr[user].t_num) {
       if (alpha!=3) lasthost=1;
       continue;
      }
     else {
      if (alpha==3) {
         fputs(line,fp2); fputs("\n",fp2);
         lasthost=0;
         }
      else if ((alpha==2) || (alpha==1)) {
        if (line[0]<ustr[user].t_name[0]) {
         fputs(line,fp2); fputs("\n",fp2);
         lasthost=1;
         }
        else if (line[0]==ustr[user].t_name[0]) {
         if (line[1]<ustr[user].t_name[1]) {
          fputs(line,fp2); fputs("\n",fp2);
          lasthost=1;
          }
         else if (line[1]==ustr[user].t_name[1]) {
          fputs(line,fp2); fputs("\n",fp2);
          sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
          fputs(mess,fp2);
          alpha=3;
          lasthost=0;
          }
         else if (line[1]>ustr[user].t_name[1]) {
          sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
          fputs(mess,fp2);
          fputs(line,fp2); fputs("\n",fp2);
          alpha=3;
          lasthost=0;
          }
         } /* end of else if line equals */
        else if (line[0]>ustr[user].t_name[0]) {
         sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
         fputs(mess,fp2);
         fputs(line,fp2); fputs("\n",fp2);
         lasthost=0;
         alpha=3;
         }
       } /* end of else if alpha */
      } /* end of else */
     strcpy(line,""); line[0]=0;
     } /* end of while */
     if (lasthost) {
          sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
          fputs(mess,fp2);
          lasthost=0;
       }
     i=0;
     alpha=0;
     ustr[user].t_ent=0;
     ustr[user].t_num        = 0;
     ustr[user].t_name[0]    = 0;
     ustr[user].t_host[0]    = 0;
     ustr[user].t_ip[0]      = 0;
     ustr[user].t_port[0]    = 0;
     write_str(user,"Talker entry modified.");
     FCLOSE(fp);
     FCLOSE(fp2);
     remove(filename);
     rename(filename2,filename);
     return;
    } /* end of if t_num */
    strcpy(line,""); line[0]=0;
   while (fgets(line,80,fp)!=NULL) {
    line[strlen(line)-1]=0;
      if (alpha==3) {
         fputs(line,fp2); fputs("\n",fp2);
         lasthost=0;
         }
      else if ((alpha==2) || (alpha==1)) {
        if (line[0]<ustr[user].t_name[0]) {
         fputs(line,fp2); fputs("\n",fp2);
         lasthost=1;
         }
        else if (line[0]==ustr[user].t_name[0]) {
         if (line[1]<ustr[user].t_name[1]) {
          fputs(line,fp2); fputs("\n",fp2);
          lasthost=1;
          }
         else if (line[1]==ustr[user].t_name[1]) {
          fputs(line,fp2); fputs("\n",fp2);
          sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
          fputs(mess,fp2);
          alpha=3;
          lasthost=0;
          }
         else if (line[1]>ustr[user].t_name[1]) {
          sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
          fputs(mess,fp2);
          fputs(line,fp2); fputs("\n",fp2);
          alpha=3;
          lasthost=0;
          }
         } /* end of else if line equals */
        else if (line[0]>ustr[user].t_name[0]) {
         sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
         fputs(mess,fp2);
         fputs(line,fp2); fputs("\n",fp2);
         lasthost=0;
         alpha=3;
         }
       } /* end of else if alpha */
     strcpy(line,""); line[0]=0;
    } /* end of while */
    if (lasthost) {
          sprintf(mess,"%-23s %-30s %-15s %s\n",ustr[user].t_name,
                ustr[user].t_host,ustr[user].t_ip,ustr[user].t_port);      
          fputs(mess,fp2);
          lasthost=0;
      }
    ustr[user].t_ent=0;
    ustr[user].t_num        = 0;
    ustr[user].t_name[0]    = 0;
    ustr[user].t_host[0]    = 0;
    ustr[user].t_ip[0]      = 0;
    ustr[user].t_port[0]    = 0;
    alpha=0;
    write_str(user,"Talker added.");
    FCLOSE(fp);
    FCLOSE(fp2);
    remove(filename);
    rename(filename2,filename);
  } /* end of filename else */
 } /* end of else if */
 } /* end of t_ent else */
strcpy(line,""); line[0]=0; i=0;
}

/** bbbbbbbuuubbles for eve **/
void bubble(int user)
{
sprintf(mess,"OoOoo Ooo oOOo oOo oOooOOo OooOOooo oOoOooO ooo Ooo");
write_str(user,mess);
writeall_str(mess, 1, user, 0, user, NORM, ECHOM, 0);
}

/** same as .think but remotely **/
void sthink(int user, char *inpstr)
{
int point=0,count=0,i=0,lastspace=0,lastcomma=0,gotchar=0;
int point2=0,multi=0;
int multilistnums[MAX_MULTIS];
char multilist[MAX_MULTIS][ARR_SIZE];
char multiliststr[ARR_SIZE];
char other_user[ARR_SIZE];
int u=-1;
char prefix[25];

for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; multilistnums[i]=-1; }
multiliststr[0]=0;
i=0;

if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

for (i=0;i<strlen(inpstr);++i) {
        if (inpstr[i]==' ') {
                if (lastspace && !gotchar) { point++; point2++; continue; }
                if (!gotchar) { point++; point2++; }
                lastspace=1;
                continue;
          } /* end of if space */
        else if (inpstr[i]==',') {
                if (!gotchar) {
                        lastcomma=1;
			point++;
			point2++;
			continue;
                }
                else {
                if (count <= MAX_MULTIS-1) {
                midcpy(inpstr,multilist[count],point,point2-1);
                count++;
                }
                point=i+1;
                point2=point;
                gotchar=0;
                lastcomma=1;
                continue;
                }
                
        } /* end of if comma */
        if ((inpstr[i-1]==' ') && (gotchar)) {
                if (count <= MAX_MULTIS-1) {
                midcpy(inpstr,multilist[count],point,point2-1);
                count++;  
                }
                break;
        }
        gotchar=1;
        lastcomma=0;
        lastspace=0;
        point2++;
} /* end of for */
midcpy(inpstr,multiliststr,i,ARR_SIZE);
                
if (!strlen(multiliststr)) {
        /* no message string, copy last user */
        midcpy(inpstr,multilist[count],point,point2);
        count++;
        strcpy(inpstr,"");
        }
else {
        strcpy(inpstr,multiliststr);
        multiliststr[0]=0;
     }

i=0;
point=0;
point2=0;
gotchar=0;
        
if (!strlen(inpstr)) 
  {
   write_str(user,"Who are you thinking about?");  
	for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; } 
	multiliststr[0]=0;
   return;
  }

tells++; 

if (count>1) multi=1;

/* go into loop and check users */
for (i=0;i<count;++i) {

strcpy(other_user,multilist[i]);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   if (!multi) {
   write_str(user,"Illegal name.");
   return;
   }
   else continue;
  }

strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   if (!read_user(other_user)) {
      write_str(user,NO_USER_STR);
      if (!multi) return;
      else continue;
      }
   not_signed_on(user,t_ustr.say_name);
if (user_wants_message(user,FAILS)) {
   sprintf(mess,"%s",t_ustr.fail);
   write_str(user,mess);
   }
      if (!multi) return;
      else continue;
  }

if (!gag_check(user,u,0)) {
      if (!multi) return;
      else continue;
   }

if (ustr[u].pro_enter || ustr[u].vote_enter || ustr[u].roomd_enter) {
    write_str(user,IS_ENTERING);
      if (!multi) return;
      else continue;
    }

if (ustr[u].afk)
  {
    if (ustr[u].afk == 1) {
      if (!strlen(ustr[u].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[u].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }
     else {
      if (!strlen(ustr[u].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[u].say_name);
      else
      sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }

    write_str(user,t_mess);
  }

if (ustr[u].igtell && ustr[user].tempsuper<WIZ_LEVEL) 
  {
   sprintf(mess,"%s is ignoring tells, semotes, and sthinks",ustr[u].say_name);
   write_str(user,mess);
   if (user_wants_message(user,FAILS)) write_str(user,ustr[u].fail);
      if (!multi) return;
      else continue;
  }
  
/* check if this user is already in the list */
/* we're gonna reuse some ints here          */
for (point2=0;point2<MAX_MULTIS;++point2) {
        if (multilistnums[point2]==u) { gotchar=1; break; }
   }
point2=0;
if (gotchar) {
  gotchar=0;
  continue;
  }   
    
/* it's ok to send the tell to this user, add them to the multistr */
/* add this user to the list for our next loop */
multilistnums[point]=u;
point++;
} /* end of user for */
i=0;

/* no multilistnums, must be all bad users */
if (!point) {
        return;
  } 

/* loop to compose the messages and print to the users */
for (i=0;i<point;++i) {
u=multilistnums[i];

count=0;
point2=0;
multiliststr[0]=0;
/* make multi string to send to this user */
for (point2=0;point2<point;++point2) {
/* dont send recipients name to themselves */
if (u==multilistnums[point2]) continue;
else count++;
if (count>0)
 strcat(multiliststr,",");
/* add their name to the output string */
if (!ustr[multilistnums[point2]].vis)
 strcat(multiliststr,INVIS_ACTION_LABEL);
else
 strcat(multiliststr,ustr[multilistnums[point2]].say_name);
}

if ((ustr[u].monitor==1) || (ustr[u].monitor==3))
  {
    strcpy(prefix,"<");
    strcat(prefix,ustr[user].say_name);
    strcat(prefix,"> ");
  }
 else
  {
   prefix[0]=0;
  }

if (ustr[user].frog) strcpy(inpstr,"I'm a frog, I'm a frog!");

/* write to user being told */
if (ustr[user].vis) {
  if (!multi)
   sprintf(mess,"--> %s thinks . o O ( %s )", ustr[user].say_name, inpstr);
  else
   sprintf(mess,"--> %s thinks (To you%s) . o O ( %s )", ustr[user].say_name,multiliststr,inpstr);
  }
else {
  if (!multi)
   sprintf(mess,"%s--> %s thinks . o O ( %s )",prefix,INVIS_ACTION_LABEL,inpstr); 
  else
   sprintf(mess,"%s--> %s thinks (To you%s) . o O ( %s )",prefix,INVIS_ACTION_LABEL,multiliststr,inpstr); 
  }

if (ustr[u].beeps) {
 if (user_wants_message(u,BEEPS))
  strcat(mess,"\07");
 }

/*-----------------------------------*/
/* store the sthink in the rev buffer*/
/*-----------------------------------*/
/* moved because of multi tells */
strncpy(ustr[u].conv[ustr[u].conv_count],mess,MAX_LINE_LEN);
ustr[u].conv_count = ( ++ustr[u].conv_count ) % NUM_LINES;

if (ustr[u].hilite==2)
 write_str(u,mess);
else {
 strcpy(mess, strip_color(mess));
 write_hilite(u,mess);
 }

} /* end of message compisition for loop */

if (multi) {
point2=0;
multiliststr[0]=0;
/* make multi string to send to this user */
for (point2=0;point2<point;++point2) {
/* dont send recipients name to themselves */
if (point2>0)
 strcat(multiliststr,",");
/* add their name to the output string */
if (!ustr[multilistnums[point2]].vis)
 strcat(multiliststr,INVIS_ACTION_LABEL);
else
 strcat(multiliststr,ustr[multilistnums[point2]].say_name);
}
} /* end of if multi */

/* write to teller */
if (!multi)
 sprintf(mess,"You thought to %s: %s",ustr[u].say_name,inpstr);
else
 sprintf(mess,"You thought to %s: %s",multiliststr,inpstr);

write_str(user,mess);
if (!multi) {
if (user_wants_message(user,SUCCS) && strlen(ustr[u].succ))
 write_str(user,ustr[u].succ);
}

strncpy(ustr[user].conv[ustr[user].conv_count],mess,MAX_LINE_LEN);
ustr[user].conv_count = ( ++ustr[user].conv_count ) % NUM_LINES;

i=0;
for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; }
multiliststr[0]=0;   

}


/** locate a user, online or not **/
void where(int user, char *inpstr)
{
char other_user[ARR_SIZE];
int u;

if  (!strlen(inpstr)) {
      for (u=0; u < MAX_USERS; ++u) {
            if (ustr[u].area!= -1) {
           if (!ustr[u].vis && ustr[user].tempsuper < MIN_HIDE_LEVEL) continue;
           if (astr[ustr[u].area].hidden) {
                if ((ustr[user].tempsuper >= ROOMVIS_LEVEL) && SHOW_HIDDEN)
                 sprintf(mess,"%s is in <%s>",ustr[u].say_name,astr[ustr[u].area].name);
                else
                 sprintf(mess,"%s is in ??",ustr[u].say_name);
               }
           else
              sprintf(mess,"%s is in %s",ustr[u].say_name,astr[ustr[u].area].name);
           write_str(user,mess);
         }
       }
    return;
    }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);
CHECK_NAME(other_user);

if (!check_for_user(other_user)) 
  {
   write_str(user,NO_USER_STR);
   return;
  }

 if ((u=get_user_num(other_user,user))== -1)
    {
     read_user(other_user);
     if (astr[t_ustr.area].hidden)
    sprintf(mess,"%s was last seen in ??",t_ustr.say_name);
     else
    sprintf(mess,"%s was last seen in %s",t_ustr.say_name,astr[t_ustr.area].name);
    write_str(user,mess);
    }
   else {  
     if (astr[ustr[u].area].hidden) {
       if ((ustr[user].tempsuper >= ROOMVIS_LEVEL) &&  SHOW_HIDDEN)
        sprintf(mess,"%s is in <%s>",ustr[u].say_name,astr[ustr[u].area].name);
      else
        sprintf(mess,"%s is in ??",ustr[u].say_name);
     }
     else
    sprintf(mess,"%s is in %s",ustr[u].say_name,astr[ustr[u].area].name);
    write_str(user,mess);
    }
}


/** phone a user for a tell link **/
void call(int user, char *inpstr)
{
int u;
char other_user[ARR_SIZE];

if (!strlen(inpstr)) {
   write_str(user,"Who do you want to call?");
   return;
   }

if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"Clear")) {
    ustr[user].phone_user[0]=0;
    write_str(user,"Your tell link has been disconnected.");
    return;
    }
if (strlen(inpstr) > NAME_LEN) {
   write_str(user,"Name too long.");
   return;
   }

sscanf(inpstr,"%s ",other_user);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }


strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   if (!read_user(other_user)) {
      write_str(user,NO_USER_STR);
      return;
      }
   not_signed_on(user,t_ustr.say_name);
if (user_wants_message(user,FAILS)) {
   sprintf(mess,"%s",t_ustr.fail);
   write_str(user,mess);
   }
   return;
  }

if (!gag_check(user,u,0)) return;

if (ustr[u].afk)
  {
    if (ustr[u].afk == 1) {
      if (!strlen(ustr[u].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[u].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }
     else {
      if (!strlen(ustr[u].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[u].say_name);
      else
      sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }

    write_str(user,t_mess);
    write_str(user,"Tell link was not established.");
    return;
  }

if (ustr[u].igtell) 
  {
   sprintf(mess,"%s is ignoring tells",ustr[u].say_name);
   write_str(user,mess);
   write_str(user,"Tell link not established.");
   return;
  }
  

strcpy(ustr[user].phone_user,ustr[u].say_name);
sprintf(mess,"Tell link to ^HY%s^ established.\nUse .cr <mess> to reply to that user.",ustr[u].say_name);

write_str(user,mess);
write_str(user,"^HY*WARNING*^ If you are invis, the user you are telling will see your name");
sprintf(mess,"          instead of *%s*",INVIS_ACTION_LABEL);
write_str(user,mess);

}


/** reply to your phone partner **/
void creply(int user, char *inpstr)
{
int u;
char lowername[NAME_LEN+1];
char comstr[ARR_SIZE];

if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

if (!strlen(inpstr)) {
   write_str(user,"What do you want to say?");
   return;
   }

if (strlen(ustr[user].phone_user) < 3) {
   write_str(user,"You don't have a tell link to anyone. Use  .call <user>  first."); 
   return;
   }

/* Copy phoned users name to temp arrays so we can display their name */
/* capitalized right and do checks at the same time                   */
strcpy(lowername,ustr[user].phone_user);
strtolower(lowername);

if ((u=get_user_num(lowername,user))== -1)
  {
   not_signed_on(user,ustr[user].phone_user);
   return;
  }

if (!gag_check(user,u,0)) return;

if (ustr[u].pro_enter || ustr[u].vote_enter || ustr[u].roomd_enter) {
    write_str(user,IS_ENTERING);
    return;
    }

if (ustr[u].afk)
  {
    if (ustr[u].afk == 1) {
      if (!strlen(ustr[u].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[u].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }
     else {
      if (!strlen(ustr[u].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[u].say_name);
      else
      sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }

    write_str(user,t_mess);
  }

if (ustr[u].igtell) 
  {
   sprintf(mess,"%s is ignoring tells",ustr[u].say_name);
   write_str(user,mess);
   return;
  }
  
tells++;

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

/* write to user being told */
comstr[0]=inpstr[0];
comstr[1]=0;
if (comstr[0] == ustr[user].custAbbrs[get_emote(user)].abbr[0]) 
  {
  inpstr[0] = ' ';
  while(inpstr[0] == ' ') inpstr++;
  sprintf(mess,"--> %s %s",ustr[user].say_name,inpstr);
  } 
else if (comstr[0] == '\'') {
  inpstr[0] = ' ';
  while(inpstr[0] == ' ') inpstr++;
  sprintf(mess,"--> %s\'%s",ustr[user].say_name,inpstr);
  }
 else {   
   sprintf(mess,VIS_TELLS,ustr[user].say_name,inpstr);
   }

strncpy(ustr[u].conv[ustr[u].conv_count],mess,MAX_LINE_LEN);
ustr[u].conv_count = ( ++ustr[u].conv_count ) % NUM_LINES;

if (ustr[u].beeps) {
 if (user_wants_message(u,BEEPS))
  strcat(mess,"\07");
 }

if (ustr[u].hilite==2)
 write_str(u,mess);
else {
 strcpy(mess, strip_color(mess));
 write_hilite(u,mess);
 }

/* write to teller */
if (comstr[0] == '\'')
sprintf(mess,"You posess-posed to %s: %s\'%s",ustr[u].say_name,ustr[user].say_name,inpstr);
else if (comstr[0] == ustr[user].custAbbrs[get_emote(user)].abbr[0])
sprintf(mess,"You posed to %s: %s %s",ustr[u].say_name,ustr[user].say_name,inpstr);
else
sprintf(mess,VIS_FROMLINK,ustr[u].say_name,inpstr);

write_str(user,mess);

/** store reply in the tell buffer **/
strncpy(ustr[user].conv[ustr[user].conv_count],mess,MAX_LINE_LEN);
ustr[user].conv_count = ( ++ustr[user].conv_count ) % NUM_LINES;
return;
}

/** set your fail message for when you are not online **/
void failm(int user, char *inpstr)
{

if (!strlen(inpstr)) { 
   sprintf(mess,"^HYYour fail is:^ %s",ustr[user].fail);
   write_str(user,mess);
   return; 
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") || 
    !strcmp(inpstr,"-c")) {
    ustr[user].fail[0]=0;
    copy_from_user(user);
    write_user(ustr[user].name);
    write_str(user,"Fail message cleared.");
    return; 
    } 

if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

strcpy(ustr[user].fail,inpstr);
strcat(ustr[user].fail,"@@");
copy_from_user(user);
write_user(ustr[user].name);
sprintf(mess,"^HYNew fail:^ %s",ustr[user].fail);
write_str(user,mess);

}

/** set succ message for tells, remotes, beeps **/
void succm(int user, char *inpstr)
{

if (!strlen(inpstr)) { 
   sprintf(mess,"^HYYour success is:^ %s",ustr[user].succ);
   write_str(user,mess);
   return; 
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") || 
    !strcmp(inpstr,"-c")) {
    ustr[user].succ[0]=0;
    copy_from_user(user);
    write_user(ustr[user].name);
    write_str(user,"Success message cleared.");
    return; 
    } 
if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

strcpy(ustr[user].succ,inpstr);
strcat(ustr[user].succ,"@@");
copy_from_user(user);
write_user(ustr[user].name);
sprintf(mess,"^HYNew success:^ %s",ustr[user].succ);
write_str(user,mess);

}


/** say a message to everyone in room but specified user **/
void mutter(int user, char *inpstr)
{
int u;
char other_user[ARR_SIZE];
char comstr[ARR_SIZE];

if (!strlen(inpstr)) {
   write_str(user,"You must specify a user and a message.");
   return;
   }

sscanf(inpstr,"%s ",other_user);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

strtolower(other_user);

if (!strcmp(ustr[user].name,other_user)) {
   write_str(user,"Mutter to everyone but yourself? Get real.");
   return;
   }

if ((u=get_user_num(other_user,user))== -1) 
  {
   if (check_for_user(other_user) == 1) {
      not_signed_on(user,other_user);
      return;
     }
   else {
      write_str(user,NO_USER_STR);
      return;
     }
  }

if (strcmp(ustr[u].name,other_user)) {
   write_str(user,NO_USER_STR);
   return;
   }

if (strcmp(astr[ustr[user].area].name,astr[ustr[u].area].name)) {
   write_str(user,"User is not in this room");
   return;
   }

strcpy(ustr[user].mutter,ustr[u].say_name);
remove_first(inpstr);

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

	comstr[0]=inpstr[0];
	comstr[1]=0;

if (comstr[0] == ustr[user].custAbbrs[get_emote(user)].abbr[0])
  {
	comstr[0]=inpstr[1];
	comstr[1]=0;

     if (comstr[0] == '\'') {
        inpstr[0] = ' ';
        while(inpstr[0] == ' ') inpstr++;
        sprintf(mess,VIS_MUTEMOTE,ustr[user].say_name,inpstr,ustr[user].mutter);
	}
    else {
     inpstr[0] = ' ';
     sprintf(mess,VIS_MUTEMOTE,ustr[user].say_name,inpstr,ustr[user].mutter);
     }

    write_str(user,mess);
   if (!ustr[user].vis) {
     if (comstr[0] == '\'') {
        inpstr[0] = ' ';
        while(inpstr[0] == ' ') inpstr++;
        sprintf(mess,INVIS_MUTEMOTE,INVIS_ACTION_LABEL,inpstr,ustr[user].mutter);
	}
      else
        inpstr[0] = ' ';
        sprintf(mess,INVIS_MUTEMOTE,INVIS_ACTION_LABEL,inpstr,ustr[user].mutter);
    } /* end of !vis */
  } /* end of if emote */
else {
    sprintf(mess,VIS_MUTTERS,ustr[user].say_name,inpstr,ustr[user].mutter);
    write_str(user,mess);
   if (!ustr[user].vis)
    sprintf(mess,INVIS_MUTTERS,INVIS_TALK_LABEL,inpstr,ustr[user].mutter);   
  }
  
  writeall_str(mess,1,user,0,user,NORM,SAY_TYPE,0);
ustr[user].mutter[0]=0;
}


/*------------------------------------------------*/
/* set autoread                                   */
/*------------------------------------------------*/
void set_autoread(int user)
{

  if (ustr[user].autor==3)
    {
      write_str(user, "Autoread now ^HYoff^.");
      ustr[user].autor = 0;
    }
   else if (ustr[user].autor==2)
    {
      write_str(user, "Autoread now on ^HYfor logins and online^.");
      ustr[user].autor = 3;
    }
   else if (ustr[user].autor==1)
    {
      write_str(user, "Autoread now on ^HYfor online only^.");
      ustr[user].autor = 2;
    }
   else if (ustr[user].autor==0)
    {
      write_str(user, "Autoread now on ^HYfor logins only^.");
      ustr[user].autor = 1;
    }
  write_str(user,"* .set autoread again for more options *");  

  copy_from_user(user);
  write_user(ustr[user].name);
}

/** auto forward mail to email_addr **/
void set_autofwd(int user)
{

  if (ustr[user].autof==2)
    {
      write_str(user, "Autofwd now ^HYoff^.");
      ustr[user].autof = 0;
    }
   else if (ustr[user].autof==0)
    {
      write_str(user, "Autofwd now on ^HYall the time^.");
      ustr[user].autof = 1;
    }
   else if (ustr[user].autof==1)
    {
      write_str(user, "Autofwd now on ^HYonly when you're not online^.");
      ustr[user].autof = 2;
    }
  write_str(user,"* .set autofwd again for more options *");  

  copy_from_user(user);
  write_user(ustr[user].name);
}


/** forward mail to an email address **/
void fmail(int user, char *inpstr)
{
int nosubject=0;
int sendmail=0;
char line[301];
char mail_addr[ARR_SIZE];
char temp[EMAIL_LENGTH+1];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
FILE *fp;
FILE *pp;

if (!strlen(inpstr)) {
    write_str(user,"Where do you want to forward your mail?");
    write_str(user,"Specify a valid email address or use *mine* to use your set address.");
    return;
    }

/* Check for illegal characters in email addy */
if (strpbrk(inpstr,";/[]\\") ) {
   write_str(user,"Illegal email address");
   return;
   }

if (strstr(inpstr,"^")) {
   write_str(user,"Email can't have color or hilite codes in it");
   return;
   }

 inpstr[EMAIL_LENGTH-1]=0;
 strcpy(temp,inpstr);
 strtolower(temp);

 if (strstr(temp,"whitehouse.gov"))
      {
       write_str(user,"Email address not valid.");
       return;
      }

sprintf(filename2,"%s/%s",MAILDIR,ustr[user].name);

if (!check_for_file(filename2)) {
    write_str(user,"You don't have a mailfile to forward!");
    return;
    }

if (!strcmp(inpstr,"mine") || !strcmp(inpstr,"Mine")) {
    if (!strcmp(ustr[user].email_addr,DEF_EMAIL)) {
       write_str(user,"Email address not valid.");
       return;
      }
    else
     strcpy(mail_addr,ustr[user].email_addr);
  }
else if (!strstr(inpstr,".") || !strstr(inpstr,"@")) {
       write_str(user,"Email address not valid.");
       return;
      }
else strcpy(mail_addr,inpstr);

if (!(fp=fopen(filename2,"r"))) {
  sprintf(mess,"%s: Could not open mailfile in fmail for user %s!\n",get_time(0,0),ustr[user].say_name);
  print_to_syslog(mess);
  return;
  }


/*---------------------------------------------------*/
/* write email message                               */
/*---------------------------------------------------*/

if (strstr(MAILPROG,"sendmail")) {
  sprintf(t_mess,"%s",MAILPROG);
  sendmail=1;
  }
else {
  sprintf(t_mess,"%s %s",MAILPROG,mail_addr);
  if (strstr(MAILPROG,"-s"))
	nosubject=0;
  else
	nosubject=1;
  }  
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(pp=popen(filename,"w"))) 
  {
   sprintf(mess,"%s : fmail message cannot be written\n", syserror);
   fclose(fp);
   write_str(user,mess);
   return;
  }

if (sendmail) {
fprintf(pp,"From: %s <%s>\n",SYSTEM_NAME,SYSTEM_EMAIL);
fprintf(pp,"To: %s <%s>\n",ustr[user].say_name,mail_addr);
fprintf(pp,"Subject: Your mailfile from %s\n\n",SYSTEM_NAME);
}
else if (nosubject) {
fprintf(pp,"Your mailfile from %s\n",SYSTEM_NAME);
}

fgets(line,300,fp);

while (!feof(fp)) {
   fputs(line,pp);
   fgets(line,300,fp);
  } /* end of while */
fclose(fp);

fputs("\n",pp);
fputs(EXT_MAIL1,pp);
fputs(".\n",pp);
pclose(pp);

write_str(user,"Mail forwarded.");
write_str(user,"**Note: If email address is not valid, mail will bounce.");

}

/*** wipe suggestionboard (erase file) ***/
void swipe(int user, char *inpstr)
{
char filename[FILE_NAME_LEN];
FILE *bfp;
int lower=-1;
int upper=-1;
int mode=0;

   write_str(user,"***Suggestion Wipe***");
   sprintf(t_mess,"%s/suggs",MESSDIR);
  
strncpy(filename,t_mess,FILE_NAME_LEN);

/*---------------------------------------------*/
/* check if there is any mail                  */
/*---------------------------------------------*/

if (!(bfp=fopen(filename,"r"))) 
  {
   write_str(user,"There are no messages to Swipe off the board."); 
   return;
  }
FCLOSE(bfp);

/*---------------------------------------------*/
/* get the delete parameters                   */
/*---------------------------------------------*/

get_bounds_to_delete(inpstr, &lower, &upper, &mode);
 
if (upper == -1 && lower == -1)
  {
   write_str(user,"No messages wiped.  Specification of what to ");
   write_str(user,"wipe did not make sense.  Type: .help wipe ");
   write_str(user,"for detailed instructions on use. ");
   return;
  }
    
   switch(mode)
    {
     case 0: return;
             break;
        
     case 1: 
            sprintf(mess,"SWiped all messages.");
            upper = -1;
            lower = -1;
            break;
        
     case 2: 
            sprintf(mess,"SWiped line %d.", lower);
            
            break;
        
     case 3: 
            sprintf(mess,"SWiped from line %d to the end.",lower);
            break;
        
     case 4: 
            sprintf(mess,"SWiped from begining of board to line %d.",upper);
            break;
        
     case 5: 
            sprintf(mess,"SWiped all except lines %d to %d.",upper, lower);
            break;
        
     default: return;
              break;
    }


remove_lines_from_file(user, 
                       filename, 
                       lower, 
                       upper);

write_str(user,mess);
if (!file_count_lines(filename)) remove(filename);

}

/*-----------------------------------------------*/
/* anchor a user down                            */
/*-----------------------------------------------*/
void anchor_user(int user, char *inpstr)
{
char buf[256];
char other_user[ARR_SIZE];
int u,inlen;
unsigned int i;

if (!strlen(inpstr)) 
  {
   write_str(user,"Users Anchored & logged on     Time left"); 
   write_str(user,"--------------------------     ---------"); 
   for (u=0;u<MAX_USERS;++u) 
    {
     if (ustr[u].anchor  && ustr[u].area > -1) 
       {
        if (ustr[u].anchor_time == 0)
           sprintf(mess,"%-29s   %s",ustr[u].say_name,"Perm");
        else
           sprintf(mess,"%-29s   %s",ustr[u].say_name,converttime((long)ustr[u].anchor_time));
        write_str(user, mess);
       };
    }
   write_str(user,"(end of list)");
   return;
  }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   not_signed_on(user,other_user);
   return;
  }
  
if (u == user)
  {   
   write_str(user,"You are definitly wierd! Trying to anchor yourself, geesh."); 
   return;
  }

 if ((!strcmp(ustr[u].name,ROOT_ID)) || (!strcmp(ustr[u].name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    return;
    }

if (ustr[user].tempsuper <= ustr[u].super) 
  {
   write_str(user,"That would not be wise...");
   sprintf(mess,ANCHOR_CANT,ustr[user].say_name);
   write_str(u,mess);
   return;
  }

if (ustr[u].anchor == 0) {
remove_first(inpstr);
if (strlen(inpstr) && strcmp(inpstr,"0")) {
   if (strlen(inpstr) > 5) {
      write_str(user,"Minutes cant exceed 5 digits.");
      return;
      }
   inlen=strlen(inpstr);
   for (i=0;i<inlen;++i) {
     if (!isdigit((int)inpstr[i])) {
        write_str(user,"Numbers only!");
        return;
        }
     }
    i=0;
    i=atoi(inpstr);
    if ( i > 32767) {
       write_str(user,"Minutes cant exceed 32767.");
       i=0;
       return;
      }
  i=0;
  ustr[u].anchor_time=atoi(inpstr);
  ustr[u].anchor = 1;
    write_str(u,ANCHORON_MESS);
    sprintf(mess,"ANCHOR ON : %s by %s for %s\n",ustr[u].say_name, ustr[user].say_name, converttime((long)ustr[u].anchor_time));
}
else {
    ustr[u].anchor = 1;
    ustr[u].anchor_time=0;
    write_str(u,ANCHORON_MESS);
    sprintf(mess,"ANCHOR ON : %s by %s\n",ustr[u].say_name, ustr[user].say_name);
  }
}     /* end of if anchored */

else {
    ustr[u].anchor = 0;
    ustr[u].anchor_time=0;
    write_str(u,ANCHOROFF_MESS);
    sprintf(mess,"ANCHOR OFF: %s by %s\n",ustr[u].say_name, ustr[user].say_name);
  } 

btell(user, mess);

 strcpy(buf,get_time(0,0));    
 strcat(buf," ");
 strcat(buf,mess);
print_to_syslog(buf);

write_str(user,"Ok");
}


/** turn welcome quote on or off **/
void quote_op(int user, char *inpstr)
{
char line[257];
FILE *pp;

if (!strlen(inpstr)) {
  if (ustr[user].quote)
    {
      write_str(user, "Quote feature now  off.");
      ustr[user].quote = 0;
    }
   else
    {
      write_str(user,"Quote feature now  on.");
      ustr[user].quote = 1;
    }
    
  read_user(ustr[user].login_name);
  t_ustr.quote = ustr[user].quote;
  write_user(ustr[user].login_name);
 }
else {

if (!strcmp(inpstr,"-l") || !strcmp(inpstr,"-s")) {
write_str(user,"+---------------------------------------------------------------------------+");
   if (!strcmp(inpstr,"-l"))
    sprintf(t_mess,"%s 2> /dev/null",FORTPROG);
   else if (!strcmp(inpstr,"-s"))
    sprintf(t_mess,"%s -s 2> /dev/null",FORTPROG);

 if (!(pp=popen(t_mess,"r"))) {
        write_str(user,"No quote.");
        return;
        }
while (fgets(line,256,pp) != NULL) {
        line[strlen(line)-1]=0;
        write_str(user,line);
      } /* end of while */
pclose(pp);

write_str(user,"+---------------------------------------------------------------------------+");
 }
 else write_str(user,"Option not understood.");

 } /* end of else */

}

/** list last current logins **/
void list_last(int user, char *inpstr)
{
  int num=0;
  char line[257];
  char filename[FILE_NAME_LEN];
  FILE *fp;
  FILE *pp;

if (!strlen(inpstr)) {
write_str(user,"^HG+----------------------------------------------------------+^");
sprintf(mess,"    ^HYLast 10 logins to %s^",SYSTEM_NAME);
write_str(user,mess);
write_str(user,"^HG+----------------------------------------------------------+^");
write_str(user,"");

  strcpy(filename,get_temp_file());
  sprintf(mess,"tail -10 %s",LASTLOGS);
 
 if (!(pp=popen(mess,"r"))) {
	write_str(user,"Can't open pipe to get those logs!");
	return;
	}
 if (!(fp=fopen(filename,"w"))) {
	write_str(user,"Can't open temp file for writing!");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	fputs(line,fp);
      } /* end of while */
fclose(fp);
pclose(pp);

   cat(filename,user,0);
   write_str(user,"");
   return;
   }

  for (num=0;num<strlen(inpstr);num++) {
  if (!isdigit((int)inpstr[num])) {
     write_str(user,"Lines from bottom must be a number.");
     return;
     }
  }
  num=0;
  num=atoi(inpstr);

  if (num > 100) {
   write_str(user,"Input number too big. No greater than 100");
   return;
   }

write_str(user,"^HG+----------------------------------------------------------+^");
sprintf(mess,"    ^HYLast %2d logins to %s^",num,SYSTEM_NAME);
write_str(user,mess);
write_str(user,"^HG+----------------------------------------------------------+^");
write_str(user,"");

  strcpy(filename,get_temp_file());
  sprintf(mess,"tail -%d %s",num,LASTLOGS);
 
 if (!(pp=popen(mess,"r"))) {
	write_str(user,"Can't open pipe to get those logs!");
	return;
	}
 if (!(fp=fopen(filename,"w"))) {
	write_str(user,"Can't open temp file for writing!");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	fputs(line,fp);
      } /* end of while */
fclose(fp);
pclose(pp);

   cat(filename,user,0);
   write_str(user,"");
}

/*
 ** authuser.c
 **
 ** 2/2/93: Added auth_setreadtimeout() call.
 ** 7/21/92: Fixed SIGPIPE bug in auth_tcpuser3(). <pen@lysator.liu.se>
 ** 2/9/92: authuser 4.0. Public domain.
 ** 2/9/92: added bunches of zeroing just in case.
 ** 2/9/92: added auth_tcpuser3. uses bsd 4.3 select interface.
 ** 2/9/92: added auth_tcpsock, auth_sockuser.
 ** 2/9/92: added auth_fd2, auth_tcpuser2, simplified some of the code.
 ** 12/27/91: fixed up usercmp to deal with restricted tolower XXX
 ** 5/6/91 DJB baseline authuser 3.1. Public domain.
 */
static void clearsa(struct sockaddr_in *sa)
{
    register char *x;
    for (x = (char *) sa;x < sizeof(*sa) + (char *) sa;++x)
	*x = 0;
}

static int usercmp(register char *u,register char *v)
{
    register char uc;
    register char vc='\0';
    register char ucvc;
    /* is it correct to consider Foo and fOo the same user? yes */
    /* but the function of this routine may change later */
    while ((uc = *u) && (vc = *v))
    {
	ucvc = (isupper((int)uc) ? tolower((int)uc) : uc) - (isupper((int)vc) ? tolower((int)vc) : vc);
	if (ucvc)
	    return ucvc;
	else
	    ++u,++v;
    }
    return uc || vc;
}

static char authline[SIZ];

char *auth_xline(register char *user, register int fd, register unsigned long *in)
{
    unsigned short local;
    unsigned short remote;
    register char *ruser;
    
    if (auth_fd(fd,in,&local,&remote) == -1)
	return 0;
    ruser = auth_tcpuser(*in,local,remote);
    if (!ruser)
	return 0;
    if (!user)
	user = ruser; /* forces X-Auth-User */
    sprintf(authline,
	    (usercmp(ruser,user) ? "X-Forgery-By: %s" : "X-Auth-User: %s"),
	    ruser);
    return authline;
}

int auth_fd(register int fd, register unsigned long *in, register unsigned short *local, register unsigned short *remote)
{
    unsigned long inlocal;
    return auth_fd2(fd,&inlocal,in,local,remote);
}

int auth_fd2(register int fd, register unsigned long *inlocal, register unsigned long *inremote, register unsigned short *local, register unsigned short *remote)
{
    struct sockaddr_in sa;
    socklen_t dummy;
    
    dummy = sizeof(sa);
    if (getsockname(fd,(struct sockaddr *)&sa,&dummy) == -1)
	return -1;
    if (sa.sin_family != AF_INET)
    {
	errno = EAFNOSUPPORT;
	return -1;
    }
    *local = ntohs(sa.sin_port);
    *inlocal = sa.sin_addr.s_addr;
    dummy = sizeof(sa);
    if (getpeername(fd,(struct sockaddr *)&sa,&dummy) == -1)
	return -1;
    *remote = ntohs(sa.sin_port);
    *inremote = sa.sin_addr.s_addr;
    return 0;
}

static char ruser[SIZ];
static char realbuf[SIZ];
static char *buf;

char *auth_tcpuser(register unsigned in, register unsigned short local, register unsigned short remote)
{
    return auth_tcpuser2(0,in,local,remote);
}

#define CLORETS(e) { saveerrno = errno; CLOSE(s); errno = saveerrno; return e; }

int auth_tcpsock(register unsigned long inlocal, register unsigned long inremote)
{
    struct sockaddr_in sa;
    register int s;
    register int fl;
    register int saveerrno;
    
    if ((s = socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET)
	return -1;
    if (inlocal)
    {
	clearsa(&sa);
#if defined(__FreeBSD__)
	sa.sin_len = sizeof(sa);
#endif /* __FreeBSD__ */
	sa.sin_family = AF_INET;
	sa.sin_port = 0;
	sa.sin_addr.s_addr = inlocal;
	if (bind(s,(struct sockaddr *)&sa,sizeof(sa)) == -1)
	    CLORETS(-1)
	    }

    if ((fl = fcntl(s,F_GETFL,0)) == -1)
	CLORETS(-1);
    if (fcntl(s,F_SETFL,NBLOCK_CMD | fl) == -1)
	CLORETS(-1);
    clearsa(&sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(auth_tcpport);
    sa.sin_addr.s_addr = inremote;
#if defined(WIN32) && !defined(__CYGWIN32__)
    if (connect(s,(struct sockaddr *)&sa,sizeof(sa)) == SOCKET_ERROR)
#else
    if (connect(s,(struct sockaddr *)&sa,sizeof(sa)) == -1)
#endif
	if (errno != EINPROGRESS)
	    CLORETS(-1)
		return s;
}

char *auth_tcpuser2(register unsigned long inlocal, register unsigned long inremote, register unsigned short local, register unsigned short remote)
{
    register int s;
    
    s = auth_tcpsock(inlocal,inremote);
    if (s == -1)
	return 0;
    return auth_sockuser(s,local,remote);
}

char *auth_tcpuser3(register unsigned long inlocal, register unsigned long inremote, register unsigned short local, register unsigned short remote, register int ctimeout)
{
    return auth_tcpuser4(inlocal,
			 inremote,
			 local,
			 remote,
			 ctimeout,
			 auth_rtimeout);
}


char *auth_tcpuser4(register unsigned long inlocal, register unsigned long inremote, register unsigned short local, register unsigned short remote, register int ctimeout, register int rtimeout)
{
    register int s;
    struct timeval ctv;
    fd_set wfds;
    register int r;
    register int saveerrno;
    /* void *old_sig; */
    char *retval;
    
    
    /* old_sig = signal(SIGPIPE, SIG_IGN); */
    
    s = auth_tcpsock(inlocal,inremote);
    if (s == -1)
    {
	/* signal(SIGPIPE, old_sig); */
	return 0;
    }
    ctv.tv_sec = ctimeout;
    ctv.tv_usec = 0;
    FD_ZERO(&wfds);
    FD_SET(s,&wfds);
    r = select(s + 1,(void *) 0,(void *)&wfds,(void *) 0,&ctv);
    /* XXX: how to handle EINTR? */
    if (r == -1)
    {
	/* signal(SIGPIPE, old_sig); */
	CLORETS(0);
    }
    if (!FD_ISSET(s,&wfds))
    {
	CLOSE(s);
	FD_CLR(s,&wfds);
	errno = ETIMEDOUT;
	/* signal(SIGPIPE, old_sig); */
	return 0;
    }
    retval = auth_sockuser2(s,local,remote,rtimeout);
    /* signal(SIGPIPE, old_sig); */
    
    return retval;
}
 
char *auth_sockuser(register int s, register unsigned short local, register unsigned short remote)
{
    return auth_sockuser2(s, local, remote, auth_rtimeout);
}

char *auth_sockuser2(register int s, register unsigned short local, register unsigned short remote, int rtimeout)
{
    register int buflen;
    register int w;
    register int saveerrno;
    char ch;
    unsigned short rlocal;
    unsigned short rremote;
    register int fl;
    fd_set wfds;
    /* void *old_sig; */
    struct timeval rtv;
    
    /* old_sig = signal(SIGPIPE, SIG_IGN); */
    
    FD_ZERO(&wfds);
    FD_SET(s,&wfds);
    
    select(s + 1,
	   (void *) 0,
	   (void *)&wfds,(void *) 0,
	   (struct timeval *) 0);
    
    /* now s is writable */
    if ((fl = fcntl(s,F_GETFL,0)) == -1)
    {
	/* signal(SIGPIPE, old_sig); */
	CLORETS(0);
    }
    if (fcntl(s,F_SETFL,~NBLOCK_CMD & fl) == -1)
    {
	/* signal(SIGPIPE, old_sig); */
	CLORETS(0);
    }
    buf = realbuf;
    sprintf(buf,"%u , %u\r\n",(unsigned int) remote,(unsigned int) local);
    /* note the reversed order---the example in RFC 931 is misleading */
    buflen = strlen(buf);
    while ((w = S_WRITE(s,buf,buflen)) < buflen)
	if (w == -1) /* should we worry about 0 as well? */
	{
	    /* signal(SIGPIPE, old_sig); */
	    CLORETS(0);
	}
	else
	{
	    buf += w;
	    buflen -= w;
	}
    buf = realbuf;
    
    do
    {
	fd_set rd_fds;
	
	rtv.tv_sec = rtimeout;
	rtv.tv_usec = 0;
    
	FD_ZERO(&rd_fds);
	FD_SET(s, &rd_fds);
	if (select(s+1,
		   (void *)&rd_fds,
		   (void *) 0,
		   (void *) 0,
		   &rtv) == 0)
	{
	    w = -1;
	    goto END;
	}
	
	if ((w = S_READ(s,&ch,1)) == 1)
	{
	    *buf = ch;
	    if ((ch != ' ') && (ch != '\t') && (ch != '\r'))
		++buf;
	    if ((buf - realbuf == sizeof(realbuf) - 1) || (ch == '\n'))
		break;
	}
    } while (w == 1);
    
    END: 
    /* signal(SIGPIPE, old_sig); */
    if (w == -1)
	CLORETS(0)
	    *buf = 0;
    
    if (sscanf(realbuf, "%hd,%hd: USERID :%*[^:]:%s",
	       &rremote, &rlocal, ruser) < 3)
    {
	CLOSE(s);
	FD_CLR(s,&wfds);
	errno = EIO;
	/* makes sense, right? well, not when USERID failed to match ERROR */
	/* but there's no good error to return in that case */
	return 0;
    }
    if ((remote != rremote) || (local != rlocal))
    {
	CLOSE(s);
	FD_CLR(s,&wfds);
	errno = EIO;
	return 0;
    }
    /* we're not going to do any backslash processing */
    CLOSE(s);
    FD_CLR(s,&wfds);
    return ruser;
}

/*-----------------------------------------------------------------*/
/*  The realuser command. returns the real login of a user if the  */
/*  site the user telnet from runs an ident daemon port 113 tcp	   */
/*-----------------------------------------------------------------*/
void real_user(int user, char *inpstr)
{
 char           other_user[ARR_SIZE];
 int	        u;
 unsigned long  inlocal;
 unsigned long  inremote;
 unsigned short local;
 unsigned short remote;
 char         * real_name;

 if (!strlen(inpstr)) 
   {
    write_str(user,"usage: .realuser user_id     - returns the actual login user id if it is available  "); 
    return;
   }
 
 sscanf(inpstr, "%s ", other_user);
 strtolower(other_user);
 
 remove_first(inpstr);
 
 if ((u = get_user_num(other_user, user))== -1) 
   {
    not_signed_on(user, other_user);  
    write_str(user,"- done -");
    return;
   }
   
 /*------------------------------------------*/
 /* if ident has not been run on this user,  */
 /* run now                                  */
 /*------------------------------------------*/
 
 if (ustr[u].real_id[0] == 0)
   {
    auth_fd2(ustr[u].sock, &inlocal, &inremote, &local, &remote);
 
    if ( (real_name = auth_tcpuser2(inlocal, inremote, local, remote)) == NULL ) 
      {
       sprintf(t_mess,"- NO IDENT @%s -", ustr[u].net_name);
       strcpy(ustr[u].real_id, "NO IDENT");
      } 
     else 
      {
       sprintf(t_mess,"- %s@%s -", real_name, ustr[u].net_name);
       strcpy(ustr[u].real_id, real_name);
      }
   
    write_str(user, t_mess);
 
    write_str(user,"- done (non-cached) -");
   }
  else
   /*----------------------------------------------*/
   /* ident was already run, use the cached info   */
   /*----------------------------------------------*/
   {
    sprintf(t_mess,"- %s@%s -", ustr[u].real_id, ustr[u].net_name);
    write_str(user, t_mess);
    write_str(user,"- done (cached) -");
   }
 
 }

/*** Use system's nslookup command to resolve an ip address ***/
void pukoolsn(int user, char *inpstr)
{
   char line[257];
   char filename[FILE_NAME_LEN];
   FILE *fp;
   FILE *pp;

   if (strpbrk(inpstr,";$/+*[]\\") ) {
        write_str(user,"Illegal character in ip address");
        return;
        }

   if ((!strlen(inpstr)) || (strlen(inpstr) < 7)) {
       write_str(user,"You need to specify a valid ip address");
       return;
       }
   if (strlen(inpstr) > 25) {
       write_str(user,"Address specified too long. No greater than 25 chars.");
       return;
       }
  strcpy(filename,get_temp_file());
  sprintf(mess,"nslookup %s 2> /dev/null",inpstr);
 
 if (!(pp=popen(mess,"r"))) {
	write_str(user,"Can't open pipe to do an nslookup!");
	return;
	}
 if (!(fp=fopen(filename,"w"))) {
	write_str(user,"Can't open temp file for writing!");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	fputs(line,fp);
      } /* end of while */
fclose(fp);
pclose(pp);

if (!cat(filename,user,0))
    write_str(user,"No info.");
print_to_syslog("NSLOOKUP QUERY TO HOST\n");
write_str(user,"Done.");
}

/*** Use system's finger command to get info on troublesome user ***/
void regnif(int user, char *inpstr)
{
   char line[257];
   char filename[FILE_NAME_LEN];
   FILE *fp;
   FILE *pp;

   if (strpbrk(inpstr,";$/+*[]\\") ) {
        write_str(user,"Illegal character in address");
        return;
        }
   if ((!strlen(inpstr)) || (strlen(inpstr) < 8)) {
       write_str(user,"You need to specify address in form: user@host");
       return;
       }
   if (strlen(inpstr) > 60) {
       write_str(user,"Address specified too long. No greater than 60 chars.");
       return;
       }
strcpy(filename,get_temp_file());
sprintf(mess,"finger %s 2> /dev/null",inpstr);
 if (!(pp=popen(mess,"r"))) {
	write_str(user,"Can't open pipe to do a finger!");
	return;
	}
 if (!(fp=fopen(filename,"w"))) {
	write_str(user,"Can't open temp file for writing!");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	fputs(line,fp);
      } /* end of while */
fclose(fp);
pclose(pp);

if (!cat(filename,user,0))
    write_str(user,"No info.");
print_to_syslog("FINGER QUERY TO HOST\n");
write_str(user,"Done.");
}

/*** Use system's whois command to find what a domain name is ***/
void siohw(int user, char *inpstr)
{
   char line[257];
   char filename[FILE_NAME_LEN];
   FILE *fp;
   FILE *pp;
   
   if (strpbrk(inpstr,";:$/+*[]\\") ) {
        write_str(user,"Illegal character in search string");
        return;
        }

   if ((!strlen(inpstr)) || (strlen(inpstr) < 3)) {
       write_str(user,"You need to specify a valid search string");
       return;
       }
   if (strlen(inpstr) > 45) {
       write_str(user,"String specified too long. No greater than 45 chars.");
       return;
       }
strcpy(filename,get_temp_file());
#if defined(__linux__)
sprintf(mess,"fwhois %s 2> /dev/null",inpstr);
#else
sprintf(mess,"whois -h rs.internic.net %s 2> /dev/null",inpstr);
#endif

 if (!(pp=popen(mess,"r"))) {
	write_str(user,"Can't open pipe to do a whois!");
	return;
	}
 if (!(fp=fopen(filename,"w"))) {
	write_str(user,"Can't open temp file for writing!");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	fputs(line,fp);
      } /* end of while */
fclose(fp);
pclose(pp);

if (!cat(filename,user,0))
    write_str(user,"No info.");
print_to_syslog("WHOIS QUERY TO HOST\n");
write_str(user,"Done.");
}

/*** Clear Sent Mail ***/
void clear_sent(int user, char *inpstr)
{
char filename[FILE_NAME_LEN];
FILE *bfp;
int lower=-1;
int upper=-1;
int mode=0;

/*--------------------------------------------------*/
/* check if there is any sent mail                  */
/*--------------------------------------------------*/

sprintf(t_mess,"%s/%s.sent",MAILDIR,ustr[user].name);
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(bfp=fopen(filename,"r"))) 
  {
   write_str(user,"You have no sent mail."); 
   return;
  }
FCLOSE(bfp);

/* remove the mail file */
if (ustr[user].clrmail== -1) 
  {
   /*---------------------------------------------*/
   /* get the delete parameters                   */
   /*---------------------------------------------*/

   get_bounds_to_delete(inpstr, &lower, &upper, &mode);
 
   if (upper == -1 && lower == -1)
     {
      write_str(user,"No sent mail deleted.  Specification of what to ");
      write_str(user,"delete did not make sense.  Type: .help csent ");
      write_str(user,"for detailed instructions on use. ");
      return;
     }
    
   switch(mode)
    {
     case 0: return;
             break;
        
     case 1: 
            sprintf(mess,"Csent: Delete all sent mail messages? ");
            upper = -1;
            lower = -1;
            break;
        
     case 2: 
            sprintf(mess,"Csent: Delete line %d? ", lower);            
            break;
        
     case 3: 
            sprintf(mess,"Csent: Delete from line %d to the end?",lower);
            break;
        
     case 4: 
            sprintf(mess,"Csent: Delete from begining to line %d?",upper);
            break;
        
     case 5: 
            sprintf(mess,"Csent: Delete all except lines %d to %d?",upper,lower);
            break;
        
     default: return;
              break;
    }

   ustr[user].lower = lower;
   ustr[user].upper = upper;

   ustr[user].clrmail=user; 
   noprompt=1;
   delete_sent=1;
   write_str(user,mess);
   write_str_nr(user,"Do you wish to do this? (y/n) ");
	telnet_write_eor(user);
   return;
  }
  
remove_lines_from_file(user, 
                       filename, 
                       ustr[user].lower, 
                       ustr[user].upper);

sprintf(mess,"You deleted specified sent mail messages.");
write_str(user,mess);

if (!file_count_lines(filename))  remove(filename);
delete_sent=0;

}

/*** Read Mail ***/
void read_sent(int user, char *inpstr)
{
int b=0;
int a=0;
int lines=0;
int num_lines=0;
char junk[1001];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
FILE *fp;
FILE *tfp;

if (strlen(inpstr)) {
   if ((strlen(inpstr) <3) && (strlen(inpstr) > 0) &&
      (!isalpha((int)inpstr[0])))
       {
       num_lines=atoi(inpstr);
       a=1;
       }
   else {
       write_str(user,"Number invalid.");
       return;
       }
  }

/* Send output to user */
sprintf(t_mess,"%s/%s.sent",MAILDIR,ustr[user].name);
strncpy(filename,t_mess,FILE_NAME_LEN);

sprintf(mess,"\n** Your Private SENT Mail Console **");
write_str(user,mess);

if (a==1) {
   if (num_lines < 1) {
      if (!cat(filename,user,1))
          {
           write_str(user,"Your sent mail box is empty.");
          }
     goto DONE; 
    }
lines = file_count_lines(filename);
if (num_lines <= lines) {
   ustr[user].numbering = (lines - num_lines) +1;
   }
 else {
      num_lines=0;
      }
num_lines = lines - num_lines;

if (!(fp=fopen(filename,"r"))) {
    write_str(user,"Your sent mail box is empty.");
    num_lines=0; a=0; lines=0;
    ustr[user].numbering = 0;
    return;
    }
strcpy(filename2,get_temp_file());
tfp=fopen(filename2,"w");

while (!feof(fp)) {
         fgets(junk,1000,fp);
         b++;
         if (b <= num_lines)  {
             junk[0]=0;
             continue;
            }
          else {
             fputs(junk,tfp);
             junk[0]=0;
             }
       }
FCLOSE(fp);
FCLOSE(tfp);
num_lines=0;  lines=0;
if (!cat(filename2,user,1))
    write_str(user,"Your sent mail box is empty.");

DONE:
b=0;
}

if (a==0) {
ustr[user].numbering = 0;
if (!cat(filename,user,1))
   write_str(user,"Your sent mail box is empty.");
 }

ustr[user].numbering= 0;
}

/*--------------------------------------------------------------------*/
/* this command basically takes a ip address OR hostname string       */
/* that's inputted by the user and checks for all users with that     */
/* string in their last site.     BY REQUEST *grin*                   */
/*--------------------------------------------------------------------*/
void same_site(int user, char *inpstr)
{
int a=0;
int num,netname=0,foundnum=0;
int len=strlen(inpstr);
char buffer[ARR_SIZE];
char buffer2[ARR_SIZE];
char small_buff[ARR_SIZE];
char filerid[FILE_NAME_LEN];
char filename[FILE_NAME_LEN];
time_t tm;
struct dirent *dp;
FILE *fp;
DIR  *dirp;
 
 if (strlen(inpstr) < 3) {
    write_str(user,"Search string too short..spam precaution *grin*");
    return;
    }

num=0;
foundnum=0;
for (a=0;a<len;++a) {
    if (isdigit((int)inpstr[a]) && (num==-2)) {
      num=-1; break;
      }
    else if (isdigit((int)inpstr[a])) {
     foundnum=1;
     num=-3;
     }
    else
     num=-3;

    if (isalpha((int)inpstr[a])) {
      if (strstr(inpstr,".")) {
        num=-1; break;
        }
      else {
        if (foundnum!=1) num=-2;
        else { num=-1; break; }
        } /* end of else */
      } /* end of sub if */

   }  /* end of for */
a=0;

/* Is user search */
 if (num==-2) {
     strtolower(inpstr);
     if (!read_user(inpstr)) {
       write_str(user,NO_USER_STR);
       return;
       }
     strcpy(buffer,t_ustr.last_site);
     /* write_user(inpstr); */
     strtolower(buffer);
     }

/* Check for hostname search string */
 else if (num==-1) {
     netname=1;
     strcpy(buffer,inpstr);
     strtolower(buffer);
     }

/* Check for IP search string */
 else if (num==-3) {
     strcpy(buffer,inpstr);
     strtolower(buffer);
     }

 else {
     write_str(user,"Invalid search string or user.");
     return;
     }

 sprintf(t_mess,"%s",USERDIR);
 strncpy(filerid,t_mess,FILE_NAME_LEN);
 
 num=0;
 dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    return;
   }

 time(&tm);   /* for time ago from rawtime */

   strcpy(filename,get_temp_file());
   if (!(fp=fopen(filename,"w"))) {
     sprintf(mess,"%s Can't create file for paged same_site listing!",ctime(&tm));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     (void) closedir(dirp);
     return;
     }
   
 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
    if (small_buff[0]=='.') continue;
    read_user(small_buff);     /* Read user's profile */
      if (!strcmp(t_ustr.last_site,"127.0.0.1")) {
          t_ustr.last_site[0]=0; continue; }
      if (!netname) {
       strcpy(buffer2,t_ustr.last_site);
       if (strstr(buffer2,buffer))  /* Search for string in user's last site */
        {
            sprintf(mess,"%-18s from %14s, %s ago",t_ustr.say_name,t_ustr.last_site,converttime((long)((tm-t_ustr.rawtime)/60)) );
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.last_site[0]=0;
            num++;
        } 
       t_ustr.last_site[0]=0;
       buffer2[0]=0;
      }
     else {
       strcpy(buffer2,t_ustr.last_name);
       strtolower(buffer2);
       if (strstr(buffer2,buffer))  /* Search for string in user's last hostname */
        {
            sprintf(mess,"%-18s from %s\n\r%s ago\n\r",t_ustr.say_name,t_ustr.last_name,converttime((long)((tm-t_ustr.rawtime)/60)) );
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.last_name[0]=0;
            num++;
        } 
       t_ustr.last_name[0]=0;
       buffer2[0]=0;
      }
   }       /* End of while */
 
 t_ustr.last_site[0]=0;
 t_ustr.last_name[0]=0;
 buffer2[0]=0;
 netname=0;
 fputs("\n",fp);
 sprintf(mess,"Displayed %d user%s",num,num == 1 ? "" : "s");
 fputs(mess,fp);
 fputs("\n",fp);

 fclose(fp);
 (void) closedir(dirp);

 if (!cat(filename,user,0)) {
     sprintf(mess,"%s Can't cat same_site file!",ctime(&tm));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
    }

 return;
}

/*---------------------------------------------------------------------*/
/* Gags user from .telling, .semote, .sthink, .ptell .beep, and .smail */
/*---------------------------------------------------------------------*/
void gag(int user, char *inpstr)
{
int i=0,found=0,u;
unsigned long diff=0;
char other_user[ARR_SIZE];
char check[50];
time_t tm;

if (!strlen(inpstr)) {
    write_str(user,"Users that you have gagged");
    write_str(user,"+-----------------------------------------------------------+");
    time(&tm);
     for (i=0;i<MAX_GAG;++i) {
       if (strlen(ustr[user].gagged[i])) {
             strcpy(check,ustr[user].gagged[i]);
             strtolower(check);

            if ((u=get_user_num_exact(check,user))==-1) {
             if (!read_user(check)) {
              sprintf(mess,"%-18s doesn't exist yet",ustr[user].gagged[i]);
              write_str(user,mess);
              found=1; continue;
              }
             diff=tm-t_ustr.rawtime;
             sprintf(mess,"%-18s last on %s ago",t_ustr.say_name,converttime((long)(diff/60)));
             write_str(user,mess);
             diff=0; t_ustr.rawtime=0;
             }
             else {
		if (!ustr[u].vis) {
		  if (ustr[user].tempsuper >= MIN_HIDE_LEVEL)
                   sprintf(mess,"^HY%-18s^ is online right NOW!",ustr[user].gagged[i]);
		  else {
                    diff=tm-ustr[u].rawtime;
                    sprintf(mess,"%-18s last on %s ago",ustr[user].gagged[i],converttime((long)(diff/60)));
		   }
		  } /* end of vis if */
		else {
                 sprintf(mess,"^HY%-18s^ is online right NOW!",ustr[user].gagged[i]);
		}
              write_str(user,mess);
             }
         found=1;
        } /* end of if strlen */
      } /* end of for */
     i=0;
    if (!found) write_str(user,"You have noone gagged.");
    write_str(user,"+-----------------------------------------------------------+");
    return;
    }

if (strlen(inpstr) > NAME_LEN) {
   write_str(user,"Name too long. Sorry.");
   return;
   }

/* plug security hole */
if (check_fname(inpstr,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

if (!strcmp(ustr[user].name,other_user)) {
    write_str(user,"Why would you want to add yourself?!");
    return;
    }

      for (i=0; i<MAX_GAG; ++i) {
        strcpy(check,ustr[user].gagged[i]);
        strtolower(check);
        if (!strcmp(other_user,check)) { 
            found=1;
            break;
           }
        check[0]=0;
       } /* end of for */

/* If not found, add user to list */
if (!found) {
       if (ustr[user].gag_num < MAX_GAG) {
        if (!read_user(other_user)) {
            write_str(user,NO_USER_STR);
            return;
            }
       if ((t_ustr.super >= WIZ_LEVEL) && (ustr[user].tempsuper < t_ustr.super)) {
	   if (strcmp(t_ustr.name,BOT_ID)) {
           write_str(user,"You can't gag users of that level.");
           return;
	   }
          }
        /* User exists, so add to a free slot */
        i=0;
        for (i=0; i<MAX_GAG; ++i) {
           if (!strlen(ustr[user].gagged[i])) break;
          }
        strcpy(ustr[user].gagged[i],t_ustr.say_name);
        write_str(user,"User gagged.");
        ustr[user].gag_num++;
        }
       else {
        write_str(user,"You can't add any more users to your list.");
        }
   }
else {
        write_str(user,"User UN-gagged.");
        ustr[user].gagged[i][0]=0;
        ustr[user].gag_num--;
     }

copy_from_user(user);
write_user(ustr[user].name);
}


/*---------------------------------------------------------------------*/
/* Alert you with beep when specified user logs into the talker        */
/*---------------------------------------------------------------------*/
void alert(int user, char *inpstr)
{
int i=0,found=0,u;
unsigned long diff=0;
char other_user[ARR_SIZE];
char check[50];
time_t tm;

if (!strlen(inpstr)) {
    write_str(user,"Users that you are being alerted of");
    write_str(user,"+-----------------------------------------------------------+");
    time(&tm);
     for (i=0;i<MAX_ALERT;++i) {
       if (strlen(ustr[user].friends[i])) {
             strcpy(check,ustr[user].friends[i]);
             strtolower(check);

            if ((u=get_user_num_exact(check,user))==-1) {
             if (!read_user(check)) {
              sprintf(mess,"%-18s doesn't exist yet",ustr[user].friends[i]);
              write_str(user,mess);
              found=1; continue;
              }
             diff=tm-t_ustr.rawtime;
             sprintf(mess,"%-18s last on %s ago",t_ustr.say_name,converttime((long)(diff/60)));
             write_str(user,mess);
             diff=0; t_ustr.rawtime=0;
             }
             else {
		if (!ustr[u].vis) {
		  if (ustr[user].tempsuper >= MIN_HIDE_LEVEL)
                   sprintf(mess,"^HY%-18s^ is online right NOW!",ustr[user].friends[i]);
		  else {
                    diff=tm-ustr[u].rawtime;
                    sprintf(mess,"%-18s last on %s ago",ustr[user].friends[i],converttime((long)(diff/60)));
		   }
		  } /* end of vis if */
		else {
                 sprintf(mess,"^HY%-18s^ is online right NOW!",ustr[user].friends[i]);
		}
              write_str(user,mess);
             }
         found=1;
        } /* end of if strlen */
      } /* end of for */
     i=0;
    if (!found) write_str(user,"You are not being alerted of anyone");
    write_str(user,"+-----------------------------------------------------------+");
    check[0]=0;
    return;
    }

if (strlen(inpstr) > NAME_LEN) {
   write_str(user,"Name too long. Sorry.");
   return;
   }

/* plug security hole */
if (check_fname(inpstr,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

if (!strcmp(ustr[user].name,other_user)) {
    write_str(user,"Why would you want to add yourself?!");
    return;
    }

      for (i=0; i<MAX_ALERT; ++i) {
        strcpy(check,ustr[user].friends[i]);
        strtolower(check);
        if (!strcmp(other_user,check)) { 
            found=1;
            break;
           }
        check[0]=0;
       } /* end of for */

/* If not found, add user to list */
if (!found) {
       if (ustr[user].friend_num < MAX_ALERT) {
        if (!read_user(other_user)) {
            write_str(user,NO_USER_STR);
            return;
            }
        /* User exists, so add to a free slot */
        i=0;
        for (i=0; i<MAX_ALERT; ++i) {
           if (!strlen(ustr[user].friends[i])) break;
          }
        strcpy(ustr[user].friends[i],t_ustr.say_name);
        write_str(user,"User added to Alert list.");
        ustr[user].friend_num++;
        }
       else {
        write_str(user,"You can't add any more users to your list.");
        }
   }
else {
        write_str(user,"User removed from Alert list.");
        ustr[user].friends[i][0]=0;
        ustr[user].friend_num--;
     }

copy_from_user(user);
write_user(ustr[user].name);
}


/** read the monthly schedule file **/
void schedule(int user)
{
char filename[FILE_NAME_LEN];

write_str(user," ");
sprintf(filename,"%s",SCHEDFILE);
if (!cat(filename,user,0))
  write_str(user,"No schedule exists for the month!");
write_str(user," ");

}

/** sing function for music heads **/
void sing(int user, char *inpstr)
{

if (!strlen(inpstr)) {
    write_str(user,NO_SINGSTR);
    return;
    }

sprintf(t_mess,"^<singing along>^ %s",inpstr);
strcat(t_mess,"\0");
emote(user,t_mess);

}


/*--------------------------------------------------*/
/* Show users that have been gone one month or more */
/* Nuke them if  -n is specified                    */
/*--------------------------------------------------*/
void show_expire(int user, char *inpstr)
{
int i,num,u,nuke_user=0;
int found=0;
int add=0;
unsigned long diff=0;
unsigned long limit=0;
char small_buff[64],n_option[80];
char tempname[NAME_LEN+1];
char now_date[30];
char filerid[FILE_NAME_LEN];
time_t tm;
struct dirent *dp;
DIR  *dirp;

if (!strlen(inpstr)) goto SHOW;

sscanf(inpstr,"%s ",n_option);
if (!strcmp(n_option,"exempt") || !strcmp(n_option,"-e")) {
    remove_first(inpstr);
    if (!strlen(inpstr)) {
        write_str(user," Users exempted from .expire");
        write_str(user,"+-----------------------------------------------------------+");
        time(&tm);
         for (i=0;i<NUM_EXPIRES;++i) {
            if (!strcmp(expired[i],"name"))
             write_str(user,"< not in use >");
            else {
             strcpy(tempname,expired[i]);
             strtolower(tempname);
            if ((u=get_user_num_exact(tempname,user))==-1) {
             if (!read_user(tempname)) {
              sprintf(mess,"%-18s doesn't exist yet",expired[i]);
              write_str(user,mess);
              continue;
              }
             diff=tm-t_ustr.rawtime;
             sprintf(mess,"%-18s last on %s ago",t_ustr.say_name,converttime((long)(diff/60)));
             write_str(user,mess);
             diff=0; t_ustr.rawtime=0;
             }
             else {
              sprintf(mess,"%-18s is online right NOW!",expired[i]);
              write_str(user,mess);
             }
            } /* end of else */
           } /* end of for */
        i=0;
        write_str(user,"+-----------------------------------------------------------+");
        return;
        }
        
else if (((strlen(inpstr) < 3) && (strlen(inpstr) > 0)) || (strlen(inpstr) > NAME_LEN))
       {
        write_str(user,"Name length invalid.");
        return;
       }
else strtolower(inpstr);

          if ((remove_exem_data(inpstr))==1) {
           write_str(user,"User un-exempted.");
           return;
           }

 i=0;

     for (i=0;i<NUM_EXPIRES;++i) {
      if (!strcmp(expired[i],"name")) {
        if (!read_user(inpstr)) {
          write_str(user,NO_USER_STR);
          return;
         }
        strcpy(expired[i],t_ustr.say_name);
        write_str(user,"User exempted.");
        write_exem_data();
        i=0;
        return;
        }
      }

  i=0;

  write_str(user,"Max users are exempted. Un-exempt someone first.");
  return;
      
 }

else if (!strcmp(n_option,"-n")) {
   nuke_user=1;
   remove_first(inpstr);

   if (!strlen(inpstr)) goto SHOW;

    if (((strlen(inpstr) < 3) && (strlen(inpstr) > 0)) || (strlen(inpstr) > NAME_LEN)) 
       {
        write_str(user,"Name length invalid.");
        return;
       }
   strtolower(inpstr);
   }

 else {
      write_str(user,"Option not understood.");
      return;
      }

 SHOW:
 sprintf(t_mess,"%s",USERDIR);
 strncpy(filerid,t_mess,FILE_NAME_LEN);
 
 num=0;
 dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    return;
   }
    
  time(&tm);       
  strcpy(now_date,ctime(&tm));
  sprintf(t_mess,"Today is  %s",now_date);
  write_str(user,t_mess);

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
     if (!strcmp(small_buff,inpstr)) {
        write_str(user,"Excepted user found.. skipping.");
        continue;
        }       
     for (i=0;i<NUM_EXPIRES;++i) {
        strcpy(tempname,expired[i]);
        strtolower(tempname);
        if (!strcmp(small_buff,tempname)) {
            write_str(user,"Excepted user found.. skipping.");
            found=1; break;
            }
       }

    if (found) { found=0; continue; }

    if (small_buff[0]=='.') continue;
    read_user(small_buff);     /* Read user's profile */
      if (!strcmp(t_ustr.last_site,"127.0.0.1")) {
          t_ustr.last_site[0]=0;
          continue;
          }
    /* Wizards get a month extra before expiration, if set */
    if (EXPIRE_EXEMPT > -1) {
      if (t_ustr.super >= EXPIRE_EXEMPT) add=30;
      else add=0;
      }

 /* difference between user last login and now AND over time if goes here */
    diff  = tm-t_ustr.rawtime;
    limit = 3600*24*(DAYS_OFF+add);

    if (diff >= limit)
        {
           if (nuke_user!=1) {
            sprintf(mess,"%-18s on %s ago from %s",t_ustr.say_name,converttime((long)(diff/60)),t_ustr.last_site);
            write_str(user,mess);
            num++;
            }
            t_ustr.last_site[0]=0;
            t_ustr.rawtime=0;
             if (nuke_user==1) {
                nuke(user,small_buff,1);
                num++;
                system_stats.tot_expired++;
                }
        } 
   t_ustr.last_site[0]=0;
   t_ustr.rawtime=0;
   }       /* End of while */
 
 t_ustr.last_site[0]=0;
 t_ustr.rawtime=0;
 if (num>0) write_str(user,"");
 if (nuke_user==1) {
  sprintf(mess,"did a user expire, %d user%s %s nuked.",num,num == 1 ? "" : "s", num == 1 ? "was" : "were");
  btell(user,mess);
  sprintf(mess,"Nuked %d user%s",num,num == 1 ? "" : "s");
  }
 else {
  sprintf(mess,"Displayed %d user%s",num,num == 1 ? "" : "s");
  }
 write_str(user,mess);
 nuke_user=0;
 i=0;

 (void) closedir(dirp);

}


/* Auto expire users at midnight */
void auto_expire(void)
{
int i=0,num=0,warns=0,sendmail=0,nosubject=0;
int found=0;
int add=0;
unsigned long diff=0;
unsigned long limit=0;
unsigned long to_go=0;
char small_buff[64];
char tempname[NAME_LEN+1];
char filerid[FILE_NAME_LEN];
char filename[FILE_NAME_LEN];
char line[301];
time_t tm;
struct dirent *dp;
DIR  *dirp;
FILE *fp;
FILE *pp;

/* check to see if we need to do anything */
if (autoexpire==0) return;

 sprintf(t_mess,"%s",USERDIR);
 strncpy(filerid,t_mess,FILE_NAME_LEN);
 
 num=0;
 dirp=opendir((char *)filerid);
  
 if (dirp == NULL) return;

 time(&tm);       

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
    for (i=0;i<NUM_EXPIRES;++i) {
        strcpy(tempname,expired[i]);
        strtolower(tempname);
        if (!strcmp(small_buff,tempname)) {
            found=1; break;
            }
       }

    if (found) { found=0; continue; }

    if (small_buff[0]=='.') continue;
    read_user(small_buff);     /* Read user's profile */
      if (!strcmp(t_ustr.last_site,"127.0.0.1")) {
          t_ustr.last_site[0]=0;
          continue;
          }
 /* Wizards get a month extra before expiration, if set */
    if (EXPIRE_EXEMPT > -1) {
      if (t_ustr.super >= EXPIRE_EXEMPT) add=30;
      else add=0;
     }

 /* difference between user last login and now AND over time if goes here */
    diff  = tm-t_ustr.rawtime;
    limit = 3600*24*(DAYS_OFF+add);

    if ((diff >= limit) && ((autoexpire==2) || (autoexpire==3)))
        {
            t_ustr.last_site[0]=0;
            t_ustr.rawtime=0;
            remove_exem_data(small_buff);
            remove_user(small_buff);
            /* nuke(user,small_buff,1); */
            system_stats.tot_expired++;
            num++;
        }  /* end of if */
    else {
	if ((TIME_TO_GO > 0) && ((autoexpire==1) || (autoexpire==2))) {
		/* user not at limit, but if we have a TIME_TO_GO set */
		/* mail the user to tell them they have TIME_TO_GO to */
		/* logon before their account gets nuked	      */
	    to_go = limit - diff; /* seconds left til nuking */
/*	    if (to_go <= (TIME_TO_GO*86400)) {  MAIL EVERY DAY CODE */
if ((to_go <= (TIME_TO_GO*86400)) && (to_go >= ((TIME_TO_GO-1)*86400))) {
		/* user is over limit, if they have an email addy set */
		/* mail them about it, but only ONCE */
	    if ((strlen(t_ustr.email_addr) < 8) || !strcmp(t_ustr.email_addr,DEF_EMAIL)) {
		  t_ustr.last_site[0]=0;
		  t_ustr.rawtime=0;
		  continue;
	    } /* email not set correctly or at all */
	    else {
		if (!(fp=fopen(NUKEWARN,"r"))) {
		  sprintf(mess,"%s: Could not open nukewarn file to email to user %s!\n",get_time(0,0),t_ustr.say_name);
		  print_to_syslog(mess);
		  t_ustr.last_site[0]=0;
		  t_ustr.rawtime=0;
		  continue;
		  }

		if (strstr(MAILPROG,"sendmail")) {
		  sprintf(t_mess,"%s",MAILPROG);
		  sendmail=1;
		  }
		else {
  		  sprintf(t_mess,"%s %s",MAILPROG,t_ustr.email_addr);
		  if (strstr(MAILPROG,"-s"))
			nosubject=0;
		  else
			nosubject=1;
		  }
		strncpy(filename,t_mess,FILE_NAME_LEN);

		if (!(pp=popen(filename,"w")))
		  {
		   sprintf(mess,"%s : nukewarn message cannot be written to mail program!\n", syserror);
		   print_to_syslog(mess);
		   fclose(fp);
		   continue;
		  }    
  
		if (sendmail) {
		fprintf(pp,"From: %s <%s>\n",SYSTEM_NAME,SYSTEM_EMAIL);
		fprintf(pp,"To: %s <%s>\n",t_ustr.say_name,t_ustr.email_addr);
		fprintf(pp,"Subject: Your account on %s\n\n",SYSTEM_NAME);
		}  
		else if (nosubject) {
		fprintf(pp,"Your account on %s\n",SYSTEM_NAME);
		}
   
		fgets(line,300,fp);
		strcpy(line,check_var(line,SYS_VAR,SYSTEM_NAME));
		strcpy(line,check_var(line,USER_VAR,t_ustr.say_name));
		strcpy(line,check_var(line,"%var1%",itoa(TIME_TO_GO)));

		while (!feof(fp)) {
		   fputs(line,pp);
		   fgets(line,300,fp);
		   strcpy(line,check_var(line,SYS_VAR,SYSTEM_NAME));
		   strcpy(line,check_var(line,USER_VAR,t_ustr.say_name));
		   strcpy(line,check_var(line,"%var1%",itoa(TIME_TO_GO)));
		  } /* end of while */
		fclose(fp);

		fputs("\n",pp);
		fputs(EXT_MAIL1,pp);
		fputs(".\n",pp);
		pclose(pp);

		sprintf(mess,"%s: Sent out nukewarn to %s at %s\n",get_time(0,0),t_ustr.say_name,t_ustr.email_addr);
		print_to_syslog(mess);
		warns++;
	    } /* end of else email address ok */
	    } /* end of if over limit */
	  } /* end of main if */
	} /* end of else */
   t_ustr.last_site[0]=0;
   t_ustr.rawtime=0;
   }       /* End of while */
 
 t_ustr.last_site[0]=0;
 t_ustr.rawtime=0;

  sprintf(mess,"%s Talker user expire: %d user%s %s nuked, %d %s warned",STAFF_PREFIX,num,num == 1 ? "" : "s", num == 1 ? "was" : "were", warns, warns == 1 ? "was" : "were");
  writeall_str(mess, WIZ_ONLY, -1, 0, -1, BOLD, NONE, 0);
 num=0;
 warns=0;
 i=0;
}

/* Ban a name from logging in or being created */
void banname(int user, char *inpstr)
{
int i=0;
char tempname[NAME_LEN+1];

if (!strlen(inpstr)) {
        write_str(user," Names banned from use");
        write_str(user,"+----------------------------+");
         for (i=0;i<NUM_NAMEBANS;++i) {
            if (!strcmp(nbanned[i],"name"))
             write_str(user,"< not in use >");
            else 
             write_str(user,nbanned[i]);
           }
        i=0;
        write_str(user,"+----------------------------+");
        return;
        }
        
else if (((strlen(inpstr) < 3) && (strlen(inpstr) > 0)) || (strlen(inpstr) > NAME_LEN))
       {
        write_str(user,"Name length invalid.");
        return;
       }
else strtolower(inpstr);

if (!strcmp(inpstr,ROOT_ID)) {
   write_str(user,"Yeah, right!");
   return;
   }

     for (i=0;i<NUM_NAMEBANS;++i) {
         strcpy(tempname,nbanned[i]);
         strtolower(tempname);
       if (!strcmp(tempname,inpstr)) {
          strcpy(nbanned[i],"name");
          write_str(user,"Name un-banned.");
          write_nban_data();
          i=0;
          return;
          }
       }

 i=0;

     for (i=0;i<NUM_NAMEBANS;++i) {
      if (!strcmp(nbanned[i],"name")) {
        strcpy(nbanned[i],inpstr);
        write_str(user,"Name banned from use.");
        write_nban_data();
        i=0;
        return;
        }
      }

  i=0;

  write_str(user,"Max number of names are banned. Un-ban one first.");
  return;
      
}


/*----------------------------------------------------*/
/* Create a new player with defaults settings         */
/*----------------------------------------------------*/
void player_create(int user, char *inpstr)
{
int i=0;
int f=0;
int level=0;
char newname[ARR_SIZE];
char lowername[ARR_SIZE];
char newpass[ARR_SIZE];
char levelstr[ARR_SIZE];
time_t tm;

if (!strlen(inpstr)) {
   write_str(user,"Syntax: .pcreate <user_name> <level> <password>");
   return;
  }

sscanf(inpstr,"%s ",newname);

 if (newname[0]<32 || strlen(newname)< 3) 
    {
     write_str(user,"Invalid name given [must be at least 3 letters].");  
     return;
    }

 if (strstr(newname,"^")) {
    write_str(user,"Name cannot have color or hilites in it.");
    return;
    }

  if (strlen(newname)>NAME_LEN-1) 
    {
     write_str(user,"Name too long");  
     return;
    }

	/* see if only letters in login */
     for (f=0; f<strlen(newname); ++f) 
       {
         if (!isalpha((int)newname[f]) || newname[f]<'A' || newname[f] >'z') 
           {
	     write_str(user,"Name can only contain letters.");
	     return;
	   }
       }
        

  /*----------------------------------------------------------------*/
  /* copy capitalized name to a temp array and convert to lowercase */
  /*----------------------------------------------------------------*/
  
  strcpy(lowername,newname);
  strtolower(lowername);
 
if (check_for_user(lowername))
    {
      write_str(user,"Sorry, that name is already used.");
      return;
    }

/* Check for level input string */
remove_first(inpstr);

if (!strlen(inpstr)) {
  write_str(user,"Missing required field(s).");
   write_str(user,"Syntax: .pcreate <user_name> <level> [<password>]");
  return;
  }

sscanf(inpstr,"%s ",levelstr);

if (strlen(levelstr) > 1) {
  write_str(user,"Level number length cannot be greater than 1");
  return;
  }

if (!isdigit((int)levelstr[0])) {
  write_str(user,"Level given is not a number.");
  return;
  }
level=atoi(levelstr);

if (level > MAX_LEVEL) {
  write_str(user,"Level does not exist.");
  return;
  }

if (PROMOTE_TO_ABOVE)
  {
  }
else
  {
    if ((level > ustr[user].tempsuper) && strcmp(ustr[user].name,ROOT_ID)) {
     write_str(user,"You can't make a player with a level that high.");
     return;
    }
  }

if (PROMOTE_TO_SAME)
  {
    if ((level > ustr[user].tempsuper) && strcmp(ustr[user].name,ROOT_ID)) {
     write_str(user,"You can't make a player with a level that high.");
     return;
    }
  }
else
  {
    if ((level >= ustr[user].tempsuper) && strcmp(ustr[user].name,ROOT_ID)) {
     write_str(user,"You can't make a player with a level that high.");
     return;
    }
  }


/* Check for password */
remove_first(inpstr);

sscanf(inpstr,"%s",newpass);

 if (newpass[0]<32 || strlen(newpass)< 3) 
    {
     write_str(user,"Invalid password given [must be at least 3 letters].");  
     return;
    }

 if (strstr(newpass,"^")) {
    write_str(user,"Password cannot have color or hilites in it.");
    return;
    }
        
  if (strlen(newpass)>NAME_LEN-1) 
    {
     write_str(user,"Password too long");  
     return;
    }

  /*-------------------------------------------------------------*/
  /* convert password to lowercase and encrypt the password      */
  /*-------------------------------------------------------------*/
  
  strtolower(newpass);  
 
 if (!strcmp(lowername, newpass))
    {
        write_str(user,"Password cannot be the login name."); 
        return;  
    }
  
  st_crypt(newpass);                                   
  strcpy(t_ustr.password,newpass);   

/* Set new structures */
time(&tm);
   strcpy(t_ustr.name,       lowername);
   strcpy(t_ustr.say_name,   newname);

   strcpy(t_ustr.email_addr, DEF_EMAIL);
   strcpy(t_ustr.desc,       "was just created");
   strcpy(t_ustr.sex,        DEF_GENDER);
   strcpy(t_ustr.init_date,  ctime(&tm));
   t_ustr.init_date[24]=0;
   strcpy(t_ustr.last_date,  ctime(&tm));
   t_ustr.last_date[24]=0;
   strcpy(t_ustr.init_site,    "127.0.0.1");
   strcpy(t_ustr.last_site,    "127.0.0.1");
   strcpy(t_ustr.last_name,    "localhost");
   strcpy(t_ustr.init_netname, "localhost");
   strcpy(t_ustr.succ,       DEF_SUCC);
   strcpy(t_ustr.fail,       DEF_FAIL);
   strcpy(t_ustr.entermsg,   DEF_ENTER);
   strcpy(t_ustr.exitmsg,    DEF_EXIT);
   strcpy(t_ustr.homepage,   DEF_URL);
   strcpy(t_ustr.webpic,     DEF_PICURL);
   strcpy(t_ustr.home_room, astr[new_room].name);
   strcpy(t_ustr.creation,  ctime(&tm));
   t_ustr.creation[24]=0;
   t_ustr.rawtime   = tm;

  if (level > 0)
   t_ustr.promote   = 1;
  else
   t_ustr.promote   = 0;

   for (i=0;i<MAX_AREAS;i++)
     {
      t_ustr.security[i]='N';
     }
   i=0;
   for (i=0; i<MAX_ALERT; i++)
     {
      t_ustr.friends[i][0]=0;
     }
   i=0;
   for (i=0; i<MAX_GAG; i++)
     {
      t_ustr.gagged[i][0]=0;
     }
   for (i=0; i<MAX_GRAVOKES; i++)
     {
      t_ustr.revokes[i][0]=0;
     }
   for (i=0; i<NUM_LINES; i++)
     {
      t_ustr.conv[i][0]=0;
     }

if (level > 0)
 t_ustr.super=            level;
else 
 t_ustr.super=            0;

t_ustr.area=             new_room;
t_ustr.shout=            1;
t_ustr.vis=              1;
t_ustr.locked=           0;
t_ustr.suspended=        0;
t_ustr.monitor=          0;
t_ustr.rows=             24;
t_ustr.cols=             256;
t_ustr.car_return=       1;
t_ustr.abbrs =           1;
t_ustr.times_on =        0;
t_ustr.white_space =     1;
t_ustr.aver =            0;
t_ustr.totl =            0;
t_ustr.autor =           0;
t_ustr.autof =           0;
t_ustr.automsgs =        0;
t_ustr.gagcomm =         0;
t_ustr.semail =          0;
t_ustr.quote =           1;
t_ustr.hilite =          1;
t_ustr.new_mail =        0;
t_ustr.color =           COLOR_DEFAULT;
t_ustr.passhid =         0;
t_ustr.pbreak =          0;
t_ustr.numcoms =         0;
t_ustr.mail_num =        0;
t_ustr.numbering =       0;
t_ustr.friend_num =      0;
t_ustr.revokes_num =     0;
t_ustr.gag_num =         0;
t_ustr.nerf_kills =      0;
t_ustr.nerf_killed =     0;
t_ustr.muz_time =        0;
t_ustr.xco_time =        0;
t_ustr.gag_time =        0;
t_ustr.frog =            0;
t_ustr.frog_time =       0;
t_ustr.anchor =          0;
t_ustr.anchor_time =     0;
t_ustr.beeps =           0;
t_ustr.mail_warn =       0;
t_ustr.ttt_kills =       0;
t_ustr.ttt_killed =      0;
t_ustr.ttt_board =       0;
t_ustr.ttt_opponent =   -3;
t_ustr.ttt_playing =     0;
t_ustr.hang_wins =       0;
t_ustr.hang_losses =     0;
t_ustr.hang_stage =      -1;   
t_ustr.hang_word[0] =    '\0';
t_ustr.hang_word_show[0]='\0';
t_ustr.hang_guess[0] =   '\0';
strcpy(t_ustr.icq,	DEF_ICQ); 
strcpy(t_ustr.miscstr1,	"NA");
strcpy(t_ustr.miscstr2, "NA");
strcpy(t_ustr.miscstr3, "NA");
strcpy(t_ustr.miscstr4, "NA");
t_ustr.pause_login   = 1;
t_ustr.miscnum2      = 0;
t_ustr.miscnum3      = 0;
t_ustr.miscnum4      = 0;
t_ustr.miscnum5      = 0;

/* code copied right from initabbrs */
f=0;
i=0;
	for (i=0;i<NUM_ABBRS;++i)
	{
		t_ustr.custAbbrs[i].abbr[0] = 0;
		t_ustr.custAbbrs[i].com[0] = 0;
	}

        i=0;

        for (i=0;i<NUM_ABBRS;++i)
        {

         REDO:
          if (strlen(sys[f].cabbr) > 0) {
            strcpy(t_ustr.custAbbrs[i].com,sys[f].command);
            strcpy(t_ustr.custAbbrs[i].abbr,sys[f].cabbr);
            f++;
           }
          else {
            f++;
            goto REDO;
           }

        }	

i=0;

 for (i=0; i<NUM_MACROS; i++) {
  t_ustr.Macros[i].name[0]=0;
  t_ustr.Macros[i].body[0]=0;
 }

listen_all(-1);

/* write out data to new file */
write_user(lowername);

 write_str(user,"User created.");

}

/*----------------------------------------*/
/* REVOKE command from user, rank, or all */
/*----------------------------------------*/
void revoke_com(int user, char *inpstr)
{
int i=0,found=0,num=0,a=0,u,type=0;
int was_granted=-1;
char name[NAME_LEN+1];
char command[81];
char other_user[81];
char small_buff[64];
char filerid[FILE_NAME_LEN];
struct dirent *dp;
DIR  *dirp;

inpstr[80]=0;

sscanf(inpstr,"%s ",command);

if (!strlen(inpstr)) {
  write_str(user,"No command specified!");
  return;
  }

strtolower(command);

if (!strcmp(command,"-r")) {
if (strcmp(ustr[user].name,ROOT_ID)) return;

  sprintf(t_mess,"%s",USERDIR);

  strncpy(filerid,t_mess,FILE_NAME_LEN);
 
  dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    a=0;
    return;
   }

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
       if (small_buff[0]=='.') continue;

    	a=0;
	read_user(small_buff);

        	for (a=0; a < MAX_GRAVOKES; ++a) {
		  t_ustr.revokes[a][0]=0;
          	} /* end of for */
		  t_ustr.revokes_num=0;
		  write_user(t_ustr.name);

	 small_buff[0]=0; continue;
    } /* end of directory while */
	(void) closedir(dirp);

a=0;
write_str(user,"Reset.");
return;
} /* end of if root option -r */

/* Check for list option and list commands user cant use */
if (!strcmp(command,"-l")) {
   remove_first(inpstr);
   if (!strlen(inpstr)) {
	write_str(user,"Whose revoke list do you want to see?");
	return;
	}
   if (!strcmp(inpstr,"all")) {
	listall_gravokes(user,0);
	return;
	}

   sscanf(inpstr,"%s",other_user);

	   if (!read_user(other_user)) {
		write_str(user,NO_USER_STR);
		return;
		}

    write_str(user,"Commands that this user can't use");
    write_str(user,"+---------------------------------+");

        	for (a=0; a < MAX_GRAVOKES; ++a) {
		if (!isrevoke(t_ustr.revokes[a])) continue;

		  	found=1;
			for (i=0; sys[i].jump_vector != -1 ;++i) {
			if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) break;
			}
		  write_str(user,sys[i].command);
		  i=0;
          	}
	if (!found) {
	  write_str(user,"None.");
	  }

    write_str(user,"+----------------------------------+");

   i=0; found=0; a=0;
   return;
  } /* end of list if */

/* Check if command really exists */
for (i=0; sys[i].jump_vector != -1 ;++i) {
   if (!strcmp(sys[i].command,command)) {
     found=1;
     break;
     } /* end of if */
   } /* end of for */

if (!found) {
  write_str(user,"Command does not exist!");
  found=0; i=0;
  return;
  } /* end of if */

found=0;
remove_first(inpstr);

if (!strlen(inpstr)) {
  write_str(user,"Who do you want to take it away from?");
  return;
  }

sscanf(inpstr,"%s",other_user);
strtolower(other_user);

/* Types */
/* -2        , all users */
/* -1        , by user   */
/* 0 or above, by rank   */

if (!strcmp(other_user,"all")) {
  type=-2;
  }
else {
  for (a=0;a<strlen(other_user);++a) {
     if (!isdigit((int)other_user[a])) { found=1; break; }
     }
  if (!found) {
     /* we're a true number, but are we a existing level? */
     other_user[4]=0; /* strip to 4 digit number to prevent buffer */
                      /* overrun to int max size */
     num=atoi(other_user);
     if (num > MAX_LEVEL) {
       write_str(user,"That level doesn't exist!");
       return;
       }
     if ((ustr[user].tempsuper <= num) && strcmp(ustr[user].name,ROOT_ID)) {
	  write_str(user,"You dont have that much power!");
	  found=0; a=0; num=0; i=0;
	  return;
	  }
	type=num;
    } /* end of sub-if */
  else {
	type=-1;
    } /* end of sub-else */
  } /* end of main type else */

if (type==-1) {

	/* we're either a alpha-numeric mix or all alpha */
	/* either way, check as a username               */
	   if (!read_user(other_user)) {
		write_str(user,NO_USER_STR);
		return;
		}

	if ((ustr[user].tempsuper <= t_ustr.super) && strcmp(ustr[user].name,ROOT_ID)) {
	  write_str(user,"You dont have that much power!");
	  found=0; a=0; num=0; i=0;
	  return;
	  }

	/* Check to see if user has had this command revoked */
	/* if so, nothing changes */
        	for (a=0; a < MAX_GRAVOKES; ++a) {
		if (!isrevoke(t_ustr.revokes[a])) continue;
           	if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
		  write_str(user,"That user already has that command revoked!");
		  found=0; a=0; num=0; i=0;
		  return;
		  }
		}
	 	a=0;


	/* Check to see if user was granted the command    */
	/* if so take the grant away by blanking the entry */
        	for (a=0; a < MAX_GRAVOKES; ++a) {
		if (!isgrant(t_ustr.revokes[a])) continue;
           	if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
		  was_granted=a;
		  break;
		  }
		}


	/* Check to see if user has that command to begin with */
	/* if it wasn't granted                                */
	if (was_granted==-1) {
	 if (t_ustr.super < sys[i].su_com) {
	   sprintf(mess,"%s doesn't even have that command!",t_ustr.say_name);
	   write_str(user,mess);
	   found=0; a=0; num=0; i=0;
	   return;
	   }
	}

if (was_granted!=-1) {
/* only clear the grant entry to give normal permissions back */
t_ustr.revokes[a][0]=0;
t_ustr.revokes_num--;
	   strcpy(name,t_ustr.say_name);
	   write_user(other_user);
	   sprintf(mess,"%s has taken the %s command BACK from you",ustr[user].say_name,sys[i].command);
	   if ((u=get_user_num_exact(other_user,user)) != -1) {
		ustr[u].revokes_num--;
		ustr[u].revokes[a][0] = 0;
		write_str(u,mess);
		write_str(user,mess);
	     } /* end of if user online */
		else {
			sprintf(t_mess,"%s %s",other_user,mess);
			send_mail(user,t_mess,1);
		   }
			sprintf(t_mess,"%s You took the %s command BACK from %s",ustr[user].name,sys[i].command,name);
			send_mail(user,t_mess,1);
			goto END;
} /* end of if was_granted */

	if (t_ustr.revokes_num < MAX_GRAVOKES) {
	        /* Add to a free slot */
        	for (a=0; a < MAX_GRAVOKES; ++a) {
           	if (!strlen(t_ustr.revokes[a])) break;
          	}
	   t_ustr.revokes_num++;
	   sprintf(t_ustr.revokes[a],"%d -",sys[i].jump_vector);
	   strcpy(name,t_ustr.say_name);
	   write_user(other_user);
	   sprintf(mess,"%s has taken the %s command from you",ustr[user].say_name,sys[i].command);
	   if ((u=get_user_num_exact(other_user,user)) != -1) {
		ustr[u].revokes_num++;
		sprintf(ustr[u].revokes[a],"%d -",sys[i].jump_vector);
		write_str(u,mess);
	     } /* end of if user online */
		else {
			sprintf(t_mess,"%s %s",other_user,mess);
			send_mail(user,t_mess,1);
		   }
			sprintf(t_mess,"%s You took the %s command from %s",ustr[user].name,sys[i].command,name);
			send_mail(user,t_mess,1);
	  } /* end of num revokes if */
	  else {
	  sprintf(mess,"You can't revoke any more commands from %s until you revoke one that was granted",t_ustr.say_name);
	  write_str(user,mess);
	  found=0; a=0; num=0; i=0;
	  return;
	  }
END:
	/* wiztell here */
	if (was_granted!=-1)
	sprintf(mess,"has taken the %s command BACK from %s",sys[i].command,name);
	else
	sprintf(mess,"has taken the %s command away from %s",sys[i].command,name);
	btell(user,mess);
	found=0; a=0; num=0; i=0;
	return;
  } /* end of type -1 */
else if (type==-2) {
	/* ALL USERS */
  sprintf(t_mess,"%s",USERDIR);

  strncpy(filerid,t_mess,FILE_NAME_LEN);
 
  dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    found=0; a=0; num=0; i=0;
    return;
   }

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
       if (small_buff[0]=='.') continue;

    	found=0; a=0; num=0; was_granted=-1;
	read_user(small_buff);

	/* Check to see if we want to clear it or take it */
	
	/* cant take it from own level or above */
	if ((ustr[user].tempsuper <= t_ustr.super) && strcmp(ustr[user].name,ROOT_ID)) continue;

	/* does the user already have this command revoked */
        	for (a=0; a < MAX_GRAVOKES; ++a) {
		if (!isrevoke(t_ustr.revokes[a])) continue;
           	if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
		  found=1; break;
		  }
		}
		if (found==1) continue;
	 	a=0;

	/* Check to see if user was granted the command    */
	/* if so take the grant away by blanking the entry */
        	for (a=0; a < MAX_GRAVOKES; ++a) {
		if (!isgrant(t_ustr.revokes[a])) continue;
           	if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
		  was_granted=a;
			break;
		  }
		}

	/* Check to see if user has that command to begin with */
	if (was_granted==-1) {
	 if (t_ustr.super < sys[i].su_com) {
	   sprintf(mess,"%s doesn't even have that command!",t_ustr.say_name);
	   write_str(user,mess);
	   continue;
	   }
	}

if (was_granted!=-1) {
/* only clear the grant entry to give normal permissions back */
t_ustr.revokes[a][0]=0;
t_ustr.revokes_num--;
		  write_user(t_ustr.name);
		  sprintf(mess,"%s has taken the %s command BACK from you and everyone",ustr[user].say_name,sys[i].command);
	   	if ((u=get_user_num_exact(small_buff,user)) != -1) {
			ustr[u].revokes_num--;
			ustr[u].revokes[a][0] = 0;
		  write_str(u,mess);
	     	  } /* end of if user online */		  
		else {
			sprintf(t_mess,"%s %s",small_buff,mess);
			send_mail(user,t_mess,1);
		   }
	 small_buff[0]=0; continue;
} /* end of if was_granted */
	a=0;

	if (t_ustr.revokes_num < MAX_GRAVOKES) {
	        /* Add to a free slot */
        	for (a=0; a < MAX_GRAVOKES; ++a) {
           	if (!strlen(t_ustr.revokes[a])) break;
          	}
	   t_ustr.revokes_num++;
	   sprintf(t_ustr.revokes[a],"%d -",sys[i].jump_vector);
	   write_user(t_ustr.name);
		sprintf(mess,"%s has taken the %s command from you and everyone",ustr[user].say_name,sys[i].command);
	   if ((u=get_user_num_exact(small_buff,user)) != -1) {
		ustr[u].revokes_num++;
		sprintf(ustr[u].revokes[a],"%d -",sys[i].jump_vector);
		write_str(u,mess);
	     } /* end of if user online */
		else {
			sprintf(t_mess,"%s %s",small_buff,mess);
			send_mail(user,t_mess,1);
		   }
	  } /* end of num revokes if */
	  else {
	  sprintf(mess,"You can't revoke any more commands from %s until you revoke one that was granted.",t_ustr.say_name);
	  write_str(user,mess);
	  small_buff[0]=0; continue;
	  }

    } /* end of directory while */
	(void) closedir(dirp);
	/* wiztell here */
	sprintf(mess,"has taken the %s command away from everyone",sys[i].command);
	btell(user,mess);
	sprintf(t_mess,"%s You took the %s command from everyone",ustr[user].name,sys[i].command);
	send_mail(user,t_mess,0);
  } /* end of type -2 */
else {

  /* USERS OF RANK <TYPE> */
  sprintf(t_mess,"%s",USERDIR);

  strncpy(filerid,t_mess,FILE_NAME_LEN);
 
  dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    found=0; a=0; num=0; i=0;
    return;
   }

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
       if (small_buff[0]=='.') continue;

    	found=0; a=0; num=0;
	read_user(small_buff);

	/* Is user of the rank we're taking or giving back to? */
	if (t_ustr.super != type) continue;

	/* cant take it from own level or above */
	if ((ustr[user].tempsuper <= t_ustr.super) && strcmp(ustr[user].name,ROOT_ID)) continue;

	/* does the user already have this command revoked */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!isrevoke(t_ustr.revokes[a])) continue;
                if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
                  found=1; break;
                  }
                }
                if (found==1) continue;
                a=0;

        /* Check to see if user was granted the command    */
        /* if so take the grant away by blanking the entry */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!isgrant(t_ustr.revokes[a])) continue;
                if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
                  was_granted=a;
			break;
                  }
                }

        /* Check to see if user has that command to begin with */
        if (was_granted==-1) {
         if (t_ustr.super < sys[i].su_com) {
           sprintf(mess,"%s doesn't even have that command!",t_ustr.say_name);
           write_str(user,mess);
           continue;
           }
        }

if (was_granted!=-1) {
/* only clear the grant entry to give normal permissions back */ 
t_ustr.revokes[a][0]=0;
t_ustr.revokes_num--;
                  write_user(t_ustr.name);
		  sprintf(mess,"%s has taken the %s command BACK from you and all level %d's",ustr[user].say_name,sys[i].command,type);
	   	if ((u=get_user_num_exact(small_buff,user)) != -1) {
			ustr[u].revokes_num--;
			ustr[u].revokes[a][0] = 0;
		  write_str(u,mess);
	     	  } /* end of if user online */		  
		else {
			sprintf(t_mess,"%s %s",small_buff,mess);
			send_mail(user,t_mess,1);
		   }
	 small_buff[0]=0; continue;
} /* end of if was_granted */
	a=0;

	if (t_ustr.revokes_num < MAX_GRAVOKES) {
	        /* Add to a free slot */
        	for (a=0; a < MAX_GRAVOKES; ++a) {
           	if (!strlen(t_ustr.revokes[a])) break;
          	}
	   t_ustr.revokes_num++;
	   sprintf(t_ustr.revokes[a],"%d -",sys[i].jump_vector);
	   write_user(t_ustr.name);
		sprintf(mess,"%s has taken the %s command from you and all level %d's",ustr[user].say_name,sys[i].command,type);
	   if ((u=get_user_num_exact(small_buff,user)) != -1) {
		ustr[u].revokes_num++;
		sprintf(ustr[u].revokes[a],"%d -",sys[i].jump_vector);
		write_str(u,mess);
	     } /* end of if user online */
		else {
			sprintf(t_mess,"%s %s",small_buff,mess);
			send_mail(user,t_mess,1);
		   }
	  } /* end of num revokes if */
	  else {
	  sprintf(mess,"You can't revoke any more commands from %s until you revoke one that was granted",t_ustr.say_name);
	  write_str(user,mess);
	  small_buff[0]=0; continue;
	  }

    } /* end of directory while */
	(void) closedir(dirp);
	/* wiztell here */
	sprintf(mess,"has taken the %s command away from level %d's",sys[i].command,type);
	btell(user,mess);
	sprintf(t_mess,"%s You took the %s command from level %d's",ustr[user].name,sys[i].command,type);
	send_mail(user,t_mess,0);

  } /* end of type rank */

found=0; a=0; num=0; i=0;
}


/*----------------------------------------*/
/* GRANT command from user, rank, or all  */
/*----------------------------------------*/
void grant_com(int user, char *inpstr)
{
int i=0,found=0,num=0,a=0,u,type=0,level=0;
int was_revoked=-1;
char name[NAME_LEN+1];
char opt[81];
char command[81];
char other_user[81];
char small_buff[64];
char filerid[FILE_NAME_LEN];
struct dirent *dp;
DIR  *dirp;

inpstr[80]=0;

sscanf(inpstr,"%s ",command);

if (!strlen(inpstr)) {
  write_str(user,"No command specified!");
  return;
  }

strtolower(command);

if (!strcmp(command,"-r")) {
if (strcmp(ustr[user].name,ROOT_ID)) return;

  sprintf(t_mess,"%s",USERDIR);

  strncpy(filerid,t_mess,FILE_NAME_LEN);
 
  dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    a=0;
    return;
   }

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
       if (small_buff[0]=='.') continue;

    	a=0;
	read_user(small_buff);

        	for (a=0; a < MAX_GRAVOKES; ++a) {
		  t_ustr.revokes[a][0]=0;
          	} /* end of for */
		  t_ustr.revokes_num=0;
		  write_user(t_ustr.name);

	 small_buff[0]=0; continue;
    } /* end of directory while */
	(void) closedir(dirp);

a=0;
write_str(user,"Reset.");
return;
} /* end of if root option -r */

/* Check for list option and list commands user cant use */
if (!strcmp(command,"-l")) {
   remove_first(inpstr);
   if (!strlen(inpstr)) {
	write_str(user,"Whose grant list do you want to see?");
	return;
	}
   if (!strcmp(inpstr,"all")) {
	listall_gravokes(user,1);
	return;
	}

   sscanf(inpstr,"%s",other_user);

	   if (!read_user(other_user)) {
		write_str(user,NO_USER_STR);
		return;
		}

    write_str(user,"Commands that this user is granted");
    write_str(user,"+----------------------------------+");

        	for (a=0; a < MAX_GRAVOKES; ++a) {
		if (!isgrant(t_ustr.revokes[a])) continue;

		  	found=1;
			for (i=0; sys[i].jump_vector != -1 ;++i) {
			if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) break;
			}
		  write_str(user,sys[i].command);
		  i=0;
          	}
	if (!found) {
	  write_str(user,"None.");
	  }

    write_str(user,"+----------------------------------+");

   i=0; found=0; a=0;
   return;
  } /* end of list if */

/* Check if command really exists */
for (i=0; sys[i].jump_vector != -1 ;++i) {
   if (!strcmp(sys[i].command,command)) {
     found=1;
     break;
     } /* end of if */
   } /* end of for */

if (!found) {
  write_str(user,"Command does not exist!");
  found=0; i=0;
  return;
  } /* end of if */

found=0;
remove_first(inpstr);

if (ustr[user].tempsuper <= sys[i].su_com) {  
	write_str(user,"You dont have that much power to give that command!");
	return;
  }

if (!strlen(inpstr)) {
  write_str(user,"Who do you want to give it to?");
  return;
  }

sscanf(inpstr,"%s",other_user);
strtolower(other_user);

/* Types */
/* -2        , all users */
/* -1        , by user   */
/* 0 or above, by rank   */

if (!strcmp(other_user,"all")) {
  remove_first(inpstr);
  if (strlen(inpstr)) {
  sscanf(inpstr,"%s",opt);
  for (a=0;a<strlen(opt);++a) {
     if (!isdigit((int)opt[a])) { found=1; break; }
     }
  if (!found) {
     /* we're a true number, but are we a existing level? */
     opt[4]=0; /* strip to 4 digit number to prevent buffer */
                      /* overrun to int max size */
     num=atoi(opt);
     if (num > MAX_LEVEL) {
       write_str(user,"That level doesn't exist!");
       return;
       }
	level=num;
      }
  else {
       write_str(user,"That level doesn't exist!");
       return;
       } /* non-numbers in level field */
   } /* end of if strlen inpstr */
	else level=sys[i].su_com;

  type=-2;
  }
else {
  for (a=0;a<strlen(other_user);++a) {
     if (!isdigit((int)other_user[a])) { found=1; break; }
     }
  if (!found) {
     /* we're a true number, but are we a existing level? */
     other_user[4]=0; /* strip to 4 digit number to prevent buffer */
                      /* overrun to int max size */
     num=atoi(other_user);
     if (num > MAX_LEVEL) {
       write_str(user,"That level doesn't exist to affect!");
       return;
       }
     if ((ustr[user].tempsuper <= num) && strcmp(ustr[user].name,ROOT_ID)) {
	  write_str(user,"You dont have that much power to affect that level!");
	  found=0; a=0; num=0; i=0;
	  return;
	  }
	type=num;
	} /* end of sub-if */
  else {
	type=-1;
       } /* non-numbers in level field */

	num=0; found=0;

	/* now check if we can give out this much power */
  remove_first(inpstr);
  if (strlen(inpstr)) {
  sscanf(inpstr,"%s",opt);
  for (a=0;a<strlen(opt);++a) {
     if (!isdigit((int)opt[a])) { found=1; break; }
     }
  if (!found) {
     /* we're a true number, but are we a existing level? */
     opt[4]=0; /* strip to 4 digit number to prevent buffer */
                      /* overrun to int max size */
     num=atoi(opt);
     if (num > MAX_LEVEL) {
       write_str(user,"That level doesn't exist!");
       return;
       }
        level=num;
    } /* end of sub-if */
   else {
       write_str(user,"That level doesn't exist!");
       return;
       } /* end of sub-else */
    } /* end of if strlen inpstr */
    else level=sys[i].su_com;

  } /* end of main type else */

if (type==-1) {

	/* we're either a alpha-numeric mix or all alpha */
	/* either way, check as a username               */
	   if (!read_user(other_user)) {
		write_str(user,NO_USER_STR);
		return;
		}

	/* cant give from own level or above */
	if ((ustr[user].tempsuper <= t_ustr.super) && strcmp(ustr[user].name,ROOT_ID)) {
		write_str(user,"You dont have that much power!");
		found=0; a=0; num=0; i=0;
		return;
		}

	/* Check to see if user has had this command granted */
	/* if so, nothing changes */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!isgrant(t_ustr.revokes[a])) continue;
                if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
                  write_str(user,"That user already has that command granted to them!");
                  found=0; a=0; num=0; i=0;
                  return;
                  }
                }
                a=0;

        /* Check to see if user was revoked the command     */
        /* if so take the revoke away by blanking the entry */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!isrevoke(t_ustr.revokes[a])) continue;
                if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
                  was_revoked=a;
			break;
                  }
                }   

/* if command was revoked check to see if we're allowed to give it */
/* back based on our level against the commands level              */
if (was_revoked==-1) {
	/* can we give them this much access to that command */
	if (level > sys[i].su_com) {
		if (ustr[user].tempsuper <= level && strcmp(ustr[user].name,ROOT_ID)) {
		/* we cant give out access at or above our level */
		write_str(user,"You cant give that much power!");
		found=0; a=0; num=0; i=0;
		return;
		}
	} /* end of if level */
	else if (level < sys[i].su_com) {
	write_str(user,"You cant give out less access than the command normally gives!");
	found=0; a=0; num=0; i=0;
	return;	
	} /* end of else if level */
	else {
		if (t_ustr.super>=level) {
		/* user already has that much power, sheesh */
		write_str(user,"The user already has that much power!");
		found=0; a=0; num=0; i=0;
		return;		
		}
	} /* end of level else */
} /* end of if was_revoked */

if (was_revoked!=-1) {
/* only clear the revoke entry to give normal permissions back */
t_ustr.revokes[a][0]=0;
t_ustr.revokes_num--;
           strcpy(name,t_ustr.say_name);
           write_user(other_user);
           sprintf(mess,"%s has given the %s command BACK to you",ustr[user].say_name,sys[i].command);
           if ((u=get_user_num_exact(other_user,user)) != -1) {
                ustr[u].revokes_num--;
                ustr[u].revokes[a][0] = 0;
                write_str(u,mess);
             } /* end of if user online */
                else {
                        sprintf(t_mess,"%s %s",other_user,mess);
                        send_mail(user,t_mess,1);
                   }
                        sprintf(t_mess,"%s You gave the %s command BACK to %s",ustr[user].name,sys[i].command,name);   
                        send_mail(user,t_mess,1);
                        goto END;
} /* end of if was_revoked */

        if (t_ustr.revokes_num < MAX_GRAVOKES) {
                /* Add to a free slot */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!strlen(t_ustr.revokes[a])) break;
                }
           t_ustr.revokes_num++;
           sprintf(t_ustr.revokes[a],"%d + %d",sys[i].jump_vector,level);
           strcpy(name,t_ustr.say_name);
           write_user(other_user);
           sprintf(mess,"%s has give the %s command to you",ustr[user].say_name,sys[i].command);
           if ((u=get_user_num_exact(other_user,user)) != -1) {
                ustr[u].revokes_num++;
                sprintf(ustr[u].revokes[a],"%d + %d",sys[i].jump_vector,level);
                write_str(u,mess);
             } /* end of if user online */
                else {
                        sprintf(t_mess,"%s %s",other_user,mess);
                        send_mail(user,t_mess,1);
                   }
                        sprintf(t_mess,"%s You gave the %s command to %s",ustr[user].name,sys[i].command,name);
                        send_mail(user,t_mess,1);
          } /* end of num revokes if */
          else {
          sprintf(mess,"You can't grant any more commands to %s until you grant one that was revoked",t_ustr.say_name);
          write_str(user,mess);
          found=0; a=0; num=0; i=0;
          return;
          }
END:
        /* wiztell here */
        if (was_revoked!=-1)
        sprintf(mess,"has given the %s command BACK to %s",sys[i].command,name);
        else
        sprintf(mess,";has given the %s command to %s",sys[i].command,name);
        btell(user,mess);
        found=0; a=0; num=0; i=0;
        return;
  } /* end of type -1 */
else if (type==-2) {
	/* ALL USERS */
  sprintf(t_mess,"%s",USERDIR);

  strncpy(filerid,t_mess,FILE_NAME_LEN);
 
  dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    found=0; a=0; num=0; i=0;
    return;
   }

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
       if (small_buff[0]=='.') continue;

    	found=0; a=0; num=0; was_revoked=-1;
	read_user(small_buff);

	/* cant give from own level or above */
	if ((ustr[user].tempsuper <= t_ustr.super) && strcmp(ustr[user].name,ROOT_ID)) continue;

        /* Check to see if user has had this command granted */
        /* if so, nothing changes */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!isgrant(t_ustr.revokes[a])) continue; 
                if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
		  found=1; break;
                  }
                }   
		if (found==1) continue;
                a=0;

        /* Check to see if user was revoked the command     */
        /* if so take the revoke away by blanking the entry */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!isrevoke(t_ustr.revokes[a])) continue;
                if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
                  was_revoked=a;
			break;
                  }
                }

if (was_revoked==-1) {
        if (level > sys[i].su_com) {
                if (ustr[user].tempsuper <= level && strcmp(ustr[user].name,ROOT_ID)) {
                /* we cant give out access at or above our level */
		continue;
                }
        } /* end of if level */
        else if (level < sys[i].su_com) {
		/* dont give out less access than the command */
		/* normally has				      */
		continue;
        } /* end of else if level */
        else {
                if (t_ustr.super==level) {
                /* user already has that much power, sheesh */
		continue;
                }
        } /* end of level else */   
} /* end of if was_revoked */

if (was_revoked!=-1) {
/* only clear the grant entry to give normal permissions back */ 
t_ustr.revokes[a][0]=0;
t_ustr.revokes_num--;
                  write_user(t_ustr.name);
                  sprintf(mess,"%s has given you and everyone BACK the %s command",ustr[user].say_name,sys[i].command);
                if ((u=get_user_num_exact(small_buff,user)) != -1) {
                        ustr[u].revokes_num--;
                        ustr[u].revokes[a][0] = 0;
                  write_str(u,mess);
                  } /* end of if user online */
                else {
                        sprintf(t_mess,"%s %s",small_buff,mess);
                        send_mail(user,t_mess,1);
                   }
         small_buff[0]=0; continue;
} /* end of if was_revoked */
        a=0;

        if (t_ustr.revokes_num < MAX_GRAVOKES) {
                /* Add to a free slot */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!strlen(t_ustr.revokes[a])) break;
                }
           t_ustr.revokes_num++;
           sprintf(t_ustr.revokes[a],"%d + %d",sys[i].jump_vector,level);
           write_user(t_ustr.name);
                sprintf(mess,"%s has given you and everyone the %s command",ustr[user].say_name,sys[i].command);
           if ((u=get_user_num_exact(small_buff,user)) != -1) {
                ustr[u].revokes_num++;
                sprintf(ustr[u].revokes[a],"%d + %d",sys[i].jump_vector,level);
                write_str(u,mess);
             } /* end of if user online */
                else {
                        sprintf(t_mess,"%s %s",small_buff,mess);
                        send_mail(user,t_mess,1);
                   }
          } /* end of num revokes if */
          else {
          sprintf(mess,"You can't grant any more commands to %s until you grant one that was revoked.",t_ustr.say_name);
          write_str(user,mess);
	 small_buff[0]=0; continue;
	 }

    } /* end of directory while */
	(void) closedir(dirp);
	/* wiztell here */
	sprintf(mess,"has given the %s command back to everyone",sys[i].command);
	btell(user,mess);
	sprintf(t_mess,"%s You gave the %s command back to everyone",ustr[user].name,sys[i].command);
	send_mail(user,t_mess,0);
  } /* end of type -2 */
else {

  /* USERS OF RANK <TYPE> */
  sprintf(t_mess,"%s",USERDIR);

  strncpy(filerid,t_mess,FILE_NAME_LEN);
 
  dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    found=0; a=0; num=0; i=0;
    return;
   }

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
       if (small_buff[0]=='.') continue;

    	found=0; a=0; num=0; was_revoked=-1;
	read_user(small_buff);

	/* Is user of the rank we're taking or giving back to? */
	if (t_ustr.super != type) continue;

        /* Check to see if user has had this command granted */
        /* if so, nothing changes */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!isgrant(t_ustr.revokes[a])) continue;
                if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
                  found=1; break;
                  }
                }
                if (found==1) continue;
                a=0;

        /* Check to see if user was revoked the command     */
        /* if so take the revoke away by blanking the entry */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!isrevoke(t_ustr.revokes[a])) continue;
                if (strip_com(t_ustr.revokes[a]) == sys[i].jump_vector) {
                  was_revoked=a;
			break;
                  }
                }

if (was_revoked==-1) {
        if (level > sys[i].su_com) {
                if (ustr[user].tempsuper <= level && strcmp(ustr[user].name,ROOT_ID)) {
                /* we cant give out access at or above our level */
                continue;
                }
        } /* end of if level */
        else if (level < sys[i].su_com) {
                /* dont give out less access than the command */
                /* normally has                               */
                continue;
        } /* end of else if level */
        else {
                if (t_ustr.super==level) {
                /* user already has that much power, sheesh */
                continue;
                }
        } /* end of level else */
} /* end of if was_revoked */
                
if (was_revoked!=-1) {   
/* only clear the grant entry to give normal permissions back */
t_ustr.revokes[a][0]=0;
t_ustr.revokes_num--;
                  write_user(t_ustr.name);
                  sprintf(mess,"%s has given you and all level %d's BACK the %s command",ustr[user].say_name,type,sys[i].command);
                if ((u=get_user_num_exact(small_buff,user)) != -1) {
                        ustr[u].revokes_num--;
                        ustr[u].revokes[a][0] = 0;
                  write_str(u,mess);
                  } /* end of if user online */
                else {
                        sprintf(t_mess,"%s %s",small_buff,mess);
                        send_mail(user,t_mess,1);
                   }
         small_buff[0]=0; continue;
} /* end of if was_revoked */
        a=0;
        if (t_ustr.revokes_num < MAX_GRAVOKES) {
                /* Add to a free slot */
                for (a=0; a < MAX_GRAVOKES; ++a) {
                if (!strlen(t_ustr.revokes[a])) break;
                }
           t_ustr.revokes_num++;
           sprintf(t_ustr.revokes[a],"%d + %d",sys[i].jump_vector,level);
           write_user(t_ustr.name);
                sprintf(mess,"%s has given you and all level %d's the %s command with permissions of level %d",ustr[user].say_name,type,sys[i].command,level);
           if ((u=get_user_num_exact(small_buff,user)) != -1) {
                ustr[u].revokes_num++;
                sprintf(ustr[u].revokes[a],"%d + %d",sys[i].jump_vector,level);
                write_str(u,mess);
             } /* end of if user online */
                else {
                        sprintf(t_mess,"%s %s",small_buff,mess);
                        send_mail(user,t_mess,1);
                   }
          } /* end of num revokes if */
          else {
          sprintf(mess,"You can't grant any more commands to %s until you grant one that was revoked.",t_ustr.say_name);
          write_str(user,mess);
         small_buff[0]=0; continue;
         }      

    } /* end of directory while */
        (void) closedir(dirp);
        /* wiztell here */
        sprintf(mess,"has given the %s command back to level %d's with permissions of level %d",sys[i].command,type,level);
        btell(user,mess);
        sprintf(t_mess,"%s You gave the %s command back to level %d's with permissions of level %d",ustr[user].name,sys[i].command,type,level);   
	send_mail(user,t_mess,0);
  } /* end of type rank */

found=0; a=0; num=0; i=0;
}


/*----------------------------------------------------*/
/* Show to user various info specified on other users */
/*----------------------------------------------------*/
void clist(int user, char *inpstr)
{
int num;
int len=0;
int type=0;
int n_option=0,timenum,search=0;
char small_buff[64];
char option[7];
char timebuf[23];
char comment[ARR_SIZE];
char filerid[FILE_NAME_LEN];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
char line[ARR_SIZE];
char line2[ARR_SIZE];
time_t tm;
time_t tm_then;
struct dirent *dp;
FILE *fp;
FILE *fp2;
DIR  *dirp;
 
 if (!strlen(inpstr)) {
    write_str(user,"These are the topics you can search under..");
    write_str(user,"+-----------------------------------------+");
    write_hilite(user,"  rank    email    homepage    desc");
    write_hilite(user,"  bans    newbans");
    return;
    }

sscanf(inpstr,"%s ",option);
if (!strcmp(option,"rank")) {
    remove_first(inpstr);
    if (!strlen(inpstr)) {
        type=0;
        search=0;
        }
    else if ((strlen(inpstr)==1) && isdigit((int)inpstr[0])) {
        n_option=atoi(inpstr);
        type=0;
        search=1;
        }
    else { write_str(user,"Search string not valid.");  return; }
   }
else if (!strcmp(option,"email")) {
    remove_first(inpstr);
    if (!strlen(inpstr)) {
        type=1;
        search=0;
        }
    else if (strlen(inpstr)<=EMAIL_LENGTH) {
        type=1;
        search=1;
        }
    else { 
       sprintf(mess,"Search string too long. Max %d chars.",EMAIL_LENGTH);
       write_str(user,mess);
       return; 
       }
   }
else if (!strcmp(option,"homepage")) {
    remove_first(inpstr);
    if (!strlen(inpstr)) {
        type=2;
        search=0;
        }
    else if (strlen(inpstr)<=HOME_LEN) {
        type=2;
        search=1;
        }
    else { 
       sprintf(mess,"Search string too long. Max %d chars.",HOME_LEN);
       write_str(user,mess);
       return; 
       }
   }
else if (!strcmp(option,"desc")) {
    remove_first(inpstr);
    if (!strlen(inpstr)) {
        type=3;
        search=0;
        }
    else if (strlen(inpstr)<=DESC_LEN) {
        type=3;
        search=1;
        }
    else { 
       sprintf(mess,"Search string too long. Max %d chars.",DESC_LEN);
       write_str(user,mess);
       return; 
       }
   }
else if (!strcmp(option,"bans")) {
    remove_first(inpstr);
    if (!strlen(inpstr)) {
        write_str(user,"IP or hostname search string must be given");
        return;
        }
    else if (strlen(inpstr)<=FILE_NAME_LEN) {
        type=4;
        }
    else { 
       sprintf(mess,"Search string too long. Max %d chars.",FILE_NAME_LEN);
       write_str(user,mess);
       return; 
       }

/* Check for hostname search string */
 if (isalpha((int)inpstr[strlen(inpstr)-1])) {
     }       
/* Check for IP search string */
 else if (!isalpha((int)inpstr[strlen(inpstr)-1]) && strstr(inpstr,".")) {
     }
 else {
     write_str(user,"Invalid search string.");     
     return;
     }
   }
else if (!strcmp(option,"newbans")) {
    remove_first(inpstr);
    if (!strlen(inpstr)) {
        write_str(user,"IP or hostname search string must be given");
        return;
        }
    else if (strlen(inpstr)<=FILE_NAME_LEN) {
        type=5;
        }
    else { 
       sprintf(mess,"Search string too long. Max %d chars.",FILE_NAME_LEN);
       write_str(user,mess);
       return; 
       }

/* Check for hostname search string */
 if (isalpha((int)inpstr[strlen(inpstr)-1])) {
     }
/* Check for IP search string */
 else if (!isalpha((int)inpstr[strlen(inpstr)-1]) && strstr(inpstr,".")) {
     }
 else {
     write_str(user,"Invalid search string.");     
     return;
     }
   }

else {
    write_str(user,"Unknown option");
    write_str(user,"These are the topics you can search under..");
    write_str(user,"+-----------------------------------------+");
    write_hilite(user,"  rank    email    homepage    desc");
    write_hilite(user,"  bans    newbans");
    return;
   }

 if (type == 4)
  sprintf(t_mess,"%s",RESTRICT_DIR);
 else if (type == 5)
  sprintf(t_mess,"%s",RESTRICT_NEW_DIR);
 else
  sprintf(t_mess,"%s",USERDIR);

 strncpy(filerid,t_mess,FILE_NAME_LEN);
 
 num=0;
 dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    return;
   }

 time(&tm);

 strcpy(filename,get_temp_file());
 if (!(fp=fopen(filename,"w"))) {
     sprintf(mess,"%s Cant create file for paged clist listing!",get_time(0,0));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     (void) closedir(dirp);
     return;
     }

 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
    if ((type==4) || (type==5)) {
           len=strlen(small_buff);
       if ((small_buff[0]=='.') ||
           ( (small_buff[len-2]=='.') &&
             ((small_buff[len-1]=='c') ||
              (small_buff[len-1]=='r')) ) ) {
         small_buff[0]=0;
         len=0;
         continue;
      }
    }
    else {
     if (small_buff[0]=='.') continue;
     read_user(small_buff);     /* Read user's profile */
    }
       if (type==0) {
         if (search) {
            if (t_ustr.super==n_option) {
             sprintf(mess,"%-18s with rank  %d",t_ustr.say_name,t_ustr.super);
             fputs(mess,fp);
             fputs("\n",fp);
             t_ustr.super=0;
             num++;
             }
            else { t_ustr.super=0; continue; }
            }
         else {
            sprintf(mess,"%-18s with rank  %d",t_ustr.say_name,t_ustr.super);
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.super=0;
            num++;
           }
      }  /* end of type if */
    else if (type==1) {
       if (search) {
       strcpy(line2,t_ustr.email_addr);
       strtolower(line2);
       strcpy(line,inpstr);
       strtolower(line);
       if (strstr(line2,line))
        {
            sprintf(mess,"%-18s w/email  %s",t_ustr.say_name,t_ustr.email_addr);
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.email_addr[0]=0;
            num++;
        } 
       else {
		t_ustr.email_addr[0]=0;
		line[0]=0; line2[0]=0;
		continue;
	    }
       }
      else {
            sprintf(mess,"%-18s w/email  %s",t_ustr.say_name,t_ustr.email_addr);
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.email_addr[0]=0;
            line[0]=0; line2[0]=0;
            num++;
           } 
     } /* end of type else if */    
    else if (type==2) {
       if (search) {
       strcpy(line2,t_ustr.homepage);
       strtolower(line2);
       strcpy(line,inpstr);
       strtolower(line);
       if (strstr(line2,line))
        {
            sprintf(mess,"%-18s w/page  %s",t_ustr.say_name,t_ustr.homepage);
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.homepage[0]=0;
            line[0]=0; line2[0]=0;
            num++;
        } 
       else {
		t_ustr.homepage[0]=0;
		line[0]=0; line2[0]=0;
		continue;
	    }
       }
      else {
            sprintf(mess,"%-18s w/page  %s",t_ustr.say_name,t_ustr.homepage);
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.homepage[0]=0;
            line[0]=0; line2[0]=0;
            num++;
           } 
     } /* end of type else if */    
    else if (type==3) {
       if (search) {
       strcpy(line2,t_ustr.desc);
       strtolower(line2);
       strcpy(line,inpstr);
       strtolower(line);
       if (strstr(line2,line))
        {
            sprintf(mess,"%-18s w/desc  %s",t_ustr.say_name,t_ustr.desc);
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.desc[0]=0;
            line[0]=0; line2[0]=0;
            num++;
        } 
       else { 
		t_ustr.desc[0]=0;
		line[0]=0; line2[0]=0;
		continue;
	    }
       }
      else {
            sprintf(mess,"%-18s w/desc  %s",t_ustr.say_name,t_ustr.desc);
            fputs(mess,fp);
            fputs("\n",fp);
            t_ustr.desc[0]=0;
            line[0]=0; line2[0]=0;
            num++;
           } 
     } /* end of type else if */    
    else if ((type==4) || (type==5)) {
       strcpy(line2,small_buff);
       strtolower(line2);
       strcpy(line,inpstr);
       strtolower(line);
      if (strstr(line2,line)) {
         sprintf(filename2,"%s/%s",filerid,small_buff);
         if (!(fp2=fopen(filename2,"r"))) {
            write_str(user,"Cant open file for reading!");
            continue;
            }
         fgets(timebuf,13,fp2);
         FCLOSE(fp2);
         timenum=atoi(timebuf);
         tm_then=((time_t) timenum);
         sprintf(mess,"%-35s %s ago",small_buff,converttime((long)((tm-tm_then)/60)));
         fputs(mess,fp);
         fputs("\n",fp);
         timebuf[0]=0;
         timenum=0;  
         len=0;
         num++;
         fputs("COMMENT: ",fp);
         sprintf(filename2,"%s/%s.c",filerid,small_buff);
         fp2=fopen(filename2,"r");   
         fgets(comment,ARR_SIZE,fp2);
         FCLOSE(fp2);
         fputs(comment,fp);
         fputs("\n\n",fp);
         comment[0]=0;
        }
       small_buff[0]=0;
       line[0]=0; line2[0]=0;
       len=0;
     } /* end of type else if */
   }       /* End of while */
 
if (type==0)
 t_ustr.super=0;
else if (type==1) {
 t_ustr.email_addr[0]=0;
 line[0]=0; line2[0]=0;
 }
else if (type==2) {
 t_ustr.homepage[0]=0;
 line[0]=0; line2[0]=0;
 }
else if (type==3) {
 t_ustr.desc[0]=0;
 line[0]=0; line2[0]=0;
 }

 if ((type==4) || (type==5))
  sprintf(mess,"Displayed %d banned site%s",num,num == 1 ? "" : "s");
 else {
  fputs("\n",fp);
  sprintf(mess,"Displayed %d user%s",num,num == 1 ? "" : "s");
  }
 fputs(mess,fp);
 fputs("\n",fp);

 fclose(fp);
 (void) closedir(dirp);

 if (!cat(filename,user,0)) {
     sprintf(mess,"%s Cant cat clist listing file!",get_time(0,0));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     }

 return;
}

/*-----------------------------------------------------------------*/
/*  Vote command - ALWAYS, ALWAYS do a .vote -c to start a vote    */
/*   For votes, do NOT create any files but the "votefile"         */
/*-----------------------------------------------------------------*/
void vote(int user, char *inpstr)
{
int a,b,c;
char line[ARR_SIZE];
char filename[FILE_NAME_LEN];
FILE *fp;
FILE *fp2;

/* First check to see if file exist, no matter what */
   sprintf(filename,"%s/%s",LIBDIR,"votetallies");
   if (!(fp=fopen(filename,"r"))) {
   fp=fopen(filename,"w");
   fputs("0\n0\n0\n",fp);
   }
   FCLOSE(fp);
if (!strlen(inpstr)) {
   sprintf(filename,"%s/%s",LIBDIR,"votefile");
   if (!cat(filename,user,0)) {
      write_str(user,"There is no current topic that we are voting on.");
      return;
      }
   write_str(user," ");
if (!strcmp(ustr[user].name,ROOT_ID)) {
   write_str(user,"Current tally");
   sprintf(filename,"%s/%s",LIBDIR,"votetallies");
   if (!(fp2=fopen(filename,"r")))
     {     
      write_str(user,"Noone has voted yet.");
      return;
     }
     fscanf(fp2,"%s\n",line);
   sprintf(mess,"Choice 1 - %s votes",line);
   write_str(user,mess);
   line[0]=0;
     fscanf(fp2,"%s\n",line);
   sprintf(mess,"Choice 2 - %s votes",line);
   write_str(user,mess);
   line[0]=0;
     fscanf(fp2,"%s\n",line);
   sprintf(mess,"Choice 3 - %s votes",line);
   write_str(user,mess);
   line[0]=0;
   write_str(user," ");
   FCLOSE(fp2);
   return;
   }  /* end of tally if */
  else { return; }
 }  /* end of main if */
else if ( (!strcmp(inpstr,"1")) || (!strcmp(inpstr,"2"))
           || (!strcmp(inpstr,"3")) ) {
   sprintf(filename,"%s/%s",LIBDIR,"voteusers");
   if (!(fp=fopen(filename,"r"))) {
     goto ADD;
     }

   while (!feof(fp)) {
     fscanf(fp,"%s\n",line);
     if (!strcmp(line,ustr[user].name)) {
        write_str(user,"You already voted!");
        FCLOSE(fp); 
        return;
        }
     line[0]=0;
    }  /* end of while*/
   FCLOSE(fp);

  ADD:
  sprintf(filename,"%s/%s",LIBDIR,"votetallies");
   if (!(fp2=fopen(filename,"r")))
     {     
      write_str(user,"Cannot read tallies!");
      return;
     }
     fscanf(fp2,"%s\n",line);
     a=atoi(line);
     line[0]=0;
     fscanf(fp2,"%s\n",line);
     b=atoi(line);
     line[0]=0;
     fscanf(fp2,"%s\n",line);
     c=atoi(line);
     line[0]=0;
   FCLOSE(fp2);
   if (!(fp=fopen(filename,"w")))
     {     
      write_str(user,"Cannot write tallies!");
      a=0; b=0; c=0;
      return;
     }
  if (!strcmp(inpstr,"1")) a++;
  else if (!strcmp(inpstr,"2")) b++;
  else c++;

  sprintf(line,"%d\n",a);
  fputs(line,fp);
  line[0]=0;
  sprintf(line,"%d\n",b);
  fputs(line,fp);
  line[0]=0;
  sprintf(line,"%d\n",c);
  fputs(line,fp);
  line[0]=0;
  FCLOSE(fp);

   sprintf(filename,"%s/%s",LIBDIR,"voteusers");
   if (!(fp2=fopen(filename,"a")))
     {
      write_str(user,"Cannot append to user's vote file!");
      return;
     }
   fputs(ustr[user].name,fp2);
   fputs("\n",fp2);
   FCLOSE(fp2);

  write_str(user,"*CHIK* *CHIK* Thanx for your vote!");
sprintf(mess,"VOTE: by %s\n",ustr[user].say_name);
print_to_syslog(mess);
return;
}

else if ( (!strcmp(inpstr,"-c")) && (ustr[user].tempsuper==MAX_LEVEL) ) {
   sprintf(filename,"%s/%s",LIBDIR,"voteusers");
   remove(filename);
   sprintf(filename,"%s/%s",LIBDIR,"votefile");
   remove(filename);
   write_str(user,"Vote topic deleted..");
   sprintf(filename,"%s/%s",LIBDIR,"votetallies");
   if (!(fp2=fopen(filename,"w"))) {
      write_str(user,"Reset user's file but cant erase and reset tallies");
      return;
      }
   fputs("0\n0\n0\n",fp2);
   FCLOSE(fp2);
   write_str(user,"Users and tallies erased..Files reset.");
   sprintf(mess,"VOTE RESET: by %s\n",ustr[user].say_name);
   print_to_syslog(mess);
   return;
   }

else if ( (!strcmp(inpstr,"-d")) && (ustr[user].tempsuper==MAX_LEVEL) ) {
     inpstr[0]=0;
     enter_votedesc(user,inpstr);
     return;
     }

else {
  write_str(user,"Choice not understood. Pick 1, 2, or 3.");
  return;
  }
}

/* Enter vote file */
void enter_votedesc(int user, char *inpstr)
{
char *c;
int ret_val;
int redo = 0;
char filename2[FILE_NAME_LEN];
FILE *fp;

/* get memory */
STARTVOTED:

if (!ustr[user].vote_enter) {
        if (!(ustr[user].vote_start=(char *)malloc(82*VOTE_LINES))) {
        logerror("Couldn't allocate mem. in enter_votedesc()");
        sprintf(mess,"%s : cant allocate buffer mem.",syserror);
        write_str(user,mess);
        return;
        }
    ustr[user].vote_enter=1;
    ustr[user].vote_end=ustr[user].vote_start;
    if (!redo) strcpy(ustr[user].mutter,ustr[user].flags);
    user_ignore(user,"all");
    write_str(user,"");
    write_str(user,"** Entering next vote topic, finish with a '.' on a line by itself **");
    sprintf(mess,"** Max lines you can write is %d",VOTE_LINES);
    write_str(user,mess);
    write_str(user,"");
    write_str_nr(user,"1: ");
	 telnet_write_eor(user);
    noprompt=1;
    return;
    }
inpstr[80]=0;  c=inpstr;

/* check for dot terminator */
ret_val=0;

if (ustr[user].vote_enter > VOTE_LINES) {
   if (*c=='s' && *(c+1)==0) {
     ret_val=write_vote(user);
        if (ret_val) {
	  write_str(user,"");
	  write_str(user,"Vote Topic stored");
          write_str(user,"*** Don't forget to clear the votes with .vote -c ***");
          }
        else {
	write_str(user,"");
	write_str(user,"Vote Topic not stored");
	}
        free(ustr[user].vote_start);  ustr[user].vote_enter=0;
        ustr[user].vote_end='\0';
        noprompt=0;
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;
        return;
     }
   else if (*c=='v' && *(c+1)==0) {
write_str(user,"+-----------------------------------------------------------------------------+");
c='\0';
strcpy(filename2,get_temp_file());
fp=fopen(filename2,"w");
for (c=ustr[user].vote_start;c<ustr[user].vote_end;++c) { 
    putc(*c,fp);
    }
    fclose(fp);
    cat(filename2,user,0);
    remove(filename2);
c='\0';
write_str(user,"+-----------------------------------------------------------------------------+");
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
            noprompt=1;  return;
        }
   else if (*c=='r' && *(c+1)==0) {
        free(ustr[user].vote_start); ustr[user].vote_enter=0;
        ustr[user].vote_end='\0';
        redo=1;
        goto STARTVOTED;
        }             
   else if (*c=='a' && *(c+1)==0) {
        free(ustr[user].vote_start); ustr[user].vote_enter=0;
        ustr[user].vote_end='\0';
	write_str(user,"");
        write_str(user,"Vote Topic not stored");
        noprompt=0;
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;
        return;
        }             
   else {
    write_str_nr(user,PROFILE_PROMPT);
	 telnet_write_eor(user);
    return;
   } 
  }

if (*c=='.' && *(c+1)==0) {
        if (ustr[user].vote_enter!=1)   {
            ustr[user].vote_enter= VOTE_LINES + 1;
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
            noprompt=1;  return; 
            }
        else {
	write_str(user,"");
	write_str(user,"Vote Topic not stored");
	}
        free(ustr[user].vote_start);  ustr[user].vote_enter=0;
        noprompt=0;
        strcpy(ustr[user].flags,ustr[user].mutter);
        ustr[user].mutter[0]=0;
        return;
        }

/* write string to memory */
while(*c) *ustr[user].vote_end++=*c++;
*ustr[user].vote_end++='\n';

/* end of lines */
if (ustr[user].vote_enter==VOTE_LINES) {
            ustr[user].vote_enter= VOTE_LINES + 1;
            write_str_nr(user,PROFILE_PROMPT);
		 telnet_write_eor(user);
            noprompt=1;  return;
        }
sprintf(mess,"%d: ",++ustr[user].vote_enter);
write_str_nr(user,mess);
telnet_write_eor(user);
}
     
/*--------------------------------------------*/
/* Force a user to change his or her settings */
/* or to execute a command                    */
/*--------------------------------------------*/
void force_user(int user, char *inpstr)
{
int u,com_num_two,value=0;
int online=1;
char temp[EMAIL_LENGTH+1];
char other_user[ARR_SIZE],command[256];
char filename[FILE_NAME_LEN];

command[0]=0;

if (!strlen(inpstr)) {
   write_str(user,"Who do you want to force?");
   write_str(user,".force <user> <option> <setting>");
   return;
   }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1)
  {
    if (!read_user(other_user)) {
    write_str(user,NO_USER_STR);
    return;
    }
    online=0;
  }

if (online) {
if (u == user)
  {
   write_str(user,"Why go through all that trouble when you can do it yourself!");
   return;
  }
}

if (online) {
 if ( (!strcmp(ustr[u].name,ROOT_ID)) ||
      (!strcmp(ustr[u].name,BOT_ID) && strcmp(ustr[user].name,BOTS_ROOTID)) ||
      ((ustr[user].tempsuper <= ustr[u].super) && (strcmp(ustr[user].name,ROOT_ID)))
    ) {
    write_str(user,"Yeah, right!");
    return;
    }
 }
else {
 if ((!strcmp(other_user,ROOT_ID)) || 
    (!strcmp(other_user,BOT_ID) && strcmp(ustr[user].name,BOTS_ROOTID)) ||
    ((ustr[user].tempsuper <= t_ustr.super) && (strcmp(ustr[user].name,ROOT_ID)))
    ) {
    write_str(user,"Yeah, right!");
    return;
    }
}


remove_first(inpstr);
sscanf(inpstr,"%s ",command);
strtolower(command);
if (strlen(command) > 8) {
   write_str(user,"Setting name too long");
   write_str(user,"Valid options are:");
   write_str(user,"  abbrs (toggle)   autoread (toggle)   autofwd  (toggle)");  
   write_str(user,"  car   (toggle)   color    (on|off)   cols     (16-256)");  
   write_str(user,"  desc  ( mess )   email    ( mess )   entermsg ( mess )");  
   write_str(user,"  fail  ( mess )   gender   ( mess )   hi       (toggle)");
   write_str(user,"  quote (toggle)   rows     (5-256)    homepage ( mess )");
   write_str(user,"  space (toggle)   succ     ( mess )   exitmsg  ( mess )");
   write_str(user,"  passhid (toggle) pbreak   (toggle)   beeps    (toggle)");
   write_str(user,"  icq   ( mess )   profdel");
   write_str(user,"");
   return;
   }

remove_first(inpstr);
if (!strcmp(command,"abbrs")) goto ABBR;
else if (!strcmp(command,"autoread")) goto AUTOR;
else if (!strcmp(command,"autofwd")) goto AUTOF;
else if (!strcmp(command,"car") || !strcmp(command,"carriage")) goto CAR;
else if (!strcmp(command,"color")) goto COLOR;
else if (!strcmp(command,"cols") || !strcmp(command,"width")) goto COLS;
else if (!strcmp(command,"desc")) goto DESC;
else if (!strcmp(command,"email")) goto EMAIL;
else if (!strcmp(command,"entermsg")) goto ENTER;
else if (!strcmp(command,"exitmsg")) goto EXITM;
else if (!strcmp(command,"fail")) goto FAIL;
else if (!strcmp(command,"gender")) goto GENDER;
else if (!strcmp(command,"hi")) goto HILI;
else if (!strcmp(command,"quote")) goto QUOTEP;
else if (!strcmp(command,"profdel")) goto PROFDEL;
else if (!strcmp(command,"rows") || !strcmp(command,"lines")) goto ROWS;
else if (!strcmp(command,"homepage")) goto HOMEP;
else if (!strcmp(command,"space")) goto SPACE;
else if (!strcmp(command,"succ")) goto SUCC;
else if (!strcmp(command,"com")) goto COMMS;
else if (!strcmp(command,"passhid")) goto PASS;
else if (!strcmp(command,"pbreak")) goto P_BREAK;
else if (!strcmp(command,"beeps")) goto BEEPSET;
else if (!strcmp(command,"icq")) goto ICQSET;
else {
      write_str(user,"Option unknown.");
      write_str(user,"Valid options are:");
   write_str(user,"  abbrs (toggle)   autoread (toggle)   autofwd  (toggle)");  
   write_str(user,"  car   (toggle)   color    (on|off)   cols     (16-256)");  
   write_str(user,"  desc  ( mess )   email    ( mess )   entermsg ( mess )");  
   write_str(user,"  fail  ( mess )   gender   ( mess )   hi       (toggle)");
   write_str(user,"  quote (toggle)   rows     (5-256)    homepage ( mess )");
   write_str(user,"  space (toggle)   succ     ( mess )   exitmsg  ( mess )");
   write_str(user,"  passhid (toggle) pbreak   (toggle)   beeps    (toggle)");
   write_str(user,"  icq   ( mess )   profdel");
   write_str(user,"");
   return;
     }

ABBR:
if (online) {
  if (ustr[u].abbrs)
    {
      write_str(user, "Abbreviations are now off for them.");
      ustr[u].abbrs = 0;
    }
   else
    {
      write_str(user, "They can now use abbreviations");
      ustr[u].abbrs = 1;
    }

  read_user(ustr[u].login_name);
  t_ustr.abbrs = ustr[u].abbrs;
  write_user(ustr[u].login_name);
sprintf(mess,"%s FORCED %s with abbrs_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.abbrs)
    {
      write_str(user, "Abbreviations are now off for them.");
      t_ustr.abbrs = 0;
    }
   else
    {
      write_str(user, "They can now use abbreviations");
      t_ustr.abbrs = 1;
    }
sprintf(mess,"%s FORCED %s with abbrs_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

AUTOR:
if (online) {
  if (ustr[u].autor==3)
    {
      write_str(user, "Autoread now ^HYoff^ for them.");
      ustr[u].autor = 0;
    }
   else if (ustr[u].autor==2)
    {
      write_str(user, "Autoread now on ^HYfor logins and online^ for them.");
      ustr[u].autor = 3;
    }
   else if (ustr[u].autor==1)
    {
      write_str(user, "Autoread now on ^HYfor online only^ for them.");
      ustr[u].autor = 2;
    }
   else if (ustr[u].autor==0)
    {
      write_str(user, "Autoread now on ^HYfor logins only^ for them.");
      ustr[u].autor = 1;
    }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Autoread_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.autor==3)
    {
      write_str(user, "Autoread now ^HYoff^ for them.");
      t_ustr.autor = 0;
    }
   else if (t_ustr.autor==2)
    {
      write_str(user, "Autoread now on ^HYfor logins and online^ for them.");
      t_ustr.autor = 3;
    }
   else if (t_ustr.autor==1)
    {
      write_str(user, "Autoread now on ^HYfor online only^ for them.");
      t_ustr.autor = 2;
    }
   else if (t_ustr.autor==0)
    {
      write_str(user, "Autoread now on ^HYfor logins only^ for them.");
      t_ustr.autor = 1;
    }

sprintf(mess,"%s FORCED %s with Autoread_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

AUTOF:
if (online) {
  if (ustr[u].autof==2)
    {
      write_str(user, "Autofwd now ^HYoff^ for them.");
      ustr[u].autof = 0;
    }
   else if (ustr[u].autof==0)
    {
      write_str(user, "Autofwd now on ^HYall the time^ for them.");
      ustr[u].autof = 1;
    }
   else if (ustr[u].autof==1)
    {
      write_str(user, "Autofwd now on ^HYonly when not online^ for them.");
      ustr[u].autof = 2;
    }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Autofwd_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.autof==2)
    {
      write_str(user, "Autofwd now ^HYoff^ for them.");
      t_ustr.autof = 0;
    }
   else if (t_ustr.autof==0)
    {
      write_str(user, "Autofwd now on ^HYall the time^ for them.");
      t_ustr.autof = 1;
    }
   else if (t_ustr.autof==1)
    {
      write_str(user, "Autofwd now on ^HYonly when not online^ for them.");
      t_ustr.autof = 2;
    }

sprintf(mess,"%s FORCED %s with Autofwd_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

CAR:

if (online) {
  
if (!strlen(inpstr)) {
  if (!ustr[u].car_return) {
   write_str(user,"Set their carriage returns ON");
   ustr[u].car_return = 1;
   }
   else {
   write_str(user,"Set their carriage returns OFF");
   ustr[u].car_return = 0;
   }
 }
else {
 if (!strcmp(inpstr,"1")) {
   write_str(user,"Set their carriage returns ON");
   ustr[u].car_return = 1;
   }
 else if (!strcmp(inpstr,"0")) {
   write_str(user,"Set their carriage returns OFF");
   ustr[u].car_return = 0;
  }
 else {
   write_str(user,"Set their carriage returns ON");
   ustr[u].car_return = 1;
  }   
 } /* end of else */
  copy_from_user(u);
  write_user(ustr[u].login_name);
sprintf(mess,"%s FORCED %s with Carriages_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
if (!strlen(inpstr)) {
  if (!t_ustr.car_return) {
   write_str(user,"Set their carriage returns ON");
   t_ustr.car_return = 1;
   }
   else {
   write_str(user,"Set their carriage returns OFF");
   t_ustr.car_return = 0;
   }
 }
else {
 if (!strcmp(inpstr,"1")) {
   write_str(user,"Set their carriage returns ON");
   t_ustr.car_return = 1;
   }
 else if (!strcmp(inpstr,"0")) {
   write_str(user,"Set their carriage returns OFF");
   t_ustr.car_return = 0;
  }
 else {
   write_str(user,"Set their carriage returns ON");
   t_ustr.car_return = 1;
  }   
 } /* end of else */

sprintf(mess,"%s FORCED %s with Carriages_set\n",ustr[user].say_name,t_ustr.say_name);
  write_user(other_user);
}
print_to_syslog(mess);
goto END;

COLOR:
if (online) {
if (!strcmp(inpstr,"on") || !strcmp(inpstr,"ON")) {
   write_str(user,"Color is now   On for them.");
   ustr[u].color=1;
   }

if (!strcmp(inpstr,"off") || !strcmp(inpstr,"OFF")) {
   write_str(user,"Color is now   Off for them.");
   ustr[u].color=0;
   }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Color_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
if (!strcmp(inpstr,"on") || !strcmp(inpstr,"ON")) {
   write_str(user,"Color is now   On for them.");
   t_ustr.color=1;
   }

if (!strcmp(inpstr,"off") || !strcmp(inpstr,"OFF")) {
   write_str(user,"Color is now   Off for them.");
   t_ustr.color=0;
   }

sprintf(mess,"%s FORCED %s with Color_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

COLS:
  value=5;

  sscanf(inpstr,"%d", &value);

  if (value < 16 || value > 256)
    {
      write_str(user,"cols set to 256 (valid range is 16 to 256)");
      value = 256;
    }

  sprintf(mess,"Set their terminal cols to: %d",value);
  write_str(user,mess);

if (online) {
  ustr[u].cols = value;
  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Cols_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  t_ustr.cols     = value;
sprintf(mess,"%s FORCED %s with Cols_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

DESC:
if (online) {
if (!strlen(inpstr))
  {
   sprintf(mess,"Their description is : %s",ustr[u].desc);
   write_str(user,mess);
   return;
  }

if (strlen(inpstr) > DESC_LEN-1)
  {
    write_str(user,"Description too long");
    return;
  }

strcat(inpstr,"@@");

strcpy(ustr[u].desc,inpstr);
copy_from_user(u);
write_user(ustr[u].login_name);
sprintf(mess,"Their new desc: %s",ustr[u].desc);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with Desc_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
if (!strlen(inpstr))
  {
   sprintf(mess,"Their description is : %s",t_ustr.desc);
   write_str(user,mess);
   return;
  }

if (strlen(inpstr) > DESC_LEN-1)
  {
    write_str(user,"Description too long");
    return;
  }

strcat(inpstr,"@@");

strcpy(t_ustr.desc,inpstr);
sprintf(mess,"Their new desc: %s",t_ustr.desc);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with Desc_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

EMAIL:
if (online) {
  /* Check for illegal characters in email addy */
  if (strpbrk(inpstr,";/[]\\") ) {
     write_str(user,"Illegal email address");
     return;
     }

  if (strstr(inpstr,"^")) {
      write_str(user,"Email cant have color or hilite codes in it.");
      return;
      }
  if (strlen(inpstr)>EMAIL_LENGTH)
    {
      write_str(user,"Email address truncated");
      inpstr[EMAIL_LENGTH-1]=0;
    }
  if ((!strcmp(inpstr,"-c")) || (!strcmp(inpstr,"clear"))) {
      strcpy(inpstr,DEF_EMAIL);
      write_str(user,"Email address cleared and reset.");
      goto SKIP;
      }       

 strcpy(temp,inpstr);
 strtolower(temp);

  if (strstr(temp,"whitehouse.gov"))
      {
       write_str(user,"Email address not valid.");
       return;
      }
  else if (!strlen(inpstr)) {
      write_str(user,"Must specify an address or -c|clear");
      return;
      }
  else if (!strstr(inpstr,".") || !strstr(inpstr,"@")) {
       write_str(user,"Email address not valid.");
       return;
      }

  sprintf(mess,"Set their email address to: %s",inpstr);
  write_str(user,mess);

  SKIP:
  strcpy(ustr[u].email_addr,inpstr);
  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Email_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  /* Check for illegal characters in email addy */
  if (strpbrk(inpstr,";/[]\\") ) {
     write_str(user,"Illegal email address");
     return;
     }

  if (strstr(inpstr,"^")) {
      write_str(user,"Email cant have color or hilite codes in it.");
      return;
      }
  if (strlen(inpstr)>EMAIL_LENGTH)
    {
      write_str(user,"Email address truncated");
      inpstr[EMAIL_LENGTH-1]=0;
    }
  if ((!strcmp(inpstr,"-c")) || (!strcmp(inpstr,"clear"))) {
      strcpy(inpstr,DEF_EMAIL);
      write_str(user,"Email address cleared and reset.");
      goto SKIP2;
      }       

 strcpy(temp,inpstr);
 strtolower(temp);

  if (strstr(temp,"whitehouse.gov"))
      {
       write_str(user,"Email address not valid.");
       return;
      }
  else if (!strlen(inpstr)) {
      write_str(user,"Must specify an address or -c|clear");
      return;
      }
  else if (!strstr(inpstr,".") || !strstr(inpstr,"@")) {
       write_str(user,"Email address not valid.");
       return;
      }

  sprintf(mess,"Set their email address to: %s",inpstr);
  write_str(user,mess);

  SKIP2:
  strcpy(t_ustr.email_addr,inpstr);
  sprintf(mess,"%s FORCED %s with Email_set\n",ustr[user].say_name,t_ustr.say_name);
  write_user(other_user);
}
print_to_syslog(mess);
goto END;

ENTER:
if (online) {
if (!strlen(inpstr)) {
   sprintf(mess,"Their entermessage is: %s",ustr[u].entermsg);
   write_str(user,mess);
   return;
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(ustr[u].entermsg,DEF_ENTER);
    copy_from_user(u);
    write_user(ustr[u].name);
    write_str(user,"Their entermsg now set to default.");
    sprintf(mess,"%s FORCED %s with EnterMess_set\n",ustr[user].say_name,ustr[u].say_name);
    print_to_syslog(mess);
    goto END;
    }
if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(ustr[u].entermsg,inpstr);
strcat(ustr[u].entermsg,"@@");
copy_from_user(u);
write_user(ustr[u].name);
sprintf(mess,"Their new entermsg: %s",ustr[u].entermsg);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with EnterMess_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
if (!strlen(inpstr)) {
   sprintf(mess,"Their entermessage is: %s",t_ustr.entermsg);
   write_str(user,mess);
   return;
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(t_ustr.entermsg,DEF_ENTER);
    write_str(user,"Their entermsg now set to default.");
    sprintf(mess,"%s FORCED %s with EnterMess_set\n",ustr[user].say_name,t_ustr.say_name);
    write_user(other_user);
    print_to_syslog(mess);
    goto END;
    }
if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(t_ustr.entermsg,inpstr);
strcat(t_ustr.entermsg,"@@");
sprintf(mess,"Their new entermsg: %s",t_ustr.entermsg);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with EnterMess_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

EXITM:
if (online) {
if (!strlen(inpstr)) {
   sprintf(mess,"Their exitmessage is: %s",ustr[u].exitmsg);
   write_str(user,mess);
   return;
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(ustr[u].exitmsg,DEF_EXIT);
    copy_from_user(u);
    write_user(ustr[u].name);
    write_str(user,"Their exitmsg now set to default.");
    sprintf(mess,"%s FORCED %s with ExitMess_set\n",ustr[user].say_name,ustr[u].say_name);
    print_to_syslog(mess);
    goto END;
    }
if (strlen(inpstr) > MAX_EXITM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(ustr[u].exitmsg,inpstr);
strcat(ustr[u].exitmsg,"@@");
copy_from_user(u);
write_user(ustr[u].name);
sprintf(mess,"Their new exitmsg: %s",ustr[u].exitmsg);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with ExitMess_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
if (!strlen(inpstr)) {
   sprintf(mess,"Their exitmessage is: %s",t_ustr.exitmsg);
   write_str(user,mess);
   return;
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(t_ustr.exitmsg,DEF_EXIT);
    write_str(user,"Their exitmsg now set to default.");
    sprintf(mess,"%s FORCED %s with ExitMess_set\n",ustr[user].say_name,t_ustr.say_name);
    write_user(other_user);
    print_to_syslog(mess);
    goto END;
    }
if (strlen(inpstr) > MAX_EXITM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(t_ustr.exitmsg,inpstr);
strcat(t_ustr.exitmsg,"@@");
sprintf(mess,"Their new exitmsg: %s",t_ustr.exitmsg);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with ExitMess_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

FAIL:
if (online) {
if (!strlen(inpstr)) {
   sprintf(mess,"Their fail is: %s",ustr[u].fail);
   write_str(user,mess);
   return;
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(ustr[u].fail,"");
    copy_from_user(u);
    write_user(ustr[u].name);
    write_str(user,"Fail message cleared.");
    sprintf(mess,"%s FORCED %s with Fail_set\n",ustr[user].say_name,ustr[u].say_name);
    print_to_syslog(mess);
    return;
    }

if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(ustr[u].fail,inpstr);
strcat(ustr[u].fail,"@@");
copy_from_user(u);
write_user(ustr[u].name);
sprintf(mess,"Their new fail: %s",ustr[u].fail);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with Fail_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
if (!strlen(inpstr)) {
   sprintf(mess,"Their fail is: %s",t_ustr.fail);
   write_str(user,mess);
   return;
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(t_ustr.fail,"");
    write_str(user,"Fail message cleared.");
    sprintf(mess,"%s FORCED %s with Fail_set\n",ustr[user].say_name,t_ustr.say_name);
    write_user(other_user);
    print_to_syslog(mess);
    return;
    }

if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(t_ustr.fail,inpstr);
strcat(t_ustr.fail,"@@");
sprintf(mess,"Their new fail: %s",t_ustr.fail);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with Fail_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

GENDER:
if (online) {
  if (strlen(inpstr)>29)
    {
      write_str(user,"Gender truncated");
      inpstr[29]=0;
    }
strcat(inpstr,"@@");

  sprintf(mess,"Set their gender to: %s",inpstr);
  write_str(user,mess);

  strcpy(ustr[u].sex,inpstr);
  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Gender_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (strlen(inpstr)>29)
    {
      write_str(user,"Gender truncated");
      inpstr[29]=0;
    }
strcat(inpstr,"@@");

  sprintf(mess,"Set their gender to: %s",inpstr);
  write_str(user,mess);

  strcpy(t_ustr.sex,inpstr);
sprintf(mess,"%s FORCED %s with Gender_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

HILI:
if (online) {
  if (ustr[u].hilite==2)
    {
      write_str(user, "High_lighting now off for them.");
      ustr[u].hilite = 0;
    }
   else if (ustr[u].hilite==1)
    {
      write_str(user, "High_lighting now on for them for everything except private communication which will be normal with color.");
      ustr[u].hilite = 2;
    }
   else
    {
      write_str(user, "High_lighting now on for them for everyhting.");
      ustr[u].hilite = 1;
    }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Hi_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.hilite==2)
    {
      write_str(user, "High_lighting now off for them.");
      t_ustr.hilite = 0;
    }
   else if (t_ustr.hilite==1)
    {
      write_str(user, "High_lighting now on for them for everything except private communication which will be normal with color.");
      t_ustr.hilite = 2;
    }
   else
    {
      write_str(user, "High_lighting now on for them for everything.");
      t_ustr.hilite = 1;
    }

sprintf(mess,"%s FORCED %s with Hi_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

PASS:
if (online) {
  if (ustr[u].passhid)
    {
      write_str(user, "Password WILL be echoed during logins for them.");
      ustr[u].passhid = 0;
    }
   else
    {
      write_str(user, "Password will NOT be echoed during logins for them.");
      ustr[u].passhid = 1;
    }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Passhid_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.passhid)
    {
      write_str(user, "Password WILL be echoed during logins for them.");
      t_ustr.passhid = 0;
    }
   else
    {
      write_str(user, "Password will NOT be echoed during logins for them.");
      t_ustr.passhid = 1;
    }

sprintf(mess,"%s FORCED %s with Passhid_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

P_BREAK:
if (online) {
  if (ustr[u].pbreak)
    {
      write_str(user, "Who listing will be continuous for them.");
      ustr[u].pbreak = 0;
    }
   else
    {
      write_str(user, "Who listing will be paged for them.");
      ustr[u].pbreak = 1;
    }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Pbreak_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.pbreak)
    {
      write_str(user, "Who listing will be continuous for them.");
      t_ustr.pbreak = 0;
    }
   else
    {
      write_str(user, "Who listing will be paged for them.");
      t_ustr.pbreak = 1;
    }

sprintf(mess,"%s FORCED %s with Pbreak_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

BEEPSET:
if (online) {
  if (ustr[u].beeps)
    {
      write_str(user, "They now will ^NOT^ get beeps on priv. comms.");
      ustr[u].beeps = 0;
    }
   else
    {
      write_str(user, "They now ^WILL^ get beeps on priv. comms.");
      ustr[u].beeps = 1;
    }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with beep_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.beeps)
    {
      write_str(user, "They now will ^NOT^ get beeps on priv. comms.");
      t_ustr.beeps = 0;
    }
   else
    {
      write_str(user, "They now ^WILL^ get beeps on priv. comms.");
      t_ustr.beeps = 1;
    }

sprintf(mess,"%s FORCED %s with beep_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

QUOTEP:
if (online) {
  if (ustr[u].quote)
    {
      write_str(user, "Quote feature now  off for them.");
      ustr[u].quote = 0;
    }
   else
    {
      write_str(user,"Quote feature now  on for them.");
      ustr[u].quote = 1;
    }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Quote_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.quote)
    {
      write_str(user, "Quote feature now  off for them.");
      t_ustr.quote = 0;
    }
   else
    {
      write_str(user,"Quote feature now  on for them.");
      t_ustr.quote = 1;
    }

sprintf(mess,"%s FORCED %s with Quote_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

ROWS:
  value=5;

  sscanf(inpstr,"%d", &value);

  if (value < 5 || value > 256)
    {
      write_str(user,"rows set to 25 (valid range is 5 to 256)");
      value = 25;
    }

  sprintf(mess,"Set their terminal rows to: %d",value);
  write_str(user,mess);

if (online) {
  ustr[u].rows = value;
  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Rows_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  t_ustr.rows     = value;
sprintf(mess,"%s FORCED %s with Rows_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

HOMEP:
if (online) {
  if (!strlen(inpstr)) {
	sprintf(mess,"Their homepage is: %s",ustr[u].homepage);
	write_str(user, mess);
	return;
	}

  if (strstr(inpstr,"^")) {
      write_str(user,"Homepage cant have color or hilite codes in it.");
      return;
      }

  if (strlen(inpstr) > HOME_LEN)
     {
      write_str(user,"Home page address truncated");
      inpstr[HOME_LEN-1]=0;
     }

  sprintf(mess,"Set page to: %s",inpstr);
  write_str(user,mess);

  strcpy(ustr[u].homepage,inpstr);
  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Homepage_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (!strlen(inpstr)) {
	sprintf(mess,"Their homepage is: %s",t_ustr.homepage);
	write_str(user, mess);
	return;
	}

  if (strstr(inpstr,"^")) {
      write_str(user,"Homepage cant have color or hilite codes in it.");
      return;
      }

  if (strlen(inpstr) > HOME_LEN)
     {
      write_str(user,"Home page address truncated");
      inpstr[HOME_LEN-1]=0;
     }

  sprintf(mess,"Set page to: %s",inpstr);
  write_str(user,mess);

  strcpy(t_ustr.homepage,inpstr);
sprintf(mess,"%s FORCED %s with Homepage_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

ICQSET:
if (online) {
  if (!strlen(inpstr)) {
	sprintf(mess,"Their ICQ # is: %s",ustr[u].icq);
	write_str(user, mess);
	return;
	}

  if (strstr(inpstr,"^")) {
      write_str(user,"ICQs cant have color or hilite codes in them.");
      return;
      }

  if (strlen(inpstr) > 20)
     {
      write_str(user,"ICQ number truncated");
      inpstr[20-1]=0;
     }

  sprintf(mess,"Set ICQ # to: %s",inpstr);
  write_str(user,mess);

  strcpy(ustr[u].icq,inpstr);
  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with ICQ_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (!strlen(inpstr)) {
	sprintf(mess,"Their ICQ # is: %s",t_ustr.icq);
	write_str(user, mess);
	return;
	}

  if (strstr(inpstr,"^")) {
      write_str(user,"ICQs cant have color or hilite codes in them.");
      return;
      }

  if (strlen(inpstr) > 20)
     {
      write_str(user,"ICQ number truncated");
      inpstr[20-1]=0;
     }

  sprintf(mess,"Set ICQ # to: %s",inpstr);
  write_str(user,mess);

  strcpy(t_ustr.icq,inpstr);
sprintf(mess,"%s FORCED %s with ICQ_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

SPACE:
if (online) {
  if (ustr[u].white_space)
    {
      write_str(user, "White space removal is now off for them.");
      ustr[u].white_space = 0;
    }
   else
    {
      write_str(user, "White space removal is now on for them.");
      ustr[u].white_space = 1;
    }

  copy_from_user(u);
  write_user(ustr[u].name);
sprintf(mess,"%s FORCED %s with Space_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
  if (t_ustr.white_space)
    {
      write_str(user, "White space removal is now off for them.");
      t_ustr.white_space = 0;
    }
   else
    {
      write_str(user, "White space removal is now on for them.");
      t_ustr.white_space = 1;
    }

sprintf(mess,"%s FORCED %s with Space_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

SUCC:
if (online) {
if (!strlen(inpstr)) {
   sprintf(mess,"Their success is: %s",ustr[u].succ);
   write_str(user,mess);
   return;
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(ustr[u].succ,"");
    copy_from_user(u);
    write_user(ustr[u].name);
    write_str(user,"Success message cleared.");
    sprintf(mess,"%s FORCED %s with Succ_set\n",ustr[user].say_name,ustr[u].say_name);
    print_to_syslog(mess);
    return;
    }
if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(ustr[u].succ,inpstr);
strcat(ustr[u].succ,"@@");
copy_from_user(u);
write_user(ustr[u].name);
sprintf(mess,"Their new success: %s",ustr[u].succ);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with Succ_set\n",ustr[user].say_name,ustr[u].say_name);
}
else {
if (!strlen(inpstr)) {
   sprintf(mess,"Their success is: %s",t_ustr.succ);
   write_str(user,mess);
   return;
   }
if (!strcmp(inpstr,"clear") || !strcmp(inpstr,"none") ||
    !strcmp(inpstr,"-c")) {
    strcpy(t_ustr.succ,"");
    write_str(user,"Success message cleared.");
    sprintf(mess,"%s FORCED %s with Succ_set\n",ustr[user].say_name,t_ustr.say_name);
    print_to_syslog(mess);
    write_user(other_user);
    return;
    }
if (strlen(inpstr) > MAX_ENTERM-2) {
   write_str(user,"Message too long.");
   return;
   }

strcpy(t_ustr.succ,inpstr);
strcat(t_ustr.succ,"@@");
sprintf(mess,"Their new success: %s",t_ustr.succ);
write_str(user,mess);
sprintf(mess,"%s FORCED %s with Succ_set\n",ustr[user].say_name,t_ustr.say_name);
write_user(other_user);
}
print_to_syslog(mess);
goto END;

PROFDEL:
if (online) {
  sprintf(filename,"%s/%s",PRO_DIR,ustr[u].name);
  remove(filename);
  sprintf(mess,"%s FORCED %s with Profile_Delete\n",ustr[user].say_name,ustr[u].say_name);
  print_to_syslog(mess);
}
else {
  sprintf(filename,"%s/%s",PRO_DIR,t_ustr.name);
  remove(filename);
  sprintf(mess,"%s FORCED %s with Profile_Delete\n",ustr[user].say_name,t_ustr.say_name);
  print_to_syslog(mess);
}
  write_str(user,"User profile deleted.");
  goto END;


COMMS:
	if (!online) {
	write_str(user,"You cant force a command on someone that isn't here!");
	return;
	}
	       com_num_two=get_com_num(u,inpstr);

                if ((com_num_two == -1) &&
                    (inpstr[0] == '.' || !strcmp(ustr[u].name,BOT_ID)))
		  {
		   write_str(user,SYNTAX_ERROR);
		   return;
		  }
		  
		if (com_num_two != -1) 
		  {
		   last_user=u;
		   sprintf(mess,"%s FORCED %s with COM %s\n",ustr[user].say_name,ustr[u].say_name,inpstr);
		   print_to_syslog(mess);
                   if ((!strcmp(ustr[u].name,BOT_ID) || !strcmp(ustr[u].name,ROOT_ID)) && inpstr[0]=='_')
			bot_com(com_num_two,u,inpstr);
                   else
			exec_com(com_num_two,u,inpstr);
			
		   last_user= -1;
                   write_str(user,"Forced.");
		  }
goto END;

END:
online=0;
}

/*** Read the system log, search for string if specified ***/
void readlog(int user, char *inpstr)
{
int occured=0;
char word[ARR_SIZE],filename[FILE_NAME_LEN],line[ARR_SIZE],line2[ARR_SIZE];
FILE *fp;
FILE *pp;

if (!strlen(inpstr)) 
  {
   sprintf(filename,"%s",LOGFILE);
   if (!cat(filename,user,0)) {
      write_str(user,"System log doesn't exist!");
      return;
      }
   return;
  }
  
sscanf(inpstr,"%s ",word);
strtolower(word);

if (word[0]=='-') {
   midcpy(word,word,1,3);
   strcpy(filename,get_temp_file());
   sprintf(mess,"tail -%s %s",word,LOGFILE);
 if (!(pp=popen(mess,"r"))) {
	write_str(user,"Can't open pipe to get the log!");
	return;
	}
 if (!(fp=fopen(filename,"w"))) {
	write_str(user,"Can't open temp file for writing!");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	fputs(line,fp);
      } /* end of while */
fclose(fp);
pclose(pp);

   if (!cat(filename,user,0))
     write_str(user,"Syslog empty");
   return;
   }

/* look through syslog */
	sprintf(t_mess,"%s",LOGFILE);
	strncpy(filename,t_mess,FILE_NAME_LEN);

	if (!(fp=fopen(filename,"r"))) { 
           write_str(user,"Cant open file.");
           return;
           }
	fgets(line,256,fp);
	while(!feof(fp)) {
		strcpy(line2,line);
	        strtolower(line);
		if (instr2(0,line,word,0)== -1) goto NEXT;
                   line2[strlen(line2)-1]=0;
		   write_str(user,line2);	
		   ++occured;
		NEXT:
		fgets(line,256,fp);
		}
	FCLOSE(fp);

if (!occured) write_str(user,"No occurences found");
}

/** Send a user to his or her home room */
void home_user(int user)
{
int area=ustr[user].area;
int new_area;
int found=0;

if (ustr[user].anchor) {
  write_str(user,ANCHORED_DOWN);
  return;
  }

      if ((ustr[user].tempsuper==0) && (!strcmp(ustr[user].desc,DEF_DESC))
          && (area==new_room)) {
          write_str(user,"You cant leave this room until you set a description with .desc");
          return;
         }

   found = FALSE;
   for (new_area=0; new_area < NUM_AREAS; ++new_area)
    {
     if ( !strcmp(astr[new_area].name, ustr[user].home_room) )
       {
         found = TRUE;
         break;
       }
    }
   
   if (!found) {
      write_str(user,"That room no longer exists.");
      return;
     }  

if (!strcmp(astr[area].name,ARREST_ROOM) &&
     strcmp(ustr[user].name,ROOT_ID)) {
   write_str(user,"You cant go home from this room!");
   return;
   }

/*----------------------------------------------*/
/* check to see if the user is in that room     */
/*----------------------------------------------*/
   
if (ustr[user].area == new_area)
  {
    write_str(user,"You are in that room now!");
    return;
  }

/*-----------------------------------------------------------*/
/* if the room is private abort home...inform user           */
/*-----------------------------------------------------------*/

if (astr[new_area].private && ustr[user].invite != new_area )
  {
   write_str(user,"Sorry - that room is currently private");
   return;
  }

  sprintf(mess,"%s goes home.",ustr[user].say_name);
      
/* send output to old room & to conv file */
if (!ustr[user].vis)
        strcpy(mess,INVIS_MOVES);

writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);

   if (!strcmp(astr[area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ left:%s", ustr[user].say_name);
    write_bot(mess);
    }

/*-----------------------------------------------------------*/
/* return room to public     (if needed)                     */
/*-----------------------------------------------------------*/
      
if (astr[area].private && (find_num_in_area(area) <= PRINUM))
  {
   strcpy(mess, NOW_PUBLIC);
   writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);
   cbuff(user);
   astr[area].private=0;
  }

/* record movement */ 
sprintf(mess,"%s %s",ustr[user].say_name,ustr[user].entermsg);

/* send output to new room */
if (!ustr[user].vis) 
	strcpy(mess,INVIS_MOVES);

ustr[user].area = new_area;

writeall_str(mess, 1, user, 0, user, NORM, NONE, 0);

   if (!strcmp(astr[new_area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ came in:%s", ustr[user].say_name);
    write_bot(mess);
    }

look(user,"");
}


/*** nerf command for use in nerf arena only ***/
void nerf(int user, char *inpstr)
{
int user2;
int i=0;
char name[ARR_SIZE];
int success;

if (!strlen(inpstr)) {
  write_str(user, "Usage: .nerf <user>");
  return;
  }

sscanf(inpstr, "%s", name);
strtolower(name);

user2=get_user_num(name, user);

    if (user2 == -1 )
     {
      not_signed_on(user,name);
      return;
     }

     if (!user_wants_message(user2,NERFS)) {
	write_str(user,"User is ignoring nerfs right now.");
	return;
	}

        /* See if user2 has user gagged */
        for (i=0; i<NUM_IGN_FLAGS; ++i) {
           if (NERFS==gagged_types[i]) {
		if (!gag_check(user,user2,0)) return;
             }
          }
        i=0;
     
    if (!ustr[user].vis || !ustr[user2].vis) {
       write_str(user,"Both of you must be visible first.");
       return;
       }

    if (ustr[user2].afk) 
     {
    if (ustr[user2].afk == 1) {
      if (!strlen(ustr[user2].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[user2].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[user2].say_name,ustr[user2].afkmsg);
      }
     else {
      if (!strlen(ustr[user2].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[user2].say_name);
      else
      sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[user2].say_name,ustr[user2].afkmsg);
      }

       write_str(user,t_mess);
       return;
     }
    
    if (strcmp(ustr[user2].name,name)) {
       write_str(user,"Name of person you are nerfing must be typed in full!");
       return;
       }

if (user==user2) {
  write_str(user, "You aren't supposed to nerf yourself!");
  return;
  }

/* can the person you're nerfing nerf back? */
for (i=0;sys[i].jump_vector!=-1;++i) {
	if (!strcmp(sys[i].command,".nerf")) break;
	}

if (ustr[user2].super < sys[i].su_com) {
  write_str(user,"That user doesn't have the .nerf command yet!");
  return;
  }

/*** change this bit to only use ppl in the nerf room ***/
if ( strcmp(astr[ustr[user].area].name,NERF_ROOM) ) {
           sprintf(t_mess,"You must be in the %s to do nerfs.",NERF_ROOM);
           write_str(user,t_mess);
           return;
        }

       if (ustr[user2].area != ustr[user].area)
         {
           sprintf(t_mess,"%s is not here to nerf with you!",ustr[user2].say_name);
           write_str(user,t_mess);
           return;
         }

if (ustr[user].nerf_shots==0) {
        write_hilite(user, "Your nerf-gun is empty!!!");
        sprintf(t_mess, "%s's nerf-gun clicks as the trigger is pulled - it's empty!!!", ustr[user].say_name);
        writeall_str(t_mess,1,user,0,user,NORM,NERFS,0);
        return;
        }
        
success = (((unsigned short) rand()) > 32000); /* should be # between 0-65535 */
         
if (!success) { /* beep here */
        sprintf(t_mess, "You fire at %s but miss", ustr[user2].say_name);
        write_str(user, t_mess);
        sprintf(t_mess, "-> %s tries to nerf %s, but misses", ustr[user].say_name, ustr[user2].say_name);
        writeall_str(t_mess,1,user,0,user,NORM,NERFS,0);
        }
else {  
        ustr[user2].nerf_energy--;
        if (ustr[user2].nerf_energy == 0) {
                ustr[user].nerf_energy=10;      /* full health to survivor */
                ustr[user].nerf_kills++;
                sprintf(t_mess, "You destroy %s - your health has been restored to FULL\n", ustr[user2].say_name);
                write_hilite(user, t_mess);
                sprintf(t_mess, "%s destroys %s\n", ustr[user].say_name, ustr[user2].say_name);
                writeall_str(t_mess,1,user,0,user,BOLD,NERFS,0);
                write_str(user2, "You have been destroyed. But don't worry - you can log back in and seek revenge");
                sprintf(t_mess, "*** %s was nerfed by %s ***\n", ustr[user2].say_name, ustr[user].say_name);
                writeall_str(t_mess,0,user,0,user,NORM,NERFS,0);
                ustr[user2].nerf_killed++;
                user_quit(user2);
                }
        else {
                sprintf(t_mess, "You nerf %s", ustr[user2].say_name);
                write_hilite(user, t_mess);
                sprintf(t_mess, "-> %s nerfs %s\n", ustr[user].say_name, ustr[user2].say_name);
                writeall_str(t_mess,1,user,0,user,BOLD,NERFS,0);
                if (ustr[user2].nerf_energy == 2) {
                        write_str(user2, "You will be destroyed after 2 more hits");
                        sprintf(t_mess, "%s has almost been destroyed", ustr[user2].say_name);
                        writeall_str(t_mess,1,user,0,user,NORM,NERFS,0);
                        }
                }
        }
                
ustr[user].nerf_shots--;
                
return;
}


/*** User to reload his or her nerf gun ***/
void reload(int user)
{
if ( strcmp(astr[ustr[user].area].name,NERF_ROOM) ) {
           sprintf(t_mess,"You must be in the %s before you can reload.",NERF_ROOM);
           write_str(user,t_mess);
           return;
        }
         
if (ustr[user].nerf_shots > 0) {
        sprintf(t_mess, "You have %d rounds left before you can reload",
                ustr[user].nerf_shots);
        write_str(user, t_mess);
        }
else {
        ustr[user].nerf_shots = 5;
        write_str(user, "You reload your nerf-gun with another 5 rounds");
        sprintf(t_mess, "An empty clip bounces on the ground by %s's feet", ustr[user].say_name);
        writeall_str(t_mess,1,user,0,user,NORM,NERFS,0);
        }

return;
}

/*----------------------------------------------------------*/
/* Here is where we begin, my hope to incorporate           */
/* Tic-tac-toe into the talker.                             */
/*----------------------------------------------------------*/
int ttt_is_end(int user)
{
        int i, board[9], draw = 1;
                        
        for (i = 0; i < 9; i++) {
                board[i] = (ustr[user].ttt_board>>(i*2))%4;   
                if (!(board[i]))
                        draw = 0;
        }

        if (board[0] && (((board[0] == board[1]) && (board[0] == board[2])) ||
                        ((board[0] == board[3]) && (board[0] == board[6]))))
                        return board[0];
        if (board[4] && (((board[4] == board[0]) && (board[4] == board[8])) ||
                        ((board[4] == board[1]) && (board[4] == board[7])) ||
                        ((board[4] == board[2]) && (board[4] == board[6])) ||
                        ((board[4] == board[3]) && (board[4] == board[5]))))
                        return board[4];
        if (board[8] && (((board[8] == board[2]) && (board[8] == board[5])) ||
                        ((board[8] == board[7]) && (board[8] == board[6]))))
                        return board[8];
        if (draw)
                return 3;
        
        return 0;
}

void ttt_print_board(int user)
{
        int i, temp, gameover = 0;
        char cells[9];
	char cell1[7];
	char cell2[7];
	char cell3[7];

        if (ustr[user].ttt_opponent == -3) {
                write_str(user,"TTT: EEEK.. seems we've lost your opponent!");
                return; 
        }
        if (((ustr[user].ttt_board)%(1<<17)) != ((ustr[ustr[user].ttt_opponent].ttt_board)%(1<<17))) {
                write_str(user,"TTT: EEEK.. seems like you're playing on different boards!");
                ttt_end_game(user, 0);
                return;
        }

        for (i = 0; i < 9; i++) {
                temp = ((ustr[user].ttt_board)>>(i*2))%4;
                switch (temp) {
                case 0: cells[i] = ' ';
                                break;
                case 1: cells[i] = 'O';
                                break;
                case 2: cells[i] = 'X';
                                break;
                default:
                        write_str(user,"TTT: EEEK.. corrupt board file!!");
                        return;
                }
        }

        if (ustr[user].ttt_board & TTT_MY_MOVE){
                write_str(user," It's your move - with the board as:");
                }
        else if (ustr[ustr[user].ttt_opponent].ttt_board & TTT_MY_MOVE){
                sprintf(mess," %s to move - with the board as:", ustr[ustr[user].ttt_opponent].say_name);
                write_str(user,mess);
                }
        else {
                write_str(user," The final board was:");
                gameover++;
        }
        sprintf(mess,"  ^LY+-------+  +-------+^");
        write_str(user,mess);

        for (i = 0; i < 9; i+=3) {
		if (cells[i]=='O') strcpy(cell1,"^HGO^");
		else if (cells[i]=='X') strcpy(cell1,"^HRX^");
		else if (cells[i]==' ') strcpy(cell1," ");
		if (cells[i+1]=='O') strcpy(cell2,"^HGO^");
		else if (cells[i+1]=='X') strcpy(cell2,"^HRX^");
		else if (cells[i+1]==' ') strcpy(cell2," ");
		if (cells[i+2]=='O') strcpy(cell3,"^HGO^");
		else if (cells[i+2]=='X') strcpy(cell3,"^HRX^");
		else if (cells[i+2]==' ') strcpy(cell3," ");

                sprintf(mess,"  ^LY|^ %s %s %s ^LY|^  ^LY|^ %d %d %d ^LY|^",
                cell1, cell2, cell3, i+1, i+2, i+3);
                write_str(user,mess);
        }

        sprintf(mess,"  ^LY+-------+  +-------+^");
        write_str(user,mess);
        if (!gameover) {
                if (ustr[user].ttt_board & TTT_AM_NOUGHT) {
                        write_str(user,"You are playing Os");
                } else {
                        write_str(user,"You are playing Xs");
                }
        }
        
}


void ttt_end_game(int user, int winner)
{
	int plyr = 1;

	if (ustr[user].ttt_opponent == -3) {
		write_str(user,"TTT: EEEK.. seems we've lost your opponent!");
		return;
	}

	if (!(ustr[user].ttt_board & TTT_AM_NOUGHT))
		plyr++;
	
	if (winner == 3) {
		sprintf(mess,TTT_DRAW, ustr[ustr[user].ttt_opponent].say_name);
		write_str(user,mess);
		sprintf(mess,TTT_DRAW, ustr[user].say_name);
		write_str(ustr[user].ttt_opponent,mess);
	} else if (winner && (winner != plyr)) {
		sprintf(mess,TTT_LOST, ustr[ustr[user].ttt_opponent].say_name);
		write_str(user,mess);
		sprintf(mess,TTT_WON, ustr[user].say_name);
		write_str(ustr[user].ttt_opponent,mess);
		ustr[user].ttt_killed = ustr[user].ttt_killed + 1;
		ustr[ustr[user].ttt_opponent].ttt_kills = ustr[ustr[user].ttt_opponent].ttt_kills + 1;
	} else if (winner) {
		sprintf(mess,TTT_WON, ustr[ustr[user].ttt_opponent].say_name);
		write_str(user,mess);
		sprintf(mess,TTT_LOST, ustr[user].say_name);
		write_str(ustr[user].ttt_opponent,mess);
		ustr[user].ttt_kills = ustr[user].ttt_kills + 1;
		ustr[ustr[user].ttt_opponent].ttt_killed = ustr[ustr[user].ttt_opponent].ttt_killed + 1;
	} else {
		sprintf(mess,TTT_ABORT1, ustr[ustr[user].ttt_opponent].say_name);
		write_str(user,mess);
		sprintf(mess,TTT_ABORT2, ustr[user].say_name);
		write_str(ustr[user].ttt_opponent,mess);
	}

	ustr[user].ttt_board &= ~TTT_MY_MOVE;
	ustr[ustr[user].ttt_opponent].ttt_board &= ~TTT_MY_MOVE;

	if (winner) {
	ttt_print_board(user);
	ttt_print_board(ustr[user].ttt_opponent);
	}
	
	ustr[ustr[user].ttt_opponent].ttt_opponent = -3;
	ustr[user].ttt_opponent = -3;
}

void ttt_new_game(int user, char *str)
{
int p2;
int i=0;
char name[ARR_SIZE];

	if (!strlen(str)) {
		write_str(user,"Usage: .ttt <user>");
		return;
	}

	sscanf(str, "%s", name);
	strtolower(name);

	p2=get_user_num(name, user);

        if (p2 == -1 )
     	{
      	not_signed_on(user,name);
      	return;
     	}
	
     if (!user_wants_message(p2,NERFS)) {
	write_str(user,"User is ignoring tic-tac-toes right now.");
	return;
	}

        /* See if p2 has user gagged */
        for (i=0; i<NUM_IGN_FLAGS; ++i) {
           if (TTTS==gagged_types[i]) {
		if (!gag_check(user,p2,0)) return;
             }
          }
        i=0;

	if (ustr[p2].afk) {
	   write_str(user,"User is AFK, wait until they come back.");
   	   return;
   	}

    if (!ustr[user].vis || !ustr[p2].vis) {
       write_str(user,"Both of you must be visible first.");
       return;
       }

    if (ustr[p2].afk)
     {
    if (ustr[p2].afk == 1) {
      if (!strlen(ustr[p2].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[p2].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[p2].say_name,ustr[p2].afkmsg);
      } 
     else {
      if (!strlen(ustr[p2].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[p2].say_name);
      else
      sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[p2].say_name,ustr[p2].afkmsg);
      }

       write_str(user,t_mess);
       return;
     }

	if (user==p2) {
		write_str(user,"You're not supposed to play with yourself!");
		return;
	}

/* can the person you're nerfing nerf back? */
for (i=0;sys[i].jump_vector!=-1;++i) {
        if (!strcmp(sys[i].command,".ttt")) break;
        }
     
if (ustr[p2].super < sys[i].su_com) {
  write_str(user,"That user doesn't have the .ttt command yet!");
  return;  
  }
	
	if (ustr[p2].ttt_opponent != -3) {
		sprintf(mess, TTT_PLAYING, ustr[p2].say_name);
		write_str(user,mess);
		return;
	}
	ustr[user].ttt_board = 0;
	ustr[p2].ttt_board = TTT_MY_MOVE + TTT_AM_NOUGHT;
	ustr[user].ttt_opponent = p2;
	ustr[p2].ttt_opponent = user;
	
	sprintf(mess,TTT_OFFERED,ustr[user].say_name);
	write_str(p2,mess);
	write_str(p2," Type: \".ttt abort\" to decline, or \".ttt <first move>\" to start");
	ttt_print_board(p2);
	ttt_print_board(user);
}

void ttt_make_move(int p, char *str)
{
	int winner, temp;
	
	if (!strlen(str)) {
		write_str(p,"You're playing a game, so");
		write_str(p,"Type: .ttt <square # to play>");
		write_str(p,"Type: \".ttt abort\" to abort this game");
		return;
	}
	if (((ustr[p].ttt_board)%(1<<17)) != ((ustr[ustr[p].ttt_opponent].ttt_board)%(1<<17))) {
		write_str(p,"Seems you're playing on different boards!");
		ttt_end_game(p, 0);
	}
	
	temp = atoi(str);

	if ((temp < 1) || (temp > 9)) {
		write_str(p,"Please play to a square on the board!!");
		ttt_print_board(p);
		return;
	}

	temp--;
	temp *= 2;
	
	if (ustr[p].ttt_board & (3<<temp)) {
		write_str(p,"Sorry, that square is already taken.. please choose another");
		ttt_print_board(p);
		return;
	}
	if (ustr[p].ttt_board & TTT_AM_NOUGHT) {
		ustr[p].ttt_board |= (1<<temp);
		ustr[ustr[p].ttt_opponent].ttt_board |= (1<<temp);
	}
	else {
		ustr[p].ttt_board |= (2<<temp);
		ustr[ustr[p].ttt_opponent].ttt_board |= (2<<temp);
	}

	ustr[p].ttt_board &= ~TTT_MY_MOVE;
	ustr[ustr[p].ttt_opponent].ttt_board |= TTT_MY_MOVE;
	
	winner = ttt_is_end(p);
	if (winner) {
		ttt_end_game(p, winner);
	}
	else {
		ttt_print_board(p);
		ttt_print_board(ustr[p].ttt_opponent);
	}
}

/* wrappers */

void ttt_print(int p)
{
	if (ustr[p].ttt_opponent == -3){
		write_str(p,"But you aren't playing a game!");
	}
	else
		ttt_print_board(p);
}

void ttt_abort(int p)
{
	if (ustr[p].ttt_opponent == -3){
		write_str(p,"But you aren't playing a game!");
	}
	else ttt_end_game(p, 0);
}

void ttt_cmd(int p, char *str)
{
	if (ustr[p].ttt_opponent == -3)
		ttt_new_game(p, str);
	else if (!strcmp(str, "abort"))
		ttt_abort(p);
	else if (!strcmp(str, "print"))
		ttt_print(p);
	else if (ustr[p].ttt_board & TTT_MY_MOVE)
		ttt_make_move(p, str);
	else {
		sprintf(mess, "Sorry, but %s gets to make the next move", ustr[ustr[p].ttt_opponent].say_name);
		write_str(p,mess);
		ttt_print_board(p);
	}
}
/*----------------------------------------------------------*/
/* End Tic-Tac-Toe routines				    */
/*----------------------------------------------------------*/


/*----------------------------------------------------------*/
/* Begin Hangman Routines				            */
/*----------------------------------------------------------*/
/* lets a user start, stop or check out their status of a game of hangman */
void play_hangman(int user, char *inpstr)
{
int i;

if (!strlen(inpstr)) {
  write_str(user,"Usage: .hangman <start|stop|status>");
  return;
  }
/* srand(time(0)); */
strtolower(inpstr);
i=0;
if (!strcmp("status",inpstr)) {
  if (ustr[user].hang_stage==-1) {
    write_str(user,"You haven't started a game of hangman yet.");
    return;
    }
  write_str(user,"Your current hangman game status is:");
  if (strlen(ustr[user].hang_guess)<1) sprintf(mess,hanged[ustr[user].hang_stage],ustr[user].hang_word_show,"None yet!");
  else sprintf(mess,hanged[ustr[user].hang_stage],ustr[user].hang_word_show,ustr[user].hang_guess);
  write_str(user,mess);
  return;
  }
if (!strcmp("stop",inpstr)) {
  if (ustr[user].hang_stage==-1) {
    write_str(user,"You haven't started a game of hangman yet.");
    return;
    }
  ustr[user].hang_stage=-1;
  ustr[user].hang_word[0]='\0';
  ustr[user].hang_word_show[0]='\0';
  ustr[user].hang_guess[0]='\0';
  write_str(user,"You stop your current game of hangman.");
  return;
  }
if (!strcmp("start",inpstr)) {
  if (ustr[user].hang_stage>-1) {
    write_str(user,"You have already started a game of hangman.");
    return;
    }
  get_hang_word(ustr[user].hang_word);
  strcpy(ustr[user].hang_word_show,ustr[user].hang_word);
  for (i=0;i<strlen(ustr[user].hang_word_show);++i) ustr[user].hang_word_show[i]='-';
  ustr[user].hang_stage=0;
  write_str(user,"Your current hangman game status is:");
  sprintf(mess,hanged[ustr[user].hang_stage],ustr[user].hang_word_show,"None yet!");
  write_str(user,mess);
  return;
  }
write_str(user,"Usage: .hangman <start|stop|status>");
}

/* returns a word from a list for hangman.
   this will save loading words into memory, and the list could be updated as and when
   you feel like it */
char *get_hang_word(char *aword)
{
int lines,cnt,i;
char filename[FILE_NAME_LEN];
FILE *fp;

lines=cnt=i=0;
sprintf(filename,"%s",HANGDICT);
lines=file_count_lines(filename);
/* srand(time(0)); */
cnt=rand()%lines;
if (!(fp=fopen(filename,"r"))) return("hangman");
fscanf(fp,"%s\n",aword);
while (!feof(fp)) {
  if (i==cnt) {
    fclose(fp);
    strtolower(aword);
    return aword;
    }
  ++i;
  fscanf(fp,"%s\n",aword);
  }
fclose(fp);
/* if no word was found, just return a generic word */
return("hangman");
}


/* Lets a user guess a letter for hangman */
void guess_hangman(int user, char *inpstr)
{
int count,i,blanks;

count=blanks=i=0;
if (!strlen(inpstr)) {
  write_str(user,"Usage: .guess <letter|word>");
  return;
  }
if (ustr[user].hang_stage==-1) {
  write_str(user,"You haven't started a game of hangman yet.");
  return;
  }
if (strlen(inpstr)>1) {
  strtolower(inpstr);
  if (!strcmp(ustr[user].hang_word,inpstr)) {
	strcpy(ustr[user].hang_word_show,ustr[user].hang_word);
	blanks=0;
	goto HDONE;
  }  
  else {
  ustr[user].hang_stage++;
  write_str(user, HANG_BADGUESS);
  if (ustr[user].hang_stage>=7) strcpy(ustr[user].hang_word_show,ustr[user].hang_word);

  sprintf(mess,hanged[ustr[user].hang_stage],ustr[user].hang_word_show,ustr[user].hang_guess);
  write_str(user,mess);
  if (ustr[user].hang_stage>=7) {
    write_str(user, HANG_LOST);
    ustr[user].hang_losses++;
    ustr[user].hang_stage=-1;
    ustr[user].hang_word[0]='\0';
    ustr[user].hang_word_show[0]='\0';
    ustr[user].hang_guess[0]='\0';
    }
  else {
    sprintf(mess,"You have ^%d^ guess%s left",7-ustr[user].hang_stage,(7-ustr[user].hang_stage)==1 ? "" : "es");
    write_str(user,mess);
    }
  return;
  }
 }
else {
 strtolower(inpstr);
 }

if (strstr(ustr[user].hang_guess,inpstr)) {
  ustr[user].hang_stage++;
  write_str(user, HANG_GUESSED);
  if (ustr[user].hang_stage>=7) strcpy(ustr[user].hang_word_show,ustr[user].hang_word);

  sprintf(mess,hanged[ustr[user].hang_stage],ustr[user].hang_word_show,ustr[user].hang_guess);
  write_str(user,mess);
  if (ustr[user].hang_stage>=7) {
    write_str(user, HANG_LOST);
    ustr[user].hang_losses++;
    ustr[user].hang_stage=-1;
    ustr[user].hang_word[0]='\0';
    ustr[user].hang_word_show[0]='\0';
    ustr[user].hang_guess[0]='\0';
    }
  else {
    sprintf(mess,"You have ^%d^ guess%s left",7-ustr[user].hang_stage,(7-ustr[user].hang_stage)==1 ? "" : "es");
    write_str(user,mess);
    }
  return;
  }
for (i=0;i<strlen(ustr[user].hang_word);++i) {
  if (ustr[user].hang_word[i] == inpstr[0]) {
    ustr[user].hang_word_show[i]=ustr[user].hang_word[i];
    ++count;
    }
  if (ustr[user].hang_word_show[i]=='-') ++blanks;
  }
strcat(ustr[user].hang_guess,inpstr);
if (!count) {
  ustr[user].hang_stage++;
  write_str(user, HANG_BADLETTER);
  if (ustr[user].hang_stage>=7) strcpy(ustr[user].hang_word_show,ustr[user].hang_word);

  sprintf(mess,hanged[ustr[user].hang_stage],ustr[user].hang_word_show,ustr[user].hang_guess);
  write_str(user,mess);
  if (ustr[user].hang_stage>=7) {
    write_str(user, HANG_LOST);
    ustr[user].hang_losses++;
    ustr[user].hang_stage=-1;
    ustr[user].hang_word[0]='\0';
    ustr[user].hang_word_show[0]='\0';
    ustr[user].hang_guess[0]='\0';
    }
  else {
    sprintf(mess,"You have ^%d^ guess%s left",7-ustr[user].hang_stage,(7-ustr[user].hang_stage)==1 ? "" : "es");
    write_str(user,mess);
    }
  return;
  }
if (count==1) sprintf(mess,HANG_1OCCUR,inpstr);
else sprintf(mess,HANG_MOCCUR,count,inpstr);
write_str(user,mess);
HDONE:
sprintf(mess,hanged[ustr[user].hang_stage],ustr[user].hang_word_show,ustr[user].hang_guess);
write_str(user,mess);
if (!blanks) {
  write_str(user, HANG_WON);
  ustr[user].hang_wins++;
  ustr[user].hang_stage=-1;
  ustr[user].hang_word[0]='\0';
  ustr[user].hang_word_show[0]='\0';
  ustr[user].hang_guess[0]='\0';
  }
else {
    sprintf(mess,"You have ^%d^ guess%s left",7-ustr[user].hang_stage,(7-ustr[user].hang_stage)==1 ? "" : "es");
    write_str(user,mess);
  }
}


/*---------------------------------------------------------*/
/*    SOCIAL SECTION                                       */
/*---------------------------------------------------------*/
/*** List social commands ***/
void list_socs(int user)
{
int c=0;
int nl=0;

write_str(user,"+-------------------+");
write_str(user," Socials Available");
write_str(user,"+-------------------+");

nl = 0;

/* All socials have a type of NONE in the commands structure, so only
   show those */

for (c=0; sys[c].su_com != -1 ;++c) {
   if (sys[c].type==NONE) {
        sprintf(mess,"%-11.11s",sys[c].command);
        mess[0]=' ';
        if (nl== -1)
          {write_str_nr(user, "  ");
           nl=0;
          }
        write_hilite_nr(user,mess);
        ++nl;
        if (nl==5)
          {
            write_str(user," "); 
            nl= 0;
          }
       }   /* end of if */
   else continue;
  }   /* end of for */
write_str(user," ");
write_str(user," ");

}

/*** Function for all socials based on type ***/
void socials(int user, char *inpstr, int type)
{
int u;
int in_room=0;
char name[100],nametemp[100];
char other_user[ARR_SIZE];

if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

if (!strlen(inpstr)) {
       switch(type) {
	case 1: emote(user,"hugs everyone.\0");  break;
	case 2: emote(user,"laughs!\0");  break;
	case 3: emote(user,"pokes everyone.\0");  break;
	case 4: emote(user,"tickles everyone!\0");  break;
	case 5: emote(user,"blows everyone a big kiss!\0");  break;
	case 6: emote(user,"thwaps everyone in the room!\0");  break;
	case 7: emote(user,"bops everyone in the room!\0");  break;
	case 8: emote(user,"tackles everyone in a tackle-frenzy!\0"); break;
	case 9: emote(user,"smirks at everyone.\0");  break;
	case 10: emote(user,"licks everyone in sight! Dog imitation!\0"); break;
	case 11: emote(user,"smiles brightly at everyone\0");  break;
        case 12: emote(user,"chuckles insanely at everyone\0"); break;
        case 13: emote(user,"coughs up a lung in everyone's general direction\0"); break;
        case 14: emote(user,"dances with everyone in the room!\0"); break;
        case 15: emote(user,"DOH's!\0"); break;
        case 16: emote(user,"turns to everyone in the room and says \"Hey baby\"\0"); break;
        case 17: emote(user,"gooses everyone in the room..pervert!\0"); break;
        case 18: emote(user,"grabs everyone and SCREAMS!\0"); break;
        case 19: emote(user,"growls at everyone\0"); break;
        case 20: emote(user,"hisses at everyone like a snake, hisssssss\0"); break;
        case 21: emote(user,"turns to everyone and says \"Yo mama sleeps with my dog!\"\0"); break;
        case 22: emote(user,"kicks everyone in the room!\0"); break;
        case 23: emote(user,"laughs out loud!\0"); break;
        case 24: emote(user,"shakes their head(s) at everyone\0"); break;
        case 25: emote(user,"shoves everyone to the ground!\0"); break;
        case 26: emote(user,"slaps everyone in the room!\0"); break;
        case 27: emote(user,"whines to everyone..big baby.\0"); break;
        case 28: emote(user,"winks at everyone in the room.\0"); break;
        case 29: emote(user,"WOOHOO's!\0"); break;
        case 30: emote(user,"starts flinging chicken at everyone!\0"); break;
        case 31: emote(user,"puts everyone in a headlock and gives them a noogie\0"); break;
        case 32: emote(user,"runs around the room giving atomic wedgies!\0"); break;
       }  /* end of switch */
   }

/* One case for every social..check if victim is in room, then check
   is victim is user, if not, emote to the room..if victim is not in the
   room, semote the action */

else {
	sscanf(inpstr,"%s ",other_user);
	strtolower(other_user);
	if ((u=get_user_num(other_user,user)) == -1 ) {
		not_signed_on(user,other_user);
		return;
            }
            if (ustr[user].area==ustr[u].area) in_room=1;

	if (!ustr[u].vis) {
	strcpy(nametemp,INVIS_ACTION_LABEL);
	nametemp[0]=tolower((int)nametemp[0]);
	strcpy(name,nametemp);
	}
	else {
	strcpy(name,ustr[u].say_name);
	}

       switch(type) {
           case 1: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"hugs theirself.");
                   else
   		    sprintf(t_mess,"hugs %s warmly",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," hugs you warmly");
		   semote(user,other_user);
		   }
		   break;
           case 2: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"laughs at themselves.");
                   else
   		    sprintf(t_mess,"laughs at %s.",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," laughs!");
		   semote(user,other_user);
		   }
		   break;
           case 3: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"pokes themselves.");
                   else
   		    sprintf(t_mess,"pokes %s in some choice spots",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," pokes you.");
		   semote(user,other_user);
		   }
		   break;
           case 4: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"tickles themselves for some strange reason");
                   else
   		    sprintf(t_mess,"tickle-attacks %s!",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," tickles you!");
		   semote(user,other_user);
		   }
		   break;
           case 5: if (in_room) {
                   if (user==u) {
                     write_str(user,"You cant kiss yourself!");
                     }
                   else {
  		    sprintf(t_mess,"kisses %s!",name);
		    emote(user,t_mess);
                    }
		   }
		   else {
		   strcat(other_user," kisses you!");
		   semote(user,other_user);
		   }
		   break;
           case 6: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"thwaps theirself just for the hell of it!");
                   else
   		    sprintf(t_mess,"thwaps %s into the ground!",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," thwaps you!");
		   semote(user,other_user);
		   }
		   break;
           case 7: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"bops theirself on the head!");
                   else
   		    sprintf(t_mess,"bops %s on the head!",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," bops you on the head!");
		   semote(user,other_user);
		   }
		   break;
           case 8: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"tried to tackle themselves! Call the men in white.");
                   else
   		    sprintf(t_mess,"tackles %s to the ground!",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," tackles you to the ground!");
		   semote(user,other_user);
		   }
		   break;
           case 9: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"tried to smirk at themselves! Call the men in white.");
                   else
   		    sprintf(t_mess,"smirks at %s",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," smirks at you");
		   semote(user,other_user);
		   }
		   break;
           case 10: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"sticks their tongue out and tries to lick themselves! Oooooook.");
                   else
   		    sprintf(t_mess,"licks %s on the cheek! Ewwww!",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," licks you on the cheek!");
		   semote(user,other_user);
		   }
		   break;
           case 11: if (in_room) {
                   if (user==u) 
                    strcpy(t_mess,"tried to smile at themselves! Oooooook.");
                   else
   		    sprintf(t_mess,"smiles innocently at %s",name);

		   emote(user,t_mess);
		   }
		   else {
		   strcat(other_user," smiles innocently");
		   semote(user,other_user);
		   }
		   break;
           case 12: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"chuckles to themself.");
                   else   
                    sprintf(t_mess,"chuckles at %s",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," chuckles at ya");
                   semote(user,other_user);
                   }
                   break;
           case 13: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"coughs on themself.");        
                   else
                    sprintf(t_mess,"coughs on %s, how disgusting",name);       

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," coughs in your direction");   
                   semote(user,other_user);
                   }
                   break;
           case 14: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"dances around by themself.");        
                   else
                    sprintf(t_mess,"dances around the room with %s",name);       

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," grabs your hand and dances with you");
                   semote(user,other_user);
                   }
                   break;
           case 15: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"doh's to themself.");        
                   else
                    sprintf(t_mess,"looks at %s and doh's",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," doh's!");
                   semote(user,other_user);
                   }
                   break;
           case 16: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"flirts with themself. Makes ya wonder.");
                   else
                    sprintf(t_mess,"turns to %s and says \"Hey baby\"",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," turns to you and says \"Hey baby\"");
                   semote(user,other_user);
                   }
                   break;
           case 17: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"gooses themself. Yeah.. ok. Weirdo!");
                   else
                    sprintf(t_mess,"gooses %s!",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," gooses ya!");
                   semote(user,other_user);
                   }
                   break;
           case 18: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"grabs themself like Michael Jackson!");
                   else
                    sprintf(t_mess,"grabs %s and SCREAMS!",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," grabs you and screams!");     
                   semote(user,other_user);
                   }
                   break;
           case 19: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"growls at themself.");
                   else
                    sprintf(t_mess,"growls at %s..kinky!",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," growls at you");
                   semote(user,other_user);
                   }
                   break;
           case 20: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"hisses at themself.");
                   else
                    sprintf(t_mess,"hisses at %s",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," hisses at you");
                   semote(user,other_user);
                   }
                   break;
           case 21: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"insults themself. Yep, must be bored.");
                   else
                    sprintf(t_mess,"looks at %s and says \"Yo mama sleeps with my dog!\"",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," looks at you and says \"Yo mama sleeps with my dog!\"");
                   semote(user,other_user);
                   }
                   break;
           case 22: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"kicks themself.");
                   else
                    sprintf(t_mess,"kicks %s! OUCH!",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," kicks you! OUCH!");
                   semote(user,other_user);
                   }
                   break;
           case 23: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"laughs out loud at themself!");    
                   else
                    sprintf(t_mess,"laughs out loud at %s",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," laughs out loud at you!");
                   semote(user,other_user);
                   }
                   break;
           case 24: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"shakes their head(s) at themself.");    
                   else
                    sprintf(t_mess,"shakes their head(s) at %s",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," shakes their head(s) at you");
                   semote(user,other_user);
                   }
                   break;
           case 25: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"shoves themself to the ground. Idiot.");    
                   else
                    sprintf(t_mess,"shoves %s to the ground!",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," shoves you to the ground!");
                   semote(user,other_user);
                   }
                   break;
           case 26: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"slaps themself.");    
                   else
                    sprintf(t_mess,"slaps %s! *SMACK*",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," slaps you!");
                   semote(user,other_user);
                   }
                   break;
           case 27: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"whines to themself.");    
                   else
                    sprintf(t_mess,"whines to %s",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," whines to you");
                   semote(user,other_user);
                   }
                   break;
           case 28: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"winks at themself.");    
                   else
                    sprintf(t_mess,"winks at %s",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," winks at you");
                   semote(user,other_user);
                   }
                   break;
           case 29: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"WOOHOO's to themself.");    
                   else
                    sprintf(t_mess,"WOOHOO's at %s",name);

                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," WOOHOO's at you");
                   semote(user,other_user);
                   }
                   break;
           case 30: if (in_room) {
                   if (user==u)
                    strcpy(t_mess,"examines a piece of chicken then hits themselves in the face with it repeatedly.");
                   else  
                    sprintf(t_mess,"flings a piece of chicken at %s!",name);
                    
                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," flings a piece of chicken at you!");
                   semote(user,other_user);
                   }
                   break;
           case 31: if (in_room) {
                   if (user==u)
                     strcpy(t_mess,"puts themselves in a headlock and tries to perform a self-noogie. What's this person on?");
                   else
                     sprintf(t_mess,"puts %s in a headlock and gives them a noogie!",name);
                   
                   emote(user,t_mess);
                   }
                   else {
                   strcat(other_user," gives you a noogie!");
                   semote(user,other_user);
                   }
                   break;
           case 32: if (in_room) {
                   if (user==u)
                     strcpy(t_mess,"twists around, grabs some underwear, and pulls them straight up..going head over heals in the process!");
                   else
                     sprintf(t_mess,"runs behind %s and pulls their underwear up over their head. Atomic wedgie!",name);
                   
                   emote(user,t_mess);
                   echo(user,"Now doesn't that feel goooooood?\0");
                   }
                   else {
                   strcat(other_user," pulls your underwear up over your head for a patented atmoic wedgie! Now doesn't that feel goooooood?");
                   semote(user,other_user);
                   }   
                   break;
           default: return;  break;
       }  /* end of switch */
   }  /* end of else */

}

/* MEMORY SUB FUNCTIONS FOR memcheck() */
long temp_mem_get(void)
{
int i=0;
int j=0;
int macro=0;
int abbrev1=0;
int con=0;
long tot_mem=0;

     for (i=0;i<NUM_MACROS;++i) {
	 macro += strlen(t_ustr.Macros[i].name);
	 macro += strlen(t_ustr.Macros[i].body);
	 }
     for (i=0;i<NUM_ABBRS;++i) {
	 abbrev1 += strlen(t_ustr.custAbbrs[i].abbr);
	 abbrev1 += strlen(t_ustr.custAbbrs[i].com);
	 }
     for (j=0;j<NUM_LINES+1;++j) {
	 con += strlen(t_ustr.conv[j]);
	 }

tot_mem=
	strlen(t_ustr.name)
	+strlen(t_ustr.password)
	+strlen(t_ustr.desc)
	+strlen(t_ustr.email_addr)
	+strlen(t_ustr.sex)
	+strlen(t_ustr.site)
	+strlen(t_ustr.init_date)
	+strlen(t_ustr.init_site)
	+strlen(t_ustr.init_netname)
	+strlen(t_ustr.last_date)
	+strlen(t_ustr.last_site)
	+strlen(t_ustr.last_name)
	+strlen(t_ustr.say_name)
	+strlen(t_ustr.entermsg)
	+strlen(t_ustr.exitmsg)
	+strlen(t_ustr.succ)
	+strlen(t_ustr.fail)
	+strlen(t_ustr.homepage)
	+strlen(t_ustr.creation)
	+strlen(t_ustr.security)
	+strlen(t_ustr.login_name)
	+strlen(t_ustr.login_pass)
	+strlen(t_ustr.phone_user)
	+strlen(t_ustr.mutter)
	+strlen(t_ustr.page_file)
	+2 /* pro_enter */
	+2 /* roomd_enter */
	+2 /* vote_enter */
	+2 /* locked */
	+2 /* suspended */
	+2 /* area */
	+2 /* shout */
	+2 /* igtell */
	+2 /* color */
	+2 /* clrmail */ 
	+2 /* sock */
	+2 /* monitor */
	+2 /* time */ 
	+2 /* vis */
	+2 /* super */
	+2 /* invite */
	+2 /* last_input */
	+2 /* warning_given */
	+2 /* logging_in */
	+2 /* attleft */
	+2 /* file_posn */
	+strlen(t_ustr.net_name)
	+macro /* all macros, since a list */
	+abbrev1 /* all abbreviations, since a list */
	+2 /* conv_count */
	+con   /* all conv buffers, since a list */
	+2 /* cat_mode */
	+2 /* rows */
	+2 /* cols */
	+2 /* car_return */
	+2 /* abbrs */
	+2 /* white_space */
	+2 /* line_count */
	+2 /* number_lines */
	+2 /* times_on */
	+2 /* afk */
	+2 /* lockafk */
	+2 /* upper */
	+2 /* lower */
	+2 /* aver */
	+4 /* totl */
	+2 /* autor */
	+2 /* autof */
	+2 /* automsgs */
	+2 /* gagcomm */
	+2 /* semail */
	+2 /* quote */
	+2 /* hilite */
	+2 /* new_mail */
	+4 /* numcoms */
	+2 /* mail_num */
	+2 /* numbering */
	+strlen(t_ustr.flags)
	+strlen(t_ustr.real_id)
	+1 /* attach_port */
	+2 /* char_buffer_size */
	+strlen(t_ustr.char_buffer)
	+2 /* friend_num */
	+2 /* revokes_num */
	+2 /* gag_num */
	+2 /* nerf_shots */
	+2 /* nerf_energy */
	+2 /* nerf_kills */
	+2 /* nerf_killed */
	+2 /* ttt_kills */
	+2 /* ttt_killed */
	+2 /* ttt_opponent */
	+2 /* ttt_playing */
	+strlen(t_ustr.icq)
	+strlen(t_ustr.miscstr1)
	+strlen(t_ustr.miscstr2)
	+strlen(t_ustr.miscstr3)
	+strlen(t_ustr.miscstr4)
	+2 /* pause_login */
	+2 /* miscnum2 */
	+2 /* miscnum3 */
	+2 /* miscnum4 */
	+2 /* miscnum5 */
	+2 /* hang_wins */
	+2 /* hang_losses */
	+2 /* hang_stage */
	+strlen(t_ustr.hang_word)
	+strlen(t_ustr.hang_word_show)
	+strlen(t_ustr.hang_guess)
	+2 /* passhid */
	+2 /* pbreak */
	+2 /* beeps */
	+4 /* rawtime */
	+2 /* muz_time */
	+2 /* xco_time */
	+2 /* gag_time */
	+2 /* frog */
	+2 /* frog_time */
	+2 /* anchor */
	+2 /* anchor_time */
	+2 /* promote */
	+2 /* tempsuper */
	+strlen(t_ustr.home_room)
	+strlen(t_ustr.webpic)
	+strlen(t_ustr.afkmsg);

       if (t_ustr.pro_enter)
         tot_mem += (82*PRO_LINES);
       else if (t_ustr.roomd_enter)
         tot_mem += (82*ROOM_DESC_LINES);
       else if (t_ustr.vote_enter)
         tot_mem += (82*VOTE_LINES);

	macro=0;
	con=0;
	i=0;
	j=0;

  return tot_mem;
}


long user_mem_get(void)
{
int u=0;
int i=0;
int j=0;
int macro=0;
int abbrev1=0;
int con=0;
long userm=0;
long tot_mem=0;

for (u=0;u<MAX_USERS;++u) {
     for (i=0;i<NUM_MACROS;++i) {
	 macro += strlen(ustr[u].Macros[i].name);
	 macro += strlen(ustr[u].Macros[i].body);
	 }
     for (i=0;i<NUM_ABBRS;++i) {
	 abbrev1 += strlen(ustr[u].custAbbrs[i].abbr);
	 abbrev1 += strlen(ustr[u].custAbbrs[i].com);
	 }
     for (j=0;j<NUM_LINES+1;++j) {
	 con += strlen(ustr[u].conv[j]);
	 }
     userm=   
	strlen(ustr[u].name)
	+strlen(ustr[u].password)
	+strlen(ustr[u].desc)
	+strlen(ustr[u].email_addr)
	+strlen(ustr[u].sex)
	+strlen(ustr[u].site)
	+strlen(ustr[u].init_date)
	+strlen(ustr[u].init_site)
	+strlen(ustr[u].init_netname)
	+strlen(ustr[u].last_date)
	+strlen(ustr[u].last_site)
	+strlen(ustr[u].last_name)
	+strlen(ustr[u].say_name)
	+strlen(ustr[u].entermsg)
	+strlen(ustr[u].exitmsg)
	+strlen(ustr[u].succ)
	+strlen(ustr[u].fail)
	+strlen(ustr[u].homepage)
	+strlen(ustr[u].creation)
	+strlen(ustr[u].security)
	+strlen(ustr[u].login_name)
	+strlen(ustr[u].login_pass)
	+strlen(ustr[u].phone_user)
	+strlen(ustr[u].mutter)
	+strlen(ustr[u].page_file)
	+2  /* pro_enter */
	+2  /* roomd_enter */
	+2  /* vote_enter */
	+2 /* locked */
	+2 /* suspended */
	+2 /* area */
	+2 /* shout */
	+2 /* igtell */
	+2 /* color */
	+2 /* clrmail */ 
	+2 /* sock */
	+2 /* monitor */
	+2 /* time */ 
	+2 /* vis */
	+2 /* super */
	+2 /* invite */
	+2 /* last_input */
	+2 /* warning_given */
	+2 /* logging_in */
	+2 /* attleft */
	+2 /* file_posn */
	+strlen(ustr[u].net_name)
	+macro
	+abbrev1 /* all abbreviations, since a list */
	+2 /* conv_count */
	+con
	+2 /* cat_mode */
	+2 /* rows */
	+2 /* cols */
	+2 /* car_return */
	+2 /* abbrs */
	+2 /* white_space */
	+2 /* line_count */
	+2 /* number_lines */
	+2 /* times_on */
	+2 /* afk */
	+2 /* lockafk */
	+2 /* upper */
	+2 /* lower */
	+2 /* aver */
	+4 /* totl */
	+2 /* autor */
	+2 /* autof */
	+2 /* automsgs */
	+2 /* gagcomm */
	+2 /* semail */
	+2 /* quote */
	+2 /* hilite */
	+2 /* new_mail */
	+4 /* numcoms */
	+2 /* mail_num */
	+2 /* numbering */
	+strlen(ustr[u].flags)
	+strlen(ustr[u].real_id)
	+1 /* attach_port */
	+2 /* char_buffer_size */
	+strlen(ustr[u].char_buffer)
	+2 /* friend_num */
	+2 /* revokes_num */
	+2 /* gag_num */
	+2 /* nerf_shots */
	+2 /* nerf_energy */
	+2 /* nerf_kills */
	+2 /* nerf_killed */
	+2 /* ttt_kills */
	+2 /* ttt_killed */
	+2 /* ttt_opponent */
	+2 /* ttt_playing */
	+strlen(ustr[u].icq)
	+strlen(ustr[u].miscstr1)
	+strlen(ustr[u].miscstr2)
	+strlen(ustr[u].miscstr3)
	+strlen(ustr[u].miscstr4)
	+2 /* pause_login */
	+2 /* miscnum2 */
	+2 /* miscnum3 */
	+2 /* miscnum4 */
	+2 /* miscnum5 */
	+2 /* hang_wins */
	+2 /* hang_losses */
	+2 /* hang_stage */
	+strlen(ustr[u].hang_word)
	+strlen(ustr[u].hang_word_show)
	+strlen(ustr[u].hang_guess)
	+2 /* passhid */
	+2 /* pbreak */
	+2 /* beeps */
	+4 /* rawtime */
	+2 /* muz_time */
	+2 /* xco_time */
	+2 /* gag_time */
	+2 /* frog */
	+2 /* frog_time */
	+2 /* anchor */
	+2 /* anchor_time */
	+2 /* promote */
	+2 /* tempsuper */
	+strlen(ustr[u].home_room)
	+strlen(ustr[u].webpic)
	+strlen(ustr[u].afkmsg);

       if (ustr[u].pro_enter)
         userm += (82*PRO_LINES);
       else if (ustr[u].roomd_enter)
         userm += (82*ROOM_DESC_LINES);
       else if (ustr[u].vote_enter)
         userm += (82*VOTE_LINES);

       tot_mem += userm;
       userm=0;
       macro=0;
       con=0;
       i=0;
       j=0;
    }
 return tot_mem;
}

long area_mem_get(void)
{
int a=0;
int aream=0;
long tot_mem=0;

for (a=0;a<MAX_AREAS;++a) {
     aream=   
	strlen(astr[a].name)
	+strlen(astr[a].move)
	+strlen(astr[a].topic)
	+2  /* private   */
	+2  /* hidden    */
	+2  /* secure    */
	+2  /* mess_num  */
	+2  /* conv_line */
        +2; /* atmos     */

     tot_mem += aream;
     aream=0;
   }
 return tot_mem;
}

long conv_mem_get(void)
{
int i=0;
int j=0;
int conv_buf=0;
long tot_mem=0;

     for (i=0;i<MAX_AREAS;++i) {
          for (j=0;j<NUM_LINES;++j) {
             conv_buf += strlen(conv[i][j]);
             }
	 tot_mem += conv_buf;
         conv_buf=0;
         j=0;
	 }

conv_buf=0;
return tot_mem;
}

long wiz_mem_get(void)
{
int i=0;
long tot_mem=0;

     for (i=0;i<NUM_LINES;++i) {
             tot_mem += strlen(bt_conv[i]);
	 }

return tot_mem;
}

long shout_mem_get(void)
{
int i=0;
long tot_mem=0;

     for (i=0;i<NUM_LINES;++i) {
             tot_mem += strlen(sh_conv[i]);
	 }

return tot_mem;
}


/** Check on memory usage and allocations ***/
void memcheck(int user)
{
long tot1=0;
long tot2=0;
long mfree=0;
float per;

write_str(user,"+-----------------------------------------------------------------------+");
write_str(user,"|                          Memory Usage (in bytes)                      |");
write_str(user,"+-----------------------------------------------------------------------+");
write_str(user,"|                                                                       |");
write_str(user,"|  Type                Allocated      Used      Free     Capacity       |");

 per=((float)user_mem_get()/(float)sizeof(ustr))*100;
 mfree=sizeof(ustr)-user_mem_get();
 if (mfree < 0) mfree=0;
sprintf(mess,"|  User Data           %-6ld         %-6ld    %-6ld      %5.1f%%      |",
(long)sizeof(ustr),user_mem_get(),mfree,per);
write_str(user,mess);
 per=0;
 mfree=0;
 per=((float)conv_mem_get()/(float)sizeof(conv))*100;
 mfree=sizeof(conv)-conv_mem_get();
 if (mfree < 0) mfree=0;
sprintf(mess,"|  Area Convo Bufs     %-6ld         %-6ld    %-6ld      %5.1f%%      |",
(long)sizeof(conv),conv_mem_get(),mfree,per);
write_str(user,mess);
 per=0;
 mfree=0;
 per=((float)temp_mem_get()/(float)sizeof(t_ustr))*100;
 mfree=sizeof(t_ustr)-temp_mem_get();
 if (mfree < 0) mfree=0;
sprintf(mess,"|  Temp User Data      %-6ld         %-6ld    %-6ld      %5.1f%%      |",
(long)sizeof(t_ustr),temp_mem_get(),mfree,per);
write_str(user,mess);
 per=0;
 mfree=0;
 mfree=sizeof(astr)-area_mem_get();
 if (mfree < 0) mfree=0;
 per=((float)area_mem_get()/(float)sizeof(astr))*100;
sprintf(mess,"|  Area Data           %-6ld         %-6ld    %-6ld      %5.1f%%      |", 
(long)sizeof(astr),area_mem_get(),mfree,per);
write_str(user,mess);
 per=0;
 mfree=0;
 per=((float)wiz_mem_get()/(float)sizeof(bt_conv))*100;
 mfree=sizeof(bt_conv)-wiz_mem_get();
 if (mfree < 0) mfree=0;
sprintf(mess,"|  Wiz Convo Bufs      %-6ld         %-6ld    %-6ld      %5.1f%%      |",
(long)sizeof(bt_conv),wiz_mem_get(),mfree,per);
write_str(user,mess);
 per=0;
 mfree=0;
 per=((float)shout_mem_get()/(float)sizeof(sh_conv))*100;
 mfree=sizeof(sh_conv)-shout_mem_get();
 if (mfree < 0) mfree=0;
sprintf(mess,"|  Shout Convo Bufs    %-6ld         %-6ld    %-6ld      %5.1f%%      |",
(long)sizeof(sh_conv),shout_mem_get(),mfree,per);
write_str(user,mess);
 per=0;

 tot1=sizeof(ustr)+sizeof(astr)+sizeof(t_ustr)+sizeof(conv)+sizeof(bt_conv)+sizeof(sh_conv);
 tot2=user_mem_get()+area_mem_get()+temp_mem_get()+conv_mem_get()+wiz_mem_get()+shout_mem_get();

write_str(user,"|                                                                       |");
 per=((float)tot2/(float)tot1)*100;
 mfree=0;
 mfree=tot1-tot2;
 if (mfree < 0) mfree=0;

sprintf(mess,"|   TOTALS             %-6lu         %-6lu    %-6lu      %5.1f%%      |", tot1,tot2,mfree,per);
 per=0;
 mfree=0;
write_str(user,mess);
write_str(user,"+-----------------------------------------------------------------------+");

 tot1=0;
 tot2=0;
 per=0;
 mfree=0;
}


/***        Add an atmosphere to the list        ***/
/* V 1.2 adds numerical sorting of probabilities   */
void add_atmos(int user, char *inpstr)
{
int i=1, warn=0, lim=0, fprob, probint;
char filename[FILE_NAME_LEN], filename2[FILE_NAME_LEN];
char temp[ATMOS_LEN+11];
char temp2[ARR_SIZE];
FILE *fp, *fp2;
time_t tm;

sprintf(filename, "%s/%s.atmos",datadir, astr[ustr[user].area].name);

sscanf(inpstr,"%s ",temp2);
if (!temp2[0]) {
	write_str(user, "Usage: .addatmos <chance to occur (1-99)> <atmosphere>");
	return;
	}
strcpy(temp2, strip_color(temp2));
probint=atoi(temp2);
if ((probint<1) || (probint>99)) {
	write_str(user, "The chance factor must be between 1 and 99");
	return;
	}
remove_first(inpstr);
if (!inpstr[0]) {
	write_str(user, "Usage: .addatmos <chance to occur (1-99)> <atmosphere>");
	return;
	}
/* inpstr now points to atmosphere string - need to truncate it if too long */
if (strlen(inpstr)>ATMOS_LEN) inpstr[ATMOS_LEN]=0;

if ((fp=fopen(filename, "r"))) {
	/* count atmospheres */
	fgets(temp, 80, fp);
	while(!feof(fp)) {
		fgets(temp, ATMOS_LEN+10, fp);
		i++;
		fgets(temp, 80, fp);
		}
	fclose(fp);
	if (i>MAX_ATMOS) {
                sprintf(mess,"You already have the maximum of %d atmospheres - you must delete one first",MAX_ATMOS);
                write_str(user,mess);
		return;
		}
	}

if ((fp=fopen(filename, "r"))) {
  strcpy(filename2, get_temp_file());
  if (!(fp2=fopen(filename2, "w"))) {
	write_str(user, "Couldn't open temp file for atmos writing");
	print_to_syslog("ERROR: Couldn't open temp file in add_atmos() for writing\n");
	return;
	}
  
        /* get prob */
        temp[0]=0;
	fgets(temp, 80, fp);
        fprob=atoi(temp);
        if (probint<=fprob) {
          if (probint==fprob) warn=1;
          fprintf(fp2, "%d\n%s\n", probint, inpstr);
          lim=0;
          goto END;
          }

	while(!feof(fp)) {
                if (!lim) {
                temp[0]=0;
		fgets(temp, ATMOS_LEN+10, fp);
                fprintf(fp2, "%d\n%s", fprob, temp);
                lim=1;
                }
                else {
                /* get probability */
                temp[0]=0;
		fgets(temp, 80, fp);
                lim=0;
	        fprob=atoi(temp);
                if (probint<=fprob) {
                  if (probint==fprob) warn=1;
                  lim=0;
                  fprintf(fp2, "%d\n%s\n", probint, inpstr);
                  goto END;
                  }
                } /* end of else */
            } /* end of while */
        
        /* its greater than everything else, so just append it */
        fprintf(fp2, "%d\n%s\n", probint, inpstr);     
        goto THEEND;

END:
        /* write out probability */
        fprintf(fp2, "%d\n", fprob);

	while(!feof(fp)) {
               if (!lim) {
                temp[0]=0;
		fgets(temp, ATMOS_LEN+10, fp);
                fputs(temp, fp2);
                lim=1;
                }
               else {
                temp[0]=0;
		fgets(temp, 80, fp);
                fputs(temp, fp2);
                lim=0;
                }
           } /* end of while */

THEEND:
       fclose(fp);
       fclose(fp2);
       if (rename(filename2, filename)==-1) {
	write_str(user, "Couldn't rename new atmosphere file");
	print_to_syslog("ERROR: Couldn't rename temp file to atmosphere file in add_atmos()\n");
	return;
	}
      } /* end of file open if */
else {
  /* No atmosphere file exists..create one with new atmosphere */
  if (!(fp=fopen(filename, "a"))) {
	write_str(user, "Couldn't add new atmosphere to the file");
	sprintf(mess, "ERROR: Couldn't open %s's atmosphere file in add_atmos()\n", ustr[user].name);
	print_to_syslog(mess);
	return;
	}

  fprintf(fp, "%d\n%s\n", probint, inpstr);
  fclose(fp);
 } /* end of else */

if (i==1) strcpy(mess, "You now have 1 atmosphere");
else sprintf(mess, "You now have %d atmospheres", i);
write_str(user, mess);

if (warn)
 write_str(user,"WARNING: Two distinct atmospheres have the same probability number. Doing this disregards the latter since a random number will match them equally.");

time(&tm);
strcpy(temp2, ctime(&tm));
sprintf(mess,"%s:ATMOS added to %s by %s\n",temp2,astr[ustr[user].area].name,ustr[user].say_name);
print_to_syslog(mess);

}


/*** Remove an atmosphere from the list ***/
void del_atmos(int user, char *inpstr)
{
char filename[FILE_NAME_LEN], tempfile[FILE_NAME_LEN];
char atmos[ATMOS_LEN+11], probch[81];
FILE *fp, *dp;
int atm_num, i=1, wrote=0;

sprintf(filename, "%s/%s.atmos",datadir, astr[ustr[user].area].name);
strcpy(tempfile,get_temp_file());

if (!inpstr[0]) {
	write_str(user, "Usage: .delatmos <atmosphere number>");
	write_str(user, "       .delatmos all");
	return;
	}

if (!strcmp(inpstr,"all")) {
   remove(filename);
   write_str(user,"All atmospheres have been deleted for this room.");
   return;
   }

atm_num=atoi(inpstr);
if ((atm_num<1) || (atm_num>MAX_ATMOS)) {
        sprintf(mess,"Atmosphere number must be between 1 and %d",MAX_ATMOS);
	write_str(user,mess);
	return;
	}

if (!(fp=fopen(filename,"r"))) {
	write_str(user, "You have no atmospheres at the moment");
	return;
	}

if (!(dp=fopen(tempfile, "w"))) {
	write_str(user, "Couldn't open a temporary file");
	print_to_syslog("ERROR: Couldn't open tempfile to write in del_atmos()\n");
	fclose(dp);  return;
	}

fgets(probch, 80, fp);
while(!feof(fp)) {
	fgets(atmos, ATMOS_LEN+10, fp);
	if (i!=atm_num) {
		fputs(probch, dp);
		fputs(atmos, dp);
                wrote=1;
		}
	i++;
	fgets(probch, 80, fp);
	}

fclose(fp);
fclose(dp);

if (wrote) {
  /* Make the temp file the new atmospheres file for the user */
  if (rename(tempfile, filename)==-1) {
	write_str(user, "Couldn't make new atmosphere file");
	print_to_syslog("ERROR: Couldn't rename temp file to atmosphere file in del_atmos()\n");
	return;
	}
  }
else {
  remove(filename);
  remove(tempfile);
  }

if (atm_num>=i) sprintf(mess, "There is no atmosphere %d", atm_num);
else sprintf(mess, "Atmosphere %d deleted", atm_num);
write_str(user, mess);
}



/*** List the current atmospheres ***/
void list_atmos(int user)
{
char filename[80], atmos[ATMOS_LEN+11], probch[81];
FILE *fp;
int probint, i=1;

sprintf(filename, "%s/%s.atmos",datadir, astr[ustr[user].area].name);
if (!(fp=fopen(filename,"r"))) {
	write_str(user, "This room has no atmospheres at the moment");
	return;
	}

/* probint is num. between 0 - 100 (hopefully) */
fgets(probch, 80, fp);
write_str(user, "--------------------------------------------------");
write_str(user, "Room atmospheres are:     (a @ denotes a new line)");
write_str(user, "--------------------------------------------------");
while(!feof(fp)) {
	probint=atoi(probch);
	fgets(atmos, ATMOS_LEN+10, fp);
	atmos[strlen(atmos)-1]=0;
	sprintf(mess, "(%d) %2d%% %s", i, probint, atmos);
	write_str(user, mess);
	i++;
	fgets(probch, 80, fp);
	}
write_str(user, "--------------------------------------------------");
fclose(fp);
}


/*** shout sends speech to all users regardless of area ***/
void shout_think(int user, char *inpstr)
{
int pos = sh_count%NUM_LINES;
int f; 

if (!ustr[user].shout) 
  {
   write_str(user,NO_SHOUT);
   return;
  }
  
if (!strlen(inpstr)) 
  {
   write_str(user,"Review shouts:"); 
    
    for (f=0;f<NUM_LINES;++f) 
      {
        if ( strlen( sh_conv[pos] ) )
         {
	  write_str(user,sh_conv[pos]);  
	 }
	pos = ++pos % NUM_LINES;
      }

    write_str(user,"<Done>");  
    return;
  }

if (ustr[user].frog) {
   strcpy(inpstr,"I'm a frog, I'm a frog!");
   think(user,inpstr);
   return;
   }

sprintf(mess,USER_SHTHINKS,ustr[user].say_name,inpstr);

if (!ustr[user].vis)
	sprintf(mess,INVIS_SHTHINKS,INVIS_ACTION_LABEL,inpstr);

/** Store the shout in the buffer **/
strncpy(sh_conv[sh_count],mess,MAX_LINE_LEN);
sh_count = ( ++sh_count ) % NUM_LINES;
	
writeall_str(mess, 0, user, 0, user, NORM, SHOUT, 0);
sprintf(mess,YOU_SHTHINKS,inpstr);
write_str(user,mess);

}

/*** Let a user delete his or her account ***/
void suicide_user(int user, char *inpstr)
{
char nuke_name[NAME_LEN+1];
char buf1[30];
time_t tm;

/* Demotion check. If user was demoted to a 0 and they were */
/* promoted at the beginning, dont let them suicide         */
/* Doing so would mean they could just make a new character */
/* with the same name and auto-promote themselves           */
/*  Thanx to Jazzin for pointing this out.                  */
if (ustr[user].super==0 && ustr[user].promote==1) {
    write_str(user,"Nope. Sorry, we cant let you do that.");
    return;
   }

if (!strlen(inpstr)) {
    write_str(user,"To delete your account, you must enter your password after the command.");
    return;
    }

strtolower(inpstr);
st_crypt(inpstr);

if (strcmp(ustr[user].password,inpstr)) {
   write_str(user,NO_MATCH_SUIC);
   return;
   }

strcpy(nuke_name,ustr[user].name);
strtolower(nuke_name);
write_str(user,"");
write_str(user,"Quitting you and deleting this account..");
write_str(user,"");

sprintf(mess,"%s SUICIDE: By user %s",STAFF_PREFIX,ustr[user].say_name);
writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);

strncpy(bt_conv[bt_count],mess,MAX_LINE_LEN);
bt_count = ( ++bt_count ) % NUM_LINES;

        time(&tm);                                            
	sprintf(buf1,"%s",ctime(&tm));
        buf1[strlen(buf1)-6]=0; /* get rid of nl and year */

sprintf(mess,"%s: SUICIDE: By user %s\n",buf1,ustr[user].say_name);
print_to_syslog(mess);

user_quit(user);
 
remove_exem_data(nuke_name);
remove_user(nuke_name);

}

/*** Directed say to a person in the same room ***/
void say_to_user(int user, char *inpstr)
{
int u;
int area = ustr[user].area;
char other_user[ARR_SIZE];
char comstr[ARR_SIZE];

if (!strlen(inpstr)) {
   write_str(user,"You must specify a user and a message.");
   return;
   }

sscanf(inpstr,"%s ",other_user);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1)
  {
   if (check_for_user(other_user) == 1) {
      not_signed_on(user,other_user);
      return;
     }
   else {
      write_str(user,NO_USER_STR);
      return;
     }
  }

if (!gag_check(user,u,0)) return;

if (strcmp(astr[area].name,astr[ustr[u].area].name)) {
   write_str(user,"User is not in this room");
   return;
   }

remove_first(inpstr);

if (!strlen(inpstr)) {
     write_str(user, "What do you want to say to them?");
     return;
     }

if (ustr[u].afk) {
   write_str(user,"User is AFK, wait until they come back.");
   return;
   }

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

        comstr[0]=inpstr[0];
        comstr[1]=0;

if (comstr[0] == ustr[user].custAbbrs[get_emote(user)].abbr[0])
  {
        inpstr[0] = ' ';
        while(inpstr[0] == ' ') inpstr++;

        comstr[0]=inpstr[0];
        comstr[1]=0;

     if (comstr[0] == '\'') {
        /* inpstr[0] = ' '; */
        while(inpstr[0] == ' ') inpstr++;
	if (!ustr[u].vis)
        sprintf(mess,VIS_DIREMOTE,ustr[user].say_name,inpstr,INVIS_ACTION_LABEL);
	else
        sprintf(mess,VIS_DIREMOTE,ustr[user].say_name,inpstr,ustr[u].say_name);
        }
    else {
     strcpy(comstr," ");
     strcat(comstr,inpstr);
     strcpy(inpstr,comstr);
     if (!ustr[u].vis)
     sprintf(mess,VIS_DIREMOTE,ustr[user].say_name,inpstr,INVIS_ACTION_LABEL);
     else
     sprintf(mess,VIS_DIREMOTE,ustr[user].say_name,inpstr,ustr[u].say_name);
     }
    
    write_str(user,mess);
   if (!ustr[user].vis) {
     if (comstr[0] == '\'') {
	/*
        inpstr[0] = ' ';
        while(inpstr[0] == ' ') inpstr++;
	*/
	if (!ustr[u].vis)
        sprintf(mess,INVIS_DIREMOTE,INVIS_ACTION_LABEL,inpstr,INVIS_ACTION_LABEL);
	else
        sprintf(mess,INVIS_DIREMOTE,INVIS_ACTION_LABEL,inpstr,ustr[u].say_name);
        }
      else {
	if (!ustr[u].vis)
        sprintf(mess,INVIS_DIREMOTE,INVIS_ACTION_LABEL,inpstr,INVIS_ACTION_LABEL);
	else
        sprintf(mess,INVIS_DIREMOTE,INVIS_ACTION_LABEL,inpstr,ustr[u].say_name);
	}
    } /* end of !vis */
  } /* end of if emote */
else {
  if (!ustr[u].vis)
  sprintf(mess,VIS_DIRECTS,ustr[user].say_name,INVIS_ACTION_LABEL,inpstr);
  else
  sprintf(mess,VIS_DIRECTS,ustr[user].say_name,ustr[u].say_name,inpstr);
  write_str(user,mess);

  if (!ustr[user].vis) {
    if (!ustr[u].vis)
    sprintf(mess,INVIS_DIRECTS,INVIS_TALK_LABEL,INVIS_ACTION_LABEL,inpstr);
    else
    sprintf(mess,INVIS_DIRECTS,INVIS_TALK_LABEL,ustr[u].say_name,inpstr);
    }
}

  writeall_str(mess,1,user,0,user,NORM,SAY_TYPE,0);

says++;

/*--------------------------------*/
/* store say to the review buffer */
/*--------------------------------*/
  strncpy(conv[area][astr[area].conv_line],mess,MAX_LINE_LEN);
  astr[area].conv_line=(++astr[area].conv_line)%NUM_LINES;	

}


/** Gagcomm a user, takes away their capability **/
/** for all private communication commands      **/
void gag_comm(int user, char *inpstr, int type)
{
char buf1[256];
char other_user[ARR_SIZE];
int u,inlen;
unsigned int i;

if (!strlen(inpstr)) 
  {
   write_str(user,"Users GCommed & logged on     Time left"); 
   write_str(user,"-------------------------     ---------"); 
   for (u=0; u<MAX_USERS; ++u) 
    {
     if (ustr[u].gagcomm == 1 && ustr[u].area > -1) 
       {
        if (ustr[u].gag_time == 0)
           sprintf(mess,"%-29s %s",ustr[u].say_name,"Perm");
        else
           sprintf(mess,"%-29s %s",ustr[u].say_name,converttime((long)ustr[u].gag_time));
        write_str(user, mess);
       }
    }
   write_str(user,"(end of list)");
   return;
  }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   not_signed_on(user,other_user);
   return;
  }
if (u == user)
  {   
   write_str(user,"You are definitly wierd! Trying to gagcomm yourself, geesh."); 
   return;
  }

 if ((!strcmp(ustr[u].name,ROOT_ID)) || (!strcmp(ustr[u].name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    return;
    }
    
if (ustr[user].tempsuper <= ustr[u].super) 
  {
   write_str(user,"That would not be wise...");
   sprintf(mess,GCOMM_CANT,ustr[user].say_name);
   write_str(u,mess);
   return;
  }

if (type >= 1) goto ARREST;

if (ustr[u].gagcomm == 0) {
  remove_first(inpstr);
if (strlen(inpstr) && strcmp(inpstr,"0")) {
   if (strlen(inpstr) > 5) {
      write_str(user,"Minutes cant exceed 5 digits.");
      return;
      }
   inlen=strlen(inpstr);
   for (i=0;i<inlen;++i) {
     if (!isdigit((int)inpstr[i])) {
        write_str(user,"Numbers only!");
        return;
        }
     }
    i=0;
    i=atoi(inpstr);
    if ( i > 32767) {
       write_str(user,"Minutes cant exceed 32767.");
       i=0;
       return;
      }
  i=0;
  ustr[u].gag_time=atoi(inpstr);
  ustr[u].gagcomm = 1;
    write_str(u,GCOMMON_MESS);
    sprintf(mess,"GCOM ON : %s by %s for %s\n",ustr[u].say_name,
ustr[user].say_name, converttime((long)ustr[u].gag_time));
}
 else {
   ustr[u].gagcomm = 1;
   ustr[u].gag_time= 0;
   write_str(u,GCOMMON_MESS);
   sprintf(mess,"GCOM ON : %s by %s\n",ustr[u].say_name, ustr[user].say_name);
  }

 btell(user, mess);
 strcpy(buf1,get_time(0,0));    
 strcat(buf1," ");
 strcat(buf1,mess);
 print_to_syslog(mess);
 write_str(user,"Ok");
 return;
} /* end of if gagcommed */

else {
    ustr[u].gagcomm = 0;
    ustr[u].gag_time = 0;
    write_str(u,GCOMMOFF_MESS);
    sprintf(mess,"GCOM OFF: %s by %s",ustr[u].say_name, ustr[user].say_name);
    btell(user, mess);
 strcpy(buf1,get_time(0,0));    
 strcat(buf1," ");
 strcat(buf1,mess);
    strcat(mess,"\n");
    print_to_syslog(mess);
    write_str(user,"Ok");
    return;
  } 


ARREST:
if (type==1)
 ustr[u].gagcomm=1;
else
 ustr[u].gagcomm=0;
ustr[u].gag_time=0;

if (type==1) {
write_str(u,GCOMMON_MESS);
sprintf(mess,"GCOM ON : %s by %s\n",ustr[u].say_name, ustr[user].say_name);
}
else {
write_str(u,GCOMMOFF_MESS);
sprintf(mess,"GCOM OFF: %s by %s\n",ustr[u].say_name, ustr[user].say_name);
}
btell(user, mess);


 strcpy(buf1,get_time(0,0));    
 strcat(buf1," ");
 strcat(buf1,mess);
print_to_syslog(buf1);

write_str(user,"Ok");
}

/*-----------------------------------------------*/
/* frog a user                                   */
/*-----------------------------------------------*/
void frog_user(int user, char *inpstr)
{
char buf1[256];
char other_user[ARR_SIZE];
int u,inlen;
unsigned int i;

if (!strlen(inpstr)) 
  {
   write_str(user,"Users Frogged & logged on     Time left"); 
   write_str(user,"-------------------------     ---------"); 
   for (u=0;u<MAX_USERS;++u) 
    {
     if (ustr[u].frog  && ustr[u].area > -1) 
       {
        if (ustr[u].frog_time == 0)
           sprintf(mess,"%-29s %s",ustr[u].say_name,"Perm");
        else
           sprintf(mess,"%-29s %s",ustr[u].say_name,converttime((long)ustr[u].frog_time));
        write_str(user, mess);
       };
    }
   write_str(user,"(end of list)");
   return;
  }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   not_signed_on(user,other_user);
   return;
  }
  
if (u == user)
  {   
   write_str(user,"You are definitly wierd! Trying to frog yourself, geesh."); 
   return;
  }

 if ((!strcmp(ustr[u].name,ROOT_ID)) || (!strcmp(ustr[u].name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    return;
    }

if (ustr[user].tempsuper <= ustr[u].super) 
  {
   write_str(user,"That would not be wise...");
   sprintf(mess,FROG_CANT,ustr[user].say_name);
   write_str(u,mess);
   return;
  }

if (ustr[u].frog == 0) {
remove_first(inpstr);
if (strlen(inpstr) && strcmp(inpstr,"0")) {
   if (strlen(inpstr) > 5) {
      write_str(user,"Minutes cant exceed 5 digits.");
      return;
      }
   inlen=strlen(inpstr);
   for (i=0;i<inlen;++i) {
     if (!isdigit((int)inpstr[i])) {
        write_str(user,"Numbers only!");
        return;
        }
     }
    i=0;
    i=atoi(inpstr);
    if ( i > 32767) {
       write_str(user,"Minutes cant exceed 32767.");
       i=0;
       return;
      }
  i=0;
  ustr[u].frog_time=atoi(inpstr);
  ustr[u].frog = 1;
    write_str(u,FROGON_MESS);
    sprintf(mess,"FROG ON : %s by %s for %s\n",ustr[u].say_name,
ustr[user].say_name, converttime((long)ustr[u].frog_time));
}
else {
    ustr[u].frog = 1;
    ustr[u].frog_time=0;
    write_str(u,FROGON_MESS);
    sprintf(mess,"FROG ON : %s by %s\n",ustr[u].say_name, ustr[user].say_name);
  }
}     /* end of if frogged */

else {
    ustr[u].frog = 0;
    ustr[u].frog_time=0;
    write_str(u,FROGOFF_MESS);
    sprintf(mess,"FROG OFF: %s by %s\n",ustr[u].say_name, ustr[user].say_name);
  } 

btell(user, mess);

 strcpy(buf1,get_time(0,0));    
 strcat(buf1," ");
 strcat(buf1,mess);
print_to_syslog(buf1);

write_str(user,"Ok");
}

/* Set talkers auto-nuke flag on or off */
void auto_nuke(int user)
{
char line[132];
char buf1[30];

  if (autonuke)
    {
      write_str(user,"Auto-nuke DISABLED");
      autonuke=0;  
      sprintf(line,"Auto-nuke DISABLED by %s\n",ustr[user].say_name);
    }
   else
    {
      write_str(user,"Auto-nuke ENABLED");
      autonuke=1;  
      sprintf(line,"Auto-nuke ENABLED by %s\n",ustr[user].say_name);
    }
    
 btell(user,line);

 strcpy(mess,line);
 strcpy(buf1,get_time(0,0));    
 strcpy(line,buf1);
 strcat(line,mess);
 print_to_syslog(line);

}

/* Jazzin - makes .autopromote .autonuke and .autoexpire into one command */
void auto_com(int user, char *inpstr)
{
char option[ARR_SIZE];
char onoff[2][4];

if (!strlen(inpstr))
  {
   write_str(user,"To use .auto you have to specify which auto-setting you want.");
   write_str(user," .auto nuke      Will turn on/off the auto-nuke flag");
   write_str(user," .auto expire    Will turn on/off the auto-expire flag");
   write_str(user," .auto promote   Will turn on/off the auto-promotion flag");
   write_str(user," .auto settings  Lets you see what they are set at currently");
   write_str(user,"");
   return;
  }

sscanf(inpstr,"%s",option);
strtolower(option);
strcpy(onoff[0],"OFF");
strcpy(onoff[1],"ON");
   if (!strcmp(option,"nuke")) 
     {
      auto_nuke(user);
      return;
     }
    else if (!strcmp(option,"expire"))
     {
      auto_expr(user);
      return;
     }
   else if (!strcmp(option,"promote"))
     {
      auto_prom(user);
      return;
     }
   else if (!strcmp(option,"settings"))
     {
      write_str(user,"----------------------------------------------------");
      sprintf(mess,"Auto-promote : %s",onoff[autopromote]);
      write_str(user,mess);
      sprintf(mess,"Auto-nuke    : %s",onoff[autonuke]);
      write_str(user,mess);
      if (autoexpire==0)
      write_str(user,"Auto-expire  : Expiring DISABLED, Warnings DISABLED");
      else if (autoexpire==1)
      write_str(user,"Auto-expire  : Expiring DISABLED, Warnings ENABLED");
      else if (autoexpire==2)
      write_str(user,"Auto-expire  : Expiring ENABLED, Warnings ENABLED");
      else if (autoexpire==3)
      write_str(user,"Auto-expire  : Expiring ENABLED, Warnings DISABLED");
      write_str(user,"----------------------------------------------------");
      return;
    } 
  else
    {
   write_str(user,"Option Unkown.");
   write_str(user,"To use .auto you have to specify which auto-setting you want.");
   write_str(user," .auto nuke     Will turn on/off the auto-nuke flag");
   write_str(user," .auto expire   Will turn on/off the auto-expire flag");
   write_str(user," .auto promote  Will turn on/off the auto-promotion flag");
   write_str(user," .auto settings  Lets you see what they are set at currently");
   write_str(user,"");
   }

}

/* Set talkers auto-promote flag on or off */
void auto_prom(int user)
{
char buf1[30];
char line[132];

  if (autopromote==1)
    {
      write_str(user,"Auto-promote DISABLED");
      autopromote=0;  
      sprintf(line,"Auto-promote DISABLED by %s\n",ustr[user].say_name);
    }
   else
    {
      write_str(user,"Auto-promote ENABLED");
      autopromote=1;
      sprintf(line,"Auto-promote ENABLED by %s\n",ustr[user].say_name);
    }

 btell(user,line);

 strcpy(mess,line);
 strcpy(buf1,get_time(0,0));    
 strcpy(line,buf1);
 strcat(line,mess);
 print_to_syslog(line);
}

/* Set talkers auto-promote flag on or off */
void auto_expr(int user)
{
char line[132];
char buf1[30];

  if (autoexpire==0)
    {
      write_str(user,"Auto-expiring DISABLED, warnings ENABLED");
      autoexpire=1;
      sprintf(line,"Auto-expiring DISABLED, warnings ENABLED  by %s\n",ustr[user].say_name);
    }
  else if (autoexpire==1)
    {
      write_str(user,"Auto-expiring ENABLED, warnings ENABLED");
      autoexpire=2;
      sprintf(line,"Auto-expiring ENABLED, warnings ENABLED by %s\n",ustr[user].say_name);
    }
  else if (autoexpire==2)
    {
      write_str(user,"Auto-expiring ENABLED, warnings DISABLED");
      autoexpire=3;
      sprintf(line,"Auto-expiring ENABLED, warnings DISABLED by %s\n",ustr[user].say_name);
    }
  else if (autoexpire==3)
    {
      write_str(user,"Auto-expiring DISABLED, warnings DISABLED");
      autoexpire=0;
      sprintf(line,"Auto-expiring DISABLED, warnings DISABLED by %s\n",ustr[user].say_name);
    }
    
 btell(user,line);

 strcpy(mess,line);
 strcpy(buf1,get_time(0,0));    
 strcpy(line,buf1);
 strcat(line,mess);
 print_to_syslog(line);

}


/** Eight ball command..I promised smoothie i would do this **/
void eight_ball(int user, char *inpstr)
{
int i;

if (ustr[user].area==INIT_ROOM) {
  write_str(user,"The oracle does not respond well to public places");
  write_str(user,"Try somewhere less public.");
  return;
  }

if (!strlen(inpstr)) {
  write_str(user,"Your thoughts have floated away. Try thinking harder!");
  return;
  }

/* User is writing gibberish */
/* They might still be writing gibberish after this check */
/* but it's the best I can do                             */
if (!strstr(inpstr," ") || !strstr(inpstr,"?")) {
  write_str(user,"Your thoughts are still unclear to me.");
  write_str(user,"Please put your thoughts into the form of a quesiton.");
  return;
  }

i = rand() % NUM_BALL_LINES;

inpstr[0]=toupper((int)inpstr[0]);
sprintf(mess,"%s asks the all knowing Guru of %s: %s",ustr[user].say_name,SYSTEM_NAME,inpstr);
write_str(user,mess);
writeall_str(mess,1,user,0,user,NORM,SAY_TYPE,0);

sprintf(mess,"The Guru looks up from his deep trance and says: %s",
              ball_text[i]);
write_str(user,mess);
write_str(user,"");
writeall_str(mess,1,user,0,user,NORM,SAY_TYPE,0);
strcpy(mess,"");
writeall_str(mess,1,user,0,user,NORM,SAY_TYPE,0);

}

/* Add warning log for a user */
void warning(int user, char *inpstr)
{
char timestr[30];
char other_user2[ARR_SIZE];
char z_mess[ARR_SIZE+45];
char filename[FILE_NAME_LEN];
FILE *fp;
time_t tm;

if (ustr[user].tempsuper >= WIZ_LEVEL) {

  if (!strlen(inpstr)) {
   write_str(user,"Whose warning log do you want to search?");
   return;
   }

sscanf(inpstr,"%s ",other_user2);
other_user2[80]=0;

/* plug security hole */
if (check_fname(other_user2,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

 } /* end of it*/
else {
 strcpy(other_user2,ustr[user].name);
 inpstr[0]=0;
 }


strtolower(other_user2);
remove_first(inpstr);

if (!strlen(inpstr)) {
   sprintf(filename,"%s/%s",WLOGDIR,other_user2);
   if (!check_for_file(filename)) {
     write_str(user,"No log on that user!");
     return;
     }
   write_str(user,"^HG***^ ^HYWarning log^ ^HG***^");
   cat(filename,user,0);
  }
else {
   if (!check_for_user(other_user2)) {
     write_str(user,NO_USER_STR);
     return;
     }

   sprintf(filename,"%s/%s",WLOGDIR,other_user2);

   if (!(fp=fopen(filename,"a"))) {
     strcpy(mess,"ERROR: Cannot append warning to user's warning log.\n");
     write_str(user,mess);
     print_to_syslog(mess);
     return;
    }

   time(&tm);
   strcpy(timestr,ctime(&tm));
   midcpy(timestr,timestr,4,15);

   if (strlen(inpstr) > REASON_LEN) {
      inpstr[REASON_LEN]=0;
      write_str(user,"Reason too long..truncated.");
     }

   z_mess[0]=0;
   sprintf(z_mess,"(%s) From %s: %s\n",timestr,ustr[user].say_name,inpstr);
   fputs(z_mess,fp);
   fclose(fp);
   sprintf(mess,"logged a warning about %s",other_user2);
   btell(user,mess);
 }
}


/*** invite someone into private room ***/
void invite_user(int user, char *inpstr)
{
int u,area=ustr[user].area;
char other_user[ARR_SIZE];

if (!astr[area].private) {
	write_str(user,"The area is public anyway");  return;
	}
if (!strlen(inpstr)) {
	write_str(user,"Invite who?");  return;
	}
sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

/* see if other user exists */
if ((u=get_user_num(other_user,user))== -1) {
	not_signed_on(user,other_user);
        return;
       }

if (!strcmp(other_user,ustr[user].name)) {
	write_str(user,"You cannot invite yourself!");  return;
	}
if (ustr[u].area==ustr[user].area) {
	sprintf(mess,"%s is already in the room!",ustr[u].say_name);
	write_str(user,mess);
	return;
	}

if (ustr[u].pro_enter || ustr[u].vote_enter || ustr[u].roomd_enter) {
    write_str(user,IS_ENTERING);
    return;
    }

write_str(user,"Ok");
sprintf(mess,"%s has invited you to the %s",ustr[user].say_name,astr[area].name);
if (!ustr[user].vis) 
	sprintf(mess,"%s has invited you to the %s",INVIS_ACTION_LABEL,astr[area].name);
write_str(u,mess);
ustr[u].invite=area;
}



/*** emote func used for expressing emotional or visual stuff ***/
void emote(int user, char *inpstr)
{
int area;
char comstr[ARR_SIZE];

if (!strlen(inpstr))
  {
   write_str(user,"Emote what?");  
   return;
  }

if (ustr[user].frog == 1) inpstr = FROG_EMOTE;

if ((strstr(inpstr,"ARRIVING")) || (strstr(inpstr,"LEAVING")) ) {
   write_str(user,"You cant emote that.");
   return;
   }

if (!ustr[user].vis) {
   comstr[0]=inpstr[0];
   comstr[1]=0;
   if (comstr[0] == '\'') {
        inpstr[0] = ' ';
        while(inpstr[0] == ' ') inpstr++;
        sprintf(mess,"%s\'%s",INVIS_ACTION_LABEL,inpstr);
        }
    else
       sprintf(mess,"%s %s",INVIS_ACTION_LABEL,inpstr);
 }
 else {
   comstr[0]=inpstr[0];
   comstr[1]=0;
   if (comstr[0] == '\'') {
        inpstr[0] = ' ';
        while(inpstr[0] == ' ') inpstr++;
        sprintf(mess,"%s\'%s",ustr[user].say_name,inpstr);
        }
    else
       sprintf(mess,"%s %s",ustr[user].say_name,inpstr);
 }

/* write output */
write_str(user,mess);
writeall_str(mess, 1, user, 0, user, NORM, SAY_TYPE, 0);

/*-----------------------------------*/
/* store the emote in the rev buffer */
/*-----------------------------------*/

area = ustr[user].area;
strncpy(conv[area][astr[area].conv_line],mess,MAX_LINE_LEN);
astr[area].conv_line = ( ++astr[area].conv_line ) % NUM_LINES;
	
}

/*** emote func used for expressing emotional or visual stuff ***/
void semote(int user, char *inpstr)
{
int point=0,count=0,i=0,lastspace=0,lastcomma=0,gotchar=0;
int point2=0,multi=0;
int multilistnums[MAX_MULTIS];
char multilist[MAX_MULTIS][ARR_SIZE];
char multiliststr[ARR_SIZE];
int u=-1;
char prefix[25];
char other_user[ARR_SIZE];
char comstr[ARR_SIZE];
char *other_input='\0';

for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; multilistnums[i]=-1; }
multiliststr[0]=0;
i=0;

if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

if (!strlen(inpstr)) 
  {
   write_str(user,"Secret emote who what?");  
   return;
  }

sscanf(inpstr,"%s ",other_user);
if (!strcmp(other_user,"-f")) {
        other_user[0]=0;
        for (i=0;i<MAX_ALERT;++i) {
         if (strlen(ustr[user].friends[i])) {
          strcpy(multilist[count],ustr[user].friends[i]);
          count++;
	  if (count==MAX_MULTIS) break;
          }
        }
        if (!count) {
                write_str(user,"You dont have any friends!");
                return;
        }
        i=0;
        remove_first(inpstr);
  }
else {
other_user[0]=0;

for (i=0;i<strlen(inpstr);++i) {
        if (inpstr[i]==' ') {
                if (lastspace && !gotchar) { point++; point2++; continue; }
                if (!gotchar) { point++; point2++; }
                lastspace=1;
                continue;
          } /* end of if space */
        else if (inpstr[i]==',') {
                if (!gotchar) {
                        lastcomma=1;
			point++;
			point2++;
			continue;
                }
                else {
                if (count <= MAX_MULTIS-1) {
                midcpy(inpstr,multilist[count],point,point2-1);
                count++;
                }
                point=i+1;
                point2=point;
                gotchar=0;
                lastcomma=1;
                continue;
                }
                
        } /* end of if comma */
        if ((inpstr[i-1]==' ') && (gotchar)) {
                if (count <= MAX_MULTIS-1) {
                midcpy(inpstr,multilist[count],point,point2-1);
                count++;
                }
                break;
        }
        gotchar=1;
        lastcomma=0;
        lastspace=0;
        point2++;
} /* end of for */
midcpy(inpstr,multiliststr,i,ARR_SIZE);
                
if (!strlen(multiliststr)) {
        /* no message string, copy last user */
        midcpy(inpstr,multilist[count],point,point2);
        count++;  
        strcpy(inpstr,"");
        }
else {
        strcpy(inpstr,multiliststr);
        multiliststr[0]=0;
     }          
} /* end of friend else */

if (!strlen(inpstr)) {
	write_str(user,"What do you want to secret emote to them?");
	return;
   }

i=0;
point=0;
point2=0;
gotchar=0;

if ((strstr(inpstr,"ARRIVING")) || (strstr(inpstr,"LEAVING")) ) {
   write_str(user,"You cant semote that.");
   for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; multilistnums[i]=-1; }
   multiliststr[0]=0;
   return;
   }

tells++;
        
if (count>1) multi=1;
                
/* go into loop and check users */
for (i=0;i<count;++i) {

strcpy(other_user,multilist[i]);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   if (!multi) {
   write_str(user,"Illegal name.");
   return;
   }
   else continue;
  }
  
strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   if (!read_user(other_user)) {
      write_str(user, NO_USER_STR);
      if (!multi) return;
      else continue;
      }
   not_signed_on(user,t_ustr.say_name);  
if (user_wants_message(user,FAILS)) {
   sprintf(mess,"%s",t_ustr.fail);
   write_str(user,mess);
   }
	if (!multi) return;
	else continue;
  }

if (!gag_check(user,u,0)) {
	if (!multi) return;
	else continue;
  }

if (ustr[u].pro_enter || ustr[u].vote_enter || ustr[u].roomd_enter) {
    write_str(user,IS_ENTERING);
    if (!multi) return;
    else continue;
    }

if (ustr[u].afk)
  {
    if (ustr[u].afk == 1) {
      if (!strlen(ustr[u].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[u].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }
     else {
      if (!strlen(ustr[u].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[u].say_name);
      else
      sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }

    write_str(user,t_mess);
  }

if (ustr[u].igtell && ustr[user].super<WIZ_LEVEL) 
  {
   sprintf(mess,"%s is ignoring tells and secret emotes",ustr[u].say_name);
   write_str(user,mess);
   if (user_wants_message(user,FAILS)) write_str(user,ustr[u].fail);
   if (!multi) return;
   else continue;
  }

/* check if this user is already in the list */
/* we're gonna reuse some ints here          */
for (point2=0;point2<MAX_MULTIS;++point2) {
        if (multilistnums[point2]==u) { gotchar=1; break; }
   }
point2=0;
if (gotchar) {
  gotchar=0;
  continue;
  }

/* it's ok to send the tell to this user, add them to the multistr */
/* add this user to the list for our next loop */
multilistnums[point]=u;
point++;
} /* end of user for */
i=0;
  
/* no multilistnums, must be all bad users */
if (!point) {
        return;
  }

/* loop to compose the messages and print to the users */
for (i=0;i<point;++i) {

u=multilistnums[i];

count=0;
point2=0;
multiliststr[0]=0;
/* make multi string to send to this user */
for (point2=0;point2<point;++point2) {
/* dont send recipients name to themselves */
if (u==multilistnums[point2]) continue;
else count++;
if (count>0)
 strcat(multiliststr,",");
/* add their name to the output string */
if (!ustr[multilistnums[point2]].vis)
 strcat(multiliststr,INVIS_ACTION_LABEL);
else
 strcat(multiliststr,ustr[multilistnums[point2]].say_name);
}

other_input='\0';
other_input=inpstr;

if ((ustr[u].monitor==1) || (ustr[u].monitor==3))
  {
    strcpy(prefix,"<");
    strcat(prefix,ustr[user].say_name);
    strcat(prefix,"> ");
  }
 else
  {
   prefix[0]=0;
  }


/* write to user being told */
if (!strlen(inpstr)) {
if (ustr[user].vis) {
	sprintf(mess,"You sense that %s is looking for you in the %s",
ustr[user].say_name, astr[ustr[user].area].name);
       }
else {
  if (!strlen(prefix)) {
    sprintf(mess,"You sense that %s is looking for you",INVIS_ACTION_LABEL);
    mess[15]=tolower((int)mess[15]);
   }
  else {
   sprintf(mess,"You sense that %s %s is looking for you",prefix,INVIS_ACTION_LABEL);
    mess[strlen(prefix)+16]=tolower((int)mess[strlen(prefix)+16]);
  }
 }
}
else {
if (ustr[user].frog) strcpy(other_input,FROG_SEMOTE);

if (ustr[user].vis) {
   comstr[0]=other_input[0];
   comstr[1]=0;
   if (comstr[0] == '\'') {
        while(other_input[0] == '\'') { other_input++; }

	if (!multi)
   	 sprintf(mess,VIS_SEMOTES_P,ustr[user].say_name, other_input);
	else
   	 sprintf(mess,VIS_SEMOTE_MP,multiliststr,ustr[user].say_name,other_input);
       }
   else {
	if (!multi)
	 sprintf(mess,VIS_SEMOTES,ustr[user].say_name, other_input);
	else
	 sprintf(mess,VIS_SEMOTES_M,multiliststr,ustr[user].say_name,other_input);
    }
   }
 else {
   comstr[0]=other_input[0];
   comstr[1]=0;
   if (comstr[0] == '\'') {
      while(other_input[0] == '\'') other_input++;
      if (!multi)
       sprintf(mess,INVIS_SEMOTES_P,prefix,INVIS_ACTION_LABEL,other_input);
      else
       sprintf(mess,INVIS_SEMOTE_MP,prefix,multiliststr,INVIS_ACTION_LABEL,other_input);
     }
   else {
      if (!multi)
       sprintf(mess,INVIS_SEMOTES,prefix,INVIS_ACTION_LABEL,other_input);
      else
       sprintf(mess,INVIS_SEMOTES_M,prefix,multiliststr,INVIS_ACTION_LABEL,other_input);
   }
 }
}

if (ustr[u].beeps) {
 if (user_wants_message(u,BEEPS))
  strcat(mess,"\07");
 }

/*-----------------------------------*/
/* store the semote in the rev buffer*/
/*-----------------------------------*/
/* moved because of multi tells */
strncpy(ustr[u].conv[ustr[u].conv_count],mess,MAX_LINE_LEN);
ustr[u].conv_count = ( ++ustr[u].conv_count ) % NUM_LINES;

if (ustr[u].hilite==2)
 write_str(u,mess);
else {
 strcpy(mess, strip_color(mess));
 write_hilite(u,mess);
 }

} /* end of message compisition for loop */

if (multi) {
point2=0;
multiliststr[0]=0; 
/* make multi string to send to this user */
for (point2=0;point2<point;++point2) {
/* dont send recipients name to themselves */
if (point2>0)
 strcat(multiliststr,",");
/* add their name to the output string */  
if (!ustr[multilistnums[point2]].vis)
 strcat(multiliststr,INVIS_ACTION_LABEL);
else
 strcat(multiliststr,ustr[multilistnums[point2]].say_name);
}
} /* end of if multi */

/* write to teller */
if (strlen(inpstr)) {
   if (inpstr[0] == '\'') {
      while(inpstr[0] == '\'') inpstr++;

    if (!ustr[user].vis) {
	if (!multi)
    sprintf(mess,VISFROMSEMOTE_P,ustr[u].say_name,INVIS_ACTION_LABEL,inpstr);
	else
    sprintf(mess,VISFROMSEMOTE_P,multiliststr,INVIS_ACTION_LABEL,inpstr);
    }
    else {
	if (!multi)
    sprintf(mess,VISFROMSEMOTE_P,ustr[u].say_name,ustr[user].say_name,inpstr);
	else
    sprintf(mess,VISFROMSEMOTE_P,multiliststr,ustr[user].say_name,inpstr);
    }
    }
   else {
    if (!ustr[user].vis) {
	if (!multi)
    sprintf(mess,VISFROMSEMOTE,ustr[u].say_name,INVIS_ACTION_LABEL,inpstr);
	else
    sprintf(mess,VISFROMSEMOTE,multiliststr,INVIS_ACTION_LABEL,inpstr);
    }
    else {
	if (!multi)
    sprintf(mess,VISFROMSEMOTE,ustr[u].say_name,ustr[user].say_name,inpstr);
	else
    sprintf(mess,VISFROMSEMOTE,multiliststr,ustr[user].say_name,inpstr);
    }
   }
 }
else {
	if (!multi)
 sprintf(mess,"Telepathic message sent to %s.",ustr[u].say_name);
	else
 sprintf(mess,"Multi-Telepathic message sent to %s.",ustr[u].say_name);
 }
write_str(user,mess);

if (!multi) {
if (user_wants_message(user,SUCCS) && strlen(inpstr) && strlen(ustr[u].succ))
write_str(user,ustr[u].succ);
}

strncpy(ustr[user].conv[ustr[user].conv_count],mess,MAX_LINE_LEN);
ustr[user].conv_count = ( ++ustr[user].conv_count ) % NUM_LINES;

i=0;
for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; }
multiliststr[0]=0;

}



/*** gives current status of rooms */
void rooms(int user, char *inpstr)
{
int area;
int totl_hide;
int showhide=0;
int i,j=1;
char pripub[2][8];
char cbe[3];
char atm[3];
char filename[FILE_NAME_LEN];
FILE *fp;
time_t tm;

if (!strlen(inpstr)) showhide=0;
else if (!strcmp(inpstr,"-h")) {
  if (ustr[user].tempsuper < ROOMVIS_LEVEL) {
    write_str(user,"You don't have that much power");
    return;
    }
  showhide=1;
  }
else {
  write_str(user,"Invalid option.");
  write_str(user,"Usage: .rooms [-h]");
  return;
  }

strcpy(pripub[0],"public");
strcpy(pripub[1],"private");
strcpy(cbe," ");
strcpy(atm," ");

totl_hide = 0;

 time(&tm);
 strcpy(filename,get_temp_file());
 if (!(fp=fopen(filename,"w"))) {
     sprintf(mess,"%s Cant create file for paged rooms listing!",ctime(&tm));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     return;
     }

fputs("------------------------------------------------------------------------------\n",fp);
fputs("At   Occupied Rooms:          Usrs  Msgs  Topic\n",fp);
fputs("------------------------------------------------------------------------------\n",fp);
for (area=0;area<NUM_AREAS;++area) 
  {
   i = find_num_in_area(area);
   
   if (strchr(area_nochange, (char) area+65)== NULL)
     cbe[0]=' ';
    else 
     cbe[0]='*';

   if (astr[area].atmos)
     atm[0]='A';
    else
     atm[0]=' ';

   sprintf(mess,"%s%s %-*s : %-7s : %2d : %3d  ",  cbe,  atm,
                                                  ROOM_LEN,astr[area].name,
                                                  pripub[astr[area].private],
                                                  i,astr[area].mess_num);
                                              
   if (!strlen(astr[area].topic)) 
     strcat(mess,"<no topic>\n");
   else {
     strcat(mess,astr[area].topic);
     strcat(mess,"\n");
     }
     
   mess[0]=toupper((int)mess[0]);

if (!showhide) {   
   if (!astr[area].hidden )
     {
      if ( i ) fputs(mess,fp);
     }
    else
     totl_hide++;
   }
else {
   if (astr[area].hidden )
     {
      if ( i ) fputs(mess,fp);
      totl_hide++;
     }
   }
  } /* end of for */
  
fputs("\n",fp);
fputs("------------------------------------------------------------------------------\n",fp);
fputs("Other Rooms: (with number of messages)\n",fp);
fputs("------------------------------------------------------------------------------\n",fp);
  
for (area=0;area<NUM_AREAS;++area) 
  {
   i = find_num_in_area(area);  
    
   if (strchr(area_nochange, (char) area+65) == NULL) {
      if (astr[area].private)
       cbe[0]='P';
      else
       cbe[0]=' ';
    }
    else {
     cbe[0]='*';
     }
   
   if (astr[area].atmos)
    atm[0]='A';
   else
    atm[0]=' ';

   sprintf(mess,"%*s(%s%s%3d) ",ROOM_LEN,astr[area].name,cbe,atm,astr[area].mess_num);
   mess[0]=toupper((int)mess[0]);

 if (!showhide) {   
   if (!astr[area].hidden)
     {
      if (!i)
        {
         fputs(mess,fp);
	 if (!(j++%3) )
	   {j = 1;
	    fputs("\n",fp);
	   }
	}
     }
   }
 else {
   if (astr[area].hidden)
     {
      if (!i)
        {
         fputs(mess,fp);
	 if (!(j++%3) )
	   {j = 1;
	    fputs("\n",fp);
	   }
	}
     }
  }
 } /* end of for */
fputs(" \n \n",fp);

if (!showhide)
 sprintf(mess,"Currently there are %d rooms\n",NUM_AREAS - totl_hide);
else
 sprintf(mess,"Currently there are %d hidden rooms\n",totl_hide);

fputs(mess,fp);
fclose(fp);

 if (!cat(filename,user,0)) {
     time(&tm);
     sprintf(mess,"%s Cant cat room listing file!",ctime(&tm));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     }

return;
}


/*** save message to room message board file ***/
void write_board(int user, char *inpstr, int mode)
{
FILE *fp;
char stm[20],filename[FILE_NAME_LEN],name[NAME_LEN];
time_t tm;

/* process wiz notes */
if (mode == 1)
  {
   if (!strlen(inpstr))
     {
      read_board(user,1,"");
      return;
     } 
   if ((strlen(inpstr) < 3) && (strlen(inpstr) > 0)) {
      if (isdigit((int)inpstr[0])) {
        read_board(user,1,inpstr);
        return;
        }
    }
   sprintf(t_mess,"%s/wizmess",MESSDIR);
  }
else if (mode == 2)
  {
   if (!strlen(inpstr))
     {
      if (ustr[user].tempsuper < GRIPE_LEVEL) {
        write_str(user,"^You cant read the gripe board.^");
        return;
        }
      else {
        read_board(user,2,"");
        return;
        }
     }
   if ((strlen(inpstr) < 3) && (strlen(inpstr) > 0)) {
      if (ustr[user].tempsuper < GRIPE_LEVEL) {
        write_str(user,"^You cant read the gripe board.^");
        return;
        }
      if (isdigit((int)inpstr[0])) {
        read_board(user,2,inpstr);
        return;
        }
    }
   sprintf(t_mess,"%s/gripes",MESSDIR);
  }
else if (mode == 3) {
   if (!strlen(inpstr))
     {
      read_board(user,3,"");
      return;
     } 
   if ((strlen(inpstr) < 3) && (strlen(inpstr) > 0)) {
      if (isdigit((int)inpstr[0])) {
        read_board(user,3,inpstr);
        return;
        }
    }
   sprintf(t_mess,"%s/suggs",MESSDIR);
  }
 else
  {
   sprintf(t_mess,"%s/board%d",MESSDIR,ustr[user].area);
  }
  
if (!strlen(inpstr)) 
  {
   write_str(user,"You forgot the message"); return;
  }

if (ustr[user].frog) {
   write_str(user,"Frogs cant write, silly.");
   return;
   }

/* open board file */
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(fp=fopen(filename,"a"))) 
  {
   sprintf(mess,"%s : message cannot be written",syserror);
   write_str(user,mess);
   sprintf(mess,"Cant open %s message board file to write in write_board()",
                astr[ustr[user].area].name);
   logerror(mess);
   return;
  }

/* write message - alter nums. in midcpy to suit */
time(&tm);
midcpy(ctime(&tm),stm,4,15);
strcpy(name,ustr[user].say_name);


if (mode == 2)
 sprintf(mess,"(%s) %s gripes: %s\n",stm,name,inpstr);
else if (mode == 3) {
 if (ANON_SUGGEST)
  sprintf(mess,"Someone suggests: %s\n",inpstr);
 else
  sprintf(mess,"%s suggests: %s\n",name,inpstr);
 }
else 
 sprintf(mess,"(%s) From %s: %s\n",stm,name,inpstr);
fputs(mess,fp);
FCLOSE(fp);

/* send output */
if (mode == 2)
 write_str(user,"Your gripe is duely noted.");
else if (mode == 3)
 write_str(user,"Suggestion written. Thank you.");
else 
 write_str(user,USER_WRITE);

if (mode == 0)
  {
   sprintf(mess,OTHER_WRITE,ustr[user].say_name);

   if (!ustr[user].vis) 
	sprintf(mess,INVIS_WRITE);

   writeall_str(mess, 1, user, 0, user, NORM, MESSAGE, 0);
   astr[ustr[user].area].mess_num++;
  }
 else if ((mode == 2) || (mode == 3))
  {
  }
 else
  {
   strcpy(mess,"added a wiz note.");
   btell(user,mess);
  }
}


/*** read the message board  ***/
void read_board(int user, int mode, char *inpstr)
{
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
char name_area[ARR_SIZE];
char junk[1001];
int number = 0;
int area, found, new_area,a=0;
int num_lines=0;
int lines=0;
int b=0;
FILE *fp;
FILE *tfp;

/* send output to user */
area = ustr[user].area;

if (mode==1)
 {
  sprintf(mess, "*** The wizards note board (important info only, non-wipeable) ***");
  sprintf(filename,"%s/wizmess",MESSDIR);
  goto WIZ;
 }
else if (mode==2)
 {
  sprintf(mess, "*** User gripes ***");
  sprintf(filename,"%s/gripes",MESSDIR);
  goto WIZ;
 }
else if (mode==3)
 {
  sprintf(mess,"^HG***^^HYReading suggestion board^^HG***^");
  sprintf(filename,"%s/suggs",MESSDIR);
  goto WIZ;
 }
else
 {
  if (!strlen(inpstr)) {
  if (astr[area].hidden && ustr[user].tempsuper < GRIPE_LEVEL)
    {
     write_str(user,"^Secured message board, read not allowed.^");
     return;
    }
  
  if (astr[area].hidden)
    sprintf(mess,"** A secured message board **");
   else
    sprintf(mess,"** The %s message board **",astr[area].name);
 }

if (strlen(inpstr)) {
   if (isalpha((int)inpstr[0])) {
       if (strlen(inpstr) > 20) {
              write_str(user,"Room name is too long");  return;
              }
       name_area[0]=0;
       sscanf(inpstr,"%s ",name_area);
       remove_first(inpstr);        
        }   
   else {  name_area[0]=0; }
  }
  else {
        name_area[0]=0;
       }

if ((strlen(inpstr) < 3) && (strlen(inpstr) > 0)) {
      num_lines=atoi(inpstr);
      a=1;
      }

   if (strlen(name_area))               
     {
      found = FALSE;
      for (new_area=0; new_area < NUM_AREAS; ++new_area)
       { 
        if (! instr2(0, astr[new_area].name, name_area, 0) )         
          { 
            found = TRUE;
            area = new_area;
           if (astr[new_area].hidden && ustr[user].tempsuper < GRIPE_LEVEL)
             {
              write_str(user,"^Secured message board, read not allowed.^");
              return;
             }
           if (astr[new_area].hidden)
            sprintf(mess,"** A secured message board **");
           else
            sprintf(mess,"***Reading message board in %s***",astr[new_area].name);
            break;
          }
       }
 
      if (!found)
        {
         write_str(user, NO_ROOM);
         return;
        }
     }

  /* If number given but no area name */
  else {
           if (astr[area].hidden)
            sprintf(mess,"** A secured message board **");
           else
            sprintf(mess,"***Reading message board in %s***",astr[area].name);
       }
  
   sprintf(filename,"%s/board%d",MESSDIR,area);

 }  /* End of main else */


WIZ:
write_str(user," ");
write_str(user,mess);
write_str(user," ");

if (ustr[user].tempsuper >= LINENUM_LEVEL) number = 1;

if ((strlen(inpstr) < 3) && (strlen(inpstr) > 0)) {
      num_lines=atoi(inpstr);
      a=1;
      }

if (a==1) {
       if (num_lines < 1) {
           if (!cat(filename,user,number))
               write_str(user,"^There are no messages on the board.^");
               goto DONE;
               }
lines = file_count_lines(filename);
if (num_lines <= lines) {
    ustr[user].numbering = (lines - num_lines) +1;
    }
 else {
      num_lines=lines;
      }
num_lines = lines - num_lines;

if (!(fp=fopen(filename,"r"))) {
      write_str(user,"^There are no messages on the board.^");
      num_lines=0;  a=0;  lines=0;
      ustr[user].numbering = 0;
      return;
      }

strcpy(filename2,get_temp_file());
tfp=fopen(filename2,"w");

while (!feof(fp)) {
          fgets(junk,1000,fp);
          b++;
          if (b <= num_lines) {
              junk[0]=0;
              continue;
              }
          else {
              fputs(junk,tfp);
              junk[0]=0;
              }
         }
FCLOSE(fp);
FCLOSE(tfp);
num_lines=0;  lines=0;
  if (!cat(filename2,user,number))
      write_str(user,"^There are no messages on the board.^");
 
DONE:
b=0;
}

if (a==0) {
ustr[user].numbering=0;
if (!cat(filename, user, number)) 
   write_str(user,"^There are no messages on the board.^");
  }

name_area[0]=0;
ustr[user].numbering = 0;
}


/*** wipe board (erase file) ***/
void wipe_board(int user, char *inpstr, int wizard)
{
char filename[FILE_NAME_LEN];
FILE *bfp;
int lower=-1;
int upper=-1;
int mode=0;

if (wizard==0)
  {
   sprintf(t_mess,"%s/board%d",MESSDIR,ustr[user].area);
  }
 else if (wizard==2)
  {
   write_str(user,"***Gripe Wipe***");
   sprintf(t_mess,"%s/gripes",MESSDIR);
  }
 else
  {
   write_str(user,"***Wizard Note Wipe***");
   sprintf(t_mess,"%s/wizmess",MESSDIR);
  }
  
strncpy(filename,t_mess,FILE_NAME_LEN);

/*---------------------------------------------*/
/* check if there is any mail                  */
/*---------------------------------------------*/

if (!(bfp=fopen(filename,"r"))) 
  {
   write_str(user,"^There are no messages to wipe off the board.^"); 
   return;
  }
FCLOSE(bfp);

/*---------------------------------------------*/
/* get the delete parameters                   */
/*---------------------------------------------*/

get_bounds_to_delete(inpstr, &lower, &upper, &mode);
 
if (upper == -1 && lower == -1)
  {
   write_str(user,"No messages wiped.  Specification of what to ");
   write_str(user,"wipe did not make sense.  Type: .help wipe ");
   write_str(user,"for detailed instructions on use. ");
   return;
  }
    
   switch(mode)
    {
     case 0: return;
             break;
        
     case 1: 
            sprintf(mess,"Wiped all messages.");
            upper = -1;
            lower = -1;
            break;
        
     case 2: 
            sprintf(mess,"Wiped line %d.", lower);
            
            break;
        
     case 3: 
            sprintf(mess,"Wiped from line %d to the end.",lower);
            break;
        
     case 4: 
            sprintf(mess,"Wiped from begining of board to line %d.",upper);
            break;
        
     case 5: 
            sprintf(mess,"Wiped all except lines %d to %d.",upper, lower);
            break;
        
     default: return;
              break;
    }


remove_lines_from_file(user, 
                       filename, 
                       lower, 
                       upper);

write_str(user,mess);
if (wizard == 0)
  {
   astr[ustr[user].area].mess_num = file_count_lines(filename);
   if (!astr[ustr[user].area].mess_num)  remove(filename);
  }
 else
  {
    if (!file_count_lines(filename)) remove(filename);
  }
}

/*** sets room topic ***/
void set_topic(int user, char *inpstr)
{

if (!strlen(inpstr)) 
  {
   if (!strlen(astr[ustr[user].area].topic)) 
     {
      write_str(user,"There is no current topic here");  
      return;
     }
    else 
     {
      sprintf(mess,"Current topic is : %s",astr[ustr[user].area].topic);
      write_str(user,mess);  
      return;
     }
  }
	
if (strlen(inpstr)>TOPIC_LEN) 
  {
   write_str(user,"Topic description is too long");  
   return;
  }

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

strcpy(astr[ustr[user].area].topic,inpstr);

/* send output to users */
sprintf(mess,"Topic set to %s",inpstr);
write_str(user,mess);

if (!ustr[user].vis)
  sprintf(mess,"%s set the topic to %s",INVIS_ACTION_LABEL,inpstr);
 else
  sprintf(mess,"%s has set the topic to %s",ustr[user].say_name,inpstr);

writeall_str(mess, 1, user, 0, user, NORM, TOPIC, 0);

sprintf(mess,"%s: TOPIC in %s changed by %s to %s\n",get_time(0,0),
        astr[ustr[user].area].name,ustr[user].say_name,inpstr);
print_to_syslog(mess);

}


/*** force annoying user to quit prog ***/
void kill_user(int user, char *inpstr)
{
char name[ARR_SIZE];
char line[132];
int a;

int victim;

if (!strlen(inpstr))
  {
   write_str(user,"Kill who?");  
   return;
  }
	
sscanf(inpstr,"%s ",name);
strtolower(name);

remove_first(inpstr);

if (strlen(inpstr) > KILL_LEN) {
   sprintf(mess,"Kill message too long. Max is %d characters",KILL_LEN);
   write_str(user,mess);
   return;
   }

if ((victim=get_user_num(name,user)) == -1)
  {
   not_signed_on(user,name);  
   return;
  }

/* cant kill master user */
if (ustr[user].tempsuper<=ustr[victim].super) 
   {
    write_str(user,"That wouldn't be wise....");
    sprintf(mess,"%s actually tried to blast you, HAH!",ustr[user].say_name);
    write_str(victim,mess);
    return;
   }

sprintf(line,"KILL %s performed by %s",ustr[victim].say_name,ustr[user].say_name);
btell(user,line);
strcat(line,"\n");
print_to_syslog(line);

if (!strlen(inpstr)) {
  a = rand() % NUM_KILL_MESSAGES;
  sprintf(mess, kill_text[a], name);
  writeall_str(mess, 0, victim, 0, user, NORM, KILL, 0);
 }
else {
  strcpy(mess, inpstr);
  writeall_str(mess, 0, victim, 0, user, NORM, KILL, 0);
 }

write_str(victim,"");
write_str(victim,DEF_KILL_MESS);

/* kill user */
user_quit(victim);
}

/*** shutdown talk server ***/
void shutdown_d(int user, char *inpstr)
{
int u,inlen,said_reboot=0;
int j=0;
int fd;
unsigned int i;
char option[ARR_SIZE];
char timestr[30];
time_t tm;

#if defined(WIN32) && !defined(__CYGWIN32__)
PROCESS_INFORMATION p_info;
STARTUPINFO s_info;
char *args=strdup(thisprog); strcat(args," "); strcat(args,datadir);
#else
char *args[]={ PROGNAME,datadir,NULL };
#endif

if ((shutd == -1) && !strlen(inpstr)) goto NORMAL;
else if (strlen(inpstr) > 0) {
       sscanf(inpstr,"%s ",option);
         if (!strcmp(option,"cancel") || !strcmp(option,"-c")) {
          if (down_time==0) {
                  write_str(user,"Auto shutdown/reboot is not on in the first place!");
                  return;
                  }
           else {
                   down_time=0;
		if (treboot) {
                   write_str(user,"Auto reboot cancelled!");
		   treboot=0;
		   }
		else {
                   write_str(user,"Auto shutdown cancelled!");
		  }
                   return;
                 }
           } /* end of cancel if */
         if (!strcmp(option,"-r")) {            
            remove_first(inpstr);
              if (strlen(inpstr) > 0) {
               option[0]=0;
               sscanf(inpstr,"%s",option);
               said_reboot=1;
               }
              else { 
               treboot=1;
               goto NORMAL;
               }
            }
         if ((!strcmp(option,"0")) || (strlen(option) > 5)) {
            write_str(user,"Specify only numbers between 1 and 32766");
            return;
            }

         inlen=strlen(option);
         for (i=0;i<inlen;++i) {
           if (!isdigit((int)option[i])) 
              {
              write_str(user,"Specify only numbers between 1 and 32766");
              return;
              }
         }
           i=0;
          if (down_time > 0) {
	    if (said_reboot)
             write_str(user,"Auto-reboot is already enabled, cancel it first before you change the delay.");
	    else
             write_str(user,"Auto-shutdown is already enabled, cancel it first before you change the delay.");
             return;
             }
          i=atoi(option);

          if (i > 32766) { 
             write_str(user,"Minutes cant exceed 32766.");
             i=0;
             return;
             }
         i=0;
         down_time=atoi(option);
	if (said_reboot) {
         sprintf(t_mess,"System will automatically reboot in  %i minutes.",down_time);
         treboot=1;   /* actually tell the talker that reboot is on */
         }
	else {
         sprintf(t_mess,"System will be automatically shutdown in  %i  minutes.",down_time);
         treboot=0;
         }
         broadcast(user,t_mess);
         down_time=down_time+1;   /* Make sure down_time is at least 1 */
         return;                  /* to keep away from conflicts */
}

else goto START_DOWN;

NORMAL:
write_str_nr(user,"\nAre you sure about this (y/n)? ");
telnet_write_eor(user);
   shutd=user; 
   noprompt=1;
   return;

START_DOWN:
signal(SIGILL,SIG_IGN);
signal(SIGINT,SIG_IGN);
signal(SIGABRT,SIG_IGN);
signal(SIGFPE,SIG_IGN);
signal(SIGSEGV,SIG_IGN);
signal(SIGTERM,SIG_IGN);

#if !defined(WIN32) || defined(__CYGWIN32__)
signal(SIGTRAP,SIG_IGN);
#if !defined(__CYGWIN32__)
signal(SIGIOT,SIG_IGN);
#endif
signal(SIGBUS,SIG_IGN);
signal(SIGTSTP,SIG_IGN);
signal(SIGCONT,SIG_IGN);
signal(SIGHUP,SIG_IGN);
signal(SIGQUIT,SIG_IGN);
#if !defined(__CYGWIN32__)
signal(SIGURG,SIG_IGN);
#endif
signal(SIGPIPE,SIG_IGN);
signal(SIGTTIN,SIG_IGN);
signal(SIGTTOU,SIG_IGN);
#endif

write_str(user,"Quitting users...");

if (!treboot) {
sprintf(mess,"echo \"SYSTEM_NAME: %s\nPID: %u\nSTATUS: DOWN Normal shutdown\nEND:\n\" > .tinfo",
SYSTEM_NAME,(unsigned int)getpid());
system(mess);
system("mail tinfo@ncohafmuta.com < .tinfo");
}

if (treboot) {
#if defined(WIN32) && !defined(__CYGWIN32__)
  GetStartupInfo(&s_info);
#endif
}

for (u=0;u<MAX_USERS;++u) 
  {
   if (ustr[u].area == -1 && !ustr[u].logging_in) continue;
   if (u == user) continue;
   write_str(u, " ");
  if (treboot) {
   write_str(u,"^*** System rebooting ***^");
   write_bot("+++++ REBOOT");
   }
  else {
   write_str(u,"^*** System shutting down ***^");
   write_bot("+++++ SHUTDOWN");
   }
   write_str(u, " ");
   user_quit(u);
   }
   
write_str(user,"Logging shutdown...");
sprintf(mess,"%s.pid",thisprog);
remove(mess);
sysud(0,user);
check_mess(2);

write_str(user,"Now quitting you...");
if (treboot)
 write_str(user,"^* System will be back momentarily. *^");
else
 write_str(user,"^* System is now off. *^");

strtolower(ustr[user].name);

user_quit(user);

/* close listening sockets */
for (j=0;j<4;++j) {
 while (CLOSE(listen_sock[j]) == -1 && errno == EINTR)
	; /* empty while */
 /* FD_CLR(listen_sock[j],&readmask); */
 }

#if defined(WIN32) && !defined(__CYGWIN32__)
/* Shutdown winsock + timer thread before exit */
WSACleanup();
TerminateThread(hThread,0);
#endif

if (treboot) {
	/* If someone has changed the binary or the config filename while */
        /* this prog has been running this won't work */
        sleep(2);
        fd = open( "/dev/null", O_RDONLY );
        if (fd != 0) {
            dup2( fd, 0 );
            CLOSE(fd);
        }
        fd = open( "/dev/null", O_WRONLY );
        if (fd != 1) {
            dup2( fd, 1 );
            CLOSE(fd);
        }
        fd = open( "/dev/tty", O_WRONLY );
        if (fd != 2) {
            dup2( fd, 2 );
            CLOSE(fd);
        }

#if defined(WIN32) && !defined(__CYGWIN32__)
  CreateProcess(thisprog,args,NULL,NULL,0,
                DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS,
                NULL, NULL, &s_info, &p_info);
#else
  execvp(PROGNAME,args);
#endif

	/* If we get this far it hasn't worked */
	CLOSE(0);
	CLOSE(1);
	CLOSE(2);
        time(&tm);
        strcpy(timestr,ctime(&tm));
        timestr[strlen(timestr)-6]=0;  /* get rid of nl and year */
        sprintf(mess,"%s: REBOOT Failed!",timestr);
        print_to_syslog(mess);
        exit(12);
       }
exit(0); 
}

void shutdown_auto()
{
int u;
int j=0;
int fd;
char timestr[30];
time_t tm;

#if defined(WIN32) && !defined(__CYGWIN32__)
PROCESS_INFORMATION p_info;
STARTUPINFO s_info;
char *args=strdup(thisprog); strcat(args," "); strcat(args,datadir);
#else
char *args[]={ PROGNAME,datadir,NULL };
#endif

signal(SIGILL,SIG_IGN);
signal(SIGINT,SIG_IGN);
signal(SIGABRT,SIG_IGN);
signal(SIGFPE,SIG_IGN);
signal(SIGSEGV,SIG_IGN);
signal(SIGTERM,SIG_IGN);

#if !defined(WIN32) || defined(__CYGWIN32__)
signal(SIGTRAP,SIG_IGN);
#if !defined(__CYGWIN32__)
signal(SIGIOT,SIG_IGN);
#endif
signal(SIGBUS,SIG_IGN);
signal(SIGTSTP,SIG_IGN);
signal(SIGCONT,SIG_IGN);
signal(SIGHUP,SIG_IGN);
signal(SIGQUIT,SIG_IGN);
#if !defined(__CYGWIN32__)
signal(SIGURG,SIG_IGN);
#endif
signal(SIGPIPE,SIG_IGN);
signal(SIGTTIN,SIG_IGN);
signal(SIGTTOU,SIG_IGN);
#endif

if (!treboot) {
sprintf(mess,"echo \"SYSTEM_NAME: %s\nPID: %u\nSTATUS: DOWN Normal shutdown\nEND:\n\" > .tinfo",
SYSTEM_NAME,(unsigned int)getpid());
system(mess);
system("mail tinfo@ncohafmuta.com < .tinfo");
}

if (treboot) {
#if defined(WIN32) && !defined(__CYGWIN32__)
  GetStartupInfo(&s_info);
#endif
}

if (!num_of_users) goto CONT;

for (u=0;u<MAX_USERS;++u) 
  {
   if (ustr[u].area == -1 && !ustr[u].logging_in) continue;
   write_str(u, " ");
 if (user_wants_message(u,BEEPS)) {
  if (treboot)
   write_str(u,"\07^*** System auto-rebooting ***^\07");
  else
   write_str(u,"\07^*** System auto-shutting down ***^\07");
  }
 else {
  if (treboot)
   write_str(u,"^*** System auto-rebooting ***^");
  else
   write_str(u,"^*** System auto-shutting down ***^");
  }
   write_str(u, " ");
   user_quit(u);
   }

CONT:
check_mess(2);
if (treboot) {
 print_to_syslog("AUTO-REBOOT in progress..\n");
 write_bot("+++++ REBOOT");
 }
else {
 print_to_syslog("AUTO-SHUTDOWN executed\n");
 write_bot("+++++ SHUTDOWN");
 }

/* close listening sockets */
for (j=0;j<4;++j) {
 while (CLOSE(listen_sock[j]) == -1 && errno == EINTR)
	; /* empty while */
 /* FD_CLR(listen_sock[j],&readmask); */
 }

sprintf(mess,"%s.pid",thisprog);
remove(mess);
sysud(0,-1);

#if defined(WIN32) && !defined(__CYGWIN32__)
/* Shutdown winsock + timer thread before exit */
WSACleanup();
TerminateThread(hThread,0);
#endif

if (treboot) {
	/* If someone has changed the binary or the config filename while */
        /* this prog has been running this won't work */
        /* sprintf(args,"%s %s\0",PROGNAME,datadir); */
        sleep(2);
        fd = open( "/dev/null", O_RDONLY );
        if (fd != 0) {
            dup2( fd, 0 );
            CLOSE(fd);
        }
        fd = open( "/dev/null", O_WRONLY );
        if (fd != 1) {
            dup2( fd, 1 );
            CLOSE(fd);
        }
        fd = open( "/dev/tty", O_WRONLY );
        if (fd != 2) {
            dup2( fd, 2 );
            CLOSE(fd);
        }

#if defined(WIN32) && !defined(__CYGWIN32__)
  CreateProcess(thisprog,args,NULL,NULL,0,
                DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS,
                NULL, NULL, &s_info, &p_info);
#else
  execvp(PROGNAME,args);
#endif

	/* If we get this far it hasn't worked */
	CLOSE(0);
	CLOSE(1);
	CLOSE(2);
        time(&tm);
        strcpy(timestr,ctime(&tm));
        timestr[strlen(timestr)-6]=0;  /* get rid of nl and year */
        sprintf(mess,"%s: REBOOT Failed!",timestr);
        print_to_syslog(mess);
        exit(12);
       }

exit(0); 
}


void shutdown_error(int error)
{
int u;
int j=0;
int fd;
char timestr[30];
char message[256];
time_t tm;

#if defined(WIN32) && !defined(__CYGWIN32__)
PROCESS_INFORMATION p_info;
STARTUPINFO s_info;
char *args=strdup(thisprog); strcat(args," "); strcat(args,datadir);
#else
char *args[]={ PROGNAME,datadir,NULL };
#endif

signal(SIGILL,SIG_IGN);
signal(SIGINT,SIG_IGN);
signal(SIGABRT,SIG_IGN);
signal(SIGFPE,SIG_IGN);
signal(SIGSEGV,SIG_IGN);
signal(SIGTERM,SIG_IGN);

#if !defined(WIN32) || defined(__CYGWIN32__)
signal(SIGTRAP,SIG_IGN);
#if !defined(__CYGWIN32__)
signal(SIGIOT,SIG_IGN);
#endif
signal(SIGBUS,SIG_IGN);
signal(SIGTSTP,SIG_IGN);
signal(SIGCONT,SIG_IGN);
signal(SIGHUP,SIG_IGN);
signal(SIGQUIT,SIG_IGN);
#if !defined(__CYGWIN32__)
signal(SIGURG,SIG_IGN);
#endif
signal(SIGPIPE,SIG_IGN);
signal(SIGTTIN,SIG_IGN);
signal(SIGTTOU,SIG_IGN);
#endif

switch(error) {
   case 1: strcpy(message,"a failure to accept a new who socket"); break;
   case 2: strcpy(message,"a failure to set the who socket non-blocking"); break;
   case 3: strcpy(message,"a failure to accept a new www socket"); break;
   case 4: strcpy(message,"a failure to set the www socket non-blocking"); break;
   case 5: strcpy(message,"a failure to accept a new user socket"); break;
   case 6: strcpy(message,"a failure to accept a new wiz socket"); break;
   case 7: strcpy(message,"a failure to set the user or wiz socket non-blocking"); break;
   case 8: strcpy(message,"a loop panic. Talker broke out of main loop!"); break;
   case 9: strcpy(message,"an error during select()"); break;
   case 10: strcpy(message,"job kill"); break;
   case 11: strcpy(message,"a segmentation fault! - SIGSEGV"); break;
#if !defined(WIN32) || defined(__CYGWIN32__)
   case 12: strcpy(message,"a bus error! - SIGBUS"); break;
#endif
   case 13: strcpy(message,"the command line shutdown script"); break;
   case 14: strcpy(message,"a failure to write to the system logs"); break;
   default: strcpy(message,"an unknown failure"); break;
  }

if (!treboot) {
sprintf(mess,"echo \"SYSTEM_NAME: %s\nPID: %u\nSTATUS: DOWN %s\nEND:\n\" > .tinfo",
SYSTEM_NAME,(unsigned int)getpid(),message);
system(mess);
system("mail tinfo@ncohafmuta.com < .tinfo");
}

if (treboot) {
#if defined(WIN32) && !defined(__CYGWIN32__)
  GetStartupInfo(&s_info);
#endif
}

for (u=0;u<MAX_USERS;++u) 
  {
   if (ustr[u].area == -1 && !ustr[u].logging_in) continue;
   write_str(u, " ");
   if (treboot)
    sprintf(mess,"^*** System rebooting due to %s ***^",message);
   else
    sprintf(mess,"^*** System shutting down due to %s ***^",message);
   write_str(u,mess);
   write_str(u, " ");
   user_quit(u);
  }

check_mess(2);

time(&tm);
strcpy(timestr,ctime(&tm));
timestr[strlen(timestr)-6]=0;  /* get rid of nl and year */

if (treboot) {
 write_bot("+++++ REBOOT");
 sprintf(mess,"%s: REBOOTING due to %s\n",timestr,message);
 }
else {
 write_bot("+++++ SHUTDOWN");
 sprintf(mess,"%s: SHUTDOWN due to %s\n",timestr,message);
 }
print_to_syslog(mess);


/* close listening sockets */
for (j=0;j<4;++j) {
 while (CLOSE(listen_sock[j]) == -1 && errno == EINTR)
	; /* empty while */
 FD_CLR(listen_sock[j],&readmask);
 }

sprintf(mess,"%s.pid",thisprog);
remove(mess);
sysud(0,-1);

#if defined(WIN32) && !defined(__CYGWIN32__)
/* Shutdown winsock + timer thread before exit */
WSACleanup();
TerminateThread(hThread,0);
#endif

if (treboot) {
	/* If someone has changed the binary or the config filename while */
        /* this prog has been running this won't work */
        /* sprintf(args,"%s %s\0",PROGNAME,datadir); */
        sleep(2);
        fd = open( "/dev/null", O_RDONLY );
        if (fd != 0) {
            dup2( fd, 0 );
            CLOSE(fd);
        }
        fd = open( "/dev/null", O_WRONLY );
        if (fd != 1) {
            dup2( fd, 1 );
            CLOSE(fd);
        }
        fd = open( "/dev/tty", O_WRONLY );
        if (fd != 2) {
            dup2( fd, 2 );
            CLOSE(fd);
        }

#if defined(WIN32) && !defined(__CYGWIN32__)
  CreateProcess(thisprog,args,NULL,NULL,0,
                DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS,
                NULL, NULL, &s_info, &p_info);
#else
  execvp(PROGNAME,args);
#endif

	/* If we get this far it hasn't worked */
	CLOSE(0);
	CLOSE(1);
	CLOSE(2);
        time(&tm);
        strcpy(timestr,ctime(&tm));
        timestr[strlen(timestr)-6]=0;  /* get rid of nl and year */
        sprintf(mess,"%s: REBOOT Failed!\n",timestr);
        print_to_syslog(mess);
        exit(12);
       }

exit(0); 
}


/*** search for specific word in the message files ***/
void search_boards(int user, char *inpstr)
{
int b,occured=0;
char word[ARR_SIZE],filename[FILE_NAME_LEN];
char line[ARR_SIZE],line2[ARR_SIZE];
FILE *fp;

if (!strlen(inpstr)) 
  {
    write_str(user,"Search for what?");  
    return;
  }
  
sscanf(inpstr,"%s ",word);
strtolower(word);

if (!strcmp(word,"email")) {
  remove_first(inpstr);
  if (!strlen(inpstr)) 
    {
      write_str(user,"Search for what?");  
      return;
    }
  sprintf(t_mess,"%s/%s",MAILDIR,ustr[user].name);
  strncpy(filename,t_mess,FILE_NAME_LEN);
  if (!(fp=fopen(filename,"r"))) {
    strcpy(mess,"ERROR: Cant open user mailfile for searching!\n");
    write_str(user,mess);
    print_to_syslog(mess);
    return;
    }

	strtolower(inpstr);
	fgets(line,300,fp);
	while(!feof(fp)) {
		strcpy(line2,line);
		strtolower(line2);
		if (instr2(0,line2,inpstr,0)== -1) goto NEXT2;
		write_str(user,line);	
		++occured;
		NEXT2:
		fgets(line,300,fp);
		}
	FCLOSE(fp);
  if (!occured) write_str(user,"No occurences found");
  else {
        write_str(user," ");
	sprintf(mess,"%d occurence%s found",occured,occured == 1 ? "" : "s");
	write_str(user,mess);
       }
  return;
 }
/* look through boards */
for (b=0;b<NUM_AREAS;++b) {
	sprintf(t_mess,"%s/board%d",MESSDIR,b);
	strncpy(filename,t_mess,FILE_NAME_LEN);

	if (!(fp=fopen(filename,"r"))) continue;
	fgets(line,300,fp);
	while(!feof(fp)) {
		strcpy(line2,line);
		strtolower(line2);
		if (instr2(0,line2,word,0)== -1) goto NEXT;
		sprintf(mess,"%s : %s",astr[b].name,line);
		mess[0]=toupper((int)mess[0]);
		if (!astr[b].hidden || (ustr[user].tempsuper>=GRIPE_LEVEL))
		  {
		   write_str(user,mess);	
		   ++occured;
		  }
		NEXT:
		fgets(line,300,fp);
		}
	FCLOSE(fp);
	}
if (!occured) write_str(user,"No occurences found");
else {
        write_str(user," ");
	sprintf(mess,"%d occurence%s found",occured,occured == 1 ? "" : "s");
	write_str(user,mess);
	}
}



/*** review last lines of conversation in room ***/
void review(int user)
{
int area=ustr[user].area;
int pos=astr[area].conv_line % NUM_LINES;
int f;

write_str(user,"^----^ Start of review conversation buffer ^----^");

for (f = 0; f < NUM_LINES; ++f) 
    {
     if (strlen(conv[area][pos]) )
       {
        write_str(user,conv[area][pos]);
       }  
     pos = ++pos % NUM_LINES;
    }

write_str(user,"^----^ End of review conversation buffer ^----^");    
}


/*** help function ***/
void help(int user, char *inpstr)
{
int c,nl=0,d;
char command[20];
char z_mess[20];
char result[FILE_NAME_LEN];
char filename[FILE_NAME_LEN];
char *super=RANKS;
FILE *fp;

/* help for one command */
if (strlen(inpstr)) {

   if (strlen(inpstr) < 2) {
      write_str(user,"Pattern not unique enough.");
      return;
      }

	if (strstr(inpstr,"/")) {
           write_str(user,"String has illegal character in it.");
	   sprintf(mess,"User %s attempted to .help %s",ustr[user].say_name,inpstr);
	   logerror(mess);
	   return;
	   }

        if (inpstr[0]=='.') midcpy(inpstr,inpstr,1,ARR_SIZE);

	sprintf(result,"%s",get_help_file(inpstr,user));
	if (!strcmp(result,"failed")) {
	    write_str(user,"Sorry - there is no help on that command at the moment");
	    return;
	    }
	else if (!strcmp(result,"failed2"))
	    return;

        sprintf(filename,"%s/%s",HELPDIR,result);
        c=0;
        for (c=0; sys[c].jump_vector != -1 ;++c) {
	  command[0]=0;
	  midcpy(sys[c].command,command,1,ARR_SIZE);

          if (!strcmp(command,result)) {
            if (strlen(sys[c].command) >= 6)
             sprintf(mess,"Command:  %s\t\t\t\t\t\tLevel: %d",sys[c].command,sys[c].su_com);
            else
             sprintf(mess,"Command:  %s\t\t\t\t\t\t\tLevel: %d",sys[c].command,sys[c].su_com);

            write_str(user,mess);
            write_str(user,"");
            break;
            }
          }

	cat(filename,user,0);
	return;
	}

   strcpy(filename,get_temp_file());
   if (!(fp=fopen(filename,"w"))) {
     write_str(user,"Can't create file for paged help listing!");
     return;
     } 

/***** Ncohafmuta format help *****/
if (ustr[user].help==0) {
fputs("Remember - all commands start with a '.' and can be abbreviated\n",fp);
 sprintf(mess,"       ------- Currently these are the commands for Level  ^%i^ -------\n",ustr[user].tempsuper);
fputs(mess,fp);

nl = -1;
c = 0;
mess[0]=0;

   for (c=0; sys[c].jump_vector != -1 ;++c)
      {
	if (sys[c].type==COMM && sys[c].su_com<=ustr[user].tempsuper) {
	   if (nl == -1) {
	       fputs("   ^COMMS:^ ",fp);
	       nl=0;
	       }
           else if (nl == 0) {
               fputs("          ",fp);
               }
	 sprintf(t_mess,"%s %s",sys[c].command,sys[c].cabbr);
         midcpy(t_mess,t_mess,1,20);
	 if (nl==4) 
          sprintf(z_mess,"%s",t_mess);
	 else
          sprintf(z_mess,"%-12s",t_mess);
        strcat(mess,z_mess);
	++nl;
	if (nl==5)
	  {
	    fputs(mess,fp);
	    fputs("\n",fp);
            mess[0]=0;
	    nl= 0;
	  }
	 }   /* end of COMM if */
       }    /* end of  command for */
if (nl) {
 fputs(mess,fp);
 fputs("\n",fp);
}

nl = -1;
c = 0;
mess[0]=0;

   for (c=0; sys[c].jump_vector != -1 ;++c)
      {
	if (sys[c].type==INFO && sys[c].su_com<=ustr[user].tempsuper) {
	   if (nl == -1) {
	       fputs("    ^INFO:^ ",fp);
	       nl=0;
	       }
           else if (nl == 0) {
               fputs("          ",fp);
               }
	 sprintf(t_mess,"%s %s",sys[c].command,sys[c].cabbr);
         midcpy(t_mess,t_mess,1,20);
	 if (nl==4) 
          sprintf(z_mess,"%s",t_mess);
	 else
          sprintf(z_mess,"%-12s",t_mess);
        strcat(mess,z_mess);
	++nl;
	if (nl==5)
	  {
	    fputs(mess,fp);
	    fputs("\n",fp);
            mess[0]=0;
	    nl= 0;
	  }
	 }   /* end of INFO if */
       }    /* end of  command for */
if (nl) {
 fputs(mess,fp);
 fputs("\n",fp);
}

nl = -1;
c = 0;
mess[0]=0;

   for (c=0; sys[c].jump_vector != -1 ;++c)
      {
	if (sys[c].type==MAIL && sys[c].su_com<=ustr[user].tempsuper) {
	   if (nl == -1) {
	       fputs("    ^MAIL:^ ",fp);
	       nl=0;
	       }
           else if (nl == 0) {
               fputs("          ",fp);
               }
	 sprintf(t_mess,"%s %s",sys[c].command,sys[c].cabbr);
         midcpy(t_mess,t_mess,1,20);
	 if (nl==4) 
          sprintf(z_mess,"%s",t_mess);
	 else
          sprintf(z_mess,"%-12s",t_mess);
        strcat(mess,z_mess);
	++nl;
	if (nl==5)
	  {
	    fputs(mess,fp);
	    fputs("\n",fp);
            mess[0]=0;
	    nl= 0;
	  }
	 }   /* end of MAIL if */
       }    /* end of  command for */
if (nl) {
 fputs(mess,fp);
 fputs("\n",fp);
}

nl = -1;
c = 0;
mess[0]=0;

   for (c=0; sys[c].jump_vector != -1 ;++c)
      {
	if (sys[c].type==MESG && sys[c].su_com<=ustr[user].tempsuper) {
	   if (nl == -1) {
	       fputs("^MESSAGIN:^ ",fp);
	       nl=0;
	       }
           else if (nl == 0) {
               fputs("          ",fp);
               }
	 sprintf(t_mess,"%s %s",sys[c].command,sys[c].cabbr);
         midcpy(t_mess,t_mess,1,20);
	 if (nl==4) 
          sprintf(z_mess,"%s",t_mess);
	 else
          sprintf(z_mess,"%-12s",t_mess);
        strcat(mess,z_mess);
	++nl;
	if (nl==5)
	  {
	    fputs(mess,fp);
	    fputs("\n",fp);
            mess[0]=0;
	    nl= 0;
	  }
	 }   /* end of MESG if */
       }    /* end of  command for */
if (nl) {
 fputs(mess,fp);
 fputs("\n",fp);
}

nl = -1;
c = 0;
mess[0]=0;

   for (c=0; sys[c].jump_vector != -1 ;++c)
      {
	if (sys[c].type==MISC && sys[c].su_com<=ustr[user].tempsuper) {
	   if (nl == -1) {
	       fputs("    ^MISC:^ ",fp);
	       nl=0;
	       }
           else if (nl == 0) {
               fputs("          ",fp);
               }
	 sprintf(t_mess,"%s %s",sys[c].command,sys[c].cabbr);
         midcpy(t_mess,t_mess,1,20);
	 if (nl==4) 
          sprintf(z_mess,"%s",t_mess);
	 else
          sprintf(z_mess,"%-12s",t_mess);
        strcat(mess,z_mess);
	++nl;
	if (nl==5)
	  {
	    fputs(mess,fp);
	    fputs("\n",fp);
            mess[0]=0;
	    nl= 0;
	  }
	 }   /* end of MISC if */
       }    /* end of  command for */
if (nl) {
 fputs(mess,fp);
 fputs("\n",fp);
}

nl = -1;
c = 0;
mess[0]=0;

   for (c=0; sys[c].jump_vector != -1 ;++c)
      {
	if (sys[c].type==MOOV && sys[c].su_com<=ustr[user].tempsuper) {
	   if (nl == -1) {
	       fputs("^MOVEMENT:^ ",fp);
	       nl=0;
	       }
           else if (nl == 0) {
               fputs("          ",fp);
               }
	 sprintf(t_mess,"%s %s",sys[c].command,sys[c].cabbr);
         midcpy(t_mess,t_mess,1,20);
	 if (nl==4) 
          sprintf(z_mess,"%s",t_mess);
	 else
          sprintf(z_mess,"%-12s",t_mess);
        strcat(mess,z_mess);
	++nl;
	if (nl==5)
	  {
	    fputs(mess,fp);
	    fputs("\n",fp);
            mess[0]=0;
	    nl= 0;
	  }
	 }   /* end of MOOV if */
       }    /* end of  command for */
if (nl) {
 fputs(mess,fp);
 fputs("\n",fp);
}

nl = -1;
c = 0;
mess[0]=0;

   for (c=0; sys[c].jump_vector != -1 ;++c)
      {
	if (sys[c].type==BANS && sys[c].su_com<=ustr[user].tempsuper) {
	   if (nl == -1) {
	       fputs(" ^BANNING:^ ",fp);
	       nl=0;
	       }
           else if (nl == 0) {
               fputs("          ",fp);
               }
	 sprintf(t_mess,"%s %s",sys[c].command,sys[c].cabbr);
         midcpy(t_mess,t_mess,1,20);
	 if (nl==4) 
          sprintf(z_mess,"%s",t_mess);
	 else
          sprintf(z_mess,"%-12s",t_mess);
        strcat(mess,z_mess);
	++nl;
	if (nl==5)
	  {
	    fputs(mess,fp);
	    fputs("\n",fp);
            mess[0]=0;
	    nl= 0;
	  }
	 }   /* end of BANS if */
       }    /* end of  command for */
if (nl) {
 fputs(mess,fp);
 fputs("\n",fp);
}

nl = -1;
c = 0;
mess[0]=0;

   for (c=0; sys[c].jump_vector != -1 ;++c)
      {
	if (sys[c].type==SETS && sys[c].su_com<=ustr[user].tempsuper) {
	   if (nl == -1) {
	       fputs("^SETTINGS:^ ",fp);
	       nl=0;
	       }
           else if (nl == 0) {
               fputs("          ",fp);
               }
	 sprintf(t_mess,"%s %s",sys[c].command,sys[c].cabbr);
         midcpy(t_mess,t_mess,1,20);
	 if (nl==4) 
          sprintf(z_mess,"%s",t_mess);
	 else
          sprintf(z_mess,"%-12s",t_mess);
        strcat(mess,z_mess);
	++nl;
	if (nl==5)
	  {
	    fputs(mess,fp);
	    fputs("\n",fp);
            mess[0]=0;
	    nl= 0;
	  }
	 }   /* end of SETS if */
       }    /* end of  command for */
if (nl) {
 fputs(mess,fp);
 fputs("\n",fp);
}

fputs("\n",fp);
fputs("For further help type  .help <command>\n",fp);

fclose(fp);
cat(filename,user,0);
}

/***** Iforms format help *****/
else if (ustr[user].help==1) {
fputs("Remember - all commands start with a '.' and can be abbreviated\n",fp);
 sprintf(mess,"       ------- Currently these are the commands for Level  ^%i^ -------\n",ustr[user].tempsuper);
fputs(mess,fp);

nl = -1;
    
for (d=0;d<ustr[user].tempsuper+1;d++)
  {
    
    if (nl!= -1)
      {
       fputs(" \n",fp);
      }

    sprintf(mess,"%c)",super[d]);
    fputs(mess,fp);
    nl=0;
   
    for (c=0; sys[c].su_com != -1 ;++c)
      {
        if (sys[c].type==NONE) continue;
        sprintf(mess,"%-11.11s",sys[c].command);
        mess[0]=' ';
        if (d!=sys[c].su_com) continue;
        if (nl== -1)
          {fputs("  ",fp);
           nl=0;
          }
        fputs(mess,fp);
        ++nl;
        if (nl==6)
          {
            fputs(" \n",fp);
            nl= -1;
          }
       }
   }
if (nl) fputs(" \n",fp);
fputs(" \n",fp);
fputs("For further help type  .help <command>\n",fp);
fclose(fp);
cat(filename,user,0);
}

/***** Nuts 3 format help *****/
else if (ustr[user].help==2) {
 sprintf(mess,"*** Commands available for user level ^%s^ ***\n",ranks[ustr[user].tempsuper]);
fputs(mess,fp);

nl = -1;
    
for (d=0;d<ustr[user].tempsuper+1;d++)
  {
    
    if (nl!= -1)
      {
       fputs(" \n",fp);
      }

    sprintf(mess,"^HC(%s)^",ranks[d]);
    fputs(mess,fp);
    fputs("\n",fp);
    nl=0;
    c=0;

    for (c=0; sys[c].jump_vector != -1 ;++c)
      {
        if (sys[c].type==NONE) continue;
        if (ustr[user].tempsuper < sys[c].su_com) continue;
        sprintf(mess,"%-11s ",sys[c].command);
        mess[0]=' ';
        if (d!=sys[c].su_com) continue;
        if (nl == -1) nl=0;
        fputs(mess,fp);
        ++nl;
        if (nl==6) {
          fputs("\n",fp);
          nl=-1;
          }
        }

} /* end of level for */

if (nl) fputs(" \n",fp);
fputs(" \n",fp);
fputs("For further help type  .help <command>\n",fp);

fclose(fp);
cat(filename,user,0);
}

/***** Nuts 2 format help *****/
else if (ustr[user].help==3) {
 sprintf(mess,"*** Commands available for user level ^%s^ ***\n",ranks[ustr[user].tempsuper]);
fputs(mess,fp);

    for (c=0; sys[c].jump_vector != -1 ;++c)
      {
        if (sys[c].type==NONE) continue;
        if (ustr[user].tempsuper < sys[c].su_com) continue;
        sprintf(mess,"%-11s ",sys[c].command);
        mess[0]=' ';
        fputs(mess,fp);
        ++nl;
        if (nl==6) {  fputs("\n",fp);  nl=0; }
      }

if (nl) fputs(" \n",fp);
fputs(" \n",fp);
fputs("For further help type  .help <command>\n",fp);

fclose(fp);
cat(filename,user,0);
}

}


/*---------------------------------------*/
/* Figure out full help filename from    */
/* inputed abbreviation..                */
/* something like get_user_num()         */
/*---------------------------------------*/
char *get_help_file(char *i_name, int user)
{
int found = 0;
static char last[64];
char name[ARR_SIZE];
char small_buff[64];
char z_mess[600];
char filerid[FILE_NAME_LEN];
struct dirent *dp;
DIR  *dirp;

strcpy(z_mess,"");
strcpy(last,"");
strcpy(name,i_name);

 sprintf(t_mess,"%s",HELPDIR);
 strncpy(filerid,t_mess,FILE_NAME_LEN);

 dirp=opendir((char *)filerid);

 if (dirp == NULL)
   {
    write_str(user,"Directory information not found.");
    strcpy(last,"failed2");
    return last;
   }

 while ((dp = readdir(dirp)) != NULL)
   {
    sprintf(small_buff,"%s",dp->d_name);
    if (small_buff[0]!='.') {
       if (!strcmp(small_buff,name))
	{
	 found = -1;
	 break;
	}
       if ((instr2(0, small_buff, name, 1) != -1) && (found != -1))
	{
	  strcat(z_mess, small_buff);
	  strcat(z_mess, " ");
	  found++;
	  strcpy(last,small_buff);
	}
    }
    small_buff[0]=0;
   }       /* End of while */

 if (found == -1) {
    small_buff[0]=0;
    strcpy(last,name);
    (void) closedir(dirp);
    return last;
    }

 if (found == 0) {
    (void) closedir(dirp);
    strcpy(last,"failed");
    return last;
    }

if (found >1)
  {
   sprintf(mess, "String was not unique, matched: %s", z_mess);
   write_str(user,mess);
   (void) closedir(dirp);
   strcpy(last,"failed2");
   return last;
  }
 else {
  (void) closedir(dirp);
  return last;
  }
}


/*** broadcast message to everyone without the "X shouts:" bit ***/
void broadcast(int user, char *inpstr)
{
if (!strlen(inpstr)) {
	write_str(user,"Broadcast what?");  return;
	}

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

sprintf(mess,"*** [ %s ] ***",inpstr);
writeall_str(mess, 0, user, 1, user, BOLD, BCAST, 0);
sprintf(mess,"%s: BROADCAST MESSAGE from %s\n",get_time(0,0),ustr[user].say_name);
print_to_syslog(mess);
}



/*** give system status ***/
void system_status(int user)
{
int per_day;
int new_per_day;
int days;
int hours;
long minutes;
int ms;
float total_time;
float dec_hours;
char stm[30];
char onoff[2][4];
char yesno[2][4];
char new[3][4];
time_t tm;

strcpy(onoff[0],"OFF");
strcpy(onoff[1],"ON ");
strcpy(yesno[0],"NO ");
strcpy(yesno[1],"YES");
strcpy(new[0],"NO ");
strcpy(new[1],"VFY");
strcpy(new[2],"YES");
write_str(user,"+------------------------SYSTEM STATUS----------------------+");
write_str(user,"| Overall:                                                  |");

time(&tm);
strcpy(stm,ctime(&start_time));
stm[strlen(stm)-1]=0;  /* get rid of nl */

minutes=(tm-start_time)/60;
days=(int)minutes/1440;
ms=(int)minutes%1440;
hours=ms/60;
dec_hours= (float)hours/24;

 if (days==0) {
     per_day=system_stats.logins_since_start;
     new_per_day=system_stats.new_since_start;
     }
 else {
     total_time=days+dec_hours;
     per_day=system_stats.logins_since_start / total_time;
     new_per_day=system_stats.new_since_start / total_time;
     }

sprintf(mess,  "|        System started: %24.24s           |",stm);
write_str(user,mess);
sprintf(mess,  "|                        %29.29s ago  |",converttime((long)((tm-start_time)/60)));
write_str(user,mess);
write_str(user,"|                                                           |");
write_str(user,"|        Atmos    Sys Open    New users     Max Users       |");
sprintf(mess,  "|         %s        %s         %s           %3.3d          |",
             onoff[atmos_on], yesno[sys_access], new[allow_new], MAX_USERS);
write_str(user,mess);

if (atmos_on)
  {
   write_str(user,"|                                                           |");
   write_str(user,"| Atmos: Cycle Time       Factor           Count   Last     |");
   sprintf(mess,  "|        %4d             %4d            %4d    %3d       |",
                                    ATMOS_RESET, ATMOS_FACTOR, ATMOS_COUNTDOWN, ATMOS_LAST);
   write_str(user,mess);
  }

/*
write_str(user,"|                                                           |");
*/     
write_str(user,"|        New Total        New Today        New Per Day      |");
sprintf(mess,  "|           %3.3ld              %3.3ld               %3.3d          |",
                                            system_stats.new_since_start,
system_stats.new_users_today, new_per_day);

write_str(user,mess);
/*
write_str(user,"|                                                           |");
*/
write_str(user,"|        Logins Total     Logins Today     Avg Per Day      |");
sprintf(mess,  "|         %10.10ld         %6.6ld            %3.3d          |",
system_stats.logins_since_start, system_stats.logins_today, per_day);
write_str(user,mess);
/*
write_str(user,"|                                                           |");
*/
write_str(user,"|        New Quota        Message Life     Total expired    |");
sprintf(mess,  "|           %3.3ld              %3.3d days          %3.3ld          |",
system_stats.quota, MESS_LIFE, system_stats.tot_expired);
write_str(user,mess);
write_str(user,"|        Cache Hits       Cache Misses                      |");
sprintf(mess,  "|           %4.4d             %4.4d                           |",
system_stats.cache_hits,system_stats.cache_misses);
write_str(user,mess);
write_str(user,"+-----------------------------------------------------------+");
write_str(user,"^");

per_day=0;
new_per_day=0;
total_time=0;
dec_hours=0;
}


/*** move user somewhere else ***/
void move(int user, char *inpstr)
{
char other_user[ARR_SIZE],area_name[260], tempstr[50];
int area,user2,online=1;

/* check user */
if (!strlen(inpstr)) 
  {
   user2 = user;
   area = INIT_ROOM;
   write_str(user,"^HB*** Warp to main room ***^");  
   goto FOUND;
  }
  
sscanf(inpstr,"%s %s",other_user,area_name);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

strtolower(other_user);

user2 = get_user_num(other_user,user);
if (user2 == -1) 
  {
   if (!check_for_user(other_user)) {
      write_str(user,NO_USER_STR);
      return;
      }
   read_user(other_user);
   online=0;
  }
else { }

if (online==1) {
/* see if user is moving himself */
if (user==user2) 
  {
   write_str(user,"What do you want to do that for?");
   return;
  }

 if ((!strcmp(ustr[user2].name,ROOT_ID)) || (!strcmp(ustr[user2].name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    return;
    }
  
/* see if user to be moved is superior */
if (ustr[user].tempsuper < ustr[user2].super) 
  {
   write_str(user,"Hmm... inadvisable");
   sprintf(mess,"%s thought about moving you",ustr[user].say_name);
   write_str(user2,mess);
   return;
  }
	
/* check area */
remove_first(inpstr);
if (!strlen(inpstr)) 
  {
   area = INIT_ROOM;
   sprintf(mess,"%s moved to %s",ustr[user2].say_name, astr[area].name);
   write_str(user,mess);  
   goto FOUND;
  }

if (!strcmp(inpstr,HEAD_ROOM)) {
   write_str(user,"Authorized personnel only..sorry");
   return;
   }
  
for (area=0;area<NUM_AREAS;++area) 
	if (!strcmp(astr[area].name,area_name)) goto FOUND;
		
write_str(user, NO_ROOM);
return;

FOUND:
if (area==ustr[user2].area) 
  {
   sprintf(mess,"%s is already in that room!",ustr[user2].say_name);
   write_str(user,mess);
   return;
  }

/*-----------------------------------------------------------*/
/* if the room is private abort move...inform user           */
/*-----------------------------------------------------------*/

if (astr[area].private && ustr[user2].invite != area ) 
  {
   write_str(user,"Sorry - that room is currently private");
   return;
  }

/** send output **/
write_str(user2,MOVE_TOUSER);

if (!ustr[user2].vis) 
  strcpy(tempstr,INVIS_ACTION_LABEL);
else
  strcpy(tempstr,ustr[user2].say_name);

/** to old area */
sprintf(mess,MOVE_TOREST,tempstr);
writeall_str(mess, 1, user2, 0, user, NORM, MOVE, 0);

   if (!strcmp(astr[ustr[user2].area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ left:%s", ustr[user2].say_name);
    write_bot(mess);
    }

if ((find_num_in_area(ustr[user2].area) <= PRINUM) && astr[ustr[user2].area].private)
  {
    strcpy(mess, NOW_PUBLIC);
    writeall_str(mess, 1, user2, 0, user, NORM, NONE, 0);
    astr[ustr[user2].area].private=0;
  }
  
ustr[user2].area=area;
look(user2,"");

/* to new area */
sprintf(mess,MOVE_TONEW,tempstr,"s");
writeall_str(mess, 1, user2, 0, user, NORM, MOVE, 0);

   if (!strcmp(astr[ustr[user2].area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ came in:%s", ustr[user2].say_name);
    write_bot(mess);
    }
}
else {

 if ((!strcmp(t_ustr.name,ROOT_ID)) || (!strcmp(t_ustr.name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    return;
    }
  
/* see if user to be moved is superior */
if (ustr[user].tempsuper < t_ustr.super) 
  {
   write_str(user,"Hmm... inadvisable");
   return;
  }
	
/* check area */
remove_first(inpstr);
if (!strlen(inpstr)) 
  {
   area = INIT_ROOM;
   goto NEW_FOUND;
  }

if (!strcmp(inpstr,HEAD_ROOM)) {
   write_str(user,"Authorized personnel only..sorry");
   return;
   }
  
for (area=0;area<NUM_AREAS;++area) 
	if (!strcmp(astr[area].name,area_name)) goto NEW_FOUND;
		
write_str(user, NO_ROOM);
return;

NEW_FOUND:
if (area==t_ustr.area) 
  {
   sprintf(mess,"%s is already in that room!",t_ustr.say_name);
   write_str(user,mess);
   return;
  }

t_ustr.area=area;
sprintf(mess,"%s moved to %s",t_ustr.say_name, astr[t_ustr.area].name);
write_str(user,mess);  
write_user(other_user);
sprintf(mess,"%s moved %s (offline) to %s\n",ustr[user].name,t_ustr.name, astr[t_ustr.area].name);
print_to_syslog(mess);
}

write_str(user,"Ok");
}



/*** set system access to allow or disallow further logins ***/
void system_access(int user, char *inpstr, int co)
{
char line[132];

if (!strlen(inpstr)) {
   sprintf(line,"Main port is     %s",opcl[sys_access]);
   write_str(user,line);
   sprintf(line,"Wizard port is   %s",opcl[wiz_access]);
   write_str(user,line);
   sprintf(line,"Who port is      %s",opcl[who_access]);
   write_str(user,line);
   sprintf(line,"WWW port is      %s",opcl[www_access]);
   write_str(user,line);
   return;
   }

strtolower(inpstr);

if (!co) {
  if(!strcmp(inpstr,"all")) {
        sprintf(line,"ALL PORTS CLOSED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: ALL PORTS CLOSED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	strcpy(line,"*** System is now closed to all further logins ***");
	writeall_str(line, 0, user, 1, user, BOLD, NONE, 0);
	sys_access=0;

	if (WIZ_OFFSET!=0)
        wiz_access=0;

	if (WHO_OFFSET!=0)
        who_access=0;

	if (WWW_OFFSET!=0)
        www_access=0;

	return;
       }
  if(!strcmp(inpstr,"main")) {
        sprintf(line,"MAIN PORT CLOSED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: MAIN PORT CLOSED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	sys_access=0;
	return;
       }
  if(!strcmp(inpstr,"wiz")) {
	if (WIZ_OFFSET==0) {
	write_str(user,"The wizard port has been disabled in the code.");
	return;
	}
        sprintf(line,"WIZ PORT CLOSED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: WIZ PORT CLOSED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	wiz_access=0;
	return;
       }
  if(!strcmp(inpstr,"who")) {
	if (WHO_OFFSET==0) {
	write_str(user,"The who port has been disabled in the code.");
	return;
	}
        sprintf(line,"WHO PORT CLOSED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: WHO PORT CLOSED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	who_access=0;
	return;
       }
  if(!strcmp(inpstr,"www")) {
	if (WWW_OFFSET==0) {
	write_str(user,"The web port has been disabled in the code.");
	return;
	}
        sprintf(line,"WWW PORT CLOSED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: WWW PORT CLOSED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	www_access=0;
	return;
       }
  else {
        write_str(user,"Unknown option.");
        return;
       }
  }  /* end of co if */
else {	
  if(!strcmp(inpstr,"all")) {
        sprintf(line,"ALL PORTS OPENED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: ALL PORTS OPENED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	strcpy(line,"*** System is now open to all further logins ***");
	writeall_str(line, 0, user, 1, user, BOLD, NONE, 0);
	sys_access=1;

	if (WIZ_OFFSET!=0)
        wiz_access=1;

	if (WHO_OFFSET!=0)
        who_access=1;

	if (WWW_OFFSET!=0)
        www_access=1;

	return;
       }
  if(!strcmp(inpstr,"main")) {
        sprintf(line,"MAIN PORT OPENED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: MAIN PORT OPENED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	sys_access=1;
	return;
       }
  if(!strcmp(inpstr,"wiz")) {
	if (WIZ_OFFSET==0) {
	write_str(user,"The wizard port has been disabled in the code.");
	return;
	}
        sprintf(line,"WIZ PORT OPENED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: WIZ PORT OPENED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	wiz_access=1;
	return;
       }
  if(!strcmp(inpstr,"who")) {
	if (WHO_OFFSET==0) {
	write_str(user,"The who port has been disabled in the code.");
	return;
	}
        sprintf(line,"WHO PORT OPENED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: WHO PORT OPENED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	who_access=1;
	return;
       }
  if(!strcmp(inpstr,"www")) {
	if (WWW_OFFSET==0) {
	write_str(user,"The web port has been disabled in the code.");
	return;
	}
        sprintf(line,"WWW PORT OPENED BY %s",ustr[user].say_name);
        btell(user,line);
        sprintf(line,"%s: WWW PORT OPENED BY %s\n",get_time(0,0),ustr[user].say_name);
        print_to_syslog(line);
	www_access=1;
	return;
       }
  else {
        write_str(user,"Unknown option.");
        return;
       }
 }

}



/*** echo function writes straight text to screen ***/
void echo(int user, char *inpstr)
{
char fword[ARR_SIZE];
char *err="Sorry - you cant echo that";
int u=0;
int area;

if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

if (!strlen(inpstr))
  {
   write_str(user,"Echo what?");  
   return;
  }

if (ustr[user].frog) strcpy(inpstr,FROG_ECHO);

/* get first word & check it for illegal words */
sscanf(inpstr,"%s",fword);
if (   
    instr2(0,fword,"SYSTEM",0) != -1||
    instr2(0,fword,"***",0) != -1||
    instr2(0,fword,"myster",0) != -1 ||
    instr2(0,fword,"Someone",0) != -1||
    instr2(0,fword,"someone",0) != -1||
    instr2(0,fword,"[",0) != -1||
    instr2(0,fword,"-->",0) != -1) 
    {
     write_str(user,err);  
     return;
    }


if (strstr(fword,"^")) {
    write_str(user,"System does not allow the hiliting of echos.");
    return;
   }

/* check for user names */
strtolower(fword);

for (u=0;u<MAX_USERS;++u) {
	if ((instr2(0,fword,ustr[u].name,0)!= -1) ||
            (instr2(0,fword,BOTS_ROOTID,0)!= -1)) {
		write_str(user,err);  return;
		}
	}

/* even check if user is not online */
if (check_for_user(fword)) {
  write_str(user,err);
  return;
  }

/* write message */
strcpy(mess,inpstr);
mess[0]=toupper((int)mess[0]);
write_str(user,mess);
writeall_str(mess, 1, user, 0, user, NORM, ECHOM, 0);

/*-----------------------------------*/
/* store the echo in the rev buffer  */
/*-----------------------------------*/

area = ustr[user].area;
strncpy(conv[area][astr[area].conv_line],mess,MAX_LINE_LEN);
astr[area].conv_line = ( ++astr[area].conv_line ) % NUM_LINES;

}



/*** set user description ***/
void set_desc(int user, char *inpstr)
{

if (!strlen(inpstr)) 
  {
   sprintf(mess,"Your description is : %s",ustr[user].desc);
   write_str(user,mess);  
   return;
  }

if (!strcmp(inpstr,DEF_DESC) && !strcmp(ustr[user].desc,DEF_DESC)) {
   write_str(user,"No, you need to set a different one.");
   return;
   }

strcat(inpstr,"@@");
  
if (strlen(inpstr) > DESC_LEN-1) 
  {
    write_str(user,"Description too long");  
    return;
  }

strcpy(ustr[user].desc,inpstr);

if (autopromote == 1)
 check_promote(user,6);

if ((ustr[user].tempsuper==0) && (ustr[user].area==new_room))
 strcpy(ustr[user].home_room,astr[INIT_ROOM].name);

copy_from_user(user);
write_user(ustr[user].name);
sprintf(mess,"New desc: %s",ustr[user].desc);
write_str(user,mess);

}


/*** print out greeting in large letters ***/
void greet(int user, char *inpstr)
{
char pbuff[256];
int slen,lc,c,i,j,found=0;

if (ustr[user].frog) return;

slen = strlen(inpstr);
if (!slen) 
  {
   write_str(user,"Greet whom?"); 
   return;
  }
  
if (slen>10) slen=10;

/* check for special characters in string */ 
for (i=0;i<slen;++i) {
        if (!isalpha((int)inpstr[i])) { found=1; break; }
}
if (found==1) {
write_str(user,"Message cannot have special characters in it.");
return;
}       
i=0;

strcpy(mess,"");
write_str(user,mess);
writeall_str(mess, 1, user, 0, user, NORM, GREET, 0);

for (i=0; i<5; ++i) 
  {
   pbuff[0] = '\0';
   for (c=0; c<slen; ++c) 
     {
      lc = tolower((int)inpstr[c]) - 'a';
      if (lc >= 0 && lc < 27) 
        {
         for (j=0;j<5;++j)
           {
            if(biglet[lc][i][j]) 
              strcat(pbuff,"#"); 
             else 
              strcat(pbuff," ");
           }
         strcat(pbuff,"  ");
        }
      }
   sprintf(mess,"%s",pbuff);
   write_str(user,mess);
   writeall_str(mess, 1, user, 0, user, NORM, GREET, 0);
  }
strcpy(mess,"");
write_str(user,mess);
writeall_str(mess, 1, user, 0, user, NORM, GREET, 0);
}


/** Place a user under arrest, move him to brig **/
/** or unarrest user                            **/
void arrest(int user, char *inpstr, int mode)
{
char other_user[ARR_SIZE],area_name[260];
int area,user2;

/* check for user to move */
if (!strlen(inpstr)) 
  {
   if (mode==0)
    write_str(user,"Arrest whom?"); 
   else if (mode==1)
    write_str(user,"UNarrest whom?");
   return;
  }
 
sscanf(inpstr,"%s",other_user);
if ((user2=get_user_num(other_user,user))== -1) 
  {
    not_signed_on(user,other_user); 
    return;
  }

/* User cannot arrest himself */
if (user==user2) 
  {
   write_str(user,"What do you want to arrest/unarrest yourself for?");
   return;
  }

 if ((!strcmp(ustr[user2].name,ROOT_ID)) || (!strcmp(ustr[user2].name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    return;
    }

/* See if arrest target user is an SU */
if (ustr[user].tempsuper < ustr[user2].super) 
  {
   write_str(user,"That would not be wise");
   if (mode==0)
   sprintf(mess,"%s thought of placing you under arrest.",ustr[user].say_name);
   else if (mode==1)
   sprintf(mess,"%s thought of unarresting you.",ustr[user].say_name);
   write_str(user2,mess);
   return;
  }

if (mode==0) {
/* Define target area */
sprintf(area_name,"%s",ARREST_ROOM);
for (area=0; area<NUM_AREAS; ++area)
  {
   if (!strcmp(astr[area].name,area_name)) goto FOUND;
  }
  
write_str(user,"Unexpected Error: Arrest_Area Not Found");
return;

FOUND:
if (area==ustr[user2].area) 
  {
   sprintf(mess,"%s is already under arrest!",ustr[user2].say_name);
   write_str(user,mess);
   return;
  }

/** Send output **/
write_str(user2,ARREST_TOUSER);

/* to old area */
sprintf(mess,ARREST_TOREST,ustr[user2].say_name);
writeall_str(mess, 1, user2, 0, user, NORM, NONE, 0);

   if (!strcmp(astr[ustr[user2].area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ left:%s", ustr[user2].say_name);
    write_bot(mess);
    }

muzzle(user,ustr[user2].name,1);
gag_comm(user,ustr[user2].name,1);

if ((find_num_in_area(ustr[user2].area)<=PRINUM) && astr[ustr[user2].area].private)
  {
   strcpy(mess, NOW_PUBLIC);
   writeall_str(mess, 1, user2, 0, user, NORM, NONE, 0);
   astr[ustr[user2].area].private=0;
  }

ustr[user2].area=area;
look(user2,"");

/* to brig area */
sprintf(mess,ARREST_TOJAIL,ustr[user2].say_name);
writeall_str(mess, 1, user2, 0, user, NORM, NONE, 0);
sprintf(mess, "ARREST: %s by %s",ustr[user2].say_name,ustr[user].say_name);
btell(user,mess);
strcat(mess,"\n");
print_to_syslog(mess);
write_str(user,"Ok");
}
else if (mode==1) {
  unmuzzle(user,ustr[user2].name);
  gag_comm(user,ustr[user2].name,2);
  ustr[user2].area = INIT_ROOM;
  look(user2,"");
  sprintf(mess, "UNARREST: %s by %s",ustr[user2].say_name,ustr[user].say_name);
  btell(user,mess);
  strcat(mess,"\n");
  print_to_syslog(mess);
 }

}


/** Clear the conversation buffer(s) **/
void cbuff2(int user, char *inpstr)
{
int i,area=ustr[user].area;

if (!strlen(inpstr)) {
 for (i=0;i<NUM_LINES;++i) conv[area][i][0]=0;
 write_str(user,"Conversation buffer cleared!");
 }

else if (!strcmp(inpstr,"temp") && (ustr[user].tempsuper >= CBUFF_LEVEL) ) {
 remove_junk(0);
 write_str(user,"All temp files deleted.");
 return;
 }
else if (!strcmp(inpstr,"rooms") && (ustr[user].tempsuper >= CBUFF_LEVEL) ) {
 area=0;
 for (area=0;area<NUM_AREAS;++area) {
  for (i=0;i<NUM_LINES;++i) {
      conv[area][i][0]=0;
      }
   i=0;
  }
 write_str(user,"All room conversation buffers cleared!");
 }

else if (!strcmp(inpstr,"tells")) {
 ctellbuff(user);
 write_str(user,"Tell buffer cleared!");
 }

else if (!strcmp(inpstr,"shouts") && (ustr[user].tempsuper >= CBUFF_LEVEL) ) {
 cshbuff();
 write_str(user,"Shout buffers cleared!");
 }

else if (!strcmp(inpstr,"wiz") && (ustr[user].tempsuper >= CBUFF_LEVEL) ) {
 cbtbuff();
 write_str(user,"Wiztell buffers cleared!");
 }

else if (!strcmp(inpstr,"all") && (ustr[user].tempsuper >= CBUFF_LEVEL) ) {
 area=0;
 for (area=0;area<NUM_AREAS;++area) {
  for (i=0;i<NUM_LINES;++i) {
      conv[area][i][0]=0;
      }
   i=0;
  }
 cshbuff();
 cbtbuff();
 write_str(user,"All buffers cleared!");
 }

else {
 if (ustr[user].tempsuper >= CBUFF_LEVEL)
  write_str(user,"That option doesn't exist.");
 else
  write_str(user,NOT_WORTHY);
 return;
 }

}

/** Clear the conversation buffer in the user's room  NON-COMMAND **/
void cbuff(int user)
{
int i,area=ustr[user].area;

 for (i=0;i<NUM_LINES;++i) conv[area][i][0]=0;
 write_str(user,"Conversation buffer cleared!");

}


/** Clear the wiz conversation buffer  **/
void cbtbuff()
{
int i;

for (i=0;i<NUM_LINES;++i) 
  bt_conv[i][0]=0;
  
bt_count = 0;
  
}

/** Clear the shout conversation buffer  **/
void cshbuff()
{
int i;

for (i=0;i<NUM_LINES;++i) 
  sh_conv[i][0]=0;
  
sh_count = 0;
  
}

/** Clear the users tell conversation buffer  **/
void ctellbuff(int user)
{
int i;

for (i=0;i<NUM_LINES;++i) 
  ustr[user].conv[i][0]=0;

ustr[user].conv_count = 0;
    
}


/*** Display user macros ***/
void macros(int user, char *inpstr)
{
int m=0;
int num=0;
int found=0;
char mname[ARR_SIZE];
char chunk[3];

if (!strlen(inpstr)) {
write_str(user,"Your current macros:");
for (m=0;m<NUM_MACROS;++m) {
   if (strlen(ustr[user].Macros[m].body)) {
    sprintf(mess,"%-11s ^=^ %s",ustr[user].Macros[m].name,ustr[user].Macros[m].body);
    write_str(user,mess);
    num++;
   }
  }
if (num) {
  sprintf(mess,"You have ^HG%d^ of ^HG%d^ defined",num,NUM_MACROS);
  write_str(user,mess);
  return;
  }
else {
  sprintf(mess,"You have no macros defined. You can define up to ^HG%d^",NUM_MACROS);
  write_str(user,mess);
  return;
  }
 } /* end of if not strlen */

sscanf(inpstr,"%s ",mname);
if (!strcmp(mname,"-c") || !strcmp(mname,"clear")) {
   for (m=0;m<NUM_MACROS;++m) {
     ustr[user].Macros[m].name[0]=0;
     ustr[user].Macros[m].body[0]=0;
    }
   copy_from_user(user);
   write_user(ustr[user].name);
   write_str(user,"Macros cleared.");
   return;
  } /* end of if */
else if (!strcmp(mname,"-d") || !strcmp(mname,"del")) {
   remove_first(inpstr);
   if (!strlen(inpstr)) {
     write_str(user,"You must specify a macro to delete.");
     return;
     }
   for (m=0;m<NUM_MACROS;++m) {
      if (!strcmp(ustr[user].Macros[m].name,inpstr)) {
        ustr[user].Macros[m].name[0]=0;
        ustr[user].Macros[m].body[0]=0;
        copy_from_user(user);
        write_user(ustr[user].name);
        write_str(user,"Macro deleted.");
        return;
        }
    }  /* end of for */
   write_str(user,"No such macro defined.");
   return;
  } /* end of if */
else {
  if (strlen(mname)>11) {
    write_str(user,MACRO_NLONG);
    return;
    }
   /* Check to see if we alreayd have a macro named this */
   for (m=0;m<NUM_MACROS;++m) {
      if (!strcmp(ustr[user].Macros[m].name,mname)) {
        write_str(user,"You have a macro named that! Delete it first if you wish to redefine it.");
        return;
        }
    }  /* end of for */
  remove_first(inpstr);
  if (strlen(inpstr) > MACRO_LEN) {
    write_str(user,MACRO_LONG);
    return;
    }
  for (m=0;m<NUM_MACROS;++m) {
    if (!strlen(ustr[user].Macros[m].body)) {
      num=m;
      found=1;
      break;
      }
    } /* end of for */
  if (!found) {
    sprintf(mess,"You have all ^HG%d^ macros defined. Delete one/some first.",NUM_MACROS);
    write_str(user,mess);
    return;
   }

  /* Do nerf macro check */
  midcpy(inpstr,chunk,0,1);
  if (!NERF_MACRO) {
    if (!strcmp(chunk,".n")) {
     midcpy(inpstr,chunk,1,2);
     if (!strcmp(chunk,"ne")) {
       write_str(user,CANT_MACRO);
       inpstr[0]=0;
       return;
       }
     midcpy(inpstr,chunk,2,2);
     if (!strcmp(chunk," ")) {
       write_str(user,CANT_MACRO);
       inpstr[0]=0;
       return;
      }
     }
    }       

  strcpy(ustr[user].Macros[num].name, strip_color(mname));
  strcpy(ustr[user].Macros[num].body, inpstr);
  copy_from_user(user);
  write_user(ustr[user].name);
  write_str(user,"Macro set.");
 } /* end of main else */
}


/*** Read Mail ***/
void read_mail(int user, char *inpstr)
{
int b=0;
int a=0;
int lines=0;
int num_lines=0;
int buf_lines=0;
long filesize = 0;
char junk[1001];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
struct stat fileinfo;
FILE *fp;
FILE *tfp;

if (strlen(inpstr)) {
     if (!strcmp(inpstr,"-s")) {
      sprintf(t_mess,"%s/%s",MAILDIR,ustr[user].name);
      strncpy(filename,t_mess,FILE_NAME_LEN);

      /* Get filename size */
      if (stat(filename, &fileinfo) == -1) {
       if (check_for_file(filename)) {
       sprintf(mess,"SYSTEM: Could not read mailfile size for user %s\n",ustr[user].say_name);
       print_to_syslog(mess);
        }
       }
      else filesize = fileinfo.st_size;

      sprintf(mess,"Your inbox is using %ld out of the max %d bytes.",filesize,MAX_MAILSIZE);
      write_str(user,mess);

     filesize = 0;

      sprintf(t_mess,"%s/%s.sent",MAILDIR,ustr[user].name);
      strncpy(filename,t_mess,FILE_NAME_LEN);

      /* Get filename size */
      if (stat(filename, &fileinfo) == -1) {
       if (check_for_file(filename)) {
       sprintf(mess,"SYSTEM: Could not read mailfile size for user %s\n",ustr[user].say_name);
       print_to_syslog(mess);
       }
      }
      else filesize = fileinfo.st_size;

      sprintf(mess,"Your sent mailbox is using %ld out of the max %d bytes.",filesize,MAX_SMAILSIZE);
      write_str(user,mess);
      filesize = 0;
      return;
     }

     if ((strlen(inpstr) < 3) && (strlen(inpstr) > 0) && 
        (!isalpha((int)inpstr[0]))) 
         {
         num_lines=atoi(inpstr);
         buf_lines=num_lines;
         a=1;
         }
     else {
         write_str(user,"Number invalid.");
         return;
         }
    }

/* Send output to user */
sprintf(t_mess,"%s/%s",MAILDIR,ustr[user].name);
strncpy(filename,t_mess,FILE_NAME_LEN);

sprintf(mess,"\n^HR** Your Private Mail Console **^");
write_str(user,mess);

if (a==1) {
   if (num_lines < 1) {
       if (!cat(filename,user,1))
           {
             write_str(user,"You don't have any mail waiting");
             if (ustr[user].new_mail) {
                 write_str(user,"^ ** You should have new mail but your mailfile was deleted. See the admin. **^");
                 }
            }
        goto DONE;
       }
    lines = file_count_lines(filename);
    if (num_lines <= lines) {
       ustr[user].numbering = (lines - num_lines) +1;
       }
     else {
          num_lines=lines;
          }
    num_lines = lines - num_lines;

    if (!(fp=fopen(filename,"r"))) {
        write_str(user,"You don't have any mail waiting");
        num_lines=0;  a=0;  lines=0;
        ustr[user].numbering = 0;
        return;
        }
strcpy(filename2,get_temp_file());
tfp=fopen(filename2,"w");

while (!feof(fp)) {
         fgets(junk,1000,fp);
         b++;
         if (b <= num_lines)  {
             junk[0]=0;
             continue;
            }
          else {
             fputs(junk,tfp);
             junk[0]=0;
             }
       }
FCLOSE(fp);
FCLOSE(tfp);
num_lines=0;  lines=0;
if (!cat(filename2,user,1))
    write_str(user,"You don't have any mail waiting");

DONE:
b=0;
}

if (a==0) {
ustr[user].numbering = 0;
if (!cat(filename,user,1))
  {
   write_str(user,"You don't have any mail waiting");
   if (ustr[user].new_mail)
     {
       write_str(user,"");
       write_str(user,"^ ** You should have new mail but your mailfile was deleted. See the admin. **^");
     }
  }
}

if (a==1) {
   if ((buf_lines >= ustr[user].mail_num) || (buf_lines < 1))
    ustr[user].new_mail = FALSE;
   
   if ((buf_lines >=1) && (buf_lines <= ustr[user].mail_num))
    ustr[user].mail_num-=buf_lines;
   else
    ustr[user].mail_num=0;
 }
else {
ustr[user].new_mail = FALSE;
ustr[user].mail_num = 0;
}

ustr[user].numbering= 0;

copy_from_user(user);
write_user(ustr[user].name);

}

 
/*-----------------------------------------------------------*/
/* Send mail routing                                         */
/*-----------------------------------------------------------*/
void send_mail(int user, char *inpstr, int mode)
{
int u=-1,ret=0;
int newread=0,sendmail=0,nosubject=0;
long filesize=0;
char stm[20],mess2[ARR_SIZE+25],filename[FILE_NAME_LEN],name[NAME_LEN];
char other_user[ARR_SIZE];
char message[ARR_SIZE];
struct stat fileinfo;
time_t tm;
FILE *fp;
FILE *pp;

/* Check if user is gagcommed */
if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

/*-------------------------------------------------------*/
/* check for any input                                   */
/*-------------------------------------------------------*/

if (!strlen(inpstr)) 
  {
   write_str(user,"Who do you want to mail?"); 
   return;
  }

/*-------------------------------------------------------*/
/* get the other user name                               */
/*-------------------------------------------------------*/

sscanf(inpstr,"%s ",other_user);
other_user[NAME_LEN+1]=0;
CHECK_NAME(other_user);
strtolower(other_user);
remove_first(inpstr);

/*-------------------------------------------------------*/
/* check to see if a message was supplied                */
/*-------------------------------------------------------*/

if (!strlen(inpstr)) 
  {
   write_str(user,"You have not specified a message"); 
   return;
  }

if (ustr[user].frog) {
  if (mode==0)
   write_str(user,"Frogs cant write, silly.");
   return;
   }

if (!check_for_user(other_user)) 
  {
   write_str(user,NO_USER_STR);
   return;
  }

ret=gag_check2(user,other_user);
if (!ret) return;
else if (ret==2) {
 sprintf(mess,"SYSTEM: An error has occured reading %s's user file",other_user);
 write_str(user,mess);
 print_to_syslog(mess);
 print_to_syslog("\n");
 return;
 }

if (!read_user(other_user)) {
 sprintf(mess,"SYSTEM: An error has occured reading %s's user file",other_user);
 write_str(user,mess);
 print_to_syslog(mess);
 print_to_syslog("\n");
 return;
 }


/*--------------------------------------------------*/
/* prepare message to be sent                       */
/*--------------------------------------------------*/
time(&tm);
midcpy(ctime(&tm),stm,4,15);
strcpy(name,ustr[user].say_name);


strcpy(message,inpstr);
sprintf(mess,"(%s) From %s: %s\n",stm,name,message);

sprintf(t_mess,"%s/%s",MAILDIR,other_user);
strncpy(filename,t_mess,FILE_NAME_LEN);

/* Get filename size */
if (stat(filename, &fileinfo) == -1) {
    if (check_for_file(filename)) {
    sprintf(mess2,"SYSTEM: Could not read mailfile size for user %s\n",other_user);
    print_to_syslog(mess2);
    }
   }
else filesize = fileinfo.st_size;

/* THIS IS TO PREVENT EXCESSIVE MAIL SPAMMING */
/* If recepients mailsize is at or over size limit, tell sender the */
/* mail send failed, mail recepient from the talker that mailfile   */
/* is over limit, and notify user, if online.                       */
if (filesize >= MAX_MAILSIZE) {
   if (mode==0)
   write_str(user,"Recipient's mailfile is at or over the size limit. Send failed.");

 if (t_ustr.mail_warn == 0) {
   sprintf(t_mess,MAILFILE_NOTIFY,filesize-MAX_MAILSIZE);
   sprintf(mess,"(%s) From THE TALKER: %s\n",stm,t_mess);
   t_mess[0]=0;
   if (!(fp=fopen(filename,"a")))
     {
	if (mode==0) {
         sprintf(mess,"%s : message cannot be written\n", syserror);
         write_str(user,mess);
	}
      return;
     }
   fputs(mess,fp);
   FCLOSE(fp);
   t_ustr.mail_warn = 1;
   t_ustr.new_mail = TRUE;
   t_ustr.mail_num++;

/*-------------------------------------------------------*/
/* write users to inform them of transaction             */
/*-------------------------------------------------------*/

if ((u=get_user_num(other_user,user))!= -1) 
  {
   strcpy(t_mess,MAILFROM_TALKER);
   write_str(u,t_mess);
   ustr[u].new_mail = TRUE;
   ustr[u].mail_warn = 1;
   if (ustr[u].autor > 1) {
      if (ustr[u].mail_num > 0) ustr[u].new_mail = TRUE;
      else {
       ustr[u].new_mail = FALSE;
       t_ustr.new_mail = FALSE;
       }
     t_ustr.mail_num--;
     newread=1;
   } /* end if autor */
   else ustr[u].mail_num++;

   if ((ustr[u].autof==1) && (ustr[u].automsgs < MAX_AUTOFORS)) {
       if ((strlen(ustr[u].email_addr) < 8) ||
           !strcmp(ustr[u].email_addr,DEF_EMAIL)) { 
           write_str(u,"Your set email address is not a valid address..aborting autofwd.");
           copy_from_user(u);
           write_user(ustr[u].name);
           return;
           }

/*---------------------------------------------------*/
/* write email message                               */
/*---------------------------------------------------*/

if (strstr(MAILPROG,"sendmail")) {
  sprintf(t_mess,"%s",MAILPROG);
  sendmail=1;
  }
else {
  sprintf(t_mess,"%s %s",MAILPROG,ustr[u].email_addr);
  if (strstr(MAILPROG,"-s"))
	nosubject=0;
  else
	nosubject=1;
  }  
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(pp=popen(filename,"w"))) 
  {
	if (mode==0) {
   	sprintf(mess,"%s : autofwd message cannot be written\n", syserror);
   	write_str(user,mess);
	}
   return;
  }

if (sendmail) {
fprintf(pp,"From: %s <%s>\n",SYSTEM_NAME,SYSTEM_EMAIL);
fprintf(pp,"To: %s <%s>\n",ustr[u].say_name,ustr[u].email_addr);
fprintf(pp,"Subject: Mailfile error on %s\n\n",SYSTEM_NAME);
}
else if (nosubject) {
nosubject=0;
fprintf(pp,"Mailfile error on %s\n",SYSTEM_NAME);
}

strcpy(mess, strip_color(mess));
fputs(mess,pp);
fputs(EXT_MAIL1,pp);
sprintf(mess,EXT_MAIL2,SYSTEM_NAME);
fputs(mess,pp);

fputs(EXT_MAIL4,pp);
fputs(EXT_MAIL5,pp);
   fputs(EXT_MAIL8,pp);

fputs(EXT_MAIL7,pp);
fputs(".\n",pp);

pclose(pp);

/*
      sprintf(mess,"%s -s \'Mailfile error on %s\' %s < %s/%s.email 2> /dev/null",MAILPROG,SYSTEM_NAME,ustr[u].email_addr,MAILDIR,ustr[u].say_name); 
      system(mess);
      remove(filename);
*/
      write_str(u,MAIL_AUTOFWD);
      ustr[u].automsgs++;
     } /* end of if autof */
  } /* end of if user online if */
 else if ((t_ustr.autof > 0) && (t_ustr.automsgs < MAX_AUTOFORS)) {
     if ((strlen(t_ustr.email_addr) < 8) ||
         !strcmp(t_ustr.email_addr,DEF_EMAIL)) { write_user(t_ustr.name); return; }

/*---------------------------------------------------*/
/* write email message                               */
/*---------------------------------------------------*/

if (strstr(MAILPROG,"sendmail")) {
  sprintf(t_mess,"%s",MAILPROG);
  sendmail=1;
  }
else {
  sprintf(t_mess,"%s %s",MAILPROG,t_ustr.email_addr);
  if (strstr(MAILPROG,"-s"))
	nosubject=0;
  else
	nosubject=1;
  }  
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(pp=popen(filename,"w"))) 
  {
	if (mode==0) {
 	sprintf(mess,"%s : autofwd message cannot be written\n", syserror);
   	write_str(user,mess);
	}
   return;
  }

if (sendmail) {
fprintf(pp,"From: %s <%s>\n",SYSTEM_NAME,SYSTEM_EMAIL);
fprintf(pp,"To: %s <%s>\n",t_ustr.say_name,t_ustr.email_addr);
fprintf(pp,"Subject: Mailfile error on %s\n\n",SYSTEM_NAME);
}
else if (nosubject) {
nosubject=0;
fprintf(pp,"Mailfile error on %s\n",SYSTEM_NAME);
}

strcpy(mess, strip_color(mess));
fputs(mess,pp);
fputs(EXT_MAIL1,pp);
sprintf(mess,EXT_MAIL2,SYSTEM_NAME);
fputs(mess,pp);

fputs(EXT_MAIL4,pp);
fputs(EXT_MAIL5,pp);
   fputs(EXT_MAIL8,pp);

fputs(EXT_MAIL7,pp);
fputs(".\n",pp);

pclose(pp);

/*
   sprintf(mess,"%s -s \'Mailfile error on %s\' %s < %s/%s.email 2> /dev/null",MAILPROG,SYSTEM_NAME,t_ustr.email_addr,MAILDIR,t_ustr.say_name); 
   system(mess);
   remove(filename);
*/
   t_ustr.automsgs++;
   } /* end of if autof */
 } /* end of if mail warn is 0 */
 else { }

 write_user(other_user);
/* If recepient is online and has autoread in dual mode, */
/* read their new message                                */

if (u && newread) read_mail(u,"1");

  return;
 } /* end of if over filesize */

/* End of mailfile size check */

filesize = 0;

/*---------------------------------------------------*/
/* write mail message                                */
/*---------------------------------------------------*/

if (!(fp=fopen(filename,"a"))) 
  {
	if (mode==0) {
   	sprintf(mess,"%s : message cannot be written\n", syserror);
  	write_str(user,mess);
	}
   return;
  }
fputs(mess,fp);
FCLOSE(fp);

/*--------------------------------------------------*/
/* set a new mail flag for that other user          */
/*--------------------------------------------------*/

t_ustr.new_mail = TRUE;
t_ustr.mail_num++;

/*--------------------------------------------------------*/
/* write sent mail message                                */
/*--------------------------------------------------------*/

sprintf(mess2,"(%s) To %s: %s\n",stm,other_user,message);

sprintf(t_mess,"%s/%s.sent",MAILDIR,ustr[user].name);
strncpy(filename,t_mess,FILE_NAME_LEN);

/* Get filename size */
if (stat(filename, &fileinfo) == -1) {
    if (check_for_file(filename)) {
    sprintf(t_mess,"SYSTEM: Could not read mailfile size for user %s\n",ustr[user].say_name);
    print_to_syslog(t_mess);
    }
   }
else filesize = fileinfo.st_size;

if (filesize >= MAX_SMAILSIZE) {
	if (mode==0) {
    	sprintf(t_mess,MAILFILE2_NOTIFY,filesize-MAX_SMAILSIZE);
    	write_str(user,t_mess);
	}
    t_mess[0]=0;
    }
else {
if (!(fp=fopen(filename,"a"))) 
  {
	if (mode==0) {
  	sprintf(mess2,"%s : sent mail message cannot be written\n", syserror);
   	write_str(user,mess2);
	}
   goto NEXT;
  }
fputs(mess2,fp);
FCLOSE(fp);
}

NEXT:
/* End of mailfile size check */

filesize = 0;


/*-------------------------------------------------------*/
/* write users to inform them of transaction             */
/*-------------------------------------------------------*/

if (mode==0) {
  sprintf(t_mess,MAIL_TO,other_user);
  write_str(user,t_mess);
  }
if ((u=get_user_num_exact(other_user,user))!= -1) 
  {
   sprintf(t_mess,MAILFROM_USER,ustr[user].say_name);
   write_str(u,t_mess);
   ustr[u].new_mail = TRUE;
   if (ustr[u].autor > 1) {
      if (ustr[u].mail_num > 0) ustr[u].new_mail = TRUE;
      else {
       ustr[u].new_mail = FALSE;
       t_ustr.new_mail = FALSE;
       }
     t_ustr.mail_num--;
     newread=1;
   }
   else ustr[u].mail_num++;

   if ((ustr[u].autof==1) && (ustr[u].automsgs < MAX_AUTOFORS)) {
       if ((strlen(ustr[u].email_addr) < 8) ||
           !strcmp(ustr[u].email_addr,DEF_EMAIL)) { 
           write_str(u,"Your set email address is not a valid address..aborting autofwd.");
           copy_from_user(u);
           write_user(ustr[u].name);
           return;
           }
/*---------------------------------------------------*/
/* write email message                               */
/*---------------------------------------------------*/

if (strstr(MAILPROG,"sendmail")) {
  sprintf(t_mess,"%s",MAILPROG);
  sendmail=1;
  }
else {
  sprintf(t_mess,"%s %s",MAILPROG,ustr[u].email_addr);
  if (strstr(MAILPROG,"-s"))
	nosubject=0;
  else
	nosubject=1;
  }  
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(pp=popen(filename,"w"))) 
  {
	if (mode==0) {
   	sprintf(mess,"%s : autofwd message cannot be written\n", syserror);
   	write_str(user,mess);
	}
   goto NEXT3;
  }

if (sendmail) {
if (!ustr[user].semail && strstr(ustr[user].email_addr,"@"))
fprintf(pp,"From: %s on %s <%s>\n",ustr[user].say_name,SYSTEM_NAME,ustr[user].email_addr);
else
fprintf(pp,"From: %s <%s>\n",SYSTEM_NAME,SYSTEM_EMAIL);

fprintf(pp,"To: %s <%s>\n",ustr[u].say_name,ustr[u].email_addr);
fprintf(pp,"Subject: New smail from %s on %s\n\n",ustr[user].say_name,SYSTEM_NAME);
}
else if (nosubject) {
nosubject=0;
fprintf(pp,"New smail from %s on %s\n",ustr[user].say_name,SYSTEM_NAME);
}

strcpy(mess, strip_color(mess));
fputs(mess,pp);
fputs(EXT_MAIL1,pp);
sprintf(mess,EXT_MAIL2,SYSTEM_NAME);
fputs(mess,pp);
if (ustr[user].semail)
 fputs(EXT_MAIL3,pp);

fputs(EXT_MAIL4,pp);
fputs(EXT_MAIL5,pp);
if (sendmail) {
if (!ustr[user].semail && strstr(ustr[user].email_addr,"@")) {
   fputs(EXT_MAIL8,pp);
   }
else {
   fputs(EXT_MAIL9,pp);
   }
}
else {
if (!ustr[user].semail && strstr(ustr[user].email_addr,"@")) {
   sprintf(mess,EXT_MAIL10,ustr[user].email_addr);
   fputs(mess,pp);
   }
else {
   fputs(EXT_MAIL9,pp);
   }
}

fputs(EXT_MAIL7,pp);
fputs(".\n",pp);

pclose(pp);

/*
      sprintf(mess,"%s %s < %s/%s.email 2> /dev/null",MAILPROG,ustr[u].email_addr,MAILDIR,ustr[u].name); 
      system(mess);
      remove(filename);
*/
      write_str(u,MAIL_AUTOFWD);
      ustr[u].automsgs++;
     }
  } /* end of if user online if */
 else if ((t_ustr.autof > 0) && (t_ustr.automsgs < MAX_AUTOFORS)) {
     if ((strlen(t_ustr.email_addr) < 8) ||
         !strcmp(t_ustr.email_addr,DEF_EMAIL)) { write_user(t_ustr.name); return; }

/*---------------------------------------------------*/
/* write email message                               */
/*---------------------------------------------------*/

if (strstr(MAILPROG,"sendmail")) {
  sprintf(t_mess,"%s",MAILPROG);
  sendmail=1;
  }
else {
  sprintf(t_mess,"%s %s",MAILPROG,t_ustr.email_addr);
  if (strstr(MAILPROG,"-s"))
	nosubject=0;
  else
	nosubject=1;
  }  
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(pp=popen(filename,"w"))) 
  {
	if (mode==0) {
   	sprintf(mess,"%s : autofwd message cannot be written\n", syserror);
   	write_str(user,mess);
	}
   goto NEXT3;
  }

if (sendmail) {
if (!ustr[user].semail && strstr(ustr[user].email_addr,"@"))
fprintf(pp,"From: %s on %s <%s>\n",ustr[user].say_name,SYSTEM_NAME,ustr[user].email_addr);
else
fprintf(pp,"From: %s <%s>\n",SYSTEM_NAME,SYSTEM_EMAIL);

fprintf(pp,"To: %s <%s>\n",t_ustr.say_name,t_ustr.email_addr);
fprintf(pp,"Subject: New smail from %s on %s\n\n",ustr[user].say_name,SYSTEM_NAME);
}
else if (nosubject) {
nosubject=0;
fprintf(pp,"New smail from %s on %s\n",ustr[user].say_name,SYSTEM_NAME);
}

strcpy(mess, strip_color(mess));
fputs(mess,pp);
fputs(EXT_MAIL1,pp);
sprintf(mess,EXT_MAIL2,SYSTEM_NAME);
fputs(mess,pp);
if (ustr[user].semail)
 fputs(EXT_MAIL3,pp);

fputs(EXT_MAIL4,pp);
fputs(EXT_MAIL5,pp);
if (sendmail) {
if (!ustr[user].semail && strstr(ustr[user].email_addr,"@")) {
   fputs(EXT_MAIL8,pp);
   }
else {
   fputs(EXT_MAIL9,pp);
   }
}
else {
if (!ustr[user].semail && strstr(ustr[user].email_addr,"@")) {
   sprintf(mess,EXT_MAIL10,ustr[user].email_addr);
   fputs(mess,pp);
   }
else {
   fputs(EXT_MAIL9,pp);
   }
}

fputs(EXT_MAIL7,pp);
fputs(".\n",pp);

pclose(pp);

/*
   sprintf(mess,"%s %s < %s/%s.email 2> /dev/null",MAILPROG,t_ustr.email_addr,MAILDIR,t_ustr.name); 
   system(mess);
   remove(filename);
*/
   t_ustr.automsgs++;
   }

NEXT3:
write_user(other_user);

/* If recepient is online and has autoread in dual mode, */
/* read their new message                                */

if (u && newread) read_mail(u,"1");

}


/*** Clear Mail ***/
void clear_mail(int user, char *inpstr)
{
char filename[FILE_NAME_LEN];
FILE *bfp;
int lower=-1;
int upper=-1;
int mode=0;

/*---------------------------------------------*/
/* check if there is any mail                  */
/*---------------------------------------------*/

sprintf(t_mess,"%s/%s",MAILDIR,ustr[user].name);
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(bfp=fopen(filename,"r"))) 
  {
   write_str(user,"You have no mail."); 
   return;
  }
FCLOSE(bfp);

/* remove the mail file */
if (ustr[user].clrmail== -1) 
  {
   /*---------------------------------------------*/
   /* get the delete parameters                   */
   /*---------------------------------------------*/

   get_bounds_to_delete(inpstr, &lower, &upper, &mode);
 
   if (upper == -1 && lower == -1)
     {
      write_str(user,"No mail deleted.  Specification of what to ");
      write_str(user,"delete did not make sense.  Type: .help cmail ");
      write_str(user,"for detailed instructions on use. ");
      return;
     }
    
   switch(mode)
    {
     case 0: return;
             break;
        
     case 1: 
            sprintf(mess,"Cmail: Delete all mail messages? ");
            upper = -1;
            lower = -1;
            break;
        
     case 2: 
            sprintf(mess,"Cmail: Delete line %d? ", lower);
            
            break;
        
     case 3: 
            sprintf(mess,"Cmail: Delete from line %d to the end?",lower);
            break;
        
     case 4: 
            sprintf(mess,"Cmail: Delete from begining to line %d?",upper);
            break;
        
     case 5: 
            sprintf(mess,"Cmail: Delete all except lines %d to %d?",upper, lower);
            break;
        
     default: return;
              break;
    }

   ustr[user].lower = lower;
   ustr[user].upper = upper;

   ustr[user].clrmail=user; 
   noprompt=1;
   write_str(user,mess);
   write_str_nr(user,"Do you wish to do this? (y/n) ");
	 telnet_write_eor(user);
   return;
  }
  
remove_lines_from_file(user, 
                       filename, 
                       ustr[user].lower, 
                       ustr[user].upper);

sprintf(mess,"You deleted specified mail messages.");
write_str(user,mess);
ustr[user].mail_warn = 0;

if (!file_count_lines(filename))  remove(filename);

}


/*** Check for new mail ***/
void check_mail(int user)
{
struct stat stbuf;
char filename[FILE_NAME_LEN], datestr[24];

sprintf(t_mess,"%s/%s",MAILDIR,ustr[user].name);
strncpy(filename,t_mess,FILE_NAME_LEN);
if (stat(filename, &stbuf) == -1)  
  {
   return;
  } 
  
if (ustr[user].new_mail)
  {
   write_str(user," ");
      sprintf(t_mess,MAIL_NEW,ustr[user].mail_num,ustr[user].mail_num == 1 ? "" : "s");
   write_str(user,t_mess);
   write_str(user," ");
  }
  
strcpy(datestr,ctime(&stbuf.st_mtime));

sprintf(mess,MAIL_ACCESS,datestr);
write_str(user,mess);

if (ustr[user].new_mail && 
    ((ustr[user].autor==1) || (ustr[user].autor==3)) ) {
   sprintf(mess,"%i",ustr[user].mail_num);
   read_mail(user,mess);
   }

}



/*------------------------------------------------------------------------*/
/* promote a user                                                         */
/*------------------------------------------------------------------------*/
void promote(int user, char *inpstr)
{
char other_user[ARR_SIZE];
int u, new_level, a=0;

if (!strlen(inpstr)) 
  {
   write_str(user,"Promote who?"); 
   return;
  }

sscanf(inpstr,"%s ",other_user);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }
 

strtolower(other_user);

if (!strcmp(ustr[user].name,other_user)) {
   write_str(user,"You cannot promote yourself!");
   return;
   }

   if (!read_user(other_user))
     {
      write_str(user,NO_USER_STR);
      return;
     }

remove_first(inpstr);  

new_level = t_ustr.super;

 if (new_level == MAX_LEVEL)
   {
    write_str(user,"That person is already the highest level in the system.");
    return;
   }

if (ustr[user].tempsuper <= new_level)
  {
    sprintf(mess,"Cant promote %s: That is beyond your authority",t_ustr.say_name);
    write_str(user,mess);
    return;
  }

sscanf(inpstr,"%d", &new_level);
  
if (new_level < 0)
  {
   new_level = 0;
  }
  
if ( ((new_level == MAX_LEVEL-1) && (t_ustr.super == MAX_LEVEL-1)) || 
    (new_level == MAX_LEVEL) )
  {
   write_str(user,"That person is being set to the highest level in the system.");
   new_level = MAX_LEVEL;
  }
  
if (new_level == t_ustr.super) new_level++;
  
if (new_level == ustr[user].tempsuper && PROMOTE_TO_SAME == FALSE)
    {
      sprintf(mess,"Cant promote %s: That is beyond your authority",t_ustr.say_name);
      write_str(user,mess);
      return;
    }

if (new_level > ustr[user].tempsuper && PROMOTE_TO_ABOVE == FALSE)
    {
      sprintf(mess,"Cant promote %s: That is beyond your authority",t_ustr.say_name);
      write_str(user,mess);
      return;
    }

/* Add new level */
t_ustr.super   = new_level;

/* Tell talker that user is promoted     */
/* so they cant auto-promote themselves */
t_ustr.promote = 1;

/* Give new wizzes access to hidden rooms */
for (a=0;a<NUM_AREAS;++a)
  {
    if (astr[a].hidden)
      {
       if (t_ustr.super >= WIZ_LEVEL) {
       t_ustr.security[a]='Y';
       if((u=get_user_num_exact(other_user,user))>-1) {
        ustr[u].security[a]='Y';
        sprintf(mess,"You have been cleared to enter room %s",astr[a].name);
        write_str(u,mess);
        } /* end of if user on */
       } /* end of if level */
      }
  }
a=0;

write_user(other_user); 

sprintf(mess,"%s: PROMOTION to level %d by %s for %s\n",
            get_time(0,0), t_ustr.super,
            ustr[user].say_name, t_ustr.say_name);
print_to_syslog(mess);

if((u=get_user_num_exact(other_user,user))>-1) 
  {
   ustr[u].super=new_level;
   sprintf(mess,PROMOTE_MESS,ustr[user].say_name,ranks[ustr[u].super]);
   write_str(u,mess);
  }
else
  {
   sprintf(mess,PROMOTE_MESS,ustr[user].say_name,ranks[t_ustr.super]);
   sprintf(t_mess,"%s %s",other_user,mess);
   send_mail(user,t_mess,1);
  }

 sprintf(mess,UPROMOTE_MESS,t_ustr.say_name,ranks[t_ustr.super]);
 write_str(user,mess);

sprintf(mess,"has PROMOTED %s to %s",other_user,ranks[t_ustr.super]);
btell(user,mess);
}

/* Check which part of auto-promotion user is in and change accordingly */
void check_promote(int user, int mode)
{
 int num=0;

 /* User is already promoted */
 if (ustr[user].promote==1) return;

 num = ustr[user].promote + mode;

 /* user is trying to use same command twice to promote themselves */
 if ((num==12) || (num==14) || (num==18)) return;

 if (ustr[user].super==0) {
  if ((num==6) || (num==7) || (num==9)) {
    ustr[user].promote=num;
    strcpy(mess,"Finished step 1 of 3 for AUTO-PROMOTION");
    write_str(user,mess);
    copy_from_user(user);
    write_user(ustr[user].name);
    return;
    }
  else if ((num==13) || (num==15) || (num==16)) {
    ustr[user].promote=num;
    strcpy(mess,"Finished step 2 of 3 for AUTO-PROMOTION");
    write_str(user,mess);
    copy_from_user(user);
    write_user(ustr[user].name);
    return;
    }
  else if (num==22) {
    ustr[user].promote=1;
    ustr[user].super++;
    copy_from_user(user);
    write_user(ustr[user].name);

    /* Inform user of their promotion */
    sprintf(mess,APROMOTE_MESS,ranks[ustr[user].super]);
    write_str(user,mess);

    /* Inform wizards of their promotion */
    sprintf(mess,"%s %s has been AUTO-PROMOTED",STAFF_PREFIX,ustr[user].say_name);
    writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);

    /* Inform system log */
    sprintf(mess,"%s: AUTO-PROMOTION by THE TALKER for %s\n",get_time(0,0),ustr[user].say_name);
    print_to_syslog(mess);
    return;
    }
 } /* end of if super */
}

/* Demote a user */
void demote(int user, char *inpstr)
{
char other_user[ARR_SIZE];
int u, a=0;
char z_mess[132];
 
if (!strlen(inpstr)) 
  {
   write_str(user,"Demote who?"); 
   return;
  }

sscanf(inpstr,"%s ",other_user);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }
 
strtolower(other_user);

if (!read_user(other_user))
  {
   write_str(user,NO_USER_STR);
   return;
  }

if (t_ustr.super == 0)
  {
    sprintf(z_mess,"Cant demote %s: Bottom ranked already",t_ustr.say_name);
    write_str(user,z_mess);
    return;
  }

if (!DEMOTE_SAME) {  
if ((t_ustr.super >= ustr[user].tempsuper) &&
    (strcmp(ustr[user].name,ROOT_ID)) )
  {
    sprintf(z_mess,"Cant demote %s, they hold rank over you or are same rank.",t_ustr.say_name);
    write_str(user,z_mess);
    return;
  }
 }
else {
if ((t_ustr.super > ustr[user].tempsuper) &&
    (strcmp(ustr[user].name,ROOT_ID)) )
  {
    sprintf(z_mess,"Cant demote %s, they hold rank over you or are same rank.",t_ustr.say_name);
    write_str(user,z_mess);
    return;
  }
 }

t_ustr.super--;
  /* if user being demoted below the monitor privledge level, take */
  /* their monitor privledges away */
if (t_ustr.super == MONITOR_LEVEL-1) t_ustr.monitor=0;

/* If were a wiz level or higher and not now take away */
/* hidden room privledges                              */
for (a=0;a<NUM_AREAS;++a)
  {
    if (astr[a].hidden)
      {
       if (t_ustr.super < WIZ_LEVEL) {
       t_ustr.security[a]='N';
       if((u=get_user_num_exact(other_user,user))>-1) {
        ustr[u].security[a]='N';
        sprintf(mess,"You have been locked from room %s",astr[a].name);
        write_str(u,mess);
        } /* end of if user on */
       } /* end of if level */
      }
  }
a=0;

write_user(other_user);
sprintf(z_mess,"%s: DEMOTION to level %d by %s for %s\n",
              get_time(0,0), t_ustr.super,
              ustr[user].say_name, t_ustr.say_name);
print_to_syslog(z_mess);

if ((u=get_user_num_exact(other_user,user))>-1) 
  {
    ustr[u].super--;
    if (ustr[u].super == MONITOR_LEVEL-1) ustr[u].monitor=0;
    sprintf(z_mess,DEMOTE_MESS,ustr[user].say_name,ranks[ustr[u].super]);
    write_str(u,z_mess);
   }
else
  {
   sprintf(z_mess,DEMOTE_MESS,ustr[user].say_name,ranks[t_ustr.super]);
   sprintf(t_mess,"%s %s",other_user,z_mess);
   send_mail(user,t_mess,1);
  }

sprintf(z_mess,UDEMOTE_MESS,t_ustr.say_name,ranks[t_ustr.super]);
write_str(user,z_mess);

sprintf(z_mess,"has DEMOTED %s to %s",other_user,ranks[t_ustr.super]);
btell(user,z_mess);
} 


/** Muzzle a user, takes away his .shout capability **/
void muzzle(int user, char *inpstr, int type)
{
char buf1[256];
char other_user[ARR_SIZE];
int u,inlen;
unsigned int i;

if (!strlen(inpstr)) 
  {
   write_str(user,"Users Muzzled & logged on     Time left"); 
   write_str(user,"-------------------------     ---------"); 
   for (u=0; u<MAX_USERS; ++u) 
    {
     if (ustr[u].shout == 0 && ustr[u].area > -1) 
       {
        if (ustr[u].muz_time == 0)
           sprintf(mess,"%-29s %s",ustr[u].say_name,"Perm");
        else
           sprintf(mess,"%-29s %s",ustr[u].say_name,converttime((long)ustr[u].muz_time));
        write_str(user, mess);
       }
    }
   write_str(user,"(end of list)");
   return;
  }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   not_signed_on(user,other_user);
   return;
  }
if (u == user)
  {   
   write_str(user,"You are definitly wierd! Trying to muzzle yourself, geesh."); 
   return;
  }

if (!ustr[u].shout) {
   write_str(user,"They are already muzzled!");
   return;
   }

 if ((!strcmp(ustr[u].name,ROOT_ID)) || (!strcmp(ustr[u].name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    return;
    }
    
if (ustr[user].tempsuper <= ustr[u].super) 
  {
   write_str(user,"That would not be wise...");
   sprintf(mess,MUZZLE_CANT,ustr[user].say_name);
   write_str(u,mess);
   return;
  }

if (type==1)
  goto ARREST;
else
  remove_first(inpstr);

if (strlen(inpstr) && strcmp(inpstr,"0")) {
   if (strlen(inpstr) > 5) {
      write_str(user,"Minutes cant exceed 5 digits.");
      return;
      }
   inlen=strlen(inpstr);
   for (i=0;i<inlen;++i) {
     if (!isdigit((int)inpstr[i])) {
        write_str(user,"Numbers only!");
        return;
        }
     }
    i=0;
    i=atoi(inpstr);
    if ( i > 32767) {
       write_str(user,"Minutes cant exceed 32767.");
       i=0;
       return;
      }
  i=0;
  ustr[u].muz_time=atoi(inpstr);
}
else { ustr[u].muz_time=0; }

ARREST:
ustr[u].shout=0;

sprintf(mess,"%s cant shout anymore",ustr[u].say_name);
writeall_str(mess, 1, u, 1, user, NORM, NONE, 0);
write_str(u,MUZZLEON_MESS);

if (ustr[u].muz_time > 0) {
 sprintf(mess,"MUZZLE: %s by %s for %s\n",ustr[u].say_name,
         ustr[user].say_name, converttime((long)ustr[u].muz_time));
 }
else {
 sprintf(mess,"MUZZLE: %s by %s\n",ustr[u].say_name, ustr[user].say_name);
 }
btell(user, mess);

 strcpy(buf1,get_time(0,0));    
 strcat(buf1," ");
 strcat(buf1,mess);
print_to_syslog(buf1);

}


/** Unmuzzle a muzzled user, so they can shout again **/
void unmuzzle(int user, char *inpstr)
{
char buf1[256];
char other_user[ARR_SIZE];
int u;
 
if (!strlen(inpstr)) 
  {
   write_str(user,"Users Currently Unmuzzled and logged on"); 
   write_str(user,"---------------------------------------"); 
   for (u=0;u<MAX_USERS;++u) 
    {
     if (ustr[u].shout && ustr[u].area > -1) 
       {
        write_str(user,ustr[u].say_name);
       };
    }
   write_str(user,"(end of list)");
   return;
  }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);
 
if ((u=get_user_num(other_user,user))== -1) 
  {
   not_signed_on(user,other_user);
   return;
  }
 
if (ustr[user].tempsuper <= ustr[u].super) 
  {
   write_str(user,"Why do you want to do that?");
   return;
  }

if (ustr[u].shout)
  {
   sprintf(mess,"%s is not muzzled",ustr[u].say_name);
   write_str(user,mess);
   return;
  }
 
if (u == user && ustr[u].super < SENIOR_LEVEL)
  {
   write_str(user,"Silly user, think it would be that simple.");
   return;
  }
 
ustr[u].shout=1;
ustr[u].muz_time=0;
sprintf(mess,"%s can shout again",ustr[u].say_name);
writeall_str(mess, 1, u, 1, user, NORM, NONE, 0);
write_str(u,MUZZLEOFF_MESS);

sprintf(mess,"UNMUZZLE: %s by %s\n",ustr[u].say_name, ustr[user].say_name);
btell(user, mess);

 strcpy(buf1,get_time(0,0));    
 strcat(buf1," ");
 strcat(buf1,mess);
print_to_syslog(buf1);

}


/*** Bring a user to you ***/
void bring(int user, char *inpstr)
{
int point=0,count=0,i=0,lastspace=0,lastcomma=0,gotchar=0;
int point2=0,multi=0;
int multilistnums[MAX_MULTIS];
char multilist[MAX_MULTIS][ARR_SIZE];
char multiliststr[ARR_SIZE];
int user2=-1,area=ustr[user].area;
char other_user[ARR_SIZE];
char tempstr[50];

for (i=0;i<MAX_MULTIS;++i) { multilist[i][0]=0; multilistnums[i]=-1; }
multiliststr[0]=0;
i=0;

if (!strlen(inpstr)) {
   write_str(user,"Bring who?");
   return;
   }

if (!strcmp(astr[area].name,HEAD_ROOM)) {
   write_str(user,"Noone can be .brung into this room  Hehe.");
   return;
   }

sscanf(inpstr,"%s ",other_user);
if (!strcmp(other_user,"-f")) {
        other_user[0]=0; 
        for (i=0;i<MAX_ALERT;++i) {
         if (strlen(ustr[user].friends[i])) {
          strcpy(multilist[count],ustr[user].friends[i]);
          count++;
	  if (count==MAX_MULTIS) break;
          }
        }
        if (!count) {
                write_str(user,"You dont have any friends!");
                return;
        }
        i=0;
        remove_first(inpstr);  
  }
else {
other_user[0]=0;
          
for (i=0;i<strlen(inpstr);++i) {
        if (inpstr[i]==' ') {
                if (lastspace && !gotchar) { point++; point2++; continue; }
                if (!gotchar) { point++; point2++; }
                lastspace=1;
                continue;
          } /* end of if space */
        else if (inpstr[i]==',') {
                if (!gotchar) {
                        lastcomma=1;
                        point++;
                        point2++;
                        continue;
                }
                else {
                if (count <= MAX_MULTIS-1) {
                midcpy(inpstr,multilist[count],point,point2-1);
                count++;
                }
                point=i+1;
                point2=point;
                gotchar=0;
                lastcomma=1;
                continue;
                }
                        
        } /* end of if comma */
        if ((inpstr[i-1]==' ') && (gotchar)) {
                if (count <= MAX_MULTIS-1) {
                midcpy(inpstr,multilist[count],point,point2-1);
                count++;
                }
                break;
        }
        gotchar=1;
        lastcomma=0;
        lastspace=0;
        point2++;
} /* end of for */      
midcpy(inpstr,multiliststr,i,ARR_SIZE);
if (!strlen(multiliststr)) {
        /* no message string, copy last user */
        midcpy(inpstr,multilist[count],point,point2);
        count++; 
        strcpy(inpstr,"");
        }
else {
        strcpy(inpstr,multiliststr);
        multiliststr[0]=0;
     }
} /* end of friend else */

i=0;
point=0; 
point2=0;
gotchar=0;

if (count>1) multi=1;

/* go into loop and check users */
for (i=0;i<count;++i) {

strcpy(other_user,multilist[i]);

/* plug security hole */
if (check_fname(other_user,user))
  { 
   if (!multi) {
   write_str(user,"Illegal name.");
   return;
   }
   else continue;
  }

strtolower(other_user);


if ((user2=get_user_num(other_user,user))== -1) {
   not_signed_on(user,other_user);
   if (!multi) return;
   else continue;
   }

if (user==user2) {
   write_str(user,"You cant bring yourself!");
   if (!multi) return;
   else continue;
   }

 if ((!strcmp(ustr[user2].name,ROOT_ID)) || (!strcmp(ustr[user2].name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    if (!multi) return;
    else continue;
    }

/* Cant bring a master user */
if (ustr[user2].super > ustr[user].tempsuper) {
   write_str(user,"Hmm... inadvisable");
   sprintf(mess,"%s thought about bringing you to the %s",ustr[user].say_name,astr[ustr[user].area].name);
   write_str(user2,mess);
   if (!multi) return;
   else continue;
   }

if (area==ustr[user2].area) {
   sprintf(mess,"%s is already in this room!",ustr[user2].say_name);
   write_str(user,mess);
   if (!multi) return;
   else continue;
   }

/* check if this user is already in the list */
/* we're gonna reuse some ints here          */
for (point2=0;point2<MAX_MULTIS;++point2) {
        if (multilistnums[point2]==user2) { gotchar=1; break; }
   }
point2=0;
if (gotchar) {
  gotchar=0;
  continue;
  }

/* it's ok to send the tell to this user, add them to the multistr */
/* add this user to the list for our next loop */
multilistnums[point]=user2;
point++;
} /* end of user for */
i=0;
    
/* no multilistnums, must be all bad users */
if (!point) { 
        return;
  }

/* loop to compose the messages and print to the users */
for (i=0;i<point;++i) {

user2=multilistnums[i];

count=0;
point2=0;
multiliststr[0]=0;

/** send output **/
write_str(user2,MOVE_TOUSER);

/** to old area **/
if (!ustr[user2].vis) 
  strcpy(tempstr,INVIS_ACTION_LABEL);
else
  strcpy(tempstr,ustr[user2].say_name);

sprintf(mess,MOVE_TOREST,tempstr);
writeall_str(mess, 1, user2, 0, user, NORM, MOVE, 0);

   if (!strcmp(astr[ustr[user2].area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ left:%s", ustr[user2].say_name);
    write_bot(mess);
    }

if ((find_num_in_area(ustr[user2].area)<=PRINUM) && astr[ustr[user2].area].private)
   {
   strcpy(mess, NOW_PUBLIC);
   writeall_str(mess, 1, user2, 0, user, NORM, NONE, 0);
   astr[ustr[user2].area].private=0;
   }
ustr[user2].area=area;
look(user2,"");

   if (!strcmp(astr[ustr[user2].area].name,BOT_ROOM)) {
    sprintf(mess,"+++++ came in:%s", ustr[user2].say_name);
    write_bot(mess);
    }

} /* end of message compisition for loop */

/* make multi string to send to this user */
if (multi) {
point2=0;
multiliststr[0]=0;
for (point2=0;point2<point;++point2) {
if (point2>0)
 strcat(multiliststr,",");
/* add their name to the output string */
if (!ustr[multilistnums[point2]].vis)
 strcat(multiliststr,INVIS_ACTION_LABEL);
else
 strcat(multiliststr,ustr[multilistnums[point2]].say_name);
}
} /* end of if multi */
else strcpy(multiliststr,tempstr);

/* To new area */
sprintf(mess,MOVE_TONEW,multiliststr,multi == 1 ? "" : "s");
if (!multi)
 writeall_str(mess, 1, user2, 0, user, NORM, MOVE, 0);
else
 writeall_str(mess, 1, -1, 0, user, NORM, MOVE, 0);

write_str(user,"Ok");
}

/* This commands allows a user to hide their entry on the who list */
/* and make themselves a shadow in the current room, masking their */
/* name                                                            */
void hide(int user, char *inpstr)
{
char name[ARR_SIZE];
int victim,userlevel;
char str2[ARR_SIZE];

name[0]=0;

if (!strlen(inpstr)) 
  {
   victim=user;
   userlevel=ustr[victim].tempsuper;
  }
 else
  {
   sscanf(inpstr,"%s",name);
   strtolower(name);
   if ((victim=get_user_num(name,user))== -1) 
     {
      not_signed_on(user,name);
      return;
     }
   userlevel=ustr[victim].super;
  }
  
if (userlevel < MIN_HIDE_LEVEL)
  {
    write_str(user,"Cannot use hide on that person");
    ustr[victim].vis=1;
    return;
  }
  
if ((ustr[victim].monitor==1) || (ustr[victim].monitor==3)) 
  {
   strcpy(str2,"<");
   strcat(str2,ustr[user].say_name);
   strcat(str2,"> ");
  }
 else
  { str2[0]=0; }


if ( (userlevel >= ustr[user].tempsuper) && 
     (strcmp(ustr[user].name,ustr[victim].name)) ) 
  {
   write_str(user,"That would not be wise...");
   if (!ustr[victim].vis)
      sprintf(mess,"%s wanted to make you visible.",  ustr[user].say_name);
     else
      sprintf(mess,"%s wanted to make you invisible.",  ustr[user].say_name);

   write_str(victim, mess);
   return;
  }
  
if (!ustr[victim].vis) 
  {
   sprintf(mess,COME_VIS,ustr[victim].say_name);
   writeall_str(mess, 1, victim, 0, user, NORM, MOVE, 0);
	if (strlen(name)) write_str(user,mess);
   sprintf(mess,UCOME_VIS,str2);
   write_str(victim,mess);
   ustr[victim].vis=1;
  }
 else
  {
   sprintf(mess,GO_INVIS,ustr[victim].say_name);
   writeall_str(mess, 1, victim, 0, user, NORM, MOVE, 0);
	if (strlen(name)) write_str(user,mess);
   sprintf(mess,UGO_INVIS,str2);
   write_str(victim,mess);
   ustr[victim].vis=0;
  }

}

/* Display of list of levels. Also display the fight odds */
/* and number of commands associated with each.           */
void display_ranks(int user)
{
  char fields[30];
  char z_mess[80];
  int i=0;
  int c=0;
  int count=0;
  int numcmds=0;
  strcpy(fields,RANKS);

write_str(user,"");
sprintf(z_mess,"Your rank is ^%d^ (%s)",ustr[user].tempsuper,ranks[ustr[user].tempsuper]);
write_str(user,z_mess);  
write_str(user,"------------------------------------------------------------------");
write_str(user,"lvl  rank                  odds     cmds this level    cmds total");
write_str(user,"------------------------------------------------------------------");

for(i=0;i<MAX_LEVEL+1;i++)
  {
    for (c=0; sys[c].su_com != -1; ++c) {
     if ((sys[c].type != NONE) && (sys[c].su_com==i)) numcmds++;
     }
    count += numcmds;
    if (ustr[user].tempsuper==i)
	    sprintf(z_mess,"^HG%c    %-20.20s (%-5.5d)          %-3d              %-3d^",fields[i],ranks[i],odds[i],numcmds,count);
    else
	    sprintf(z_mess,"%c    %-20.20s (%-5.5d)          %-3d              %-3d",fields[i],ranks[i],odds[i],numcmds,count);
    write_str(user,z_mess);
    c=0;
    numcmds=0;
  }
write_str(user,"");
sprintf(z_mess,"There are %d commands in the system.",count);
write_str(user,z_mess);
write_str(user,"");
}


/*----------------------------------------------------------*/
/* no-op code for disabled commands                         */
/*----------------------------------------------------------*/
void command_disabled(int user)
{
write_str(user,"Sorry: That command is temporarily disabled");
}


int check_restriction(int user, int type)
{
int i=0;
char small_buff[128];
char mess_buf[321];
char filerid[FILE_NAME_LEN];
char filename[FILE_NAME_LEN];
struct dirent *dp;
FILE *fp;
DIR  *dirp;

if (type==ANY) { 
 sprintf(t_mess,"%s",RESTRICT_DIR);
 }
else {
 sprintf(t_mess,"%s",RESTRICT_NEW_DIR);
 }

 strncpy(filerid,t_mess,FILE_NAME_LEN);
 
 dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {
    write_str(user,"Directory information not found.");
    return 0;
   }
   
 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
    if (small_buff[0]=='.') continue;
      i=strlen(small_buff);
      if (isdigit((int)small_buff[i-1])) 
       {
        if (!strcmp(small_buff,ustr[user].site)) {
           sprintf(filename,"%s/%s.r",filerid,small_buff);
           if (!(fp=fopen(filename,"r"))) {
              write_str(user,"Cant open restrict file.");
              return 1;
              }
           mess_buf[0]=0;
           fgets(mess_buf,REASON_LEN+1,fp);
           write_str(user,mess_buf);
           write_str(user," ");
           FCLOSE(fp);
           (void) closedir(dirp); 
           return 1;
           }
        else if (check_site(ustr[user].site,small_buff,1)) {
           sprintf(filename,"%s/%s.r",filerid,small_buff);
           if (!(fp=fopen(filename,"r"))) {
              write_str(user,"Cant open restrict file.");
              return 1;
              }
           mess_buf[0]=0;
           fgets(mess_buf,REASON_LEN+1,fp);
           write_str(user,mess_buf);
           write_str(user," ");
           FCLOSE(fp);
           (void) closedir(dirp); 
           return 1;
           }
        else continue;
       }
      else if (!isdigit((int)small_buff[i-1]))
       {
        if (!strcmp(small_buff,ustr[user].net_name)) {
           sprintf(filename,"%s/%s.r",filerid,small_buff);
           if (!(fp=fopen(filename,"r"))) {
              write_str(user,"Cant open restrict file.");
              return 1;
              }
           mess_buf[0]=0;
           fgets(mess_buf,REASON_LEN+1,fp);
           write_str(user,mess_buf);
           write_str(user," ");
           FCLOSE(fp);
           (void) closedir(dirp); 
           return 1;
           }
        else if (check_site(ustr[user].net_name,small_buff,0)) {
           sprintf(filename,"%s/%s.r",filerid,small_buff);
           if (!(fp=fopen(filename,"r"))) {
              write_str(user,"Cant open restrict file.");
              return 1;
              }
           mess_buf[0]=0;
           fgets(mess_buf,REASON_LEN+1,fp);
           write_str(user,mess_buf);
           write_str(user," ");
           FCLOSE(fp);
           (void) closedir(dirp); 
           return 1;
           }
        else continue;
       }
      else { continue; }
   }       /* End of while */

 (void) closedir(dirp);
 return 0;
 
}

/* Check restrictions for who and www port */
int check_misc_restrict(int sock2, char *site, char *namesite)
{
int i=0;
char small_buff[128];
char mess_buf[321];
char filerid[FILE_NAME_LEN];
char filename[FILE_NAME_LEN];
struct dirent *dp;
FILE *fp;
DIR  *dirp;

 sprintf(t_mess,"%s",RESTRICT_DIR);
 strncpy(filerid,t_mess,FILE_NAME_LEN);
 
 dirp=opendir((char *)filerid);
  
 if (dirp == NULL)
   {
    return 0;
   }
   
 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
    if (small_buff[0]=='.') continue;
      i=strlen(small_buff);
      if (isdigit((int)small_buff[i-1])) 
       {
        if (!strcmp(small_buff,site)) {
           sprintf(filename,"%s/%s.r",filerid,small_buff);
           if (!(fp=fopen(filename,"r"))) {
              return 1;
              }
           mess_buf[0]=0;
           fgets(mess_buf,REASON_LEN+1,fp);
           write_it(sock2,mess_buf);
           write_it(sock2,"\n\r");
           FCLOSE(fp);
           (void) closedir(dirp); 
           return 1;
           }
        else if (check_site(site,small_buff,1)) {
           sprintf(filename,"%s/%s.r",filerid,small_buff);
           if (!(fp=fopen(filename,"r"))) {
              return 1;
              }
           mess_buf[0]=0;
           fgets(mess_buf,REASON_LEN+1,fp);
           write_it(sock2,mess_buf);
           write_it(sock2,"\n\r");
           FCLOSE(fp);
           (void) closedir(dirp); 
           return 1;
           }
        else continue;
       }
      else if (!isdigit((int)small_buff[i-1]))
       {
        if (!strcmp(small_buff,namesite)) {
           sprintf(filename,"%s/%s.r",filerid,small_buff);
           if (!(fp=fopen(filename,"r"))) {
              return 1;
              }
           mess_buf[0]=0;
           fgets(mess_buf,REASON_LEN+1,fp);
           write_it(sock2,mess_buf);
           write_it(sock2,"\n\r");
           FCLOSE(fp);
           (void) closedir(dirp); 
           return 1;
           }
        else if (check_site(namesite,small_buff,0)) {
           sprintf(filename,"%s/%s.r",filerid,small_buff);
           if (!(fp=fopen(filename,"r"))) {
              return 1;
              }
           mess_buf[0]=0;
           fgets(mess_buf,REASON_LEN+1,fp);
           write_it(sock2,mess_buf);
           write_it(sock2,"\n\r");
           FCLOSE(fp);
           (void) closedir(dirp); 
           return 1;
           }
        else continue;
       }
      else { continue; }
   }       /* End of while */

 (void) closedir(dirp);
 return 0;
 
}


/* check if site ends are the same (search starts from the end of string */
/* for hostnames, beginning of string for ips)				 */
int check_site(char *str1, char *str2, int mode)
{
int i,j=strlen(str1);

if (j<strlen(str2)) return 0;

if (!mode) {                        
/* hostname check */
for (i=strlen(str2)-1;i>=0;i--) if (str1[--j]!=str2[i]) return 0;
return 1;
}
else {
/* ip check */
for (i=0;i<strlen(str2);++i) if (str1[i]!=str2[i]) return 0;
return 1;
}
}


/* Function to check name to see if it is banned */
int check_nban(char *str, char *sitename)
{
int found=0;
int i=0;
char tempname[NAME_LEN+1];

     for (i=0;i<NUM_NAMEBANS;++i) {
        strcpy(tempname,nbanned[i]);
        strtolower(tempname);
        if (SUB_BANNAME) {
          if (strstr(str,tempname)) {
               found=1;
               break;
              }
          }
        else {
          if (!strcmp(str,tempname)) {
               found=1;
               break;
              }
          }
       }

if (found==1) {
 sprintf(mess,"User from site %s tried to login with banned name %s\n",
               sitename,str);
 print_to_syslog(mess);
 return 1;
}
else return 0;

}


/*----------------------------------------------------------*/
/* print out all users, or those with letter matches        */
/*----------------------------------------------------------*/
void print_users(int user, char *inpstr)
{
char filename[FILE_NAME_LEN];

sprintf(t_mess,"%s",USERDIR);
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!strcmp(inpstr," "))
   print_dir(user, filename, NULL);
  else
   print_dir(user, filename, inpstr);
return;  
}

    
    
/*----------------------------------------------------------*/
/* create a file in the restrict library for an ip site     */
/* that has been banned.                                    */
/* to make it easier, large sites can be banned by level    */
/* a b c or d.                                              */
/* where host address is a.b.c.d                            */
/* or where ip address is d.c.b.a                           */
/*----------------------------------------------------------*/
void restrict(int user, char *inpstr, int type)
{
int num;
int len=0;
int timenum;
char small_buff[64];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
char filename3[FILE_NAME_LEN];
char text_mess[35];
char timestr[23];
char timebuf[23];
char site_name[65];
char chunk[10];
char reason[ARR_SIZE];
char comment[ARR_SIZE];
time_t tm;
time_t tm_then;
struct dirent *dp;
FILE *fp;
FILE *fp2;
DIR *dirp;

if (!strcmp(inpstr,"list") || !strlen(inpstr))
  {
   if (type==ANY)
     { 
      sprintf(t_mess,"%s",RESTRICT_DIR);
     }
   else
     {
      sprintf(t_mess,"%s",RESTRICT_NEW_DIR);
     }

   strncpy(filename,t_mess,FILE_NAME_LEN);

 num=0;
 dirp=opendir((char *)filename);
  
 if (dirp == NULL)
   {write_str(user,"Directory information not found.");
    return;
   }

   time(&tm);

   write_str(user,"Site/Cluster/Domain                 Ban Started"); 
   write_str(user,"-------------------------           -----------"); 

   strcpy(filename3,get_temp_file());
   if (!(fp2=fopen(filename3,"w"))) {
     sprintf(mess,"%s Cant create file for paged restrict listing!",ctime(&tm));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     (void) closedir(dirp);
     return;
     }
   
 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
    len=strlen(small_buff);
       if ((small_buff[0]=='.') || 
           ( (small_buff[len-2]=='.') && 
             ((small_buff[len-1]=='c') ||
              (small_buff[len-1]=='r')) ) ) {
         small_buff[0]=0;
         len=0;
         continue;
        }
      else
        {
         sprintf(filename2,"%s/%s",filename,small_buff);
         if (!(fp=fopen(filename2,"r"))) {
            write_str(user,"Cant open file for reading!");
            continue;
            }
         fgets(timebuf,13,fp);
         FCLOSE(fp);
         timenum=atoi(timebuf);
         tm_then=((time_t) timenum);
         sprintf(mess,"%-35s %s ago",small_buff,converttime((long)((tm-tm_then)/60)));
         fputs(mess,fp2);
         fputs("\n",fp2);
         timebuf[0]=0;
         timenum=0;
         len=0;
         num++;
         fputs("COMMENT: ",fp2);
         sprintf(filename2,"%s/%s.c",filename,small_buff);
         fp=fopen(filename2,"r");
         fgets(comment,ARR_SIZE,fp);
         FCLOSE(fp);
         fputs(comment,fp2);
         fputs("\n\n",fp2);
         comment[0]=0;
        }
      small_buff[0]=0;
      len=0;
  }
 sprintf(mess,"Displayed %d banned site%s",num,num == 1 ? "" : "s");
 fputs(mess,fp2);
 fputs("\n",fp2);

 fclose(fp2);
 (void) closedir(dirp);

 if (!cat(filename3,user,0)) {
     sprintf(mess,"%s Cant cat restrict listing file!",ctime(&tm));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     }

 return;
 }

if (inpstr[0]=='.' ||
    inpstr[0]=='*' ||
    inpstr[0]=='/' ||
    inpstr[0]=='+' ||
    inpstr[0]=='-' ||
    inpstr[0]=='?' )
  {
   write_str(user,"Invalid site name.");
   return;
  }  
   
 sscanf(inpstr,"%s ",site_name);
 remove_first(inpstr);


/* open board file */
if (type==ANY)
  {
   sprintf(t_mess,"%s/%s",RESTRICT_DIR,site_name);
   strcpy(text_mess,"RESTRICTED from access.");
  }
else
  {
   sprintf(t_mess,"%s/%s",RESTRICT_NEW_DIR,site_name);
   strcpy(text_mess,"NEW USER restricted.");
  }
strncpy(filename,t_mess,FILE_NAME_LEN);


if (!strlen(inpstr)) {
 if ((fp=fopen(filename,"r"))) {
    write_str(user,"That site is already restricted.");
    fclose(fp);
    return;
    }
 FCLOSE(fp);
if (type==ANY)
  {
   sprintf(t_mess,"%s/%s.r",RESTRICT_DIR,site_name);
   strncpy(filename2,t_mess,FILE_NAME_LEN);
   fp=fopen(filename2,"w");
   sprintf(mess,BANNED_MESS,SYSTEM_EMAIL);   
   fputs(mess,fp);
   fclose(fp);
   sprintf(t_mess,"%s/%s.c",RESTRICT_DIR,site_name);
   strncpy(filename2,t_mess,FILE_NAME_LEN);
   fp=fopen(filename2,"w");
   fputs(BANNED_COMMENT,fp);
   fclose(fp);
  }
else
  {
   sprintf(t_mess,"%s/%s.r",RESTRICT_NEW_DIR,site_name);
   strncpy(filename2,t_mess,FILE_NAME_LEN);
   fp=fopen(filename2,"w");
   fputs(BANNED_NEW_MESS,fp);
   fclose(fp);
   sprintf(t_mess,"%s/%s.c",RESTRICT_NEW_DIR,site_name);
   strncpy(filename2,t_mess,FILE_NAME_LEN);
   fp=fopen(filename2,"w");
   fputs(BANNED_COMMENT,fp);
   fclose(fp);
  }
 }

else {
 midcpy(inpstr,chunk,0,1);

 if (strlen(inpstr) > 2) {
   if (!strcmp(chunk,"-c")) {
      midcpy(inpstr,comment,3,ARR_SIZE);
      if (!strlen(comment)) {
       write_str(user,"If you specify option, must give a message also.");
       chunk[0]=0;
       return;
       }
      if (type==ANY) {
      sprintf(t_mess,"%s/%s.c",RESTRICT_DIR,site_name);
      strncpy(filename2,t_mess,FILE_NAME_LEN);
      fp=fopen(filename2,"w");
      fputs(comment,fp);
      fclose(fp);
      }
      else {
      sprintf(t_mess,"%s/%s.c",RESTRICT_NEW_DIR,site_name);
      strncpy(filename2,t_mess,FILE_NAME_LEN);
      fp=fopen(filename2,"w");
      fputs(comment,fp);
      fclose(fp);
      }      
     }  /* end of comment if */
   else if (!strcmp(chunk,"-r")) {
      midcpy(inpstr,reason,3,ARR_SIZE);
      if (!strlen(reason)) {
       write_str(user,"If you specify option, must give a message also.");
       chunk[0]=0;
       return;
       }
      if (strlen(reason) > REASON_LEN) {
         write_str(user,"Reason too long.");
         reason[0]=0;
         chunk[0]=0;
         return;
         }
      if (type==ANY) {
      sprintf(t_mess,"%s/%s.r",RESTRICT_DIR,site_name);
      strncpy(filename2,t_mess,FILE_NAME_LEN);
      fp=fopen(filename2,"w");
      fputs(reason,fp);
      fclose(fp);
      }
      else {
      sprintf(t_mess,"%s/%s.r",RESTRICT_NEW_DIR,site_name);
      strncpy(filename2,t_mess,FILE_NAME_LEN);
      fp=fopen(filename2,"w");
      fputs(reason,fp);
      fclose(fp);
      }      
     }  /* end of reason if */
   else {
      write_str(user,"Option not understood..Aborting.");
      chunk[0]=0;
      return;
      }
   } /* end of sub-if */

 else {
   write_str(user,"Option not understood..Aborting.");
   chunk[0]=0;
   return;
   }
  /* inpstr exists but less than one or not equal to -c or -r */
} /* end of main else */

/* Write time banned to site file if file doesn't exist */
/* if file exists, leave alone */

if ((fp2=fopen(filename,"r"))) {
    fclose(fp2);
    sprintf(mess,"Comment/Reason added to site %s by %s",site_name,ustr[user].say_name);
    btell(user,mess);
    sprintf(mess,"%s: Comment/Reason added to site %s by %s\n",get_time(0,0),site_name,ustr[user].say_name);
    print_to_syslog(mess);
   }  /* end of if */

else {
 if (!(fp2=fopen(filename,"w"))) {
         sprintf(mess,"%s : restriction could not be applied: ",syserror);
         write_str(user,mess);
         sprintf(mess,"Cant open %s to restrict access.",filename);
         logerror(mess);
         return;
         }
 time(&tm);
 sprintf(timestr,"%ld\n",(unsigned long)tm);
 fputs(timestr,fp2);
 fclose(fp2);
  /* Now since we're creating a new ban, check which - option we       */
  /* specified..if we didn't give one, the default reason and comment  */
  /* are already written to the files from above. Write now the reason  */
  /* file if -c was specified or write the comment file if -r was given */
   if (!strcmp(chunk,"-c")) {
     sprintf(filename,"%s.r",filename);
     fp2=fopen(filename,"w");
     if (type==ANY) {
   	sprintf(mess,BANNED_MESS,SYSTEM_EMAIL);   
   	fputs(mess,fp2);
	}
     else
      fputs(BANNED_NEW_MESS,fp2);
     fclose(fp2);
     }
   else if (!strcmp(chunk,"-r")) {
     sprintf(filename,"%s.c",filename);
     fp2=fopen(filename,"w");
     fputs(BANNED_COMMENT,fp2);
     fclose(fp2);
     }

 sprintf(mess,"Site %s is now %s.",site_name, text_mess);
 write_str(user,mess);
 sprintf(mess,"%s: %s site %s by %s\n", get_time(0,0), text_mess, site_name, ustr[user].say_name);
 print_to_syslog(mess);
 sprintf(mess,"%s site %s by %s", text_mess, site_name, ustr[user].say_name);
 btell(user,mess);
 }  /* end of else */
}


/* Unban a site */
void unrestrict(int user, char *inpstr, int type)
{
char filename[FILE_NAME_LEN];

if (!strlen(inpstr)) {
        write_str(user,"You forgot the address"); return;
        }
 
/* check site name for stupid shit     */
/* like "." and ".." and "/etc/passwd" */
if (inpstr[0]=='.' ||
    inpstr[0]=='*' ||
    inpstr[0]=='/' ||
    inpstr[0]=='+' ||
    inpstr[0]=='-' ||
    inpstr[0]=='?' )
  {
   write_str(user,"Invalid address.");
   return;
  }  

/* Remove time restrict file */
 if (type==ANY)
   {
    sprintf(t_mess,"%s/%s",RESTRICT_DIR,inpstr);
   }
 else
   {
    sprintf(t_mess,"%s/%s",RESTRICT_NEW_DIR,inpstr);
   }
strncpy(filename,t_mess,FILE_NAME_LEN);

remove(filename);

/* Remove comment file */
 if (type==ANY)
   {
    sprintf(t_mess,"%s/%s.c",RESTRICT_DIR,inpstr);
   }
 else
   {
    sprintf(t_mess,"%s/%s.c",RESTRICT_NEW_DIR,inpstr);
   }
strncpy(filename,t_mess,FILE_NAME_LEN);

remove(filename);

/* Remove reason file */
 if (type==ANY)
   {
    sprintf(t_mess,"%s/%s.r",RESTRICT_DIR,inpstr);
   }
 else
   {
    sprintf(t_mess,"%s/%s.r",RESTRICT_NEW_DIR,inpstr);
   }
strncpy(filename,t_mess,FILE_NAME_LEN);

remove(filename);

sprintf(mess,"Site %s is ALLOWED ACCESS again.",inpstr);
write_str(user,mess);

if (type==ANY)
 sprintf(mess,"UNRESTRICT site %s by %s",inpstr,ustr[user].say_name);
else
 sprintf(mess,"UNBANNEW site %s by %s",inpstr,ustr[user].say_name);

btell(user,mess);

if (type==ANY)
 sprintf(mess,"%s: UNRESTRICT site %s by %s\n",get_time(0,0),inpstr,ustr[user].say_name);
else
 sprintf(mess,"%s: UNBANNEW site %s by %s\n",get_time(0,0),inpstr,ustr[user].say_name);

print_to_syslog(mess);
}


/* Ignore private tells */
void igtells(int user)
{
if (ustr[user].igtell) {
  write_str(user,"You are already ignoring .tells");
  return;
  }
 else {
  write_str(user,"You ignore your .tells");
  ustr[user].igtell=1;
  }
}

/* Listen to private tells */
void heartells(int user)
{
if (!ustr[user].igtell) {
  write_str(user,"You are already listening to your .tells");
  return;
  }
 else {
  write_str(user,"You listen to your .tells");
  ustr[user].igtell=0;
  }
}


/*** START OF BOT COMMANDS ***/

void get_whoinfo(int user, char *inpstr)
{
int u,v;

for (v=0;v<NUM_AREAS;++v) 
     {
       for (u=0;u<MAX_USERS;++u) {
	if ((ustr[u].area!= -1) && (ustr[u].area == v) && (!ustr[u].logging_in))
	        {
		sprintf(mess,"+++++ command: who: %s %s %d",ustr[u].say_name,astr[ustr[u].area].name,ustr[u].vis);
		write_bot(mess);
		} /* end of in area if */
	  } /* end of user for */
     } /* end of area for */
}


/*-----------------------------------------------------------*/
/* atmospherics code                                         */
/*-----------------------------------------------------------*/
/* until future changes, this is how the files must be:      */
/*                                                           */
/*  example:                                                 */
/*          10                                               */
/*          line of text                                     */
/*          20                                               */
/*          line of text                                     */
/*          30                                               */
/*          line of text                                     */
/*          40                                               */
/*          line of text                                     */
/*                                                           */
/*  To have a multi line message, use '@' in the line        */
/*-----------------------------------------------------------*/
/*** atmospheric function (uses area directory) ***/
void atmospherics()
{
FILE *fp;
char filename[FILE_NAME_LEN],probch[10],line[512];
int probint,area;
int rnd;

ATMOS_COUNTDOWN = ATMOS_COUNTDOWN - ((rand() % ATMOS_FACTOR) +1);

if ( ATMOS_COUNTDOWN > 0) return;

ATMOS_COUNTDOWN = ATMOS_RESET;

for (area=0; area<NUM_AREAS; ++area) 
  {
   if (!find_num_in_area(area)) continue;

   if (astr[area].atmos==0) continue;
	
   sprintf(t_mess, "%s/%s.atmos",datadir, astr[area].name);
   strncpy(filename,t_mess,FILE_NAME_LEN);

   if (!(fp=fopen(filename,"r"))) continue;

   rnd=rand() % 100;
   ATMOS_LAST = rnd;
	
   fgets(probch,6,fp);
   while(!feof(fp)) 
    {
     probint=atoi(probch);
     strcpy(line,"");
     line[0]=0;
     fgets(line,511,fp);
		
     if (rnd<probint) 
       { 
        write_area(area,line);  
        break;
       } 

      strcpy(probch,"");
      probch[0]=0;
      fgets(probch,6,fp);
     }
	  
   FCLOSE(fp);
  }
}


/*** write to areas - if area= -1 write to all areas ***/
void write_area(int area, char *inpstr)
{
int u;
int i=0;
int j=0;
char buff[ARR_SIZE];
char buff2[10];

for (u=0;u<MAX_USERS;++u) 
  {
    strcpy(buff,"");
    buff[0]=0;

    if (!user_wants_message(u,ATMOS)) continue;
    if (ustr[u].area==-1)             continue;
     
    if (ustr[u].area==area || area== -1)  
      { 
        j = strlen(inpstr);

        for (i=0;i<j;i++)
          {
            if (inpstr[i]=='@') {
              strcat(buff,"\n");
              if (ustr[u].car_return) { strcat(buff,"\r"); }
             }
            else {
             strcpy(buff2,"");
             buff2[0]=0;
             midcpy(inpstr,buff2,i,i);
             strcat(buff,buff2);
            }
          } /* end of for */

       write_str(u,buff);
      }
   }
}


/*** check to see if messages are out of date ***/
void check_mess(int startup)
{
int b,day,day2;
int normcount=0;
int wizcount=0;
char line[ARR_SIZE+31],datestr[30],timestr[7],boardfile[FILE_NAME_LEN],tempfile[FILE_NAME_LEN];
char daystr[11],daystr2[3];
time_t tm;
FILE *bfp,*tfp;

timestr[0]=0;

time(&tm);
strcpy(datestr,ctime(&tm));
midcpy(datestr,timestr,11,15);
midcpy(datestr,daystr,8,9);
day=atoi(daystr);

/* see if its time to check (midnight) */
if (startup==1) goto SKIP;
else if (startup==2) goto WLOG;

if (checked)
  {
   checked = 0;
   return;
  }
else if ((!strcmp(timestr,"00:01") || !strcmp(timestr,"00:00")) && !checked)
  {
   checked = 1;
  }
else return;

SKIP:
if (!startup) {
        write_area(-1,"");
	write_area(-1,"SYSTEM:: Midnight system check taking place, please wait...");
        print_to_syslog("SYSTEM:: Midnight system check in progress..\n");
   }

/* cycle through files */

strcpy(tempfile,get_temp_file());

for(b=0;b<NUM_AREAS+1;++b) {
        if (b == NUM_AREAS)
          sprintf(boardfile,"%s/wizmess",MESSDIR);
         else
	  sprintf(boardfile,"%s/board%d",MESSDIR,b);

	if (!(bfp=fopen(boardfile,"r"))) continue;
	if (!(tfp=fopen(tempfile,"w"))) {
               if (startup==1) {
		perror("\nSYSTEM: Cant open temp file to write in check_mess()");
		FCLOSE(bfp);
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
                exit(0);
                }
                else {
		logerror("Cant open temp file to write in check_mess()");
		FCLOSE(bfp);
                checked = 0;
		return;
                }
	   }

	/* go through board and write valid messages to temp file */
	fgets(line,ARR_SIZE+30,bfp);
	while(!feof(bfp)) {
		midcpy(line,daystr2,5,6);
		day2=atoi(daystr2);
		if (day2>day) day2 -= 30;  /* if mess from prev. month */
		if (day2>=day-MESS_LIFE)
                 fputs(line,tfp);
		else {
                 if (b == NUM_AREAS) wizcount++;
                 else normcount++;

                 astr[b].mess_num--;
                 }
		fgets(line,1050,bfp);
		}
	FCLOSE(bfp);  
	FCLOSE(tfp);
	remove(boardfile);

	/* rename temp file back to board file */
        if (rename(tempfile,boardfile) == -1)
         astr[b].mess_num=0;

        remove(tempfile);

        /* If changed boards are left with no messages, remove them */
        if (!file_count_lines(boardfile)) remove(boardfile);

	} /* end of for */
	
  /* Reset user auto-forward limits to nill */
  if (startup==1)
    printf("Resetting auto-forward limits and checking user abbreviations..\n");
   reset_userfors(startup);

  /* Remove all temp files from the junk directory */
  if (startup==1)
    printf("Removing temp files..\n");
   remove_junk(startup);

  if (!startup) {
  /* Auto expire users if set */
  auto_expire();
  /* Expire email verifications from today */
  check_verify(-1,1);
  }

WLOG:
  /* Append the day's user activity to the activity file */
  if ((startup==2) || (!startup)) {
    if (checked) write_meter(1);
    else write_meter(0);
   }

  /* Reset logins per hour numbers */
  b=0;
  for (b=0;b<24;++b) {
     logstat[b].logins = 0;
     }

        system_stats.logins_today       = 0;    
        system_stats.new_users_today    = 0;

if (!startup) {
        write_area(-1,"");
	write_area(-1,"SYSTEM:: Check completed. Carry on!");
        print_to_syslog("SYSTEM:: Check completed.\n");
        sprintf(mess,"SYSTEM:: %d old messages deleted from boards\n",normcount);
        print_to_syslog(mess);
        sprintf(mess,"SYSTEM:: %d old messages deleted from wiz board\n",wizcount);
        print_to_syslog(mess);
   }
if (startup==1) {
        sprintf(mess,"SYSTEM:: %d old messages deleted from boards\n",normcount);
        print_to_syslog(mess);
        sprintf(mess,"SYSTEM:: %d old messages deleted from wiz board\n",wizcount);
        print_to_syslog(mess);
   }

return;
}


/* Write login activity graph to file */
void write_meter(int mode)
{
int i=0;
int j=0;
char timebuf[100];
char datestr[80];
char daystr[11];
char filename[FILE_NAME_LEN];
time_t tm;
time_t day_ago = 60 * 60 * 24;
FILE *fp;

time(&tm);

if (mode==1) tm -= day_ago;
strcpy(datestr,ctime(&tm));
midcpy(datestr,daystr,0,9);

sprintf(mess,"User login activity for %s\n",daystr);

/* Open the file to write to it */
sprintf(filename,"%s",ACTVYFILE);
if (!(fp=fopen(filename,"a"))) {
      strcpy(mess,"Cant append login activity to file\n");
      print_to_syslog(mess);
      return;
  }

fputs(mess,fp);
time(&tm);
strcpy(datestr,ctime(&start_time));
datestr[strlen(datestr)-1]=0;
sprintf(mess,"System Booted: %s\n",datestr);
fputs(mess,fp);
sprintf(mess,"Total logins for this day: %ld\n\n",system_stats.logins_today);
fputs(mess,fp);
fputs("LOGS\n",fp);

timebuf[0]=0;

/* If the hour's login numbers fall into the range, put an asterix */
/* in the buffer, else put a space                                 */

strcpy(timebuf,"100+  | ");

for (i=0;i<24;++i) {
   if (logstat[i].logins > 100)
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);

i=0;
j=0;

strcpy(timebuf,"91-100| ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 90) && (logstat[i].logins <= 100))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"81-90 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 80) && (logstat[i].logins <= 90))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"71-80 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 70) && (logstat[i].logins <= 80))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"61-70 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 60) && (logstat[i].logins <= 70))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"51-60 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 50) && (logstat[i].logins <= 60))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"41-50 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 40) && (logstat[i].logins <= 50))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"31-40 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 30) && (logstat[i].logins <= 40))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"21-30 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 20) && (logstat[i].logins <= 30))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"11-20 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 10) && (logstat[i].logins <= 20))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"0-10  | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 0) && (logstat[i].logins <= 10))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

fputs(timebuf,fp);
fputs("\n",fp);
i=0;
j=0;

strcpy(timebuf,"      +-|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|");
fputs(timebuf,fp);
fputs("\n",fp);
timebuf[0]=0;
strcpy(timebuf,"      12a  1  2  3  4  5  6  7  8  9 10 11 12p 1  2  3  4  5  6  7  8  9 10 11");
fputs(timebuf,fp);
fputs("\n\n\n",fp);

fclose(fp);
strcpy(timebuf,"");
timebuf[0]=0;
i=0;
j=0;

return;
}


/*** Check for auto_shutdown and auto_reboot ***/
void check_shut()
{
int offset=0;
int c=0;

if (down_time > 1) offset=down_time-1;
        
if ((down_time >= 61) && (down_time < 1441)) {
for (c=1;c<24;++c) {
 if (down_time == (60*c+1)) {
   if (c==1) {
   if (treboot)
    sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  hour",c);
   else
    sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  hour",c);
    }
   else {
   if (treboot)
    sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  hours",c);
   else
    sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  hours",c);
    }
   write_area(-1,"");
   write_area(-1,mess);
   break;
   }   
 }  /* end of for */
c=0;
}  /* end of if */

else if ((down_time >= 1441) && (down_time <= 32767)) {
for (c=1;c<23;++c) {
 if (down_time == (1440*c+1)) {
   if (c==1) {
   if (treboot)
    sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  day",c);
   else
    sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  day",c);
    }
   else {
   if (treboot)
    sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  days",c);
   else
    sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  days",c);
    }
   write_area(-1,"");
   write_area(-1,mess);
   break;
   }
 }  /* end of for */
c=0;
}  /* end of if */

if (down_time==51) {
if (treboot)
 sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  minutes",offset);
else
 sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  minutes",offset);
write_area(-1,"");
write_area(-1,mess);
   }
if (down_time==41) {
if (treboot)
 sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  minutes",offset);
else
 sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  minutes",offset);
write_area(-1,"");
write_area(-1,mess);
   }
if (down_time==31) {
if (treboot)
 sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  minutes",offset);
else
 sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  minutes",offset);
write_area(-1,"");
write_area(-1,mess);
   }
if (down_time==21) {
if (treboot)
 sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  minutes",offset);
else
 sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  minutes",offset);
write_area(-1,"");
write_area(-1,mess);
   }
if (down_time==11) {
if (treboot)
 sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  minutes",offset);
else
 sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  minutes",offset);
write_area(-1,"");
write_area(-1,mess);
   }
if (down_time==6) {
if (treboot)
 sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  minutes",offset);
else
 sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  minutes",offset);
write_area(-1,"");
write_area(-1,mess);
   }
if (down_time==2) {
if (treboot)
 sprintf(mess,"SYSTEM:: Talker auto-reboot in  %i  minutes",offset);
else
 sprintf(mess,"SYSTEM:: Talker auto-shutdown in  %i  minutes",offset);
write_area(-1,"");
write_area(-1,mess);
   }
if (down_time==1) shutdown_auto();
down_time = offset;

}

/*** see if any users are near or at idle limit or need flags reset***/
void check_idle()
{
int min,user;

for (user=0; user<MAX_USERS; ++user) 
  {
   if (ustr[user].logging_in) 
     {
       min=(int)((time(0) - ustr[user].last_input)/60);
       if (min >= LOGIN_TIMEOUT)
         {
          if (user_wants_message(user,BEEPS))
           write_str(user,"\07");
          write_str(user,"Connection closed due to exceeeded login time limit");
          user_quit(user);
         }
      }
   }


for (user=0;user<MAX_USERS;++user) 
  {
    if (ustr[user].suspended && (ustr[user].xco_time==1)) 
     {
      ustr[user].suspended=0;
      ustr[user].xco_time=0;
      write_str(user,XCOMMOFF_MESS);
      sprintf(mess,"%s XCOM OFF: %s, by the talker.",STAFF_PREFIX,ustr[user].say_name);
      writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);
      strncpy(bt_conv[bt_count],mess,MAX_LINE_LEN);
      bt_count = ( ++bt_count ) % NUM_LINES;
    }
   else if (ustr[user].suspended && (ustr[user].xco_time > 1)) {
      ustr[user].xco_time--;
      }

    if (ustr[user].frog && (ustr[user].frog_time==1)) 
     {
      ustr[user].frog=0;
      ustr[user].frog_time=0;
      write_str(user,FROGOFF_MESS);
      sprintf(mess,"%s FROG OFF: %s, by the talker.",STAFF_PREFIX,ustr[user].say_name);
      writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);
      strncpy(bt_conv[bt_count],mess,MAX_LINE_LEN);
      bt_count = ( ++bt_count ) % NUM_LINES;
    }
   else if (ustr[user].frog && (ustr[user].frog_time > 1)) {
      ustr[user].frog_time--;
      }

    if (ustr[user].anchor && (ustr[user].anchor_time==1)) 
     {
      ustr[user].anchor=0;
      ustr[user].anchor_time=0;
      write_str(user,ANCHOROFF_MESS);
      sprintf(mess,"%s ANCHOR OFF: %s, by the talker.",STAFF_PREFIX,ustr[user].say_name);
      writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);
      strncpy(bt_conv[bt_count],mess,MAX_LINE_LEN);
      bt_count = ( ++bt_count ) % NUM_LINES;
    }
   else if (ustr[user].anchor && (ustr[user].anchor_time > 1)) {
      ustr[user].anchor_time--;
      }

    if (ustr[user].gagcomm && (ustr[user].gag_time==1)) 
     {
      ustr[user].gagcomm=0;
      ustr[user].gag_time=0;
      write_str(user,GCOMMOFF_MESS);
      sprintf(mess,"%s GCOM OFF: %s, by the talker.",STAFF_PREFIX,ustr[user].say_name);
      writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);
      strncpy(bt_conv[bt_count],mess,MAX_LINE_LEN);
      bt_count = ( ++bt_count ) % NUM_LINES;
    }
   else if (ustr[user].gagcomm && (ustr[user].gag_time > 1)) {
      ustr[user].gag_time--;
      }

   if (!ustr[user].shout && (ustr[user].muz_time==1))
     {
       ustr[user].shout=1;
       ustr[user].muz_time=0;
       sprintf(mess,"%s can shout again",ustr[user].say_name);
       writeall_str(mess, 1, user, 1, user, NORM, NONE, 0);
       write_str(user,MUZZLEOFF_MESS);
       sprintf(mess,"%s UNMUZZLE: %s, by the talker.",STAFF_PREFIX,ustr[user].say_name);
       writeall_str(mess, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);
       strncpy(bt_conv[bt_count],mess,MAX_LINE_LEN);
       bt_count = ( ++bt_count ) % NUM_LINES;
     }
   else if (!ustr[user].shout && (ustr[user].muz_time > 1)) {
      ustr[user].muz_time--;
      }

   if (ustr[user].super >= IDLE_LEVEL) continue;
   
   if (!strcmp(ustr[user].name,BOT_ID)) continue;

   if ((ustr[user].area == -1 && !ustr[user].logging_in)) continue; 
   
   min=(int)((time(0) - ustr[user].last_input)/60);
   
   if ( ( min >= (IDLE_TIME - 2)) && !ustr[user].warning_given) 
     {
      if (user_wants_message(user,BEEPS))
       write_str(user,"\07*** Warning - input within 2 minutes or you will be disconnected ***");
      else
       write_str(user,"*** Warning - input within 2 minutes or you will be disconnected ***");
      ustr[user].warning_given=1;
      continue;
     }
     
   if (min >= IDLE_TIME ) 
    {
     write_str(user,IDLE_BYE_MESS);
     user_quit(user);
    }
   }
   
says_running     = (says     + says_running)     / 2;
tells_running    = (tells    + tells_running)    / 2;
commands_running = (commands + commands_running) / 2;

tells = 0;
commands = 1;
says = 0;

if (fight.issued)
  {
    min = (int)((time(0)-fight.time)/60);
    if (min > IDLE_TIME)
      reset_chal(0," ");

  }
  
}	

/*----------------------------------------------------------------------*/
/* initialize a new user                                                */
/*----------------------------------------------------------------------*/
void init_user(int user)
{
int i;
time_t tm;

time(&tm);
   strcpy(ustr[user].name,       ustr[user].login_name);
   strcpy(ustr[user].say_name,   ustr[user].name);

ustr[user].say_name[0]=toupper((int)ustr[user].say_name[0]);

   strcpy(ustr[user].email_addr, DEF_EMAIL);
   strcpy(ustr[user].desc,       DEF_DESC);
   strcpy(ustr[user].sex,        DEF_GENDER);
   strcpy(ustr[user].init_date,  ctime(&tm));
   ustr[user].init_date[24]=0;
   strcpy(ustr[user].last_date,  ctime(&tm));
   ustr[user].last_date[24]=0;
   strcpy(ustr[user].init_site,    ustr[user].site);
   strcpy(ustr[user].last_site,    ustr[user].site);
   strcpy(ustr[user].last_name,    ustr[user].net_name);
   strcpy(ustr[user].init_netname, ustr[user].net_name);
   strcpy(ustr[user].succ,       DEF_SUCC);
   strcpy(ustr[user].fail,       DEF_FAIL);
   strcpy(ustr[user].entermsg,   DEF_ENTER);
   strcpy(ustr[user].exitmsg,    DEF_EXIT);
   strcpy(ustr[user].homepage,   DEF_URL);
   strcpy(ustr[user].webpic,     DEF_PICURL);
   strcpy(ustr[user].home_room, astr[new_room].name);
   ustr[user].rawtime   = tm;
   ustr[user].afkmsg[0] = 0;
   ustr[user].promote   = 0;

   for(i=0;i<MAX_AREAS;i++)
     {
      ustr[user].security[i]='N';
     }

ustr[user].super=            0;
ustr[user].area=             new_room;
ustr[user].shout=            1;
ustr[user].vis=              1;
ustr[user].locked=           0;
ustr[user].suspended=        0;
ustr[user].monitor=          0;
ustr[user].rows=             24;
ustr[user].cols=             256;
ustr[user].car_return=       1;
ustr[user].abbrs =           1;
ustr[user].times_on =        0;
ustr[user].white_space =     1;
ustr[user].aver =            0;
ustr[user].totl =            0;
ustr[user].autor =           0;
ustr[user].autof =           0;
ustr[user].automsgs =        0;
ustr[user].gagcomm =         0;
ustr[user].semail =          0;
ustr[user].quote =           1;
ustr[user].hilite =          1;
ustr[user].new_mail =        0;
ustr[user].color =           COLOR_DEFAULT;
ustr[user].numcoms =         0;
ustr[user].mail_num =        0;
ustr[user].numbering =       0;
ustr[user].ttt_kills =       0;
ustr[user].ttt_killed =      0;
ustr[user].ttt_board =       0;
ustr[user].ttt_opponent =   -3;
ustr[user].ttt_playing =     0;
ustr[user].hang_stage =     -1;   
ustr[user].hang_word[0] =   '\0';
ustr[user].hang_word_show[0]='\0';
ustr[user].hang_guess[0] =  '\0';
ustr[user].hang_wins =       0;
ustr[user].hang_losses =     0;
strcpy(ustr[user].icq, DEF_ICQ);
strcpy(ustr[user].miscstr1, "NA");
strcpy(ustr[user].miscstr2, "NA");
strcpy(ustr[user].miscstr3, "NA");
strcpy(ustr[user].miscstr4, "NA");
ustr[user].pause_login =     1;
ustr[user].miscnum2 =        0;
ustr[user].miscnum3 =        0;
ustr[user].miscnum4 =        0;
ustr[user].miscnum5 =        0;

for (i=0;i<NUM_MACROS;i++) {
 ustr[user].Macros[i].name[0]=0;
 ustr[user].Macros[i].body[0]=0;
}
i=0;
   for (i=0; i<MAX_ALERT; i++)
     {
      ustr[user].friends[i][0]=0;
     }
   i=0;
   for (i=0; i<MAX_GAG; i++)
     {
      ustr[user].gagged[i][0]=0;
     }
   i=0;
   for (i=0; i<MAX_GRAVOKES; i++)
     {
      ustr[user].revokes[i][0]=0;
     }
   i=0;
   for (i=0; i<NUM_LINES; i++)
     {
      ustr[user].conv[i][0]=0;
     }

initabbrs(user);
listen_all(user);

if (strcmp(ustr[user].login_name,ROOT_ID)==0) 
  {
    ustr[user].super = MAX_LEVEL;
    ustr[user].promote = 1;

    for(i=0;i<MAX_AREAS;i++)
     {
      ustr[user].security[i]='Y';
     }
  }

}

/*----------------------------------------------------------------------*/
/* copy the user structures from the temp buffer to an actual user      */
/*----------------------------------------------------------------------*/
void copy_to_user(int user)
{
int i=0;

strcpy(ustr[user].name,t_ustr.name);
strcpy(ustr[user].say_name,t_ustr.say_name);
strcpy(ustr[user].password,t_ustr.password);

ustr[user].super=t_ustr.super;

strcpy(ustr[user].email_addr,t_ustr.email_addr);
strcpy(ustr[user].desc,t_ustr.desc);
strcpy(ustr[user].sex,t_ustr.sex);
strcpy(ustr[user].init_date,t_ustr.init_date);
strcpy(ustr[user].last_date,t_ustr.last_date);
strcpy(ustr[user].init_site,t_ustr.init_site);
strcpy(ustr[user].last_site,t_ustr.last_site);
strcpy(ustr[user].last_name,t_ustr.last_name);
strcpy(ustr[user].init_netname,t_ustr.init_netname);

while (strlen(t_ustr.custAbbrs[i].com) > 1) {   
  strcpy(ustr[user].custAbbrs[i].abbr,t_ustr.custAbbrs[i].abbr);
  strcpy(ustr[user].custAbbrs[i].com,t_ustr.custAbbrs[i].com);
  i++;
 }
i=0;
for (i=0;i<NUM_MACROS;i++) {
 if (strlen(t_ustr.Macros[i].name)) {
  strcpy(ustr[user].Macros[i].name,t_ustr.Macros[i].name);
  strcpy(ustr[user].Macros[i].body,t_ustr.Macros[i].body);
  }
 else {
  ustr[user].Macros[i].name[0]=0;
  ustr[user].Macros[i].body[0]=0;
 }
}
i=0;
for (i=0; i<MAX_ALERT; i++) {
 if (strlen(t_ustr.friends[i]))
  strcpy(ustr[user].friends[i],t_ustr.friends[i]);
 else
  ustr[user].friends[i][0]=0;
 }
i=0;
for (i=0; i<MAX_GAG; i++) {
 if (strlen(t_ustr.gagged[i]))
  strcpy(ustr[user].gagged[i],t_ustr.gagged[i]);
 else
  ustr[user].gagged[i][0]=0;
 }

ustr[user].area           =t_ustr.area;
ustr[user].shout          =t_ustr.shout;
ustr[user].vis            =t_ustr.vis;
ustr[user].locked         =t_ustr.locked;
ustr[user].suspended      =t_ustr.suspended;

strcpy(ustr[user].entermsg,t_ustr.entermsg);
strcpy(ustr[user].exitmsg,t_ustr.exitmsg);
strcpy(ustr[user].home_room,t_ustr.home_room);
strcpy(ustr[user].fail,t_ustr.fail);
strcpy(ustr[user].succ,t_ustr.succ);
strcpy(ustr[user].homepage,t_ustr.homepage);
strcpy(ustr[user].creation,t_ustr.creation);

strcpy(ustr[user].security,t_ustr.security);
strcpy(ustr[user].flags, t_ustr.flags);
strcpy(ustr[user].webpic, t_ustr.webpic);

ustr[user].numcoms        =t_ustr.numcoms;
ustr[user].totl           =t_ustr.totl;
ustr[user].rawtime        =t_ustr.rawtime;

ustr[user].monitor        =t_ustr.monitor;
ustr[user].rows           =t_ustr.rows;
ustr[user].cols           =t_ustr.cols;
ustr[user].car_return     =t_ustr.car_return;
ustr[user].abbrs          =t_ustr.abbrs;
ustr[user].times_on       =t_ustr.times_on;
ustr[user].white_space    =t_ustr.white_space;
ustr[user].aver           =t_ustr.aver;
ustr[user].autor          =t_ustr.autor;
ustr[user].autof          =t_ustr.autof;
ustr[user].automsgs       =t_ustr.automsgs;
ustr[user].gagcomm        =t_ustr.gagcomm;
ustr[user].semail         =t_ustr.semail;
ustr[user].quote          =t_ustr.quote;
ustr[user].hilite         =t_ustr.hilite;
ustr[user].new_mail       =t_ustr.new_mail;
ustr[user].color          =t_ustr.color;
ustr[user].passhid        =t_ustr.passhid;

ustr[user].pbreak         =t_ustr.pbreak;
ustr[user].beeps          =t_ustr.beeps;
ustr[user].mail_warn      =t_ustr.mail_warn;
ustr[user].mail_num       =t_ustr.mail_num;
ustr[user].friend_num     =t_ustr.friend_num;
ustr[user].revokes_num    =t_ustr.revokes_num;
ustr[user].gag_num        =t_ustr.gag_num;
ustr[user].nerf_kills     =t_ustr.nerf_kills;
ustr[user].nerf_killed    =t_ustr.nerf_killed;
ustr[user].muz_time       =t_ustr.muz_time;
ustr[user].xco_time       =t_ustr.xco_time;
ustr[user].gag_time       =t_ustr.gag_time;
ustr[user].frog           =t_ustr.frog;
ustr[user].frog_time      =t_ustr.frog_time;
ustr[user].anchor         =t_ustr.anchor;
ustr[user].anchor_time    =t_ustr.anchor_time;
ustr[user].promote        =t_ustr.promote;
ustr[user].help           =t_ustr.help;
ustr[user].who            =t_ustr.who;
ustr[user].ttt_kills      =t_ustr.ttt_kills;
ustr[user].ttt_killed     =t_ustr.ttt_killed;
ustr[user].hang_wins      =t_ustr.hang_wins;
ustr[user].hang_losses    =t_ustr.hang_losses;
ustr[user].pause_login    =t_ustr.pause_login;
ustr[user].miscnum2       =t_ustr.miscnum2;
ustr[user].miscnum3       =t_ustr.miscnum3;
ustr[user].miscnum4       =t_ustr.miscnum4;
ustr[user].miscnum5       =t_ustr.miscnum5;
strcpy(ustr[user].icq,t_ustr.icq);
strcpy(ustr[user].miscstr1,t_ustr.miscstr1);
strcpy(ustr[user].miscstr2,t_ustr.miscstr2);
strcpy(ustr[user].miscstr3,t_ustr.miscstr3);
strcpy(ustr[user].miscstr4,t_ustr.miscstr4);

i=0;
for (i=0; i<MAX_GRAVOKES; i++) {
 if (strlen(t_ustr.revokes[i]))
  strcpy(ustr[user].revokes[i],t_ustr.revokes[i]);
 else
  ustr[user].revokes[i][0]=0;
 }

return;
}

/*----------------------------------------------------------------------*/
/* copy structures from an online user to a temp buffer                 */
/*----------------------------------------------------------------------*/
void copy_from_user(int user)
{
int i=0;

strcpy(t_ustr.name,ustr[user].name);
strcpy(t_ustr.say_name,ustr[user].say_name);
strcpy(t_ustr.password,ustr[user].password);

t_ustr.super = ustr[user].super;

strcpy(t_ustr.email_addr, ustr[user].email_addr);
strcpy(t_ustr.desc,       ustr[user].desc);
strcpy(t_ustr.sex,        ustr[user].sex);
strcpy(t_ustr.init_date,  ustr[user].init_date);
strcpy(t_ustr.last_date,  ustr[user].last_date);
strcpy(t_ustr.init_site,  ustr[user].init_site);
strcpy(t_ustr.last_site,  ustr[user].last_site);
strcpy(t_ustr.last_name,  ustr[user].last_name);
strcpy(t_ustr.init_netname,  ustr[user].init_netname);

while (strlen(ustr[user].custAbbrs[i].com) > 1) {
  strcpy(t_ustr.custAbbrs[i].abbr,ustr[user].custAbbrs[i].abbr);
  strcpy(t_ustr.custAbbrs[i].com, ustr[user].custAbbrs[i].com);
  i++;
 }
i=0;
for (i=0;i<NUM_MACROS;i++) {
 if (strlen(ustr[user].Macros[i].name)) {
  strcpy(t_ustr.Macros[i].name,ustr[user].Macros[i].name);
  strcpy(t_ustr.Macros[i].body,ustr[user].Macros[i].body);
 }
 else {
  t_ustr.Macros[i].name[0]=0;
  t_ustr.Macros[i].body[0]=0;
 }
}
i=0;
for (i=0; i<MAX_ALERT; i++) {
 if (strlen(ustr[user].friends[i]))
  strcpy(t_ustr.friends[i],ustr[user].friends[i]);
 else
  t_ustr.friends[i][0]=0;
}
i=0;
for (i=0; i<MAX_GAG; i++) {
 if (strlen(ustr[user].gagged[i]))
  strcpy(t_ustr.gagged[i],ustr[user].gagged[i]);
 else
  t_ustr.gagged[i][0]=0;
}

t_ustr.area            =ustr[user].area;
t_ustr.shout           =ustr[user].shout;
t_ustr.vis             =ustr[user].vis;
t_ustr.locked          =ustr[user].locked;
t_ustr.suspended       =ustr[user].suspended;

strcpy(t_ustr.entermsg, ustr[user].entermsg);
strcpy(t_ustr.exitmsg, ustr[user].exitmsg);
strcpy(t_ustr.home_room, ustr[user].home_room);
strcpy(t_ustr.fail, ustr[user].fail);
strcpy(t_ustr.succ, ustr[user].succ);
strcpy(t_ustr.homepage, ustr[user].homepage);
strcpy(t_ustr.creation, ustr[user].creation);
strcpy(t_ustr.security, ustr[user].security);
strcpy(t_ustr.flags, ustr[user].flags);
strcpy(t_ustr.webpic, ustr[user].webpic);

t_ustr.numcoms         =ustr[user].numcoms;
t_ustr.totl            =ustr[user].totl;
t_ustr.rawtime         =ustr[user].rawtime;

t_ustr.monitor         =ustr[user].monitor;
t_ustr.rows            =ustr[user].rows;
t_ustr.cols            =ustr[user].cols;
t_ustr.car_return      =ustr[user].car_return;
t_ustr.abbrs           =ustr[user].abbrs;
t_ustr.times_on        =ustr[user].times_on;
t_ustr.white_space     =ustr[user].white_space;
t_ustr.aver            =ustr[user].aver;
t_ustr.autor           =ustr[user].autor;
t_ustr.autof           =ustr[user].autof;
t_ustr.automsgs        =ustr[user].automsgs;
t_ustr.gagcomm         =ustr[user].gagcomm;
t_ustr.semail          =ustr[user].semail;
t_ustr.quote           =ustr[user].quote;
t_ustr.hilite          =ustr[user].hilite;
t_ustr.new_mail        =ustr[user].new_mail;
t_ustr.color           =ustr[user].color;
t_ustr.passhid         =ustr[user].passhid;

t_ustr.pbreak          =ustr[user].pbreak;
t_ustr.beeps           =ustr[user].beeps;
t_ustr.mail_warn       =ustr[user].mail_warn;
t_ustr.mail_num        =ustr[user].mail_num;
t_ustr.friend_num      =ustr[user].friend_num;
t_ustr.revokes_num     =ustr[user].revokes_num;
t_ustr.gag_num         =ustr[user].gag_num;
t_ustr.nerf_kills      =ustr[user].nerf_kills;
t_ustr.nerf_killed     =ustr[user].nerf_killed;
t_ustr.muz_time        =ustr[user].muz_time;
t_ustr.xco_time        =ustr[user].xco_time;
t_ustr.gag_time        =ustr[user].gag_time;
t_ustr.frog            =ustr[user].frog;
t_ustr.frog_time       =ustr[user].frog_time;
t_ustr.anchor          =ustr[user].anchor;
t_ustr.anchor_time     =ustr[user].anchor_time;
t_ustr.promote         =ustr[user].promote;
t_ustr.help            =ustr[user].help;
t_ustr.who             =ustr[user].who;
t_ustr.ttt_kills       =ustr[user].ttt_kills;
t_ustr.ttt_killed      =ustr[user].ttt_killed;
t_ustr.hang_wins       =ustr[user].hang_wins;
t_ustr.hang_losses     =ustr[user].hang_losses;
t_ustr.pause_login     =ustr[user].pause_login;
t_ustr.miscnum2        =ustr[user].miscnum2;
t_ustr.miscnum3        =ustr[user].miscnum3;
t_ustr.miscnum4        =ustr[user].miscnum4;
t_ustr.miscnum5        =ustr[user].miscnum5;
strcpy(t_ustr.icq,ustr[user].icq);
strcpy(t_ustr.miscstr1,ustr[user].miscstr1);
strcpy(t_ustr.miscstr2,ustr[user].miscstr2);
strcpy(t_ustr.miscstr3,ustr[user].miscstr3);
strcpy(t_ustr.miscstr4,ustr[user].miscstr4);

i=0;
for (i=0; i<MAX_GRAVOKES; i++) {
 if (strlen(ustr[user].revokes[i]))
  strcpy(t_ustr.revokes[i],ustr[user].revokes[i]);
 else
  t_ustr.revokes[i][0]=0;
}

return;
}


/*----------------------------------------------------------------------*/
/* read the users data file into the temp buffer                        */
/*----------------------------------------------------------------------*/
int read_user(char *name)
{
int i=0;
int l=0;
int num=0;
char buff1[ARR_SIZE];
char filename[FILE_NAME_LEN];
char z_mess[FILE_NAME_LEN+NAME_LEN+1];
FILE *f;                 /* user file*/
struct stat fileinfo;

buff1[0]=0;

if (!strlen(name)) return 0;

sprintf(z_mess,"%s/%s",USERDIR,name);
strncpy(filename,z_mess,FILE_NAME_LEN);

f = fopen (filename, "r"); /* open for output */
if (f == NULL)
  {
    return 0;
  }

stat(filename, &fileinfo);

if (fileinfo.st_size == 0) {
 fclose(f);
 remove(filename);
 sprintf(z_mess,"Found 0 length user file for %s..removing.\n",name);
 print_to_syslog(z_mess);
 return -1;
 }

  
/*--------------------------------------------------------*/
/* values added after initial release must be initialized */
/*--------------------------------------------------------*/
t_ustr.numcoms       = 0;
t_ustr.totl          = 0;
t_ustr.rawtime       = 0;

t_ustr.monitor       = 0;
t_ustr.rows          = 24;
t_ustr.cols          = 256;
t_ustr.car_return    = 1;
t_ustr.abbrs         = 1;
t_ustr.white_space   = 1;
t_ustr.times_on      = 1;
t_ustr.aver          = 0;
t_ustr.autor         = 0;
t_ustr.autof         = 0;
t_ustr.automsgs      = 0;
t_ustr.gagcomm       = 0;
t_ustr.semail        = 0;
t_ustr.quote         = 1;
t_ustr.hilite        = 0;
t_ustr.new_mail      = 0;
t_ustr.color         = COLOR_DEFAULT;
t_ustr.passhid       = 0;

t_ustr.pbreak        = 0;
t_ustr.beeps         = 0;
t_ustr.mail_warn     = 0;
t_ustr.mail_num      = 0;
t_ustr.friend_num    = 0;
t_ustr.revokes_num   = 0;
t_ustr.gag_num       = 0;
t_ustr.nerf_kills    = 0;
t_ustr.nerf_killed   = 0;
t_ustr.muz_time      = 0;
t_ustr.xco_time      = 0;
t_ustr.gag_time      = 0;
t_ustr.frog          = 0;
t_ustr.frog_time     = 0;
t_ustr.anchor        = 0;
t_ustr.anchor_time   = 0;
t_ustr.promote       = 0;
t_ustr.ttt_kills     = 0;
t_ustr.ttt_killed    = 0;
t_ustr.hang_wins     = 0;
t_ustr.hang_losses   = 0;
t_ustr.pause_login   = 0;
t_ustr.miscnum2      = 0;
t_ustr.miscnum3      = 0;
t_ustr.miscnum4      = 0;
t_ustr.miscnum5      = 0;
listen_all(-1);

/* first line is either version number or users name */
rbuf(buff1,NAME_LEN);
if (strstr(buff1,".ver")) {
	/* this is a standardized file format */
	/* now check for version difference */
	if (!strcmp(buff1,UDATA_VERSION)) {
	  /* no difference, continue on reading */
	  rbuf(t_ustr.name,NAME_LEN);            /* users name */
	}
	else {
	  /* version difference, lets try and convert */
	  if (!convert_file(f,filename,0)) { return 0; }
	  /* reopen file after the conversion and continue on */
	  /* convert_file() closes the file		      */
	  f = fopen (filename, "r"); /* open for output */
		if (f == NULL)
		  {
		    return 0;
		  }

	  rbuf(buff1,NAME_LEN); /* VERSION */
	  rbuf(t_ustr.name,NAME_LEN); /* users name */
	} /* end of else */
  }
else {
	/* old data file format */
	/* we need to convert to new format */
	if (!convert_file(f,filename,1)) { return 0; }
	/* reopen file after the conversion and continue on */
	/* convert_file() closes the file		    */
	f = fopen (filename, "r"); /* open for output */
		if (f == NULL)
		  {
		    return 0;
		  }
	rbuf(buff1,NAME_LEN);		       /* VERSION */
	rbuf(t_ustr.name,NAME_LEN);            /* users name */
  }

rbuf(t_ustr.say_name,NAME_LEN);        /* users properly capitalized name */
rbuf(t_ustr.password,-1);              /* users encrypted password */
rval(t_ustr.super);                    /* users level or rank */
rbuf(t_ustr.email_addr,EMAIL_LENGTH);  /* users email address */
rbuf(t_ustr.desc,DESC_LEN);            /* users description */
rbuf(t_ustr.sex,32);                   /* users gender */
rbuf(t_ustr.init_date,25);             /* users original login time */
rbuf(t_ustr.last_date,25);             /* users last login time */
rbuf(t_ustr.init_site,21);             /* users original site */
rbuf(t_ustr.last_site,21);             /* users last site */
rbuf(t_ustr.last_name,64);             /* users last hostname */
rbuf(t_ustr.init_netname,64);          /* users original hostname */

/* Clear first ..Abbrs.. line */
rbuf(buff1,-1);
strcpy(buff1,"");

for (;;) {
 rbuf(buff1,20);
 if (!strcmp(buff1,"..End abbrs..")) break;
  if (i) {
    strcpy(t_ustr.custAbbrs[l].com,buff1);
    i=0; l++; continue;
   }
  else {
    strcpy(t_ustr.custAbbrs[l].abbr,buff1);
    i=1;
   }
 }
 
i=0;
l=0;
    
/* Clear first ..Macros.. line */
rbuf(buff1,-1);
strcpy(buff1,"");
    
for (;;) {
 rbuf(buff1,MACRO_LEN);
 if (!strcmp(buff1,"..End macros..")) break;
 if (i) {
  strcpy(t_ustr.Macros[num].body,buff1);
  l=strlen(t_ustr.Macros[num].body);
  t_ustr.Macros[num].body[l]=0;
  i=0; num++; continue;
  }
 else {
  strcpy(t_ustr.Macros[num].name,buff1);
  l=strlen(t_ustr.Macros[num].name);
  t_ustr.Macros[num].name[l]=0;
  i=1;
  }
 }  

for (i=num;i<NUM_MACROS;++i) {
 t_ustr.Macros[i].name[0]=0;
 t_ustr.Macros[i].body[0]=0;
 }

i=0;
num=0;

/* Clear first ..Friends.. line */
rbuf(buff1,-1);
strcpy(buff1,"");
    
for (;;) {
 rbuf(buff1,NAME_LEN);
 if (!strcmp(buff1,"..End friends..")) break;
 else {
  strcpy(t_ustr.friends[i],buff1);
  l=strlen(t_ustr.friends[i]);
  t_ustr.friends[i][l]=0;
  i++;
  }
 }  

for (l=i;l<MAX_ALERT;++l) t_ustr.friends[l][0]=0;

i=0;

/* Clear first ..Gagged.. line */
rbuf(buff1,-1);
strcpy(buff1,"");
    
for (;;) {
 rbuf(buff1,NAME_LEN);
 if (!strcmp(buff1,"..End gagged..")) break;
 else {
  strcpy(t_ustr.gagged[i],buff1);
  l=strlen(t_ustr.gagged[i]);
  t_ustr.gagged[i][l]=0;
  i++;
  }
 }  

for (l=i;l<MAX_GAG;++l) t_ustr.gagged[l][0]=0;

/*----------------------------------------*/
/*  users last area in and will login to  */
/*  users muzzled or not                  */
/*  users visible or not                  */
/*  users locked or not                   */
/*  users xcommed or not                  */
/*----------------------------------------*/
fscanf(f, "%d %d %d %d %d\n", &t_ustr.area, &t_ustr.shout,
         &t_ustr.vis, &t_ustr.locked, &t_ustr.suspended);

rbuf(t_ustr.entermsg,MAX_ENTERM);    /* users room enter message */
rbuf(t_ustr.exitmsg,MAX_EXITM);      /* users room exit message */
rbuf(t_ustr.home_room,NAME_LEN);     /* users home room */
rbuf(t_ustr.fail,MAX_ENTERM);        /* users fail message */
rbuf(t_ustr.succ,MAX_ENTERM);        /* users success message */
rbuf(t_ustr.homepage,HOME_LEN);      /* users homepage */

rbuf(t_ustr.creation,25);    /* users creation date */
rbuf(t_ustr.security,MAX_AREAS);    /* users room permissions */

i=0;
if (MAX_AREAS > strlen(t_ustr.security)) {
 for (i=0;i<(MAX_AREAS-strlen(t_ustr.security));++i)
  strcat(t_ustr.security,"N");
 }
i=0;

rbuf(t_ustr.flags,NUM_IGN_FLAGS+2);       /* users listening and ignoring flags */
rlong(t_ustr.numcoms);    /* users number of commands done */
rlong(t_ustr.totl);       /* users total minutes online */
rtime(t_ustr.rawtime);    /* users last login in time_t format */

/* Read rest of values, too many to document here */
fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
         &t_ustr.monitor, &t_ustr.rows, &t_ustr.cols, &t_ustr.car_return,
         &t_ustr.abbrs, &t_ustr.times_on, &t_ustr.white_space, &t_ustr.aver,
         &t_ustr.autor, &t_ustr.autof, &t_ustr.automsgs,
         &t_ustr.gagcomm, &t_ustr.semail, &t_ustr.quote, &t_ustr.hilite,
         &t_ustr.new_mail, &t_ustr.color, &t_ustr.passhid);

fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
         &t_ustr.pbreak, &t_ustr.mail_num, &t_ustr.friend_num, &t_ustr.gag_num,
         &t_ustr.nerf_kills, &t_ustr.nerf_killed, &t_ustr.muz_time,
         &t_ustr.xco_time, &t_ustr.gag_time, &t_ustr.frog, &t_ustr.frog_time,
         &t_ustr.promote, &t_ustr.beeps, &t_ustr.mail_warn);

fscanf(f,"%d %d %d %d %d %d\n", &t_ustr.help, &t_ustr.who, &t_ustr.anchor,
       &t_ustr.anchor_time, &t_ustr.ttt_kills, &t_ustr.ttt_killed);

rbuf(t_ustr.webpic,HOME_LEN);       /* users url for picture */

if (feof(f)) goto END;
rval(t_ustr.revokes_num);	    /* number of revoked commands */

i=0;

/* Clear first ..Revokes.. line */
rbuf(buff1,-1);
strcpy(buff1,"");
    
for (;;) {
 rbuf(buff1,NAME_LEN);
 if (!strcmp(buff1,"..End revokes..")) break;
 else {
	if (!strcmp(buff1,"-1") || (strlen(buff1)<=3)) buff1[0]=0;
  strcpy(t_ustr.revokes[i],buff1);
  l=strlen(t_ustr.revokes[i]);
  t_ustr.revokes[i][l]=0;
  i++;
  }
 } 

for (l=i;l<MAX_GRAVOKES;++l) t_ustr.revokes[l][0]=0;

i=0;

rval(t_ustr.hang_wins);	    /* number of hangman wins */
rval(t_ustr.hang_losses);    /* number of hangman losses */

rbuf(t_ustr.icq,20);		/* icq number */
rbuf(t_ustr.miscstr1,10);	/* miscstr1 */
rbuf(t_ustr.miscstr2,10);	/* miscstr2 */
rbuf(t_ustr.miscstr3,10);	/* miscstr3 */
rbuf(t_ustr.miscstr4,10);	/* miscstr4 */
fscanf(f,"%d %d %d %d %d\n", &t_ustr.pause_login, &t_ustr.miscnum2,
       &t_ustr.miscnum3, &t_ustr.miscnum4, &t_ustr.miscnum5);

rbuf(buff1,-1);	/* ENDVER STRING */

/* add your own structures to read in here */


/* STOP adding your own structures to read in here */

END:
/*---------------------------------------------------------------------*/
/* check for possible bad values in the users config                   */
/*---------------------------------------------------------------------*/

/* longs */
if (t_ustr.numcoms > 10000000 || t_ustr.numcoms < 0)   t_ustr.numcoms = 1;
if (t_ustr.totl > 1439999    || t_ustr.totl < 0)        t_ustr.totl = 0;

/* ints, first set */
if (t_ustr.super > MAX_LEVEL || t_ustr.super < 0)      t_ustr.super = 0;
if (t_ustr.area > MAX_AREAS || t_ustr.area < 0)        t_ustr.area = 0;
if (t_ustr.shout > 1        || t_ustr.shout < 0)       t_ustr.shout = 1;
if (t_ustr.vis > 1          || t_ustr.vis < 0)         t_ustr.vis = 1;
if (t_ustr.locked > 1       || t_ustr.locked < 0)      t_ustr.locked = 0;
if (t_ustr.suspended > 1    || t_ustr.suspended < 0)   t_ustr.suspended = 0;

/* ints, second set */
if (t_ustr.monitor > 3      || t_ustr.monitor < 0)     t_ustr.monitor = 0;
if (t_ustr.rows > 256       || t_ustr.rows < 0)        t_ustr.rows = 24;
if (t_ustr.cols > 256       || t_ustr.cols < 0)        t_ustr.cols = 256;
if (t_ustr.car_return > 1   || t_ustr.car_return < 0)  t_ustr.car_return = 1;
if (t_ustr.abbrs > 1        || t_ustr.abbrs < 0)       t_ustr.abbrs = 0;
if (t_ustr.times_on > 32767 || t_ustr.times_on < 0)    t_ustr.times_on = 0;
if (t_ustr.white_space > 1  || t_ustr.white_space < 0) t_ustr.white_space = 0;
if (t_ustr.aver > 16000     || t_ustr.aver < 0)        t_ustr.aver = 16000;
if (t_ustr.autor > 3        || t_ustr.autor < 0)       t_ustr.autor = 0;
if (t_ustr.autof > 2        || t_ustr.autof < 0)       t_ustr.autof = 0;
if (t_ustr.automsgs > MAX_AUTOFORS        || t_ustr.automsgs < 0) t_ustr.automsgs = 0;
if (t_ustr.gagcomm > 1      || t_ustr.gagcomm < 0)     t_ustr.gagcomm = 0;
if (t_ustr.semail > 1       || t_ustr.semail < 0)      t_ustr.semail = 0;
if (t_ustr.quote > 1        || t_ustr.quote < 0)       t_ustr.quote = 0;
if (t_ustr.hilite > 2       || t_ustr.hilite < 0)      t_ustr.hilite = 0;
if (t_ustr.color > 1        || t_ustr.color < 0)       t_ustr.color = COLOR_DEFAULT;
if (t_ustr.passhid > 1      || t_ustr.passhid < 0)     t_ustr.passhid =0;

/* ints, third set */
if (t_ustr.pbreak > 1       || t_ustr.pbreak < 0)      t_ustr.pbreak =0;
if (t_ustr.beeps > 1        || t_ustr.beeps < 0)      t_ustr.beeps =0;
if (t_ustr.mail_warn > 1        || t_ustr.mail_warn < 0) t_ustr.mail_warn =0;
if (t_ustr.mail_num > 200   || t_ustr.mail_num < 0)    t_ustr.mail_num = 0;
if (t_ustr.friend_num > MAX_ALERT || t_ustr.friend_num < 0)  t_ustr.friend_num=0;
if (t_ustr.gag_num > MAX_GAG   || t_ustr.gag_num < 0)    t_ustr.gag_num =0;
if (t_ustr.revokes_num > MAX_GRAVOKES   || t_ustr.revokes_num < 0)	t_ustr.revokes_num =0;
if (t_ustr.nerf_kills > 32767     || t_ustr.nerf_kills < 0)       t_ustr.nerf_kills =0;
if (t_ustr.nerf_killed > 32767     || t_ustr.nerf_killed < 0)       t_ustr.nerf_killed =0;
if (t_ustr.muz_time > 32767     || t_ustr.muz_time < 0)       t_ustr.muz_time =0;
if (t_ustr.xco_time > 32767      || t_ustr.xco_time < 0)       t_ustr.xco_time =0;
if (t_ustr.gag_time > 32767      || t_ustr.gag_time < 0)  t_ustr.gag_time =0;
if (t_ustr.frog > 1      || t_ustr.frog < 0)       t_ustr.frog =0;
if (t_ustr.frog_time > 32767      || t_ustr.frog_time < 0)  t_ustr.frog_time =0;
if (t_ustr.anchor > 1      || t_ustr.anchor < 0)       t_ustr.anchor =0;
if (t_ustr.anchor_time > 32767      || t_ustr.anchor_time < 0) t_ustr.anchor_time =0;
if (t_ustr.promote > 22      || t_ustr.promote < 0)       t_ustr.promote=1;
if (t_ustr.ttt_kills > 32767     || t_ustr.ttt_kills < 0)       t_ustr.ttt_kills =0;
if (t_ustr.ttt_killed > 32767     || t_ustr.ttt_killed < 0)       t_ustr.ttt_killed =0;
if (t_ustr.hang_wins > 32767     || t_ustr.hang_wins < 0)       t_ustr.hang_wins =0;
if (t_ustr.hang_losses > 32767     || t_ustr.hang_losses < 0)       t_ustr.hang_losses =0;
if (t_ustr.pause_login > 32767     || t_ustr.pause_login < 0)	t_ustr.pause_login=0;
if (t_ustr.miscnum2 > 32767     || t_ustr.miscnum2 < 0)	t_ustr.miscnum2=0;
if (t_ustr.miscnum3 > 32767     || t_ustr.miscnum3 < 0)	t_ustr.miscnum3=0;
if (t_ustr.miscnum4 > 32767     || t_ustr.miscnum4 < 0)	t_ustr.miscnum4=0;
if (t_ustr.miscnum5 > 32767     || t_ustr.miscnum5 < 0)	t_ustr.miscnum5=0;

FCLOSE(f);
return 1;

}



/*----------------------------------------------------------------------*/
/* read the users data file to the online users strcuture               */
/*----------------------------------------------------------------------*/
int read_to_user(char *name, int user)
{
int i=0;
int l=0;
int num=0;
char buff1[ARR_SIZE];
char filename[FILE_NAME_LEN];
FILE *f;                 /* user file */
struct stat fileinfo;

buff1[0]=0;

if (!strlen(name)) return 0;

sprintf(t_mess,"%s/%s",USERDIR,name);
strncpy(filename,t_mess,FILE_NAME_LEN);

f = fopen (filename, "r"); /* open for output */
if (f == NULL)
  {
    return 0;
  }

stat(filename, &fileinfo);

if (fileinfo.st_size == 0) {
 fclose(f);
 remove(filename);
 sprintf(t_mess,"Found 0 length user file for %s..removing.\n",name);
 print_to_syslog(t_mess);
 return -1;
 }


/*--------------------------------------------------------*/
/* values added after initial release must be initialized */
/*--------------------------------------------------------*/
ustr[user].numcoms       = 0;
ustr[user].totl          = 0;
ustr[user].rawtime       = 0;

ustr[user].monitor       = 0;
ustr[user].rows          = 24;
ustr[user].cols          = 256;
ustr[user].car_return    = 1;
ustr[user].abbrs         = 1;
ustr[user].white_space   = 1;
ustr[user].times_on      = 1;
ustr[user].aver          = 0;
ustr[user].autor         = 0;
ustr[user].autof         = 0;
ustr[user].automsgs      = 0;
ustr[user].gagcomm       = 0;
ustr[user].semail        = 0;
ustr[user].quote         = 1;
ustr[user].hilite        = 0;
ustr[user].new_mail      = 0;
ustr[user].color         = COLOR_DEFAULT;
ustr[user].passhid       = 0;

ustr[user].pbreak        = 0;
ustr[user].beeps         = 0;
ustr[user].mail_warn     = 0;
ustr[user].mail_num      = 0;
ustr[user].friend_num    = 0;
ustr[user].revokes_num   = 0;
ustr[user].gag_num       = 0;
ustr[user].nerf_kills    = 0;
ustr[user].nerf_killed   = 0;
ustr[user].muz_time      = 0;
ustr[user].xco_time      = 0;
ustr[user].gag_time      = 0;
ustr[user].frog          = 0;
ustr[user].frog_time     = 0;
ustr[user].anchor        = 0;
ustr[user].anchor_time   = 0;
ustr[user].promote       = 0;
ustr[user].help          = 0;
ustr[user].who           = 0;
ustr[user].ttt_kills     = 0;
ustr[user].ttt_killed    = 0;
ustr[user].hang_wins     = 0;
ustr[user].hang_losses   = 0;
ustr[user].pause_login   = 0;
ustr[user].miscnum2      = 0;
ustr[user].miscnum3      = 0;
ustr[user].miscnum4      = 0;
ustr[user].miscnum5      = 0;
listen_all(-1);

/* first line is either version number or users name */
rbuf(buff1,NAME_LEN);
if (strstr(buff1,".ver")) {
	/* this is a standardized file format */
	/* now check for version difference */
	if (!strcmp(buff1,UDATA_VERSION)) {
	  /* no difference, continue on reading */
	  rbuf(ustr[user].name,NAME_LEN);            /* users name */
	}
	else {
	  /* version difference, lets try and convert */
	  if (!convert_file(f,filename,0)) { return 0; }
	  /* reopen file after the conversion and continue on */
	  /* convert_file() closes the file		      */
	  f = fopen (filename, "r"); /* open for output */
		if (f == NULL)
		  {
		    return 0;
		  }

	  rbuf(buff1,NAME_LEN); /* VERSION */
	  rbuf(ustr[user].name,NAME_LEN); /* users name */
	} /* end of else */
  }
else {
	/* old data file format */
	/* we need to convert to new format */
	if (!convert_file(f,filename,1)) { return 0; }
	/* reopen file after the conversion and continue on */
	/* convert_file() closes the file		    */
	f = fopen (filename, "r"); /* open for output */
		if (f == NULL)
		  {
		    return 0;
		  }
	rbuf(buff1,NAME_LEN);		       /* VERSION */
	rbuf(ustr[user].name,NAME_LEN);            /* users name */
  }

rbuf(ustr[user].say_name,NAME_LEN);       /* users properly capitalized name */
rbuf(ustr[user].password,-1);             /* users encrypted password */
rval(ustr[user].super);                   /* users level or rank */
rbuf(ustr[user].email_addr,EMAIL_LENGTH); /* users email address */
rbuf(ustr[user].desc,DESC_LEN);           /* users description */
rbuf(ustr[user].sex,32);                  /* users gender */
rbuf(ustr[user].init_date,25);            /* users original login time */
rbuf(ustr[user].last_date,25);            /* users last login time */
rbuf(ustr[user].init_site,21);            /* users original site */
rbuf(ustr[user].last_site,21);            /* users last site */
rbuf(ustr[user].last_name,64);            /* users last hostname */
rbuf(ustr[user].init_netname,64);         /* users original hostname */

/* Clear first ..Abbrs.. line */
rbuf(buff1,-1);
strcpy(buff1,"");

for (;;) {
 rbuf(buff1,20);
 if (!strcmp(buff1,"..End abbrs..")) break;
  if (i) {
    strcpy(ustr[user].custAbbrs[l].com,buff1);
    i=0; l++; continue;
   }
  else {
    strcpy(ustr[user].custAbbrs[l].abbr,buff1);
    i=1;
   }
 }

i=0;
l=0;
  
/* Clear first ..Macros.. line */
rbuf(buff1,-1);
strcpy(buff1,"");
  
for (;;) {
 rbuf(buff1,MACRO_LEN);
 if (!strcmp(buff1,"..End macros..")) break;
 if (i) {
  strcpy(ustr[user].Macros[num].body,buff1);
  l=strlen(ustr[user].Macros[num].body);
  ustr[user].Macros[num].body[l]=0;
  i=0; num++; continue;
  }
 else {
  strcpy(ustr[user].Macros[num].name,buff1);
  l=strlen(ustr[user].Macros[num].name);
  ustr[user].Macros[num].name[l]=0;
  i=1;
  }
 }

for (i=num;i<NUM_MACROS;++i) {
 ustr[user].Macros[i].name[0]=0;
 ustr[user].Macros[i].body[0]=0;
 }

i=0;
num=0;

/* Clear first ..Friends.. line */
rbuf(buff1,-1);
strcpy(buff1,"");
    
for (;;) {
 rbuf(buff1,NAME_LEN);
 if (!strcmp(buff1,"..End friends..")) break;
 else {
  strcpy(ustr[user].friends[i],buff1);
  l=strlen(ustr[user].friends[i]);
  ustr[user].friends[i][l]=0;
  i++;
  }
 }  

for (l=i;l<MAX_ALERT;++l) ustr[user].friends[l][0]=0;

i=0;

/* Clear first ..Gagged.. line */
rbuf(buff1,-1);
strcpy(buff1,"");
    
for (;;) {
 rbuf(buff1,NAME_LEN);
 if (!strcmp(buff1,"..End gagged..")) break;
 else {
  strcpy(ustr[user].gagged[i],buff1);
  l=strlen(ustr[user].gagged[i]);
  ustr[user].gagged[i][l]=0;
  i++;
  }
 }  

for (l=i;l<MAX_GAG;++l) ustr[user].gagged[l][0]=0;

/*----------------------------------------*/
/*  users last area in and will login to  */
/*  users muzzled or not                  */
/*  users visible or not                  */
/*  users locked or not                   */
/*  users xcommed or not                  */
/*----------------------------------------*/
fscanf(f, "%d %d %d %d %d\n", &ustr[user].area, &ustr[user].shout,
         &ustr[user].vis, &ustr[user].locked, &ustr[user].suspended);

rbuf(ustr[user].entermsg,MAX_ENTERM);   /* users room enter message */
rbuf(ustr[user].exitmsg,MAX_EXITM);     /* users room exit message */
rbuf(ustr[user].home_room,NAME_LEN);    /* users home room */
rbuf(ustr[user].fail,MAX_ENTERM);       /* users fail message */
rbuf(ustr[user].succ,MAX_ENTERM);       /* users success message */
rbuf(ustr[user].homepage,HOME_LEN);     /* users homepage */
rbuf(ustr[user].creation,25);           /* users creation date */
rbuf(ustr[user].security,MAX_AREAS);    /* users room permissions */

i=0;
if (MAX_AREAS > strlen(ustr[user].security)) {
 for (i=0;i<(MAX_AREAS-strlen(ustr[user].security));++i)
  strcat(ustr[user].security,"N");
 }
i=0;

rbuf(ustr[user].flags,NUM_IGN_FLAGS+2); /* users listening and ignoring flags */
rlong(ustr[user].numcoms);              /* users number of commands done */
rlong(ustr[user].totl);                 /* users total minutes online */
rtime(ustr[user].rawtime);              /* users last login in time_t format */

/* Read rest of values, too many to document here */
fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
         &ustr[user].monitor, &ustr[user].rows, &ustr[user].cols,
         &ustr[user].car_return, &ustr[user].abbrs, &ustr[user].times_on,
         &ustr[user].white_space, &ustr[user].aver, &ustr[user].autor,
         &ustr[user].autof, &ustr[user].automsgs, &ustr[user].gagcomm,
         &ustr[user].semail, &ustr[user].quote, &ustr[user].hilite,
         &ustr[user].new_mail, &ustr[user].color, &ustr[user].passhid);

fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
         &ustr[user].pbreak, &ustr[user].mail_num, &ustr[user].friend_num,
         &ustr[user].gag_num, &ustr[user].nerf_kills, &ustr[user].nerf_killed,
         &ustr[user].muz_time, &ustr[user].xco_time, &ustr[user].gag_time,
         &ustr[user].frog, &ustr[user].frog_time, &ustr[user].promote,
         &ustr[user].beeps, &ustr[user].mail_warn);

fscanf(f,"%d %d %d %d %d %d\n",&ustr[user].help,&ustr[user].who,
&ustr[user].anchor,&ustr[user].anchor_time,&ustr[user].ttt_kills,&ustr[user].ttt_killed);

rbuf(ustr[user].webpic,HOME_LEN);       /* users url for picture */

if (feof(f)) goto END;
rval(ustr[user].revokes_num);		/* number of revoked commands */

i = 0;

/* Clear first ..Revokes.. line */
rbuf(buff1,-1);
strcpy(buff1,"");

for (;;) {
 rbuf(buff1,NAME_LEN);
 if (!strcmp(buff1,"..End revokes..")) break;
 else {
	if (!strcmp(buff1,"-1") || (strlen(buff1)<=3)) buff1[0]=0;
  strcpy(ustr[user].revokes[i],buff1);
  l=strlen(ustr[user].revokes[i]);
  ustr[user].revokes[i][l]=0;
  i++;
  }
 }

for (l=i;l<MAX_GRAVOKES;++l) ustr[user].revokes[l][0]=0;

i=0;

rval(ustr[user].hang_wins);		/* number of hangman wins */
rval(ustr[user].hang_losses);		/* number of hangman losses */

rbuf(ustr[user].icq,20);            /* icq number */
rbuf(ustr[user].miscstr1,10);       /* miscstr1 */
rbuf(ustr[user].miscstr2,10);       /* miscstr2 */
rbuf(ustr[user].miscstr3,10);       /* miscstr3 */
rbuf(ustr[user].miscstr4,10);       /* miscstr4 */
fscanf(f,"%d %d %d %d %d\n", &ustr[user].pause_login, &ustr[user].miscnum2,       
       &ustr[user].miscnum3, &ustr[user].miscnum4, &ustr[user].miscnum5);

rbuf(buff1,-1);	/* ENDVER STRING */

/* add your own structures to read in here */


/* STOP adding your own structures to read in here */

END:
/*---------------------------------------------------------------------*/
/* check for possible bad values in the users config                   */
/*---------------------------------------------------------------------*/

/* longs */
if (ustr[user].numcoms > 10000000 || ustr[user].numcoms < 0) ustr[user].numcoms = 1;
if (ustr[user].totl > 1439999    || ustr[user].totl < 0) ustr[user].totl = 0;

/* ints, first set */
if (ustr[user].super > MAX_LEVEL || ustr[user].super < 0)      ustr[user].super = 0;
if (ustr[user].area > MAX_AREAS || ustr[user].area < 0)        ustr[user].area = 0;
if (ustr[user].shout > 1        || ustr[user].shout < 0)       ustr[user].shout = 0;
if (ustr[user].vis > 1          || ustr[user].vis < 0)         ustr[user].vis = 0;
if (ustr[user].locked > 1       || ustr[user].locked < 0)      ustr[user].locked = 0;
if (ustr[user].suspended > 1    || ustr[user].suspended < 0)   ustr[user].suspended = 0;

/* ints, second set */
if (ustr[user].monitor > 3      || ustr[user].monitor < 0)     ustr[user].monitor = 0;
if (ustr[user].rows > 256       || ustr[user].rows < 0)        ustr[user].rows = 24;
if (ustr[user].cols > 256       || ustr[user].cols < 0)        ustr[user].cols = 256;
if (ustr[user].car_return > 1   || ustr[user].car_return < 0)  ustr[user].car_return = 1;
if (ustr[user].abbrs > 1        || ustr[user].abbrs < 0)       ustr[user].abbrs = 0;
if (ustr[user].times_on > 32767 || ustr[user].times_on < 0)    ustr[user].times_on = 0;
if (ustr[user].white_space > 1  || ustr[user].white_space < 0) ustr[user].white_space = 0;
if (ustr[user].aver > 16000     || ustr[user].aver < 0)        ustr[user].aver = 16000;
if (ustr[user].autor > 3        || ustr[user].autor < 0)       ustr[user].autor = 0;
if (ustr[user].autof > 2        || ustr[user].autof < 0)       ustr[user].autof = 0;
if (ustr[user].automsgs > MAX_AUTOFORS  || ustr[user].automsgs < 0) ustr[user].automsgs = 0;
if (ustr[user].gagcomm > 1      || ustr[user].gagcomm < 0)     ustr[user].gagcomm = 0;
if (ustr[user].semail > 1       || ustr[user].semail < 0)      ustr[user].semail = 0;
if (ustr[user].quote > 1        || ustr[user].quote < 0)       ustr[user].quote = 0;
if (ustr[user].hilite > 2       || ustr[user].hilite < 0)      ustr[user].hilite = 0;
if (ustr[user].color > 1        || ustr[user].color < 0)       ustr[user].color = COLOR_DEFAULT;
if (ustr[user].passhid > 1      || ustr[user].passhid < 0)     ustr[user].passhid = 0; 

/* ints, third set */
if (ustr[user].pbreak > 1       || ustr[user].pbreak < 0)      ustr[user].pbreak = 0; 
if (ustr[user].beeps > 1        || ustr[user].beeps < 0)       ustr[user].beeps = 0; 
if (ustr[user].mail_warn > 1    || ustr[user].mail_warn < 0) ustr[user].mail_warn = 0;
if (ustr[user].mail_num > 200   || ustr[user].mail_num < 0)    ustr[user].mail_num = 0;
if (ustr[user].friend_num > MAX_ALERT || ustr[user].friend_num < 0 ) ustr[user].friend_num=0;
if (ustr[user].gag_num > MAX_GAG || ustr[user].gag_num < 0) ustr[user].gag_num =0;
if (ustr[user].revokes_num > MAX_GRAVOKES || ustr[user].revokes_num < 0)	ustr[user].revokes_num =0;
if (ustr[user].nerf_kills > 32767 || ustr[user].nerf_kills < 0)  ustr[user].nerf_kills =0;
if (ustr[user].nerf_killed > 32767 || ustr[user].nerf_killed < 0)  ustr[user].nerf_killed =0;
if (ustr[user].muz_time > 32767 || ustr[user].muz_time < 0)  ustr[user].muz_time =0;
if (ustr[user].xco_time > 32767 || ustr[user].xco_time < 0)   ustr[user].xco_time =0;
if (ustr[user].gag_time > 32767 || ustr[user].gag_time < 0)   ustr[user].gag_time =0;
if (ustr[user].frog > 1 || ustr[user].frog < 0)    ustr[user].frog =0;
if (ustr[user].frog_time > 32767 || ustr[user].frog_time < 0)  ustr[user].frog_time =0;
if (ustr[user].anchor > 1 || ustr[user].anchor < 0)    ustr[user].anchor =0;
if (ustr[user].anchor_time > 32767 || ustr[user].anchor_time < 0) ustr[user].anchor_time =0;
if (ustr[user].promote > 22 || ustr[user].promote < 0)  ustr[user].promote=1;
if (ustr[user].help > 3 || ustr[user].help < 0)  ustr[user].help=0;
if (ustr[user].who > 4 || ustr[user].who < 0)  ustr[user].who=0;
if (ustr[user].ttt_kills > 32767 || ustr[user].ttt_kills < 0)  ustr[user].ttt_kills =0;
if (ustr[user].ttt_killed > 32767 || ustr[user].ttt_killed < 0)  ustr[user].ttt_killed =0;
if (ustr[user].hang_wins > 32767 || ustr[user].hang_wins < 0)  ustr[user].hang_wins =0;
if (ustr[user].hang_losses > 32767 || ustr[user].hang_losses < 0)  ustr[user].hang_losses =0;
if (ustr[user].pause_login  > 32767 || ustr[user].pause_login < 0)  ustr[user].pause_login=0;
if (ustr[user].miscnum2  > 32767 || ustr[user].miscnum2 < 0) ustr[user].miscnum2=0;
if (ustr[user].miscnum3  > 32767 || ustr[user].miscnum3 < 0) ustr[user].miscnum3=0;
if (ustr[user].miscnum4  > 32767 || ustr[user].miscnum4 < 0) ustr[user].miscnum4=0;
if (ustr[user].miscnum5  > 32767 || ustr[user].miscnum5 < 0) ustr[user].miscnum5=0;

FCLOSE(f);
return 1;
}

/*----------------------------------------------------------------------*/
/* write a user temp buffer to a users data file                        */
/*----------------------------------------------------------------------*/
void write_user(char *name)
{
int i=0;
FILE *f;                 /* user file*/
char filename[FILE_NAME_LEN];
char buff1[25];

buff1[0]=0;

sprintf(t_mess,"%s/%s",USERDIR,name);
strncpy(filename,t_mess,FILE_NAME_LEN);

f = fopen (filename, "w"); /* open for output */

if (f==NULL)
  {
   sprintf(mess,"Cant write to user data file %s! (%s)\n",filename,strerror(errno));
   print_to_syslog(mess);
   return;
  }

wbuf(UDATA_VERSION);	  /* FILE VERSION */
wbuf(t_ustr.name);        /* users name */
wbuf(t_ustr.say_name);    /* users properly capitalized name */
wbuf(t_ustr.password);    /* users excrypted password */
wval(t_ustr.super);       /* users level or rank */
wbuf(t_ustr.email_addr);  /* users email address */
wbuf(t_ustr.desc);        /* users description */
wbuf(t_ustr.sex);         /* users gender */
wbuf(t_ustr.init_date);   /* users original login time */
wbuf(t_ustr.last_date);   /* users last login time */
wbuf(t_ustr.init_site);   /* users original site */
wbuf(t_ustr.last_site);   /* users last site */
wbuf(t_ustr.last_name);   /* users last hostname */
wbuf(t_ustr.init_netname);   /* users original hostname */

/*
  while (strlen(t_ustr.custAbbrs[i].com) > 1) {  
    wbuf(t_ustr.custAbbrs[i].abbr);
    wbuf(t_ustr.custAbbrs[i].com);
	sprintf(mess,"Abbr %d: %s\n",i,t_ustr.custAbbrs[i].abbr);
	print_to_syslog(mess);
	sprintf(mess,"Com  %d: %s\n",i,t_ustr.custAbbrs[i].com);
	print_to_syslog(mess);
    i++;
   }
*/

   wbuf("..Abbrs..");
   for (i=0;i<NUM_ABBRS;++i) {
    wbuf(t_ustr.custAbbrs[i].abbr);
    wbuf(t_ustr.custAbbrs[i].com);
   }
   wbuf("..End abbrs..");
i=0;
   wbuf("..Macros..");
   for (i=0;i<NUM_MACROS;i++) {
    wbuf(t_ustr.Macros[i].name);
    wbuf(t_ustr.Macros[i].body);
    }
   wbuf("..End macros..");
i=0;
   wbuf("..Friends..");
   for (i=0;i<MAX_ALERT;i++) wbuf(t_ustr.friends[i]);
   wbuf("..End friends..");
i=0;
   wbuf("..Gagged..");
   for (i=0;i<MAX_GAG;i++) wbuf(t_ustr.gagged[i]);
   wbuf("..End gagged..");
i=0;

/*----------------------------------------*/
/*  users last area in and will login to  */
/*  users muzzled or not                  */
/*  users visible or not                  */
/*  users locked or not                   */
/*  users xcommed or not                  */
/*----------------------------------------*/
fprintf(f, "%d %d %d %d %d\n", t_ustr.area, t_ustr.shout,
           t_ustr.vis, t_ustr.locked, t_ustr.suspended);

wbuf(t_ustr.entermsg);    /* users room enter message */
wbuf(t_ustr.exitmsg);     /* users room exit message */
wbuf(t_ustr.home_room);   /* users home room */
wbuf(t_ustr.fail);        /* users fail message */
wbuf(t_ustr.succ);        /* users success message */
wbuf(t_ustr.homepage);    /* users homepage */
wbuf(t_ustr.creation);    /* users creation date */
wbuf(t_ustr.security);    /* users room permissions */
wbuf(t_ustr.flags);       /* users listening and ignoring flags */
wlong(t_ustr.numcoms);    /* users number of commands done */
wlong(t_ustr.totl);       /* users total minutes online */
wtime(t_ustr.rawtime);    /* users last login in time_t format */

/* Read rest of values, too many to document here */
fprintf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
          t_ustr.monitor, t_ustr.rows, t_ustr.cols, t_ustr.car_return,
          t_ustr.abbrs, t_ustr.times_on, t_ustr.white_space, t_ustr.aver,
          t_ustr.autor, t_ustr.autof, t_ustr.automsgs,
          t_ustr.gagcomm, t_ustr.semail, t_ustr.quote, t_ustr.hilite,
          t_ustr.new_mail, t_ustr.color, t_ustr.passhid);

fprintf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
          t_ustr.pbreak, t_ustr.mail_num, t_ustr.friend_num, t_ustr.gag_num,
          t_ustr.nerf_kills, t_ustr.nerf_killed, t_ustr.muz_time,
          t_ustr.xco_time, t_ustr.gag_time, t_ustr.frog, t_ustr.frog_time,
          t_ustr.promote, t_ustr.beeps, t_ustr.mail_warn);

fprintf(f,"%d %d %d %d %d %d\n",t_ustr.help, t_ustr.who, t_ustr.anchor,
          t_ustr.anchor_time, t_ustr.ttt_kills, t_ustr.ttt_killed);

wbuf(t_ustr.webpic);       /* users url for picture */

wval(t_ustr.revokes_num);	/* number of revoked commands */

i=0;
   wbuf("..Revokes..");
   for (i=0;i<MAX_GRAVOKES;i++) wbuf(t_ustr.revokes[i]);
   wbuf("..End revokes..");

wval(t_ustr.hang_wins);		/* number of hangman wins */
wval(t_ustr.hang_losses);	/* number of hangman losses */

wbuf(t_ustr.icq);            /* icq number */
wbuf(t_ustr.miscstr1);       /* miscstr1 */
wbuf(t_ustr.miscstr2);       /* miscstr2 */
wbuf(t_ustr.miscstr3);       /* miscstr3 */
wbuf(t_ustr.miscstr4);       /* miscstr4 */
fprintf(f,"%d %d %d %d %d\n", t_ustr.pause_login, t_ustr.miscnum2,       
       t_ustr.miscnum3, t_ustr.miscnum4, t_ustr.miscnum5);      

/* tack on marker */
sprintf(buff1,"--ENDVER %s",UDATA_VERSION);
wbuf(buff1);

/* add your own structures to write out, here */


/* STOP adding your own structures to write out, here */

FCLOSE(f);
return;

}


/*-------------------------------------------------------------------------*/
/* check to see if the user exists                                         */
/*-------------------------------------------------------------------------*/

int check_for_user(char *name)
{
char filename[ARR_SIZE];
FILE * bfp;

sprintf(filename,"%s/%s",USERDIR,name);
bfp = fopen(filename, "r");

if (bfp) 
  {
    FCLOSE(bfp);     
    return 1;
  }
return 0;
}

/*----------------------------------------*/
/* check to see if the file exists        */
/*----------------------------------------*/
int check_for_file(char *name)
{
char filename[FILE_NAME_LEN];
FILE * bfp;

sprintf(filename,"%s",name);
bfp = fopen(filename, "r");

if (bfp) 
  {
    FCLOSE(bfp);     
    return 1;
  }
return 0;
}

/*-------------------------------------------------------------------------*/
/* remove a user                                                           */
/*-------------------------------------------------------------------------*/
void remove_user(char *name)
{
char filename[FILE_NAME_LEN];

         /* Remove user's data file */
        sprintf(t_mess,"%s/%s",USERDIR,name);
        strncpy(filename,t_mess,FILE_NAME_LEN);
        remove(filename);

         /* Remove user's INBOX mail file */
        sprintf(t_mess,"%s/%s", MAILDIR, name);
        strncpy(filename, t_mess, FILE_NAME_LEN);
        remove(filename);

         /* Remove user's SENT mail file */
        sprintf(t_mess,"%s/%s.sent", MAILDIR, name);
        strncpy(filename, t_mess, FILE_NAME_LEN);
        remove(filename);

         /* Remove user's EMAIL mail file */
        sprintf(t_mess,"%s/%s.email", MAILDIR, name);
        strncpy(filename, t_mess, FILE_NAME_LEN);
        remove(filename);

         /* Remove user's PROFILE file */
        sprintf(t_mess,"%s/%s",PRO_DIR, name);
        strncpy(filename, t_mess, FILE_NAME_LEN);
        remove(filename);

} 


/*-----------------------------------------------------*/
/* Read a string with return from a file               */
/*-----------------------------------------------------*/
void getbuf(FILE *f, char *buf2, int buflen)
{
char temp[1000];

temp[0]=0;
if (fgets((char *)temp,1000,f) == NULL) {
	return;
	}

temp[strlen(temp)-1]=0;

if (buflen != -1) {
  if (strlen(temp) >= buflen)
    temp[buflen-1]=0;
 }

strcpy(buf2,temp);
}

/*-----------------------------------------------------*/
/* Read an int with return from a file                 */
/*-----------------------------------------------------*/
int getint(FILE *f)
{
int val1=0;
char temp[1000];

temp[0]=0;
if (fgets((char *)temp,1000,f) == NULL) {
	return 0;
	}

temp[strlen(temp)-1]=0;
sscanf(temp,"%d",&val1);
return val1;
}

/*-----------------------------------------------------*/
/* Read a long value with return from a file           */
/*-----------------------------------------------------*/
long getlong(FILE *f)
{
long val2=0;
char temp[1000];

temp[0]=0;
if (fgets((char *)temp,1000,f) == NULL) {
	return 0;
	}

temp[strlen(temp)-1]=0;
sscanf(temp,"%ld",&val2);
return val2;
}

/*-----------------------------------------------------*/
/* Put a string with return to a file                  */
/*-----------------------------------------------------*/
void putbuf(FILE *f, char *buf2)
{
fputs(buf2,f);
fputs("\n",f);
}


/*--------------------------------------------------------------------*/
/* a simple attempt to encrypt a password                             */
/*--------------------------------------------------------------------*/

void st_crypt(char str[])
{
int i = 0;
char last = ' ';

while(str[i])
  {str[i]= (( (str[i] - 32) + lock[(i%num_locks)] + (last - 32) ) % 94) + 32;
   last = str[i];
   i++;
   }
}



/*-----------------------------------------------------------------------*/
/* the preview command                                                   */
/*-----------------------------------------------------------------------*/
void preview(int user, char *inpstr)
{
int num=0;
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
char buffer[ARR_SIZE];
char line[257];
FILE *fp;
FILE *pp;

if (!strlen(inpstr)) 
  {
   strcpy(inpstr,"names"); 
   }

inpstr[80]=0;
        
/* plug security hole */
if (check_fname(inpstr,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

sscanf(inpstr,"%s ",buffer);
remove_first(inpstr);
/* open board file */
sprintf(t_mess,"%s/%s",PICTURE_DIR,buffer);
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!check_for_file(filename)) {
    write_str(user,"Picture does not exist.");
    return;
    }

 if (!strlen(inpstr)) {
    cat(filename,user,0);
  }
 else {
  for (num=0;num<strlen(inpstr);num++) {
  if (!isdigit((int)inpstr[num])) {
     write_str(user,"Lines from bottom must be a number.");
     return;
     }
  }
  num=0;
  num=atoi(inpstr);
  strcpy(filename2,get_temp_file());

  sprintf(mess,"tail -%d %s",num,filename);

 if (!(pp=popen(mess,"r"))) {
	write_str(user,"Can't open pipe to get those picture lines!");
	return;
	}
 if (!(fp=fopen(filename2,"w"))) {
	write_str(user,"Can't open temp file for writing!");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	fputs(line,fp);
      } /* end of while */
fclose(fp);
pclose(pp);

  cat(filename2,user,0);
  return;
 }

}

/*-----------------------------------------------------------------------*/
/* the picture command                                                   */
/*-----------------------------------------------------------------------*/
void picture(int user, char *inpstr)
{
int num=0;
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
char buffer[ARR_SIZE];
char name[NAME_LEN+1], z_mess[NAME_LEN+20];
char line[257];
FILE *fp;
FILE *pp;

if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

if (!strlen(inpstr)) 
  {
   strcpy(inpstr,"names");
   preview(user, inpstr);
   return;
  }
        
inpstr[80]=0;

/* plug security hole */
if (check_fname(inpstr,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

sscanf(inpstr,"%s ",buffer);
if (strlen(buffer) < 2) return;

remove_first(inpstr);  
sprintf(t_mess,"%s/%s",PICTURE_DIR,buffer);
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!check_for_file(filename)) 
  {
   write_str(user,"Picture does not exist.");
   return;
  }

 if (!strlen(inpstr)) {
  strcpy(name,ustr[user].say_name);
  sprintf(z_mess,"(%s) shows:",name);
  writeall_str(z_mess, 1, user, 0, user, NORM, PICTURE, 0);
  catall(user,filename);
  }
 else {
  for (num=0;num<strlen(inpstr);num++) {
  if (!isdigit((int)inpstr[num])) {
     write_str(user,"Lines from bottom must be a number.");
     return;
     }
  }
  num=0;
  num=atoi(inpstr);
  strcpy(filename2,get_temp_file());

  sprintf(mess,"tail -%d %s",num,filename);

 if (!(pp=popen(mess,"r"))) {
	write_str(user,"Can't open pipe to get those picture lines!");
	return;
	}
 if (!(fp=fopen(filename2,"w"))) {
	write_str(user,"Can't open temp file for writing!");
	return;
	}
while (fgets(line,256,pp) != NULL) {
	fputs(line,fp);
      } /* end of while */
fclose(fp);
pclose(pp);

  strcpy(name,ustr[user].say_name);
  sprintf(z_mess,"(%s) shows:",name);
  writeall_str(z_mess, 1, user, 0, user, NORM, PICTURE, 0);
  catall(user,filename2);
  return;
 }

}



/*-------------------------------------------------------------*/
/* change a password                                           */
/*-------------------------------------------------------------*/
void password(int user, char *pword)
{

 if (pword[0]<32 || strlen(pword)< 3) 
    {
     write_str(user,SYS_PASSWD_INVA);  
     return;
    }
        
  if (strlen(pword)>NAME_LEN-1) 
    {
     write_str(user,SYS_PASSWD_LONG);  
     return;
    }

  if (strstr(pword,"^")) {
     write_str(user,"Password cant have color or hilite codes in it.");
     return;
     }

  if (strstr(pword," ")) {
     write_str(user,"Password cant have spaces in it.");
     write_str(user,"Syntax: .password <new_password>");
     return;
     }

  /*-------------------------------------------------------------*/
  /* convert name & passwd to lowercase and encrypt the password */
  /*-------------------------------------------------------------*/
  
  strtolower(pword);
  
  if (strcmp(ustr[user].login_name, pword) ==0)
    {
        write_str(user,"\nPassword cannot be the login name. \nPassword not changed."); 
        return;  
    }
   
  st_crypt(pword);                                   
  strcpy(ustr[user].password,pword);   
  copy_from_user(user);
  write_user(ustr[user].name);
  write_str(user,"Password is now changed.");
} 

/*-------------------------------------------------------------*/
/* toggle-monitoring                                           */
/*-------------------------------------------------------------*/
void tog_monitor(int user)
{

 if (ustr[user].monitor==3)
   {
    ustr[user].monitor=0;
    write_str(user," *** Monitoring is now totally off for you ***");
   }
 else if (ustr[user].monitor==2)
   {
    ustr[user].monitor=3;
    write_str(user," *** Monitoring is now on for users and incoming logins ***");
   }
 else if (ustr[user].monitor==1)
   {
    ustr[user].monitor=2;
    write_str(user," *** Monitoring is now on for incoming logins ***");
   }
 else if (ustr[user].monitor==0)
   {
    ustr[user].monitor=1;
    write_str(user," *** Monitoring is now on for users ***");
   }
 write_str(user," *** .monitor again for more options ***");

copy_from_user(user);
write_user(ustr[user].name);
} 

/*-----------------------------------------------------------*/
/* check file name for hack                                  */
/*-----------------------------------------------------------*/
int check_fname(char *inpstr, int user)
{
if (strpbrk(inpstr,".$/+*[]\\") )
  { 
   sprintf(mess,"User %s: illegal file: %s",ustr[user].say_name,inpstr);   
   logerror(mess);
   return(1);
  }
return 0;
}

/*--------------------------------------------------------------*/
/* display file to all in a room                                */
/*--------------------------------------------------------------*/
void catall(int user, char *filename)
{
FILE *fp;

if (!(fp=fopen(filename,"r"))) 
  {
   return;
  }
                                
/* jump to reading posn in file */
  fseek(fp,0,0);

/* loop until end of file or end of page reached */
strcpy(mess," ");
while(!feof(fp)) 
   {
     writeall_str(mess, 1, user, 1, user, NORM, PICTURE, 0);
     fgets(mess,sizeof(mess)-1,fp);
     mess[strlen(mess)-1]=0;
   }
   
FCLOSE(fp);
return;
}

/*----------------------------------------------------------------------*/
/* Add or remove security clearance for a user for a room               */
/*----------------------------------------------------------------------*/
void permission_u(int user, char *inpstr)
{
int u,a,b,j=1;
char permit;
char permission[81];
char other_user[ARR_SIZE];
char room[ARR_SIZE];
char filename[FILE_NAME_LEN];
FILE *fp;

if (!strlen(inpstr)) {
   write_str(user,"Usage: .permission [add|sub] <user> <room>"); 
   write_str(user,"Leave <room> blank for permission list"); 
   return;
   }
sscanf(inpstr,"%s ",permission);
strtolower(permission);

if (!strcmp("add",permission))
  {
   permit = 'Y';
  }
else if (!strcmp("sub",permission))
  {
   permit = 'N';
  }
else
  {
   write_str(user,"Option does not exist.");
   return;
  }

remove_first(inpstr);
 
if (!strlen(inpstr)) {
   write_str(user,"Security clear who?"); 
   return;
   }
 
sscanf(inpstr,"%s ",other_user);
strtolower(other_user);
CHECK_NAME(other_user);

remove_first(inpstr);

/* List room permissions */   
if (!strlen(inpstr))
  {

    if (!read_user(other_user))
     {
      write_str(user,NO_USER_STR);
      return;
     }

 strcpy(filename,get_temp_file());
 if (!(fp=fopen(filename,"w"))) {
     sprintf(mess,"%s Cant create file for paged permission listing!",get_time(0,0));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     return;
     }

    fputs("---------------------------------------------------------\n",fp);
    sprintf(mess," Room permissions for ^%s^\n\n",t_ustr.say_name);
    fputs(mess,fp);
    fputs("---------------------------------------------------------\n",fp);
    fputs("ALLOWED ROOMS:\n",fp);

    for (a=0;a<NUM_AREAS;++a) 
      {
	if (t_ustr.security[a]=='Y' && strlen(astr[a].name))
	  {
	   sprintf(mess,"%*s ",ROOM_LEN,astr[a].name);
	   fputs(mess,fp);
	  }
	if (!(j++%3) )
           {
            j = 1;
            fputs("\n",fp);
	   }
      } /* end of for */
if (j<=3) fputs("\n",fp);
j=1;
a=0;

    fputs("DENIED ROOMS:\n",fp);
    for (a=0;a<NUM_AREAS;++a) 
      {
	if (t_ustr.security[a]=='N' && strlen(astr[a].name))
	  {
	   sprintf(mess,"%*s ",ROOM_LEN,astr[a].name);
	   fputs(mess,fp);
	  }
	if (!(j++%3) )
           {
            j = 1;
            fputs("\n",fp);
	   }
      } /* end of for */
if (j<=3) fputs("\n",fp);
   fputs("\n",fp);
   fclose(fp);

 if (!cat(filename,user,0)) {
     sprintf(mess,"%s Cant cat permission listing file!",get_time(0,0));
     write_str(user,mess);
     strcat(mess,"\n");
     print_to_syslog(mess);
     }

   a=0; j=1;
   return;
  }     /* end of if room list */

sscanf(inpstr,"%s ",room);
strtolower(room);

if (!read_user(other_user))
  {
   write_str(user,NO_USER_STR);
   return;
  }

b= -1;

/* read in descriptions and joinings */
for (a=0;a<NUM_AREAS;++a) 
  {
    if (!strcmp(room,astr[a].name))
      {
       t_ustr.security[a]=permit;
       b=a;
       a=NUM_AREAS;
       write_str(user,"Security clearance set");
      }
  }
  
/* Do room check here, so it goes no further if room doesn't exist */
if (b == -1) {
   write_str(user,NO_ROOM);
   return;
   }

write_user(other_user);

if ((u=get_user_num(other_user,user))>-1) 
  {
    if (b > -1) 
      ustr[u].security[b]=permit;
      
    if (permit == 'Y')
      {
       sprintf(mess,"%s has cleared you to enter room %s",ustr[user].say_name,room);
       write_str(u,mess);
      }
     else
      {
       sprintf(mess,"%s has locked you from entering room %s",ustr[user].say_name,room);
       write_str(u,mess);
      }
   }

}

/*----------------------------------------------------------------------*/
/* Picture tell                                                         */
/*----------------------------------------------------------------------*/
void ptell(int user, char *inpstr)
{
int u;
char x_name[256],filename[FILE_NAME_LEN],temp[NAME_LEN+80];
char other_user[ARR_SIZE];
  
if (ustr[user].gagcomm) {
   write_str(user,NO_COMM);
   return;
   }

if (!strlen(inpstr)) {
   strcpy(inpstr, "names");
   preview(user, inpstr);
   return;
   }
 
strcpy(filename,"*not found*");
sscanf(inpstr,"%s ",other_user);

/* plug security hole */
if (check_fname(other_user,user)) 
  {
   write_str(user,"Illegal name.");
   return;
  }

strtolower(other_user);

   if (!check_for_user(other_user)) {
     write_str(user,NO_USER_STR);
     return;
     }

if ((u=get_user_num(other_user,user))== -1)
  {
   not_signed_on(user,other_user);
   return;
   }

if (!gag_check(user,u,0)) return;
   
if (ustr[u].pro_enter || ustr[u].vote_enter || ustr[u].roomd_enter) {
    write_str(user,IS_ENTERING);
    return;
    }

if (ustr[u].afk)
  {
    if (ustr[u].afk == 1) {
      if (!strlen(ustr[u].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[u].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }
     else {
      if (!strlen(ustr[u].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[u].say_name);
      else
      sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[u].say_name,ustr[u].afkmsg);
      }

    write_str(user,t_mess);
  }

remove_first(inpstr);
inpstr[80]=0;
sscanf(inpstr,"%s",filename);

if (!strlen(filename))
  {
   write_str(user,"Show what picture?");
   return;
  }    
  
/* plug security hole */
if (check_fname(filename,user)) 
  {
   sprintf(mess,"Illegal file name: %s",filename);
   write_str(user,mess);
   return;
  }

sprintf(t_mess,"%s/%s",PICTURE_DIR,filename);
strncpy(x_name,t_mess,256);

if (!check_for_file(x_name)) 
  {
   write_str(user,"Picture does not exist.");
   return;
  }
   
if (!user_wants_message(u,PICTURE)) {
  sprintf(temp,"%s is ignoring pictures. Not sent.",ustr[u].say_name);
  write_str(user,temp);
  }
else {
  sprintf(temp,"Picture request sent to %s",ustr[u].say_name);
  write_str(user,temp);

  sprintf(temp,"Private picture sent by: %s ",ustr[user].say_name);
  write_str(u,temp); 

  cat(x_name,u,0);

  }
}


/*------------------------------------------------*/
/* follow someone to a room                       */
/*------------------------------------------------*/
void follow(int user, char *inpstr)
{
int u;
char other_user[ARR_SIZE];

if (!strlen(inpstr)) {
   write_str(user,"Follow who?");
   return;
   }

sscanf(inpstr,"%s",other_user);
if ((u=get_user_num(other_user,user))== -1) {
   not_signed_on(user,other_user);
   return;
   }

if (!gag_check(user,u,0)) return;

strcpy(inpstr,astr[ustr[u].area].name);
go(user,inpstr,2);
}


/*-------------------------------------------------*/
/* log to syslog a string                          */
/*-------------------------------------------------*/
void print_to_syslog(char *str)
{
FILE *fp;

if (!syslog_on) return;

 if ((fp=fopen(LOGFILE,"a"))) 
   {
    fputs(str,fp);
    FCLOSE(fp);
   } 
 else {
   shutdown_error(14);
   return;
   }

}

void write_log(char *str, int type, int nl)
{
char logfile[256];
FILE *fp;

if (!syslog_on) return;

if (type==0)
 strcpy(logfile,EMAILLOG);
else if (type==1)
 strcpy(logfile,LOGINLOG);
else if (type==2)
 strcpy(logfile,ERRORLOG);
else if (type==3)
 strcpy(logfile,WARNINGLOG);

 if ((fp=fopen(logfile,"a"))) 
   {
    fputs(str,fp);
    if (nl) fputs("\n",fp);
    FCLOSE(fp);
   } 
 else {
   shutdown_error(14);
   return;
   }

if (SYSLOG_ALSO) {
 if ((fp=fopen(LOGFILE,"a"))) 
   {
    fputs(str,fp);
    if (nl) fputs("\n",fp);
    FCLOSE(fp);
   } 
 else {
   shutdown_error(14);
   return;
   }
 } /* end of SYSLOG_ALSO if */


}

/*--------------------------------------------------------------------*/
/* this command basically lists out a specified directory to the user */
/* the directory is specified in the inpstr                           */
/*--------------------------------------------------------------------*/
void print_ban_dir(int user, char *inpstr, char *s_search)
{
int num,timenum;
char buffer[132];
char small_buff[64];
char timebuf[23];
time_t tn;
time_t tm_then;
struct dirent *dp;
FILE *fp2;
DIR  *dirp;
 
 strcpy(buffer,"    ");
 num=0;
 dirp=opendir((char *)inpstr);
  
 if (dirp == NULL)
   {
    write_str(user,"Directory information not found.");
    return;
   }

   write_str(user,"Site/Cluster/Domain                 Ban Started"); 
   write_str(user,"-------------------------           -----------"); 
   
 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%s",dp->d_name);
       if (small_buff[0]!='.')
        {
          write_str(user,mess);
         if (!(fp2=fopen(small_buff,"r"))) {
            write_str(user,"Cant open file for reading!");
            continue;
            }
         fgets(timebuf,13,fp2);
         FCLOSE(fp2);
         time(&tn);
	 timenum=atoi(timebuf);
         tm_then=((time_t) timenum);
         sprintf(mess,"%-35s %s ago",small_buff,converttime((long)((tn-tm_then)/60)));
         write_str(user,mess);
         timebuf[0]=0;
         num++;
        }
      
  }
 write_str(user,"");
 sprintf(mess,"Displayed %d banned site%s",num,num == 1 ? "" : "s");
 write_str(user,mess);

 (void) closedir(dirp);
}


/*--------------------------------------------------------------------*/
/* this command basically lists out a specified directory to the user */
/* the directory is specified in the inpstr                           */
/*--------------------------------------------------------------------*/
void print_dir(int user, char *inpstr, char *s_search)
{
int num,mode=0;
char buffer[132];
char small_buff[64];
struct dirent *dp;
DIR  *dirp;
 
 if (!strcmp(s_search,"-n")) { mode=1; }
 else { mode=0; }

 strcpy(buffer,"    ");
 num=0;
 dirp=opendir((char *)inpstr);
  
 if (dirp == NULL)
   {
    write_str(user,"Directory information not found.");
    return;
   }

 if (mode) write_str(user,"Counting..");
   
 while ((dp = readdir(dirp)) != NULL) 
   { 
    sprintf(small_buff,"%-18s ",dp->d_name);
    if (s_search && !mode)
      { if (strstr(small_buff,s_search))
        {
         if (small_buff[0]!='.')
          { write_str_nr(user,small_buff);
            num++;
           if (num%4==0) write_str(user,"");
          }
          
        }
      }
     else
      {
       if (small_buff[0]!='.')
        { if (!mode) write_str_nr(user,small_buff);
         num++;
         if ((num%4==0) && (!mode)) write_str(user,"");
        }
      }
      
  }
 if (!mode)
  write_str(user,"");

 sprintf(mess,"%s %d item%s",mode == 1 ? "Counted" : "Displayed",num,num == 1 ? "" : "s");
 write_str(user,mess);

 (void) closedir(dirp);
}


/*------------------------------------------------------------*/
/* this command shows detailed statistics and info on a user  */
/*------------------------------------------------------------*/
void usr_stat(int user, char *inpstr, int mode)
{
char temp2[7];
char temp3[7];
char other_user[ARR_SIZE];
char sw[4][4];
char yesno[2][4];
char noyes[2][4];
char filename[FILE_NAME_LEN];
int i,min=0,ret=0;
int signed_on = FALSE;
time_t tm;
time_t tm_then;

strcpy(sw[0],"off");
strcpy(sw[1],"on ");
strcpy(sw[2],"on ");
strcpy(sw[3],"on ");
strcpy(yesno[0],"no ");
strcpy(yesno[1],"yes");
strcpy(noyes[0],"yes");
strcpy(noyes[1],"no ");

if (strlen(inpstr)) 
  {
   
   sscanf(inpstr,"%s ",other_user);
   strtolower(other_user);
   CHECK_NAME(other_user);


   if ((i = get_user_num(other_user,user)) != -1)
     if (strcmp(ustr[i].name, other_user) == 0) 
     {
      signed_on = TRUE;
     }
  }
 else
  {
    strcpy(other_user, ustr[user].name);
    signed_on = TRUE;
    i=user;
  }

ret=gag_check2(user,other_user);
if (!ret) return;
else if (ret==2) {
 write_str(user,NO_USER_STR);
 other_user[0]=0;
 inpstr[0]=0;
 return;
 }


time(&tm);  
if (signed_on) min=(tm-ustr[i].time)/60;

if (!read_user(other_user))
  {
   write_str(user,NO_USER_STR);
   other_user[0]=0;
   inpstr[0]=0;
   return;
  }

if (strlen(inpstr) && (signed_on == TRUE))
write_str(user,"+---------------^*** NOTE: That user is currently logged on. ***^---------------+");
else
write_str(user,"+-----------------------------------------------------------------------------+");

if (signed_on == TRUE) {
sprintf(mess,"| Id:    %-21.21s %s", ustr[i].say_name, ustr[i].desc );
write_str(user,mess);

if (!mode) {
if (ustr[i].new_mail)
  { 
     sprintf(mess,"|                                      Has %3.3d new mail message%s.",ustr[i].mail_num,ustr[i].mail_num == 1 ? "" : "s");
     write_str(user,mess);
     write_str(user,"|");
  }
else
 write_str(user,"|");
} /* end of no mode */

if (mode) {
sprintf(mess,"| Homepage:  %s",ustr[i].homepage);
write_str(user,mess);

if (ustr[i].semail && ustr[user].tempsuper < EMAIL_LEVEL && (i!=user))
strcpy(mess,"| Email:      -hidden-");
else if (ustr[i].semail && ((ustr[user].tempsuper >= EMAIL_LEVEL) || (i==user)))
sprintf(mess,"| Email:     %s  ^-hidden-^",ustr[i].email_addr);
else
sprintf(mess,"| Email:     %s",ustr[i].email_addr);
write_str(user,mess);

sprintf(mess,"| Gender:    %-32s  ICQ: %s",ustr[i].sex,ustr[i].icq);
write_str(user,mess);
} /* end of mode */

if (!mode) {
sprintf(mess,"| Created:    %-25s  commands total: %-8.8li",
                           ustr[i].creation, ustr[i].numcoms);
write_str(user,mess);

if (astr[ustr[i].area].hidden)
sprintf(mess,"| Last room:  %-16.16s           logins to date: %-5.5d",
                           "",ustr[i].times_on);
else
sprintf(mess,"| Last room:  %-16.16s           logins to date: %-5.5d",
                           astr[ustr[i].area].name, 
                           ustr[i].times_on);
write_str(user,mess);

sprintf(mess,"|                                        ave time/login: %-5.5d mins",
             ustr[i].aver);
write_str(user,mess);
} /* end of no mode */

if (mode) {
sprintf(mess,"| Has been signed on for:  %s", converttime((long)min));
write_str(user,mess);
} /* end of mode */

if (!mode) {
sprintf(mess,"| Cumulative time:         %s", 
               converttime(ustr[i].totl+(long)min)); write_str(user,mess);
sprintf(mess,"| From site:  %-15.15s %s",
              ustr[i].last_site,ustr[i].last_name);
if (ustr[user].tempsuper >= WIZ_LEVEL)      write_str(user,mess);
sprintf(mess,"| Made from:  %-15.15s %s",
              ustr[i].init_site,ustr[i].init_netname);
if (ustr[user].tempsuper >= WIZ_LEVEL)      write_str(user,mess);

sprintf(mess,"| Rank:       %d  (^%s^)",
              ustr[i].super,ranks[ustr[i].super]);
if ((ustr[user].tempsuper >= WIZ_LEVEL) || (i==user)) write_str(user,mess); 

write_str(user,"|");

sprintf(mess,"| Set: Rows   %-3d  Cols    %-3d  Carriages %s  Abbrs     %s  Hilite  %s", 
             ustr[i].rows, ustr[i].cols,sw[ustr[i].car_return], sw[ustr[i].abbrs],sw[ustr[i].hilite]);
write_str(user,mess);

sprintf(mess,"|      Space  %s  Visible %s  PrivBeeps %s  Igtells   %s  Passhid %s",
             sw[ustr[i].white_space],yesno[ustr[i].vis],sw[ustr[i].beeps],
sw[ustr[i].igtell],sw[ustr[i].passhid]);
write_str(user,mess);
sprintf(mess,"|      Color  %s  AutoFwd %s  AutoRead  %s",
             sw[ustr[i].color],sw[ustr[i].autof],sw[ustr[i].autor]); 
write_str(user,mess);
sprintf(mess,"|      Gagged %s  Xcommed %s  Anchored  %s  Muzzled   %s  Frogged %s",
   yesno[ustr[i].gagcomm],yesno[ustr[i].suspended],yesno[ustr[i].anchor],
   noyes[ustr[i].shout],yesno[ustr[i].frog]);
write_str(user,mess);
write_str(user,"|");
sprintf(mess,"|      TTT Wins       %3d  TTT Losses  %3d  Hangman Wins  %3d",
   ustr[i].ttt_kills,ustr[i].ttt_killed,ustr[i].hang_wins);
write_str(user,mess);
sprintf(mess,"|      Hangman Losses %3d  Nerf shots   %2d  Nerf energy    %2d",
   ustr[i].hang_losses,ustr[i].nerf_shots,ustr[i].nerf_energy);
write_str(user,mess);
if (ustr[i].nerf_kills==1) strcpy(temp2, "person");
else strcpy(temp2, "people");
if (ustr[i].nerf_killed==1) strcpy(temp3, "time");
else strcpy(temp3, "times");
sprintf(mess,"|      Has nerfed %3d %s and has been nerfed %3d %-5s",  
  ustr[i].nerf_kills, temp2, ustr[i].nerf_killed, temp3);
write_str(user,mess);
 if (ustr[i].nerf_kills > (ustr[i].nerf_killed + 5))
write_str(user,"|      This nerfer will eat your nerf-gun for breakfast!");
 else if (ustr[i].nerf_kills > ustr[i].nerf_killed)
write_str(user,"|      This nerfer is pretty good.");
 else if (ustr[i].nerf_kills < (ustr[i].nerf_killed - 5))
write_str(user,"|      This nerfer really sucks wind!");
 else if (ustr[i].nerf_kills < ustr[i].nerf_killed)
write_str(user,"|      This nerfer is mediocre..so-so");
 else { }
} /* end of no mode */
}

else {
sprintf(mess,"| Id:    %-21.21s %s", t_ustr.say_name, t_ustr.desc );
write_str(user,mess);

if (!mode) {
if (t_ustr.new_mail)
  { 
     sprintf(mess,"|                                      Has %3.3d new mail message%s.",t_ustr.mail_num,t_ustr.mail_num == 1 ? "" : "s");
   write_str(user,mess);
   write_str(user,"|");
  }
else
 write_str(user,"|");
} /* end of no mode */

if (mode) {
sprintf(mess,"| Homepage:  %s",t_ustr.homepage);
write_str(user,mess);

if (t_ustr.semail && ustr[user].tempsuper < EMAIL_LEVEL)
strcpy(mess,"| Email:      -hidden-");
else if (t_ustr.semail && ustr[user].tempsuper >= EMAIL_LEVEL)
sprintf(mess,"| Email:     %s  ^-hidden-^",t_ustr.email_addr);
else
sprintf(mess,"| Email:     %s",t_ustr.email_addr);
write_str(user,mess);

sprintf(mess,"| Gender:    %-32s  ICQ: %s",t_ustr.sex,t_ustr.icq);
write_str(user,mess);
} /* end of mode */

if (!mode) {
sprintf(mess,"| Created:    %-25s  commands total: %-8.8li",
                           t_ustr.creation,t_ustr.numcoms);
write_str(user,mess);

if (astr[t_ustr.area].hidden)
sprintf(mess,"| Last room:  %-16.16s           logins to date: %-5.5d",
                           "",t_ustr.times_on);
else
sprintf(mess,"| Last room:  %-16.16s           logins to date: %-5.5d",
                           astr[t_ustr.area].name, 
                           t_ustr.times_on);
write_str(user,mess);

sprintf(mess,"|                                        ave time/login: %-5.5d mins",
             t_ustr.aver);
write_str(user,mess);
} /* end of no mode */

if (mode) {
tm_then=((time_t) t_ustr.rawtime);
sprintf(mess,"| Last login: %-16.16s that was %s ago",
                           t_ustr.last_date, 
                           converttime((long)((tm-tm_then)/60)));
write_str(user,mess);
} /* end of mode */

if (!mode) {
sprintf(mess,"| Cumulative time:         %s",
              converttime(t_ustr.totl));
write_str(user,mess);
sprintf(mess,"| From site:  %-15.15s %s",
              t_ustr.last_site,t_ustr.last_name);
if (ustr[user].tempsuper >= WIZ_LEVEL)      write_str(user,mess);
sprintf(mess,"| Made from:  %-15.15s %s",
              t_ustr.init_site,t_ustr.init_netname);
if (ustr[user].tempsuper >= WIZ_LEVEL)      write_str(user,mess);

sprintf(mess,"| Rank:       %d  (^%s^)",
              t_ustr.super,ranks[t_ustr.super]);
if (ustr[user].tempsuper >= WIZ_LEVEL)      write_str(user,mess); 

write_str(user,"|");

sprintf(mess,"| Set: Rows   %-3d  Cols    %-3d  Carriages %s  Abbrs     %s  Hilite  %s", 
             t_ustr.rows, t_ustr.cols,sw[t_ustr.car_return],sw[t_ustr.abbrs],sw[t_ustr.hilite]);
write_str(user,mess);

sprintf(mess,"|      Space  %s  Visible %s  PrivBeeps %s  Igtells   %s  Passhid %s",
             sw[t_ustr.white_space],yesno[t_ustr.vis],sw[t_ustr.beeps],
sw[t_ustr.igtell],sw[t_ustr.passhid]);
write_str(user,mess);
sprintf(mess,"|      Color  %s  AutoFwd %s  AutoRead  %s",
             sw[t_ustr.color],sw[t_ustr.autof],sw[t_ustr.autor]); 
write_str(user,mess);
sprintf(mess,"|      Gagged %s  Xcommed %s  Anchored  %s  Muzzled   %s  Frogged %s",
   yesno[t_ustr.gagcomm],yesno[t_ustr.suspended],yesno[t_ustr.anchor],
   noyes[t_ustr.shout],yesno[t_ustr.frog]);
write_str(user,mess);
write_str(user,"|");
sprintf(mess,"|      TTT Wins       %3d  TTT Losses  %3d  Hangman Wins  %3d",
   t_ustr.ttt_kills,t_ustr.ttt_killed,t_ustr.hang_wins);
write_str(user,mess);
sprintf(mess,"|      Hangman Losses %3d  Nerf shots   %2d  Nerf energy    %2d",
   t_ustr.hang_losses,t_ustr.nerf_shots,t_ustr.nerf_energy);
write_str(user,mess);
if (t_ustr.nerf_kills==1) strcpy(temp2, "person");  
else strcpy(temp2, "people");
if (t_ustr.nerf_killed==1) strcpy(temp3, "time");
else strcpy(temp3, "times");
sprintf(mess,"|      Has nerfed %3d %s and has been nerfed %3d %-5s",
  t_ustr.nerf_kills, temp2, t_ustr.nerf_killed, temp3);
write_str(user,mess);
 if (t_ustr.nerf_kills > (t_ustr.nerf_killed + 5))
write_str(user,"|      This nerfer will eat your nerf-gun for breakfast!");
 else if (t_ustr.nerf_kills > t_ustr.nerf_killed)
write_str(user,"|      This nerfer is pretty good.");
 else if (t_ustr.nerf_kills < (t_ustr.nerf_killed - 5))
write_str(user,"|      This nerfer really sucks wind!");
 else if (t_ustr.nerf_kills < t_ustr.nerf_killed)
write_str(user,"|      This nerfer is mediocre..so-so");
 else { }
} /* end of no mode */
}

if (mode) {
write_str(user,"+-----------------------------------------------------------------------------+");
sprintf(filename,"%s/%s",PRO_DIR,other_user);
if (!cat(filename,user,0))
  write_str(user,"No profile. Sorry :)");
write_str(user,"+-----------------------------------------------------------------------------+");
} /* end of mode */
else {
write_str(user,"+-----------------------------------------------------------------------------+");
} /* end of no mode */

   other_user[0]=0;
   inpstr[0]=0;

}

/*------------------------------------------------*/
/* set email address                              */
/*------------------------------------------------*/
void set_email(int user, char *inpstr)
{
char temp[EMAIL_LENGTH+1];

  if (!strlen(inpstr)) {
	sprintf(mess,"Your email is: %s",ustr[user].email_addr);
	write_str(user, mess);
	return;
	}

  /* Check for illegal characters in email addy */
  if (strpbrk(inpstr,";/[]\\") ) {
     write_str(user,"Illegal email address");
     return;
     }

  if (strstr(inpstr,"^")) {
     write_str(user,"Email cannot have color or hilite codes in it.");
     return;
     }

  if (strlen(inpstr)>EMAIL_LENGTH)
    {
      write_str(user,"Email address truncated");
      inpstr[EMAIL_LENGTH-1]=0;
    }

 strcpy(temp,inpstr);
 strtolower(temp);

 if ((!strcmp(inpstr,"-c")) || (!strcmp(inpstr,"clear"))) {
     strcpy(inpstr,DEF_EMAIL);
     write_str(user,"Email address cleared and reset.");
     goto SKIP;
     }
 else if (strstr(temp,"whitehouse.gov"))
      {
       write_str(user,"Email address not valid.");
       return;
      }
  else if (!strstr(inpstr,".") || !strstr(inpstr,"@")) {
       write_str(user,"Email address not valid.");
       return;
      }

  sprintf(mess,"Set user email address to: %s",inpstr);
  write_str(user,mess);

SKIP:
  strcpy(ustr[user].email_addr,inpstr);

  copy_from_user(user);
  write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set homepage                                   */
/*------------------------------------------------*/
void set_homepage(int user, char *inpstr)
{

  if (!strlen(inpstr)) {
	sprintf(mess,"Your homepage is: %s",ustr[user].homepage);
	write_str(user, mess);
	return;
	}

  if (strstr(inpstr,"^")) {
     write_str(user,"Homepage cannot have color or hilite codes in it.");
     return;
     }

  if (strlen(inpstr) > HOME_LEN) 
     {
      write_str(user,"Home page address truncated");
      inpstr[HOME_LEN-1]=0;
     }

  strcpy(ustr[user].homepage,inpstr);

  copy_from_user(user);
  write_user(ustr[user].login_name);
  sprintf(mess,"Set page to: %s",inpstr);
  write_str(user,mess);

}

/*------------------------------------------------*/
/* set URL where users picture can be found       */
/*------------------------------------------------*/
void set_webpic(int user, char *inpstr)
{

  if (!strlen(inpstr)) {
	sprintf(mess,"Your picture URL is: %s",ustr[user].webpic);
	write_str(user, mess);
	return;
	}

  if (strstr(inpstr,"^")) {
     write_str(user,"Picture URL address cannot have color or hilite codes in it.");
     return;
     }

  if (strlen(inpstr) > HOME_LEN) 
     {
      write_str(user,"Picture URL address truncated");
      inpstr[HOME_LEN-1]=0;
     }

  strcpy(ustr[user].webpic,inpstr);

  copy_from_user(user);
  write_user(ustr[user].login_name);
  sprintf(mess,"Set picture URL to: %s",inpstr);
  write_str(user,mess);

}

/*------------------------------------------------*/
/* set gender                                     */
/*------------------------------------------------*/
void set_sex(int user, char *inpstr)
{

if (!strlen(inpstr))
  {
   sprintf(mess,"Your gender is : %s",ustr[user].sex);
   write_str(user,mess);
   return;
  }

  if (strlen(inpstr)>29)
    {
      write_str(user,"Gender truncated");
      inpstr[29]=0;
    }

  strcat(inpstr,"@@");

  strcpy(ustr[user].sex,inpstr);

  if (autopromote == 1)
   check_promote(user,7);

  copy_from_user(user);
  write_user(ustr[user].name);
  sprintf(mess,"Set user gender to: %s",inpstr);
  write_str(user,mess);
}

/*------------------------------------------------*/
/* set rows                                       */
/*------------------------------------------------*/
void set_rows(int user, char *inpstr)
{

  int value=5;
  
  sscanf(inpstr,"%d", &value);
  
  if (value < 5 || value > 256)
    {
      write_str(user,"Rows set to 25 (valid range is 5 to 256)");
      value = 25;
    }
    
  ustr[user].rows = value;

  copy_from_user(user);
  write_user(ustr[user].login_name);
  sprintf(mess,"Set terminal rows to: %d",value);
  write_str(user,mess);
}

/*------------------------------------------------*/
/* set cols                                       */
/*------------------------------------------------*/
void set_cols(int user, char *inpstr)
{

  int value=5;
  
  sscanf(inpstr,"%d", &value);
  
  if (value < 16 || value > 256)
    {
      write_str(user,"Columns set to 256 (valid range is 16 to 256)");
      value = 256;
    }
    
  ustr[user].cols = value;

  copy_from_user(user);
  write_user(ustr[user].login_name);
  sprintf(mess,"Set terminal cols to: %d",value);
  write_str(user,mess);
}

/*------------------------------------------------*/
/* set car_return                                 */
/*------------------------------------------------*/
void set_car_ret(int user, char *inpstr)
{

if (!strlen(inpstr)) {
  if (!ustr[user].car_return) {
   write_str(user,"Set carriage returns ON");
   ustr[user].car_return = 1;
   }
   else {
   write_str(user,"Set carriage returns OFF");
   ustr[user].car_return = 0;
   }
 }
else {
 if (!strcmp(inpstr,"1")) {
   write_str(user,"Set carriage returns ON");
   ustr[user].car_return = 1;
   }
 else if (!strcmp(inpstr,"0")) {
   write_str(user,"Set carriage returns OFF");
   ustr[user].car_return = 0;
  }
 else {
   write_str(user,"Set carriage returns ON");
   ustr[user].car_return = 1;
  }   
 } /* end of else */

  copy_from_user(user);
  write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set atmos                                      */
/*------------------------------------------------*/
void set_atmos(int user, char *inpstr)
{

  int    value  = -1;
  int    factor = -1;
  
  sscanf(inpstr,"%d %d", &value, &factor);
  
  if (value < 0 || value > 1000)
    {
      value = 100;
    }
    
 if (factor < 0 || factor >1000)
    {
      factor = 5;
    }

  sprintf(mess,"Atmos frequency chance set to: %d %d",value, factor);
  write_str(user,mess);

  ATMOS_RESET     = value;
  ATMOS_FACTOR    = factor;
  ATMOS_COUNTDOWN = value;
}

/*------------------------------------------------*/
/* set abbrs                                      */
/*------------------------------------------------*/
void set_abbrs(int user, char *inpstr)
{


  if (ustr[user].abbrs)
    {
      write_str(user, "Abbreviations are now off for you.");
      ustr[user].abbrs = 0;
    }
   else
    {
      write_str(user, "You can now use abbreviations");
      ustr[user].abbrs = 1;
    }
    
  copy_from_user(user);
  write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set white space                                */
/*------------------------------------------------*/
void set_white_space(int user, char *inpstr)
{


  if (ustr[user].white_space)
    {
      write_str(user, "White space removal is now off.");
      ustr[user].white_space = 0;
    }
   else
    {
      write_str(user, "White space removal is now on.");
      ustr[user].white_space = 1;
    }
    
  copy_from_user(user);
  write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set hilights                                   */
/*------------------------------------------------*/
void set_hilite(int user, char *inpstr)
{

  if (ustr[user].hilite==2)
    {
      write_str(user, "High_lighting now off.");
      ustr[user].hilite = 0;
    }
  else if (ustr[user].hilite==1)
    {
      write_str(user, "High_lighting now on for everything except private communication which will be normal with color.");
      ustr[user].hilite = 2;
    }
  else
    {
      write_str(user, "High_lighting now on for everything.");
      ustr[user].hilite = 1;
    }
    
  copy_from_user(user);
  write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set password echo                              */
/*------------------------------------------------*/
void set_hidden(int user, char *inpstr)
{


  if (ustr[user].passhid)
    {
      write_str(user, "Password WILL be echoed during logins");
      ustr[user].passhid = 0;
    }
   else
    {
      write_str(user, "Password will NOT be echoed during logins.");
      ustr[user].passhid = 1;
    }
    
  copy_from_user(user);
  write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set login pause                                */
/*------------------------------------------------*/
void set_pause(int user)
{

  if (ustr[user].pause_login)
    {
      write_str(user, "You will NOT get a pause when your login");
      ustr[user].pause_login = 0;
    }
   else
    {
      write_str(user, "You WILL get a pause when your login");
      ustr[user].pause_login = 1;
    }
    
  copy_from_user(user);
  write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set break in who or not                        */
/*------------------------------------------------*/
void set_pbreak(int user, char *inpstr)
{


  if (ustr[user].pbreak)
    {
      write_str(user, "Who listing will be continuous.");
      ustr[user].pbreak = 0;
    }
   else
    {
      write_str(user, "Who listing will be paged.");
      ustr[user].pbreak = 1;
    }
    
  copy_from_user(user);
  write_user(ustr[user].login_name);
}


/*------------------------------------------------*/
/* set beeps on private communication             */
/*------------------------------------------------*/
void set_beep(int user, char *inpstr)
{

  if (ustr[user].beeps)
    {
      write_str(user, "You now will ^NOT^ get beeps on private communications");
      ustr[user].beeps = 0;
    }
   else
    {
      write_str(user, "You now ^WILL^ get beeps on private communications");
      ustr[user].beeps = 1;
    }
    
  copy_from_user(user);
  write_user(ustr[user].login_name);
}


/*------------------------------------------------*/
/* set your name capitalization                   */
/*------------------------------------------------*/
void set_recap(int user, char *inpstr)
{
int len = strlen(inpstr);
char name[NAME_LEN];
char tempname[NAME_LEN];

if (!len) {
  write_str(user,GIVE_CAPNAME);
  return;
 }

if (len > NAME_LEN) {
  write_str(user,SYS_NAME_LONG);
  return;
 }

strcpy(name, strip_color(inpstr));

strcpy(tempname, name);
strtolower(tempname);

if (strcmp(tempname, ustr[user].login_name)) {
  sprintf(mess,"New name (%s) is not the same as original name (%s)",
          name, ustr[user].login_name);
  write_str(user,mess);
  return;
  }

strcpy(ustr[user].say_name, name);
sprintf(mess, "Your name is recapped to: %s", ustr[user].say_name);
write_str(user,mess);

/* change exempt file */
/* first arguement is to check against names in file for user */
/* second is the new name we're changing to */
change_exem_data(ustr[user].name,ustr[user].say_name);

copy_from_user(user);
write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set your home room                             */
/*------------------------------------------------*/
void set_home(int user, char *inpstr)
{
int found = 0;
int new_area;

  if (!strlen(inpstr)) {
     sprintf(mess,"Your home room is the: %s",ustr[user].home_room);
     write_str(user,mess);
     return;
     }

  if (strstr(inpstr,"^")) {
     write_str(user,"Room cannot have color or hilite codes in it.");
     return;
     }

  if (strlen(inpstr) > NAME_LEN) 
     {
      write_str(user,"Room name length too long.");
      return;
     }
         /* Cygnus */
   if ( (!strcmp(inpstr,HEAD_ROOM)) || (!strcmp(inpstr,ARREST_ROOM)) ||
        (!strcmp(inpstr,"sky_palace")) ) {
      write_str(user,"You cannot make that room your home.");
      return;
      }

   /*--------------------*/
   /* see if area exists */
   /*--------------------*/

   found = FALSE;
   for (new_area=0; new_area < NUM_AREAS; ++new_area)
    { 
     if (!strcmp(astr[new_area].name, inpstr) )
       { 
         found = TRUE;
         break;
       }
    }
 
   if (!found)
     {
      write_str(user,NO_ROOM);
      return;
     }
  
/*----------------------------------------------*/
/* check for secure room                        */
/*----------------------------------------------*/

if (astr[new_area].hidden && ustr[user].security[new_area] == 'N')
  {
   write_str(user,NO_ROOM);
   return;
  }


/*-------------------------------------------------------------------*/
/* see if new room has exits, if not and not wizard, no set possible */
/*-------------------------------------------------------------------*/
found = TRUE;
if ( (!strlen(astr[new_area].move)) || (!strcmp(astr[new_area].move,"*")) ) 
    {
     found = FALSE;
    }

/*--------------------------------------------------------------*/
/* anyone above a 3 can teleport to non-connected rooms         */
/*--------------------------------------------------------------*/

if (!found)
  {
    if ((ustr[user].tempsuper < TELEP_LEVEL) && (ustr[user].security[new_area] == 'N')) 
      {
       write_str(user,"You cannot make a non-connected room your home.");
       return;
      }
  }

  strcpy(ustr[user].home_room,astr[new_area].name);

  copy_from_user(user);
  write_user(ustr[user].login_name);
  sprintf(mess,"Set home room to: %s",astr[new_area].name);
  write_str(user,mess);
}

/** command to test and turn on/off color attribs. **/
void set_color(int user, char *inpstr)
{
int a=0;

if (!strlen(inpstr)) {
  if (ustr[user].color==0) {
   write_str(user,"Color is now   On");
   ustr[user].color=1;
   }
  else {
   write_str(user,"Color is now   Off");
   ustr[user].color=0;
   }
 }
else if (!strcmp(inpstr,"on") || !strcmp(inpstr,"ON")) {
   write_str(user,"Color is now   On");
   ustr[user].color=1;
   }
else if (!strcmp(inpstr,"off") || !strcmp(inpstr,"OFF")) {
   write_str(user,"Color is now   Off");
   ustr[user].color=0;
   }
else if (!strcmp(inpstr,"test") || !strcmp(inpstr,"TEST")) {
a=ustr[user].color;
if (a==0) ustr[user].color=1;
 write_str(user,"This test is to see whether your terminal is capable of");
 write_str(user,"displaying ANSI color. During the test, you should see the color"); 
 write_str(user,"displayed next to its corresponding name, if your terminal IS compatible.");
 write_str(user,"To use a color code in something, read .h coloruse");
 write_str(user,"  COLOR                EFFECT                  CODE");
 write_str(user,"  Low Green      ^LG      XXXXXXXXXXXXXXXXX ^       LG");
 write_str(user,"  Low Yellow     ^LY      XXXXXXXXXXXXXXXXX ^       LY");
 write_str(user,"  Low Red        ^LR      XXXXXXXXXXXXXXXXX ^       LR");
 write_str(user,"  Low Blue       ^LB      XXXXXXXXXXXXXXXXX ^       LB");
 write_str(user,"  Low Magenta    ^LM      XXXXXXXXXXXXXXXXX ^       LM");
 write_str(user,"  Low White      ^LW      XXXXXXXXXXXXXXXXX ^       LW");
 write_str(user,"  Low Cyan       ^LC      XXXXXXXXXXXXXXXXX ^       LC");
 write_str(user,"  High Green     ^HG      XXXXXXXXXXXXXXXXX ^       HG");
 write_str(user,"  High Yellow    ^HY      XXXXXXXXXXXXXXXXX ^       HY");
 write_str(user,"  High Red       ^HR      XXXXXXXXXXXXXXXXX ^       HR");
 write_str(user,"  High Blue      ^HB      XXXXXXXXXXXXXXXXX ^       HB");
 write_str(user,"  High Magenta   ^HM      XXXXXXXXXXXXXXXXX ^       HM");
 write_str(user,"  High White     ^HW      XXXXXXXXXXXXXXXXX ^       HW");
 write_str(user,"  High Cyan      ^HC      XXXXXXXXXXXXXXXXX ^       HC");
 write_str(user,"  Bold       ^          XXXXXXXXXXXXXXXXX ^       None");
 write_str(user,"  Blinking             ^BLXXXXXXXXXXXXXXXXX ^       BL");
 write_str(user,"  Underlined           ^ULXXXXXXXXXXXXXXXXX ^       UL");
 write_str(user,"  Reverse Video        ^RVXXXXXXXXXXXXXXXXX ^       RV");
if (a==0) ustr[user].color=0;
 write_str(user,"<Ok>"); 
 }
else {
   write_str(user,"Usage: .set color        -  Toggle your color switch on/off");
   write_str(user,"       .set color on     -  Turns colored attributes on");
   write_str(user,"       .set color off    -  Turns colored attributes off");
   write_str(user,"       .set color test   -  Tests your terminal for ANSI color");
 }

   read_user(ustr[user].login_name);
   t_ustr.color = ustr[user].color;
   write_user(ustr[user].login_name);
}


/** show email address - yes or no **/
void set_visemail(int user)
{

  if (ustr[user].semail)
    {
      write_str(user, "Email address now visible.");
      ustr[user].semail = 0;
    }
   else
    {
      write_str(user,"Email address now hidden.");
      ustr[user].semail = 1;
    }
    
  read_user(ustr[user].login_name);
  t_ustr.semail = ustr[user].semail;
  write_user(ustr[user].login_name);
}

/* Set .help style */
void set_help(int user)
{
 char type[4][7];

   strcpy(type[0],"OURS  ");
   strcpy(type[1],"IFORMS");
   strcpy(type[2],"NUTS3 ");
   strcpy(type[3],"NUTS2 ");

 if (ustr[user].help==0)
    ustr[user].help=1;
 else if (ustr[user].help==1)
    ustr[user].help=2;
 else if (ustr[user].help==2)
    ustr[user].help=3;
 else if (ustr[user].help==3)
    ustr[user].help=0;

    sprintf(mess,".help type now set to: ^HY%s^",type[ustr[user].help]);
    write_str(user,mess);

  read_user(ustr[user].login_name);
  t_ustr.help = ustr[user].help;
  write_user(ustr[user].login_name);
}

/* Set .who style */
void set_who(int user)
{
 char type[4][7];

   strcpy(type[0],"OURS  ");
   strcpy(type[1],"NUTS  ");
   strcpy(type[2],"IFORMS");
   strcpy(type[3],"NEW   ");

 if (ustr[user].who==0)
    ustr[user].who=1;
 else if (ustr[user].who==1)
    ustr[user].who=2;
 else if (ustr[user].who==2)
    ustr[user].who=3;
 else if (ustr[user].who==3)
    ustr[user].who=0;

    sprintf(mess,".who type now set to: ^HY%s^",type[ustr[user].who]);
    write_str(user,mess);

  read_user(ustr[user].login_name);
  t_ustr.who = ustr[user].who;
  write_user(ustr[user].login_name);
}

/*------------------------------------------------*/
/* set icq number                                 */
/*------------------------------------------------*/
void set_icq(int user, char *inpstr)
{

  if (!strlen(inpstr)) {
	sprintf(mess,"Your ICQ # is: %s",ustr[user].icq);
	write_str(user, mess);
	return;
	}

  if (strstr(inpstr,"^")) {
     write_str(user,"ICQs cannot have color or hilite codes in them.");
     return;
     }

  if (strlen(inpstr) > 20) 
     {
      write_str(user,"ICQ number truncated");
      inpstr[20-1]=0;
     }

  strcpy(ustr[user].icq,inpstr);

  copy_from_user(user);
  write_user(ustr[user].login_name);
  sprintf(mess,"Set ICQ # to: %s",inpstr);
  write_str(user,mess);

}


/*------------------------------------------------*/
/* set user stuff                                 */
/*------------------------------------------------*/
void set(int user, char *inpstr)
{
  char onoff[3][4];
  char yesno[2][4];
  char whotype[4][7];
  char helptype[4][7];
  char command[ARR_SIZE];

command[0]=0;
  sscanf(inpstr,"%s ",command);
  remove_first(inpstr);  /* get rid of commmand word */
  strtolower(command);

  if (!strcmp("email",command))
    {set_email(user,inpstr);
    }
  else if (!strcmp("gender",command))
    {set_sex(user,inpstr);
    }
  else if (!strcmp("homepage",command))
    {set_homepage(user,inpstr);
    }
  else if (!strcmp("picurl",command))
    {set_webpic(user,inpstr);
    }  
  else if (!strcmp("rows",command) || !strcmp("lines",command))
    {set_rows(user,inpstr);
    }
  else if (!strcmp("cols",command) || !strcmp("width",command))
    {set_cols(user,inpstr);
    }
  else if (!strcmp("car",command) || !strcmp("carriage",command) )
    {set_car_ret(user,inpstr);
    }
  else if (!strcmp("abbrs",command))
    {set_abbrs(user,inpstr);
    }
  else if (!strcmp("space",command))
    {set_white_space(user,inpstr);
    }
  else if (!strcmp("hi",command))
    {set_hilite(user,inpstr);
    }
  else if (!strcmp("hidden",command))
    {set_hidden(user,inpstr);
    }
  else if (!strcmp("pbreak",command))
    {set_pbreak(user,inpstr);
    }
  else if (!strcmp("recap",command))
    {set_recap(user,inpstr);
    }
  else if (!strcmp("home",command))
    {set_home(user,inpstr);
    }    
  else if (!strcmp("atmos",command))
    {set_atmos(user,inpstr);
    }
  else if (!strcmp("beeps",command))
    {set_beep(user,inpstr);
    }
  else if (!strcmp("help",command))
    {set_help(user);
    }
  else if (!strcmp("who",command))
    {set_who(user);
    }
  else if (!strcmp("color",command))
    {set_color(user,inpstr);
    }
  else if (!strcmp("visemail",command))
    {set_visemail(user);
    }
  else if (!strcmp("icq",command))
    {set_icq(user,inpstr);
    }
  else if (!strcmp("pause",command))
    {set_pause(user);
    }
  else if (!strcmp("autoread",command))
    {set_autoread(user);
    }
  else if (!strcmp("autofwd",command))
    {set_autofwd(user);
    }
  else if (!strcmp("show",command)) {
   strcpy(onoff[0],"OFF");
   strcpy(onoff[1],"ON ");
   strcpy(onoff[2],"ON ");
   strcpy(yesno[0],"NO ");
   strcpy(yesno[1],"YES");
   strcpy(whotype[0],"OURS  ");
   strcpy(whotype[1],"NUTS  ");
   strcpy(whotype[2],"IFORMS");
   strcpy(whotype[3],"NEW   ");
   strcpy(helptype[0],"OURS  ");
   strcpy(helptype[1],"IFORMS");
   strcpy(helptype[2],"NUTS3 ");
   strcpy(helptype[3],"NUTS2 ");
   write_str(user,"----------------------------------------------------");
   write_str(user," Your .set settings:");
   write_str(user,"");
   sprintf(mess,"Name    : %s",ustr[user].say_name);
   write_str(user,mess);
   sprintf(mess,"Email   : %s",ustr[user].email_addr);
   write_str(user,mess);
   sprintf(mess,"Homepage: %s",ustr[user].homepage);
   write_str(user,mess);
   sprintf(mess,"Pic URL : %s",ustr[user].webpic);
   write_str(user,mess);
   sprintf(mess,"Gender  : %s",ustr[user].sex);
   write_str(user,mess);
   sprintf(mess,"Home    : %s",ustr[user].home_room);
   write_str(user,mess);
   sprintf(mess,"ICQ #   : %s",ustr[user].icq);
   write_str(user,mess);
   write_str(user,"");
   sprintf(mess,"Rows    : %-3d   Columns : %-3d   Who_Style   : %s",
    ustr[user].rows,ustr[user].cols,whotype[ustr[user].who]);
   write_str(user,mess);
   sprintf(mess,"Abbrs   : %s   Whtspace: %s   Help_Style  : %s",
    onoff[ustr[user].abbrs],onoff[ustr[user].white_space],helptype[ustr[user].help]);
   write_str(user,mess);
   sprintf(mess,"Cariages: %s   Hilites : %s   Email_Hidden: %s",
    onoff[ustr[user].car_return],onoff[ustr[user].hilite],yesno[ustr[user].semail]);
   write_str(user,mess);
   sprintf(mess,"Passhid : %s   Pbreak  : %s   Pause_Login : %s",onoff[ustr[user].passhid],onoff[ustr[user].pbreak],onoff[ustr[user].pause_login]);
   write_str(user,mess);
   sprintf(mess,"PrivBeep: %s   Color   : %s",
    onoff[ustr[user].beeps],onoff[ustr[user].color]);
   write_str(user,mess);
   write_str(user,"----------------------------------------------------");
   return;
  }
 else {
 write_str(user,"Valid options are: (help on these under ^.h set^)");
 write_str(user,"  cols               autoread (toggle)");
 write_str(user,"  email              beeps    (toggle)");
 write_str(user,"  gender             car      (toggle)");
 write_str(user,"  home               color    (toggle)");
 write_str(user,"  homepage           help     (toggle)");
 write_str(user,"  icq                hi       (toggle)");
 write_str(user,"  picurl             hidden   (toggle)");
 write_str(user,"  recap              pause    (toggle)");
 write_str(user,"  rows               pbreak   (toggle)");
 write_str(user,"  show               space    (toggle)");
 write_str(user,"  abbrs    (toggle)  visemail (toggle)");
 write_str(user,"  autofwd  (toggle)  who      (toggle)");
 }
}


/*------------------------------------------------------------------*/
/* the nuke command....remove user from user dir                    */
/*------------------------------------------------------------------*/
void nuke(int user, char *inpstr, int mode)
{
int u,a=0;
char other_user[ARR_SIZE];
char other_name[NAME_LEN+1];
char z_mess[132];
 
if (!strlen(inpstr)) 
  {
   write_str(user,"Nuke who?"); 
   return;
  }
 
sscanf(inpstr,"%s ",other_user);
strtolower(other_user);
CHECK_NAME(other_user);

if ((u=get_user_num_exact(other_user,user)) == -1) {
  if (!read_user(other_user))
  {
   write_str(user,NO_USER_STR);
   return;
  }
 }


if (u==-1) {
 if ((t_ustr.super >= ustr[user].tempsuper) && strcmp(ustr[user].name,ROOT_ID))
  {
    strcpy(z_mess,"You cannot nuke a user of same or higher rank.");
    write_str(user,z_mess);
    return;
  }
 }
else {
 if ((ustr[u].super >= ustr[user].tempsuper) && strcmp(ustr[user].name,ROOT_ID))
  {
    strcpy(z_mess,"You cannot nuke a user of same or higher rank.");
    write_str(user,z_mess);
    return;
  }
 }

/* kill user */    
if (u!=-1) {
  a = rand() % NUM_KILL_MESSAGES;

  strcpy(other_name,ustr[u].say_name);
  sprintf(mess, kill_text[a], other_name);
  writeall_str(mess, 1, u, 1, user, NORM, KILL, 0);

  write_str(u,DEF_KILL_MESS);
  user_quit(u);
 }

remove_exem_data(other_user);
remove_user(other_user);

if (u==-1) {
 if (!mode) {
  sprintf(z_mess,"NUKE %s by %s",t_ustr.say_name,ustr[user].say_name);   
  btell(user,z_mess);
  sprintf(z_mess,"%s: NUKE %s by %s\n",get_time(0,0),t_ustr.say_name,ustr[user].say_name);   
  print_to_syslog(z_mess);
  }
 }
else {
 sprintf(z_mess,"KILL-NUKE %s by %s",other_name,ustr[user].say_name);   
 btell(user,z_mess);
 sprintf(z_mess,"%s: KILL-NUKE %s by %s\n",get_time(0,0),other_name,ustr[user].say_name);   
 print_to_syslog(z_mess);
 }

}

/*-----------------------*/
/*  tell all wizards     */
/*-----------------------*/
void btell(int user, char *inpstr)
{
char line[ARR_SIZE];
char comstr[ARR_SIZE];
int pos = bt_count%NUM_LINES;
int f;

if (user==-1)
  {
   return;
  }
  
if (!strlen(inpstr)) 
  {
    write_str(user,"Review wiztells:"); 
    
    for (f=0;f<NUM_LINES;++f) 
      {
        if ( strlen(bt_conv[pos]) )
         {
	  write_str(user,bt_conv[pos]);  
	 }
	pos = ++pos % NUM_LINES;
      }

    write_str(user,"<Done>");  
    return;
  }

if (ustr[user].frog) strcpy(inpstr,FROG_TALK);

comstr[0]=inpstr[0];
comstr[1]=0;

if (comstr[0] == ustr[user].custAbbrs[get_emote(user)].abbr[0]) 
  {
  inpstr[0] = ' ';
  while(inpstr[0] == ' ') inpstr++;
  sprintf(line,"%s %s %s",STAFF_PREFIX,ustr[user].say_name,inpstr);
  }
else if (comstr[0] == '\'') {
  inpstr[0] = ' ';
  while(inpstr[0] == ' ') inpstr++;
  sprintf(line,"%s %s\'%s",STAFF_PREFIX,ustr[user].say_name,inpstr);
  }
 else   
  sprintf(line,"%s %s: %s",STAFF_PREFIX,ustr[user].say_name,inpstr);

strcpy(line, strip_color(line));
write_hilite(user,line);

writeall_str(line, WIZ_ONLY, user, 0, user, BOLD, WIZT, 0);

/*-------------------------------------*/
/* store the wiztell in the rev buffer */
/*-------------------------------------*/

strncpy(bt_conv[bt_count],line,MAX_LINE_LEN);
bt_count = ( ++bt_count ) % NUM_LINES;
	
}


/*----------------------------------------------*/
/* fix for telnet ctrl-c and ctrl-d *arctic9*   */
/*----------------------------------------------*/
void will_time_mark(int user)
{
char seq[4];

 sprintf(seq,"%c%c%c", IAC, WILL, TELOPT_TM);
 write_str(user,seq);
}
                 

/*--------------------------------*/
/* clear screen                   */
/*--------------------------------*/
void cls(int user)
{
int   i         = ustr[user].rows;
char  addem[3];

strcpy(addem,"\n\r");
mess[0] = 0;

if (!ustr[user].car_return) addem[1]=0;
if (i > 75) i = 75;

for(; i--;)
 { strcat(mess,addem); }
 
strcat(mess, "OK");
strcat(mess, addem);

write_str(user,mess);
}

/*-----------------------------------*/
/* get a random number based on rank */
/*-----------------------------------*/
int get_odds_value(int user)
{
return( (rand() % odds[ustr[user].super]) + 1 );
}

/*----------------------------------------------------------*/
/* determine the result of a random event between two users */
/*----------------------------------------------------------*/
int determ_rand(int u1, int u2)
{
int v1, v2, v3, result;
float f_fact;

v1 = get_odds_value(u1);
v2 = get_odds_value(u2);

if (v1 == v2)  /* truely amazing, a real tie */
  { return(TIE); }
  
if (v1 > v2)
  { 
    result = 1;
    f_fact = (float)((float)v2/(float)v1);
  }
 else
  { 
    result = 2;
    f_fact = (float)((float)v1/(float)v2);
  }
  
v3 = (int) (f_fact * 100.0);

if (v3 > CLOSE_NUMBER)
  { if (  rand() % 2 )
      return(TIE);
     else
      return(BOTH_LOSE);
  }
  
return(result);
}

/*----------------------------------------------------------*/
/* issue a fight challenge to another user                  */
/*----------------------------------------------------------*/
void issue_chal(int user, int user2)
{
  fight.first_user  = user;
  fight.second_user = user2;
  fight.issued      = 1;
  fight.time        = time(0);
  
  sprintf(mess, chal_text[ rand() % num_chal_text ],
                ustr[user].say_name,
                ustr[user2].say_name);
              
  writeall_str(mess, 1, user, 0, user, NORM, FIGHT, 0);
  write_str(user,mess);
  
  write_str(user2,"");
  write_str(user2,"");
  write_str(user2,CHAL_LINE);
  write_str(user2,"");
  write_str(user2,CHAL_LINE2);
  write_str(user,CHAL_ISSUED);
}

/*----------------------------------------------------------*/
/* accept a fight challenge                                 */
/*----------------------------------------------------------*/
void accept_chal(int user)
{
int x;
int a,b;

a=fight.first_user;
b=fight.second_user;

x = determ_rand(a, b);

if (x == TIE)
  {
   sprintf(mess, tie1_text[ rand() % num_tie1_text ],
                 ustr[a].say_name,
                 ustr[b].say_name);
              
   writeall_str(mess, 1, user, 0, user, NORM, FIGHT, 0);
   write_str(user,mess);
   return;
  }
  
if (x == BOTH_LOSE)
  {
   sprintf(mess, tie2_text[ rand() % num_tie2_text ],
                 ustr[a].say_name,
                 ustr[b].say_name);
              
   writeall_str(mess,1,user,0,user,NORM,FIGHT,0);
   write_str(user,mess);
   user_quit(a);
   user_quit(b);
   return;
  }
  
if (x == 1)
  {
   sprintf(mess, wins1_text[ rand() % num_wins1_text ],
                 ustr[a].say_name,
                 ustr[b].say_name);
              
   writeall_str(mess,1,user,0,user,NORM,FIGHT,0);
   write_str(user,mess);
   user_quit(b);
   return;
  }
 
if (x == 2)
  {
   sprintf(mess, wins2_text[ rand() % num_wins2_text ],
                 ustr[b].say_name,
                 ustr[a].say_name);
              
   writeall_str(mess,1,user,0,user,NORM,FIGHT,0);
   write_str(user,mess);
   user_quit(a);
   return;
  }
}

/*----------------------------------------------------------*/
/* reset the fight                                          */
/*----------------------------------------------------------*/
void reset_chal(int user, char *inpstr)
{
  fight.first_user = -1;
  fight.second_user = -1;
  fight.issued = 0;
  fight.time = 0;
}

/*----------------------------------------------------------*/
/* the fight command                                        */
/*----------------------------------------------------------*/
void fight_another(int user, char *inpstr)
{
char other_user[ARR_SIZE];
int user2=-1;
int mode;

if (!strlen(inpstr)) 
  {
   write_str(user,"Fight status:");
   if (fight.issued)
     { 
      sprintf(mess,"Aggressor:   %s",ustr[fight.first_user].say_name);
      write_str(user,mess);
      sprintf(mess,"Defender:    %s",ustr[fight.second_user].say_name);
      write_str(user,mess);
     }
    else
     write_str(user,"   No current fight is challenged.");
   return;
  }
  
mode = 0;
if (!strcmp(inpstr,"reset"))   
  {
    reset_chal(user, inpstr);
    return;
  }
  
if (!strcmp(inpstr,"1"))   mode = 2;
if (!strcmp(inpstr,"yes")) mode = 2;
if (!strcmp(inpstr,"0"))   mode = 1;
if (!strcmp(inpstr,"no"))  mode = 1;

if (!mode)
  {
    sscanf(inpstr, "%s", other_user);
    strtolower(other_user);

    if ((user2 = get_user_num(other_user, user)) == -1 ) {
       if (!check_for_user(other_user))
         write_str(user,NO_USER_STR);
       else
         not_signed_on(user,other_user);
       return;
       }    
    if (ustr[user2].afk)
     {
    if (ustr[user2].afk == 1) {
      if (!strlen(ustr[user2].afkmsg))
       sprintf(t_mess,"- %s is Away From Keyboard -",ustr[user2].say_name);
      else
       sprintf(t_mess,"- %s %-45s -(A F K)",ustr[user2].say_name,ustr[user2].afkmsg);
      }
     else {
      if (!strlen(ustr[user2].afkmsg))
      sprintf(t_mess,"- %s is blanked AFK (is not seeing this) -",ustr[user2].say_name);
      else
      sprintf(t_mess,"- %s %-45s -(B A F K)",ustr[user2].say_name,ustr[user2].afkmsg);
      }

       write_str(user,t_mess);
       return;
     }

    mode = 3;
  }
  

if (fight.issued && mode == 3)
  {
    write_str(user, "Sorry, you must wait until the others are done.");
    return;
  }
  
if (!fight.issued && (mode == 1 || mode == 2) )
  {
    write_str(user, "You are not being challenged to a fight at this time.");
    return;
  }
  
if ((mode == 1 || mode == 2) && fight.second_user != user)
  {
    write_str(user, "You are not the challenged user...type .fight to see");
    return;
  }
  
if (mode == 3)
  {
   if (user == user2)
     {
       write_str(user,"You need help! (Fighting yourself...tsk tsk tsk)");
       return;
     }
     
/*----------------------------------------------------*/
/* check for standard fight room                      */
/*----------------------------------------------------*/

   if (FIGHT_ROOM != -1) 
     {
       if (ustr[user].area == FIGHT_ROOM)
         {
           sprintf(t_mess,"Noone can fight in %s.",astr[FIGHT_ROOM].name);
           write_str(user,t_mess);
           return;
         }
         
       if (ustr[user2].area != ustr[user].area)
         {
           sprintf(t_mess,"%s is not here to fight you.",ustr[user2].say_name);
           write_str(user,t_mess);
           
           sprintf(t_mess,"%s wanted to fight you, but you must be in the same room to do that.",
                           ustr[user].say_name);
           write_str(user2,t_mess);
           return;
         }
     }
     
   issue_chal(user, user2);
   return;
  }

if (mode == 1)
  {
   sprintf(mess, wimp_text[ rand() % num_wimp_text ], ustr[user].say_name);
   writeall_str(mess,1,user,0,user,NORM,FIGHT,0);
   write_str(user,mess);
   reset_chal(user, inpstr);
   return;
  }
  
if (mode == 2)
  {
   accept_chal(user);
   reset_chal(user, inpstr);
   return;
  }
  
}

/* DNS resolver */
void resolve_names_set(int user)
{
  if (resolve_names==0) {
   resolve_names = 1;
   btell(user," Site name resolver now configured for talker-wide cache");
   }
  else if (resolve_names==1) {
   resolve_names = 2;
   btell(user," Site name resolver now configured for site-wide cache");
   }
  if (resolve_names==2) {
   resolve_names = 0;
   btell(user," Site name resolver now turned off");
   }
}

/* Atmospheric control */
void toggle_atmos(int user, char *inpstr)
{
char line[150];
int a=0;
int found=0;

if (!strlen(inpstr)) {
   write_str(user,"Specify a room name or \"all\" for all rooms.");
   return;
   }

if (!strcmp(inpstr,"all")) {
  if (atmos_on)
    {
      write_str(user,"Atmospherics OFF for all rooms");
      atmos_on=0;
      for (a=0;a<MAX_AREAS;++a) {
          astr[a].atmos=0;
          }  
      sprintf(line,"ATMOS disabled by %s for all rooms\n",ustr[user].say_name);
    }
   else
    {
      write_str(user,"Atmospherics ON for all rooms");
      atmos_on=1;  
      for (a=0;a<MAX_AREAS;++a) {
          astr[a].atmos=1;
          }  
      sprintf(line,"ATMOS enabled by %s for all rooms\n",ustr[user].say_name);
    }
  }

 else {

  if (strlen(inpstr) > NAME_LEN) 
     {
      write_str(user,"Room name length too long.");
      return;
     }

   /*--------------------*/
   /* see if area exists */
   /*--------------------*/

   found = FALSE;
   for (a=0; a < NUM_AREAS; ++a)
    { 
     if (!strcmp(astr[a].name, inpstr) )
       { 
         found = TRUE;
         break;
       }
    }
 
   if (!found)
     {
      write_str(user, NO_ROOM);
      return;
     }

  if (astr[a].atmos) {
   astr[a].atmos=0;
   sprintf(line,"ATMOS disabled by %s for %s\n",ustr[user].say_name,astr[a].name);
   }
  else {
   astr[a].atmos=1;
   atmos_on=1;
   sprintf(line,"ATMOS enabled by %s for %s\n",ustr[user].say_name,astr[a].name);
   }

 }  /* end of else */

 print_to_syslog(line);
 btell(user,line);
}	

/* New user creation control */		 
void toggle_allow(int user, char *inpstr)
{
char line[132];

  if (allow_new > 1)
    {
      write_str(user,"New users DISALLOWED totally");
      allow_new=0;
      sprintf(line,"NEW users disallowed totally by %s\n",ustr[user].say_name);
    }
   else if (allow_new==1)
    {
      write_str(user,"New users ALLOWED freely");
      allow_new=2;
      sprintf(line,"NEW users allowed freely by %s\n",ustr[user].say_name);
    }
   else
    {
      write_str(user,"New users ALLOWED w/email verify");
      allow_new=1;
      sprintf(line,"NEW users allowed v/email verify by %s\n",ustr[user].say_name);
    }
    
 print_to_syslog(line);
 btell(user,line);
}

/*--------------------------------------------------------------*/
/* selective line removal from files                            */
/*--------------------------------------------------------------*/
void remove_lines_from_file(int user, char *file, int lower, int upper)
{
int mode  = 0;
char temp[FILE_NAME_LEN];
FILE *bfp,*tfp;

/*---------------------------------------------------------*/
/* determine the mode for line deletion                    */
/*---------------------------------------------------------*/

if (lower == -1 && upper == -1)       mode = 1;  /* all lines         */
else if (lower == upper)              mode = 2;  /* one line          */
else if (lower > 0 && upper == 0)     mode = 3;  /* to end of file    */
else if (upper > 0 && lower == 0)     mode = 4;  /* from beginning    */
else if (upper < lower)               mode = 5;  /* leave middle      */

/*---------------------------------------------------------*/
/* check to make sure the file exists                      */
/*---------------------------------------------------------*/

if (!(bfp=fopen(file,"r"))) 
  {
   write_str(user,"The file was empty, could not delete");  
   return;
  }
  
  
/*-------------------------------------------*/ 
/* delete the entire file                    */
/*-------------------------------------------*/
if (mode == 1)
  {
   FCLOSE(bfp);
   remove(file);
   return;
  }         
  
/*---------------------------------------------------------*/
/* make temp file                                          */
/*---------------------------------------------------------*/

strcpy(temp,get_temp_file());

if (!(tfp=fopen(temp,"w"))) 
  {
   write_str(user,"Sorry - Cannot open temporary file");
   logerror("Can't open temporary file");
   FCLOSE(tfp);
   return;
  }
  
/*------------------------------------------------------*/
/* get the right lines from the file                    */
/*------------------------------------------------------*/

switch(mode)
  {
    case 1: break;   /* already done */
    
    case 2:
            file_copy_lines(bfp, tfp, lower);
            file_skip_lines(bfp, (upper - lower) + 2 );
            file_copy_lines(bfp, tfp, 99999);
            break;
            
    case 3:
            file_copy_lines(bfp, tfp, lower);
            break;
            
    case 4:
            file_skip_lines(bfp, upper + 1);
            file_copy_lines(bfp, tfp, 99999);
            break;
            
    case 5: 
            file_skip_lines(bfp, upper );
            file_copy_lines(bfp, tfp, (lower - upper) + 2 );
            break;
            
    default: break;
  }
         
FCLOSE(bfp);  
FCLOSE(tfp);

remove(file);

/*-----------------------------------------*/
/* copy temp file back into file           */
/*-----------------------------------------*/

if (!(bfp=fopen(file,"w"))) 
  {
   return;
  }
  
if (!(tfp=fopen(temp,"r"))) 
  {
   FCLOSE(bfp);
   return;
  }
  
file_copy(tfp, bfp);
  
FCLOSE(bfp);  
FCLOSE(tfp);
remove(temp);

}

/*---------------------------------------------*/
/* skip the number of lines specified          */
/*---------------------------------------------*/
void file_skip_lines(FILE *in_file, int lines)
{
int cnt = 1;
char c;

 while( cnt < lines ) 
   {
    c=getc(in_file);
    if (feof(in_file)) return;
    if (c == '\n') cnt++;
   }
}

/*-----------------------------------------------------------------*/
/* copy the number of lines specified from a file to a file        */
/*-----------------------------------------------------------------*/
void file_copy_lines(FILE *in_file, FILE *out_file, int lines)
{
int cnt = 1;
char c;

 while( cnt < lines ) 
   {
    c=getc(in_file);
    if (feof(in_file)) return;
    
    putc(c, out_file);
    if (c == '\n') cnt++;
   }
}

/*---------------------------------------------*/
/* copy a file to another file                 */
/*---------------------------------------------*/
void file_copy(FILE *in_file, FILE *out_file)
{
char c;
 c=getc(in_file);
 while( !feof(in_file) ) 
   {
    putc(c,out_file);
    c=getc(in_file);
   }
}

/*---------------------------------------------*/
/* count lines in a file                       */
/*---------------------------------------------*/
int file_count_lines(char *file)
{
 int lines = 0;
 char c[257];
 FILE *bfp;
 

 if (!(bfp=fopen(file,"r"))) 
   {
    return 0;
   }
 
 fgets(c, 256, bfp);
 
 while( !feof(bfp) ) 
   {
    if (strchr(c, '\n') != NULL) lines ++;
    fgets(c, 256, bfp);
   }
   
 FCLOSE(bfp);
 return lines;
}

/*---------------------------------------------*/
/* get upper/lower bounds                      */
/*---------------------------------------------*/
void get_bounds_to_delete(char *str, int *lower, int *upper, int *mode)
{
char token1[20];

int  val_1 = -1;
int  val_2 = -1;

lower[0] = -1;
upper[0] = -1;
mode[0]  = 0;

if (strlen(str)) 
  {
    sscanf(str,"%s ",t_mess);
    remove_first(str);     
    strncpy(token1, t_mess, 19);
    sscanf(token1, "%d", &val_2);
    if (strlen(str))
      {
       sscanf(str,"%s ",t_mess);
       sscanf(t_mess, "%d", &val_1);
     }
    else
     {
      t_mess[0] = 0;
      val_1 = -1;
     }
    
    /*------------------*/
    /* delete all lines */
    /*------------------*/
    if (strcmp(token1,"all") == 0)
      {
       lower[0] = 0;
       upper[0] = 0;
       mode[0] = 1;
       return;
      }
      
    /*------------------*/
    /* delete to end    */
    /*------------------*/
    if (strcmp(token1,"from") == 0)
      {
       if (val_1 > 0)
        {
         lower[0] = val_1;
         upper[0] = 0;
         mode[0] = 3;
        }
       return;
      }
      
    /*-------------------------*/
    /* delete from begining    */
    /*-------------------------*/
    if (strcmp(token1,"to") == 0)
      {
       if (val_1 > 0)
        {
         lower[0] = 0;
         upper[0] = val_1;
         mode[0]  = 4;
        }
       return;
      }
    /*-------------------------*/
    /* delete from begining    */
    /*-------------------------*/
    if (val_2 > 0)
      {
       if (val_1 > 0)
        {
         lower[0] = val_1;
         upper[0] = val_2;
         if (val_1 == val_2) 
           mode[0] = 2;
          else
           mode[0]  = 5;
         return;
        }
       lower[0] = val_2;
       upper[0] = val_2;
       mode[0]  = 2;
       return;
      }
  }
 else
  {
   /* delete the first line (default) */
   lower[0] = 1;
   upper[0] = 1;
   mode[0]  = 2; 
  }
  
}

/*--------------------------------------------------------------*/
/* create the away from keyboard command                        */
/*--------------------------------------------------------------*/
void set_afk(int user, char *inpstr)
{
int i;
char nbuffer[ARR_SIZE];

if (!AFK_NERF) {
 if (!strcmp(astr[ustr[user].area].name,NERF_ROOM)) {
     write_str(user,"That command is not allowed in this room.");
     return;
     }
 }

i = rand() % NUM_IDLE_LINES;

if ((inpstr[0]=='-') && (inpstr[1]=='l') ) {
  sscanf(inpstr,"%s ",nbuffer);
  }
else {
  strcpy(nbuffer,inpstr);
  }

if (strlen(nbuffer) > DESC_LEN) {
   write_str(user,"Message too long...not set afk");
   return;
   }
if ((strlen(nbuffer) > 1) && (strcmp(nbuffer,"-l"))) 
 {
  strcpy(ustr[user].afkmsg,nbuffer);
  strcat(ustr[user].afkmsg,"@@");
  if (!ustr[user].vis)
   sprintf(mess,"- %s %-41s -(A F K)",INVIS_ACTION_LABEL,ustr[user].afkmsg);
  else
   sprintf(mess,"- %s %-41s -(A F K)",ustr[user].say_name,ustr[user].afkmsg);
 }
else if ((strlen(nbuffer) > 1) && (!strcmp(nbuffer,"-l")) ) {
 remove_first(inpstr);
 if (strlen(inpstr) > DESC_LEN) {
    write_str(user,"Message too long..not set afk");
    return;
    }
 else if (!strlen(inpstr)) {
    if (!ustr[user].vis)
     sprintf(mess, idle_text[i], INVIS_ACTION_LABEL, "(A F K)");
    else
     sprintf(mess, idle_text[i], ustr[user].say_name, "(A F K)");
    strcpy(ustr[user].afkmsg,"");
    }
 else {
    strcpy(ustr[user].afkmsg,inpstr);
    strcat(ustr[user].afkmsg,"@@");
    if (!ustr[user].vis)
     sprintf(mess,"- %s %-41s -(A F K)",INVIS_ACTION_LABEL,ustr[user].afkmsg);
    else
     sprintf(mess,"- %s %-41s -(A F K)",ustr[user].say_name,ustr[user].afkmsg);
    }
 writeall_str(mess,1,user,0,user,NORM,AFK_TYPE,0);
 write_str(user,mess);
 write_str(user,"");
 write_str(user,"THIS TERMINAL IS NOW LOCKED.");
 write_str_nr(user,"Enter your account password to return: ");
 telnet_echo_off(user);
 telnet_write_eor(user);
 ustr[user].afk = 1;
 ustr[user].lockafk=1;
 noprompt=1;
 return;
 }
else {
 if (!ustr[user].vis)
  sprintf(mess, idle_text[i], INVIS_ACTION_LABEL, "(A F K)");
 else
  sprintf(mess, idle_text[i], ustr[user].say_name, "(A F K)");
 strcpy(ustr[user].afkmsg,"");
 }
writeall_str(mess,1,user,0,user,NORM,AFK_TYPE,0);
write_str(user,mess);

ustr[user].afk = 1;
}

/*--------------------------------------------------------------*/
/* create the boss - away from keyboard command                 */
/*--------------------------------------------------------------*/
void set_bafk(int user, char *inpstr)
{
int i;
char obuffer[ARR_SIZE];

if (!AFK_NERF) {
 if (!strcmp(astr[ustr[user].area].name,NERF_ROOM)) {
     write_str(user,"That command is not allowed in this room.");
     return;
     }
 }

i = rand() % NUM_IDLE_LINES;

if ((inpstr[0]=='-') && (inpstr[1]=='l') ) {
  sscanf(inpstr,"%s ",obuffer);
  }
else {
  strcpy(obuffer,inpstr);
  }

if (strlen(obuffer) > DESC_LEN) {
   write_str(user,"Message too long...not set afk");
   return;
   }
if ((strlen(obuffer) > 1) && (strcmp(obuffer,"-l")) ) 
 {
  strcpy(ustr[user].afkmsg,obuffer);
  strcat(ustr[user].afkmsg,"@@");
  if (!ustr[user].vis)
   sprintf(mess,"- %s %-41s -(B A F K)",INVIS_ACTION_LABEL,ustr[user].afkmsg);
  else
   sprintf(mess,"- %s %-41s -(B A F K)",ustr[user].say_name,ustr[user].afkmsg);
 }
else if ((strlen(obuffer) > 1) && (!strcmp(obuffer,"-l")) ) {
 remove_first(inpstr);
 if (strlen(inpstr) > DESC_LEN) {
    write_str(user,"Message too long..not set afk");
    return;
    }
 else if (!strlen(inpstr)) {
    if (!ustr[user].vis)
     sprintf(mess, idle_text[i], INVIS_ACTION_LABEL, "(B A F K)");
    else
     sprintf(mess, idle_text[i], ustr[user].say_name, "(B A F K)");
    strcpy(ustr[user].afkmsg,"");
    }
 else {
    strcpy(ustr[user].afkmsg,inpstr);
    strcat(ustr[user].afkmsg,"@@");
    if (!ustr[user].vis)
     sprintf(mess,"- %s %-41s -(B A F K)",INVIS_ACTION_LABEL,ustr[user].afkmsg);
    else
     sprintf(mess,"- %s %-41s -(B A F K)",ustr[user].say_name,ustr[user].afkmsg);
    }
 writeall_str(mess,1,user,0,user,NORM,AFK_TYPE,0);
 write_str(user,mess);
 cls(user);
 write_str(user,"");
 write_str(user,"THIS TERMINAL IS NOW LOCKED.");
 write_str_nr(user,"Enter your account password to return: ");
 telnet_echo_off(user);
 telnet_write_eor(user);
 ustr[user].afk = 2; 
 ustr[user].lockafk=1;
 noprompt=1;
 return;
 }
else {
 if (!ustr[user].vis)
  sprintf(mess, idle_text[i], INVIS_ACTION_LABEL, "(B A F K)");
 else
  sprintf(mess, idle_text[i], ustr[user].say_name, "(B A F K)");
 strcpy(ustr[user].afkmsg,"");
 }
writeall_str(mess,1,user,0,user,NORM,AFK_TYPE,0);
write_str(user,mess);
cls(user);
ustr[user].afk = 2;

}

/* Give time info for major timezones or whatever zone user */
/* asks for that we can show                                */
void systime(int user, char *inpstr)
{
char buf1[100];
char tzmess[500];
char wcd[FILE_NAME_LEN+10];
struct tm *clocker;
time_t tm_now;

/* Get the current working directory so we can reference */
/* to the absolute path of the TZ info                   */
getcwd(wcd,FILE_NAME_LEN);
strcat(wcd,"/tzinfo");

if (!strlen(inpstr)) {
write_str(user,"+-----------------------------------------------------------------------------+");

/*
#if defined(__bsdi__) || defined(__linux__)
unsetenv("TZ");
goto START;
#endif
*/

if (!strcmp(TZONE,"localtime")) {
#if defined(__FreeBSD__) || defined(__NetBSD__)
putenv("TZ=:/etc/localtime");
#else
putenv("TZ=localtime");      
#endif  
}
else {
sprintf(tzmess,"TZ=:%s/%s",wcd,TZONE);
putenv(tzmess);
}
/* to make sure we get no warnings about not using a defined label */
goto START;

START:
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"System time is (%s)%-*s : %s",TZONE,23-strlen(TZONE),"",buf1);
write_str(user,mess);
write_str(user," ");

sprintf(tzmess,"TZ=:%s/US/Eastern",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"US Eastern Time                          : %s",buf1);
write_str(user,mess);

sprintf(tzmess,"TZ=:%s/US/Central",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"US Central Time                          : %s",buf1);
write_str(user,mess);

sprintf(tzmess,"TZ=:%s/US/Mountain",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"US Mountain Time                         : %s",buf1);
write_str(user,mess);

sprintf(tzmess,"TZ=:%s/US/Pacific",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"US Pacific Time                          : %s",buf1);
write_str(user,mess);

sprintf(tzmess,"TZ=:%s/Brazil/East",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"East Brasilia Time                       : %s",buf1);
write_str(user,mess);

sprintf(tzmess,"TZ=:%s/Greenwich",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"Greenwich Mean Time                      : %s",buf1);
write_str(user,mess);

sprintf(tzmess,"TZ=:%s/CET",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"Central European Time                    : %s",buf1);
write_str(user,mess);

sprintf(tzmess,"TZ=:%s/Australia/NSW",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"Australian NSW Time                      : %s",buf1);
write_str(user,mess);

sprintf(tzmess,"TZ=:%s/NZ",wcd);
putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"New Zealand Time                         : %s",buf1);
write_str(user,mess);
write_str(user,"+-----------------------------------------------------------------------------+");
}
else if (!strcmp(inpstr,"-l")) {
 write_str(user,"japan    ausnorth    aussouth    auswest    hawaii");
 write_str(user,"hongkong singapore   israel      cuba       mexico");
 write_str(user,"egypt    chile       eeurope     weurope    meurope");
 write_str(user,"zulu");
}
else {
 strtolower(inpstr);
 if (!strcmp(inpstr,"japan"))
  sprintf(tzmess,"TZ=:%s/Japan",wcd);
 else if (!strcmp(inpstr,"ausnorth"))
  sprintf(tzmess,"TZ=:%s/Australia/North",wcd);
 else if (!strcmp(inpstr,"aussouth"))
  sprintf(tzmess,"TZ=:%s/Australia/South",wcd);
 else if (!strcmp(inpstr,"auswest"))
  sprintf(tzmess,"TZ=:%s/Australia/West",wcd);
 else if (!strcmp(inpstr,"hawaii"))
  sprintf(tzmess,"TZ=:%s/US/Hawaii",wcd);
 else if (!strcmp(inpstr,"hongkong"))
  sprintf(tzmess,"TZ=:%s/Hongkong",wcd);
 else if (!strcmp(inpstr,"singapore"))
  sprintf(tzmess,"TZ=:%s/Singapore",wcd);
 else if (!strcmp(inpstr,"israel"))
  sprintf(tzmess,"TZ=:%s/Israel",wcd);
 else if (!strcmp(inpstr,"cuba"))
  sprintf(tzmess,"TZ=:%s/Cuba",wcd);
 else if (!strcmp(inpstr,"mexico"))
  sprintf(tzmess,"TZ=:%s/Mexico/General",wcd);
 else if (!strcmp(inpstr,"egypt"))
  sprintf(tzmess,"TZ=:%s/Egypt",wcd);
 else if (!strcmp(inpstr,"chile"))
  sprintf(tzmess,"TZ=:%s/Chile/Continental",wcd);
 else if (!strcmp(inpstr,"eeurope"))
  sprintf(tzmess,"TZ=:%s/EET",wcd);
 else if (!strcmp(inpstr,"weurope"))
  sprintf(tzmess,"TZ=:%s/WET",wcd);
 else if (!strcmp(inpstr,"meurope"))
  sprintf(tzmess,"TZ=:%s/MET",wcd);
 else if (!strcmp(inpstr,"zulu"))
  sprintf(tzmess,"TZ=:%s/Zulu",wcd);
 else {
   write_str(user,"Zone doesn't exist. \".time -l\" for list");
   return;
  }

putenv(tzmess);
#if !defined(__CYGWIN32__)
tzset();
#endif
time(&tm_now);
clocker=localtime(&tm_now);
strftime(buf1,sizeof(buf1),"%a, %b %d %I:%M:%S %p %Y ",clocker);
sprintf(mess,"%s Time%-*s: %s",inpstr,36-strlen(inpstr),"",buf1);
mess[0]=toupper((int)mess[0]);
write_str(user,"+-----------------------------------------------------------------------------+");
write_str(user,mess);
write_str(user,"+-----------------------------------------------------------------------------+");
}

/* Reset time back to local time */

/*
#if defined(__bsdi__) || defined(__linux__)
unsetenv("TZ");
goto SSTART;
#endif
*/

if (!strcmp(TZONE,"localtime")) {
#if defined(__FreeBSD__) || defined(__NetBSD__)
putenv("TZ=:/etc/localtime");
#else
putenv("TZ=localtime");      
#endif  
}
else {
sprintf(tzmess,"TZ=:%s/%s",wcd,TZONE);
putenv(tzmess);
}
/* to make sure we get no warnings about not using a defined label */
goto SSTART;
 
SSTART:
#if !defined(__CYGWIN32__)
tzset();
#endif

}


/*--------------------------------------------------------------*/
/* meter command                                                */
/*--------------------------------------------------------------*/
void meter(int user, char *inpstr)
{
int i=0;
int j=0;
char datestr[80];
char daystr[10];
char timebuf[80];
char graph[26];
time_t tm;

if (!strlen(inpstr)) {
sprintf(mess,"+-------------------------------------------+");
write_str(user, mess);

sprintf(mess,"|         Activity meter                    |");
write_str(user, mess);

sprintf(mess,"+-------------------------------------------+");
write_str(user, mess);

sprintf(mess,"| Commands this period: %4d                |",commands);
write_str(user, mess);

sprintf(mess,"|                                           |");
write_str(user, mess);

fill_bar(says, commands, graph);
sprintf(mess,"| says  %s              |", graph);
write_str(user, mess);

fill_bar(tells, commands, graph);
sprintf(mess,"| tells %s              |", graph);
write_str(user, mess);

sprintf(mess,"|                                           |");
write_str(user, mess);

sprintf(mess,"| Commands per minute ave: %4d             |",commands_running);
write_str(user, mess);

sprintf(mess,"| Tells per minute ave:    %4d             |",tells_running);
write_str(user, mess);

sprintf(mess,"| Says per minute ave:     %4d             |",says_running);
write_str(user, mess);

sprintf(mess,"+-------------------------------------------+");
write_str(user, mess);
}

/* Do user activity graph */
else if (!strcmp(inpstr,"-l")) {

time(&tm);

strcpy(datestr,ctime(&tm));
midcpy(datestr,daystr,0,2);

time(&tm);
strcpy(datestr,ctime(&start_time));

if (!strcmp(daystr,"Sun"))
 strcpy(daystr,"Sunday");
else if (!strcmp(daystr,"Mon"))
 strcpy(daystr,"Monday");
else if (!strcmp(daystr,"Tue"))
 strcpy(daystr,"Tuesday");
else if (!strcmp(daystr,"Wed"))
 strcpy(daystr,"Wednesday");
else if (!strcmp(daystr,"Thu"))
 strcpy(daystr,"Thursday");
else if (!strcmp(daystr,"Fri"))
 strcpy(daystr,"Friday");
else if (!strcmp(daystr,"Sat"))
 strcpy(daystr,"Saturday");

write_str(user,"");
sprintf(mess,"User login activity for ^%s^",daystr);
write_str(user,mess);
datestr[strlen(datestr)-1]=0;
sprintf(mess,"System Booted: %s",datestr);
write_str(user,mess);
write_str(user,"");
write_str(user,"LOGS");

timebuf[0]=0;

/* If the hour's login numbers fall into the range, put aa asterix */
/* in the buffer, else put a space                                 */

strcpy(timebuf,"100+  | ");

for (i=0;i<24;++i) {
   if (logstat[i].logins > 100)
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"91-100| ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 90) && (logstat[i].logins <= 100))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"81-90 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 80) && (logstat[i].logins <= 90))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"71-80 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 70) && (logstat[i].logins <= 80))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"61-70 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 60) && (logstat[i].logins <= 70))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"51-60 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 50) && (logstat[i].logins <= 60))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"41-50 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 40) && (logstat[i].logins <= 50))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"31-40 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 30) && (logstat[i].logins <= 40))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"21-30 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 20) && (logstat[i].logins <= 30))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"11-20 | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 10) && (logstat[i].logins <= 20))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

strcpy(timebuf,"0-10  | ");

for (i=0;i<24;++i) {
   if ((logstat[i].logins > 0) && (logstat[i].logins <= 10))
    strcat(timebuf,"*  ");
   else
    strcat(timebuf,"   ");
 if (i==23) {
   j=strlen(timebuf);
   timebuf[j-2]=0;
  }
}

write_str(user,timebuf);
i=0;
j=0;

write_str(user,"      +-|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|");
write_str(user,"      12a  1  2  3  4  5  6  7  8  9 10 11 12p 1  2  3  4  5  6  7  8  9 10 11");

} /* end of else if */

else {
 write_str(user,"Invalid option.");
 write_str(user,"Usage: .meter [-l]");
 }

}

void fill_bar(int val1, int val2, char *str)
{
 int i, j;

  strcpy(str,"|--------------------|");

  val1 = val1 * 100;
  val2 = val2;

  if (val2 == 0) return;
  i = (val1 / val2) / 5;
  
  if (i > 20) i = 20;
  
  if (i == 0) return;

  for(j = 0; j<i; j++)
   { 
     str[1+j] = '#';
   }
}

/*------------------------------------------------*/
/* set new user quota                             */
/*------------------------------------------------*/
void set_quota(int user, char *inpstr)
{

  int value = 0;
  
  sscanf(inpstr,"%d", &value);
  
  if (value < 0)
    {
      value = 0;
    }
    
  sprintf(mess,"The new quota is: %d",value);
  write_str(user,mess);

  system_stats.logins_today++;    
  system_stats.logins_since_start++;
  system_stats.new_since_start++;
  system_stats.new_users_today    = 0;
  system_stats.quota              = value;
}

/*------------------------------------------------*/
/* IMPORTANT: make sure there are at least 10     */
/* extra bytes in the string to append on         */
/*------------------------------------------------*/
void add_hilight(char *str)
{
char buff[ARR_SIZE];

strcat(str,"^");
strcpy(buff,str);
strcpy(str,"^ ");
strcat(str,buff);

}


/*-----------------------------------------------*/
/* x communicate a user                          */
/*-----------------------------------------------*/
void xcomm(int user, char *inpstr)
{
char buf1[256];
char other_user[ARR_SIZE];
int u,inlen;
unsigned int i;

if (!strlen(inpstr)) 
  {
   write_str(user,"Users XCOMMed & logged on     Time left"); 
   write_str(user,"-------------------------     ---------"); 
   for (u=0;u<MAX_USERS;++u) 
    {
     if (ustr[u].suspended  && ustr[u].area > -1) 
       {
        if (ustr[u].xco_time == 0)
           sprintf(mess,"%-29s %s",ustr[u].say_name,"Perm");
        else
           sprintf(mess,"%-29s %s",ustr[u].say_name,converttime((long)ustr[u].xco_time));
        write_str(user, mess);
       };
    }
   write_str(user,"(end of list)");
   return;
  }

sscanf(inpstr,"%s ",other_user);
strtolower(other_user);

if ((u=get_user_num(other_user,user))== -1) 
  {
   not_signed_on(user,other_user);
   return;
  }
  
if (u == user)
  {   
   write_str(user,"You are definitly wierd! Trying to xcom yourself, geesh."); 
   return;
  }

 if ((!strcmp(ustr[u].name,ROOT_ID)) || (!strcmp(ustr[u].name,BOT_ID)
      && strcmp(ustr[user].name,ROOT_ID))) {
    write_str(user,"Yeah, right!");
    return;
    }

if (ustr[user].tempsuper <= ustr[u].super) 
  {
   write_str(user,"That would not be wise..");   
   sprintf(mess,XCOMM_CANT,ustr[user].say_name);
   write_str(u,mess);
   return;
  }

if (ustr[u].suspended == 0) {
remove_first(inpstr);
if (strlen(inpstr) && strcmp(inpstr,"0")) {
   if (strlen(inpstr) > 5) {
      write_str(user,"Minutes can't exceed 5 digits.");
      return;
      }
   inlen=strlen(inpstr);
   for (i=0;i<inlen;++i) {
     if (!isdigit((int)inpstr[i])) {
        write_str(user,"Numbers only!");
        return;
        }
     }
    i=0;
    i=atoi(inpstr);
    if ( i > 32767) {
       write_str(user,"Minutes can't exceed 32767.");
       i=0;
       return;
      }
  i=0;
  ustr[u].xco_time=atoi(inpstr);
  ustr[u].suspended = 1;
    write_str(u,XCOMMON_MESS);
    sprintf(mess,"XCOM ON : %s by %s for %s\n",ustr[u].say_name,
ustr[user].say_name, converttime((long)ustr[u].xco_time));
}
else {
    ustr[u].suspended = 1;
    ustr[u].xco_time=0;
    write_str(u,XCOMMON_MESS);
    sprintf(mess,"XCOM ON : %s by %s\n",ustr[u].say_name, ustr[user].say_name);
  }
}     /* end of if suspended */

else {
    ustr[u].suspended = 0;
    ustr[u].xco_time=0;
    write_str(u,XCOMMOFF_MESS);
    sprintf(mess,"XCOM OFF: %s by %s\n",ustr[u].say_name, ustr[user].say_name);
  } 
btell(user, mess);

 strcpy(buf1,get_time(0,0));    
 strcat(buf1," ");
 strcat(buf1,mess);
print_to_syslog(buf1);

write_str(user,"Ok");
}

/*-------------------------------------------------------------------*/
/* Set up main listening sockets                                     */
/*-------------------------------------------------------------------*/
void make_sockets()
{
struct sockaddr_in bind_addr;       /* AF_INET sockaddr structure */
int i,open_port=0;
int on=1;
int size=sizeof(struct sockaddr_in);
#if defined(WIN32) && !defined(__CYGWIN32__)
unsigned long arg = 1;
#endif

/* Zero out memory for address */
#if defined(HAVE_BZERO)
 bzero((char *)&bind_addr, size);
#else
 memset((char *)&bind_addr, 0, size);
#endif

for (i=0;i<4;++i) {

/*---------------------------------------*/
/* status text                           */
/*---------------------------------------*/
if (i==0) {
 open_port = PORT;
 printf("Main talker port:\n\n");
 printf("   use port: %d\n",open_port);
 }
else if (i==1) {   
 if (WIZ_OFFSET != 0) {
 open_port = PORT + WIZ_OFFSET;
 printf("Wizard port:\n\n");
 printf("   use port: %d\n",open_port);
 }
 else { wiz_access=0; continue; }
 }
else if (i==2) {
 if (WHO_OFFSET != 0) {
 open_port = PORT + WHO_OFFSET;
 printf("External WHO list port:\n\n");
 printf("   use port: %d\n",open_port);
 }
 else { who_access=0; continue; }
 }
else if (i==3) {   
 if (WWW_OFFSET != 0) {
 open_port = PORT + WWW_OFFSET;
 printf("Mini WWW port:\n\n");
 printf("   use port: %d\n",open_port);
 }
 else { www_access=0; continue; }
 }

#if defined(__FreeBSD__)        
bind_addr.sin_len = sizeof(bind_addr);
#endif /* __FreeBSD__ */       
bind_addr.sin_family      = AF_INET;   /* setup address struct */
bind_addr.sin_addr.s_addr = INADDR_ANY; 
bind_addr.sin_port        = htons(open_port); /* with local host info */

 
/*-------------------------------------------------*/
/* Create a socket for use                         */
/*-------------------------------------------------*/

if ((listen_sock[i] = socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET)
 {
  printf("   ***CANNOT CREATE SOCKET***\n");
  printf("\n   Cannot create socket, aborting startup!\n");
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
  exit(0);
 }

/* Set address reusable */
if (setsockopt(listen_sock[i], SOL_SOCKET, SO_REUSEADDR, (char *) &on,
    sizeof(on))== -1) {
   printf("\n   Cannot setsockopt(), aborting startup!\n");
   while (CLOSE(listen_sock[i]) == -1 && errno == EINTR)
	; /* empty while */
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
   exit(0);
   }
  
/*----------------------------*/
/* Set socket to non_blocking */
/*----------------------------*/

#if defined(WIN32) && !defined(__CYGWIN32__)
 if (ioctlsocket(listen_sock[i], FIONBIO, &arg) == -1) {
#else
 if (fcntl(listen_sock[i], F_SETFL, NBLOCK_CMD)== -1) {
#endif
   printf("\n   Cannot set binding socket to non-blocking, aborting startup!\n");
   while (CLOSE(listen_sock[i]) == -1 && errno == EINTR)
	; /* empty while */
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
   exit(0);
  }

/* vax users change the above line after #else to: */
/* socket_ioctl(listen_sock[i], FIONBIO, &arg);    */
/* declare arg int arg = 1                         */
   
/*-------------------------------------------------*/
/* Bind the socket to local machine and port       */
/* giving the socket the local address ADDR        */
/*-------------------------------------------------*/

#if defined(WIN32) && !defined(__CYGWIN32__)
if (bind(listen_sock[i], (struct sockaddr *)&bind_addr, size)!= 0)
#else
if (bind(listen_sock[i], (struct sockaddr *)&bind_addr, size)== -1)
#endif
  {
   printf("   ***CANNOT BIND TO PORT***\n");
   printf("\n   Cannot bind to port, server may be already running, aborting startup!\n");
   while (CLOSE(listen_sock[i]) == -1 && errno == EINTR)
	; /* empty while */
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
   exit(0);
  }

/*------------------------------------------------------*/ 
/* Listen on the socket                                 */
/* second arg of listen() is the backlog of connections */
/* we can hold. We'll make it 5 to be safe. Linux       */
/* silently limits to 128, SunOS to 5. Go figure        */
/*------------------------------------------------------*/

#if defined(WIN32) && !defined(__CYGWIN32__)
if (listen(listen_sock[i], 5)==SOCKET_ERROR)
#else
if (listen(listen_sock[i], 5)== -1)
#endif
  {
   printf("   ***LISTEN FAILED ON PORT***\n");
   printf("\n   Cannot listen on port, aborting startup!\n");
   while (CLOSE(listen_sock[i]) == -1 && errno == EINTR)
	; /* empty while */
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
   exit(0);
  }


printf("   port created, bound, and listening\n\n");

 } /* end of for */
}


/*--------------------------------------------------------*/
/* logic to detemine if message is to be ignored          */
/*--------------------------------------------------------*/
int user_wants_message(int user, int type)
{
 if (ustr[user].flags[type] == '1')
  return 0;
 else
  return 1;
}

/*---------------------------------------------------------*/
/* get flags position                                      */
/*---------------------------------------------------------*/
int get_flag_num(char *inpstr)
{
char comstr[ARR_SIZE];
int f;

sscanf(inpstr,"%s",comstr); 
if (strlen(comstr)<2) return(-1);

for (f=0; flag_names[f].text[0]; ++f)
  {
   if (!instr2(0,flag_names[f].text,comstr,0) ) 
     return f;
  }
  
return -1;
}

/*----------------------*/
/* listen to all flags  */
/*----------------------*/
void listen_all(int user)
{
int i=0;

if (user==-1)
 strcpy(t_ustr.flags,"");
else
 strcpy(ustr[user].flags,"");

/* all 0s */
 for (i=0;i<NUM_IGN_FLAGS;++i) {
  if (user==-1)
   strcat(t_ustr.flags,"0");
  else
   strcat(ustr[user].flags,"0");
 }

}


/*----------------------*/
/* ignore all flags     */
/*----------------------*/
void ignore_all(int user)
{
int i=0;

if (user==-1)
 strcpy(t_ustr.flags,"");
else
 strcpy(ustr[user].flags,"");

/* all 1s */
 for (i=0;i<NUM_IGN_FLAGS;++i) {
  if (user==-1)
   strcat(t_ustr.flags,"1");
  else
   strcat(ustr[user].flags,"1");
 }

}


/*----------------------------------------------------------*/
/* set listening flags                                      */
/*----------------------------------------------------------*/
void user_listen(int user, char *inpstr)
{
int u;

if (!strlen(inpstr)) 
  {
   write_str(user,"^HG+------------------------------------+^"); 
   write_str(user,"^HG|^ ^HYYou can hear:^                      ^HG|^"); 
   write_str(user,"^HG+------------------------------------+^");
   for (u=1;u<NUM_IGN_FLAGS;++u) 
    {
     if (ustr[user].flags[u] != '1') 
       {
        write_str(user,flag_names[u].text);
       };
    }
   write_str(user,"(end of list)");
   return;
  }
  
if (!instr2(0,"all",inpstr,0) )
 {
  listen_all(user);
  write_str(user,"You will now hear all messages.");
  return;
 } 

u = get_flag_num(inpstr);
if (u > -1)
  {
   ustr[user].flags[u] = '0';
  
   sprintf(mess,"You are now listening to %s",flag_names[u].text);
   write_str(user,mess);
  }
 else
  {
   write_str(user,"That message type not known");
  }
		 
}

/*----------------------------------------------------------*/
/* set ignoring flags                                       */
/*----------------------------------------------------------*/
void user_ignore(int user, char *inpstr)
{
int u;

if (!strlen(inpstr)) 
  {
   write_str(user,"^HG+------------------------------------+^"); 
   write_str(user,"^HG|^ ^HYYou are ignoring:^                  ^HG|^"); 
   write_str(user,"^HG+------------------------------------+^");
   for (u=1;u<NUM_IGN_FLAGS;++u) 
    {
     if (ustr[user].flags[u] == '1') 
       {
        write_str(user,flag_names[u].text);
       };
    }
   write_str(user,"(end of list)");
   return;
  }
  
if (!instr2(0,"all",inpstr,0) )
 {
  ignore_all(user);
  write_str(user,"You are now ignoring all messages.");
  return;
 } 

u = get_flag_num(inpstr);
if (u > -1)
  {
   ustr[user].flags[u] = '1';
  
   sprintf(mess,"You are now ignoring %s.", flag_names[u].text);
   write_str(user,mess);
  }
 else
  {
   write_str(user,"That message type not known");
  }
}

/* Parse web server port input */
void parse_input(int user, char *inpstr)
{
int i=0,found=0;
char chunk[257];
char type1[80];
char command[257];
char request[257];
char servtype[257];
char filename[FILE_NAME_LEN+50];
time_t tm;

time(&tm);
inpstr[256]=0;

if (wwwport[user].method==1)
 strcpy(command,"POST");
else
 sscanf(inpstr,"%s ",command);

if (!strcmp(command,"GET")) {
   wwwport[user].method=0;
   remove_first(inpstr);
   sscanf(inpstr,"%s ",request);
   if ((request[0]!='/') || strstr(request,"..")) {
	web_error(user,NO_HEADER,NOT_FOUND);
	goto FREE;
	}
   if (!strlen(request)) {
	web_error(user,HEADER,BAD_REQUEST);
	goto FREE;
	}
   remove_first(inpstr);
   sscanf(inpstr,"%s ",servtype);
   if (!strlen(servtype) || !strstr(servtype,"HTTP")) {
	web_error(user,NO_HEADER,BAD_REQUEST);
	goto FREE;
	}
  } /* end of GET */
else if (!strcmp(command,"POST")) {
 if (wwwport[user].method==1) {
        if (strlen(inpstr)) {
        print_to_syslog("POST line: ");
        print_to_syslog(inpstr);
        print_to_syslog("\n");
        sscanf(inpstr,"%s ",request);
        }
        else return;

        if (wwwport[user].req_length) {
          if (strstr(inpstr,"=")) {
                print_to_syslog("Shortening length..\n");
                wwwport[user].req_length-=strlen(inpstr);
                print_to_syslog("Shortened\n");
                if (!strlen(wwwport[user].keypair))
		  strcpy(wwwport[user].keypair,inpstr);
		else {
		  strcat(wwwport[user].keypair," ");
		  strcat(wwwport[user].keypair,inpstr);
		  }
                print_to_syslog("Keypair: ");
                print_to_syslog(wwwport[user].keypair);
                print_to_syslog("\n");
                if (!wwwport[user].req_length) goto POST;
           }
         }
        else if (!strcmp(request,"Content-length:")) {
          print_to_syslog("Saving content length\n");
          remove_first(inpstr);
          wwwport[user].req_length=atoi(inpstr);
          print_to_syslog("Saved content length\n");
        }
        return;
   }
 else {
   wwwport[user].method=1;
   remove_first(inpstr);
   sscanf(inpstr,"%s ",request);
   remove_first(inpstr);
   sscanf(inpstr,"%s ",servtype);
   if (!strlen(servtype) || !strstr(servtype,"HTTP")) {
        web_error(user,NO_HEADER,NOT_FOUND);
        goto FREE;
        }
   }

  } /* end of POST */
else if (!strcmp(command,"HEAD")) {
   wwwport[user].method=2;
   remove_first(inpstr);
   sscanf(inpstr,"%s ",request);
   remove_first(inpstr);
   sscanf(inpstr,"%s ",servtype);
   if (!strlen(servtype) || !strstr(servtype,"HTTP")) {
	web_error(user,NO_HEADER,NOT_FOUND);
	goto FREE;
	}
  } /* end of HEAD */
else {
	web_error(user,HEADER,BAD_REQUEST);
	goto FREE;
  }

/* Strip leading slash off filename request and truncate */
midcpy(request,request,1,256);

if (!strlen(request))
 sprintf(filename,"%s/%s",WEBFILES,web_opts[0]);
else
 sprintf(filename,"%s/%s",WEBFILES,request);

if (request[0]=='_') {
	for (i=0;i<strlen(request);++i) {
	   if (request[i]=='\?') { found=1; break; }
	   }
	if (found) midcpy(request,chunk,0,i-1);
	else midcpy(request,chunk,0,257);
   }

POST:
/* If POST method, save requested url to memory so we can read later */
if (wwwport[user].method==1) {
  if (strlen(wwwport[user].keypair)) {
   strcpy(chunk,wwwport[user].file);
   strcpy(request,wwwport[user].keypair);
   found=1;
   }
  else {
   strcpy(wwwport[user].file,chunk);
   return;
   }
  }

if (strcmp(chunk,"_who") && strcmp(chunk,"_users")) {
 if (!check_for_file(filename)) {
   web_error(user,HEADER,NOT_FOUND);
   goto FREE;
   }
 }

	write_it(wwwport[user].sock,"HTTP/1.0 200 OK\n");
	sprintf(mess, "Server: %s/1.0\n",SYSTEM_NAME);
	write_it(wwwport[user].sock, mess);
	sprintf(mess,"Date: %s",ctime(&tm));
	write_it(wwwport[user].sock, mess);

if (!strcmp(chunk,"_who")) {
  external_www(wwwport[user].sock);
  goto FREE;
  }

if (!strcmp(chunk,"_users")) {
	/* If question mark not found, its a request for all users list */
	/* else for a specific user if query is greater than 1 or group */
	/* if equal to 1						*/
   if (!found)
   	external_users(user,3,NULL);
   else {
	if (wwwport[user].method != 1) {
	  midcpy(request,request,i+1,257);
	  found=0; i=0;
	  for (i=0;i<strlen(request);++i) {
	     if (request[i]=='=') { found=1; break; }
	     }
	  if (found) midcpy(request,request,i+1,257);
	  else { web_error(user,HEADER,BAD_REQUEST); goto FREE; }
	}

	if (!strlen(request))
   	 external_users(user,0,NULL);
	else if ((strlen(request)==1) && isalpha((int)request[0]))
   	 external_users(user,1,request);
	else
   	 external_users(user,2,request);
	} /* end of else */
   goto FREE;
  } /* end of _users if */

	 sprintf(mess,"Content-length: %d\n",get_length(filename));
	 write_it(wwwport[user].sock, mess);

	strcpy(type1,get_mime_type(filename));
	if (!strcmp(type1,"bad")) {
	  web_error(user,HEADER,BAD_REQUEST);
	  goto FREE;
	  }
	sprintf(mess,"Content-type: %s\n\n",type1);
        write_it(wwwport[user].sock, mess);

cat_to_sock(filename,wwwport[user].sock);

FREE:
free_sock(user,'4');

}

void web_error(int user, int header, int mode)
{
int size=0;
char message[350];
char output[ARR_SIZE];
time_t tm;

time(&tm);

switch(mode) {
	case 0: strcpy(message,""); break;
	case BAD_REQUEST: strcpy(message,"HTTP/1.0 400 Bad request\n"); break;
	case NOT_FOUND: strcpy(message,"HTTP/1.0 404 Not found\n"); break;
	default: strcpy(message,"HTTP/1.0 500 Unknown\n"); break;
  } /* end of switch */

if (header==HEADER) {
	write_it(wwwport[user].sock,message);
	sprintf(mess, "Server: %s/1.0\n",SYSTEM_NAME);
	write_it(wwwport[user].sock, mess);
	sprintf(mess,"Date: %s",ctime(&tm));
	write_it(wwwport[user].sock, mess);
	}

if (mode==0) { }
else if (mode==BAD_REQUEST) {
	strcpy(output,"\n\rYour browser sent a message this server could not understand.");
	size=strlen(output);
  }
else if (mode==NOT_FOUND) {
	strcpy(output,"<TITLE>Not Found</TITLE><H1>Not Found</H1> The requested object does not exist on this server.");
	size=strlen(output);
  }

if ((header==HEADER) && mode) {
	sprintf(mess,"Content-length: %d\n",size);
        write_it(wwwport[user].sock, mess);
        write_it(wwwport[user].sock, "Content-type: text/html\n\n");
	}

write_it(wwwport[user].sock, output);

}


/* Get length of output */
int get_length(char *filen)
{
int size=0;
char line[257];
FILE *fp;

line[0]=0;

fp=fopen(filen,"rb");


size+=fread(line, 1, sizeof line, fp);

while(!feof(fp)) {
	size+=fread(line, 1, sizeof line, fp);
} /* end of feof */

fclose(fp);

return size;
}

/* Get type of file requested by extension */
char *get_mime_type(char *filen)
{
int i=0,found=0,count=1;
char ext[10];
char fileext[10];
char buf1[81];
char mime[80];
char filename[256];
static char type[40];
FILE *tfp;

mime[0]=0;
buf1[0]=0;
ext[0]=0;
fileext[0]=0;

/* Find where extension ends to the left */
for (i=strlen(filen)-1;i!=0;--i) {
	if (filen[i]=='.') break;
   }

/* Were not gonna take extra long extensions */
if ((strlen(filen)-i) > 9) {
	strcpy(type,"bad");
	return type;
	}

/* Copy the extension to memory */
midcpy(filen,ext,i+1,ARR_SIZE);
i=0;

strtolower(ext);

sprintf(t_mess,"%s/%s",WEBFILES,web_opts[1]);
strncpy(filename,t_mess,FILE_NAME_LEN);

/* Open up the mime types file and read in each line, searching */
/* for a matching extension, and returning the associated type  */
if (!(tfp=fopen(filename,"r"))) {
	sprintf(mess,"%s: Cant open mime type file %s!",get_time(0,0),filename);
	print_to_syslog(mess);
	strcpy(type,"text/plain");
	return type;
	}

while (fgets(buf1,80,tfp) != NULL) {
	buf1[strlen(buf1)-1]=0;
	if (!strlen(buf1)) continue;
	if (buf1[0]=='#') continue;
	sscanf(buf1,"%s",mime);
	remove_first(buf1);
	/* Count up spaces in extension part ot see how many we get */
	/* to check for a match */
	for (i=0;i<strlen(buf1);++i) {
	if (buf1[i]==' ') count++;
	} /* end of for */
	i=0;
	/* Now scan them in one by one and try to match */
	for (i=0;i<count;++i) {
	sscanf(buf1,"%s",fileext);
	remove_first(buf1);
	if (!strcmp(ext,fileext)) { strcpy(type,mime); found=1; break; }
	} /* end of for */
	i=0; count=1; mime[0]=0; fileext[0]=0;
	if (!found) {
	  /* No match, bummer! well return a default */
	  strcpy(type,"text/plain");
	  }
	else {
	  /* Ah ha! Jackpot! */
	  break;
	  }
	buf1[0]=0;
  } /* end of while */
fclose(tfp);
i=0;
found=0;
count=1;

return type;
}

char *itoa(int num) {
static char num_ascii[6];

num_ascii[0]=0;
sprintf(num_ascii,"%d",num);
return num_ascii;
}

char *check_var(char *line, char *MACRO, char *Replacement) {
int index1;
int tempPointer;
char *pointer1;
char temparray[302];
char tempspace[302];
char linetemp[302];

	temparray[0]=0;
	tempspace[0]=0;
	linetemp[0]=0;

   while (1) {
	/* find string in line */
	pointer1 = (char *)(strstr(line, MACRO));
	/* if not found, exit */
	if (pointer1 == NULL) break;
	/* find at what position the result starts */
	index1 = pointer1 - line;
	/* copy the original line to a normal char */
	/* so midcpy doesn't mangle it all up      */
	strcpy(linetemp,line);
	/* copy up to the result to our output */
	midcpy(linetemp, temparray, 0, index1-1);
	/* append the replacement for it to our output */
	strcat(temparray, Replacement);
	/* ok, where's the rest of our string */
	tempPointer = index1+strlen(MACRO);
	/* copy the rest to a temp spot */
	midcpy(linetemp, tempspace, tempPointer, strlen(linetemp));
	/* cat the rest to our output */
	strcat(temparray, tempspace);
	/* make the original line equal to the output for our loop */

	line = temparray;
       }

	return line;
}

/*** prints who is on the system to requesting user ***/
void external_www(int as)
{
	int u,min,idl,invis=0;
	time_t tm;
	char an[NAME_LEN],ud[DESC_LEN+1];
	char i_buff[5];
	char filename[256];
	FILE *fp;

	time(&tm);

        write_it(as, "Content-type: text/html\n\n");
	write_it(as, "<HTML>\n");
	write_it(as, "<HEAD>\n");
	sprintf(mess, "<TITLE>%s WWW port</title>\n",SYSTEM_NAME);
	write_it(as, mess);
	write_it(as, "</HEAD>\n\r");

	if ((web_opts[4][0]=='#') || (!strstr(web_opts[4],".")))
	 sprintf(mess,"<BODY BGCOLOR=\"%s\" TEXT=\"%s\" LINK=\"%s\" VLINK=\"%s\">\n\r",web_opts[4],web_opts[5],web_opts[6],web_opts[7]);
	else
	 sprintf(mess,"<BODY BACKGROUND=\"%s\" TEXT=\"%s\" LINK=\"%s\" VLINK=\"%s\">\n\r",web_opts[4],web_opts[5],web_opts[6],web_opts[7]);

	write_it(as, mess);

	/* Write our header file */
	sprintf(mess,"%s/%s",WEBFILES,web_opts[2]);
	strncpy(filename,mess,FILE_NAME_LEN);
	if (!(fp=fopen(filename,"r"))) { }
	else {
		while (fgets(mess,256,fp) != NULL) {
		write_it(as, mess);
		}
		fclose(fp);
	     }

	write_it(as, "<center>\n");
	sprintf(mess,"<font color=\"%s\"><h1>Users currently logged into %s</h1>\n",
					web_opts[9],SYSTEM_NAME);
	write_it(as, mess);
        strcpy(filename,ctime(&tm));
        filename[24]=0;
	sprintf(mess,"<h3>on (%s)</h3>\n",filename);
	write_it(as, mess);
        filename[0]=0;
	write_it(as, "<br><center></font>\n");
        sprintf(mess,"<A HREF=\"telnet://%s:%d\">Connect to %s</a><br>",thishost,PORT,SYSTEM_NAME);
	write_it(as, mess);
        write_it(as,"</center>\n");
        write_it(as,"<table border>\n\r");
	write_it(as, "<tr><th><b>Room</b></th><th><b>Name/Description</b></th><th><b>Time on</b></th><th><b>Status</b></th><th><b>Idle</b></th></tr>\n\r");
	
	for (u=0;u<MAX_USERS;++u) {
		if ((ustr[u].area!=-1) && (!ustr[u].logging_in))  {
			if (!ustr[u].vis) { 
	            invis++;  
	            continue; 
			}
			min=(tm-ustr[u].time)/60;
			idl=(tm-ustr[u].last_input)/60;

                if (ustr[u].afk==0)
		 strcpy(ud,ustr[u].desc);
		else if ((strlen(ustr[u].afkmsg) > 1) && (ustr[u].afk>=1))
		 strcpy(ud,ustr[u].afkmsg);
                else if (!strlen(ustr[u].afkmsg) && (ustr[u].afk>=1))
                 strcpy(ud,ustr[u].desc);

			if (!astr[ustr[u].area].hidden) {
				strcpy(an,astr[ustr[u].area].name);
			}
			else { 
				strcpy(an, "        ");
			}
			if (ustr[u].afk == 1) {
				strcpy(i_buff,"AFK ");
			}
			else if (ustr[u].afk == 2) {
				strcpy(i_buff,"BAFK");
			}
			else {
				strcpy(i_buff,"Idle"); 
			}
				sprintf(mess,"<tr> <td>%s</td> <td><a href=\"http://%s:%d/_users?search=%s\">%s</a> %s</td> <td align=\"center\">%i</td> <td align=\"center\">%s</td> <td align=\"center\">%i</td></tr>\n\r",
			an,thishost,PORT+WWW_OFFSET,ustr[u].say_name,ustr[u].say_name,ud,min,i_buff,idl);
                        strcpy(mess, convert_color(mess));
			write_it(as,mess);
		}
	}
	if(invis) {
		sprintf(mess,SHADOW_WWW,invis == 1 ? "is" : "are",invis,invis == 1 ? " " : "s");
		write_it(as,mess);
	}
	sprintf(mess,USERCNT_WWW,num_of_users == 1 ? "is" : "are",num_of_users,num_of_users == 1 ? "" : "s");
	write_it(as,mess);

	write_it(as, "</table><br>\n</center>\n");

	/* Write our footer file */
	sprintf(mess,"%s/%s",WEBFILES,web_opts[3]);
	strncpy(filename,mess,FILE_NAME_LEN);
	if (!(fp=fopen(filename,"r"))) { }
	else {
		while (fgets(mess,256,fp) != NULL) {
		write_it(as, mess);
		}
		fclose(fp);
	     }

	sprintf(mess,"<br><b><center>This web page dynamically generated on %s</b></center><br>",ctime(&tm));
	write_it(as, mess);

	write_it(as, "</body>\n\r");
	write_it(as, "</html>\n\n");
}


/*** prints list of all users, lettered group users, or specific user data ***/
void external_users(int user, int mode, char *query)
{
	int as=wwwport[user].sock;
	int u,on_now=0,num=0;
	char letter;
	char small_buffer[64];
	char filename[256];
	struct dirent *dp;
	time_t tm;
	time_t tm_then;
	FILE *fp;
	DIR *dirp;

	time(&tm);

/*
	if (wwwport[user].method==1) {
	  strcpy(username,strip_user(query));
	  strcpy(password,strip_user(query));
	  }
*/

        write_it(as, "Content-type: text/html\n\n");
	write_it(as, "<HTML>\n\r");
	write_it(as, "<HEAD>\n\r");
	if (mode==0)
		sprintf(mess, "<TITLE>All users on %s</title>\n\r",SYSTEM_NAME);
	else if (mode==1)
		sprintf(mess, "<TITLE>\"%s\" users on %s</title>\n\r",query,SYSTEM_NAME);
	else if (mode==2) {
		sprintf(mess, "<TITLE>User info for %s on %s</title>\n\r",query,SYSTEM_NAME);
		}
	else if (mode==3) {
		sprintf(mess, "<TITLE>%s\'s user page</title>\n\r",SYSTEM_NAME);
		}

	write_it(as, mess);
	write_it(as, "</HEAD>\n\r");

	if ((web_opts[4][0]=='#') || (!strstr(web_opts[4],".")))
	 sprintf(mess,"<BODY BGCOLOR=\"%s\" TEXT=\"%s\" LINK=\"%s\" VLINK=\"%s\">\n\r",web_opts[4],web_opts[5],web_opts[6],web_opts[7]);
	else
	 sprintf(mess,"<BODY BACKGROUND=\"%s\" TEXT=\"%s\" LINK=\"%s\" VLINK=\"%s\">\n\r",web_opts[4],web_opts[5],web_opts[6],web_opts[7]);

	write_it(as, mess);

	/* Write our header file */
	sprintf(mess,"%s/%s",WEBFILES,web_opts[2]);
	strncpy(filename,mess,FILE_NAME_LEN);
	if (!(fp=fopen(filename,"r"))) { }
	else {
		while (fgets(mess,256,fp) != NULL) {
		write_it(as, mess);
		}
		fclose(fp);
	     }

	switch(mode) {
	case 0:
		  sprintf(mess,"<font color=\"%s\"><h3>These are all the users that exist on %s</h3></font>\n\r",web_opts[8],SYSTEM_NAME);
		  write_it(as, mess);
		  break;
	case 1:
		  sprintf(mess,"<font color=\"%s\"><h1>The \"%s\" users</h1></font>\n\r",web_opts[8],query);
		  write_it(as, mess);
		  break;
	case 2:
		  break;
	case 3:
		  break;
	default:  break;
	} /* end of switch */

/* Wont do switch() here because cases get to hairy, esp. with only 80 columns to see */
if (mode==0) {
 sprintf(mess,"%s",USERDIR);
 strncpy(filename,mess,FILE_NAME_LEN);
 
 dirp=opendir((char *)filename);
  
 if (dirp == NULL)
   {  
      strcpy(mess,"SYSTEM: Directory information not found for external_users\n");
      print_to_syslog(mess);
      write_it(as, mess);
	write_it(as, "</body>\n\r");
	write_it(as, "</html>\n\r\n\r");
      return;
   }

    write_it(as,"<ul>");

 while ((dp = readdir(dirp)) != NULL) 
   { 

    sprintf(small_buffer,"%s",dp->d_name);
        if (small_buffer[0] == '.')
         continue;

	read_user(small_buffer);

if (strstr(t_ustr.webpic,"://")) {
    sprintf(mess,"<li><a href=\"http://%s:%d/_users?search=%s\">%s</a>&nbsp;&nbsp;<img align=\"absmiddle\" src=\"pic.gif\" border=0 alt=\"User has a picture\"></li>\n\r",thishost,
		PORT+WWW_OFFSET,t_ustr.say_name,t_ustr.say_name);
    }
else {
    sprintf(mess,"<li><a href=\"http://%s:%d/_users?search=%s\">%s</a></li>\n\r",thishost,
		PORT+WWW_OFFSET,t_ustr.say_name,t_ustr.say_name);
    }
    write_it(as, mess);
    small_buffer[0]=0;
    num++;
   } /* end of directory while */
 (void)closedir(dirp);

    write_it(as,"</ul>\n");

    sprintf(mess,"<ul><b>%d user%s exist on %s</ul><br>",num,num == 1 ? "" : "s",SYSTEM_NAME);
    write_it(as,mess);
    num=0;


  } /* end of main if */
else if (mode==1) {
 sprintf(mess,"%s",USERDIR);
 strncpy(filename,mess,FILE_NAME_LEN);
 
 dirp=opendir((char *)filename);
  
 if (dirp == NULL)
   {  
      strcpy(mess,"SYSTEM: Directory information not found for external_users\n");
      print_to_syslog(mess);
      write_it(as, mess);
	write_it(as, "</body>\n\r");
	write_it(as, "</html>\n\r\n\r");
      return;
   }

    write_it(as,"<ul>");

 while ((dp = readdir(dirp)) != NULL) 
   { 

    sprintf(small_buffer,"%s",dp->d_name);
        if (small_buffer[0] == '.')
         continue;
        if (small_buffer[0] != tolower((int)query[0]))
         continue;	

	read_user(small_buffer);

if (strstr(t_ustr.webpic,"://")) {
    sprintf(mess,"<li><a href=\"http://%s:%d/_users?search=%s\">%s</a>&nbsp;&nbsp;<img align=\"absmiddle\" src=\"pic.gif\" border=0 alt=\"User has a picture\"></li>\n\r",thishost,
		PORT+WWW_OFFSET,t_ustr.say_name,t_ustr.say_name);
    }
else {
    sprintf(mess,"<li><a href=\"http://%s:%d/_users?search=%s\">%s</a></li>\n\r",thishost,
		PORT+WWW_OFFSET,t_ustr.say_name,t_ustr.say_name);
    }
    write_it(as, mess);
    small_buffer[0]=0;
   } /* end of directory while */
 (void)closedir(dirp);

    write_it(as,"</ul><br>\n\r");
  } /* end of else if */
else if (mode==2) {
	strtolower(query);
	if (!check_for_user(query)) {
	  sprintf(mess,"<h3>%s</h3><br>",NO_USER_STR);
	  write_it(as, mess);
	  }
	else {
	  read_user(query);
	  sprintf(mess,"<font color=\"%s\"><h1>%s</h1></font>\n\r",web_opts[8],t_ustr.say_name);
	  write_it(as, mess);

	  write_it(as,"<table width=100% border=0><tr>\n");
	  write_it(as,"<td valign=top width=50%>\n");

	  write_it(as,"<table valign=top border=0 cellspacing=10><tr valign=top>\n");
	  sprintf(mess,"<td align=left>With %s since</td><td align=left>%s</td>",SYSTEM_NAME,t_ustr.creation);
	  write_it(as, mess);
	  write_it(as,"</tr>\n");

		/* Is user online? */
		if ((u = get_user_num_exact(query,-1)) != -1) {
		  on_now = 1;
		  }

	if (on_now) {
	  sprintf(mess,"<tr><td align=left>Online Status</td><td align=left><font color=\"%s\"><b><blink>IS ONLINE RIGHT NOW</b></blink></font></td>",web_opts[10]);
	  write_it(as,mess);
	  }
	else {
	  tm_then=((time_t) t_ustr.rawtime);
	  sprintf(mess,"<tr><td align=left>Online Status</td><td align=left>On %s ago</td>",converttime((long)((tm-tm_then)/60)));
	  write_it(as, mess);
	  }
	  write_it(as,"</tr>\n");
	  sprintf(mess,"<tr><td align=left>Gender</td><td align=left>%s</td>",strip_color(t_ustr.sex));
	  write_it(as, mess);
	  write_it(as,"</tr>\n");
	  sprintf(mess,"<tr><td align=left>ICQ #</td><td align=left>%s</td>",strip_color(t_ustr.icq));
	  write_it(as, mess);
	  write_it(as,"</tr>\n");

	  if ((!t_ustr.semail) &&
		strstr(t_ustr.email_addr,"@") &&
		strstr(t_ustr.email_addr,".")) {
		sprintf(mess,"<tr><td align=left>Email</td><td align=left><a href=\"mailto:%s\">%s</a></td>",t_ustr.email_addr,t_ustr.email_addr);
		write_it(as, mess);
		write_it(as,"</tr>\n");
                        }

	  if ((!strstr(t_ustr.homepage,DEF_URL)) && (strstr(t_ustr.homepage,"/"))) {
		if (strstr(t_ustr.homepage,"://")) {
		sprintf(mess,"<tr><td align=left>Homepage</td><td align=left><a href=\"%s\">%s</a></td>",t_ustr.homepage,t_ustr.homepage);
		write_it(as, mess);
		write_it(as,"</tr>\n");
                           }
	    }

	  sprintf(filename,"%s/%s",PRO_DIR,query);
	  write_it(as,"<tr><td colspan=2>\n");

        if (!(fp=fopen(filename,"r"))) {
		write_it(as,"No profile. Sorry :)<br>");
	  }
        else {
                while (fgets(mess,256,fp) != NULL) {
                write_it(as, convert_color(mess));
		write_it(as,"<br>");
                }
                fclose(fp);
             }

	  write_it(as,"</td></tr>\n");

	  write_it(as,"</table>\n");
	  
	  write_it(as,"</td><td width=50%>\n");
	  if (strstr(t_ustr.webpic,"://")) {
	   sprintf(mess,"<img src=\"%s\" border=0 alt=\"%s\'s Picture\">",t_ustr.webpic,t_ustr.say_name);
	   write_it(as, mess);
	  }
	  else write_it(as,"<font size=+1><b><center>No picture available</center></b></font>");

	  write_it(as,"</td></tr></table>\n");
	  write_it(as,"<br>\n");
	  } /* end of check_for_user else */
  } /* end of else if */
else if (mode==3) {
write_it(as, "<center>\n");
write_it(as, "<font size=+1>Pick a letter of users</font><br><br>\n");
write_it(as, "<font size=+3><b>\n");
for (letter='A';isalpha((int)letter);letter++) {
	sprintf(mess,"<a href=\"http://%s:%d/_users?search=%c\">%c</a></li>\n",thishost,
                PORT+WWW_OFFSET,letter,letter);
	write_it(as, mess);
   } /* end of for */
write_it(as, "</b></font>");
write_it(as, "<br><br>");
sprintf(mess,"<a href=\"http://%s:%d/_users?search=\">List of all users</a>",
		thishost,PORT+WWW_OFFSET);
write_it(as, mess);
write_it(as, "<br><br>");

sprintf(mess, "<FORM METHOD=\"GET\" ACTION=\"http://%s:%d/_users\">",thishost,PORT+WWW_OFFSET);
write_it(as, mess);
sprintf(mess,"<font size=+1><b>Find user</b></font><br><INPUT TYPE=\"TEXT\" NAME=\"search\" SIZE=%d>",NAME_LEN+1);
write_it(as, mess);
write_it(as, "&nbsp;&nbsp;<INPUT TYPE=\"SUBMIT\" VALUE=\"Search\">");
write_it(as, "</FORM>");

write_it(as, "</center>");

  } /* end of else if */


	/* Write our footer file */
	sprintf(mess,"%s/%s",WEBFILES,web_opts[3]);
	strncpy(filename,mess,FILE_NAME_LEN);
	if (!(fp=fopen(filename,"r"))) { }
	else {
		while (fgets(mess,256,fp) != NULL) {
		write_it(as, mess);
		}
		fclose(fp);
	     }

	sprintf(mess,"<br><b><center>This web page dynamically generated on %s</b></center><br>",ctime(&tm));
	write_it(as, mess);
	write_it(as, "</body>\n\r");
	write_it(as, "</html>\n\r\n\r");

}

