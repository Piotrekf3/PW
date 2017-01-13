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
	msgsnd(global,&sbuf,MSGSZ,0);
	msgbuf rbuf;
	rbuf.mtype=1;
	msgrcv(global,&rbuf,4,1,0);
	printf("Jestem klientem nr: %d",rbuf.number);
	
	return 0;
}
