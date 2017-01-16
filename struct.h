#define MSGSZ 255
#define global_key 1000

typedef struct msgbuf
{
	long mtype;
	int number;
} msgbuf;

typedef struct msgbuf_char
{
	long mtype;
	int number;
	char text [MSGSZ];
} msgbuf_char;

typedef struct Client_indexes
{
	int queue_index;
	int enemy_index;
} Client_indexes;

