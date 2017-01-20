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
#include"struct.h"

void wyswietl(int tab[6][7])
{
	int i, j;
	for(i=0;i<6;i++)
	{
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
	int enter_semaphore;
	if((enter_semaphore=semget(IPC_PRIVATE,1,0))==-1)
	{
		perror("semget\n");
		exit(1);
	}
	increase_semaphore(enter_semaphore);
	msgbuf sbuf;
	sbuf.mtype=1;
	int global;
	if((global=msgget(global_key,0666 | IPC_CREAT))==-1)
	{
		perror("msgget\n");
		exit(1);
	}
	msgsnd(global,&sbuf,sizeof(msgbuf),0);
	msgbuf rbuf;
	rbuf.mtype=3;
	msgrcv(global,&rbuf,sizeof(msgbuf),3,0);
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
	if(fork()==0) //odbieranie
	{
		while(1)
		{
			msgrcv(queue,&chat_rbuf,sizeof(msgbuf_char),2,0);
			printf("Gracz %d: %s\n",chat_rbuf.number,chat_rbuf.text);
		}
	}
	else //wysyłanie
	{
		if(fork()==0) //odbieranie zapytan o grę
		{
			msgbuf request_rbuf;
			while(1)
			{
				msgrcv(queue,&request_rbuf,sizeof(msgbuf),send_request_type,0);
				printf("Czy chcesz rozpoczac gre z graczem %d? (y/n)",request_rbuf.number);
				char c_answer;
				int answer;
				decrease_semaphore(enter_semaphore);
				scanf("%c",&c_answer);
				increase_semaphore(enter_semaphore);
				if(c_answer='y')
					answer=1;
				else
					answer=0;

				request_rbuf.number=answer;
				request_rbuf.mtype=receive_request_type;
				msgsnd(queue,&request_rbuf,sizeof(msgbuf),0);
				if(answer==1)
				{
					printf("Tak\n");
				}
			}
		}
		else
		{
			while(1)
			{
				decrease_semaphore(enter_semaphore);
				scanf("%s",&chat_sbuf.text);
				increase_semaphore(enter_semaphore);
				msgsnd(queue,&chat_sbuf,sizeof(msgbuf_char),0);
			}
		}
	}

	return 0;
}
