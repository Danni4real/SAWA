#ifndef MESSAGEQ_H
#define MESSAGEQ_H

#include <string>

#include "share.h"

#define MAX_MESSAGE_SIZE 1024*1024 // 1M

enum MqMessageType {InvalidType, REQUEST, ACK};

bool mq_send(uint destination_id, uint message_type, const std::string& message_content); // message_content should end with '\0'
bool mq_recv(uint          my_id, uint message_type,       std::string* message_content);

#endif
