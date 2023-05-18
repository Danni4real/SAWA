#include <thread>
#include <string>
#include <unistd.h>
#include <iostream>

#include "sawa.h"

using namespace std;

enum User {Invalid_User, User_1, User_2};

#define Compile_User_1

string process_request(unsigned int sender_id, const string& message)
{
    return message;
}

void thread_loop_send(int id)
{
    while(true)
    {
        string ack = sawa_send(User_2, to_string(id), 1);
        //cout << "ack: " << ack << endl;

        if(ack != to_string(id))
            exit(0);
    }
}

int main()
{
#ifdef Compile_User_1
    sawa_init(User_1);

    thread t1 = thread(thread_loop_send,1);
    thread t2 = thread(thread_loop_send,2);
    thread t3 = thread(thread_loop_send,3);
    thread t4 = thread(thread_loop_send,4);
    thread t5 = thread(thread_loop_send,5);
    thread t6 = thread(thread_loop_send,6);
    thread t7 = thread(thread_loop_send,7);
    thread t8 = thread(thread_loop_send,8);
    thread t9 = thread(thread_loop_send,9);
    thread t10 = thread(thread_loop_send,10);
    thread t11 = thread(thread_loop_send,11);
    thread t12 = thread(thread_loop_send,12);
    thread t13 = thread(thread_loop_send,13);
    thread t14 = thread(thread_loop_send,14);
    thread t15 = thread(thread_loop_send,15);
    thread t16 = thread(thread_loop_send,16);
    thread t17 = thread(thread_loop_send,17);
    thread t18 = thread(thread_loop_send,18);
    thread t19 = thread(thread_loop_send,19);
    thread t20 = thread(thread_loop_send,20);

#else
    sawa_init(User_2, process_request);

#endif

    while(true)
        sleep(10);

    return 0;
}

