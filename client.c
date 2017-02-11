#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/msg.h>
#include<signal.h>
#include<wait.h>
#include"struct.h"

void wyswietl(int tab[6][7])
{
	int i, j;
	printf("   ");
	for(i=0;i<7;i++)
		printf("%c ",'A'+i);
	printf("\n");
	for(i=0;i<6;i++)
	{
		printf("%d  ",i);
		for(j=0;j<7;j++)
		{
			if(tab[i][j]==1)
				printf("* ");
			else if(tab[i][j]==2)
				printf("# ");
			else
				printf("- ");
		}
		printf("\n");
	}
}

void zeruj(int tab[6][7])
{
	int i,j;
	for(i=0;i<6;i++)
		for(j=0;j<7;j++)
			tab[i][j]=0;
}

void increase_semaphore(int semid)
{
	struct sembuf temp;
	temp.sem_num=0;
	temp.sem_op=1;
	temp.sem_flg=0;
	semop(semid,&temp,1);
}

void decrease_semaphore(int semid)
{
	struct sembuf temp;
	temp.sem_num=0;
	temp.sem_op=-1;
	temp.sem_flg=0;
	semop(semid,&temp,1);
}

int main(int argc, char *argv[])
{
	int pids[10];
	int enter_semaphore;
	if((enter_semaphore=semget(IPC_PRIVATE,1,0666 | IPC_CREAT))==-1)
	{
		perror("semget\n");
		exit(1);
	}
	increase_semaphore(enter_semaphore);
	connect sbuf;
	sbuf.mtype=1;
	int global;
	if((global=msgget(global_key,0666 | IPC_CREAT))==-1)
	{
		perror("msgget\n");
		exit(1);
	}
	if(msgsnd(global,&sbuf,sizeof(connect)-sizeof(long),0)==-1)
	{
		perror("msgsnd\n");
		exit(1);
	}
	msgbuf rbuf;
	if(msgrcv(global,&rbuf,sizeof(msgbuf)-sizeof(long),3,0)==-1)
	{
		perror("msgrcv\n");
		exit(1);
	}
	printf("Jestem klientem nr: %d",rbuf.number);

	int queue;
	if((queue=msgget(rbuf.number,0666 | IPC_CREAT))==-1)
	{
		perror("msgget\n");
		exit(1);
	}
	printf("Podlaczono do czat\n");
	//bufory do czatu
	msgbuf_char chat_rbuf;
	chat_rbuf.mtype=2;
	msgbuf_char chat_sbuf;
	chat_sbuf.mtype=3;
	chat_sbuf.number=rbuf.number;
	pids[3]=fork();
	if(pids[3]==0)
	{
		while(1) //wysylanie czatu
		{
			//decrease_semaphore(enter_semaphore);
			fgets(chat_sbuf.text,255,stdin);
			//increase_semaphore(enter_semaphore);
			msgsnd(queue,&chat_sbuf,sizeof(msgbuf_char)-sizeof(long),0);
		}
	}
	else
	{

		pids[0]=fork();
		if(pids[0]==0) //odbieranie
		{
			while(1)
			{
				if(msgrcv(queue,&chat_rbuf,sizeof(msgbuf_char)-sizeof(long),2,0)==-1)
				{
					exit(1);
				}
				printf("Gracz %d: %s",chat_rbuf.number,chat_rbuf.text);
			}
		}
		else 
		{
			pids[1]=fork();
			if(pids[1]==0) //odbieranie zapytan o grę
			{
				msgbuf request_rbuf;
				if(msgrcv(queue,&request_rbuf,sizeof(request_rbuf)-sizeof(long),start_game_type,0)==-1) //zaczyna gre
				{
					perror("msgrcv\n");
					exit(1);
				}
				kill(pids[3],SIGKILL);
				kill(pids[0],SIGKILL);
				//printf("\n");
				int tab[6][7];
				zeruj(tab);
				wyswietl(tab);
				msgbuf game_rbuf;
				msgbuf game_sbuf;
				game_sbuf.mtype=move_r_type;


				msgbuf move_rbuf1;
				msgbuf move_rbuf2;
				msgbuf move_player;
				if(fork()==0)
				{
					while(1)
					{
						//przyjmowanie ruchu z serwera
						if(msgrcv(queue,&move_rbuf1,sizeof(msgbuf)-sizeof(long),move_line_server_type,0)==-1)
						{
							exit(1);
						}
						if(msgrcv(queue,&move_rbuf2,sizeof(msgbuf)-sizeof(long),move_column_server_type,0)==-1)
						{
							exit(1);
						}
						if(msgrcv(queue,&move_player,sizeof(msgbuf)-sizeof(long),move_player_type,0)==-1)
						{
							exit(1);
						}
						printf("move_player=%d\n",move_player.number);
						printf("line=%d\n column=%d\n",move_rbuf1.number,move_rbuf2.number);
						tab[move_rbuf1.number][move_rbuf2.number]=move_player.number;
						wyswietl(tab);
						sleep(1);
					}
				}
				else
				{
					while(1)
					{
						if(msgrcv(queue,&game_rbuf,sizeof(msgbuf)-sizeof(long),move_s_type,0)==-1)
						{
							exit(1);
						}

						printf("Twój ruch\n");
						char move;
						scanf("%c",&move);
						char c;
						while((c=fgetc(stdin))!='\n')
						{
						//	printf("%c\n",c);
						}
						game_sbuf.number=(int)move-'A';
						msgsnd(queue,&game_sbuf,sizeof(msgbuf)-sizeof(long),0);
					}
				}
			}
			else
			{
				pids[2]=fork();
				if(pids[2]==0)
				{
					msgbuf players_rbuf;
					while(1) //odbieranie listy graczy
					{
						if(msgrcv(queue,&players_rbuf,sizeof(msgbuf)-sizeof(long),show_players_type,0)==-1)
						{
							exit(1);
						}
						else
							printf("Gracz %d - wolny\n",players_rbuf.number);
					}
				}
				else
				{
					msgbuf rbuf;
					if(msgrcv(queue,&rbuf,sizeof(msgbuf)-sizeof(long),end_game_type,0)==-1)
					{
						perror("msgrcv\n");
						exit(1);
					}
					sleep(1);
					printf("Wygral gracz %d\n",rbuf.number);
					int i;
					for(i=0;i<4;i++)
						kill(pids[i],SIGKILL);
					return 0;

				}
			}
		}
	}
	return 0;
}
