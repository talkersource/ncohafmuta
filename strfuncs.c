#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#if defined(WIN32) && !defined(__CYGWIN32__)
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#if defined(SOL_SYS) || defined(__linux__)
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdarg.h>		/* for va_start(),va_arg(),va_end() */

#include "constants.h"
#include "protos.h"

#define RENEW(o,t,n) ((t*) realloc( (void*) o, sizeof(t) * (n) ))

static void realloc_str(char** strP, int size);

extern int bot;
extern int syslog_on;
extern char mess[ARR_SIZE+25];

void write_cygnus(int user)
{

sprintf(mess,"Flag 0: %s",flag_names[0].text);
write_str(user,mess);
sprintf(mess,"Flag 1: %s",flag_names[1].text);
write_str(user,mess);
sprintf(mess,"Bot   : %d",bot);
write_str(user,mess);
sprintf(mess,"name  : %s",ustr[user].say_name);
write_str(user,mess);

}


/*--------------------------------*/
/*   Write out a hilited string   */
/*--------------------------------*/
void write_hilite(int user, char *str)
{ 
char str2[ARR_SIZE];

if (ustr[user].hilite)
  {
   strcpy(str2,str);
   add_hilight(str2);
   write_str(user,str2);
  }
 else
  {
   write_str(user,str);
  }

}

/*--------------------------------------------------------------------------*/
/* write out a raw data (ascii and non-ascii) no special processing         */
/*--------------------------------------------------------------------------*/
void write_raw(int user, unsigned char *str, int len)
{
/* S_WRITE(ustr[user].sock, str, len); */
queue_write(user,(char *)str,-1);
}


/*** Write a NULL terminated string to a socket ***/
void write_it(int sock, char *str)
{
S_WRITE(sock, str, strlen(str));
}


/*--------------------------------------------------------*/
/*   Write out a hilited string with no carriage return   */
/*--------------------------------------------------------*/
void write_hilite_nr(int user, char *str)
{ 
char str2[ARR_SIZE];

if (ustr[user].hilite)
  {
    strcpy(str2,str);
    add_hilight(str2);
    write_str_nr(user,str2);
  }
 else
  {
   write_str_nr(user,str);
  }

}

/*------------------------------------------------*/
/* Send a string to the bot..this calls write_str */
/*------------------------------------------------*/
void write_bot(char *fmt)
{
if (bot!=-5) write_str(bot, fmt);
}

/*------------------------------------------------------------------*/
/* Send a string to the bot without the CR..this calls write_str_nr */
/*------------------------------------------------------------------*/
void write_bot_nr(char *fmt)
{
if (bot!=-5) write_str_nr(bot, fmt);
}

