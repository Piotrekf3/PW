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

int first_empty(int client_indexes[100])
{
	int i=1;
	while(i<100 && client_indexes[i]==1)
		i++;
	return i;

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
	//kolejka globalna
	int global;
	if((global=msgget(global_key,0666 | IPC_CREAT))==-1)
	{
		perror("msgget\n");
		exit(1);
	}
	//pamiec wsp na indexy klientów
	int index_memory;
	if((index_memory=shmget(IPC_PRIVATE,100 * sizeof(int),0666 | IPC_CREAT))==-1)
	{
		perror("shmget\n");
		exit(1);
	}
	//semafor do index_memory
	int index_semaphore=semget(IPC_PRIVATE,1,0666 | IPC_CREAT);
	increase_semaphore(index_semaphore);

	msgbuf rbuf;
	rbuf.mtype = 1;
	if(fork()==0)
	{
		while(1)
		{
			if(msgrcv(global,&rbuf,sizeof(msgbuf),1,0)>0)
			{	
				if(fork()==0)
				{
					//przydzielanie pam współdzielonej
					int * client_indexes;
					decrease_semaphore(index_semaphore);
					if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
					{
						perror("shmat\n");
						exit(1);
					}
					msgbuf sbuf;
					sbuf.mtype=2;
					sbuf.number=first_empty(client_indexes);
					client_indexes[sbuf.number]=1;
					msgsnd(global,&sbuf,sizeof(msgbuf),0);
					printf("Przyjalem klienta %d\n", sbuf.number);
					shmdt(client_indexes);
					increase_semaphore(index_semaphore);

					int queue; //tworzenie kolejki do komunikacji z klientem
					if((queue=msgget(sbuf.number,0666 | IPC_CREAT))==-1)
					{
						perror("msgget\n");
						exit(1);
					}

					//bufory do czatu
					msgbuf_char chat_rbuf;
					chat_rbuf.mtype=3;
					msgbuf_char chat_sbuf;
					chat_sbuf.mtype=2;

					if(fork()==0)//odbieranie
					{
						while(1)
						{
							//odbiera od klienta
							msgrcv(queue,&chat_rbuf,sizeof(msgbuf_char),3,0);
						}

					}
					else 
					{
						while(1)
						{
						}

					}
					break;
				}
				else
				{
				}
			}	
		}
	}
	else
	{
		wait();
		shmctl(index_memory,IPC_RMID,NULL);
		semctl(index_semaphore,0,IPC_RMID,NULL);
		msgctl(global,IPC_RMID,0);
	}
	return 0;
}
