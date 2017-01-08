#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "constants.h"
#include "protos.h"


/*** Resolve sock address to ip or hostname ***/
void resolve_add(int user_wait, unsigned long addr, int mode)
{
int result=0;
int lines=0;
int full=0;
char line[257];
char buf[256];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
FILE *fp;
FILE *fp2;
struct hostent *he;

/* Resolve sock address to hostname, if cant copy failed message */
if (mode==2) {
 /* site wide cache lookup? */
 if (resolve_names==2) {
  if (cache_lookup(user_wait)==-1) {
   if (request_cache_lookup(user_wait)==-1) {
     strcpy(ustr[user_wait].net_name, SYS_LOOK_FAILED);
     }
  }
 } /* end of resolve_names via site cache if */
 else if (resolve_names==1) {
 /* talker is using its own cache files instead */
 /* do the lookup in here                       */
  result = cache_lookup(user_wait);
  if (result <= -1) {
    /* this is whatever function you use to resolve a network */
    /* address to a hostname */
    he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
    if (he && he->h_name)
       strcpy(ustr[user_wait].net_name, he->h_name);
    else
       strcpy(ustr[user_wait].net_name, SYS_LOOK_FAILED);

    sprintf(filename,"%s",SITECACHE_FILE);
    if (result==-1)
     goto ADDSITE;
    else if (result==-2) {
    /* Check for empty slot in cache */
    lines=file_count_lines(filename);
    if (lines >= SITECACHE_SIZE) full=1;
    if (!full) {
    ADDSITE:
    fp=fopen(filename,"a");
    sprintf(line,"%s %s",ustr[user_wait].site,ustr[user_wait].net_name);
    fputs(line,fp);
    fputs("\n",fp);
    fclose(fp);
    system_stats.cache_misses++;
    return;
   }
  else {
   /* Cache full, bump site at top and move all others up to make */
   /* room for new site at end */
   strcpy(filename2,get_temp_file());
   fp2=fopen(filename2,"w");
   fp=fopen(filename,"r");
   /* get rid of first entry because we write from second one on */
   fgets(line,256,fp);
   line[0]=0;
   while (fgets(line,256,fp) != NULL) {
        fputs(line,fp2);
     } /* end of while */
   fclose(fp);
   sprintf(line,"%s %s",ustr[user_wait].site,ustr[user_wait].net_name);
   fputs(line,fp2);
   fputs("\n",fp2);
   fclose(fp2);
   remove(filename);
   rename(filename2,filename);
   system_stats.cache_misses++;  
   } /* end of full else */  
   } /* end of if result == -2 */
  } /* end of result if */
 } /* end of resolve_names via talker cache else if */
} /* end of mode if */
/* Copy ip address to user structure */
else if (mode==1) {
 addr=ntohl(addr);
 buf[0]=0;
 sprintf(buf,"%ld.%ld.%ld.%ld", (addr >> 24) & 0xff, (addr >> 16) & 0xff,
         (addr >> 8) & 0xff, addr & 0xff);
 strcpy(ustr[user_wait].site, buf);
 }
}


/* Cache lookup */
int cache_lookup(int user)
{
int found=0;
char ipsite[30];
char hostsite[257];
char line[257];
char filename[FILE_NAME_LEN];
FILE *fp;

/* ok, are we gonna do a talker-wide or site-wide search? */
if (resolve_names==1) {
 /* ok, talker-wide */
 /* start with default file */
 sprintf(filename,"%s",SITECACHE_FILE_DEF);
 if (!(fp=fopen(filename,"r"))) { }
 else {
 while(fgets(line,256,fp) != NULL) {
        line[strlen(line)-1]=0;   
        sscanf(line,"%s",ipsite);
        remove_first(line);
        strcpy(hostsite,line);
      if (!strcmp(ipsite,ustr[user].site)) {
        strcpy(ustr[user].net_name,hostsite);
        found=1;
        system_stats.cache_hits++;
        break;
        }
   } /* end of while */
 fclose(fp);
 line[0]=0;
 hostsite[0]=0;
 ipsite[0]=0;
 if (found) return 1;
 } /* end of default file lookup else */

 /* now main cache file */
 sprintf(filename,"%s",SITECACHE_FILE);
 if (!(fp=fopen(filename,"r"))) {
    return -1;
  }
 
 while(fgets(line,256,fp) != NULL) {
        line[strlen(line)-1]=0;   
        sscanf(line,"%s",ipsite);
        remove_first(line);
        strcpy(hostsite,line);
      if (!strcmp(ipsite,ustr[user].site)) {
        strcpy(ustr[user].net_name,hostsite);
        found=1;
        system_stats.cache_hits++;
        break;
        }
   } /* end of while */
 fclose(fp);
 line[0]=0;
if (found) return 1;
else return -2;
} /* end of resolve_names talker-wide if */
else if (resolve_names==2) {
  /* ok, site-wide */
  /* we'll use SITE_WIDE_CACHE_FILE */
  sprintf(filename,"%s",SITE_WIDE_CACHE_FILE);

  } /* end of resolve_names site-wide else if */

return 1;
}

/* Make a request for ip resolution              */
/* return -1 if we can't write to the queue file */
int request_cache_lookup(int user)
{
char filename[256];

sprintf(filename,"%s",SITE_WIDE_REQUEST_FILE);

return 1;
}

