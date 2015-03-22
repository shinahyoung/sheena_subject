///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File Name	: srv.c												///
// Data		: 2014/05/23											///
// Os		: Ubuntu 12.04 LTS 64bits									///
// Author	: Shin Ah Young											///
// Student ID	: 2012722023											///
// -------------------------------------------------------------------------------------------------------------///
// Title : System Programing Assignment #2 (ftp server)								///
// Description : Linux ls (-a, -al, -l), dir, pwd, cd, cd.., mkdir, delete, rmdir, rename using socket		///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <sys/stat.h>
#include <arpa/inet.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/wait.h> 
#include <signal.h> 
#include <time.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#define BUF_SIZE 256 
#define MAX 5
#define BUF_MAX_SIZE 8196
//process inforamtion
struct info{
	char pid[BUF_SIZE];
	char port[BUF_SIZE];
	char start[BUF_SIZE];
	int time;
};
//ls information
typedef struct NODE{
	char pm[BUF_SIZE];
	char size[BUF_SIZE];
	char month[BUF_SIZE];
	char day[BUF_SIZE];
	char hour[BUF_SIZE];
	char min[BUF_SIZE];
	char name[BUF_SIZE];
	char uid[BUF_SIZE];
	char gid[BUF_SIZE];
	char n_link[BUF_SIZE];
	struct NODE *next;
}NODE;
//wild information
typedef struct WNODE{
	char check;
	char name[BUF_SIZE];
	struct WNODE *next;
}WNODE;

int server_fd, client_fd; 
struct info info[MAX];
NODE *head=NULL;
WNODE *whead=NULL;


void catch(char* buf,const char* op,char* fn); //catch NODE information
void insert(char* _pm,char* _size,char* _month,char* _day, char* _hour,char* _min, char *_name,char* _uid,char* _gid,char* _n_link); //insert NODE
void ls_read(char* path); //read ls information option -al
void sh_chld(int); // signal handler for SIGCHLD 
void sh_alrm(int); // signal handler for SIGALRM
void print(char* s); //write string
void printi(int d); //print integer
void sh_int(int); //signal handler for SIGINT
char* myitoa(int value, char*str); //integer -> string
void deleteAll(); // delete All NODE
void test_r(); //read process inforamtions and print
void test_w(int pid,int port,int start,char* buf); //write process inforamtions
void mymkdir(char* name,int pm,char* buf); //make directory
void myrmdir(char* name,char* buf); //remove directory
void mydelete(char* name,char* buf); //delete file
void test_d(int pid,char* buf); //delete process inforamtion
void myrm(char* name); //remove directory except contents 
void swap(NODE **ptr, NODE **ntr);  //swap NODE
void wswap(WNODE **ptr, WNODE **ntr); //swap WNODE
void node_sort(); //node sort -> ASCII
void wdeleteAll(); //delete All WNODE linked
void winsert(char _check,char *_name); //insert WNODE
int wild(char* dirname,int index, char* temp1,int check); //wildcard
void from_to_sort(int min,int max); //from min to max -> sorting 
void wcatch(char *buf,char* op); //catch WNODE information as op
void al_for_name(struct dirent *dirp,char* _pm, char* _uid, char *_gid, char* _size, char* _month, char* _day, char* _hour, char* _min, char* _n_link); //for name, al information getting

////////////////////////////////////////////////////////////////////////////
//wild									////
//======================================================================////
//Input: char* dirnaem -> directory name, or path			////
//	int index -> index, =count					////
//	char* temp1 -> token word except *				////
//	int check -> wild function round count				////
//Purpose: wildcard							////
////////////////////////////////////////////////////////////////////////////
int wild(char* dirname,int index, char* temp1,int check ){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	int i,start,ti=0;
	char *temp[BUF_SIZE],*temp2,*temp3,buff[BUF_SIZE],buff2[BUF_SIZE];
	start=index;
	check++; // to open directory only one time
	if((dp=opendir(dirname))==NULL){
		print("can't open ");
		print(dirname);
		print("\n");
		return 0;
	}
	getcwd(buff,256); //save current path
	chdir(dirname);	  //change directory -> dirname
	while((dirp=readdir(dp))!=NULL){
		if(stat(dirp->d_name,&statbuf)==-1){
			print("system call error\n");
			continue;
		}
		//wildcard
		if(temp1!=NULL){
			if((strstr(dirp->d_name,temp1)!=NULL)){
				//if directory & not ./.. & check=1	
				if(check==1&&S_ISDIR(statbuf.st_mode)&&strcmp(dirp->d_name,".")&&strcmp(dirp->d_name,"..")){//directory
					temp[ti]=dirp->d_name;
					ti++;				
				}
				//else if path = . &directory name = . || ..
				else if(!strcmp(dirname,".")&&(!strcmp(dirp->d_name,".")||!strcmp(dirp->d_name,"..")))
					continue;
				else{//file
					if(S_ISDIR(statbuf.st_mode)){
						winsert('g',dirp->d_name);
					}
					else{
						winsert('f',dirp->d_name);
					}
					index++; //add index 
				}
			}//strstr	
		}
		//not wildcard
		else{//temp1==null
			if(check==1&&S_ISDIR(statbuf.st_mode)&&strcmp(dirp->d_name,".")&&strcmp(dirp->d_name,"..")){//directory
				temp[ti]=dirp->d_name;
				ti++;				
			}
			else if(!strcmp(dirname,".")&&(!strcmp(dirp->d_name,".")||!strcmp(dirp->d_name,"..")))
				continue;

			else{//file
					if(S_ISDIR(statbuf.st_mode)){ //'g' : directory buf don't want recursive 
						winsert('g',dirp->d_name);
					}
					else{
						winsert('f',dirp->d_name);
					}
					index++;
			}
			
		}//else temp1==null
	}//end of while

	from_to_sort(start,index);	//sort
	for(i=0;i<ti;i++){		//about directory
		winsert('d',temp[i]);	//put directory information
		index++;
		index=wild(temp[i],index,NULL,check);	//call wild function using directory name
	}
	chdir(buff);
	closedir(dp);
	return index;
}


