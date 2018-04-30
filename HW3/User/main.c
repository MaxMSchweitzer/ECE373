/* Max Schweitzer
   ECE 373
   Homework 3 - PCI LED Driver
   This code implements a user space program to
   test a basic PCI driver that can blink an LED.
   4/22/18
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define LENGTH 256

int main()
{
  int fd;
  uint32_t toSend = 0;

  fd = open("/dev/hw3_pci", O_RDWR);
  if (fd < 0)
  {
    printf("Open error: %d\n", fd);
    return 1;
  }

  int ret;

  // First, lets read the current value.
  ret = read(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error reading");
    return 1;
  }

  printf("Was read: 0x%08x\n", toSend);

  // Turns off LEDs1,2,3 and turns on LED0
  toSend = 0x0f0f0f0e;

  ret = write(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error writing!");
    return 1;
  }

  // Overwrite before we read again.
  toSend = 0;

  ret = read(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error reading");
    return 1;
  }

  printf("Was read after write: 0x%08x\n", toSend);

  sleep(2); 

  // Turns off LEDs1,2,3,0
  toSend = 0x0f0f0f0f;

  ret = write(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error writing!");
    return 1;
  }

  ret = close(fd);

  return ret;
}
