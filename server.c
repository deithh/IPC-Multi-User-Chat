#include <sys/types.h>   
#include <sys/ipc.h>      
#include <sys/msg.h>     
#include <unistd.h>     
#include <stdio.h>        
#include <stdlib.h>    
#include <fcntl.h>  
#include <string.h>    
#include "models.h"

typedef struct MSG handler(struct MSG msg);
int MSGSIZE = sizeof(union Data)+2*sizeof(int);
char topics[MAX_TOPICS][TOPIC_LEN];
int topic_count = 0;
struct User_info users[MAX_USERS];
int users_count = 0;
int Q_id;


int open_ipc(int qkey){
    int Q_id = msgget(qkey, 0666 | IPC_CREAT);

    if (Q_id == -1){
        printf("Q failed\n");
        exit(0);
    }
    return Q_id;
}

int compare_priority(const void *a, const void *b) {
    const struct MSG *msg_a = (const struct MSG *)a;
    const struct MSG *msg_b = (const struct MSG *)b;

    return -(msg_b->data.message.priority - msg_a->data.message.priority );
}

struct User_info create_user(char nick[], int id){
    struct User_info user;
    strcpy(user.nick, nick);
    user.id = id;
    user.subscribed_topics_count = 0;
    user.buffered = 0;
    return user;
}

struct User_info *  find_user_in_databse(int id){
    for (int i =0; i<users_count;i++){
        if(id == users[i].id){
            return &users[i];
        }
    }
    return NULL;
}

void store_subscribed_topic(int id, char topic[], int topic_span, int sync){
    struct User_info * user = (struct User_info*)find_user_in_databse(id);
    strcpy(user->topics[user->subscribed_topics_count], topic);
    user->topics_span[user->subscribed_topics_count] = topic_span;
    user->sync_topics[user->subscribed_topics_count] = sync;
    user->subscribed_topics_count +=1;
    return;
}

int nick_available(char nick[]){
    for(int i = 0; i<users_count; i++){
        if(strcmp(nick, users[i].nick)==0)
            return 0;
    }
    return 1;
}

void append_user_to_database(struct User_info new_user){
    users[users_count] = new_user;
    users_count++; 
}

int find_topic_in_database(char topic[]){
    for(int i = 0; i<topic_count; i++){
        if(strcmp(topic, topics[i])==0)
        return i;
    }
    return -1;
}

void send_sync_msgs(int id){
    struct User_info * user = find_user_in_databse(id);
    
    qsort(user->msg_buffer, user->buffered, sizeof(struct MSG), compare_priority);
    for(int i = user->buffered-1; i>=0; i--){
        if (fork()==0){
            msgsnd(Q_id, &user->msg_buffer[i], MSGSIZE, 0);
            exit(0);
        }
        user->buffered -= 1;
    }
}

void buffor_msg(struct MSG msg, int id, int topic_i){
    struct User_info * user = find_user_in_databse(id);
    struct User_info * sender = find_user_in_databse(msg.sender_id);
    
    if(user->buffered==MAX_BUFFERED){
        printf("Max buffered exceded msg lost\n");
        return;
    }
    if(user->topics_span[topic_i]>0){
        user->topics_span[topic_i] -= 1;
    }
    msg.mtype = user->id;
    strcpy(msg.data.message.sender_nick, sender->nick);

    user->msg_buffer[user->buffered] = msg;
    user->buffered++;
    return;
}

void push_msg(struct MSG msg, int id, int topic_i){
    struct User_info * user = find_user_in_databse(id);
    struct User_info * sender = find_user_in_databse(msg.sender_id);
    
    if(user->topics_span[topic_i]>0){
        user->topics_span[topic_i] -= 1;
    }
    msg.mtype = user->id;
    strcpy(msg.data.message.sender_nick, sender->nick);
    
    if(fork()==0){
        msgsnd(Q_id, &msg, MSGSIZE, 0);
        exit(0);
    }
    

}

void propagate_info(struct MSG msg){
    for(int i = 0; i<users_count;i++){
        msg.mtype = users[i].id;
        if(fork()==0){
            msgsnd(Q_id, &msg, MSGSIZE, 0);
            exit(0);
        }
    }
}

void propagate_msg(struct MSG msg){
    char topic[TOPIC_LEN];
    strcpy(topic, msg.data.message.topic);

    for(int i =0; i<users_count;i++){
        for(int topic_i = 0; topic_i < users[i].subscribed_topics_count; topic_i++){
            if(strcmp(topic, users[i].topics[topic_i])==0){
                if(users[i].topics_span[topic_i]==0){
                    break;
                }
                if(users[i].sync_topics[topic_i] == SYNC){
                    buffor_msg(msg, users[i].id, topic_i);
                }
                else if(users[i].sync_topics[topic_i] == ASYNC){
                    push_msg(msg, users[i].id, topic_i);

                }
                else break;

            }
        }
    }
}

