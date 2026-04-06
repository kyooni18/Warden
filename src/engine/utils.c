#define _POSIX_C_SOURCE 200809L
#include "engine/utils.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* One game-minute in wall milliseconds. */
#define GAME_MINUTE_MS 1000ULL

int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

uint64_t minutes_to_ms(int minutes)
{
    return (uint64_t)minutes * GAME_MINUTE_MS;
}

int roll_range(int min_value, int max_value)
{
    return min_value + rand() % (max_value - min_value + 1);
}

bool is_blank(const char *text)
{
    while (*text != '\0') {
        if (!isspace((unsigned char)*text))
            return false;
        text++;
    }
    return true;
}

void canonicalize_input(const char *input, char *output, size_t output_size)
{
    size_t out = 0;
    bool in_space = false;
    const char *cursor = input;

    while (*cursor != '\0' && isspace((unsigned char)*cursor))
        cursor++;

    while (*cursor != '\0' && out + 1 < output_size) {
        unsigned char ch = (unsigned char)*cursor;
        if (isspace(ch)) {
            if (!in_space) {
                output[out++] = ' ';
                in_space = true;
            }
        } else {
            output[out++] = (char)tolower(ch);
            in_space = false;
        }
        cursor++;
    }
    if (out > 0 && output[out - 1] == ' ')
        out--;
    output[out] = '\0';
}

bool starts_with(const char *text, const char *prefix)
{
    while (*prefix != '\0') {
        if (*text != *prefix)
            return false;
        text++;
        prefix++;
    }
    return true;
}
