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

MidiInterface* interface;

static int _Atomic playing = ATOMIC_VAR_INIT(0);
static uint32_t _Atomic time_per_quarter = ATOMIC_VAR_INIT(1);
static uint32_t _Atomic current_tick = ATOMIC_VAR_INIT(0);

void* play_track(void* data);
void* advance_counter(void* data);

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        fprintf(stderr, "Usage: musicalpi <midi file>");
        exit(-1);
    }

    printf("Loading MIDI file...\n");
    MidiParser* parser;
    parser = parseMidi(argv[1], false, true);
    if(!parser)
    {
        fprintf(stderr, "Failed to read MIDI file\n");
        exit(-1);
    }
    printf("Done. Press enter to continue.\n");
    getchar();  

    int result = open_midi_device(&interface, NULL);
    if(result < 0)
        exit(result);

    pthread_t counter_thread;
    pthread_create(&counter_thread, NULL, advance_counter, &(parser->ticks));
    pthread_detach(counter_thread);

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

void* advance_counter(void* data)
{
    uint16_t ticks_per_quarter = *((uint32_t*)data);
    struct timespec begin, end;
    while(!playing);

    clock_gettime(CLOCK_REALTIME, &begin);
    for(;;)
    {
        clock_gettime(CLOCK_REALTIME, &end);
        uint64_t diff = 1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000;
        diff -= (uint64_t)time_per_quarter / ticks_per_quarter;
        diff *= (diff < (uint64_t)time_per_quarter / ticks_per_quarter);
        clock_gettime(CLOCK_REALTIME, &begin);
        usleep((uint64_t)time_per_quarter / ticks_per_quarter - diff);
        current_tick++;
    }
}

void* play_track(void* data)
{
    Track* track = (Track*)data;
    Message* message;
    create_message(&message);
    message->length = 2;

    uint64_t tick_of_last_event = 0;
    int current_event = 0;

    while(!playing);

    while(current_event < track->nbOfEvents)
    {
        Event event = track->events[current_event];
        while(tick_of_last_event + event.timeToAppear > current_tick);

        if(event.type == MidiTempoChanged)
        {
            time_per_quarter = ((uint32_t*)(event.infos))[0];
        }
        else if(event.type == MidiNotePressed || event.type == MidiNoteReleased || event.type == MidiControllerValueChanged)
        {
            message->channel = (((uint8_t*)(event.infos))[0] & 0xF);
            message->data = event.infos + 1;

            switch(event.type)
            {
                case MidiNotePressed:
                    message->type = NOTE_ON;
                    break;

                case MidiNoteReleased:
                    message->type = NOTE_OFF;
                    break;

                case MidiControllerValueChanged:
                    message->type = CONTROLLER_CHANGE;
                    break;
            }
            // printf("Sending %d %d\n", message->data[0], message->data[1]);
            int result = write_midi_device(interface, message);
            if(result < 0)
                fprintf(stderr, "Write err: %s\n", midi_strerror(result));
        }

        current_event++;
        tick_of_last_event += event.timeToAppear;
    }
}