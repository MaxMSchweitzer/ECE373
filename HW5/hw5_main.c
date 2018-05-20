#include <stdio.h>
#include <pci/pci.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define MEM_WINDOW_SZ 0x0200000
#define LEDCTL 0x00E00
#define LED0 0x0000000F
#define LED1 0x00000F00
#define LED2 0x000F0000
#define LED3 0x0F000000
#define GOOD_PACKETS 0x00004074

static struct pci_filter filter;

static uint32_t led_start = 0x000000E;
static uint32_t led[4] = {0x0000000F, 0x00000F00, 0x000F0000, 0x0F000000};

// TODO add comments
// TODO add error cehcking
static int register_read(struct pci_dev *dev, uint32_t address, uint32_t *read_value)
{
  int pci_fd = open("/dev/mem", O_RDONLY | O_SYNC);
  if (pci_fd < 0)
  {
    perror("Open error!\n");
    return 1;
  }
  volatile void *mem;
  mem = mmap(NULL, MEM_WINDOW_SZ, PROT_READ, MAP_SHARED, pci_fd, (dev->base_addr[0] & PCI_ADDR_MEM_MASK));
  if (mem == MAP_FAILED)
  {
    perror("Something got fucked!\n");
    close(pci_fd);
    return 1;
  }
  *read_value = *((uint32_t *)(mem + address));

  munmap((void* )mem, MEM_WINDOW_SZ);
  close(pci_fd);

  return 0;
}

static int register_write(struct pci_dev *dev, uint32_t address, uint32_t *write_value)
{

  int pci_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (pci_fd < 0)
  {
    perror("Open error!\n");
    return 1;
  }
  volatile void *mem;
  mem = mmap(NULL, MEM_WINDOW_SZ, PROT_WRITE, MAP_SHARED, pci_fd, (dev->base_addr[0] & PCI_ADDR_MEM_MASK));
  if (mem == MAP_FAILED)
  {
    perror("Something got fucked!\n");
    close(pci_fd);
    return 1;
  }
  *((uint32_t *)(mem + address)) = *write_value;

  munmap((void* )mem, MEM_WINDOW_SZ);
  close(pci_fd);

  return 0;
}

int main (int argc, char **argv)
{
  uint32_t read_val, write_val;

  struct pci_access *pacc;
  struct pci_dev *dev;

  char buf[128];

  pacc = pci_alloc();
  if (pacc == NULL)
  {
    printf("Pacc NULL!\n");
    return 1;
  }

  pci_filter_init(pacc, &filter);

  pci_init(pacc);
  pci_scan_bus(pacc);

// line 150ish in pci_raw, care about 8086:100e only, hard code it in.
// Parse this from command line better
  pci_filter_parse_id(&filter, argv[1]);

  for (dev = pacc->devices; dev; dev = dev->next)
  {
    if (pci_filter_match(&filter, dev))
      break;
  }

  pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_SIZES);
  printf( "%02x:%02x.%d (%04x:%04x)\n%s\n0x%08x\n",
        dev->bus, dev->dev, dev->func, 
      dev->vendor_id, dev->device_id,
       pci_lookup_name(pacc, buf, sizeof(buf),
        PCI_LOOKUP_VENDOR|PCI_LOOKUP_DEVICE, 
         dev->vendor_id, dev->device_id, 0, 0),
        (unsigned int)dev->base_addr[0]);

  uint32_t read = 0;
  uint32_t write = 0;
  int result = register_read(dev, LEDCTL, &read);
  if (result)
    return 1;
  uint32_t LEDCTL_orig = read;
  printf("0x%08x read from 0x%08x \n", read, (unsigned int)dev->base_addr[0] + LEDCTL); 

  write = (read & ~(LED0 | LED2)) | 0x000E000E; 
  printf("Will write 0x%08x to 0x%08x \n", write,(unsigned int)dev->base_addr[0] + LEDCTL);
  result = register_write(dev, LEDCTL, &write);
  sleep(2);
  
  result = register_read(dev, LEDCTL, &read); 
  printf("0x%08x read from 0x%08x \n", read, (unsigned int)dev->base_addr[0] + LEDCTL); 
  write = (read & ~(LED0 | LED1 | LED2 | LED3)) | 0x00000000;
  printf("Will write 0x%08x to 0x%08x \n", write, (unsigned int) dev->base_addr[0] + LEDCTL);
  result = register_write(dev, LEDCTL, &write);
  sleep(2);

  for (int i = 0; i < 5; i++)
  {
    result = register_read(dev, LEDCTL, &read); 
    printf("0x%08x read from 0x%08x \n", read, (unsigned int)LEDCTL); 
    write = (read & ~(led[i])) | (led_start << (i*8));
    result = register_write(dev, LEDCTL, &write);
    sleep(1);
  }


  printf("Will write 0x%08x to 0x%08x \n", LEDCTL_orig, (unsigned int)dev->base_addr[0] + LEDCTL);
  result = register_write(dev, LEDCTL, &LEDCTL_orig);

  result = register_read(dev, GOOD_PACKETS, &read);
  printf("0x%08x read from 0x%08x \n", read, (unsigned int)(dev->base_addr[0] + GOOD_PACKETS));

  return 0;
}
