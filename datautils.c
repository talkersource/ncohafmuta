#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#if defined(SOL_SYS)
#include <string.h>
#else
#include <strings.h>
#endif

#include "constants.h"
#include "protos.h"

extern char mess[ARR_SIZE+25];
extern char t_mess[ARR_SIZE+25];
extern char datadir[255];
extern int PORT;
extern int NUM_AREAS;
extern int atmos_on;
extern int syslog_on;
extern int MESS_LIFE;
extern int allow_new;
extern int resolve_names;
extern char area_nochange[MAX_AREAS];
extern int new_room;
extern char web_opts[11][64];
extern char conv[MAX_AREAS][NUM_LINES][MAX_LINE_LEN+1];

/** read in initialize data **/
void read_init_data()
{
char filename[FILE_NAME_LEN],line[256];
char hide1[MAX_AREAS+1];
char tog_atmos[MAX_AREAS+1];
char dummy;
int a;
FILE *fp;

sprintf(t_mess,"%s/%s",datadir,INIT_FILE);
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(fp=fopen(filename,"r"))) 
  {
   perror("SYSTEM: Cannot access area data files");
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
   exit(0);
  }

fgets(line,80,fp);

/* read in important system data & do a check of some of it */
sscanf(line,"%d %d %d %d %d %d %d %s",&PORT,
                                   &NUM_AREAS,
                                   &atmos_on,
                                   &syslog_on,
                                   &MESS_LIFE,
                                   &allow_new,
				   &resolve_names,
                                   area_nochange);
                                   
if (PORT<1024 || PORT>9999) 
  {
   sprintf(mess,"SYSTEM: Bad port number (%d)",PORT);
   perror(mess);
   FCLOSE(fp);
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
   exit(0);
  }
	
if (NUM_AREAS>MAX_AREAS) 
  {
   perror("SYSTEM: No. of rooms is too great!");
   FCLOSE(fp);
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
   exit(0);
  }

/* read in descriptions and joinings */
for (a=0; a<NUM_AREAS; ++a) 
  {
   fgets(line,80,fp);
   hide1[0] = 0;
   tog_atmos[0] = 0;
   sscanf(line,"%c %s %s %s %s", &dummy,astr[a].name,astr[a].move,hide1,tog_atmos);   

   if (!strcmp(NEW_ROOM,astr[a].name))
    new_room=a;

   if (!strcmp(hide1,"1"))
    astr[a].hidden=1;
   else
    astr[a].hidden=0;
   if (!strcmp(tog_atmos,"1"))
    astr[a].atmos=1;
   else
    astr[a].atmos=0;
  }
  
FCLOSE(fp);

sprintf(t_mess,"%s",WEBCONFIG);
strncpy(filename,t_mess,FILE_NAME_LEN);

if (!(fp=fopen(filename,"r"))) 
  {
   perror("SYSTEM: Cannot access web port config file");
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
   exit(0);
  }

a=0;
while (fgets(line,256,fp) != NULL) {
 line[strlen(line)-1]=0;
 if (strlen(line)<=1) continue;
 if (line[0]=='/') continue;

 if (a < 11) {
 strcpy(web_opts[a],line);
 a++;
 }
 else break;
 } /* end of while */
a=0;

FCLOSE(fp);
}

/** read in exempted users for expire **/
void read_exem_data()
{
int i=0;
int lines=0;
char filename[FILE_NAME_LEN];
FILE *fp;

sprintf(filename,"%s",EXEMFILE);

/* If file doesn't exist, create it */
if (!(fp=fopen(filename,"r"))) 
  {
   printf("SYSTEM: Cannot access user exempt files..will create a new file\n");
   if (!(fp=fopen(filename,"a"))) {
      printf("SYSTEM: Cannot create new user exempt file.\n");
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
      exit(0);
      }
   for (i=0;i<NUM_EXPIRES;++i) {
        sprintf(mess,"%s\n",expired[i]);
        fputs(mess,fp);
        }
   i=0;
  }

FCLOSE(fp);

/*----------------------------------------------------------------*/
/* Count lines in file and compare to lines defined in global     */
/* If they differ, add the difference number of lines to the file */
/*----------------------------------------------------------------*/

lines = file_count_lines(filename);

if (lines < NUM_EXPIRES) {
    i = lines;
    fp=fopen(filename,"a");
    printf("Exempt global is higher than exempt space in file. Fixing..\n");
    for (i=lines;i<NUM_EXPIRES;++i) {
        sprintf(mess,"%s\n",expired[i]);
        fputs(mess,fp);
        }
     FCLOSE(fp);
    }
    
    fp=fopen(filename,"r");
    i=0;

 for (i=0;i<NUM_EXPIRES;++i) {
     fscanf(fp,"%s\n",expired[i]);
     }
 
 i=0;
    
FCLOSE(fp);
}

