#define MSGSZ 255
#define global_key 1000
//game request types
#define send_request_type 4
#define receive_request_type 5
#define show_players_type 6

typedef struct msgbuf
{
	long mtype;
	int number;
} msgbuf;

typedef struct msgbuf_char
{
	long mtype;
	int number;
	char text[MSGSZ];
} msgbuf_char;

typedef struct Client_indexes
{
	int queue_index;
	int enemy_index;
} Client_indexes;