/*----------------------------------------*/
/*   write_str sends string down socket   */
/*----------------------------------------*/
void write_str(int user, char *str)
{
char buff[500], tp[3], hi_on[10], hi_off[10];
char tempst[ARR_SIZE];
int  stepper,num=0;
int  left=strlen(str);
int  i, count=0;

/* Check for bot write */
/* if (!strcmp(ustr[user].name,BOT_ID)) return; */

/*--------------------------------------------------------*/
/* pick reasonable range for width                        */
/*--------------------------------------------------------*/

if (ustr[user].cols < 15 || ustr[user].cols > 256) 
  stepper = 80;
 else
  stepper = ustr[user].cols;
  
/*-------------------------------------------*/
/* Convert string to hilited if carets exist */
/*-------------------------------------------*/
if (ustr[user].hilite || ustr[user].color) {
        strcpy(hi_on, "\033[1m");
        strcpy(hi_off, "\033[0m");
        }
else {
        strcpy(hi_on, "");
        strcpy(hi_off, "");
        }

tempst[0]=0;

for(i=0; i<left; i++) {
        if (str[i]==' ') {
                strcat(tempst, " ");
                continue;
                }
        if (str[i]=='@') {
                i++;
                if (str[i]=='@') {
                 strcat(tempst,hi_off);
                 count=0; continue;
                }
                else { i--;
                       tp[0]=str[i]; tp[1]=0;
                       strcat(tempst,tp);
                       continue;
                     }
                }
        if (str[i]=='^') {
                if (count) {
                        strcat(tempst, hi_off);
                        count=0;
                        continue;
                        }
                else {
                        count=1;
                        i++;
                         if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                 if (str[i]=='H') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='R') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;31m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else if (str[i]=='G') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;32m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='Y') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;33m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='B') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;34m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='M') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;35m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='C') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;36m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='W') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;37m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='L') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='R') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[31m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else if (str[i]=='G') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[32m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='Y') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[33m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='B') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[34m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='M') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[35m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='C') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[36m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='W') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[37m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='B') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='L') {
                   if (ustr[user].color && 
                       strcmp(ustr[user].name,"llo"))
                        strcat(tempst,"\033[5m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='U') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='L') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[4m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='R') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='V') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[7m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                    }
                 else { tp[0]=str[i]; tp[1]=0;
                        strcat(tempst,hi_on);
                        strcat(tempst,tp);
                        }
                        continue;
                    }  /* end of count else */
                }  /* end of if caret */
        tp[0]=str[i];
        tp[1]=0;
        strcat(tempst, tp);
        }
if (count) strcat(tempst,hi_off);
count=0;
i=0;

if (left == 0 )
  {
/*
   if (ustr[user].car_return && ustr[user].afk<2) 
     S_WRITE(ustr[user].sock, "\r\n", 2);
    else
     S_WRITE(ustr[user].sock, "\n", 1);
*/
   if (ustr[user].car_return && ustr[user].afk<2) 
     queue_write(user, "\r\n", -1);
    else
     queue_write(user, "\n", -1);
   return;
  }

buff[0]=0;
num=0;

  str=tempst;

for(; left > 0; left -= stepper )
  {
   strncpy(buff, str, stepper);
      
   str += stepper;
   buff[stepper] = 0;
   
   if (ustr[user].car_return) 
     strcat(buff,"\r\n");
    else
     strcat(buff,"\n");
  
   if (ustr[user].afk<2)
     {
      queue_write(user, buff, -1);
/*      S_WRITE(ustr[user].sock, buff, strlen(buff)); */
     }
  
   }

buff[0]=0;
tempst[0]=0;
num=0;
}

/*--------------------------------------------------------------------------*/
/* write out a string to a user with no carriage return (note: this should  */
/* do a call to write_str but doesnt.)  This is currently like write_raw    */
/* but should be like write_str.                                            */
/*--------------------------------------------------------------------------*/
void write_str_nr(int user, char *str)
{
char tp[3], hi_on[10], hi_off[10];
char tempst[500];
int  left=strlen(str);
int  i, count=0;

/* Check for bot write */
/* if (!strcmp(ustr[user].name,BOT_ID)) return; */

/*-------------------------------------------*/
/* Convert string to hilited if carets exist */
/*-------------------------------------------*/
if (ustr[user].hilite || ustr[user].color) {
        strcpy(hi_on, "\033[1m");
        strcpy(hi_off, "\033[0m");
        }
else {
        strcpy(hi_on, "");
        strcpy(hi_off, "");
        }

tempst[0]=0;

for(i=0; i<left; i++) {
        if (str[i]==' ') {
                strcat(tempst, " ");
                continue;
                }
      if (str[i]=='@') { 
                i++;
                if (str[i]=='@') {
                 strcat(tempst,hi_off);
                 count=0; continue;
                }
                else { i--;
                       tp[0]=str[i]; tp[1]=0;
                       strcat(tempst,tp);
                       continue;
                     }
                }
        if (str[i]=='^') {
                if (count) {
                        strcat(tempst, hi_off);
                        count=0;
                        continue;
                        }
                else {
                        count=1;
                        i++;
                         if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                 if (str[i]=='H') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='R') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;31m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else if (str[i]=='G') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;32m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='Y') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;33m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='B') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;34m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='M') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;35m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='C') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;36m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='W') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[1;37m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='L') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='R') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[31m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else if (str[i]=='G') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[32m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='Y') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[33m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='B') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[34m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='M') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[35m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='C') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[36m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else if (str[i]=='W') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[37m");
                       else
                        strcat(tempst,hi_on);
                       }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='B') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='L') {
                   if (ustr[user].color && 
                       strcmp(ustr[user].name,"llo"))
                        strcat(tempst,"\033[5m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='U') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='L') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[4m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='R') {
                    i++;
                     if (i == left) {
                            strcat(tempst,hi_off);
                            count=0;
                            break;
                           }
                     if (str[i]=='V') {
                       if (ustr[user].color)
                        strcat(tempst,"\033[7m");
                       else
                        strcat(tempst,hi_on);
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
                            }
                    }
                 else { tp[0]=str[i]; tp[1]=0;
                        strcat(tempst,hi_on);
                        strcat(tempst,tp);
                        }
                        continue;
                    }
                }
        tp[0]=str[i];
        tp[1]=0;
        strcat(tempst, tp);
        } 
