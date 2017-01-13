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

#define index_const 1000;
int client_indexes[100];

int pierwszy_wolny()
{
	int i=0;
	while(i<100 && client_indexes[i]==1)
	i++;
	return i;
	
}
int main(int argc, char *argv[])
{
		int i;
	for(i=0;i<100;i++)
	client_indexes[i]=0;
	int global;
	if((global=msgget(global_key,0666 | IPC_CREAT))==-1)
	{
		perror("msgget\n");
		exit(1);
	}
	
	msgbuf rbuf;
	rbuf.mtype = 1;
	if(fork()==0)
	{
		while(1)
		{
		 	if(msgrcv(global,&rbuf,MSGSZ,1,0)>0)
			{
				if(fork()==0)
				{
					msgbuf sbuf;
					sbuf.mtype=1;
					sbuf.number=pierwszy_wolny();
					msgsnd(global,&sbuf,4,0);
					printf("Przyjalem klienta %d\n", sbuf.number);
					client_indexes[sbuf.number]=1;	
				}
			}	
		}
	}
	else
	{
		wait();
	}
	return 0;
}
