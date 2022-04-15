#ifndef _MIDI_INTERFACE_H_
#define _MIDI_INTERFACE_H_

#include <stdint.h>
#include <pthread.h>

#include "message.h"
#include "midi_error.h"

typedef struct MidiInterface
{
    int fd;
    uint8_t* out_buf;
    pthread_mutex_t write_mutex;
} MidiInterface;

int open_midi_device(MidiInterface** interface, const char* device);
int close_midi_device(MidiInterface* interface);

int read_midi_device(MidiInterface* interface, Message* buffer);
int write_midi_device(MidiInterface* interface, const Message* buffer);
int write_midi_device_raw(MidiInterface* interface, const char* buffer);


#endif