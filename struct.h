#define MSGSZ 255
#define global_key 101

typedef struct msgbuf
{
	long mtype;
	int number;
} msgbuf;

typedef struct msgbuf_char
{
	long mtype;
	char text [MSGSZ];
} msgbuf_char;

