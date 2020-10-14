#define CLIENT_TYPE 1

typedef struct
{
	long mtype;
        struct
        {
        	char c;
                pid_t pid;
        }info;
}MSG;
