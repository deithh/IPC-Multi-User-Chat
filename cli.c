#include <sys/types.h>   
#include <sys/ipc.h>      
#include <sys/msg.h>     
#include <unistd.h>     
#include <stdio.h>        
#include <stdlib.h>    
#include <fcntl.h>  
#include <string.h> 

#include "msg.h"



char blocked_users[MAX_USERS][NICK_LEN];
struct User user;
int logged_out_flag = -1;

int handle_login_input(int argc, char* argv[]){

    if(argc!=3){
        return -1;
    }

    if(strlen(argv[1]) > NICK_LEN){
        return -1;
    }

    strcpy(user.nick, argv[1]);
    user.id = atoi(argv[2]);

    return 0;

}

int login(int Q_id, struct User user){
        struct msgbuf msg;
        msg.mtype = 1;
        strcpy(msg.sender, user.nick);
        msg.sender_id = user.id;
        msgsnd(Q_id, &msg, sizeof(msg), 0);

        msgrcv(Q_id, &msg, sizeof(msg), user.id, 0);
        if(msg.code == 0){
            printf("Logged in\n");
            return 0;
        }
        else{
            printf("Login failed\n");
            return -1;
        }
}



int main(int argc, char* argv[]){

    if (handle_login_input(argc, argv)){
        printf("./client nick [0 - %d alphanumeric]\n", NICK_LEN);
        return -1;
    }


    //open IPC
    int Q_id = open_ipc(QKEY);

    //login
    if(fork()!=0){
        logged_out_flag = login(Q_id, user);
        exit(0);
    }

    if(logged_out_flag){
        exit(0);
    }


    return 0;
}
