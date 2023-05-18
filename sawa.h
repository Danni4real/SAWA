#ifndef SAWA_H
#define SAWA_H

#include <string>

/* if user need to process request from other users, who should implement process_request and pass it to sawa_init */
bool sawa_init(unsigned int user_id, /* user_id should > 0 */
               std::string (*process_request)(unsigned int sender_id, const std::string& message) = nullptr);

/* send a message and wait ack */
std::string sawa_send(unsigned int receiver_id,
                      const std::string& message,/* message size <= 1M bytes */
                      unsigned int timeout_seconds = 3);

#endif
