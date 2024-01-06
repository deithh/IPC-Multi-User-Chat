#include <sys/types.h>   
#include <sys/ipc.h>      
#include <sys/msg.h>     
#include <unistd.h>     
#include <stdio.h>        
#include <stdlib.h>    
#include <fcntl.h>  
#include <string.h>    
#include "msg.h"

struct User users[MAX_USERS];
int users_count = 0;
int Q_id;

typedef int handler(struct msgbuf msg);

//zakladam ze jak ktos sie zaloguje to go nie usuwamy z users
void append_user_to_database(struct User new_user){
    users[users_count] = new_user;
    users_count++; 
}
//returns index of user in database
int find_user_in_databse(struct User user){
    for (int i =0; i<users_count;i++){
        if(!strcmp(user.nick, users[i].nick)){
            return i;
        }
    }
    return -1;
}

int login_handler(struct msgbuf msg){ //sprawdz czy istnieje w bazie jesli nie i mozna to utwórz potwierdz wiad zwracajac id
//zakladamy ze jesli klient zna swoje id to nigdy nie dojdzie do duplikacji id
    struct msgbuf msg_get;
    for (int i =0; i<users_count;i++){
        if(!strcmp(msg.sender, users[i].nick)){
            //zakladamy ze trafilismy na tego samego uzytkownika
            msg_get.code = 0;
            strcpy(msg_get.sender, "server");
            msg_get.sender_id = msg.sender_id;
            sprintf(msg_get.mtext, "%d", i);
            msgsnd(Q_id, &msg_get, sizeof(msg_get), 0);
            return 0;}
    }
    if(users_count<MAX_USERS){
        //user handling
        struct User new_user;
        strcpy(new_user.nick, msg.sender);
        new_user.id = msg.sender_id;
        append_user_to_database(new_user);
        //response handling
        struct msgbuf msg_response;
        msg_response.code = 0;
        strcpy(msg_response.sender, "server");
        msg_response.sender_id = SERVER_ID;
        msg_response.mtype = msg.sender_id;
        //sending response
        sprintf(msg_response.mtext, "%d", users_count-1);
        msgsnd(Q_id, &msg_response, sizeof(msg_response), 0);
        
        return 0;
    }
    else{
    //error response
    msg_get.code = 1;
    strcpy(msg_get.sender, "server");
    msg_get.mtype = msg.sender_id;

    msgsnd(Q_id, &msg_get, sizeof(msg_get), 0);
    return 0;
    }
}
//send all active threads
int show_threads_handler(struct msgbuf msg){

}

// void subscribe_thread_handler(struct msgbuf msg){

// }

// void unsubscribe_thread_handler(struct msgbuf msg){

// }






handler * handlers[] = {NULL, login_handler};

void handle_msg(struct msgbuf msg){
    if(handlers[msg.context](msg)){
        printf("failed handling request\n");
    }
}

int main(){
    int Q_id = open_ipc(QKEY);
    printf("Server online! \n");

    while(1){
        struct msgbuf msg;
        int rcv = msgrcv(Q_id, &msg, sizeof(msg), 0, 0);
        
        if(rcv == -1){
            printf("Error receiving message\n");
        }

        if(msg.mtype >= LOWEST_MSG_TYPE && msg.mtype <= HIGHEST_MSG_TYPE){
            handle_msg(msg);
        }

    }

    return 0;
}
