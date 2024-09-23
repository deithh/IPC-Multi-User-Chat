#define MSG_LEN 1024
#define NICK_LEN 32
#define TOPIC_LEN 32
#define MAX_USERS 20
#define MAX_TOPICS 26
#define MAX_BUFFERED 20

#define QKEY 02137
#define SERVER_ID 1

#define LOGIN 0
#define NEWTOPIC 1
#define TOPIC_LIST 2
#define SUBSCRIBE_TOPIC 3
#define GETMSG 4
#define MESSAGE 5
#define INFO 6
#define BLOCK 7

#define SUBSCRIBE_UNLIMITED -1
#define SYNC 1
#define ASYNC 0

int open_ipc(int qkey);


struct Message{ 

    char mtext[MSG_LEN];
    int priority;
    char topic[TOPIC_LEN];
    char sender_nick[NICK_LEN];

};

struct Request_subscribe_to_topic{ 
    char topic[TOPIC_LEN];
    int topic_span;
    int sync;

};

struct Request_login{ 
    char sender_nick[NICK_LEN];
};

struct Request_topic{ 
    char topic[TOPIC_LEN];

};

struct Request_new_topic{
    char topic[TOPIC_LEN];


};

struct Server_info{
    char mtext[MSG_LEN];

};

struct Block{
    char nick[NICK_LEN];
};

struct MSG{
    long int mtype;
    int context;
    int sender_id;
    union Data{
    struct Message message;
    struct Request_login request_login;
    struct Request_subscribe_to_topic request_subscribe_to_topic;
    struct Request_topic request_topic;
    struct Request_new_topic request_new_topic;
    struct Server_info server_info;
    struct Block block;
    } data;
};

struct User{
    char nick[NICK_LEN];
    int id;
};

struct User_info{
    char nick[NICK_LEN];
    int id;
    char topics[MAX_TOPICS][TOPIC_LEN];
    int topics_span[MAX_TOPICS];
    int sync_topics[MAX_TOPICS];
    int subscribed_topics_count;
    struct MSG msg_buffer[MAX_BUFFERED];
    int buffered;
};

