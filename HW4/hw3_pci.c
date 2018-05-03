/* Max Schweitzer
   ECE 373
   Homework 3 - PCI LED Driver
   This code implements a basic pci driver,
   with open, close, read, and write functions,
   and the ability to blink an LED for the
   Intel 82540EM Gigabit Ethernet Controller.
   PCI functions implemented are probe and 
   remove.
   4/30/18
*/   

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>

#define DEV_COUNT 2
#define DEV_NAME "HW4_pci"

#define PE_REG_LEDS 0x00E00
#define LED0_MASK 0x0000000F


#define DEVICE_NAME "pci_char"
//#define CLASS_NAME "pci"

//static float per_sec;

static struct class *my_pci = NULL;

static struct timer_list my_timer;

static const struct pci_device_id pe_pci_tbl[] = {
  { PCI_DEVICE(0x8086, 0x100e), 0, 0, 0 },

  {0, }
};

static char *pe_driver_name = "test_pci_driver";

struct pes {
  struct pci_dev *pdev;
  void *hw_addr;
} *pe;

static struct dev_info
{
  struct cdev cdev;

  // User defineable and interactable variable.
  int blink_rate;
} dev;

static dev_t dev_node;

// Starting value for syscall_val, gets applied during init.
static int blink = 2;

module_param(blink, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

#define LED_ON  0x0000000e
#define LED_OFF 0x0000000f

static uint32_t toRead = 0;
static uint32_t toSend = 0;
static bool on = true;

void my_timer_callback(unsigned long data)
{
  printk(KERN_INFO "Callback!\n");

  if (blink > 0)
  {
    mod_timer(&my_timer, jiffies + msecs_to_jiffies((1000 / blink) / 2));
    printk(KERN_INFO "Blink = %d\n", ((1000 / blink)) / 2);
  
  
    toRead = readl(pe->hw_addr + PE_REG_LEDS);
    toRead = toRead & (~LED0_MASK);
    if (on)
    {
      toSend = toRead | LED_ON;
    }
    else
    {
      toSend = toRead | LED_OFF;
    }

    writel(toSend, pe->hw_addr + PE_REG_LEDS);
    on = !on;
  }
}


static int open(struct inode *inode, struct file *file)
{ 
  printk(KERN_INFO "Opened instance.\n");
  //per_sec = 1/blink;
  // TODO MAX maybe a flag here so it doesn't mod timer
  // anytime it's opened?
  if (blink > 0)
  {
    mod_timer(&my_timer, jiffies + msecs_to_jiffies((1000 / blink) / 2));
  }
  return 0;
}

static int release(struct inode *inode, struct file *file)
{
  printk(KERN_INFO "Closed instance.\n");
  return 0;
}

// Simple read function, allowing the current value of LEDCTL register to be passed to user.
static ssize_t kern_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
  ssize_t result;
  uint32_t led_reg;

  char temp[20];
 
  // Make sure we stay within our bounds and don't go into an infinite loop.
  if (*offset >= sizeof(int))
  {
    *offset = 0;
    return 0;
  }

  // Make sure the user actually gave us something valid to play with.
  if (!buf)
  {
    printk(KERN_ERR "!buf\n");
    result = -EINVAL;
    goto out;
  }


  //int result = kstrtol(

  snprintf(temp, sizeof(uint32_t),"%d\n", dev.blink_rate);


  // Send it to the user.
  if (copy_to_user(buf, temp, strlen(temp)))
  {
    printk(KERN_ERR "Copy to!\n");
    result = -EFAULT;
    goto out;
  }

  result = sizeof(uint32_t);
  *offset += len;

  // If we made it here, we passed it back okay. Probably.
  printk(KERN_INFO "Passed user %d\n", dev.blink_rate);

out:
  return result;
}


// Simple write function, allowing user space to modify LEDCTL value.
static ssize_t kern_write(struct file *file, const char __user *buf, size_t len, loff_t * offset)
{

  char *kern_buf;
  ssize_t result;
  long *ptr_result;
  long store_val;
  long to_write = 0;
  ptr_result = &store_val;
  int val;
  char **dest;

  // Did the user give us something okay?
  if (!buf)
  {
    printk(KERN_ERR "!buf\n");
    result = -EINVAL;
    goto out;
  }

  // Make sure it's only 32 bits.
  if (len > sizeof(uint32_t))
  {
    printk(KERN_ERR "Size!\n");
    result = -EFAULT;
    goto out;
  }

  // Get a chunk of memory.
  kern_buf = kmalloc(len, GFP_KERNEL);

  if (!kern_buf)
  {
    result = -ENOMEM;
    goto mem_out;
  }

  // Take in the user value.
  if (copy_from_user(kern_buf, buf, len))
  {
    printk(KERN_ERR "Copy from error!\n");
    result = -EFAULT;
    goto out;
  }

  kern_buf[len-1] = '\0';

  val = strlen(kern_buf);

  printk(KERN_INFO "strlen(kern_buf) %d\n", val);
  printk(KERN_INFO "kern_buf %s\n", kern_buf);
  val = kstrtol(kern_buf, 10, ptr_result);
 
  //to_write = simple_strtol(kern_buf, dest, 10);

  //val = sscanf(buf, "%d", &to_write);

  printk(KERN_INFO "*ptr_result %d\n", *ptr_result); 
  if (val)  
  {
    printk(KERN_ERR "val %d\n", val); 
    printk(KERN_ERR "kstrtol!\n");
    result = -EFAULT;
    goto out;
  }

  if (*ptr_result < 0)
  {

    printk(KERN_ERR "Negative value, not allowed!\n");
    result = -EINVAL;
    goto out;
  }

  result = len;

  //writel(to_write, pe->hw_addr + PE_REG_LEDS);
  dev.blink_rate = *ptr_result;  
  blink = *ptr_result;

  *offset = 0;

  // We should be good then.
  printk(KERN_INFO "User wrote %d\n", to_write);

mem_out:
  kfree(kern_buf);
out:
  return result;
}

