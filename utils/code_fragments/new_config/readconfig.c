#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEF_CONFIG_FILE "running_config.dat"

struct room_struct {
	char name[80];
	char owner[80];
	char descfile[256];
	struct room_struct_exits *exit;
	int security;
	int windy;
	int hidden;
	int boardsecurity;
	struct room_struct *prev,*next;
	};
struct room_struct *room_first,*room_last;

struct room_struct_exits {
	char name[80];
	struct room_struct_exits *prev,*next;	
	};
struct room_struct_exits *exit_first,*exit_last;

struct config_struct {
	char name[256];
	int type;
	int valuedata;
	char stringdata[256];
	struct config_struct *prev,*next;
	};
struct config_struct *run_config_first,*run_config_last;

void init_config_struct(struct config_struct *ptr);
void init_room_struct(struct room_struct *ptr);
void remove_first(char *inpstr);

int main(void) {
int aa=0;
char configtype[256];
char configname[256];
char line[512];
char tempvalue[512];
FILE *fp;
struct config_struct *config_ptr;
struct room_struct *room_ptr;
struct room_struct_exits *room_exit_ptr;
struct config_struct *tempconfig;

 if (!(fp=fopen(DEF_CONFIG_FILE,"r"))) {
	printf("Can't open config file!\n");
	return 0;
	}

 while (fgets(line,512,fp) != NULL) {
  line[strlen(line)-1]=0;
  if (line[0]=='#' || !strlen(line)) continue;
  sleep(1);
  printf("READ LINE: %s\n",line);
  sscanf(line,"%s",configtype);
  remove_first(line);

   if (!strcmp(configtype,"value")) {
    sscanf(line,"%s",configname);
    remove_first(line);
    strcpy(tempvalue,line);
    printf("PARSED: %s %s %s\n",configtype,configname,tempvalue);

    if ((config_ptr=(struct config_struct *)malloc(sizeof(struct config_struct)))==NULL) {
        printf("Malloc failed for config_struct\n");
        return 0;
        }

    if (run_config_first==NULL) {
     run_config_first=config_ptr;
     config_ptr->prev=NULL;
    }
    else {
     run_config_last->next=config_ptr;
     config_ptr->prev=run_config_last;
    }
    config_ptr->next=NULL;
    run_config_last=config_ptr;

    init_config_struct(config_ptr);

    strncpy(config_ptr->name,configname,sizeof(config_ptr->name));
    config_ptr->type=1;
    config_ptr->valuedata=atoi(tempvalue);

    printf("VALUES: NAME %s TYPE %d VALUE %d\n",config_ptr->name,config_ptr->type,config_ptr->valuedata);
	for (tempconfig=run_config_first;tempconfig!=NULL;tempconfig=tempconfig->next) {
	 if (tempconfig->type==1) printf("V %s %d\n",tempconfig->name,tempconfig->valuedata);
	 else if (tempconfig->type==2) printf("S %s %s\n",tempconfig->name,tempconfig->stringdata);
	}
   } /* value */
   else if (!strcmp(configtype,"string")) {
    sscanf(line,"%s",configname);
    remove_first(line);
    strcpy(tempvalue,line);
    printf("PARSED: %s %s %s\n",configtype,configname,tempvalue);

    if ((config_ptr=(struct config_struct *)malloc(sizeof(struct config_struct)))==NULL) {
        printf("Malloc failed for config_struct\n");
        return 0;
        }

    if (run_config_first==NULL) {
     run_config_first=config_ptr;
     config_ptr->prev=NULL;
    }
    else {
     run_config_last->next=config_ptr;
     config_ptr->prev=run_config_last;
    }
    config_ptr->next=NULL;
    run_config_last=config_ptr;

    init_config_struct(config_ptr);

    strncpy(config_ptr->name,configname,sizeof(config_ptr->name));
    config_ptr->type=2;
    strncpy(config_ptr->stringdata,tempvalue,sizeof(config_ptr->stringdata));

    printf("VALUES: NAME %s TYPE %d VALUE %s\n",config_ptr->name,config_ptr->type,config_ptr->stringdata);
	for (tempconfig=run_config_first;tempconfig!=NULL;tempconfig=tempconfig->next) {
	 if (tempconfig->type==1) printf("V %s %d\n",tempconfig->name,tempconfig->valuedata);
	 else if (tempconfig->type==2) printf("S %s %s\n",tempconfig->name,tempconfig->stringdata);
	}
   } /* string */
   else if (!strcmp(configtype,"room")) {
    
    sscanf(line,"%s",configname);
    remove_first(line);
    strcpy(tempvalue,line);
    printf("PARSED: %s %s %s\n",configtype,configname,tempvalue);

    if ((room_ptr=(struct room_struct *)malloc(sizeof(struct room_struct)))==NULL) {
        printf("Malloc failed for room_struct\n");
        return 0;
        }

    if (room_first==NULL) {
     room_first=room_ptr;
     room_ptr->prev=NULL;
    }
    else {
     room_last->next=room_ptr;
     room_ptr->prev=room_last;
    }
    room_ptr->next=NULL;
    room_last=room_ptr;

    init_room_struct(room_ptr);

   } /* room*/

 } /* while */

} /* main */



void init_config_struct(struct config_struct *ptr) {

ptr->name[0]='\0';
ptr->type=0;
ptr->valuedata=0;
ptr->stringdata[0]='\0';

}

void init_room_struct(struct room_struct *ptr) {

ptr->name[0]='\0';
ptr->owner[0]='\0';
ptr->descfile[0]='\0';
ptr->exit=NULL;
ptr->windy=0;
ptr->hidden=0;
ptr->boardsecurity=0;

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

