#if defined(HAVE_CONFIG_H)
#include "../hdrfiles/config.h"
#endif

#include "../hdrfiles/includes.h"

/*--------------------------------------------------------*/
/* Talker-related include files                           */ 
/*--------------------------------------------------------*/
#include "../hdrfiles/osdefs.h"
/*
#include "../hdrfiles/authuser.h"
#include "../hdrfiles/text.h"
*/
#include "../hdrfiles/constants.h"
#include "../hdrfiles/protos.h"

extern int num_of_users;     /* total number of users online           */
extern char thishost[101];
extern char mess[ARR_SIZE+25];    /* functions use mess to send output   */
extern char t_mess[ARR_SIZE+25];    /* functions use mess to send output   */


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
else if (port=='5') {
   for (u=0;u<MAX_MISC_CONNECTS;++u)
     {
      if (miscconn[u].sock == -1)
        return u;
     }
   return -1;
  }
else {
   for (u=0;u<MAX_USERS;++u) 
     {
      if (ustr[u].area== -1 && !ustr[u].logging_in) {
	/* CYGNUS2 */
	if (ustr[u].conv == NULL) {
		ustr[u].conv = (ConvPtr) malloc (sizeof (ConvBuffer));
		if (ustr[u].conv == NULL)       /* malloc failed */
		{
		write_log(ERRLOG,YESTIME,"MALLOC: Failed for conv buffer in find_free_slot() %s\n",get_error());
		return -1;
		}
		init_conv_buffer(ustr[u].conv);
	} /* end of if conv null */
	/* CYGNUS3 */
	if (ustr[u].Macros == NULL) {
		ustr[u].Macros = (MacroPtr) malloc (sizeof (MacroBuffer));
		if (ustr[u].Macros == NULL)       /* malloc failed */
		{
		write_log(ERRLOG,YESTIME,"MALLOC: Failed for Macros in find_free_slot() %s\n",get_error());
		return -1;
		}
		init_macro_buffer(ustr[u].Macros);
	} /* end of if Macros null */
        return u;
       } /* end of if free slot */
     } /* end of for */
  return -1;
  } /* end of else */

}

void realloc_str(char** strP, int size) {
int maxsizeP;

        maxsizeP = size * 5 / 4;
        *strP = RENEW( *strP, char, maxsizeP + 1 );

    if ( *strP == (char*) 0 )
        {
        write_log(ERRLOG,YESTIME,"QW : ERRNO: REALLOC: Out of memory!\n");
        }
}


/* If the errno is one within the cases below, the calling function is */
/* told it is ok to continue (i.e. a non-fatal error) */
int errno_ok(int myerr) {

switch(myerr) {
#ifdef EPROTO
                case EPROTO:
#endif
#if defined(EINTR) && ((EPROTO) != (EINTR))
                case EINTR:
#endif
#ifdef EAGAIN
                case EAGAIN:
#endif
#ifdef EPIPE
                case EPIPE:
#endif
#ifdef ECONNABORTED
                case ECONNABORTED:
#endif
                    /* Linux generates the rest of these, other tcp
                     * stacks (i.e. bsd) tend to hide them behind
                     * getsockopt() interfaces.  They occur when
                     * the net goes sour or the client disconnects
                     * after the three-way handshake has been done
                     * in the kernel but before userland has picked
                     * up the socket.
                     */
#ifdef ECONNRESET
                case ECONNRESET:
#endif
#ifdef ETIMEDOUT
                case ETIMEDOUT:
#endif
#ifdef EHOSTUNREACH
                case EHOSTUNREACH:
#endif
#ifdef ENETUNREACH
                case ENETUNREACH:
#endif
                    break;
		default: return 0;
}

return 1;
}

