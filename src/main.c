#include <linux/soundcard.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#include "midi_interface.h"

typedef struct Note
{
    unsigned char channel;
    unsigned char pitch;
    unsigned char velocity;
} Note;

typedef struct Offset
{
    signed char pitch;
    signed char velocity;
} Offset;

static const Offset pattern[] = {
    { 0,      0 },
    { -5,   -11 },
    { -9,   -11 },
    { -12,  -11 },
    { -9,   -11 },
    { -5,   -11 },
    { -1,     0 },
    { -5,   -11 },
    { -9,   -11 },
    { -12,  -11 },
    { -9,   -11 },
    { -5,   -11 },
    { -2,     0 },
    { -5,   -11 },
    { -9,   -11 },
    { -12,  -11 },
    { -9,   -11 },
    { -5,   -11 },
    { -3,     0 },
    { -5,   -11 },
    { -9,   -11 },
    { -12,  -11 },
    { -9,   -11 },
    { -5,   -11 }
};

static int _Atomic stop = ATOMIC_VAR_INIT(0);
static int _Atomic step = ATOMIC_VAR_INIT(0);
static Note* _Atomic note = ATOMIC_VAR_INIT(NULL);

void *arpeggio_loop(void* data);

int main(int argc, char** argv)
{
    MidiInterface interface;
    int result = open_midi_device(&interface, NULL);
    if(result < 0)
        exit(result);

    int key_down = 0;
    Note current_note;

    pthread_t arpeggio_thread;
    pthread_create(&arpeggio_thread, NULL, arpeggio_loop, (void*)&interface);

    Message* message;
    create_message(&message);

    for(;;)
    {
        result = read_midi_device(interface, message);
        if(result < 0)
        {
            fprintf(stderr, "Failed to read message: %s\n", midi_strerror(result));
            if(result == MIDI_UNKNOWN_STATUS_BYTE)
            {
                fprintf(stderr, "%x\n", message->type);
                exit(result);
            }
        }

        // if(message->type != SYSTEM_EXCLUSIVE)
            // printf("ch: %x, s: %x -- %02x %02x\n", message->channel, message->type, message->data[0], message->data[1]);

        if(message->type == NOTE_ON)
        {
            key_down = 1;
            note = &current_note;

            current_note.channel = 0x90;
            current_note.pitch = message->data[0];
            current_note.velocity = message->data[1];
        }
        else if(message->type == NOTE_OFF && message->data[0] == note->pitch)
        {
            key_down = 0;
            note = NULL;
            step = 0;
        }
    }

    pthread_join(arpeggio_thread, NULL);
    free_message(message);
    close(interface.fd);
    return 0;
}

void *arpeggio_loop(void* data)
{
    MidiInterface* interface = (MidiInterface*)data;
    Message* message;
    create_message(&message);
    message->channel = 0;
    message->length = 2;
    message->type = NOTE_ON;

    while(stop == 0)
    {
        if(note != NULL)
        {
            Note out = *note;
            message->data[0] = out.pitch + pattern[step].pitch;
            message->data[1] = out.velocity + pattern[step].velocity;
            int result = write_midi_device(*interface, message);
            if(result < 0)
                fprintf(stderr, "Failed to send message: %s\n", midi_strerror(result));

            step++;
            if(step >= (sizeof(pattern) / sizeof(Offset)))
                step = 0;

            usleep(166666);
        }
    }

    free_message(message);
}