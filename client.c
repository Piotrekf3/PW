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

int main(int argc, char *argv[])
{
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
		while(1)
		{
			scanf("%s",&chat_sbuf.text);
			msgsnd(queue,&chat_sbuf,sizeof(msgbuf_char),0);
		}
	}
	
	return 0;
}