count=0;
i=0;

if (ustr[user].afk<2)
  {
     queue_write(user, tempst, -1);
/*   S_WRITE(ustr[user].sock,tempst,strlen(tempst)); */
  }

tempst[0]=0;
}


/*--------------------------------------------------------------------------*/
/* write out a string to a web users queue with no carriage return          */
/*--------------------------------------------------------------------------*/
void write_str_www(int user, char *str, int size)
{

/* Check for bot write */
if (!strcmp(ustr[user].name,BOT_ID) && bot==-5) return;

queue_write_www(user, str, size);
}


/* Write string and arguments to a specific logging facility */
void write_log(int type, int wanttime, char *str, ...)
{
char z_mess[ARR_SIZE*2];
char logfile[FILE_NAME_LEN];
va_list args;
FILE *fp;

if (!syslog_on) return;

z_mess[0]=0;

sprintf(z_mess,logfacil[type].file,LOGDIR);
strncpy(logfile,z_mess,FILE_NAME_LEN);

 if (!(fp=fopen(logfile,"a")))
   {
    sprintf(z_mess,"%s LOGGING: Couldn't open file(a) \"%s\"! %s",STAFF_PREFIX,logfile,get_error());
    writeall_str(z_mess, WIZ_ONLY, -1, 0, -1, BOLD, NONE, 0);
    return;
   }
 else {
    va_start(args,str);
    if (wanttime) {
    sprintf(z_mess,"%s: ",get_time(0,0));
    vsprintf(z_mess+strlen(z_mess),str,args);
    }
    else
    vsprintf(z_mess,str,args);

    va_end(args);
    fputs(z_mess,fp);
    FCLOSE(fp);
   }
}


