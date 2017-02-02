#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/wait.h>
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

void wyswietl(int ** tab)
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
//pierwszy wolny indeks dla klienta
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

//wysyla czat do wszystkich klientow
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
	for(i=1;i<100;i++)
	{
		if(client_indexes[i].queue_index!=0 && client_indexes[i].enemy_index==0)
			msgsnd(client_indexes[i].queue_index,&sbuf,sizeof(msgbuf_char),0);
	}
	shmdt(client_indexes);
	increase_semaphore(semid);
}

//zmienia :nr na nr
int load_chosen_player(const char * text)
{
	char * temp=malloc(sizeof(text)-1);
	int i=0;
	while(text[i+1]!='\0')
	{
		temp[i]=text[i+1];
		i++;
	}
	int result=atoi(temp);
	free(temp);
	return result;

}

//wysyla zapytanie o rozpoczecie gry i ja tworzy
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
	if(rbuf.number)//gracz sie zgodzil
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
		if(client_indexes[player_id].memory_index==0)
		{
			if((client_indexes[player_id].memory_index=shmget(IPC_PRIVATE, 6*7*sizeof(int), 0666 | IPC_CREAT))==-1)
			{
				perror("shmget\n");
				exit(1);
			}
			client_indexes[enemy_id].memory_index=client_indexes[player_id].memory_index;
			if((client_indexes[player_id].semaphore_index=semget(IPC_PRIVATE,1,0666 | IPC_CREAT))==-1)
			{
				perror("semget\n");
				exit(1);
			}
			client_indexes[enemy_id].semaphore_index=client_indexes[player_id].semaphore_index;
			increase_semaphore(client_indexes[player_id].semaphore_index);
			client_indexes[enemy_id].player_id=2;
			client_indexes[player_id].player_id=1;
		}
		shmdt(client_indexes);
		increase_semaphore(semid);

		sbuf.mtype=start_game_type;
		msgsnd(queue,&sbuf,sizeof(msgbuf),0);
		return 1;
	}
	else
		return 0;


}

//wysyla wszystkich dostepnych graczy
void send_player_names(int index_memory,int semid, int player_queue)
{
	int i;
	msgbuf sbuf;
	sbuf.mtype=show_players_type;
	Client_indexes * client_indexes;
	decrease_semaphore(semid);
	if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
	{
		perror("shmat\n");
		increase_semaphore(semid);
		exit(1);
	}
	for(i=0;i<100;i++)
	{
		if(client_indexes[i].queue_index!=0 && client_indexes[i].enemy_index==0)
		{
			sbuf.number=i;
			msgsnd(client_indexes[player_queue].queue_index,&sbuf,sizeof(msgbuf),0);
		}
	}
	shmdt(client_indexes);
	increase_semaphore(semid);
}

int get_memory_index(int memory_index,int semid,int player)
{
	Client_indexes * client_indexes;
	decrease_semaphore(semid);
	if((client_indexes=shmat(memory_index,NULL,0))==(void*)-1)
	{
		perror("shmat\n");
		increase_semaphore(semid);
		exit(1);
	}
	int result=client_indexes[player].memory_index;
	shmdt(client_indexes);
	increase_semaphore(semid);
	return result;
}

int get_semaphore_index(int memory_index,int semid,int player)
{
	Client_indexes * client_indexes;
	decrease_semaphore(semid);
	if((client_indexes=shmat(memory_index,NULL,0))==(void*)-1)
	{
		perror("shmat\n");
		increase_semaphore(semid);
		exit(1);
	}
	int result=client_indexes[player].semaphore_index;
	shmdt(client_indexes);
	increase_semaphore(semid);
	return result;

}

int get_player_id(int memory_index,int semid, int player)
{
	Client_indexes * client_indexes;
	decrease_semaphore(semid);
	if((client_indexes=shmat(memory_index,NULL,0))==(void*)-1)
	{
		perror("shmat\n");
		increase_semaphore(semid);
		exit(1);
	}
	int result=client_indexes[player].player_id;
	shmdt(client_indexes);
	increase_semaphore(semid);
	return result;
}
int * assign_data(int memory_index, int semid)
{
	int * data;
	decrease_semaphore(semid);
	if((data=shmat(memory_index,NULL,0))==(void *)-1)
	{
		perror("shmat\n");
		exit(1);
	}
	return data;
}

