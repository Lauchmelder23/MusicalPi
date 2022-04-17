#include "mapping.h"

#include <stdio.h>
#include <stdlib.h>

uint8_t* load_program_map(const char* filename)
{
    FILE* fp = fopen(filename, "r");
    if(fp == NULL)
    {
        fprintf(stderr, "Failed to open mapping file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    uint8_t* map = (uint8_t*)calloc(1, 127);

    size_t read;
    size_t current_line = 0;
    size_t len = 0;
    char* line = NULL;
    while((read = getline(&line, &len, fp)) != -1)
    {
        current_line++;
        if(read == 1)
            continue;

        size_t colon_pos = 0;
        do
        {
            if(line[colon_pos] == '\0')
            {
                fprintf(stderr, "Invalid syntax in mapping file at line %u. Ignoring.\n", current_line);
                continue;
            }
        } while(line[++colon_pos] != ':');
  
        line[colon_pos] = '\0';
        int from = atoi(line);
        int to = atoi(line + colon_pos + 1);

        if(from < 0 || 127 < from)
        {
            fprintf(stderr, "Invalid MIDI program ID in line %u. Ignoring\n", current_line);
            continue;
        }

        map[from & 0xFF] = to & 0xFF;
    }

    fclose(fp);
    return map;
}

uint8_t* get_default_map()
{
    uint8_t* map = (uint8_t*)malloc(127);
    for(int i = 1; i < 128; i++)
    {
        map[i] = i;
    }

    return map;
}