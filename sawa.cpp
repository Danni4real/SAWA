#include <set>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <condition_variable>

#include "sawa.h"
#include "share.h"
#include "message_q.h"

#define Zero 0

#define InvalidUserID    Zero // don't change it
#define InvalidRequestID Zero // don't change it

using namespace std;

struct AckWaiter
{
    string             ack;
    bool               acked;
    mutex              lock;
    condition_variable con_var;
    uint               request_id;
};

uint MyUserId = InvalidUserID;

set<AckWaiter*>    AckWaiterList;
mutex              AckWaiterListLock;

thread_local AckWaiter ThreadLocalAckWaiter;

uint gen_request_id()
{
    static mutex lock;
    static uint id = InvalidRequestID;

    lock_guard<mutex> lk(lock);

    if((++id) == InvalidRequestID) // overflow to 0
        ++id;

    return id;
}

string combine(uint user_id, uint request_id, const string& msg)
{
    string combined("");

    combined += to_string(user_id);    combined += ",";
    combined += to_string(request_id); combined += ",";
    combined += msg;

    return combined;
}

string extract_field(const string& msg, uint pos)
{
    size_t last = 0;
    size_t next = 0;

    string field;

    for(uint i=0;i<pos;i++)
    {
        next  = msg.find(",", last);
        field = msg.substr(last, next-last);
        last  = next + 1;
    }

    return field;
}

uint extract_user_id(const string& msg)
{
    return stoul(extract_field(msg, 1));
}

uint extract_request_id(const string& msg)
{
    return stoul(extract_field(msg, 2));
}

string extract_msg_content(const string& msg)
{
    size_t last = 0;
    size_t next = 0;

    for(uint i=0;i<2;i++)
    {
        next  = msg.find(",", last);
        last  = next + 1;
    }

    return msg.substr(last);
}

bool registered(AckWaiter* ack_waiter)
{
    lock_guard<mutex> lk(AckWaiterListLock);

    if(AckWaiterList.find(ack_waiter) == AckWaiterList.end())
        return false;

    return true;
}

void register_me(AckWaiter* ack_waiter)
{
    lock_guard<mutex> lk(AckWaiterListLock);

    ack_waiter->request_id = InvalidRequestID;
    AckWaiterList.insert(ack_waiter);
}

void recv_ack_thread(uint user_id)
{
    while(true)
    {
        string recvd_msg;

        if(!mq_recv(user_id, ACK, &recvd_msg))
        {
            cout << "SAWA ERR: recv_ack_thread(): mq_recv() failed!" << endl;
            this_thread::sleep_for(chrono::seconds(1));
            continue;
        }

        lock_guard<mutex> lk(AckWaiterListLock);

        for(AckWaiter* ack_waiter: AckWaiterList)
        {
            {
                lock_guard<mutex> lk(ack_waiter->lock);

                if(ack_waiter->request_id == extract_request_id(recvd_msg))
                {
                    ack_waiter->ack   = extract_msg_content(recvd_msg);
                    ack_waiter->acked = true;
                }
                else
                    continue;
            }

            ack_waiter->con_var.notify_all();
        }
    }
}

void recv_request_thread(uint user_id, string (*process_request)(uint, const string&))
{
    while(true)
    {
        string request;

        if(!mq_recv(user_id, REQUEST, &request))
        {
            cout << "SAWA ERR: recv_request_thread(): mq_recv failed!" << endl;
            this_thread::sleep_for(chrono::seconds(1));
            continue;
        }

#ifdef Debug
        cout << time_now() << ": before mq_send()" << endl;
#endif

        // send ack
        mq_send(extract_user_id(request), ACK, combine(MyUserId,
                                                       extract_request_id(request),
                                                       process_request(extract_user_id(request),
                                                                       extract_msg_content(request))));
    }
}

bool sawa_init(uint user_id,
               string (*process_request)(uint sender_id, const string&))
{
    if(user_id == InvalidUserID)
    {
        cout << "SAWA ERR: sawa_init(): user_id can not be 0!" << endl;
        return false;
    }

    static mutex lock;
    static bool initialized = false;

    lock_guard<mutex> lk(lock);

    if(initialized)
    {
        cout << "SAWA ERR: sawa_init(): already initialized!" << endl;
        return false;
    }

    MyUserId = user_id;

    static thread recv_ack(recv_ack_thread, user_id);

    if(process_request != nullptr)
        static thread recv_request(recv_request_thread, user_id, process_request);

    initialized = true;
    return true;
}


string sawa_send(uint receiver_id, const string& msg, uint timeout_seconds)
{
    if(MyUserId == InvalidUserID)
    {
        cout << "SAWA ERR: sawa_send(): call sawa_init first!" << endl;
        return "";
    }

    if(receiver_id == InvalidUserID)
    {
        cout << "SAWA ERR: sawa_send(): receiver_id can not be 0!" << endl;
        return "";
    }

    if(!registered(&ThreadLocalAckWaiter))
        register_me(&ThreadLocalAckWaiter); // register ack waiter of this thread

    uint request_id = gen_request_id();

    unique_lock<mutex> lk(ThreadLocalAckWaiter.lock);

    ThreadLocalAckWaiter.ack        = "";
    ThreadLocalAckWaiter.acked      = false;
    ThreadLocalAckWaiter.request_id = request_id;

   if(!mq_send(receiver_id, REQUEST, combine(MyUserId, request_id, msg)))
   {
       cout << "SAWA ERR: sawa_send(): mq_send(): failed!" << endl;
       return "";
   }

    if(!ThreadLocalAckWaiter.con_var.wait_for(lk, chrono::seconds(timeout_seconds), []{return ThreadLocalAckWaiter.acked;}))
        cout << "SAWA ERR: sawa_send(): wait_for(): timeout!" << endl;

    return ThreadLocalAckWaiter.ack;
}
