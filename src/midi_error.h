#ifndef _MIDI_ERROR_H_
#define _MIDI_ERROR_H_

typedef enum MidiError
{
    MIDI_READ_ERROR = -2, 
    MIDI_WRITE_ERROR = -3,
    MIDI_UNKNOWN_STATUS_BYTE = -4
} MidiError;

const char* midi_strerror(MidiError error);

#endif