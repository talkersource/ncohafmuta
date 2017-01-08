#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#if defined(SOL_SYS)
#include <string.h>
#else
#include <strings.h>
#endif

#include "constants.h"
#include "protos.h"

extern int bot;
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
 S_WRITE(ustr[user].sock, str, len);
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
 write_str(bot, fmt);
}

/*------------------------------------------------------------------*/
/* Send a string to the bot without the CR..this calls write_str_nr */
/*------------------------------------------------------------------*/
void write_bot_nr(char *fmt)
{
 write_str_nr(bot, fmt);
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
if (!strcmp(ustr[user].name,BOT_ID) && bot==-5) return;

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
   if (ustr[user].car_return && ustr[user].afk<2) 
     S_WRITE(ustr[user].sock, "\r\n", 2);
    else
     S_WRITE(ustr[user].sock, "\n", 1);

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
      S_WRITE(ustr[user].sock, buff, strlen(buff));
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
if (!strcmp(ustr[user].name,BOT_ID) && bot==-5) return;

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
   S_WRITE(ustr[user].sock,tempst,strlen(tempst));
  }

tempst[0]=0;
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

