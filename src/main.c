#include <linux/soundcard.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

static const char* devices[] = {"/dev/midi", "/dev/midi2", NULL};

typedef struct Note
{
    unsigned char channel;
    unsigned char pitch;
    unsigned char velocity;
} Note;

typedef struct Offset
{
    signed char pitch;
    signed char velocity
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
    unsigned char data[3] = {0, 0, 0};
    int key_down = 0;
    Note current_note;

    const char* device;
    int fd;
    int i = 0;
    while((device = devices[i++]) != NULL)
    {
        printf("Trying %s...", device);

        fd = open(device, O_RDWR, 0);
        if(fd < 0)
        {
            printf("Failed: %s\n", strerror(errno));
        }
        else
        {
            printf("Success!\n");
            break;
        }
    }

    if(fd < 0)
    {
        fprintf(stderr, "No MIDI devices detected.\n");
        return -1;
    }

    printf("Using MIDI device %s\n", device);
    pthread_t arpeggio_thread;
    pthread_create(&arpeggio_thread, NULL, arpeggio_loop, (void*)fd);

    for(;;)
    {
        read(fd, (void*)data, sizeof(data));

        if(data[0] != 0xfd)
        {
            if(data[2] > 0)
            {
                key_down = 1;
                note = &current_note;

                current_note.channel = 0x90;
                current_note.pitch = data[1];
                current_note.velocity = data[2];
            }
            else if(data[1] == current_note.pitch)
            {
                key_down = 0;
                note = NULL;
                step = 0;
            }
        }
    }

    pthread_join(arpeggio_thread, NULL);
    close(fd);
    return 0;
}

void *arpeggio_loop(void* data)
{
    int fd = (int)data;

    while(stop == 0)
    {
        if(note != NULL)
        {
            Note out = *note;
            out.pitch += pattern[step].pitch;
            out.velocity += pattern[step].velocity;
            write(fd, &out, sizeof(Note));

            step++;
            if(step >= (sizeof(pattern) / sizeof(Offset)))
                step = 0;

            usleep(166666);
        }
    }
}