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
#include<ctype.h>

#define index_const 1000;

int first_empty(Client_indexes client_indexes[100])
{
	int i=1;
	while(i<100 && client_indexes[i].queue_index>0)
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

void send_to_all_clients(int index_memory,int semid, msgbuf_char sbuf)
{
	Client_indexes * client_indexes;
	decrease_semaphore(semid);
	if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
	{
		perror("shmat\n");
		increase_semaphore(semid);
		exit(1);
	}

	int i;
	for(i=1;i<first_empty(client_indexes);i++)
	{
		msgsnd(client_indexes[i].queue_index,&sbuf,sizeof(msgbuf_char),0);
	}
	shmdt(client_indexes);
	increase_semaphore(semid);
}

int load_chosen_player(const char * text)
{
	char * temp;
	int i=0;
	while(text[i+1]!='\0')
	{
		temp[i]=text[i+1];
		i++;
	}
	return atoi(temp);

}

//wysyla zapytanie o rozpoczecie gry
int send_request(int index_memory, int semid,int player_id, int enemy_id)
{
	Client_indexes * client_indexes;
	decrease_semaphore(semid);
	if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
	{
		perror("shmat\n");
		increase_semaphore(semid);
		exit(1);
	}
	int queue=client_indexes[enemy_id].queue_index;
	shmdt(client_indexes);
	increase_semaphore(semid);

	msgbuf sbuf;
	sbuf.mtype=send_request_type;
	sbuf.number=player_id;
	msgsnd(queue,&sbuf,sizeof(msgbuf),0);
	msgbuf rbuf;
	msgrcv(queue,&rbuf,sizeof(msgbuf),receive_request_type,0);
	if(rbuf.number)
	{
		decrease_semaphore(semid);
		if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
		{
			perror("shmat\n");
			increase_semaphore(semid);
			exit(1);
		}
		client_indexes[player_id].enemy_index=client_indexes[enemy_id].queue_index;
		client_indexes[enemy_id].enemy_index=client_indexes[player_id].queue_index;
		shmdt(client_indexes);
		increase_semaphore(semid);
		return 1;
	}
	else
		return 0;


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
	if((index_memory=shmget(IPC_PRIVATE,100 * sizeof(Client_indexes),0666 | IPC_CREAT))==-1)
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
					Client_indexes * client_indexes;
					decrease_semaphore(index_semaphore);
					if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
					{
						perror("shmat\n");
						exit(1);
					}
					msgbuf sbuf;
					sbuf.mtype=3;
					sbuf.number=first_empty(client_indexes);
					msgsnd(global,&sbuf,sizeof(msgbuf),0);
					printf("Przyjalem klienta %d\n", sbuf.number);
					int queue; //tworzenie kolejki do komunikacji z klientem
					if((queue=msgget(sbuf.number,0666 | IPC_CREAT))==-1)
					{
						perror("msgget\n");
						exit(1);
					}
					client_indexes[sbuf.number].queue_index=queue;
					shmdt(client_indexes);
					increase_semaphore(index_semaphore);


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
							if(msgrcv(queue,&chat_rbuf,sizeof(msgbuf_char),3,0)==-1)
							{
								perror("msgrcv\n");
								exit(1);
							}
							if(chat_rbuf.text[0]==':')
							{
								if(isdigit(chat_rbuf.text[1])) //:nr_gracza
								{
									int chosen_player=load_chosen_player(chat_rbuf.text);
									if(send_request(index_memory,index_semaphore,sbuf.number,chosen_player))
									{
										printf("ok\n");
									}

								}
								else if(chat_rbuf.text[1]=='g') // :g
								{

								}
							}
							else //zwykly czat
							{
								chat_sbuf=chat_rbuf;
								chat_sbuf.mtype=2;
								send_to_all_clients(index_memory,index_semaphore,chat_sbuf);
							}
						}

					}
					else   //wysyłanie 
					{

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
