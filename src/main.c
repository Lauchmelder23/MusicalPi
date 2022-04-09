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
#include "midi_parser.h"

uint32_t time_per_quarter;
uint32_t ticks_per_quarter;
MidiInterface* interface;

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
static int _Atomic playing = ATOMIC_VAR_INIT(0);

void* arpeggio_loop(void* data);
void* play_track(void* data);

int main(int argc, char** argv)
{
    MidiParser* parser;
    parser = parseMidi("out/pushing_onwards.mid", false, true);
    if(!parser)
    {
        fprintf(stderr, "Failed to read MIDI file\n");
        exit(-1);
    }

    time_per_quarter = ((uint32_t*)(parser->tracks[0].events[2].infos))[0];
    ticks_per_quarter = parser->ticks;

    int result = open_midi_device(&interface, NULL);
    if(result < 0)
        exit(result);

    pthread_t* threads = (pthread_t*)malloc(parser->nbOfTracks * sizeof(pthread_t));
    for(int i = 0; i < parser->nbOfTracks; i++)
    {
        pthread_create(threads + i, NULL, play_track, parser->tracks + i);
    }

    playing = 1;

    /*
    int key_down = 0;
    Note current_note;

    pthread_t arpeggio_thread;
    pthread_create(&arpeggio_thread, NULL, arpeggio_loop, (void*)interface);

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
    */
    for(int i = 0; i < parser->nbOfTracks; i++)
    {
        pthread_join(threads[i], NULL);
    }
    free(threads);
    close_midi_device(interface);
    

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
            int result = write_midi_device(interface, message);
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

void* play_track(void* data)
{
    Track* track = (Track*)data;
    Message* message;
    create_message(&message);
    message->channel = 0;
    message->length = 2;

    int current_event = 0;

    while(current_event < track->nbOfEvents)
    {
        if(!playing)
            continue;

        Event event = track->events[current_event];
        usleep(event.timeToAppear * time_per_quarter / ticks_per_quarter);

        if(event.type == MidiNotePressed || event.type == MidiNoteReleased)
        {
            message->type = (event.type == MidiNotePressed) ? NOTE_ON : NOTE_OFF;
            message->data = event.infos + 1;

            // printf("Sending %d %d\n", message->data[0], message->data[1]);
            int result = write_midi_device(interface, message);
            if(result < 0)
                fprintf(stderr, "Write err: %s\n", midi_strerror(result));
        }

        current_event++;
    }
}