/* Strip colors from string */
char *strip_color(char *str)
{
char tp[3];
static char tempst[500];
int  left=strlen(str);
int  i, count=0;

tempst[0]=0;

for(i=0; i<left; i++) {
        if (str[i]==' ') {
                strcat(tempst, " ");
                continue;
                }
      if (str[i]=='@') { 
                i++;
                if (str[i]=='@') {
                 count=0; continue;
                }
                else { i--;
                       tp[0]=str[i]; tp[1]=0;
                       strcat(tempst,tp);
                       continue;
                     }
                }
        if (str[i]=='^') {
                if (count) {
                        count=0;
                        continue;
                        }
                else {
                        count=1;
                        i++;
                         if (i == left) {
                            count=0;
                            break;
                           }
                 if (str[i]=='H') {
                    i++;
                     if (i == left) {
                            count=0;
                            break;
                           }
                     if (str[i]=='R') {
                       continue;
                      }
                     else if (str[i]=='G') {
                       continue;
                       }
                     else if (str[i]=='Y') {
                       continue;
                       }
                     else if (str[i]=='B') {
                       continue;
                       }
                     else if (str[i]=='M') {
                       continue;
                       }
                     else if (str[i]=='C') {
                       continue;
                       }
                     else if (str[i]=='W') {
                       continue;
                       }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='L') {
                    i++;
                     if (i == left) {
                            count=0;
                            break;
                           }
                     if (str[i]=='R') {
                        continue;
                      }
                     else if (str[i]=='G') {
                        continue;
                       }
                     else if (str[i]=='Y') {
                        continue;
                       }
                     else if (str[i]=='B') {
                        continue;
                       }
                     else if (str[i]=='M') {
                        continue;
                       }
                     else if (str[i]=='C') {
                        continue;
                       }
                     else if (str[i]=='W') {
                        continue;
                       }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='B') {
                    i++;
                     if (i == left) {
                            count=0;
                            break;
                           }
                     if (str[i]=='L') {
                        continue;
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='U') {
                    i++;
                     if (i == left) {
                            count=0;
                            break;
                           }
                     if (str[i]=='L') {
                        continue;
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,tp);
                            }
                   }
                 else if (str[i]=='R') {
                    i++;
                     if (i == left) {
                            count=0;
                            break;
                           }
                     if (str[i]=='V') {
                        continue;
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,tp);
                            }
                    }
                 else { tp[0]=str[i]; tp[1]=0;
                        strcat(tempst,tp);
                        }
                        continue;
                    }
                }
        tp[0]=str[i];
        tp[1]=0;
        strcat(tempst, tp);
        } 
count=0;
i=0;

return tempst;
}