static struct file_operations dev_fops = {
  .owner = THIS_MODULE,
  .open = open,
  .read = kern_read,
  .write = kern_write,
  .release = release
};

// TODO clean up comments, maybe rename some stuff
static int pe_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
  uint32_t ioremap_len;
  int bars, err;

  err = pci_enable_device_mem(pdev);
  if (err)
    return err;

  /* set up for high or low dma */
  err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
  if (err) 
  {
    dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
    goto err_dma;
  }

  bars = pci_select_bars(pdev, IORESOURCE_MEM);

  /* set up pci connections */
  err = pci_request_selected_regions(pdev, bars, pe_driver_name);
  if (err) {
    dev_info(&pdev->dev, "pci_request_selected_regions failed %d\n", err);
    goto err_pci_reg;
  }

  pci_set_master(pdev);
 
  pe = kzalloc(sizeof(*pe), GFP_KERNEL);
  if (!pe) {
    err = -ENOMEM;
    goto err_pe_alloc;
  }
  pe->pdev = pdev;
  pci_set_drvdata(pdev, pe);

  /* map device memory */
  ioremap_len = min_t(int, pci_resource_len(pdev, 0), 1024);
  pe->hw_addr = ioremap(pci_resource_start(pdev, 0), ioremap_len);
  if (!pe->hw_addr) {
    err = -EIO;
    dev_info(&pdev->dev, "ioremap(0x%04x, 0x%04x) failed: 0x%x\n",
			 (unsigned int)pci_resource_start(pdev, 0),
			 (unsigned int)pci_resource_len(pdev, 0), err);
    goto err_ioremap;
  }
  
  return 0;

err_ioremap:
  kfree(pe);
err_pe_alloc:
  pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
err_pci_reg:
err_dma:
  pci_disable_device(pdev);
  return err;
}

static void pe_remove(struct pci_dev *pdev)
{
  struct pes *pe = pci_get_drvdata(pdev);

  /* unmap device from memory */
  iounmap(pe->hw_addr);

  /* free any allocated memory */
  kfree(pe);

  pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
  pci_disable_device(pdev);
}

static struct pci_driver pe_driver = {
  .name = DEV_NAME,
  .id_table = pe_pci_tbl,
  .probe = pe_probe,
  .remove = pe_remove
};

static int __init kern_init(void)
{
  int result = 0;

  my_pci = class_create(THIS_MODULE, DEV_NAME);

  // Dynamically allocate the device.
  result = alloc_chrdev_region(&dev_node, 0, DEV_COUNT, DEV_NAME);

  if (result)
  {
    printk(KERN_ERR "Allocation failed!");
    return result;
  }

  printk(KERN_INFO "Allocated device successfuly at major: %d\n", MAJOR(dev_node));
  
  // Do our other initialization procedures, make sure they work out.
  cdev_init(&dev.cdev, &dev_fops);
  dev.cdev.owner = THIS_MODULE;

  if (cdev_add(&dev.cdev, dev_node, DEV_COUNT))
  {
    printk(KERN_ERR "Failed to add!\n");
    unregister_chrdev_region(dev_node, DEV_COUNT);
  }
  else
  {
    // Add worked, so let's initialize the value.
    dev.blink_rate = blink;
  }

  device_create(my_pci, NULL ,dev_node, NULL, DEV_NAME);

  result = pci_register_driver(&pe_driver); 
  if (result)
  {
    printk(KERN_INFO "Bad?\n");
  }

  setup_timer(&my_timer, my_timer_callback, 0);

  printk(KERN_INFO "Device %s should be created and loaded?!", DEV_NAME);

  return result;
}


static void __exit kern_exit(void)
{
  // Clean up what we used, be a good module.
  cdev_del(&dev.cdev);
  unregister_chrdev_region(dev_node, DEV_COUNT);
  device_destroy(my_pci, dev_node);
  class_destroy(my_pci);
  pci_unregister_driver(&pe_driver);
  del_timer_sync(&my_timer);
  printk(KERN_INFO "Unloaded.");
}

MODULE_AUTHOR("Max Schweitzer");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(kern_init);
module_exit(kern_exit);