struct MSG login_handler(struct MSG msg){
    struct Request_login request = msg.data.request_login;
    struct MSG post_msg;
    struct User_info user = create_user(request.sender_nick, msg.sender_id);

    post_msg.context = INFO;
    post_msg.mtype = user.id;

    if(users_count<MAX_USERS && nick_available(user.nick)){
        strcpy(post_msg.data.server_info.mtext, "[INFO] Server: Logged in\n");
        printf("Login passed, Hello %s\n", user.nick);
        append_user_to_database(user);
    }
    else{
        strcpy(post_msg.data.server_info.mtext, "[INFO] Server: Server can't handle more users or nick is already used\n");
        printf("Login attempt noticed, but server is fully occupated or nick is already used\n");
    }

    return post_msg;
}

struct MSG new_topic_handler(struct MSG msg){
    struct MSG post_msg;
    post_msg.context = INFO;
    post_msg.mtype = msg.sender_id;
    if(topic_count<MAX_TOPICS){
        struct Request_new_topic request = msg.data.request_new_topic;
        strcpy(topics[topic_count], request.topic);
        topic_count += 1;
        strcpy(post_msg.data.server_info.mtext, "[Info] Server: New topic created\n");
        struct MSG info_msg;
        info_msg.context = INFO;
        char info[128];
        strcpy(info, "");
        strcat(info, "[Info] Server: New topic ");
        strcat(info, request.topic);
        strcat(info, "\n");
        strcpy(info_msg.data.server_info.mtext, info);
        propagate_info(info_msg);
    }
    else{
        strcpy(post_msg.data.server_info.mtext, "[Info] Server: Declined, can't handle more topics\n");
    }
    

    return post_msg;
}

struct MSG list_topics_handler(struct MSG msg){
    struct MSG post_msg;
    post_msg.context = INFO;
    post_msg.mtype = msg.sender_id;
    strcpy(post_msg.data.server_info.mtext, "[info] Server: topics: \n");

    for(int i = 0; i<topic_count;i++){
        strcat(post_msg.data.server_info.mtext, topics[i]);
        strcat(post_msg.data.server_info.mtext, "\n");
    }
    return post_msg;
}

struct MSG subscribe_topic_handler(struct MSG msg){
    struct Request_subscribe_to_topic request = msg.data.request_subscribe_to_topic;

    struct MSG post_msg;
    post_msg.context = INFO;
    post_msg.mtype = msg.sender_id;

    if(find_topic_in_database(request.topic)>=0){

        store_subscribed_topic(msg.sender_id, request.topic, request.topic_span, request.sync);
        strcpy(post_msg.data.server_info.mtext, "[Info] Server: Subscribed to topic\n");
    }
    else{
        strcpy(post_msg.data.server_info.mtext, "[Info] Server: Topic doesn't exist\n");
    }

    return post_msg;
    
}

struct MSG get_msg_handler(struct MSG msg){
    struct MSG post_msg;
    post_msg.context = INFO;
    post_msg.mtype = msg.sender_id;
    strcpy(post_msg.data.server_info.mtext, "[Info] Server: No more messanges\n");
    send_sync_msgs(msg.sender_id);
    return post_msg;
    
}

struct MSG msg_handler(struct MSG msg){
    propagate_msg(msg);

    struct MSG post_msg;
    post_msg.context = INFO;
    post_msg.mtype = msg.sender_id;
    strcpy(post_msg.data.server_info.mtext, "[Info] Server: Message sent\n");
    return post_msg;    
}

handler * handlers[] = {login_handler, new_topic_handler, list_topics_handler, subscribe_topic_handler, get_msg_handler, msg_handler};

void handle_msg(struct MSG msg){

    if(msg.context != LOGIN){
        struct User_info * user = find_user_in_databse(msg.sender_id);
        if(user == NULL){
            printf("User not logged in\n");
            return;
        }
    }

    struct MSG post_msg = handlers[msg.context](msg);
    if(fork()==0){
        msgsnd(Q_id, &post_msg, MSGSIZE, 0);
        exit(0);
    }
}

int main(){
    Q_id =open_ipc(QKEY);
    printf("Server online! \n");

    while(1){
        struct MSG get_msg;


        int rcv = msgrcv(Q_id, &get_msg, MSGSIZE, SERVER_ID, 0);
        
        if(rcv == -1){
            printf("Error receiving message\n");
            exit(0);
        }

        handle_msg(get_msg);


    }

    return 0;
}
