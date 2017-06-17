/* measurement.c - example c-bus measurement app.
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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <libcgate.h>

/* Set your c-bus project and network here
 * APP, DEV and CH1 can be left as is or
 * changed to suit your application */

#define PROJECT "HOME"
#define NET 254
#define APP 228
#define DEV 1
#define CH1 1

#if defined DEBUG
    #define DEBUG_PRINT(fmt, ...) do { printf("%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#else
    #define DEBUG_PRINT(fmt, ...) /* Don't do anything in release builds */
#endif

static int32_t sockfd;

uint8_t w1_find_node(char *devPath)
{
   DIR *dir;
   struct dirent *dirent;
   char dev[16];
   char path[] = "/sys/bus/w1/devices";

   dir = opendir(path);
   if (dir != NULL)
   {
        while ((dirent = readdir (dir)))
        if (dirent->d_type == DT_LNK && strstr(dirent->d_name, "28-") != NULL) { 
            strcpy(dev, dirent->d_name);
            DEBUG_PRINT("w1 Device: %s\n", dev);
        }
        (void)closedir(dir);
    }
    else
    {
        perror ("Couldn't find the w1 devices directory\n");
        return 1;
    }
    sprintf(devPath, "%s/%s/w1_slave", path, dev);
    return 0;

}

int main(int argc, char *argv[])
{
    int32_t portno, fd;
    char devPath[128];
    char buf[256];
    char tmpData[6];
    float tempC;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = cgate_connect(argv[1],portno, (int8_t*)PROJECT, NET);
    printf("Socket = %d\n", sockfd);
    if(sockfd <= 0){
        perror("Couldn't connect to C-Gate\n");
        return -1;
    }

    if(w1_find_node(devPath))
        return -1;

    while(1)
    {
        fd = open(devPath, O_RDONLY);
        if(fd == -1){
            perror("Couldn't open the w1 device.\n");
            return -1;
        }
        /* Read the sensor */
        if(read(fd, buf, 256) > 0)
        {
           strncpy(tmpData, strstr(buf, "t=") + 2, 5);
           tempC = strtof(tmpData, NULL);
           DEBUG_PRINT("Temp: %.3f C\n", tempC / 1000);
        }
        close(fd);
        if(cgate_send_measurement(sockfd, NET, APP, DEV, CH1, (int32_t)tempC, -3, cbus_measurement_ce_centigrade)){
            perror("Couldn't sent to cbus.\n");
            close(sockfd);
            return -1;
        }
        /* send temp on bus every 20 seconds */
        sleep(20);
    }
}


