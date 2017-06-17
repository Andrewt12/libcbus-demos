/* blinds.c - example c-bus shatter relay app to control RF blinds
 * Copyright (C) 2017, Andrew Tarabaras.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <stdint.h>
#include <time.h>
#include "codes.h"
#include "libcgate.h"

/* Define the PIN where the 433mhz Transmitter will connect
 * TUNE shortens the pulses, I found the pulses ended up longer
 * on the OrangePI Zero than what I recorded on the ESP8266 */

#define PIN 11
#define TUNE 60
#define DEBUG

#define PROJECT "HOME"
#define NET 254
#define APP 56
#define CH1 50
#define CH2 51
#define CH6 52

#if defined DEBUG
    #define DEBUG_PRINT(fmt, ...) do { printf("%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#else
    #define DEBUG_PRINT(fmt, ...) /* Don't do anything in release builds */
#endif

static int32_t sockfd;

void blindsUpDown(uint8_t group, uint8_t level)
{
    int* code;
    uint32_t i;
    uint32_t codelength;
    time_t seconds;
    seconds = time (NULL);

    /* For some reason we neet to send a group off when the eDLT
     * widget is set to shutter relay otherwise it the indercator
     * on the eDLT will stay lit.
     *
     * When we receive back the group off then actuate the blind */

    if((group >= CH1) || (group <= CH6))
        if(level == 2){
            cgate_set_group(sockfd, NET, APP, group, 0);
            return;
        }

    if(group == CH1){
        if(level <= 1){
            codelength = sizeof(downch1)/sizeof(downch1[0]);
            code = downch1;
        }else if(level >= 99){
            codelength = sizeof(upch1)/sizeof(upch1[0]);
            code = upch1;
        }else{
            return;
        }
    }else if(group == CH2){
        if(level <= 1){
            codelength = sizeof(downch2)/sizeof(downch2[0]);
            code = downch2;
        }else if(level >= 99){
            codelength = sizeof(upch2)/sizeof(upch2[0]);
            code = upch2;
        }else{
            return;
        }
    }else if(group == CH6){
        if(level <= 1){
            codelength = sizeof(downch6)/sizeof(downch6[0]);
            code = downch6;
            // Send Off to group here
        }else if(level >= 99){
            codelength = sizeof(upch6)/sizeof(upch6[0]);
            code = upch6;
        }else{
            return;
        }
    }else{
        return;
    }

    digitalWrite (PIN, HIGH);
    for(i=0; i<codelength;i++)
    {
        usleep(code[i] - TUNE);
        digitalWrite(PIN, !digitalRead(PIN));
    }
    DEBUG_PRINT("Sent RF Command\n");

}

void CGate_Lighting_Event(uint8_t network,
                          uint8_t application,
                          uint8_t group,
                          uint8_t level,
                          uint8_t rate)
{
    DEBUG_PRINT("Received event from libcgate - Network = %d : Application = %d : Group = %d : Level = %d : Ramp Rate = %d \n", network, application, group, level, rate);
    blindsUpDown(group, level);
}

int main(int argc, char *argv[])
{
    int32_t portno;

    wiringPiSetup () ;
    pinMode (PIN, OUTPUT) ;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = cgate_connect(argv[1],portno, (int8_t*)PROJECT, NET);
    printf("Socket = %d\n", sockfd);
    if(sockfd <= 0)
        perror("");
    cgate_lighting_register_event_handler(CGate_Lighting_Event);
    /* Probaly should put a check in below to make sure we are still connected to cgate */
    while(1)
    sleep(200);
}
