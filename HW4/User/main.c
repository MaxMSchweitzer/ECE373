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
#define LED0_MASK 0x0000000F

int main()
{
  int fd;
  uint32_t toSend = 0;
  uint32_t toRead = 0;

  fd = open("/dev/HW4_pci", O_RDWR);
  if (fd < 0)
  {
    printf("Open error: %d\n", fd);
    return 1;
  }

  int ret;

  // First, lets read the current value.
  ret = read(fd, &toRead, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error reading");
    return 1;
  }

  printf("Was read:             0x%08x\n", toRead);

  // Turn on LED0
  toSend = 0;
 
  // Read, modify, write so we don't clobber reserved bits.
  toRead = toRead & (~LED0_MASK);
  toSend = toRead | 0x0000000e;

  printf("Will write:           0x%08x\n", toSend);

  ret = write(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error writing!");
    return 1;
  }

  // Overwrite before we read again.
  toRead = 0;

  ret = read(fd, &toRead, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error reading");
    return 1;
  }

  printf("Was read after write: 0x%08x\n", toRead);

  sleep(2); 

  // Turn off LED0
  toSend = 0;

  // Read modify write
  toRead = toRead & (~LED0_MASK);
  toSend = toRead | 0x0000000f;

  printf("Will write:           0x%08x\n", toSend);
  
  ret = write(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error writing!");
    return 1;
  }
  
  ret = close(fd);

  return ret;
}
