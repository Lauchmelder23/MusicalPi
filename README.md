# MusicalPi
A program for the RaspberryPi to play MIDI files on MIDI devices.

This program just sends MIDI data contained in a MIDI file to a connected MIDI device and also supports mapping program patch numbers.

## Program Patch Number Mapping
A mapping file looks like this
```
0:1
1:1
4:7
```
This would map program patch numbers 0, 1 and 4 in the MIDI to the patch numbers 1, 1 and 7. Those are the numbers ultimately sent by the program.
Numbers not listed stay unchanged.

Every line is one mapping, and has the format `from:to`

This lets you change the instruments contained in a MIDI file

## Usage
`./musicalpi [-m mapping_file] midi_file
