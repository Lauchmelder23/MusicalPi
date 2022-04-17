#include "message.h"
#include <stdlib.h>
#include <stdio.h>

#include "midi_error.h"

void create_message(Message** message)
{
    *message = (Message*)malloc(sizeof(Message));
    (*message)->data = (uint8_t*)malloc(8);
}

void free_message(Message* message)
{
    free(message->data);
    free(message);
}

int data_length(MessageType type)
{
    switch(type)
    {
    case NOTE_ON:   return 2;
    case NOTE_OFF:  return 2;
    case CONTROLLER_CHANGE: return 2;
    case PROGRAM_CHANGE: return 1;

    case SYSTEM_EXCLUSIVE: return 0;
    }

    return MIDI_UNKNOWN_STATUS_BYTE;
}

int decode_status_byte(Message* message, uint8_t status)
{
    message->channel = (status & 0xF);
    message->type = (status & 0xF0);
    switch(status & 0xF0)
    {
    case 0x90:
        message->type = NOTE_ON;
        break;

    case 0x80:
        message->type = NOTE_OFF;
        break;

    case 0xB0:
        message->type = CONTROLLER_CHANGE;
        break;

    case 0xC0:
        message->type = PROGRAM_CHANGE;
        break;

    case 0xF0:
        message->type = SYSTEM_EXCLUSIVE;
        break;

    default:
        return MIDI_UNKNOWN_STATUS_BYTE;
    }

    return 0;
}

int encode_status_byte(const Message* message, uint8_t* status)
{
    *status = 0;
    *status |= (message->channel & 0xF);

    switch(message->type)
    {
    case NOTE_ON:
        *status |= 0x90;
        break;

    case NOTE_OFF:
        *status |= 0x80;
        break;

    case CONTROLLER_CHANGE:
        *status |= 0xB0;
        break;

    case PROGRAM_CHANGE:
        *status |= 0xC0;
        break;

    case SYSTEM_EXCLUSIVE:
        *status |= 0xF0;
        break;

    default:
        return MIDI_UNKNOWN_STATUS_BYTE;
    }
    
    return 0;   
}