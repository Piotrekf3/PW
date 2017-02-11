#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>

struct msgbuf_char
{
	long mtype;
	int number;
	char text[255];
};

struct msgbuf
{
	long mtype;
	int number;
};


int main()
{
	struct msgbuf connect_server;
	struct msgbuf_char messages;
	int make_connection_ID, connect_to_chat_ID;
	size_t message_size = 100;

	make_connection_ID = msgget( 1000, 0666 );
	if( make_connection_ID == -1 )
	{
		printf("Server is broken, try again later\n");
		exit(1);
	}
	printf("%d connection1 \n",make_connection_ID);
	connect_server.mtype = 1;
	connect_server.number = 1;
	if( msgsnd( make_connection_ID, &connect_server, sizeof(struct msgbuf) - sizeof(long), 0 ) == -1 )
	{
		perror("Sending message failure\n");
		exit(1);
	}

	if( msgrcv( make_connection_ID, &connect_server, sizeof(struct msgbuf) - sizeof(long), 3, MSG_NOERROR ) == -1 )
	{
		perror("Recieving message failure\n");
		exit(1);
	}

	printf("Witamy na czacie, tutaj mozesz wybrac przeciwnika do gry!\n");

	connect_to_chat_ID = msgget( connect_server.number, 0666 );
	if( connect_to_chat_ID == -1 )
		perror("Connecting to chat failure\n");

		printf("%d connection2 i struct %d\n",connect_to_chat_ID,connect_server.number);

	if( fork() == 0 )
	{
		printf("%dmoj numer\n",connect_server.number);
		if( fork() == 0 )
		{
			printf("Odbieram3\n");
			if( msgrcv( connect_to_chat_ID, &messages, sizeof(struct msgbuf_char) - sizeof(long), 3, MSG_NOERROR ) == -1 )
				perror("Cannot recieve message from server type(3)\n");
			
			messages.mtype = 1;
			messages.number = connect_server.number;
			messages.text[0] = 'y';
			if( msgsnd( connect_to_chat_ID, &messages, sizeof(struct msgbuf_char) - sizeof(long), 0 ) == -1 )
				perror("Sending message to enemies, failure\n");
			printf( "%s", messages.text );	
			//fclose( stdin );
			//scanf("%c", &messages.text[0]);
			//if( msgsnd( connect_to_chat_ID, &messages, sizeof(struct msgbuf_char), 0 ) == -1 )
			//	perror("Sending response to server if player want to play\n");
		}
		else
		{
			while( 1 )
			{
				printf("Odbieram2\n");
				if( msgrcv( connect_to_chat_ID, &messages, sizeof(struct msgbuf_char) - sizeof(long), 2, MSG_NOERROR ) == -1 )
					perror("Cannot recieve message from other client\n");
				printf( "%s", messages.text );
			}
		}
	}	
	else
	{
		while( 1 )
		{
			messages.mtype = 1;
			messages.number = connect_server.number;
			printf("fgets\n");
			fgets(messages.text, message_size, stdin );
			printf("koniecfgets\n");
			if( msgsnd( connect_to_chat_ID, &messages, sizeof(struct msgbuf_char) - sizeof(long), 0 ) == -1 )
				perror("Sending message to enemies, failure\n");	
		}
	}
	return 0;
}
