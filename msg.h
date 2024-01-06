#define MSG_LEN 256
#define NICK_LEN 32
#define MAX_USERS 20
#define QKEY 2137

#define SERVER_ID 1

#define LOWEST_MSG_TYPE 1
#define HIGHEST_MSG_TYPE 9
#define CONNECT 1
#define SHOW_THREADS 2
#define SUBSCRIBE 3
#define UNSUBSCRIBE 4
#define SEND_MESSAGE 5
#define SHOW_USERS 6
#define BLOCK_USER 7
#define UNBLOCK_USER 8
#define DISCONNECT 9

int open_ipc(int qkey);

struct msgbuf{
    long int mtype;
    char mtext[MSG_LEN];
    char sender[NICK_LEN];
    int sender_id;
    int context;
    int priority;
    int code; //server response 0 pass 1 failed
};

struct User{
    char nick[NICK_LEN];
    int id;
};

//mtype

//1 - to server 
//id - to specified user

//context = 
//1 - connect
//2 - show threads
//3 - subscribe to a thread
//4 - unsubscribe from a thread
//5 - send message to a thread
//6 - show users
//7 - block user
//8 - unblock user
//9 - disconnect

//priority  <1,3>
//1 - normal
//2 - high
//3 - very high