int main(int argc, char **argv) { 
   char buff[BUF_SIZE], *token[BUF_SIZE],*buf,*temp1; 
   int n,i; 
   struct sockaddr_in server_addr, client_addr; 
   int len,x,c; 
   int port;
   time_t t;
   NODE *walk;
   //initialize
	for(i=0;i<5;i++){
		memset(info[i].pid,0,BUF_SIZE);
		memset(info[i].port,0,BUF_SIZE);
		memset(info[i].start,0,BUF_SIZE);
		info[i].time=0;
	}

   /* Applying signal handler(sh_alrm) for SIGALRM */
   signal(SIGALRM, sh_alrm);

   /* Applying signal handler(sh_chld) for SIGCHLD */
   signal(SIGCHLD, sh_chld);
   signal(SIGINT,sh_int);
   server_fd = socket(AF_INET, SOCK_STREAM, 0);
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family=AF_INET;
   server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
   server_addr.sin_port=htons(atoi(argv[1]));
   bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
   listen(server_fd, 5);
   //make myproc directory to save process information in this directory
   mkdir("myproc",0755);
   //wait....until make myproc
   sleep(3);
   //FTP server start
   while(1)
   {
      pid_t pid;

      len = sizeof(client_addr);
      client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
	memset(&buff,0,sizeof(buff));
      //connect fail, server connect exit/proecess exit
      if(client_fd==-1){ 
         print("failed connect server\n");
         exit(1);         
      }
	//10 alarm clunt!
	alarm(10);
	pid=fork();
	
	if(!pid){ //child
		close(server_fd);
		//pid, port, start time save
		test_w(getpid(),ntohs(client_addr.sin_port),(int)time(&t),buf);
		sleep(1); //to wait until test_w complete...
                print("=============Client info==============\n");
		print("client IP: ");		
		print(inet_ntoa(client_addr.sin_addr));
		print("\nclient port: ");
		printi(ntohs(client_addr.sin_port));
		print(buff);
		print("\n");
                print("======================================\n");
		print("Child Process ID : ");
		printi((getpid()));
		print(buff);
		print("\n");
		test_r();
		//wait client command
		while((n=read(client_fd,buff,sizeof(buff)))>0){
			buf=(char *)malloc(sizeof(char)*BUF_MAX_SIZE);
			memset(buf,0,BUF_SIZE);
			strcpy(buf,buff);
			//token
			token[0]=strtok(buff," \n");
			for(i=1;;i++){
				token[i]=strtok(NULL," \n");
				if(token[i]==NULL)
					break;
			}
			if(!strcmp(token[0],"QUIT")){	//if QUIT,
				test_d(getpid(),buf);
				sleep(1);
				print("child Process(PID : ");
				printi(getpid());
				print(") will be terminated.\n");
				test_r();
				break;
			}
			else if(!strcmp(token[0],"PWD")){ //pwd
				if(getcwd(buff,256)==NULL){
					print("system call error\n");
					exit(1);
				}
				strcat(buf,buff);
				strcat(buf,"\n");
			}
			else if(!strcmp(token[0],"NLST")){ //ls
				ls_read(".");
				node_sort();
				if(i==1) catch(buf,NULL,NULL);//ls
				//ls [-a/-al/-l] or dir/file name 
				else if(i==2){
					if(!strcmp(token[1],"-a")||!strcmp(token[1],"-al")||!strcmp(token[1],"-l"))
						catch(buf,token[1],NULL);
					else{ //[l] [directory/file]
						if(!strcmp(token[1],"*")){ // * 
							//wildcard
							wild(".",0,NULL,0);
							wcatch(buf,NULL); //print as option
							wdeleteAll();    //delete the inforamtion all
						}

						else if(strchr(token[1],'*')!=NULL){ // srv*
							temp1=(char *)malloc(strlen(token[1]));
							strcpy(temp1,token[1]);
							temp1=strtok(temp1,"*");
							//wildcard
							wild(".",0,temp1,0);
							wcatch(buf,NULL);
							wdeleteAll();
						}
						else{ //not wildcard
							walk=head;
							while(walk!=NULL){
								if(!strcmp(walk->name,token[1])&&walk->pm[0]=='d')
								{
									deleteAll(); //"." information delete
									ls_read(walk->name); //walk->name information read
									node_sort();		// sorting
									c=1;				//check
									break;
								}
								c=0;
								walk=walk->next;
							}

							if(c==1){
								catch(buf,NULL,NULL); //print as option
							}
							else
								catch(buf,NULL,token[1]); //prins as option

						}
					}
				}
				else if(i==3){
					// ls [directory/file] [option]
					if(!strcmp(token[2],"-a")||!strcmp(token[2],"-al")||!strcmp(token[2],"-l")){
						if(!strcmp(token[1],"*")){ //[directory/file] => *
							wild(".",0,NULL,0);
							wcatch(buf,token[2]);
							wdeleteAll();
						}
					
						else if(strchr(token[1],'*')!=NULL){ //[directory/file]=>srv* (ex)
							temp1=(char *)malloc(strlen(token[1]));
							strcpy(temp1,token[1]);
							temp1=strtok(temp1,"*");
							//wildcard
							wild(".",0,temp1,0);
							wcatch(buf,token[2]);
							wdeleteAll();
						}
						else{ //not wildcard
							walk=head;
							while(walk!=NULL){
								if(!strcmp(walk->name,token[1])&&walk->pm[0]=='d')
								{
									deleteAll();
									ls_read(walk->name);
									node_sort();
									c=1;
									break;
								}
								walk=walk->next;
								c=0;
							}
							if(c==1)
								catch(buf,token[2],NULL); // above same..
							else
								catch(buf,token[2],token[1]);
						}	

					}
					//ls [option] [file/directory]
					else if(!strcmp(token[1],"-a")||!strcmp(token[1],"-al")||!strcmp(token[1],"-l")){
						if(!strcmp(token[2],"*")){ //[file/directory] => *
							wild(".",0,NULL,0);
							wcatch(buf,token[1]);
							wdeleteAll();
						}
						else if(strchr(token[2],'*')!=NULL){ //[file/directory] => srv* (ex)
							temp1=(char *)malloc(strlen(token[2]));
							strcpy(temp1,token[2]);
							temp1=strtok(temp1,"*");
							//wildcard
							wild(".",0,temp1,0);
							wcatch(buf,token[1]);
							wdeleteAll();
						}
						else{ //not wildcard
							walk=head;
							while(walk!=NULL){
								if(!strcmp(walk->name,token[1])&&walk->pm[0]=='d')
								{
									deleteAll();
									ls_read(walk->name);
									node_sort();
									c=1;
									break;
								}
								walk=walk->next;
								c=0;
							}
							if(c==1)
								catch(buf,token[1],NULL);
							else
								catch(buf,token[1],token[2]);
						}
					}
					else{strcat(buf,"error\n");exit(1);}
				}
				else{strcat(buf,"error\n");exit(1);}
				//process information node -> deleteAll
				deleteAll();
			}// nlst
			else if(!strcmp(token[0],"LIST")){ //dir [directory/path]
				ls_read(".");
				node_sort();
						if(!strcmp(token[1],"*")){ //wildcard *
							wild(".",0,NULL,0);
							wcatch(buf,"-al"); //ls -al same
							wdeleteAll();
						}
					
						else if(strchr(token[1],'*')!=NULL){ //wildcard srv* (ex)
							temp1=(char *)malloc(strlen(token[1]));
							strcpy(temp1,token[1]);
							temp1=strtok(temp1,"*");
							//wildcard
							wild(".",0,temp1,0);
							wcatch(buf,"-al");
							wdeleteAll();
						}
						else{ //not wildcard
							walk=head;
							while(walk!=NULL){
								if(!strcmp(walk->name,token[1])&&walk->pm[0]=='d')
								{
									deleteAll();
									ls_read(walk->name);
									node_sort();
									c=1;
									break;
								}
								walk=walk->next;
								c=0;
							}
							if(c==1)
								catch(buf,"-al",NULL);
							else
								strcat(buf,"not directory/don't exist\n");
						}

				deleteAll();			//delete information all
			}// list
			//change working directory [path]
			else if(!strcmp(token[0],"CWD")){
				if(chdir(token[1])==-1){strcat(buf,"can't find this directory\n");exit(1);}
			}// cwd
			//change working directory [..]
			else if(!strcmp(token[0],"CDUP")){
				if(chdir("..")==-1){strcat(buf,"can't find this directory\n");exit(1);}
			}// cdup
			//make directory
			else if(!strcmp(token[0],"MKD")){
				for(x=1;x<i;x++) //multi...
					mymkdir(token[x],0755,buf);
			}// mkd
			//delete [file]
			else if(!strcmp(token[0],"DELE")){
				for(x=1;x<i;x++)  //multi...
					mydelete(token[x],buf);
			}//dele
			//remove directory [directory]
			else if(!strcmp(token[0],"RMD")){
				for(x=1;x<i;x++) //multi...
					myrmdir(token[x],buf);
			}//rmd
			//rename old new
			else if(!strcmp(token[0],"RNFR")){
				if(rename(token[3],token[4])==-1){
					print(token[3]);strcat(buf,token[3]);
					print(":not exist directory or file\n");strcat(buf,":not exist directory or file\n");
				}
			}//rnfr
			else{strcat(buf,"what is this??\n");exit(1);}
			strcat(buf,"\n");
			//send to client
			printf("buf: %s\n",buf);
			if(write(client_fd,buf,BUF_MAX_SIZE)<0){
				print("write error\n");exit(1);			
			}
			free(buf);
            	}//while read
		close(client_fd);
		exit(0);
         }
      close(client_fd);
   }
   return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ls_read													///
//==============================================================================================================///
// input:	char* path -> path										///
//output:	nothing												///
//Purpose:	ls information read option: -al									///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void ls_read(char *path){
	DIR *dp;
	struct dirent *dirp;
	char buff[BUF_SIZE]={0,};
	char _pm[BUF_SIZE]={0,};
	char _name[BUF_SIZE]={0,},_uid[BUF_SIZE]={0,},_gid[BUF_SIZE]={0,},_size[BUF_SIZE]={0,},_month[BUF_SIZE]={0,},_day[BUF_SIZE]={0,},_hour[BUF_SIZE]={0,},_min[BUF_SIZE]={0,},_n_link[BUF_SIZE]={0,};
	
	//start!!
	getcwd(buff,256); //save current path
	dp=opendir(path);
	chdir(path); //move path
	while((dirp=readdir(dp))!=NULL){
		//name
		strcpy(_name,dirp->d_name);
		al_for_name(dirp,_pm,_uid,_gid,_size,_month,_day,_hour,_min,_n_link);	//earn information in al option
		insert(_pm,_size,_month,_day,_hour,_min,_name,_uid,_gid,_n_link);	//insert node having the information
	
	}
	closedir(dp);
	chdir(buff);	//move "current path"
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// al_for_name													///
//==============================================================================================================///
// input:	struct dirent *dirp -> to get information 									
//		char *_pm	     -> to save permission information						///				
//		char *_n_link	     -> to save hard link information						///				
//		char *_uid	     -> to save user name information						///				
//		char *_gpid	     -> to save group name information						///
//		char *_size           -> to save size information						///
//		char *_month          -> to save month information						///
//		char *_day            -> to save day information						///			
//		char *_hour           -> to save hour information						///
//		char *_min            -> to save min information						///
//output:	noting												///
//Purpose:	get ls -al option information									///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void al_for_name(struct dirent *dirp,char* _pm, char* _uid, char *_gid, char* _size, char* _month, char* _day, char* _hour, char* _min, char* _n_link){
	struct stat statbuf;
	struct passwd *pw;
	struct group *gr;
	struct tm *tm;
	char s[BUF_SIZE]={0,};
		if(lstat(dirp->d_name,&statbuf)<0){
			print("stat error\n");exit(1);
		}
		//type
		
		if(S_ISDIR(statbuf.st_mode)) _pm[0]='d';
		else if(S_ISLNK(statbuf.st_mode)) _pm[0]='l';
		else if(S_ISCHR(statbuf.st_mode)) _pm[0]='c';
		else if(S_ISBLK(statbuf.st_mode)) _pm[0]='b';
		else if(S_ISSOCK(statbuf.st_mode)) _pm[0]='s';
		else if(S_ISFIFO(statbuf.st_mode)) _pm[0]='p';
		else _pm[0]='-';
		//user permission
		if(statbuf.st_mode&S_IRUSR) _pm[1]='r';
		else _pm[1]='-';
		if(statbuf.st_mode&S_IWUSR) _pm[2]='w';
		else _pm[2]='-';
		if(statbuf.st_mode&S_IXUSR) _pm[3]='x';
		else if(statbuf.st_mode&S_ISUID) _pm[3]='s';
		else _pm[3]='-';
		//group permission
		if(statbuf.st_mode&S_IRGRP) _pm[4]='r';
		else _pm[4]='-';
		if(statbuf.st_mode&S_IWGRP) _pm[5]='w';
		else _pm[5]='-';
		if(statbuf.st_mode&S_IXGRP) _pm[6]='x';
		else if(statbuf.st_mode&S_ISGID) _pm[6]='s';
		else _pm[6]='-';
		//other permission
		if(statbuf.st_mode&S_IROTH) _pm[7]='r';
		else _pm[7]='-';
		if(statbuf.st_mode&S_IWOTH) _pm[8]='w';
		else _pm[8]='-';
		if(statbuf.st_mode&S_IXOTH) _pm[9]='x';
		else _pm[9]='-';

		//number of hardlink
		myitoa(statbuf.st_nlink,_n_link);

		//user name
		pw=getpwuid(statbuf.st_uid);
		strcpy(_uid,pw->pw_name);
		//myitoa(statbuf.st_uid,_uid);
		
		//group name
		gr=getgrgid(statbuf.st_gid);
		strcpy(_gid,gr->gr_name); 
		//myitoa(statbuf.st_gid,_gid);

		//size 
		myitoa(statbuf.st_size,_size);

		//time
		tm=localtime(&statbuf.st_mtime);
		//month
		switch(tm->tm_mon+1){
			case 1: strcpy(_month,"Jan"); break;
			case 2: strcpy(_month,"Feb"); break;
			case 3: strcpy(_month,"Mar"); break;
			case 4: strcpy(_month,"Apr"); break;
			case 5: strcpy(_month,"May"); break;
			case 6: strcpy(_month,"Jun"); break;
			case 7: strcpy(_month,"Jul"); break;
			case 8: strcpy(_month,"Aug"); break;
			case 9: strcpy(_month,"Sep"); break;
			case 10: strcpy(_month,"Oct"); break;
			case 11: strcpy(_month,"Nov"); break;
			case 12: strcpy(_month,"Dec"); break;
			default: print("error\n");break;
		}

		//day
		myitoa(tm->tm_mday,s);
		//day format 1~9 -> 01 ~ 09
		if(strlen(s)==1){
			strcpy(_day,"0");
			strcat(_day,s);
		}
		else strcpy(_day,s);
		
		//hour
		myitoa(tm->tm_hour,s);
		if(strlen(s)==1){
			strcpy(_hour,"0");
			strcat(_hour,s);
		}
		else strcpy(_hour,s);

		//min
		myitoa(tm->tm_min,s);
		if(strlen(s)==1){
			strcpy(_min,"0");
			strcat(_min,s);
		}
		else strcpy(_min,s);
		}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// wcatch													///
