///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File Name	: cli.c												///
// Data		: 2014/05/23											///
// Os		: Ubuntu 12.04 LTS 64bits									///
// Author	: Shin Ah Young											///
// Student ID	: 2012722023											///
// -------------------------------------------------------------------------------------------------------------///
// Title : System Programing Assignment #2 (ftp server)								///
// Description : Linux ls (-a, -al, -l), dir, pwd, cd, cd.., mkdir, delete, rmdir, rename			///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include <arpa/inet.h> //for proper byte order
#include <sys/types.h> 
#include <sys/socket.h> //for socket
#include <netinet/in.h> //for proper byte order
#include <sys/wait.h> //for wait
#include <signal.h> //for signal
#include <string.h> //for memset
void sh_sigint(int); //signal handler for SIGINT

#define BUF_SIZE 256
#define BUF_MAX_SIZE 8196

int sockfd;


int main(int argc, char **argv){
	char *token[BUF_SIZE],buf[BUF_SIZE]={0,}, buf1[BUF_SIZE]={0,},recv[BUF_MAX_SIZE]={0,},buff[BUF_SIZE]={0,};
	int sockfd;
	int n,i=0,x=0;
	struct sockaddr_in serv_addr;
	signal(SIGINT, sh_sigint);
	
	for(i=0;i<BUF_SIZE;i++)	//initialize
		buf[i]=0;
	  if(argc!=3) {
        	write(1,"usage: cli <IP> <PORT>\n", 23);
        	exit(1);
    		}
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
		write(1,"error\n",6);exit(1);
	}
   
   memset(&serv_addr, 0, sizeof(serv_addr)); //initialize structure
   
 
   serv_addr.sin_family=AF_INET;
   
   serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
   
   serv_addr.sin_port=htons(atoi(argv[2]));
   
   
   if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
      write(1, "connect() error!!\n", 18);
      exit(1);
   }	
	while(1){
	write(1,"> ",2);
	memset(&buff,0,BUF_SIZE);
	read(1,buff,sizeof(buff));
	token[0]=strtok(buff," \n");
	for(i=1;;i++){
		token[i]=strtok(NULL," \n");
		if(token[i]==NULL)
			break;
	}
	if(!strcmp(token[0],"ls")){	//ls
		if(i==1)
			strcpy(buf,"NLST\n");
		else if(i==2){	// ls -a , ls -l,  ls -al , ls '*', ls [directory/file](*)
			strcpy(buf,"NLST");
			if((!strcmp(token[1],"-a"))||(!strcmp(token[1],"-al"))||(!strcmp(token[1],"-l"))){
				strcat(buf," ");
				strcat(buf,argv[2]);
				strcat(buf,"\n");
			}
			else if(token[1][0]!='-'){	//if not instruction form  
				strcat(buf," ");
				strcat(buf,token[1]);
				strcat(buf,"\n");
			}
			else {
				write(1,"don't exist option\n",strlen("don't exist option\n"));
				exit(1);
			}
		}
		else if(i==3){	//ls [directory/file] * -a, -al, -l 
			strcpy(buf,"NLST");
			if((!strcmp(token[2],"-a"))||(!strcmp(token[2],"-al"))||(!strcmp(token[2],"-l"))){
				if(argv[1][0]!='-'){ // if not instruction form 
					strcat(buf," ");
					strcat(buf,token[1]);
					strcat(buf," ");
					strcat(buf,token[2]);
					strcat(buf,"\n");
				}
				else{
					write(1,"don't exist option\n",strlen("don't exist option\n"));
					exit(1);
				}					
			}
			else{
				write(1,"Not format\n",strlen("Not format\n"));
				exit(1);
			}
		}
		else{
			write(1,"Not format\n",strlen("Not format\n"));
			exit(1);
		}
	}//end of ls
	else if(!strcmp(token[0],"dir")){
		strcat(buf,"LIST");
		if(i!=2){	//if not dir [directory/path]
			write(1,"Not format\n",strlen("Not format\n"));
			exit(1);
		}
		else{
			if(token[1][0]!='-'){ //if not instruction form
				strcat(buf," ");
				strcat(buf,token[1]);
				strcat(buf,"\n");
			}
			else{
					write(1,"don't exist option\n",strlen("don't exist option\n"));
					exit(1);				
			}
		}
	}
	else if(!strcmp(token[0],"pwd")){
		
		if(i!=1){	//pwd
			write(1,"Not format\n",strlen("Not format\n"));
			exit(1);
		}
		strcpy(buf,"PWD\n");
	}
	else if(!strcmp(token[0],"cd")){
		if(i!=2){	//if not cd [directory/file]
			write(1,"Not format\n",strlen("Not format\n"));
			exit(1);
		}
		else{
			if(!strcmp(token[1],".."))	//cd ..
				strcpy(buf,"CDUP\n");
			
			else{
				strcpy(buf,"CWD");	//cd [directory/file]
				strcat(buf," ");
				strcat(buf,token[1]);
				strcat(buf,"\n");
			}				
		}
	}
	else if(!strcmp(token[0],"mkdir")){
		if(i==1){	//if not mkdir [directory] []....
			write(1,"Not format\n",strlen("Not format\n"));
			exit(1); 
		}
		else{
			strcpy(buf,"MKD");
			for(x=1;x<i;x++){
				strcat(buf," ");
				strcat(buf,token[x]);
			}
			strcat(buf,"\n");
			}
	}
	else if(!strcmp(token[0],"delete")){
		if(i==1){	//if not delete [file][...]........
			write(1,"Not format\n",strlen("Not format\n"));
			exit(1); 

		}
		else{
			strcpy(buf,"DELE");
			for(x=1;x<i;x++){
				strcat(buf," ");
				strcat(buf,token[x]);
			}
			strcat(buf,"\n");
		}
	}
	else if(!strcmp(token[1],"rmdir")){
		if(i==1){	//if not rmdir [directory][...]....
			write(1,"Not format\n",strlen("Not format\n"));
			exit(1); 

		}
		else{
			strcpy(buf,"RMD");
			for(x=1;x<i;x++){
				strcat(buf," ");
				strcat(buf,token[x]);
			}
			strcat(buf,"\n");
		}
	
	}
	else if(!strcmp(token[0],"rename")){	
		if(i!=3){	//if not rename [directory/file] [directory/file]
			write(1,"Not format\n",strlen("Not format\n"));
			exit(1); 
		}
		else{
			strcpy(buf,"RNFR & RNTO");
			strcat(buf," ");
			strcat(buf,token[1]);
			strcat(buf," ");
			strcat(buf,token[2]);
			strcat(buf,"\n");
		}
	}
	else if(!strcmp(token[0],"quit")){
		if(i!=1){
			write(1,"Not format\n",strlen("Not foramt\n"));
			exit(1);
		}
		strcpy(buf,"QUIT\n");
	}
	else{
		write(1,"Not instruction\n",strlen("Not instruction\n"));
		exit(1); 
	
	}
	n=strlen(buf);
	//write 
	
	
	if(write(sockfd,buf,n)!=n)
		write(1,"write error\n",strlen("write error\n"));
	if(read(sockfd,recv,BUF_MAX_SIZE)<0)
	write(1,"read error\n",11);
	i=strlen(recv);
	write(1,recv,i);
}
	close(sockfd);
	exit(0);
//end of while
}

//SIGINT handler
void sh_sigint(int signum){
   char quit_buf[BUF_SIZE];
   //send "QUIT" to server
   strcpy(quit_buf,"QUIT\n");
   write(sockfd, quit_buf, strlen(quit_buf));
   exit(0); //exit
}
