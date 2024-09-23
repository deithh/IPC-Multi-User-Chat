#include <sys/types.h>   
#include <sys/ipc.h>      
#include <sys/msg.h>     
#include <unistd.h>     
#include <stdio.h>        
#include <stdlib.h>    
#include <fcntl.h>  
#include <string.h> 
#include <signal.h>

#include "models.h"

char blocked_users[MAX_USERS][NICK_LEN];
struct User user;
int logged_out_flag = -1;
int MSGSIZE = sizeof(union Data)+2*sizeof(int);
int Q_id;
int fork_pid = -1;

int open_ipc(int qkey){
    int Q_id = msgget(qkey, 0666 | IPC_CREAT);

    if (Q_id == -1){
        printf("Q failed\n");
        exit(0);
    }
    return Q_id;
}
int find_user_in_blocked(char* user_nick){
    for(int i = 0; i < MAX_USERS; i++){
        if(strcmp(blocked_users[i], user_nick)==0){
            return 1;
        }
    }
    return 0;
}
void add_user_to_blocked(char* user_nick){
    for(int i = 0; i < MAX_USERS; i++){
        if(strcmp(blocked_users[i], "")==0){
            strcpy(blocked_users[i], user_nick);
            return;
        }
    }
}
void remove_user_from_blocked(char* user_nick){
    for(int i = 0; i < MAX_USERS; i++){
        if(strcmp(blocked_users[i], user_nick)==0){
            strcpy(blocked_users[i], "");
            return;
        }
    }
}

void display_help(){
    printf("Welcome to PSiW chat, %s\n", user.nick);
    printf("Input is limited to 300 characters\n");
    printf("Commands:\n");
    
    printf("/help - display this message\n");

    printf("/requestPosts - request all synchronously subscribed posts\n");
    printf("/createThread - create new thread\n");
    printf("/listThreads - list all threads\n");
    printf("/subThread - subscribe to thread\n");
    printf("/post - post message to thread\n");

    printf("/blockUser - block user\n");

    printf("/exit - exit chat\n");
}
void handle_request_posts(){
    struct MSG post_msg;
    post_msg.context = GETMSG;
    post_msg.mtype = SERVER_ID;
    post_msg.sender_id = user.id;
    msgsnd(Q_id, &post_msg, MSGSIZE, 0);
}

void handle_list_threads(){
    struct MSG post_msg;
    post_msg.context = TOPIC_LIST;
    post_msg.mtype = SERVER_ID;
    post_msg.sender_id = user.id;
    msgsnd(Q_id, &post_msg, MSGSIZE, 0);

}

void handle_sub_thread(char* thread_name, int thread_span, int sync){
    struct MSG post_msg;
    post_msg.context = SUBSCRIBE_TOPIC;
    post_msg.mtype = SERVER_ID;
    post_msg.data.request_subscribe_to_topic.sync = sync;
    strcpy(post_msg.data.request_subscribe_to_topic.topic, thread_name);
    post_msg.data.request_subscribe_to_topic.topic_span = thread_span;

    post_msg.sender_id = user.id;
    msgsnd(Q_id, &post_msg, MSGSIZE, 0);    
}


void handle_create_new_thread(char* thread_name){
    struct MSG post_msg;
    post_msg.context = NEWTOPIC;
    post_msg.mtype = SERVER_ID;
    strcpy(post_msg.data.request_new_topic.topic, thread_name);

    post_msg.sender_id = user.id;
    msgsnd(Q_id, &post_msg, MSGSIZE, 0);
}

void handle_post(char* thread_name, char* message, int priority){
    struct MSG post_msg;
    post_msg.context = MESSAGE;
    post_msg.mtype = SERVER_ID;
    strcpy(post_msg.data.message.topic, thread_name);
    strcpy(post_msg.data.message.mtext, message);
    post_msg.data.message.priority = priority;

    post_msg.sender_id = user.id;
    msgsnd(Q_id, &post_msg, MSGSIZE, 0);
}


int handle_exit(){
    kill(fork_pid, SIGKILL);
    exit(0);
}
void handle_request_posts_command(){
    handle_request_posts();
}
void handle_create_new_thread_command(){
    char thread_name[TOPIC_LEN];
    printf("Enter thread name (up to %d characters): ", TOPIC_LEN - 1);

    if(scanf("%50s", thread_name) == 1){
        handle_create_new_thread(thread_name);
    }

}
void handle_list_threads_command(){
    handle_list_threads();
}
void handle_sub_thread_command(){
    char thread_name[TOPIC_LEN];
    int thread_span;
    int sync;
    printf("Enter thread name (up to %d characters): ", TOPIC_LEN - 1);
    if (scanf("%50s", thread_name) == 1) {
        printf("Enter thread span <-1;20> where -1 means indefinitely: ");
        if (scanf("%d", &thread_span) == 1 && thread_span >= -1 && thread_span <= 20) {
            printf("Enter %d for on request delivery or %d for async: ", SYNC, ASYNC);
            scanf("%d", &sync);
            if(sync == SYNC || sync == ASYNC)
                handle_sub_thread(thread_name, thread_span, sync);
        }
        else{
            printf("Invalid thread span");
        }

    }
    else{
        printf("Invalid thread name");}
}


