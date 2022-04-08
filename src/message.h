#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>

typedef enum MessageType
{
    NOTE_OFF, NOTE_ON,
    SYSTEM_EXCLUSIVE
} MessageType;

typedef struct Message
{
    uint8_t channel;
    MessageType type;

    int length;
    uint8_t* data;
} Message;

void create_message(Message** message);
void free_message(Message* message);

int data_length(MessageType type);
int decode_status_byte(Message* message, uint8_t status);
int encode_status_byte(const Message* message, uint8_t* status);

#endif