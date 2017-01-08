
How about caching sites. A structure that can hold say 20 sites. Check the
ip against the ips in the cache, if they match, use the resolved hostname
in the cache, if not, resolve it and add it to the structure. Another to
do it would be to save every ip and hostname to a file and check ip's
against the ip's in the file. Granted the list would probably get huge.
It's a trade-off i guess.

#define SITECACHE_SIZE 25

struct {
        char ip[21];   /* ip address */
        char host[64]; /* resolved address */
       } sitecache[SITECACHE_SIZE];

/* Get addy in main() function..ip_address defined as unsigned long */
ip_address = acc_addr.sin_addr.s_addr;
check_cache(user,ip_address);

/* then call this. addr is called ip_address */
void check_cache(user,addr)
unsigned long addr;
{
int i=0;
int found=0;
int full=0;
struct hostent *he;
static char ip[256];

 /* Resolve network address to ip numbers */
 addr=ntohl(addr);
 sprintf(ip,"%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff,
         (addr >> 8) & 0xff, addr & 0xff);
 strcpy(ustr[user].site, ip);

 /* Check ip against cached list */
 for (i=0;i<SITECACHE_SIZE;++i) {
    if (strlen(sitecache[i].ip)) {
      if (!strcmp(sitecache[i].ip,ustr[user].site)) {
        strcpy(ustr[user].net_name,sitecache[i].host);
        found=1;
        break;
      } /* end of sub-if */
    } /* end of if */
  } /* end of for */
i=0;

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
  for (i=i;i<SITECACHE_SIZE;++i) {
    if (strlen(sitecache[SITECACHE_SIZE-i].ip)) {
      if (i==1) { full=1; break; } /* cache if full,start bumping sites */
      else continue;
     } /* end of sub-if */
    else {
     /* Free slot open */
     strcpy(sitecache[SITECACHE_SIZE-i].ip,ustr[user].site);
     strcpy(sitecache[SITECACHE_SIZE-i].host,ustr[user].net_name);
    } /* end of else */
   } /* end of for */
i=0;

/* If cache full, bump site at top and move all others up to make */
/* room for new site at end */
if (full) {
   for (i=1;i<SITECACHE_SIZE;++i) {   
     strcpy(sitecache[i-1].ip,sitecache[i].ip);
     strcpy(sitecache[i-1].host,sitecache[i].host);
   } /* end of for */
 
  /* Copy new site to end */
    strcpy(sitecache[SITECACHE_SIZE-1].ip,ustr[user].site);
    strcpy(sitecache[SITECACHE_SIZE-1].host,ustr[user].net_name);
 } /* end of if */

 } /* end of else */
}

