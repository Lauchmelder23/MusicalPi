#include <linux/soundcard.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/time.h>

#include "midi_interface.h"
#include "midi_parser.h"

uint32_t time_per_quarter;
uint32_t ticks_per_quarter;
MidiInterface* interface;

static int _Atomic playing = ATOMIC_VAR_INIT(0);

void* play_track(void* data);

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        fprintf(stderr, "Usage: musicalpi <midi file>");
        exit(-1);
    }



    MidiParser* parser;
    parser = parseMidi(argv[1], false, true);
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

    for(int i = 0; i < parser->nbOfTracks; i++)
    {
        pthread_join(threads[i], NULL);
    }
    free(threads);
    close_midi_device(interface);
    

    return 0;
}

void* play_track(void* data)
{
    Track* track = (Track*)data;
    Message* message;
    create_message(&message);
    message->channel = 0;
    message->length = 2;

    int current_event = 0;
    struct timeval begin, end;

    while(!playing);
    gettimeofday(&begin, NULL);

    while(current_event < track->nbOfEvents)
    {
        Event event = track->events[current_event];

        gettimeofday(&end, NULL);
        uint64_t deltatime = ((end.tv_sec - begin.tv_sec) * 1000000) + (end.tv_usec - begin.tv_usec);
        if(event.timeToAppear > 0)
            usleep(event.timeToAppear * time_per_quarter / ticks_per_quarter - deltatime);
        gettimeofday(&begin, NULL);

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