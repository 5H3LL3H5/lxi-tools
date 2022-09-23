/*
 * Copyright (c) 2022  Perry Hung
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <lxi.h>
#include "error.h"
#include "screenshot.h"

#define IMAGE_SIZE_MAX 0x400000 // 4 MB

int lecroy_screenshot(char *address, char *id, int timeout)
{
    char* response = malloc(IMAGE_SIZE_MAX);
    char *command;
    int device, length;

    UNUSED(id);

    // Connect to LXI instrument
    device = lxi_connect(address, 0, NULL, timeout, VXI11);
    if (device == LXI_ERROR)
    {
        error_printf("Failed to connect\n");
        goto error_connect;
    }

    // Delete any existing file, if any. This ensures that the autoincrementing
    // suffix counter is reset to zero.
    command = "DELETE_FILE DISK,HDD,FILE,'D:\\HardCopy\\lxi-screenshot--00000.png'";
    lxi_send(device, command, strlen(command), timeout);

    // Set hardcopy to dump PNGs to a file
    command = "hardcopy_setup DEV,png,DEST,FILE,DIR,'D:\\HardCopy\',AREA,FULLSCREEN,FILE,'lxi-screenshot'";
    lxi_send(device, command, strlen(command), timeout);

    // Trigger screendump
    command = "scdp";
    lxi_send(device, command, strlen(command), timeout);

    // Read it back
    command = "TRFL? DISK,HDD,FILE,'D:\\HardCopy\\lxi-screenshot--00000.png'";
    lxi_send(device, command, strlen(command), timeout);
    length = lxi_receive(device, response, IMAGE_SIZE_MAX, timeout);
    if (length < 0)
    {
        error_printf("Failed to receive message\n");
        goto error_receive;
    }

    // Skip TRFL? response
    char c = response[6];
    length -= 6;

    // Skip IEEE header
    char *image = &response[7];
    int n = atoi(&c);
    image += n;
    length -= n;

    // Strip 8 byte CRC footer and 2 byte terminator
    length -= 10;

    screenshot_file_dump(image, length, "png");
    free(response);
    lxi_disconnect(device);

    return 0;

error_connect:
error_receive:

    // Free allocated memory for screenshot
    free(response);

    return 1;
}

// Screenshot plugin configuration
struct screenshot_plugin lecroy =
{
    .name = "lecroy-wp",
    .description = "LeCroy WavePro Oscilloscope",
    .regex = "LECROY WP LCRY",
    .screenshot = lecroy_screenshot
};