int ** assign_game_tab(int * data)
{
	int ** game_memory;
	game_memory=malloc(6 * sizeof(int *));
	int i;
	for(i=0;i<6;i++)
	{
		game_memory[i] = data+i*7;
	}
	return game_memory;
}

void dismiss_game_tab_memory(int memory_index,int semid, int *** game_memory,int ** data)
{
	free(*game_memory);
	*game_memory=NULL;
	shmdt(*data);
	increase_semaphore(semid);
}

//funkcje gry
int move_validation(int ** game_tab, int column)
{
	printf("move_validation1\n");
	if(column>=0 && column<7)
	{
		int i=5;
		while(i>0 && game_tab[i][column]!=0)
		{
			i--;
		}
		printf("move_validation2\n");
		printf("%d\n",i);
		printf("waruenk=%d\n",!game_tab[i][column]);
		return (!game_tab[i][column]);
	}
	else
		printf("move_validation3\n");
	return 0;
}

int make_move(int **game_tab, int column, int player)
{
	printf("make_move1\n");
	int i=5;
	while(i>=0 && game_tab[i][column]!=0)
	{
		i--;
	}
	game_tab[i][column]=player;
	printf("make_move2\n");
	return i;
}

//wysyla ruch do graczy
void send_move(int index_memory, int semid, int player_id, int line, int column,int player)
{
	msgbuf sbuf[6];
	Client_indexes * client_indexes;
	decrease_semaphore(semid);
	if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
	{
		perror("shmat\n");
		increase_semaphore(semid);
		exit(1);
	}
	//wysylanie wiersza
	sbuf[0].mtype=sbuf[1].mtype=move_line_server_type;
	sbuf[0].number=sbuf[1].number=line;
	printf("sbuf[0]=%d\n",sbuf[0].number);
	printf("player_index=%d\n enemy_index=%d\n",client_indexes[player_id].queue_index,client_indexes[player_id].enemy_index);
	msgsnd(client_indexes[player_id].queue_index,&sbuf[0],sizeof(msgbuf),0);
	msgsnd(client_indexes[player_id].enemy_index,&sbuf[1],sizeof(msgbuf),0);
	printf("Wyslano wiersz\n");

	//wysylanie kloumny
	sbuf[2].mtype=sbuf[3].mtype=move_column_server_type;
	sbuf[2].number=sbuf[3].number=column;
	msgsnd(client_indexes[player_id].queue_index,&sbuf[2],sizeof(msgbuf),0);
	msgsnd(client_indexes[player_id].enemy_index,&sbuf[3],sizeof(msgbuf),0);
	printf("wyslano kolumny\n");

	//wysyalnie gracza
	sbuf[4].mtype=sbuf[5].mtype=move_player_type;
	sbuf[4].number=sbuf[5].number=player;
	msgsnd(client_indexes[player_id].queue_index,&sbuf[4],sizeof(msgbuf),0);
	msgsnd(client_indexes[player_id].enemy_index,&sbuf[5],sizeof(msgbuf),0);

	shmdt(client_indexes);
	increase_semaphore(semid);
}

//sprawdza czy ktos wygral  
//dodac sprawdzanie na skos!!!!!!!!!!!!!!!!!!!!
int check_for_win(int ** game_tab)
{
	int i=0;
	int j=0;
	int a=0;
	int last=0;
	for(i=0;i<6;i++)
	{
		for(j=0;j<7;j++)
		{
			if(last!=0 && game_tab[i][j]==last)
				a++;
			else
				a=0;
			last=game_tab[i][j];
			if(a==3) return last;
		}
		last=0;
		a=0;
	}
	for(j=0;j<7;j++)
	{
		for(i=0;i<6;i++)
		{
			if(last!=0 && game_tab[i][j]==last)
				a++;
			else
				a=0;
			last=game_tab[i][j];
			if(a==3) return last;
		}
		last=0;
		a=0;
	}
	return 0;
}

