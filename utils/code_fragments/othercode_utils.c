/* RoadTrip SOCKET.C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>
#if !defined(linux)
#include <sys/filio.h>
#endif /* LINUX */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>

#define IPROTO 0

static char     SccsId[] = "%W%     %G%";
#include "config.h"
#include "player.h"
#include "fix.h"

/* externs */

#ifdef SOLARIS
extern int      close(int);
extern int      socket(int, int, int);  
#if !defined(linux)
extern int      getsockopt(int, int, int, char *, int);
extern int      setsockopt(int, int, int, char *, int);
extern void     perror(char *);
extern void     bzero(char *, int);
extern int      select(int, fd_set *, fd_set *, fd_set *, structtimval *);

#endif /* LINUX */
/* extern int      bind(int, struct sockaddr *, int);  */
extern int      listen(int, int);
extern int      write(int, char *, int);
extern int      read(int, char *, int);
extern char    *sprintf(char *, char *,...);
#endif /* SOLARIS */

extern void     quit(player *, char *);
extern file     load_file();
extern char    *end_string();
extern player  *create_player();
extern void     destroy_player(), connect_to_prog();
extern int      test_receive();
extern void     handle_error(char *);
extern void     log(char *, char *);
extern void	LOGF(), TELLPLAYER();
extern void 	process_review();
#ifdef TRACK
extern int addfunction(char *);
#endif

/* interns */

int             main_descriptor, alive_descriptor,oi_test;
file            banish_file, banish_msg, full_msg, splat_msg, 
		under18_msg;
void            tell_player();


/* terminal defintitions */

