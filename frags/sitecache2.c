
How about caching sites. A structure that can hold say 20 sites. Check the
ip against the ips in the cache, if they match, use the resolved hostname
in the cache, if not, resolve it and add it to the structure. Another to
do it would be to save every ip and hostname to a file and check ip's
against the ip's in the file. Granted the list would probably get huge.
It's a trade-off i guess.

/* This is the file version */

#define SITECACHE_SIZE 25
#define SITECACHE_FILE "lib/sitecache"

/* Get addy in main() function..ip_address defined as unsigned long */
ip_address = acc_addr.sin_addr.s_addr;
check_cache(user,ip_address);

/* then call this. addr is called ip_address */
void check_cache(user,addr)
unsigned long addr;
{
int i=0;
int lines=0;
int found=0;
int full=0;
char ipsite[30];
char hostsite[70];
char line[101];
char filename[FILE_NAME_LEN];
char filename2[FILE_NAME_LEN];
struct hostent *he;
static char ip[256];
FILE *fp;
FILE *fp2;

 /* Resolve network address to ip numbers */
 addr=ntohl(addr);
 sprintf(ip,"%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff,
         (addr >> 8) & 0xff, addr & 0xff);
 strcpy(ustr[user].site, ip);

 /* Check ip against cached list */
 sprintf(filename,"%s",SITECACHE_FILE);
 if (!(fp=fopen(filename,"r"))) {
    /* this is whatever function you use to resolve a network */
    /* address to a hostname */
    he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
    if (he && he->h_name)
       strcpy(ustr[user].net_name, he->h_name);
    else
       strcpy(ustr[user].net_name, SYS_LOOK_FAILED);
    goto ADDSITE;
  }

 fgets(line,100,fp);
 while(!feof(fp)) {
      sscanf(line,"%s %s",ipsite,hostsite);
      if (!strcmp(ipsite,ustr[user].site)) {
        strcpy(ustr[user].net_name,hostsite);
        found=1;
        break;
        }
      fgets(line,100,fp);
   } /* end of while */
 fclose(fp);
if (found) return;
else {
  /* this is whatever function you use to resolve a network */
  /* address to a hostname */
 he = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
 if (he && he->h_name)
    strcpy(ustr[user].net_name, he->h_name);
 else
    strcpy(ustr[user].net_name, SYS_LOOK_FAILED);

  /* Check for empty slot in cache */
  lines=file_count_lines(filename);
  if (lines >= SITECACHE_SIZE) full=1;
  if (!full) {
   ADDSITE:
    fp=fopen(filename,"a");
    sprintf(line,"%s %s",ustr[user].site,ustr[user].net_name);
    fputs(line,fp);
    fputs("\n",fp);
    fclose(fp);
    return;
   }
  else {
   /* Cache full, bump site at top and move all others up to make */
   /* room for new site at end */
   strcpy(filename2,get_temp_file());
   fp2=fopen(filename2,"w");
   fp=fopen(filename,"r");
   /* get rid of first entry because we write from second one on */
   fgets(line,100,fp);
   fgets(line,100,fp);
   while(!feof(fp)) {
        fputs(line,fp2);
        fputs("\n",fp2);
        fgets(line,100,fp);
     } /* end of while */
   fclose(fp);
   sprintf(line,"%s %s",ustr[user].site,ustr[user].net_name);
   fputs(line,fp2);
   fputs("\n",fp2);   
   fclose(fp2);
   rename(filename2,filename);
  } /* end of full else */
 } /* end of found else */
}