/** read in banned names for .banname **/
void read_nban_data()
{
int i=0;
int lines=0;
char filename[FILE_NAME_LEN];
FILE *fp;

sprintf(filename,"%s",NBANFILE);

/* If file doesn't exist, create it */
if (!(fp=fopen(filename,"r"))) 
  {
   printf("SYSTEM: Cannot access nameban files..will create a new file\n");
   if (!(fp=fopen(filename,"a"))) {
      printf("SYSTEM: Cannot create new nameban file.\n");
#if defined(WIN32) && !defined(__CYGWIN32__)
WSACleanup();
#endif
      exit(0);
      }
   for (i=0;i<NUM_NAMEBANS;++i) {
        sprintf(mess,"%s\n",nbanned[i]);
        fputs(mess,fp);
        }
   i=0;
  }

FCLOSE(fp);

/*----------------------------------------------------------------*/
/* Count lines in file and compare to lines defined in global     */
/* If they differ, add the difference number of lines to the file */
/*----------------------------------------------------------------*/

lines = file_count_lines(filename);

if (lines < NUM_NAMEBANS) {
    i = lines;
    fp=fopen(filename,"a");
    printf("Nameban global is higher than banned space in file. Fixing..\n");
    for (i=lines;i<NUM_NAMEBANS;++i) {
        sprintf(mess,"%s\n",nbanned[i]);
        fputs(mess,fp);
        }
     FCLOSE(fp);
    }
    
    fp=fopen(filename,"r");
    i=0;

 for (i=0;i<NUM_NAMEBANS;++i) {
     fscanf(fp,"%s\n",nbanned[i]);
     }
 
 i=0;
    
FCLOSE(fp);
}


/* change user1 to user2 in exempt file */
/* user 1 is passed all lowercase */
/* user 2 is passed correctly capitalized */
int change_exem_data(char *user1, char *user2)
{
int i;
char tempname[NAME_LEN+1];

     for (i=0;i<NUM_EXPIRES;++i) {
       strcpy(tempname,expired[i]);
       strtolower(tempname);
      if (!strcmp(tempname, user1)) {
        strcpy(expired[i],user2);
        if (!write_exem_data()) return 0;
        i=0;
        return 1;
        }
      } /* end of for */

return 0;
}


/* remove user from exempt file */
int remove_exem_data(char *user1)
{
int i;
char tempname[NAME_LEN+1];

     for (i=0;i<NUM_EXPIRES;++i) {
       strcpy(tempname,expired[i]);
       strtolower(tempname);
      if (!strcmp(tempname, user1)) {
        strcpy(expired[i],"name");
        if (!write_exem_data()) return 0;
        i=0;
        return 1;
        }
      } /* end of for */

return 0;
}


/*** write out exempted users for expire ***/
int write_exem_data()
{
int i;
char filename[FILE_NAME_LEN];
FILE *fp;

sprintf(filename,"%s",EXEMFILE);
if (!(fp=fopen(filename,"w")))
  {
   sprintf(mess,"Cannot write exempted users to %s\n",filename);
   print_to_syslog(mess);
   return 0;
  }

for (i=0;i<NUM_EXPIRES;++i) {
    sprintf(mess,"%s\n",expired[i]);
    fputs(mess,fp);
    }

FCLOSE(fp);
return 1;
}

/*** write out banned names for .banname ***/
int write_nban_data()
{
int i;
char filename[FILE_NAME_LEN];
FILE *fp;

sprintf(filename,"%s",NBANFILE);
if (!(fp=fopen(filename,"w")))
  {
   sprintf(mess,"Cannot write banned names to %s\n",filename);
   print_to_syslog(mess);
   return 0;
  }

for (i=0;i<NUM_NAMEBANS;++i) {
    sprintf(mess,"%s\n",nbanned[i]);
    fputs(mess,fp);
    }

FCLOSE(fp);
return 1;
}