//==============================================================================================================///
// input:	char* buf -> to error										///
//		char *op            -> option									///
//output:	noting												///
//Purpose:	wild information catch as option in buf to print 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void wcatch(char *buf,char* op){
	int c=0,i,count=0;
	WNODE *cur;
	NODE *ncur;
	char temp[BUF_SIZE]={0,};
	if(whead==NULL)
		strcat(buf,"not content\n");
	else{
		cur=whead;
		while(cur!=NULL){
			if(op==NULL){	//nothing option	
				if(!strcmp(cur->name,".") || !strcmp(cur->name,"..")){ 
					cur=cur->next;
					continue;
				}
				if(cur->check=='f'||cur->check=='g'){ //g -> directory buf don't want recursive
					if(cur->check=='g')
						strcat(buf,"/"); //directory!
					strcat(buf,cur->name);
					// one line -> 5 file/directory
					if(!(++c%5)||(cur->next==NULL)) 
						strcat(buf,"\n");
					else 
						strcat(buf,"\t");
				}
				else {	//directory
					strcat(buf,"\n\n/");
					strcat(buf,cur->name);
					strcat(buf,":\n");
					c=0;
				}
				cur=cur->next;
			}
			else if(!strcmp(op,"-a")){
				if(cur->check=='f'||cur->check=='g'){
					if(cur->check=='g')
						strcat(buf,"/");
					strcat(buf,cur->name);
					if(!(++c%5)||(cur->next==NULL)) 
						strcat(buf,"\n");
					else 
						strcat(buf,"\t");
				}
				else{ //directory
					strcat(buf,"\n\n/");
					strcat(buf,cur->name);
					strcat(buf,":\n");
					c=0;
				}
				cur=cur->next;
			}
			else if(!strcmp(op,"-al")||!strcmp(op,"-l")){
				if(!strcmp(op,"-l")){ //-l option, . .. continue
					if(!strcmp(cur->name,".")||!strcmp(cur->name,"..")){
						cur=cur->next;
						continue;
					}
				}
				ncur=head;
				while(ncur!=NULL){	//catch informations!
						if(!strcmp(ncur->name,cur->name)){
							strcat(buf,ncur->pm);strcat(buf,"\t");
							strcat(buf,ncur->n_link);strcat(buf,"\t");
							strcat(buf,ncur->uid);strcat(buf,"\t");
							strcat(buf,ncur->gid);strcat(buf,"\t");
							strcat(buf,ncur->size);strcat(buf,"\t");
							strcat(buf,ncur->month);strcat(buf,"\t");
							strcat(buf,ncur->day);strcat(buf,"\t");
							strcat(buf,ncur->hour);strcat(buf,":");
							strcat(buf,ncur->min);strcat(buf,"\t");
							if(ncur->pm[0]=='d')
								strcat(buf,"/");
							strcat(buf,ncur->name);
							strcat(buf,"\n");
							break;
						}
						else
							ncur=ncur->next;
				}//while					
				cur=cur->next;
			}
			else {
				print("error\n");strcat(buf,"error\n");
				exit(1);
			}
		}//while	
		strcat(buf,"\n");
	}//end else
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// catch													///
//==============================================================================================================///
// input:	char* buf -> to error										///
//		char *op  -> option	
//		char* fn -> file/directory nam	e								///
//output:	noting												///
//Purpose:	information catch as option in buf to print 							///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void catch(char* buf,const char* op,char* fn){
	int c=0,i;
	int check=0;
	NODE *cur;
	char temp[BUF_SIZE]={0,},*t;
