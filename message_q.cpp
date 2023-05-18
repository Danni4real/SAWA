#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include <iostream>

#include "message_q.h"

#define PERMS 0644

using namespace std;

struct MessageBuf {
   long type;
   char content[MAX_MESSAGE_SIZE+1]; // +1, because will append a '\0' to the msg
};

int connect_msgq(uint key)
{
    int msgq_id = msgget((key_t)key, PERMS | IPC_CREAT);

    if(msgq_id == -1)
        perror("SAWA ERR: connect_msgq(): msgget()");

    return msgq_id;
}

bool mq_send(uint destination_id, uint message_type, const string& message_content)
{
    int  msgq_id  = -1;
    uint msg_size = message_content.size();

    if(msg_size > MAX_MESSAGE_SIZE)
    {
        cout << "SAWA ERR: mq_send(): message oversize: " << message_content.size()
             << " message size limit: " << MAX_MESSAGE_SIZE << endl;

        goto err;
    }

#ifdef Debug
    cout << time_now() << ": before connect_msgq()" << endl;
#endif

    msgq_id = connect_msgq(destination_id); // use destination_id as key to gen msgq_id

    if(msgq_id == -1)
        goto err;

    MessageBuf buf;
    buf.type  = message_type;

    strncpy(buf.content, message_content.data(), msg_size);
    buf.content[msg_size] = '\0'; // message should end with '\0'

#ifdef Debug
    cout << time_now() << ": before msgsnd()" << endl;
#endif

    if(msgsnd(msgq_id, &buf, msg_size+1, 0) == -1) // +1 for '\0'
    {
        perror("SAWA ERR: mq_send(): msgsnd()");
        goto err;
    }

#ifdef Debug
    cout << time_now() << ": mq_send: " << message_content << endl;
#endif

    return true;

err:
    cout << "SAWA ERR: mq_send(): send failed!" << endl;
    return false;
}

bool mq_recv(uint my_id, uint message_type, string* message_content)
{
    int msgq_id = connect_msgq(my_id); // use my_id as key to gen msgq_id

    if(msgq_id == -1)
        goto err;

    MessageBuf buf;

    if(msgrcv(msgq_id, &buf, sizeof(buf.content), message_type, 1) == -1) // recv first message with type of message_type
    {
        perror("SAWA ERR: mq_recv(): msgrcv()");
        goto err;
    }

    *message_content = buf.content;

#ifdef Debug
    cout << time_now() << ": mq_recv: " << *message_content << endl;
#endif

    return true;

err:
    cout << "SAWA ERR: mq_recv(): recv failed!" << endl;
    return false;
}

