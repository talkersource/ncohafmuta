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

extern char mess[ARR_SIZE+25];    /* functions use mess to send output   */
extern char t_mess[ARR_SIZE+25];  /* functions use t_mess as a buffer    */


/*----------------------------------------------*/
/* Check to see if a site or hostname is banned */
/*----------------------------------------------*/
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
    write_log(ERRLOG,YESTIME,
    "Directory information not found for directory \"%s\" in check_restriction %s\n",
    filerid,get_error());
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


/*-----------------------------------------*/
/* Check restrictions for who and www port */
/*-----------------------------------------*/
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


/*-----------------------------------------------------------------------*/
/* check if site ends are the same (search starts from the end of string */
/* for hostnames, beginning of string for ips)                           */
/*-----------------------------------------------------------------------*/
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


/*-----------------------------------------------*/
/* Auto-restrict a site that is hacking on login */
/*-----------------------------------------------*/
void auto_restrict(int user)
{
char timestr[30];
char filename[FILE_NAME_LEN];
FILE *fp;

sprintf(t_mess,"%s/%s", RESTRICT_DIR, ustr[user].site);
strncpy(filename, t_mess, FILE_NAME_LEN);

 if (!(fp=fopen(filename,"w"))) {
  return;
 }

sprintf(timestr,"%ld\n",(unsigned long)time(0));
fputs(timestr,fp);
FCLOSE(fp);

/* Add set reason to reason file */
sprintf(t_mess,"%s/%s.r", RESTRICT_DIR,ustr[user].site);
strncpy(filename, t_mess, FILE_NAME_LEN);

 if (!(fp=fopen(filename,"w"))) {
  return;
 }

fputs("Your site is denied access for hacking\n",fp);
FCLOSE(fp);

/* Add set comment to comment file */
sprintf(t_mess,"%s/%s.c",RESTRICT_DIR,ustr[user].site);
strncpy(filename, t_mess, FILE_NAME_LEN);

 if (!(fp=fopen(filename,"w"))) {
  return;
 }

sprintf(mess,"Site denied access for hacking, possibly user %s\n",ustr[user].login_name);
fputs(mess,fp);
FCLOSE(fp);

write_log(BANLOG,YESTIME,
"AUTO-RESTRICT of site %s, possibly user %s\n",ustr[user].site,ustr[user].login_name);
}

/*-----------------------------------------------*/
/* Function to check name to see if it is banned */
/*-----------------------------------------------*/
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
 write_log(BANLOG,YESTIME,
 "BANNAME: User from site %s tried to login with banned name %s\n",sitename,str);
 return 1;
}
else return 0; 
           
}