if(head==NULL)
	strcat(buf,"not content\n");
else{
	cur=head;
	while(cur!=NULL){
		if(op==NULL){		//not option -> ls
			if(!strcmp(cur->name,".") || !strcmp(cur->name,"..")){ cur=cur->next;continue;}
			if(fn!=NULL){
				if(strcmp(cur->name,fn)){
					if(cur->next==NULL&&!check){
						strcat(buf,fn);
						strcat(buf," not exist\n");
					}
					cur=cur->next;
					continue;
				}
				check=1;
			}
			if(cur->pm[0]=='d') // /[directory]
				strcat(buf,"/");
			strcat(buf,cur->name);
			if(!(++c%5)||(cur->next==NULL)||(fn!=NULL)) 
				strcat(buf,"\n");
			else 
				strcat(buf,"\t");
			cur=cur->next;
		}// not
		//ls -l, -al option
		else if(!strcmp(op,"-l")||!strcmp(op,"-al")){
			if(!strcmp(op,"-l")&&(!strcmp(cur->name,".") || !strcmp(cur->name,".."))) {
				cur=cur->next;
				continue;
			}
			if(fn!=NULL){ //fn => filename
				if(strcmp(cur->name,fn)){
					if(cur->next==NULL&&!check){
					strcat(buf,fn);
					strcat(buf," not exist\n");
					}
					cur=cur->next;
					continue;
				}
				check=1;
			}
		//catching informations!
			strcat(buf,cur->pm);strcat(buf,"\t");
			strcat(buf,cur->n_link);strcat(buf,"\t");
			strcat(buf,cur->uid);strcat(buf,"\t");
			strcat(buf,cur->gid);strcat(buf,"\t");
			strcat(buf,cur->size);strcat(buf,"\t");
			strcat(buf,cur->month);strcat(buf,"\t");
			strcat(buf,cur->day);strcat(buf,"\t");
			strcat(buf,cur->hour);strcat(buf,":");
			strcat(buf,cur->min);strcat(buf,"\t");
			if(cur->pm[0]=='d')
				strcat(buf,"/");
			strcat(buf,cur->name);
			strcat(buf,"\n");
			cur=cur->next;
		}// -l/-al
		//ls -a option
		else if(!strcmp(op,"-a")){
			if(fn!=NULL){ //file name not 
				if(strcmp(cur->name,fn)){
					if(cur->next==NULL&&!check){
					strcat(buf,fn);
					strcat(buf," not exist\n");
					}
					cur=cur->next;
					continue;
				}
				check=1;
			}
			if(cur->pm[0]=='d')
				strcat(buf,"/");
			strcat(buf,cur->name);
			if(!(++c%5)||(cur->next==NULL)) 
				strcat(buf,"\n");
			else 
				strcat(buf,"\t");
			cur=cur->next;
		}
		//error! not option	
		else {
			print("error\n");
			strcat(buf,"error\n");
			exit(1);
		}
	}
	//check
}//end else
}//end catch function
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// swap														///
//==============================================================================================================///
// input:	NODE **ptr -> to swap..										///
//		NODE **ntr  -> to swap..									///
//output:	noting												///
//Purpose:	swap NODE to sort				 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void swap(NODE **ptr, NODE **ntr){
	NODE *temp=*ptr;
	*ptr=*ntr;
	*ntr=temp;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// wswap													///