/* Convert color codes to HTML color codes */
char *convert_color(char *str)
{
char tp[3], hi_on[10], hi_off[10], font_off[10];
char blink_on[10], blink_off[10], under_on[10], under_off[10];
static char tempst[500];
int  left=strlen(str);
int  i, count=0, space=0, font=0, hi=0, blink=0, under=0;

/*---------------------------------------------*/
/* Convert string to bold if just carets exist */
/*---------------------------------------------*/
strcpy(hi_on, "<b>");
strcpy(hi_off, "</b>");
strcpy(font_off,"</font>");
strcpy(blink_on,"<blink>");
strcpy(blink_off,"</blink>");
strcpy(under_on,"<u>");
strcpy(under_off,"</u>");

tempst[0]=0;

for(i=0; i<left; i++) {
        if (str[i]==' ') {
		if (space)
                strcat(tempst, "&nbsp;");
		else
		strcat(tempst, " ");
		space=1;
                continue;
                }
	else space=0;
      if (str[i]=='@') { 
                i++;
                if (str[i]=='@') {
		if (hi)
                 strcat(tempst,hi_off);
		if (font)
                 strcat(tempst,font_off);
		if (blink)
                 strcat(tempst,blink_off);
		if (under)
                 strcat(tempst,under_off);
                 count=0;
		 font=0;
		 hi=0;
		continue;
                }
                else { i--;
                       tp[0]=str[i]; tp[1]=0;
                       strcat(tempst,tp);
                       continue;
                     }
                }
        if (str[i]=='^') {
                if (count) {
		if (hi)
                 strcat(tempst,hi_off);
		if (font)
                 strcat(tempst,font_off);
		if (blink)
                 strcat(tempst,blink_off);
		if (under)
                 strcat(tempst,under_off);
                        count=0;
			font=0;
			hi=0;
                        continue;
                        }
                else {
                        count=1;
                        i++;
                         if (i == left) {
			if (hi)
       		          strcat(tempst,hi_off);
			if (font)
       		          strcat(tempst,font_off);
			if (blink)
	       	          strcat(tempst,blink_off);
			if (under)
       		          strcat(tempst,under_off);
                        count=0;
			font=0;
			hi=0;
                            break;
                           }
                 if (str[i]=='H') {
                    i++;
                     if (i == left) {
			if (hi)
       		          strcat(tempst,hi_off);
			if (font)
       		          strcat(tempst,font_off);
			if (blink)
	       	          strcat(tempst,blink_off);
			if (under)
       		          strcat(tempst,under_off);
                        count=0;
			font=0;
			hi=0;
                            break;
                           }
                     if (str[i]=='R') {
			strcat(tempst,"<font color=\"red\">");
			font=1;
                      }
                     else if (str[i]=='G') {
			strcat(tempst,"<font color=\"#00FF00\">");
			font=1;
                       }
                     else if (str[i]=='Y') {
			strcat(tempst,"<font color=\"yellow\">");
			font=1;
                       }
                     else if (str[i]=='B') {
			strcat(tempst,"<font color=\"#0000FF\">");
			font=1;
                       }
                     else if (str[i]=='M') {
			strcat(tempst,"<font color=\"#FF00FF\">");
			font=1;
                       }
                     else if (str[i]=='C') {
			strcat(tempst,"<font color=\"cyan\">");
			font=1;
                       }
                     else if (str[i]=='W') {
			strcat(tempst,"<font color=\"white\">");
			strcat(tempst,hi_on);
			font=1;
			hi=1;
                       }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
			    hi=1;
                            }
                   }
                 else if (str[i]=='L') {
                    i++;
                     if (i == left) {
			if (hi)
       		          strcat(tempst,hi_off);
			if (font)
       		          strcat(tempst,font_off);
			if (blink)
	       	          strcat(tempst,blink_off);
			if (under)
       		          strcat(tempst,under_off);
                        count=0;
			font=0;
			hi=0;
                            break;
                           }
                     if (str[i]=='R') {
			strcat(tempst,"<font color=\"#8E2323\">");
			font=1;
                      }
                     else if (str[i]=='G') {
			strcat(tempst,"<font color=\"green\">");
			font=1;
                       }
                     else if (str[i]=='Y') {
			strcat(tempst,"<font color=\"#CFB53B\">");
			font=1;
                       }
                     else if (str[i]=='B') {
			strcat(tempst,"<font color=\"#3232CD\">");
			font=1;
                       }
                     else if (str[i]=='M') {
			strcat(tempst,"<font color=\"#9932CD\">");
			font=1;
                       }
                     else if (str[i]=='C') {
			strcat(tempst,"<font color=\"#7093DB\">");
			font=1;
                       }
                     else if (str[i]=='W') {
			strcat(tempst,"<font color=\"white\">");
			font=1;
                       }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
			    hi=1;
                            }
                   }
                 else if (str[i]=='B') {
                    i++;
                     if (i == left) {
			if (hi)
       		          strcat(tempst,hi_off);
			if (font)
       		          strcat(tempst,font_off);
			if (blink)
	       	          strcat(tempst,blink_off);
			if (under)
       		          strcat(tempst,under_off);
                        count=0;
			font=0;
			hi=0;
                            break;
                           }
                     if (str[i]=='L') {
                        strcat(tempst,blink_on);
			blink=1;
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
			    hi=1;
                            }
                   }
                 else if (str[i]=='U') {
                    i++;
                     if (i == left) {
			if (hi)
       		          strcat(tempst,hi_off);
			if (font)
       		          strcat(tempst,font_off);
			if (blink)
	       	          strcat(tempst,blink_off);
			if (under)
       		          strcat(tempst,under_off);
                        count=0;
			font=0;
			hi=0;
                            break;
                           }
                     if (str[i]=='L') {
                        strcat(tempst,under_on);
			under=1;
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
			    hi=1;
                            }
                   }
                 else if (str[i]=='R') {
                    i++;
                     if (i == left) {
			if (hi)
       		          strcat(tempst,hi_off);
			if (font)
       		          strcat(tempst,font_off);
			if (blink)
	       	          strcat(tempst,blink_off);
			if (under)
       		          strcat(tempst,under_off);
                        count=0;
			font=0;
			hi=0;
                            break;
                           }
                     if (str[i]=='V') {
                        strcat(tempst,hi_on);
			hi=1;
                      }
                     else { i--; tp[0]=str[i]; tp[1]=0;
                            strcat(tempst,hi_on);
                            strcat(tempst,tp);
			    hi=1;
                            }
                    }
                 else { tp[0]=str[i]; tp[1]=0;
                        strcat(tempst,hi_on);
                        strcat(tempst,tp);
			hi=1;
                        }
                        continue;
                    }
                }
        tp[0]=str[i];
        tp[1]=0;
        strcat(tempst, tp);
        } 
