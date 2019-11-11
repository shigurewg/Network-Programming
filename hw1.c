/*
** server.c – 展示一個stream socket server
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#define PORT "3490" // 提供給使用者連線的 port
#define BACKLOG 10 // 有多少個特定的連線佇列（pending connections queue）
#define BUFSIZE 8096

struct {
    char *ext;
    char *filetype;
} extensions [] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"exe","text/plain" },
    {0,0} };


void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// 取得sockaddr，IPv4或IPv6：
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void handle_socket(int socket_fd){
    int j, file_fd, buflen, len,m,l;
    long i, ret;
    char * fstr;
	char *tmp;
	char *tmpp;
	char boundry[200];
	char end[200];
	char name[200];
    static char buffer[BUFSIZE+1];
//	if (send(socket_fd, "Hello, world!", 13, 0) == -1)
//		perror("send");
	while(1){
		m=recv(socket_fd,buffer,BUFSIZE,0);
		printf("%s\n",buffer);
		if (m==0||m==-1){
			if(m==0)
				exit(0);
			perror("recv");
			exit(3);
		}
		printf("%s\n",buffer);
			/*GET ONLY*/
		if(!strncmp(buffer,"GET ",4)||!strncmp(buffer,"get ",4)){
			for(i=4;i<BUFSIZE;i++) {
        		if(buffer[i] == ' ') {
            		buffer[i] = 0;
            	break;
        		}
    		}			
			if(!strncmp(buffer,"GET /\0",6)||!strncmp(buffer,"get /\0",6))
				strcpy(buffer,"GET /index.html\0");

			/* 檢查客戶端所要求的檔案格式 */
			buflen = strlen(buffer);
			fstr = (char *)0;
    		for(i=0;extensions[i].ext!=0;i++) {
        		len = strlen(extensions[i].ext);
       			if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
            		fstr = extensions[i].filetype;
            	break;
        		}
    		}
  			/* 檔案格式不支援 */
    		if(fstr == 0) {
        		fstr = extensions[i-1].filetype;
    		}
	
			// 開啟檔案 
    		if((file_fd=open(&buffer[5],O_RDONLY))==-1)
  				write(socket_fd, "Failed to open file", 19);
    		// 傳回瀏覽器成功碼 200 和內容的格式 
				sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
    		write(socket_fd,buffer,strlen(buffer));
    		// 讀取檔案內容輸出到客戶端瀏覽器 */
    		while ((ret=read(file_fd, buffer, BUFSIZE))>0) {
      		write(socket_fd,buffer,ret);
    		}
			write(socket_fd,"\r\n\r\n",4);
		}	/* GET END*/
		else if(!strncmp(buffer,"POST ",5)||!strncmp(buffer,"post ",5)){
 //   	/*	
			tmp=strstr(buffer,"boundary=");
			tmp+=9;
			tmpp=strstr(tmp,"\r\n");
			strncpy(end,tmp,tmpp-tmp);
			*(end+(tmpp-tmp)+1)=0;
			sprintf(boundry,"--%s",end);
			sprintf(end,"%s--",boundry);
			printf("%s\n%s\n",boundry,end);
			if((tmp=strstr(buffer,"filename="))==NULL){
				printf("NULL\n");
				recv(socket_fd,buffer,BUFSIZE,0);
			}
			if((tmp=strstr(buffer,"filename="))==NULL)
				printf("NULL AGAIN\n");
			tmp+=10;
			if(!strncmp(tmp,"x.c",3))
				printf("Good\n");
			for(l=0;*tmp!='\"';l++,tmp++)
				name[l]=*tmp;
			name[l]=0;
			if((file_fd=open(name,O_WRONLY|O_CREAT,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH))==-1)
				printf("Failed to open\n");
			tmp=strstr(buffer,boundry);
			tmpp=strstr(tmp,"\r\n\r\n")+4;
			while((tmp=strstr(tmpp,end))==NULL){
				l=&buffer[BUFSIZE-1]-tmpp+1;
				printf("%d\n",l);
				write(file_fd,tmpp,l);
				write(1,tmpp,l);
				recv(socket_fd,buffer,BUFSIZE,0);
				tmpp=buffer;
			}
			l=tmp-tmpp-2;
			printf("\n\n%s\n\n",buffer);
			write(file_fd,buffer,l);
			write(1,tmpp,l);
			close(file_fd);
		//	sprintf(buffer,"HTTP/1.0 200 OK\r\n");
		//	write(socket_fd,buffer,strlen(buffer));
//		*/
		}	/*POST END*/
		recv(socket_fd,buffer,BUFSIZE,0);
	}
exit(0) ;
}
int main(void)
{
  int sockfd, new_fd; // 在 sock_fd 進行 listen，new_fd 是新的連線
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // 連線者的位址資訊 
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // 使用我的 IP

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // 以迴圈找出全部的結果，並綁定（bind）到第一個能用的結果
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
      sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("server: bind");
      close(sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }

  freeaddrinfo(servinfo); // 全部都用這個 structure

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // 收拾全部死掉的 processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  while(1) { // 主要的 accept() 迴圈
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family,
      get_in_addr((struct sockaddr *)&their_addr),
      s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (!fork()) { // 這個是 child process
      close(sockfd); // child 不需要 listener

  //    if (send(new_fd, "Hello, world!", 13, 0) == -1)
    //    perror("send");

      handle_socket(new_fd);
      close(new_fd);

      exit(0);
    }

    close(new_fd); // parent 不需要這個
  }

  return 0;
}
