/* Max Schweitzer
   ECE 373
   Homework 4 - PCI LED Driver
   This code implements a user space program to
   test a basic PCI driver that can blink an LED.
   5/1/18
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define LENGTH 256
#define LED0_MASK 0x0000000F

int main()
{
  int fd;
  char toSend[LENGTH] = {0};

  strcpy(toSend, "20");
  
  fd = open("/dev/ece_led", O_RDWR);
  if (fd < 0)
  {
    printf("Open error: %d\n", fd);
    return 1;
  }

  int ret;

  // First, lets read the current value.
  ret = read(fd, toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error reading");
    return 1;
  }

  printf("Was read:             %s\n", toSend);

  // Change blink rate
  strcpy(toSend, "5");

  printf("Will write:           %s\n", toSend);

  ret = write(fd, toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error writing!");
    return 1;
  }

  ret = read(fd, toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error reading");
    return 1;
  }

  printf("Was read after write: %s\n", toSend);

  sleep(2); 

  // Change blink rate
  strcpy(toSend, "1");

  printf("Will write:           %s\n", toSend);
  
  ret = write(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error writing!");
    return 1;
  }

  ret = read(fd, toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error reading");
    return 1;
  }

  printf("Was read after write: %s\n", toSend);
  
  ret = close(fd);

  return ret;
}