/*** init user structure ***/
void init_user_struct()
{
int u,v;

puts("Initialising user structure...");
for (u=0; u<MAX_USERS; ++u) 
  {
   ustr[u].login_name[0] = 0;
   ustr[u].say_name[0]   = 0;
   ustr[u].name[0]       = 0;
   ustr[u].login_pass[0] = 0;
   ustr[u].password[0]   = 0;
   ustr[u].email_addr[0] = 0;
   ustr[u].desc[0]       = 0;
   ustr[u].sex[0]        = 0;
   ustr[u].fail[0]       = 0;
   ustr[u].succ[0]       = 0;
   ustr[u].security[0]   = 0;
   ustr[u].monitor       = 0;
   ustr[u].afk           = 0;
   ustr[u].lockafk       = 0;
   ustr[u].net_name[0]   = 0;
   ustr[u].last_name[0]  = 0;
   ustr[u].init_netname[0]  = 0;
   ustr[u].last_site[0]  = 0;
   ustr[u].init_site[0]  = 0;
   ustr[u].site[0]       = 0;

   ustr[u].sock          = -1;
   ustr[u].area          = -1; 
   ustr[u].invite        = -1;  
   ustr[u].super         = 0;
   ustr[u].vis           = 1;  
   ustr[u].warning_given = 0;
   ustr[u].logging_in    = 0;
   ustr[u].conv_count    = 0;
   ustr[u].mutter[0]     = 0;
   ustr[u].phone_user[0] = 0;
   ustr[u].homepage[0]   = 0;
   ustr[u].creation[0]   = 0;
   ustr[u].numcoms       = 0;
   ustr[u].mail_num      = 0;
   ustr[u].numbering     = 0;
   ustr[u].cat_mode      = 0;
   ustr[u].rows          = 24;
   ustr[u].cols          = 256;
   ustr[u].car_return    = 1;
   ustr[u].abbrs         = 1;
   ustr[u].white_space   = 1;
   ustr[u].line_count    = 0;
   ustr[u].number_lines  = 0;
   ustr[u].times_on      = 0;
   ustr[u].aver          = 0;
   ustr[u].totl          = 0;
   ustr[u].autor         = 0;
   ustr[u].autof         = 0;
   ustr[u].automsgs      = 0;
   ustr[u].gagcomm       = 0;
   ustr[u].semail        = 0;
   ustr[u].quote         = 1;
   ustr[u].hilite        = 0;
   ustr[u].new_mail      = 0;
   ustr[u].color         = COLOR_DEFAULT;
   ustr[u].real_id[0]    = 0;
   ustr[u].friend_num    = 0;
   ustr[u].revokes_num   = 0;
   ustr[u].gag_num       = 0;
   ustr[u].nerf_shots    = 5;
   ustr[u].nerf_energy   = 10;
   ustr[u].nerf_kills    = 0;
   ustr[u].nerf_killed   = 0;
   ustr[u].rawtime       = 0;
   ustr[u].passhid       = 0;
   ustr[u].pbreak        = 0;
   ustr[u].beeps         = 0;
   ustr[u].mail_warn     = 0;
   ustr[u].muz_time      = 0;
   ustr[u].xco_time      = 0;
   ustr[u].gag_time      = 0;
   ustr[u].frog          = 0;
   ustr[u].frog_time     = 0;
   ustr[u].anchor        = 0;
   ustr[u].anchor_time   = 0;
   ustr[u].promote       = 0;
   ustr[u].home_room[0]  = 0;
   ustr[u].entermsg[0]   = 0;
   ustr[u].exitmsg[0]    = 0;
   ustr[u].afkmsg[0]     = 0;
   ustr[u].init_date[0]  = 0;
   ustr[u].last_date[0]  = 0;
   ustr[u].pro_enter     = 0;
   ustr[u].t_ent         = 0;
   ustr[u].t_num         = 0;
   ustr[u].t_name[0]     = 0;
   ustr[u].t_host[0]     = 0;
   ustr[u].t_ip[0]       = 0;
   ustr[u].t_port[0]     = 0;
   ustr[u].help          = 0;
   ustr[u].who           = 0;
   ustr[u].webpic[0]     = 0;
   ustr[u].rwho          = 1;
   ustr[u].tempsuper     = 0;
   ustr[u].promptseq     = 0;
   ustr[u].needs_hostname = 0;
   ustr[u].ttt_board	 = 0;
   ustr[u].ttt_opponent	 = -3;
   ustr[u].ttt_playing	 = 0;
   ustr[u].ttt_kills	 = 0;
   ustr[u].ttt_killed	 = 0;
   ustr[u].hang_wins	 = 0;
   ustr[u].hang_losses   = 0;
   ustr[u].hang_stage    = -1;
   ustr[u].hang_word[0]  = '\0';
   ustr[u].hang_word_show[0] = '\0';
   ustr[u].hang_guess[0] = '\0';
   ustr[u].icq[0]	 = 0;
   ustr[u].miscstr1[0]	 = 0;
   ustr[u].miscstr2[0]	 = 0;
   ustr[u].miscstr3[0]	 = 0;
   ustr[u].miscstr4[0]	 = 0;
   ustr[u].pause_login   = 0;
   ustr[u].miscnum2	 = 0;
   ustr[u].miscnum3	 = 0;
   ustr[u].miscnum4	 = 0;
   ustr[u].miscnum5	 = 0;
   listen_all(u);

   ustr[u].char_buffer[0]     = 0;
   ustr[u].char_buffer_size   = 0;

   for (v=0; v<NUM_LINES; v++)
     {
      ustr[u].conv[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_ALERT; v++)
     {
      ustr[u].friends[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_GAG; v++)
     {
      ustr[u].gagged[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_GRAVOKES; v++)
     {
      ustr[u].revokes[v][0]=0;
     }
   v=0;
   for (v=0; v<NUM_MACROS; v++)
     {
      ustr[u].Macros[v].body[0]=0;
      ustr[u].Macros[v].name[0]=0;
     }
   for (v=0; v<NUM_ABBRS; v++)
     {
      ustr[u].custAbbrs[v].com[0]=0;
      ustr[u].custAbbrs[v].abbr[0]=0;
     }

  } /* end of for */
}

/*** init user structure ***/
void reset_user_struct(int user)
{
int u=user;
int v;

   ustr[u].login_name[0] = 0;
   ustr[u].say_name[0]   = 0;
   ustr[u].name[0]       = 0;
   ustr[u].login_pass[0] = 0;
   ustr[u].password[0]   = 0;
   ustr[u].email_addr[0] = 0;
   ustr[u].desc[0]       = 0;
   ustr[u].sex[0]        = 0;
   ustr[u].fail[0]       = 0;
   ustr[u].succ[0]       = 0;
   ustr[u].security[0]   = 0;
   ustr[u].monitor       = 0;
   ustr[u].afk           = 0;
   ustr[u].lockafk       = 0;
   ustr[u].last_name[0]  = 0;
   ustr[u].init_netname[0]  = 0;
   ustr[u].last_site[0]  = 0;
   ustr[u].init_site[0]  = 0;

   ustr[u].area          = -1; 
   ustr[u].invite        = -1;  
   ustr[u].super         = 0;
   ustr[u].vis           = 1;  
   ustr[u].warning_given = 0;
   ustr[u].conv_count    = 0;
   ustr[u].mutter[0]     = 0;
   ustr[u].phone_user[0] = 0;
   ustr[u].homepage[0]   = 0;
   ustr[u].creation[0]   = 0;
   ustr[u].numcoms       = 0;
   ustr[u].mail_num      = 0;
   ustr[u].numbering     = 0;
   ustr[u].cat_mode      = 0;
   ustr[u].rows          = 24;
   ustr[u].cols          = 256;
   ustr[u].car_return    = 1;
   ustr[u].abbrs         = 1;
   ustr[u].white_space   = 1;
   ustr[u].line_count    = 0;
   ustr[u].number_lines  = 0;
   ustr[u].times_on      = 0;
   ustr[u].aver          = 0;
   ustr[u].totl          = 0;
   ustr[u].autor         = 0;
   ustr[u].autof         = 0;
   ustr[u].automsgs      = 0;
   ustr[u].gagcomm       = 0;
   ustr[u].semail        = 0;
   ustr[u].quote         = 1;
   ustr[u].hilite        = 0;
   ustr[u].new_mail      = 0;
   ustr[u].color         = COLOR_DEFAULT;
   ustr[u].real_id[0]    = 0;
   ustr[u].friend_num    = 0;
   ustr[u].revokes_num   = 0;
   ustr[u].gag_num       = 0;
   ustr[u].nerf_shots    = 5;
   ustr[u].nerf_energy   = 10;
   ustr[u].nerf_kills    = 0;
   ustr[u].nerf_killed   = 0;
   ustr[u].rawtime       = 0;
   ustr[u].passhid       = 0;
   ustr[u].pbreak        = 0;
   ustr[u].beeps         = 0;
   ustr[u].mail_warn     = 0;
   ustr[u].muz_time      = 0;
   ustr[u].xco_time      = 0;
   ustr[u].gag_time      = 0;
   ustr[u].frog          = 0;
   ustr[u].frog_time     = 0;
   ustr[u].anchor        = 0;
   ustr[u].anchor_time   = 0;
   ustr[u].promote       = 0;
   ustr[u].home_room[0]  = 0;
   ustr[u].entermsg[0]   = 0;
   ustr[u].exitmsg[0]    = 0;
   ustr[u].afkmsg[0]     = 0;
   ustr[u].init_date[0]  = 0;
   ustr[u].last_date[0]  = 0;
   ustr[u].pro_enter     = 0;
   ustr[u].t_ent         = 0;
   ustr[u].t_num         = 0;
   ustr[u].t_name[0]     = 0;
   ustr[u].t_host[0]     = 0;
   ustr[u].t_ip[0]       = 0;
   ustr[u].t_port[0]     = 0;
   ustr[u].help          = 0;
   ustr[u].who           = 0;
   ustr[u].webpic[0]     = 0;
   ustr[u].rwho          = 1;
   ustr[u].tempsuper     = 0;
   ustr[u].needs_hostname = 0;
   ustr[u].ttt_board	 = 0;
   ustr[u].ttt_opponent	 = -3;
   ustr[u].ttt_playing	 = 0;
   ustr[u].ttt_kills	 = 0;
   ustr[u].ttt_killed	 = 0;
   ustr[u].hang_wins	 = 0;
   ustr[u].hang_losses 	 = 0;
   ustr[u].hang_stage    = -1;
   ustr[u].hang_word[0]  = '\0';
   ustr[u].hang_word_show[0] = '\0';
   ustr[u].hang_guess[0] = '\0';
   ustr[u].icq[0]	 = 0;
   ustr[u].miscstr1[0]	 = 0;
   ustr[u].miscstr2[0]	 = 0;
   ustr[u].miscstr3[0]	 = 0;
   ustr[u].miscstr4[0]	 = 0;
   ustr[u].pause_login   = 0;
   ustr[u].miscnum2	 = 0;
   ustr[u].miscnum3	 = 0;
   ustr[u].miscnum4	 = 0;
   ustr[u].miscnum5	 = 0;
   listen_all(u);

   ustr[u].char_buffer[0]     = 0;
   ustr[u].char_buffer_size   = 0;

   for (v=0; v<NUM_LINES; v++)
     {
      ustr[u].conv[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_ALERT; v++)
     {
      ustr[u].friends[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_GAG; v++)
     {
      ustr[u].gagged[v][0]=0;
     }
   v=0;
   for (v=0; v<MAX_GRAVOKES; v++)
     {
      ustr[u].revokes[v][0]=0;
     }
   v=0;
   for (v=0; v<NUM_MACROS; v++)
     {
      ustr[u].Macros[v].body[0]=0;
      ustr[u].Macros[v].name[0]=0;
     }
   v=0;
   for (v=0; v<NUM_ABBRS; v++)
     {
      ustr[u].custAbbrs[v].com[0]=0;
      ustr[u].custAbbrs[v].abbr[0]=0;
     }

}


/*** init area structure ***/
void init_area_struct()
{
int a,n;

puts("Initialising area structure & file pointers...");
for (a=0;a<NUM_AREAS;++a) 
  {
   astr[a].private=0; 
   astr[a].conv_line=0;
   for (n=0;n<NUM_LINES;++n) 
      conv[a][n][0]=0;
  }
}

/*** init misc. port structures ***/
void init_misc_struct()
{
int a;

puts("Initialising who and web port structures...");
for (a=0;a<MAX_WHO_CONNECTS;++a) 
  {
   whoport[a].sock=-1; 
   whoport[a].site[0]=0;
   whoport[a].net_name[0]=0;
  }
a=0;
for (a=0;a<MAX_WWW_CONNECTS;++a) 
  {
   wwwport[a].sock=-1;
   wwwport[a].method=-1;
   wwwport[a].req_length=0;
   wwwport[a].keypair[0]=0;
   wwwport[a].file[0]=0;
   wwwport[a].site[0]=0;
   wwwport[a].net_name[0]=0;
  }

}