//==============================================================================================================///
// input:	WNODE **ptr -> to swap..									///
//		WNODE **ntr  -> to swap..									///
//output:	noting												///
//Purpose:	swap WNODE to sort				 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 	
void wswap(WNODE **ptr, WNODE **ntr){
	WNODE *temp=*ptr;
	*ptr=*ntr;
	*ntr=temp;
}	
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// node_sort													///
//==============================================================================================================///
// input:	nothing												///
//output:	noting												///
//Purpose:	sort linked list using NODE 			 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void node_sort(){
	NODE *cur=head, **ptr; //struct address
	int count=0,i,j;
	if(head==NULL){
		print("not content\n");
		return;
	}
	while(cur!=NULL){ //count!
		count++;
		cur=cur->next;
	}
	ptr=(NODE **)malloc(sizeof(NODE *)*count); //new ptr
	for(i=0,cur=head;i<count;cur=cur->next) 
		ptr[i++]=cur;				//save cur in array
	//sorting
	for(i=0;i<count-1;i++){	
		for(j=i+1;j<count;j++){
			if(strcmp(ptr[i]->name,ptr[j]->name)>0)
				swap(&ptr[i],&ptr[j]); //swap
		}
	}
	//relinking as sorted array ptr!
	head=ptr[0];
	for(i=1,cur=head;cur&&i<count;cur=cur->next)
		cur->next=ptr[i++];
	cur->next=NULL;
	
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// from_to_sort													///
//==============================================================================================================///
// input:	min, max											///
//output:	noting												///
//Purpose:	sort linked list using WNODE only index min~ max 		 				///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void from_to_sort(int min,int max){
	WNODE *cur=whead, **ptr;
	int count=0,i,j;
	if(whead==NULL){
		print("not content\n");
		return;
	}
	while(cur!=NULL){
		count++;
		cur=cur->next;
	}
	ptr=(WNODE **)malloc(sizeof(WNODE *)*count);
	for(i=0,cur=whead;i<count;cur=cur->next)
		ptr[i++]=cur;
	//sorting
	for(i=min;i<max-1;i++){ //min - max sorting
		for(j=i+1;j<max;j++){
			if(strcmp(ptr[i]->name,ptr[j]->name)>0){
				wswap(&ptr[i],&ptr[j]);
			}
		}
	}
	whead=ptr[0]; //relink
	for(i=1,cur=whead;cur&&i<count;cur=cur->next)
		cur->next=ptr[i++];
	cur->next=NULL;
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// insert													///
//==============================================================================================================///
// input:	char* _pm,_size,_month,_day,_hour,_min,_name,_uid,_gid,_n_link					///
//output:	noting												///
//Purpose:	insert node having inputs 			 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void insert(char* _pm,char* _size,char* _month,char* _day, char* _hour, char* _min,char *_name,char* _uid,char* _gid,char* _n_link){

	NODE *cur,*p;
	//put information into NODE
	p=(NODE *)malloc(sizeof(NODE));
	strcpy(p->pm,_pm);
	strcpy(p->size,_size);
	strcpy(p->month,_month);
	strcpy(p->day,_day);
	strcpy(p->hour,_hour);
	strcpy(p->min,_min);
	strcpy(p->name,_name);
	strcpy(p->uid,_uid);
	strcpy(p->gid,_gid);
	strcpy(p->n_link,_n_link);
	p->next=NULL;
	
	//link!
	if(head==NULL) head=p;
	else{
		cur=head;
		while(cur->next!=NULL)
			cur=cur->next;
		cur->next=p;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// winsert													///
//==============================================================================================================///
// input:	char _check ==> checking directory/file/directory but wan't recursive
//		char* _name											///
//output:	noting												///
//Purpose:	insert wnode having inputs 			 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void winsert(char _check,char* _name){
	WNODE *cur, *p;
	//create wnode
	p=(WNODE *)malloc(sizeof(WNODE));
	//put inpus into the wnode
	p->check=_check;
	strcpy(p->name,_name);
	p->next=NULL;

	//link!
	if(whead==NULL) whead=p;
	else{
		cur=whead;
		while(cur->next!=NULL)
			cur=cur->next;
		cur->next=p;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// deleteAll													///
//==============================================================================================================///
// input:	nothing												///
//output:	noting												///
//Purpose:	delete all node linked	 			 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void deleteAll(){
	NODE *cur=head, *temp;
	while(cur->next!=NULL){
		temp=cur;
		cur=cur->next;
		free(temp);	//free!
		temp=NULL;	//NULL
	}
	head=NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// wdeleteAll													///
//==============================================================================================================///
// input:	nothing												///
//output:	noting												///
//Purpose:	delete all wnode linked	 			 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void wdeleteAll(){
	WNODE *cur=whead,*temp;
	while(cur->next!=NULL){
		temp=cur;
		cur=cur->next;
		free(temp); //free!
		temp=NULL; //NULL
	}
	whead=NULL;
} 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// test_w													///
//==============================================================================================================///
// input:	pid, port, start(time), buf									///
//output:	noting												///
//Purpose:	make directory named pid and in the directory, make file port, time having port number 
//		and start time		 			 						///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void test_w(int pid,int port,int start,char* buf){
	FILE *fp;
	char buff[BUF_SIZE]={0,};
	char temp[BUF_SIZE]={0,};
	char temp2[BUF_SIZE]={0,};
	char temp3[BUF_SIZE]={0,};
	myitoa(pid,buff);
	strcpy(temp,"./myproc/");
	strcat(temp,buff);
	mymkdir(temp,0755,buf);
	strcpy(temp3,temp);

	// temp : ./myproc/pid/port
	// temp2: ./myproc/pid/start

	strcpy(temp2,temp);	
	strcat(temp,"/port");
	strcat(temp2,"/start");
	
	// make port.txt content: port number 
	fp=fopen(temp,"w");
	fputs(myitoa(port,buff),fp);
	fclose(fp);		


	// make start.txt content: start time
	fp=fopen(temp2,"w");
	fputs(myitoa(start,buff),fp);
	fclose(fp);

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// myrmdir													///
//==============================================================================================================///
// input:	name -> directory name wanted to remove
//		buf -> to error..										///
//output:	noting												///
//Purpose:	remove directory and error	 								///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void myrmdir(char* name,char* buf){
	if(rmdir(name)==-1){
		print(name);strcat(buf,name);
		print(" : not exist directory or exist file/directory in this directory\n");
		strcat(buf," : not exist directory or exist file/directory in this directory\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mydelete													///
//==============================================================================================================///
// input:	name -> file name wanted to delete
//		buf -> to error..										///
//output:	noting												///
//Purpose:	remove file and error		 								///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void mydelete(char* name,char* buf){
	if(unlink(name)==-1){
		print(name);strcat(buf,name);
		print(" : not exist file\n");strcat(buf,":not exist file\n"); 
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// test_d													///
//==============================================================================================================///
// input:	pid ->process id to want delete		
//		buf -> to error											///
//output:	noting												///
//Purpose:	remove process info having input pid								///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void test_d(int pid,char* buf){
	char buff[BUF_SIZE]={0,};
	char d_pid[BUF_SIZE]={0,};
	char f_port[BUF_SIZE]={0,};
	char f_time[BUF_SIZE]={0,};
	
	myitoa(pid,buff);	
	strcpy(d_pid,"./myproc/");	
	// d_pid : ./myproc/pid
	strcat(d_pid,buff);
	strcpy(f_port,d_pid);
	strcpy(f_time,d_pid);
	// f_port : ./myproc/pid/port
	strcat(f_port,"/port");
	// f_time : ./myproc/pid/time
	strcat(f_time,"/start");

	//delete port, time file
	mydelete(f_port,buf);
	mydelete(f_time,buf);
	//remove pid directory
	myrmdir(d_pid,buf);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// myrmdir													///
//==============================================================================================================///
// input:	name -> directory name wanted to create
//		buf -> to error..										///
//output:	noting												///
//Purpose:	create directory and error	 								///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void mymkdir(char* name,int pm,char* buf){
	if(mkdir(name,pm)==-1){
		print(name);strcat(buf,name);
		print(":");strcat(buf,":");
		print("already exist directory\n");strcat(buf,"already exist directory\n");
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// test_r													///
//==============================================================================================================///
// input:	nothing												///
//output:	noting												///
//Purpose:	read all file/directory in "myproc" and earn process information				///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void test_r(){
	DIR *dir;
	int pid,t_buf1,t_buf2,i;
	int k=0;
	FILE *fp,*fpp;
	time_t t;
	char buf[BUF_SIZE]={0,};
	char path[BUF_SIZE]={0,};
	struct dirent *entry;
	struct stat fileStat;
	//char time=time_t t-char start
	t_buf1=(int)time(&t);
	if((dir=opendir("./myproc/"))==NULL){
		print("opendir error\n");
		exit(1);
	}
	while((entry=readdir(dir))!=NULL){
		lstat(entry->d_name,&fileStat);
		if(!S_ISDIR(fileStat.st_mode)) 
			continue;
		if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,"..")) 	
			continue;
	//check
		
		// path: ./myproc/pid/port
		// buf: ./myproc/pid/start
		strcpy(path,"./myproc/");
		strcat(path,entry->d_name);
		strcpy(buf,path);
		strcat(path,"/port");
		strcat(buf,"/start");
		//file open
		fp=fopen(path,"r");
		fgets(info[k].port,BUF_SIZE,fp);
	
		fpp=fopen(buf,"r");
		fgets(info[k].start,BUF_SIZE,fpp);		
		t_buf2=atoi(info[k].start);		
		info[k].time=t_buf1-t_buf2;
		strcpy(info[k].pid,entry->d_name);
		k++;
		fclose(fp);
		fclose(fpp);
			
	}
	closedir(dir);
	//print the process informations!
	print("======================================\n");	
	print("number of client: ");
	printi(k);
	print("\n-------------------------------------\n");
	print("\tPID\tPORT\tTIME\n");
	for(i=0;i<k;i++){
		print("\t");print(info[i].pid);print("\t");print(info[i].port);
		print("\t");printi(info[i].time);print("\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sh_chld													///
//==============================================================================================================///
// input:	signum -> signal number										///
//output:	noting												///
//Purpose:	SIGCHLD hanlder			 								///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sh_chld(int signum) {
   int status;
   waitpid(-1,&status,0);
   print("Status of Child process was changed.\n");
   //wait(NULL);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sh_alrm													///
//==============================================================================================================///
// input:	signum -> signal number										///
//output:	noting												///
//Purpose:	SIGALRM hanlder -> after 10s, print process information						///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sh_alrm(int signum) {
	sleep(1);
	test_r();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// print													///
//==============================================================================================================///
// input:	char* s- > string										///
//output:	noting												///
//Purpose:	print string 			 								///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void print(char* s){
	int len;
	len=strlen(s);
	if(write(1,s,strlen(s))!=len){
		write(1,"write error\n",strlen("write error\n"));
		exit(1);
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sh_int													///
//==============================================================================================================///
// input:	signum -> signal number										///
//output:	noting												///
//Purpose:	SIGINT handler -> all client , child exit	buf fail..					///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sh_int(int signo){	
	char buf[BUF_SIZE]={0,};
	strcpy(buf,"QUIT\n");	
	print("\nexisting server...\n");

	while(wait(NULL)>0);//exit all child
	myrm("myproc");	    //remove all in myproc
	close(client_fd);   //exit client	
	exit(0); 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// myrm														///
//==============================================================================================================///
// input:	name -> path/direcotyr name									///
//output:	noting												///
//Purpose:	remove directory except file/directory in this directory					///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void myrm(char* name){
	DIR *dir;
	char buf[BUF_SIZE]={0,};
	char path[BUF_SIZE]={0,};
	struct dirent *entry;
	struct stat fileStat;
	dir=opendir(name);
	//move name
	if(chdir(name)==-1){
		exit(1);}
	while((entry=readdir(dir))!=NULL){
		lstat(entry->d_name,&fileStat);
		if(S_ISDIR(fileStat.st_mode)){ 	
			if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,"..")) 	
			continue;
			myrm(entry->d_name);
			continue;
		}
		//mydelete(entry->d_name);
		unlink(entry->d_name);
		continue;
	}
	//move .. because remove name directory
	if(chdir("..")==-1){print("error\n");exit(1);}
	rmdir(name);
	closedir(dir);
}

////////////////////////////////////////////////////////////////////////////
//myitoa								////
//======================================================================////
//Input: value -> integer						////
//	 *str -> to save string						////
//Output: str -> return string changed					////
//Purpose: integer -> character string change, only 10			////
////////////////////////////////////////////////////////////////////////////
char* myitoa(int value, char*str){
	char *s=str;
	char buf[BUF_SIZE];
	int i,j;
	do{
		*s++=value%10+'0';	//only 10 not 2, 8, 16..
		
	}while((value=value/10)!=0);
	*s=0;

	for(i=0;str[i]!=0;i++)
		buf[i]=str[i];
	buf[i]=0;
	
	for(j=0;str[j]!=0;j++){		//reverse buf and put it into str
		str[j]=buf[i-1];
		i--;
	}
	return str;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// printi													///
//==============================================================================================================///
// input:	int s- > integer										///
//output:	noting												///
//Purpose:	print integer 			 								///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void printi(int d){
	char buf[BUF_SIZE]={0,};
	myitoa(d,buf);
	print(buf);
}