//wysyla koniec gry do graczy i czysci pamiec
void end_game(int index_memory, int semid,int player_id,int result)
{
	msgbuf sbuf;
	sbuf.mtype=end_game_type;
	sbuf.number=result;
	Client_indexes * client_indexes;
	decrease_semaphore(semid);
	if((client_indexes=shmat(index_memory,NULL,0))==(void*)-1)
	{
		perror("shmat\n");
		increase_semaphore(semid);
		exit(1);
	}
	msgsnd(client_indexes[player_id].queue_index,&sbuf,sizeof(msgbuf),0);
	msgsnd(client_indexes[player_id].enemy_index,&sbuf,sizeof(msgbuf),0);
	sleep(3);
	//usuwanie pamieci
	msgctl(client_indexes[player_id].queue_index,IPC_RMID,0);
	msgctl(client_indexes[player_id].enemy_index,IPC_RMID,0);
	client_indexes[player_id].queue_index=0;
	client_indexes[player_id].enemy_index=0;
	shmctl(client_indexes[player_id].memory_index,IPC_RMID,0);
	semctl(client_indexes[player_id].semaphore_index,IPC_RMID,0);

	shmdt(client_indexes);
	increase_semaphore(semid);
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
									//wysyla do nadawcy
									if(fork()==0)
									{
										if(send_request(index_memory,index_semaphore,chosen_player,sbuf.number))
										{
											printf("ok2\n");
										}
										exit(1);
									}
									//wysyla do przeciwnika
									if(send_request(index_memory,index_semaphore,sbuf.number,chosen_player))
									{
										printf("ok\n");
									}

								}
								else if(chat_rbuf.text[1]=='g') // :g
								{
									send_player_names(index_memory,index_semaphore,sbuf.number);
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
					else   //rozpoczynanie gry
					{
						msgbuf game_rbuf;
						msgrcv(queue,&game_rbuf,sizeof(msgbuf),start_game_type,0);
						printf("Rozpoczecie gry dla %d\n",sbuf.number);
						int game_memory=get_memory_index(index_memory,index_semaphore,sbuf.number);
						int game_semaphore=get_semaphore_index(index_memory,index_semaphore,sbuf.number);
						int player_id=get_player_id(index_memory,index_semaphore,sbuf.number);
						printf("player_id=%d\n",player_id);
						int * data=NULL;
						int ** game_tab=NULL;
						while(1)
						{
							//opuszczenie semafora
							data = assign_data(game_memory,game_semaphore);
							game_tab = assign_game_tab(data);
							wyswietl(game_tab);

							//ruch tego gracza
							msgbuf game_sbuf;
							game_sbuf.mtype=move_s_type;
							msgbuf game_rbuf;
							game_rbuf.number=10;
							while(!move_validation(game_tab,game_rbuf.number))
							{
								msgsnd(queue,&game_sbuf,sizeof(msgbuf),0);
								msgrcv(queue,&game_rbuf,sizeof(msgbuf),move_r_type,0);
							}
							int line = make_move(game_tab,game_rbuf.number,player_id);//do zmiany nr gracza
							printf("ruch na pole %d wykonany\n",game_rbuf.number);

							printf("line=%d\n column=%d\n",line,game_rbuf.number);
							//wysylanie ruchu do graczy
							send_move(index_memory,index_semaphore,sbuf.number,line,game_rbuf.number,player_id);
							//koniec ruchu

							//sprawdzanie zwyciestwa
							int game_result = check_for_win(game_tab);
							dismiss_game_tab_memory(game_memory,game_semaphore,&game_tab,&data);
							//podniesienie semafora
							if(game_result!=0)
							{
								end_game(index_memory,index_semaphore,sbuf.number,game_result);
								exit(1);
							}
							sleep(1);
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
		wait(NULL);
		shmctl(index_memory,IPC_RMID,NULL);
		semctl(index_semaphore,0,IPC_RMID,NULL);
		msgctl(global,IPC_RMID,0);
	}
	return 0;
}
