#include "midi_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>

static const char* devices[] = {"/dev/midi", "/dev/midi2", NULL};

int try_open_device(const char* device);

int open_midi_device(MidiInterface* interface, const char* device)
{
    int fd;

    if(device != NULL)
    {
        fd = try_open_device(device);
    }
    else 
    {
        int i = 0;
        while((device = devices[i++]) != NULL)
        {
            fd = try_open_device(device);
            if(fd >= 0)
                break;
        }
    }

    if(device == NULL)
        fprintf(stderr, "Failed to locate MIDI device.\n");
    else if(fd < 0)
        fprintf(stderr, "Failed to open MIDI device %s\n", device);
    else
        printf("Using device %s\n", device);

    interface->fd = fd;
    return fd;
}

int read_midi_device(MidiInterface interface, Message* buffer)
{
    uint8_t status;
    int result = read(interface.fd, &status, 1);
    if(result < 0)
        return MIDI_READ_ERROR;

    result = decode_status_byte(buffer, status);
    if(result < 0)
        return result;
    
    buffer->length = data_length(buffer->type);
    if(buffer->length < 0)
        return buffer->length;

    result = read(interface.fd, buffer->data, buffer->length);
    if(result < 0)
        return MIDI_READ_ERROR;

    if(buffer->type == NOTE_ON && buffer->data[1] == 0)
    {
        buffer->type = NOTE_OFF;
    }

    return 0;
}

int write_midi_device(MidiInterface interface, const Message* buffer)
{
    uint8_t status;
    int result = encode_status_byte(buffer, &status);
    if(result < 0)
        return result;

    uint8_t* buffer_out = (uint8_t*)malloc(1 + buffer->length);
    buffer_out[0] = status;
    memcpy(buffer_out + 1, buffer->data, buffer->length);

    result = write(interface.fd, buffer_out, 1 + buffer->length);
    if(result < 0)
        return MIDI_WRITE_ERROR;
}

int try_open_device(const char* device)
{
    int fd = open(device, O_RDWR, 0);

    return fd;
}