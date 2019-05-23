/* Wireless Smoke & Heat Detector
 * Ningbo Siterwell Electronics  GS 558  Sw. V05  Ver. 1.3  on 433.885MHz
 * VisorTech RWM-460.f  Sw. V05, distributed by PEARL, seen on 433.674MHz
 *
 * A short wakeup pulse followed by a wide gap (11764 us gap),
 * followed by 24 data pulses and 2 short stop pulses (in a single bit width).
 * This is repeated 8 times with the next wakeup directly following
 * the preceding stop pulses.
 *
 * Bit width is 1731 us with
 * Short pulse: -___ 436 us pulse + 1299 us gap
 * Long pulse:  ---_ 1202 us pulse + 526 us gap
 * Stop pulse:  -_-_ 434us pulse + 434us gap + 434us pulse + 434us gap
 * = 2300 baud pulse width / 578 baud bit width
 *
 * 24 bits (6 nibbles):
 * - first 5 bits are unit number with bits reversed
 * - next 15(?) bits are group id, likely also reversed
 * - last 4 bits are always 0x3 (maybe hardware/protocol version)
 * Decoding will reverse the whole packet.
 * Short pulses are 0, long pulses 1, need to invert the demod output.
 *
 * Each device has it's own group id and unit number as well as a
 * shared/learned group id and unit number.
 * In learn mode the primary will offer it's group id and the next unit number.
 * The secondary device acknowledges pairing with 16 0x555555 packets
 * and copies the offered shared group id and unit number.
 * The primary device then increases it's unit number.
 * This means the primary will always have the same unit number as the
 * last learned secondary, weird.
 * Also you always need to learn from the same primary.
 *
 * Copyright (C) 2017 Christian W. Zuckschwerdt <zany@triq.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "decoder.h"

static int instapark_n1_callback(r_device *decoder, bitbuffer_t *bitbuffer)
{
    data_t *data;
    uint8_t *b;
    int r;
    int button; // max 30
    int id;
    char code_str[16];

    if (bitbuffer->num_rows < 4)
        return 0; // truncated transmission

    bitbuffer_invert(bitbuffer);


//    b[0] = reverse8(b[0]);
//    b[1] = reverse8(b[1]);
//    b[2] = reverse8(b[2]);

    r = -1;
    for (int i = 0; i < bitbuffer->num_rows; ++i) {
        if (bitbuffer->bits_per_row[i] != 25) continue;
        
        if (count_repeats(bitbuffer, i) <= 3) continue;
        
        r = i;
        break;
    }
    
    if (r == -1) return -1;
    
    b = bitbuffer->bb[r];


    b[0] = reverse8(b[0]);
    b[1] = reverse8(b[1]);
    
    id = ((b[0] & 0x3f) << 7) | (b[1] >> 1);

    if (id == 0 || id == 0x7fff)
         return 0; // reject min/max to reduce false positives

    sprintf(code_str, "%02x %02x %02x", b[0], b[1], b[2]);

    switch(b[2]) {
    case 0x08:
      button = 1; break;
    case 0x09:
      button = 2; break;
    case 0x0A:
      button = 3; break;
    case 0x06:
      button = 4; break;
    default:
      button = 0;
    }

    data = data_make(
        "model",         "",            DATA_STRING, _X("Instapark-N1","Instapark N1 Dimmer Remote"),
        "id"   ,         "",            DATA_INT, id,
        "button",  "Button Pressed",         DATA_INT, button,
        "button1",  "Button 1",         DATA_STRING, button == 1 ? "PRESSED" : "",
        "button2",  "Button 2",         DATA_STRING, button == 2 ? "PRESSED" : "",
        "button3",  "Button 3",         DATA_STRING, button == 3 ? "PRESSED" : "",
        "button4",  "Button 4",         DATA_STRING, button == 4 ? "PRESSED" : "",
        "code",          "Raw Code",    DATA_STRING, code_str,
        NULL);
    decoder_output_data(decoder, data);

    return 1;
}

static char *output_fields[] = {
    "model",
    "id",
    "button",
    "button1",
    "button2",
    "button3",
    "button4",
    "code",
    NULL
};

r_device instapark_n1 = {
    .name           = "Instapark N1 Dimmer Remote",
    .modulation     = OOK_PULSE_PWM,
    .short_width    = 300,
    .long_width     = 920,
    .gap_limit      = 2000,
    .reset_limit    = 9500,
    .decode_fn      = &instapark_n1_callback,
    .disabled       = 0,
    .fields         = output_fields
};
