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
#include<errno.h>
#include"struct.h"

#define index_const 1000;

int pierwszy_wolny(int client_indexes[100])
{
	int i=0;
	while(i<100 && client_indexes[i]==1)
		i++;
	return i;

}
int main(int argc, char *argv[])
{
	int global;
	if((global=msgget(global_key,0666 | IPC_CREAT))==-1)
	{
		perror("msgget\n");
		exit(1);
	}
	int index_memory;
	if((index_memory=shmget(IPC_PRIVATE,100 * sizeof(int),0666 | IPC_CREAT))==-1)
	{
		perror("shmget\n");
		exit(1);
	}
	else
		printf("index_memory=%d\n",index_memory);
	msgbuf rbuf;
	rbuf.mtype = 1;
	if(fork()==0)
	{
		//przydzielanie pam współdzielonej
		int number;
		int * client_indexes;
		if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
		{
			perror("shmat\n");
			exit(1);
		}
		while(1)
		{
			if(msgrcv(global,&rbuf,sizeof(msgbuf),1,0)>0)
			{	
				number=pierwszy_wolny(client_indexes);
				client_indexes[number]=1;
				if(fork()==0)
				{
					msgbuf sbuf;
					sbuf.mtype=1;
					sbuf.number=number;
					msgsnd(global,&sbuf,sizeof(msgbuf),0);
					printf("Przyjalem klienta %d\n", sbuf.number);
					int queue; //tworzenie kolejki do komunikacji z klientem
					if((queue=msgget(sbuf.number,0666 | IPC_CREAT))==-1)
					{
						perror("msgget\n");
						exit(1);
					}
					break;	
				}
				else
				{
				}
			}	
		}
		shmdt(client_indexes);
	}
	else
	{
		wait();
		shmctl(index_memory,IPC_RMID,NULL);
	}
	return 0;
}
