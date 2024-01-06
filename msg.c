#include "msg.h"
#include <sys/types.h>   
#include <sys/ipc.h>      
#include <sys/msg.h>     
#include <stdlib.h>
#include <stdio.h>

int open_ipc(int qkey){
    int Q_id = msgget(qkey, 0666 | IPC_CREAT);

    if (Q_id == -1){
        printf("Q failed\n");
        exit(0);
    }
    return Q_id;
}