count=0;
i=0;

return tempst;
}


/* queue data to go out in the user's output_data pointer */
void queue_write(int user, char *queue_str, int length)
{

if (length==-1) length = strlen(queue_str);

#if defined(QWRITE_DEBUG)
write_log(DEBUGLOG,NOTIME,"QW : Want to write %d chars to buffer\n",length);
#endif
if (ustr[user].alloced_size) {
#if defined(QWRITE_DEBUG)
	write_log(DEBUGLOG,NOTIME,"QW : reallocing data of current size %d (malloced: %d)\n",strlen(ustr[user].output_data),ustr[user].alloced_size);
	write_log(DEBUGLOG,NOTIME,"QW : to new malloced size %d\n",ustr[user].alloced_size+length);
#endif
realloc_str( &ustr[user].output_data, ustr[user].alloced_size + length);
}
else {
  ustr[user].output_data	= NULL;
  ustr[user].write_offset	= 0;
  ustr[user].alloced_size	= 0;
  ustr[user].output_data	= (char *)malloc(length+1);
#if defined(QWRITE_DEBUG)
  write_log(DEBUGLOG,NOTIME,"QW : QUEUED-NEW %d chars to buffer\n",length);
#endif
  }
(void) memcpy( &(ustr[user].output_data[ustr[user].alloced_size]), queue_str, length );
ustr[user].alloced_size += length;

}


/* user's socket is writable, start writing output_data pointer to socket */
int queue_flush(int user)
{
int cnt=0;

if (!ustr[user].alloced_size) return 1;

#if defined(QFLUSH_DEBUG)
write_log(DEBUGLOG,NOTIME,"QF : Trying to FLUSH %d chars, of data size %d\n",ustr[user].alloced_size - ustr[user].write_offset,ustr[user].alloced_size);
#endif

cnt = S_WRITE(ustr[user].sock,
	      ustr[user].output_data + ustr[user].write_offset,
	      ustr[user].alloced_size - ustr[user].write_offset);
if (cnt >= (ustr[user].alloced_size - ustr[user].write_offset)) {
#if defined(QFLUSH_DEBUG)
	write_log(DEBUGLOG,NOTIME,"QF : Flushed ALL %d chars\n",cnt);
#endif
	free(ustr[user].output_data);
	ustr[user].output_data	= NULL;
	ustr[user].alloced_size	= ustr[user].write_offset = 0;
  }
else if (cnt < 0) {
	write_log(ERRLOG,YESTIME,"ERRNO %s in queue_flush writing %d chars for %s!\n",
		get_error(),ustr[user].alloced_size - ustr[user].write_offset,ustr[user].say_name);
	if (errno == EWOULDBLOCK || errno == EAGAIN)
	  return 1;                       
	free(ustr[user].output_data);
	ustr[user].output_data	= NULL;
	ustr[user].alloced_size	= ustr[user].write_offset = 0;
	return 0;
  }
else {
#if defined(QFLUSH_DEBUG)
	write_log(DEBUGLOG,NOTIME,"QF : Flushed SOME %d chars of %d\n",cnt,ustr[user].alloced_size);
#endif
  ustr[user].write_offset += cnt;

#if defined(QFLUSH_DEBUG)
write_log(DEBUGLOG,NOTIME,"QF: %d chars left\n",ustr[user].alloced_size-ustr[user].write_offset);
#endif
  return 1;
  }
return 1;
}


