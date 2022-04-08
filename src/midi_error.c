#include "midi_error.h"

#include <stddef.h>

static const char* error_messages[] = {
    NULL, NULL,
    "Failed to read from MIDI device.",
    "Failed to write to MIDI device.",
    "Unknown MIDI status byte"
};

const char* midi_strerror(MidiError error)
{
    return error_messages[-error];
}