struct terminal terms[] = {
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

int		valid_char_col(char ctest) {

	ctest = tolower(ctest);

	if (ctest == 'a' || ctest == 'b' || ctest == 'c' || 
	    ctest == 'g' || ctest == 'h' || ctest == 'i' ||
	    ctest == 'k' || ctest == 'p' || ctest == 'r' ||
	    ctest == 's' || ctest == 'u' || ctest == 'y') 
		return 1;
	return 0;
}


char	 	*getcolor(player * p, char col) {
	
	if (p->misc_flags & NOCOLOR && !(p->misc_flags & SYSTEM_COLOR))
		return 0;
	switch(col) {
		case 'S': case 's':
			return "\033[3m";
			break;
		case 'K': case 'k':
		     if (p->misc_flags & STOP_BAD_COLORS)
			return "";
		     else
			return "\033[5m";
			break;
		case 'U': case 'u':
			return "\033[4m";
			break;
		case 'I': case 'i':
		     if (p->misc_flags & STOP_BAD_COLORS)
			return "";
		     else
			return "\033[7m";
			break;
		case 'H': case 'h':
			return "\033[0;1m";
			break;
		case 'r':
			return "\033[0;31m";
			break;
		case 'R':
			return "\033[1;31m";
			break;
		case 'y':
			return "\033[0;33m";
			break;
		case 'Y':
			return "\033[1;33m";
			break;
		case 'g':
			return "\033[0;32m";
			break;
		case 'G':
			return "\033[1;32m";
			break;
		case 'p':
			return "\033[0;35m";
			break;
		case 'P':
			return "\033[1;35m";
			break;
		case 'c':
			return "\033[0;36m";
			break;
		case 'C':
			return "\033[1;36m";
			break;
		case 'B':
			return "\033[1;34m";
			break;
		case 'b':
			return "\033[0;34m";
			break;
		case 'a':
			return "\033[1;30m";
			break;
		case 'A':
			return "\033[0;37m";
			break;
     		case 'N': case 'n':
			if (p->misc_flags & SYSTEM_COLOR && valid_char_col(p->colorset[sys_color_atm])) 
				return getcolor(p, p->colorset[sys_color_atm]);
			else if (command_type & HIGHLIGHT && p->term)
				return terms[(p->term) - 1].bold;
			else if (p->term)
				return terms[(p->term) - 1].off;
			else return 0;
			break;
		case '^': /* unusable by the silly ressies ;-) */
			if (valid_char_col(p->colorset[SYSsc]))
				return getcolor(p, p->colorset[SYSsc]);
			else if (p->term)
				return terms[(p->term) - 1].off;
			else return 0;
			break;

		default:
			if (p->misc_flags & SYSTEM_COLOR) {
			     if (valid_char_col(col))
				return getcolor(p, p->colorset[sys_color_atm]);
			     else if (command_type & HIGHLIGHT && p->term)
				return terms[(p->term) - 1].bold;
			     else if (p->term)
				return terms[(p->term) - 1].off;
			     else
				return 0;
				}
			else if (command_type & HIGHLIGHT && p->term)		
				return terms[(p->term) - 1].bold;
			else if (p->term)
				return terms[(p->term) - 1].off;
			else return 0;
			break;
		}
	return 0;
}
void            clear_screen(player * p, char *str)
{
   if (p->term)
      tell_player(p, terms[(p->term) - 1].cls);
   else
      tell_player(p, " You have to have a termtype set for this command to"
        " work. Use the command: hitells <termtype>\n");
}


/* the hitells command */

void            hitells(player * p, char *str)
{
   char           *oldstack;
   int             i;

   oldstack = stack;
   if (!*str && !(p->term))
   {
      tell_player(p, " Format: Hitells <termtype/?/off>\n");
      return;
   }
   if (!*str)
   {
      sprintf(stack, " Hitells is on, with terminal set to %s.\n",
         terms[(p->term) - 1].name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   if (*str == '?')
   {
      strcpy(stack, " Current terminal types available : ");
      stack = strchr(stack, 0);
      for (i = 0; terms[i].name[0]!='\0'; i++)
      {
    sprintf(stack, "%s, ", terms[i].name);
    stack = strchr(stack, 0);;
      }
      stack -= 2;
      *stack++ = '.';
      *stack++ = '\n';
      *stack++ = 0;
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   if (!strcasecmp("off", str))
   {
      /* p->term = 0; */
      tell_player(p, " Hitells turned off.\n");
      if (!(p->misc_flags & NOCOLOR))
	{
	tell_player(p, " Standard color mode disabled due to lack of hitells.\n");
	p->misc_flags |= NOCOLOR;
	}
      if (p->misc_flags & SYSTEM_COLOR) {
	tell_player(p, " System colors have been disabled because hitells are off.\n");
	p->misc_flags &= ~SYSTEM_COLOR;
	}
      p->term = 0;
      return;
   }
   for (i = 0;terms[i].name[0]!='\0'; i++)
      if (!strcasecmp(str, terms[i].name))
      {
    p->term = i + 1;
    sprintf(stack, " Hitells turned on, with terminal set to %s.\n",
            terms[i].name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
      }
   sprintf(stack, " Terminal type '%s' not supported.\n"
      " Do hitells '?' to list currently supported terminals.\n", str);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
   return;
}

/* close down sockets after use */

void            close_down_socket()
{
   shutdown(main_descriptor, 2);
   close(main_descriptor);
}


/* grab the main socket */

void            init_socket(int port)
{
   struct sockaddr_in main_socket;
   int             dummy = 1;
   int             dummy2;
   char           *oldstack;
   char *hostname;
   struct hostent *hp;

   oldstack = stack;

   /* drag in all the msg files */

   banish_file = load_file("files/banish");
   banish_msg = load_file("files/banish.msg");
   full_msg = load_file("files/full.msg");

   /* grab the main socket */

   hostname=(char *)malloc(101);
   bzero((char *)&main_socket, sizeof(struct sockaddr_in));
   gethostname(hostname,100);

   hp=gethostbyname(hostname);
   if ( hp == NULL)
   {
      handle_error("Error: Host machine does not exist!\n");
   }
 
   main_socket.sin_family=hp->h_addrtype;
   main_socket.sin_port=htons(port);

   main_descriptor = socket(AF_INET, SOCK_STREAM, IPROTO);
   if (main_descriptor < 0)
   {
      log("boot", "Couldn't grab the socket!!!");
      exit(-1);
   }

   if (setsockopt(main_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *) &dummy,
       sizeof(dummy)) < 0)
      handle_error("Couldn't setsockopt()");

   if (ioctl(main_descriptor, FIONBIO, &dummy) < 0)
      handle_error("Can't set non-blocking");

/*
   main_socket.sin_addr.s_addr = INADDR_ANY;
*/

   if (bind(main_descriptor, (struct sockaddr *) & main_socket, sizeof(main_socket)) < 0)
   {
      log("boot", "Couldn't bind socket!!!");
      exit(-2);
   }


   if (listen(main_descriptor, 5) < 0)
      handle_error("Listen refused");

   sprintf(oldstack, "Main socket bound and listening on port %d", port);
   stack = end_string(oldstack);
   log("boot", oldstack);

   stack = oldstack;
}

/* tell the angel the server is alive */

void            do_alive_ping()
{
   static int      count = 5;

   count--;
   if (!count && alive_descriptor > 0)
   {
      count = 5;
      write(alive_descriptor, "SpAng!", 6);
   }
}


/* connect to the alive socket */

void            alive_connect()
{
   struct sockaddr_un sa;

   alive_descriptor = socket(PF_UNIX, SOCK_STREAM, 0);
   if (alive_descriptor < 0)
      handle_error("failed to make socket");

   sa.sun_family = AF_UNIX;
   strcpy(sa.sun_path, SOCKET_PATH);

   if (connect(alive_descriptor, (struct sockaddr *) & sa, sizeof(sa)) < 0)
   {
      close(alive_descriptor);
      alive_descriptor = -1;
      log("error", "Failed to connect to alive socket - ignoring");
      return;
   }
   do_alive_ping();

   log("boot", "Alive and kicking");

}



/* work out whether a player is site banished or not */

int             match_banish(player * p, char *line)
{
   char           *addr;

   for (addr = p->num_addr; *addr; addr++, line++)
      if (*line == '*')
      {
    while (isdigit(*addr))
       addr++;
    line++;
      } else if (*addr != *line)
    return 0;
   return 1;
}

int             do_banish(player * p)
{
   char           *scan;
   int             i;

   scan = banish_file.where;
   for (i = banish_file.length; i;)
   {
      if (*scan != '#' && match_banish(p, scan))
      {
    while (*scan != 'L' && *scan != 'C' && *scan != 'N' && *scan != '\n')
       scan++;
    switch (*scan)
    {
       case 'C':
          return 1;
       case 'N':
          p->flags |= CLOSED_TO_NEWBIES;
          break;
       case 'L':
          p->flags |= SITE_LOG;
          break;
    }
    return 0;
      }
      while (i && *scan != '\n')
      {
    scan++;
    i--;
      }
      if (i)
      {
    scan++;
    i--;
      }
   }
   return 0;
}

/* accept new connection on the main socket */

void            accept_new_connection()
{
   struct sockaddr_in incoming;
   struct hostent *hp;
   int             length, new_socket;
   char           *numerical_address;
   player         *p;
   int             dummy = 1, no1, no2, no3, no4;
   time_t          thetime;
   struct tm       *tm_time;
   
   length = sizeof(incoming);
   new_socket = accept(main_descriptor, (struct sockaddr *)&incoming, &length);
   if ((new_socket < 0) && errno == EINTR)
   {
      log("error", "EINTR accept trap");
      return;
   }
   if (new_socket < 0)
      handle_error("Error accepting new connection.");


   if (ioctl(new_socket, FIONBIO, &dummy) < 0)
      handle_error("Can't set non-blocking");

   if (current_players >= max_players)
   {
      write(new_socket, full_msg.where, full_msg.length);
      out_current += full_msg.length;
      out_pack_current++;
      return;
   }

   p = create_player();
   current_player = p;
   p->fd = new_socket;

   strncpy(p->num_addr, inet_ntoa(incoming.sin_addr), MAX_INET_ADDR - 2);
#ifdef LOLIGO
   numerical_address = p->num_addr;
#else
   hp = gethostbyaddr((char *) &(incoming.sin_addr.s_addr),
            sizeof(incoming.sin_addr.s_addr),
            AF_INET);
   if (hp)
      numerical_address = (char *)hp->h_name;
   else
      numerical_address = p->num_addr;
#endif
   strncpy(p->inet_addr, numerical_address, MAX_INET_ADDR - 2);
   sscanf(p->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);
/*TRAP for warwick, giving a shutdown message*/
/* removed -- if they want to spod it isn't my fault :P */

/* Because of CDA, ban on all kindergarden sites. */
/*
   if (strstr(p->inet_addr, ".k12.") && strstr(p->inet_addr, ".us"))
         p->flags |= BAN18;
*/
   if (do_banish(p))
   {
      write(new_socket, banish_msg.where, banish_msg.length);
      out_current += banish_msg.length;
      out_pack_current++;
      destroy_player(p);
      return;
   } 
   if (time(0) < splat_timeout && no1 == splat1 && no2 == splat2)
   {
      write(new_socket, splat_msg.where, splat_msg.length);
      out_current += banish_msg.length;
      out_pack_current++;
      destroy_player(p);
      return;
   } else
      connect_to_prog(p);
   current_player = 0;
}

/* turn on and off echo for passwords */

void            password_mode_on(player * p)
{
   p->flags |= PASSWORD_MODE;
   p->mode |= PASSWORD;
   if (!(p->flags & DO_LOCAL_ECHO))
      tell_player(p, "\377\373\001");
}

void            password_mode_off(player * p)
{
   p->flags &= ~PASSWORD_MODE;
   p->mode &= ~PASSWORD;
   if (!(p->flags & DO_LOCAL_ECHO))
      tell_player(p, "\377\374\001");
}

/* do a backspace */

void            backspace(player * p)
{
   p->ibuffer[p->ibuff_pointer] = 0;
   if (p->ibuff_pointer > 0)
      p->ibuff_pointer--;
}

/* handle telnet control codes */

void            telnet_options(player * p)
{
   unsigned char   c;

   if (read(p->fd, &c, 1) != 1)
      return;
   switch (c)
   {
      case EC:
    backspace(p);
    break;
      case EL:
    p->ibuff_pointer = 0;
    break;
      case IP:
    quit(p, 0);
    break;
      case DO:
    if (read(p->fd, &c, 1) != 1)
       return;
    switch (c)
    {
       case TELOPT_ECHO:
          if (!(p->flags & PASSWORD_MODE))
        p->flags |= DO_LOCAL_ECHO;   /* start local echo */
          break;
       case TELOPT_SGA:
          break;
       case TELOPT_EOR:
          p->flags |= EOR_ON;
          p->flags &= ~IAC_GA_DO;
          tell_player(p, "\377\031");
          break;
    }
    break;
      case DONT:
    if (read(p->fd, &c, 1) != 1)
       return;
    switch (c)
    {
       case TELOPT_ECHO:
          p->flags &= ~DO_LOCAL_ECHO;   /* stop local echo */
          break;
       case TELOPT_SGA:
          break;
       case TELOPT_EOR:
          p->flags &= ~EOR_ON;
          if (p->system_flags & IAC_GA_ON)
        p->flags |= IAC_GA_DO;
          break;
    }
    break;
   }
}


/* gets any input from one player */

void            get_player_input(player * p)
{
   int             chars_ready = 0;
   char           *oldstack, c;

#ifdef TRACK
   sprintf(functionin,"get_player_input (%s)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;

   if (ioctl(p->fd, FIONREAD, &chars_ready) == -1)
   {
      quit(p, 0);
      log("error", "PANIC on FIONREAD ioctl");
      perror("SpoooN (socket.c)");
      return;
   }
   if (!chars_ready)
   {
      if (sys_flags & VERBOSE)
    if (p->lower_name[0])
    {
       sprintf(oldstack, "%s went netdead.", p->name);
       stack = end_string(oldstack);
       log("connection", oldstack);
    } else
       log("connection", "Connection went netdead on login.");
      quit(p, 0);
      stack = oldstack;
      return;
   }
   in_current += chars_ready;
   in_pack_current++;

   stack = oldstack;

   for (; !(p->flags & PANIC) && chars_ready; chars_ready--)
      if (read(p->fd, &c, 1) != 1)
      {
    log("connection", "Player: read error");
    quit(p, 0);
    return;
      } else
    switch (c)
    {
       case -1:
          p->flags &= ~(LAST_CHAR_WAS_R | LAST_CHAR_WAS_N);
          telnet_options(p);
          return;
          break;
       case '\n':
          if (!(p->flags & LAST_CHAR_WAS_R))
          {
        p->flags |= LAST_CHAR_WAS_N;
        p->anticrash = 0;
        p->flags |= INPUT_READY;
        p->ibuffer[p->ibuff_pointer] = 0;
        p->ibuff_pointer = 0;
        p->column = 0;
        return;
          }
          break;
       case '\r':
          if (!(p->flags & LAST_CHAR_WAS_N))
          {
        p->flags |= LAST_CHAR_WAS_R;
        p->anticrash = 0;
        p->flags |= INPUT_READY;
        p->ibuffer[p->ibuff_pointer] = 0;
        p->ibuff_pointer = 0;
        p->column = 0;
        return;
          }
          break;
       default:
          p->flags &= ~(LAST_CHAR_WAS_R | LAST_CHAR_WAS_N);
          if (c == 8 || c == 127 || c == -9)
          {
        backspace(p);
        break;
          }
          if ((c > 31) && (p->ibuff_pointer < (IBUFFER_LENGTH - 3)))
          {
        p->ibuffer[p->ibuff_pointer] = c;
        p->ibuff_pointer++;
        if ((!(p->flags & PASSWORD_MODE)) && (p->flags & DO_LOCAL_ECHO))
        {
           if (write(p->fd, &c, 1) < 0 && errno != EINTR)
           {
         log("error", "Echoing back to player.\n");
         quit(p, 0);
         return;
           }
           out_current++;
           out_pack_current++;
        }
          } else
          {
        /* ANTI-PIPING bit */
/*
        p->anticrash++;
        if (p->anticrash > 100)
        {
           tell_player(p, "\n\n Auto chuckout to prevent piping.\n\n");
           quit(p, "");
           return;
        }
*/
          }
          break;
    }
}


/* this routine is called when idle */

void            scan_sockets()
{
   fd_set          fset;
   player         *scan;

   FD_ZERO(&fset);

   FD_SET(main_descriptor, &fset);
   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (!((scan->fd < 0) || (scan->flags & PANIC)))
    FD_SET(scan->fd, &fset);
   }

   if (select(FD_SETSIZE, &fset, 0, 0, 0) == -1)
      return;

   if (FD_ISSET(main_descriptor, &fset))
      accept_new_connection();
   for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (!(scan->fd <= 0 || scan->flags & (PANIC | INPUT_READY))
     && FD_ISSET(scan->fd, &fset))
    get_player_input(scan);
}


/*
 * turns a string said to a player into something ready to go down the telnet
 * connection
 */

file not_work_process_output(player * p, char *str)
{
   int i, hi = 0;
   char *save;
   file o;

   o.where = stack;
   o.length = 0;

   if (p != current_player)
   {

      if ((command_type & PERSONAL || p->flags & TAGGED) && p->term &&
           !((sys_flags & EVERYONE_TAG) || (sys_flags & ROOM_TAG)))
      {
         strcpy(stack, terms[(p->term) - 1].bold);
         while (*stack)
         {
            stack++;
            o.length++;
         }
         hi = 1;
      }
      save = stack;
      if (command_type & LOGIN_TAG) {
	if (p->tag_flags & TAG_LOGINS) {
 		*stack++ = ']';
		o.length++;
        	} else {
		*stack++ = ' ';
		o.length++;
      } 	}
      if (command_type & LOGOUT_TAG) {
	if (p->tag_flags & TAG_LOGINS) {
 		*stack++ = '[';
		o.length++;
        	} else {
		*stack++ = ' ';
		o.length++;
      } 	}
      if (command_type & RECON_TAG) {
	if (p->tag_flags & TAG_LOGINS) {
 		*stack++ = '%';
		o.length++;
        	} else {
		*stack++ = ' ';
		o.length++;
      } 	}
      if (command_type & ECHO_COM && p->tag_flags & TAG_ECHO)
      {
         *stack++ = '+';
         o.length++;
      }
      if ((command_type & PERSONAL ||
           p->flags & TAGGED) && p->tag_flags & TAG_PERSONAL &&
          !((sys_flags & EVERYONE_TAG) || (sys_flags & ROOM_TAG)))
      {
         if (sys_flags & FRIEND_TAG)
         {
            *stack++ = '*';
         } else if (sys_flags & OFRIEND_TAG)
         {
            *stack++ = '=';
         } else if (sys_flags & REPLY_TAG)
         {
            *stack++ = '&';
         } else
         {
            *stack++ = '>';
         }
         o.length++;
      }
      if ((command_type & EVERYONE || sys_flags & EVERYONE_TAG) &&
          p->tag_flags & TAG_SHOUT)
      {
         *stack++ = '!';
         o.length++;
      }
      if ((command_type & ROOM || sys_flags & ROOM_TAG)
          && p->tag_flags & TAG_ROOM && !(command_type & ECHO_COM))
      {
         *stack++ = '-';
         o.length++;
      }
      if (command_type & AUTO && p->tag_flags & TAG_AUTOS)
      {
         *stack++ = '#';
         o.length++;
      }
      if (stack != save)
      {
         *stack++ = ' ';
         o.length++;
      }
      if (command_type & ECHO_COM && p->tag_flags & SEEECHO &&
           (command_type & PERSONAL || (p->location)))
      {
         sprintf(stack, "[%s] ", current_player->name);
         while (*stack)
         {
            stack++;
            o.length++;
         }
      }
   }
   if ((!hi) && (command_type & HIGHLIGHT) && (p->term))
   {
      strcpy(stack, terms[(p->term) - 1].bold);
      while (*stack)
      {
         stack++;
         o.length++;
      }
      hi = 1;
   }
   p->column += o.length;

   while (*str)
   {
      switch (*str)
      {
         case '\n':
            if (hi && p->term)
            {
               strcpy(stack, terms[(p->term) - 1].off);
               while (*stack)
               {
                  stack++;
                  o.length++;
               }
               hi = 0;
            }
            *stack++ = '\r';
            *stack++ = '\n';
            p->column = 0;
            str++;
            o.length += 2;
            break;
         default:
            if (p->term_width && (p->column >= p->term_width))
            {
               for (i = 0; i < p->word_wrap; i++, stack--, o.length--)
               {
                  if (isspace(*(--str)))
                     break;
                  if (i != p->word_wrap)
                  {
                     *stack++ = '\r';
                     *stack++ = '\n';
                     *stack++ = ' ';
                     *stack++ = ' ';
                     *stack++ = ' ';
                     p->column = 3;
                     str++;
                     o.length += 5;
                  } else
                  {
                     for (; i; stack++, str++, o.length++)
                        i--;
                     *stack++ = '\r';
                     *stack++ = '\n';
                     *stack++ = ' ';
                     *stack++ = ' ';
                     *stack++ = ' ';
                     p->column = 3;
                     o.length += 5;
                  }
               }
            }
            *stack++ = *str++;
            o.length++;
            p->column++;
            break;
      }
   }
   return o;
}

/* OK, this is copied in from the socket.c that runs ok in the live*/


file            process_output(player * p, char *str)
{
   int sub = 0, i, hi = 0, col = 0;
   char *save, color_char;
   file o;

#ifdef TRACK
   sprintf(functionin,"process_output (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   o.where = stack;
   o.length = 0;


   if (p != current_player)
   {

      if ((command_type & PERSONAL || p->flags & TAGGED) && p->term &&
          !((sys_flags & EVERYONE_TAG) || (sys_flags & ROOM_TAG)) 
		&& !(p->misc_flags & SYSTEM_COLOR))
      {
         strcpy(stack, terms[(p->term) - 1].bold);
         while (*stack)
         {
            stack++;
            o.length++;
	    sub++;
         }
         hi = 1;
      }
			
      else if (p->misc_flags & SYSTEM_COLOR) {
	color_char = p->colorset[sys_color_atm];
	strcpy(stack, getcolor(p, color_char));
         while (*stack)
         {
            stack++;
            o.length++;
	    sub++;
         }
	 col = 1;
      }
      save = stack;
      if (command_type & LOGIN_TAG) {
	if (p->tag_flags & TAG_LOGINS) {
 		*stack++ = ']';
		o.length++;
        	} 
		*stack++ = ' ';
		o.length++;
      } 
      if (command_type & LOGOUT_TAG) {
	if (p->tag_flags & TAG_LOGINS) {
 		*stack++ = '[';
		o.length++;
        	} 
		*stack++ = ' ';
		o.length++;
      } 
      if (command_type & RECON_TAG) {
	if (p->tag_flags & TAG_LOGINS) {
 		*stack++ = '%';
		o.length++;
        	}
		*stack++ = ' ';
		o.length++;
      }
      if (command_type & ECHO_COM && p->tag_flags & TAG_ECHO)
      {
         *stack++ = '+';
         o.length++;
      }
      if ((command_type & PERSONAL ||
           p->flags & TAGGED) && p->tag_flags & TAG_PERSONAL &&
          !((sys_flags & EVERYONE_TAG) || (sys_flags & ROOM_TAG)))
      {
         if (sys_flags & FRIEND_TAG)
         {
            *stack++ = '*';
         } else if (sys_flags & OFRIEND_TAG)
         {
            *stack++ = '=';
         } else if (sys_flags & REPLY_TAG)
         {
            *stack++ = '&';
         } else
         {
            *stack++ = '>';
         }
         o.length++;
      }
      if (sys_flags & ITEM_TAG) {
	 if (p->tag_flags & TAG_ITEMS)
      	{
         *stack++ = '^';
         o.length++;
      	} else if (p->tag_flags & TAG_ROOM) {
         *stack++ = '-';
         o.length++;
        } else {
	 *stack++ = ' ';
	 o.length++;
	}
      }
      if ((command_type & EVERYONE || sys_flags & EVERYONE_TAG) &&
          p->tag_flags & TAG_SHOUT)
      {
         *stack++ = '!';
         o.length++;
      }
      if ((command_type & ROOM || sys_flags & ROOM_TAG) && 
	  p->tag_flags & TAG_ROOM && 
	  !(command_type & (RECON_TAG|LOGIN_TAG|LOGOUT_TAG|ECHO_COM)))
      {
         *stack++ = '-';
         o.length++;
      }
      if (command_type & AUTO && p->tag_flags & TAG_AUTOS)
      {
         *stack++ = '#';
         o.length++;
      }
      if (stack != save && !(command_type & (RECON_TAG|LOGIN_TAG|LOGOUT_TAG)))
      {
         *stack++ = ' ';
         o.length++;
      }
      if (command_type & ECHO_COM && p->tag_flags & SEEECHO &&
      (command_type & PERSONAL || (p->location)))
      {
         sprintf(stack, "[%s] ", current_player->name);
         while (*stack)
         {
            stack++;
            o.length++;
         }
      }
   }
   if ((!hi) && (command_type & HIGHLIGHT) && (p->term) && !(p->misc_flags & SYSTEM_COLOR))
   {
      strcpy(stack, terms[(p->term) - 1].bold);
      while (*stack)
      {
         stack++;
         o.length++;
	 sub++;
      }
      hi = 1;
   }


      else if ((!col) && p->misc_flags & SYSTEM_COLOR) {
	 color_char = p->colorset[sys_color_atm];
	 strcpy(stack, getcolor(p, color_char));
         while (*stack)
         {
            stack++;
            o.length++;
	    sub++;
         }
	 col = 1;
      }

   p->column += o.length;

   p->column -= sub;
   sub = 0;
   while (*str)
   {
      switch (*str)
      {
         case '^':
 		if (p->misc_flags & NOCOLOR)
		{
            		if (p->term_width && (p->column >= p->term_width))
            		{
               		for (i = 0; i < p->word_wrap; i++, stack--, o.length--)
                  	if (isspace(*(--str)))
                     		break;
               		if (i != p->word_wrap)
               			{
                  		*stack++ = '\r';
                  		*stack++ = '\n';
                  		*stack++ = ' ';
                  		*stack++ = ' ';
                  		*stack++ = ' ';
                  		p->column = 3;
                  		str++;
                  		o.length += 5;
               		} else {
                  	for (; i; stack++, str++, o.length++)
                     		i--;
                  	*stack++ = '\r';
                  	*stack++ = '\n';
                  	*stack++ = ' ';
                  	*stack++ = ' ';
                  	*stack++ = ' ';
                  	p->column = 3;
                  	o.length += 5;
             		  }
       			}
			if (*(str+1) == '^') {
       			   *stack++ = *str++;
       			   o.length++;
			}
			else
			   str++;
			str++;
       			p->column++;
       			break;
		}
		else switch (*(str+1)) 
		{
		   case '^':
            		if (p->term_width && (p->column >= p->term_width))
            		{
               		for (i = 0; i < p->word_wrap; i++, stack--, o.length--)
 
                  	if (isspace(*(--str)))
                     		break;
               		if (i != p->word_wrap)
               			{
                  		*stack++ = '\r';
                  		*stack++ = '\n';
                  		*stack++ = ' ';
                  		*stack++ = ' ';
                  		*stack++ = ' ';
                  		p->column = 3;
                  		str++;
                  		o.length += 5;
               		} else {
                  	for (; i; stack++, str++, o.length++)
                     		i--;
                  	*stack++ = '\r';
                  	*stack++ = '\n';
                  	*stack++ = ' ';
                  	*stack++ = ' ';
                  	*stack++ = ' ';
                  	p->column = 3;
                  	o.length += 5;
             		  }
       			}
			str++;
       			*stack++ = *str++;
       			o.length++;
       			/* p->column++; */
       			break;
		   default:
			color_char = *(str+1);
			if (!(getcolor(p, color_char))) {
				str++;
				str++;
				break;
			}
			strcpy(stack, getcolor(p, color_char));	
			col = 1;
			while (*stack)	
			{
				stack++;
				o.length++;
			}
			str++;
			str++;
			break;
		}
		break;
         case '\n':
            if (hi && p->term)
            {
               strcpy(stack, terms[(p->term) - 1].off);
               while (*stack)
               {
                  stack++;
                  o.length++;
               }
               hi = 0;
            }
            if (col && p->term)
            {
		/* clear color codes */
               strcpy(stack, getcolor(p, '^'));
               while (*stack)
               {
                  stack++;
                  o.length++;
               }
               col = 0;
            }
            *stack++ = '\r';
            *stack++ = '\n';
            p->column = 0;
            str++;
            o.length += 2;
            break;
         default:
            if (p->term_width && (p->column >= p->term_width))
            {
               for (i = 0; i < p->word_wrap; i++, stack--, o.length--)
                  if (isspace(*(--str)))
                     break;
               if (i != p->word_wrap)
               {
                  *stack++ = '\r';
                  *stack++ = '\n';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  p->column = 3;
                  str++;
                  o.length += 5;
               } else
               {
                  for (; i; stack++, str++, o.length++)
                     i--;
                  *stack++ = '\r';
                  *stack++ = '\n';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  *stack++ = ' ';
                  p->column = 3;
                  o.length += 5;
               }
       }
       *stack++ = *str++;
       o.length++;
       p->column++;
       break;
      }
   }
   return o;
}
/**/

/* generic routine to write to one player */

void tell_player(player * p, char *str)
{
   file output;
   char *oldstack, *script;

#ifdef TRACK
   sprintf(functionin,"tell_player (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (((p->fd) < 0) || (p->flags & PANIC) ||
       (!(p->location) && current_player != p))
   {
      return;
   }
   if (!(sys_flags & PANIC))
   {
      if (!test_receive(p))
         return;
   }
   output = process_output(p, str);
   if (p->script)
   {
      script = stack;
      sprintf(stack, "emergency/%s_emergency", p->lower_name);
      stack = end_string(stack);
      log(script, str);
      stack = script;
   }
   if (current_player != p && (command_type & PERSONAL || p->flags & TAGGED) 
	&& !(command_type & NO_HISTORY) && !((sys_flags & EVERYONE_TAG) 
	|| (sys_flags & ROOM_TAG)))
		process_review(p, output.where, output.length);
   if (p->flags & SCRIPTING)
   { 
      script = stack;
      sprintf(stack, "scripts/%s", p->script_file);
      stack = end_string(stack);
      log(script, str);
      script = oldstack;
   }
   if (write(p->fd, output.where, output.length) < 0 && errno != EINTR)
      quit(p, 0);
   out_current += output.length;
   out_pack_current++;
   stack = oldstack;
}

/* small derivative of tell player to save typing */

void            tell_current(char *str)
{
   if (!current_player)
      return;
   tell_player(current_player, str);
}

/* non blockable raw tell */

void            non_block_tell(player * p, char *str)
{
   file            output;
   char           *script, *oldstack;

   oldstack = stack;
   if (((p->fd) < 0) || (p->flags & PANIC))
      return;
   output = process_output(p, str);
   if (p->script)
   {
      script = stack;
      sprintf(stack, "emergency/%s_emergency", p->lower_name);
      stack = end_string(stack);
      log(script, str);
      stack = script;
   }
   if (write(p->fd, output.where, output.length) < 0 && errno != EINTR)
      quit(p, 0);
   out_current += output.length;
   out_pack_current++;
   stack = oldstack;
}


/* general routine to send a prompt */

void            do_prompt(player * p, char *str)
{
   char           *oldstack;

   oldstack = stack;
   strcpy(stack, str);
   stack = strchr(stack, 0);;
   if (p->flags & IAC_GA_DO)
   {
      *stack++ = IAC;
      *stack++ = GA;
   }
   if (p->flags & EOR_ON)
   {
      *stack++ = IAC;
      *stack++ = EOR;
   }
   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}

/* work out whether a player is site banished or not */
 
int             is_site_banned(player * p, char *line, char *check)
{
   char           *addr;

   for (addr = check; *addr; addr++, line++)
      if (*line == '*')
      {
    while (isdigit(*addr))
       addr++;
    line++;
      } else if (*addr != *line)
    return 0;
   return 1;
}

void             check_banish_status(player * p, char *str)
{
   char           *scan;
   int             i;
   char 	   Comments[300] = "", AdminBanning[30] = "";
   int	 	Com = 0, AB = 0;
   int		status = -1;
   
   

   if (!*str || !isdigit(*str)) {
	tell_player(p, " Format: ckban <IP address>\n");
	return;
	}
   scan = banish_file.where;
   for (i = banish_file.length; i;)
   {
      if (*scan != '#' && is_site_banned(p, scan, str))
      {
    while (*scan != 'L' && *scan != 'C' && *scan != 'N' && *scan != '\n')
       scan++;
    switch (*scan)
    {
       case 'C':
          status = 0;
          break;
       case 'N':
	  status = 1;
          break;
       case 'L':
	  status = 2;
          break;
    }
      }
   if (status >= 0)
	{
      scan++;
      i--;
      while (i && isspace(*(scan))) {
	i--;
	scan++;
      }
      while (i && *scan != '(' && Com < 298)
      {
	Comments[Com++] = *scan++;
	    i--;
      }
	Comments[Com++] = 0;
 	if (i)
		scan++;
      while (i && *scan != ')' && AB < 28)
	{
		AdminBanning[AB++] = *scan++;
		i--;
	}
	AdminBanning[AB++] = 0;
      while (i && *scan != '\n')
	{
		scan++;
		i--;
	}
      if (i)
      {
    scan++;
    i--;
	}
	switch (status) {
		case 0:
		TELLPLAYER(p, "Site is totally banned\nBanned by: %s\nReason: %s\n",
			AdminBanning, Comments);
		return;
		case 1:
		TELLPLAYER(p, "Site is newbie banned\nBanned by: %s\nReason: %s\n",
			AdminBanning, Comments);
		return;
		case 2:
		TELLPLAYER(p, "Site is logged, but not banned\nSited by: %s\nReason: %s\n",
			AdminBanning, Comments);
		return;
	}
      }
	else {
		while (i && *scan != '\n')
		{
			scan++;
			i--;
		}
		if (i)
		{
		scan++;
		i--;
		}
	     }
   }
	tell_player (p, " That site appears to not be banned...\n");
	return;
}


