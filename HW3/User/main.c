/* Max Schweitzer
   ECE 373
   Homework 2 - Char Driver
   This code implements a user space program to
   test a basic char driver.
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

  //char receive;
  int send = 0;
  uint32_t toSend = 0;

  //char toSend[LENGTH] = {0};

  //strcpy(toSend, "20");

  fd = open("/dev/hw2_kernel", O_RDWR);
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

  // Now get a value from the user and send it over.
  printf("Enter a number in hex: ");
  scanf("%08x", &toSend);

  printf("Will write: 0x%08x\n", toSend);

  ret = write(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error writing!");
    return 1;
  }

  // Overwrite before we read again.
  //strcpy(toSend, "0");
  toSend = 0;

  ret = read(fd, &toSend, sizeof(uint32_t));
  if (ret < 0)
  {
    printf("Error reading");
    return 1;
  }

  printf("Was read after write: 0x%08x\n", toSend);

  ret = close(fd);

  return ret;
}