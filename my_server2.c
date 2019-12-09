#include	"unp.h"
#define BUFF_SIZE 1024
#define TOT_ACCOUNT 4

int fdt[TOT_ACCOUNT]={0};
char account[TOT_ACCOUNT][10]={"one","two","three","four"};
int account_len[TOT_ACCOUNT]={3,3,5,4};
char password[TOT_ACCOUNT]={'1','2','3','4'};
int online[TOT_ACCOUNT]={0};

int game_condition(int game[3][3],char *mes){//if win , return 1
    int i,j;
    *mes=0;
    for (i = 0; i < 3; i++){
        strcat(mes,"|");
        for (j = 0; j < 3; j++){
            if (game[i][j]==1) strcat(mes,"O");
            else if (game[i][j]==2) strcat(mes,"X");
            else strcat(mes," ");
        }
        strcat(mes,"|\n\0");
    }
	strcat(mes,"-----\n\0");
    for (i = 0; i < 3; i++){
        if (game[i][0]>0 && game[i][0]==game[i][1] && game[i][0]==game[i][2]) return 1;
        if (game[0][i]>0 && game[0][i]==game[1][i] && game[0][i]==game[2][i]) return 1;
    }
    if (game[0][0]>0 && game[0][0]==game[1][1] && game[0][0]==game[2][2]) return 1;
    if (game[0][2]>0 && game[0][2]==game[1][1] && game[0][2]==game[2][0]) return 1;
    return 0;
}

void *pthread_service(void* sfd){
    int fd=*(int *)sfd;
    int recv_len,send_len,i;
    int account_num=-1;
    int room[2]={-1,-1};
    int agree[2]={0};
    int game_round=0;
    int game[3][3];
    int row,col;
    char mes[BUFF_SIZE],tmp[BUFF_SIZE];
    while((recv_len=recv(fd,mes,BUFF_SIZE,0))>0){
        if (!strncmp(mes,"!exit",5)) break;
        if (account_num==-1){   //not login yet
            send_len=send(fd,"Please enter your account:\n",28,0);
            recv(fd,mes,BUFF_SIZE,0);
            for (i = 0; i < TOT_ACCOUNT; i++){
                if (!strncmp(mes,account[i],account_len[i])){
                    account_num=i;
                    break;
                }
            }
            send_len=send(fd,"Please enter your password:\n",29,0);
            recv(fd,mes,BUFF_SIZE,0);
            if (strncmp(mes,password+i,1)){
                account_num=-1;
            }
            if (account_num==-1){
                send(fd,"Login fail. Please try again.\n",31,0);
            }else{
                send(fd,"Login successful.\n",19,0);
                fdt[i]=fd;
                online[i]=1;
            }
        }else{      // logged in  
            if (!strncmp(mes,"!logout",7)){
                online[account_num]=0;
                account_num=-1;
            }
            if (!strncmp(mes,"!play",5)){ //ask to play
                for (i = 0; i < TOT_ACCOUNT; i++){ // i is player you invited
                    if (!strncmp(mes+6,account[i],account_len[i])) break;
                }
                if (i<4){ //account_num is yourself
                    sprintf(mes,"!play %s ask you to play the game!\n",account[account_num]);
                    send(fdt[i],mes,strlen(mes)+1,0);
                }
                room[0]=account_num;
                room[1]=i;
                agree[0]=1;
                agree[1]=0;
                continue;
            }
            if (!strncmp(mes,"!list",5)){ //list online player
                *mes=0;
                for (i = 0; i < TOT_ACCOUNT; i++){
                    if (online[i]==1){
                        sprintf(tmp,"online:%s\n",account[i]);
                        strcat(mes,tmp);
                    }
                }
                send(fd,mes,strlen(mes)+1,0);
                continue;
            }
			if (!strncmp(mes,"!join",5)){ //reset and start the game 
                agree[1]=1;
                game_round=1;
                for (i = 0; i < 3; i++){
                    memset(game[i],0,3*sizeof(int));
                }
                send(fd,"!start\n",8,0);
                game_condition(game,mes);
                send(fdt[room[0]],mes,strlen(mes)+1,0);
                send(fdt[room[1]],mes,strlen(mes)+1,0);
                continue;
                //recv(fd,mes,BUFF_SIZE,0);
            }
            if (!strncmp(mes,"!transfer",9)){ //send to game master thread
                for (i = 0; i < TOT_ACCOUNT; i++){
                    if (!strncmp(mes+10,account[i],account_len[i])) break;
                }
                if (i<4){
                    sprintf(mes,"!trans %s\n",mes+11+account_len[i]);
                    send(fdt[i],mes,strlen(mes)+1,0);
                }
            }
			if (game_round>0 && agree[0]==1 && agree[1]==1){    //game
                if (game_round==1) printf("%s and %s start the game\n",account[room[0]],account[room[1]]);
                row=*(mes+8)-'0';
                col=*(mes+9)-'0';
               	if (!strncmp(mes,"!Player",7) && (game_round%2-1)){ //the one who join the game
                    if (game[row][col]==0){
                        game[row][col]=2;//player use 2 as X
                        game_round++;
                        if (game_condition(game,mes)){
                            strcat(mes,account[room[1]]);
                            strcat(mes," Win!\n");
                            game_round=0;
                        }else if (game_round>9){
                            strcat(mes,"Draw!\n");
                            game_round=0;
                        }
                        if (game_round==0){
                            sprintf(mes,"!endgame\n%s",mes);
                        }
                        send(fdt[room[0]],mes,strlen(mes)+1,0);
                        send(fdt[room[1]],mes,strlen(mes)+1,0);
                    }
                }
				else  if (!strncmp(mes,"!master",7) && game_round%2){ //the one who start the game
                    if (game[row][col]==0){
                        game[row][col]=1;//master use 1 as O
                        game_round++;
                        if (game_condition(game,mes)){
                            strcat(mes,account[room[0]]);
                            strcat(mes," Win!\n");
                            game_round=0;
                        }else if (game_round>9){
                            strcat(mes,"Draw!\n");
                            game_round=0;
                        }
                        if (game_round==0){
                            sprintf(tmp,"!endgame\n%s",mes);
                            strcpy(mes,tmp);
                        }
                        send(fdt[room[0]],mes,strlen(mes)+1,0);
                        send(fdt[room[1]],mes,strlen(mes)+1,0);
                    }
                }
            }
        }
    }
    close(fd);
    online[account_num]=0;
    printf("someone leave the server\n");
}



int main(){
	int	listenfd, connfd,i;
    int opt = SO_REUSEADDR;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
    pthread_t tid;

    bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0))<0) exit(3);
    printf("Socket %d\n",listenfd);

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(listenfd, (SA *) &servaddr, sizeof(servaddr))<0) exit(3);
    printf("bind success\n");

    if(listen(listenfd, LISTENQ)<0) exit(3);
    printf("Listen success\n");

	while(1){
		if ((connfd = accept(listenfd, (SA *) &cliaddr, &clilen))==-1){
            printf("accept error.\n");
            exit(1);
        }
        printf("someone connect to server\n");
        pthread_create(&tid,NULL,(void*)pthread_service,&connfd);
	}
    close(connfd);
    return 0;
}