void handle_post_command(){
    char thread_name[TOPIC_LEN];
    char message[MSG_LEN];
    int priority;

    printf("Enter thread name (up to %d characters): ", TOPIC_LEN - 1);

    if(scanf("%50s", thread_name) == 1){
        printf("Enter message (up to %d characters): ", MSG_LEN - 1);
        getchar();
        if(fgets(message, MSG_LEN, stdin) != NULL ){
            printf("Enter priority (0-9): ");
            if(scanf("%d", &priority) == 1 && priority >= 0 && priority <= 9){
                handle_post(thread_name, message, priority);
            }
            else{
                printf("Invalid priority ");
            }
        
    }else{
            printf("Invalid message ");
        }
    }else{
        printf("Invalid thread name ");
    }
}

void handle_block_user(char* user_nick){
    if(find_user_in_blocked(user_nick)){
        printf("User already blocked ");
        return;
    }
    add_user_to_blocked(user_nick);
}
void handle_block_user_command(){
    char user_nick[NICK_LEN];

    printf("Enter user nick(0-%d alphanumeric): ", NICK_LEN - 1);

    if (scanf("%s", user_nick) != 1)
    {
        printf("Invalid user nick ");
        return;
    }
    struct MSG post_msg;
    post_msg.context =BLOCK;
    post_msg.mtype = user.id;
    strcpy(post_msg.data.block.nick, user_nick);
    msgsnd(Q_id, &post_msg, MSGSIZE, 0);
}

void handle_exit_command(){
    handle_exit();
}

void handle_user_input(){
    char input[20];
    

    scanf("%s", input);
    if(strcmp(input, "/help")==0){
        display_help();
    }
    else if(strcmp(input, "/requestPosts") == 0){
        handle_request_posts_command();
    }
    else if (strcmp(input, "/createThread") == 0){
        handle_create_new_thread_command();
    }
    else if(strcmp(input, "/listThreads")==0){
        handle_list_threads_command();
    }
    else if(strcmp(input, "/subThread")==0){
        handle_sub_thread_command();
    }
    else if(strcmp(input, "/post")==0){
        handle_post_command();
    }
    else if(strcmp(input, "/blockUser")==0){
        handle_block_user_command();
    }
    else if(strcmp(input, "/exit")==0){
        handle_exit_command();
    }
    else{
        printf("Unknown command");
    }
}


int handle_login_input(int argc, char* argv[]){

    if(argc!=2){
        return -1;
    }

    if(strlen(argv[1]) > NICK_LEN){
        return -1;
    }

    strcpy(user.nick, argv[1]);
    user.id = getpid();

    return 0;

}

void login(int Q_id, struct User user){

        struct MSG post_msg;
        post_msg.context = LOGIN;
        post_msg.mtype = SERVER_ID;
        strcpy(post_msg.data.request_login.sender_nick, user.nick);
        post_msg.sender_id = user.id;
        msgsnd(Q_id, &post_msg, MSGSIZE, 0);


        struct MSG get_msg;
        printf("Waiting for server..\n");
        int rcv = msgrcv(Q_id, &get_msg, MSGSIZE, user.id, 0);

        if(rcv>0 && get_msg.context == INFO){
            printf("%s", get_msg.data.server_info.mtext);
        }
        else{
            printf("communication failed!\n");
        };
        

}

void handle_info(struct MSG msg){
    struct Server_info info = msg.data.server_info;
    printf("%s\n", info.mtext);
}

void handle_message(struct MSG msg){

    if(find_user_in_blocked(msg.data.message.sender_nick)){
        printf("[Info] Message from blocked user\n");
        return;
    }

    printf("[%s:p%d] %s: %s\n", msg.data.message.topic, msg.data.message.priority, msg.data.message.sender_nick, msg.data.message.mtext);
}

void handle_msg(struct MSG msg){
    if(msg.context==INFO){
        handle_info(msg);
    }
    else if(msg.context==MESSAGE){
        handle_message(msg);
    }
    else if(msg.context == BLOCK){
        handle_block_user(msg.data.block.nick);
    }
    else{
        printf("[WARN] recived invalid msg\n");
    }
}

int main(int argc, char* argv[]){

    if (handle_login_input(argc, argv)){
        printf("./client nick [0 - %d alphanumeric]\n", NICK_LEN);
        return -1;
    }
    //open IPC
    Q_id = open_ipc(QKEY);

    login(Q_id, user);
    printf("help - /help\n");
    if((fork_pid = fork()) != 0){
        while(1){

            struct MSG get_msg;
            int rcv = msgrcv(Q_id, &get_msg, MSGSIZE, user.id, 0);
            
            if(rcv == -1){
                printf("[Warn] Comunication error!\n");
            }

            handle_msg(get_msg);

        }
    }
    else{
        while(1){
            handle_user_input();
        }
    }

    return 0;
}