/* queue data to go out in the web port's output_data pointer */
void queue_write_www(int user, char *new_str, int length)
{

if (length==-1) length = strlen(new_str);

#if defined(QWRITE_DEBUG)
write_log(DEBUGLOG,NOTIME,"QWW: Want to write %d chars to buffer\n",length);
#endif
if (wwwport[user].alloced_size) {
#if defined(QWRITE_DEBUG)
	write_log(DEBUGLOG,NOTIME,"QWW: reallocing data of current size %d (malloced: %d)\n",strlen(wwwport[user].output_data),wwwport[user].alloced_size);
	write_log(DEBUGLOG,NOTIME,"QWW: to new malloced size %d\n",wwwport[user].alloced_size+length);
#endif
realloc_str( &wwwport[user].output_data, wwwport[user].alloced_size + length);
}
else {
  wwwport[user].output_data	= NULL;
  wwwport[user].write_offset	= 0;
  wwwport[user].alloced_size	= 0;
  wwwport[user].output_data	= (char *)malloc(length+1);
#if defined(QWRITE_DEBUG)
  write_log(DEBUGLOG,NOTIME,"QWW: QUEUED-NEW %d chars to buffer\n",length);
#endif
  }
(void) memcpy( &(wwwport[user].output_data[wwwport[user].alloced_size]), new_str, length );
wwwport[user].alloced_size += length;

}


/* web port's socket is writable, start writing output_data pointer to socket */
int queue_flush_www(int user)
{
int cnt=0;

if (!wwwport[user].alloced_size) return 1;

#if defined(QFLUSH_DEBUG)
write_log(DEBUGLOG,NOTIME,"QFW: Trying to FLUSH %d chars, data size %d\n",wwwport[user].alloced_size - wwwport[user].write_offset,wwwport[user].alloced_size);
#endif

cnt = S_WRITE(wwwport[user].sock,
	      wwwport[user].output_data + wwwport[user].write_offset,
	      wwwport[user].alloced_size - wwwport[user].write_offset);
if (cnt >= (wwwport[user].alloced_size - wwwport[user].write_offset)) {
#if defined(QFLUSH_DEBUG)
	write_log(DEBUGLOG,NOTIME,"QFW: Flushed ALL %d chars\n",cnt);
#endif
	free(wwwport[user].output_data);
	wwwport[user].output_data  = NULL;
	wwwport[user].alloced_size = wwwport[user].write_offset = 0;
	free_sock(user,'4');
  }
else if (cnt < 0) {
	write_log(ERRLOG,YESTIME,"ERRNO %s in queue_flush_www writing %d chars!\n",
		get_error(),wwwport[user].alloced_size - wwwport[user].write_offset);
	if (errno == EWOULDBLOCK || errno == EAGAIN)
	  return 1;
	free(wwwport[user].output_data);
	wwwport[user].output_data  = NULL;
	wwwport[user].alloced_size = wwwport[user].write_offset = 0;
	free_sock(user,'4');
	return 0;
  }
else {
#if defined(QFLUSH_DEBUG)
	write_log(DEBUGLOG,NOTIME,"QFW: Flushed SOME %d chars of %d\n",cnt,wwwport[user].alloced_size);
#endif
  wwwport[user].write_offset += cnt;

#if defined(QFLUSH_DEBUG)
write_log(DEBUGLOG,NOTIME,"QFW: %d chars left\n",wwwport[user].alloced_size - wwwport[user].write_offset);
#endif
  return 1;
  }
return 1;
}

static void realloc_str(char** strP, int size) {
int maxsizeP;

        maxsizeP = size * 5 / 4;
        *strP = RENEW( *strP, char, maxsizeP + 1 );

    if ( *strP == (char*) 0 )
        {
	write_log(ERRLOG,YESTIME,"QW : ERRNO: REALLOC: Out of memory!\n");
	}
}
