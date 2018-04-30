/* Max Schweitzer
   ECE 373
   Homework 2 - Char Driver
   This code implements a basic character driver,
   with open, close, read, and write functions,
   and the ability to set a module parameter (syscall_val)
   on the command line when using insmod.
   4/22/18
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
#define DEV_NAME "HW2-Max"

#define PE_LED_MASK 0x0
#define PE_REG_LEDS 0x0E00

static struct dev_info
{
  struct cdev cdev;

  // User defineable and interactable variable.
  int syscall_val;
} dev;

static dev_t dev_node;

// Starting value for syscall_val, gets applied during init.
static int test = 25;

module_param(test, int, S_IRUSR | S_IWUSR);

static int new_leds;
module_param(new_leds, int, 0);

static const struct pci_device_id pe_pci_tbl[] = {
  { PCI_DEVICE(0x8086, 0x100e), 0, 0, 0 },
  /* more device ids can be listed here */

  /* required last entry */
  {0, }
};

static char *pe_driver_name = "test_pci_driver";

struct pes {
  struct pci_dev *pdev;
  void *hw_addr;
} *pe;

static int open(struct inode *inode, struct file *file)
{ 
  printk(KERN_INFO "Opened instance.\n");

  return 0;
}

static int release(struct inode *inode, struct file *file)
{
  printk(KERN_INFO "Closed instance.\n");
  return 0;
}

// Simple read function, allowing the current value of syscall_val to be passed to a user.
static ssize_t kern_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
  ssize_t result;
  //char temp[20];
  uint32_t led_reg;
 

  printk(KERN_INFO "In read");
 
  // Make sure we stay within our bounds and don't go into an infinite loop.
  if (*offset >= sizeof(uint32_t))
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

  

  // Convert from int to char, so that it plays nicely with user space
  // assuming it's a char array.
  //snprintf(temp, sizeof(int),"%d", dev.syscall_val);

  led_reg = readl(pe->hw_addr + PE_REG_LEDS);
  //dev_info(&pdev->dev, "led_reg = 0x%02x\n", led_reg);

  // Send it to the user.
  if (copy_to_user(buf, &led_reg, sizeof(uint32_t)))
  {
    printk(KERN_ERR "!buf\n");
    result = -EFAULT;
    goto out;
  }

  result = sizeof(uint32_t);
  *offset += len;

  // If we made it here, we passed it back okay. Probably.
  printk(KERN_INFO "Passed user led_reg = 0x%08x\n", led_reg);

out:
  return result;
}


// Simple write function, allowing user space to modify syscall_val.
static ssize_t kern_write(struct file *file, const char __user *buf, size_t len, loff_t * offset)
{
  char *kern_buf;
  ssize_t result;
  int val;
  long *ptr_result;
  long store_val;
  ptr_result = &store_val;
  uint32_t to_write = 0;

  // Did the user give us something okay?
  if (!buf)
  {
    result = -EINVAL;
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
  if (copy_from_user(&to_write, buf, len))
  {
    result = -EFAULT;
    goto mem_out;
  }

  //printk(KERN_INFO "kern_buf: %s\n", kern_buf);

  result = len;

  // Convert from char into int and assign it to our value internally.
  //val = kstrtol(kern_buf, 16, ptr_result);
  
  //printk(KERN_INFO "*ptr_result: 0x%08x\n", *ptr_result);



  //to_write = *ptr_result;

  printk(KERN_INFO "to_write: 0x%08x\n", to_write);

  //dev.syscall_val = *ptr_result;
  // Since the test variable is our easy way to interface a parameter with syscall_val,
  // make sure that stays current.
  //test = *ptr_result;

  writel(to_write, pe->hw_addr + PE_REG_LEDS);
  
  *offset = 0;

  // We should be good then.
  printk(KERN_INFO "User wrote 0x%08x to 0x%08x\n", to_write, pe->hw_addr + PE_REG_LEDS);

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

static int pe_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
  //struct pes *pe;
  uint32_t ioremap_len;
  uint32_t led_reg;
  int bars, err;

  printk(KERN_INFO "In Probe! About to enable!\n");
  
  err = pci_enable_device_mem(pdev);
  if (err)
    return err;

  printk(KERN_INFO "About to DMA?!\n");

  /* set up for high or low dma */
  err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
  if (err) 
  {
    dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
    goto err_dma;
  }

  printk(KERN_INFO "About to BARS!\n");

  bars = pci_select_bars(pdev, IORESOURCE_MEM);

  printk(KERN_INFO "About to request region!\n");

  /* set up pci connections */
  err = pci_request_selected_regions(pdev, bars, pe_driver_name);
  if (err) {
    dev_info(&pdev->dev, "pci_request_selected_regions failed %d\n", err);
    goto err_pci_reg;
  }

  pci_set_master(pdev);
 
  printk(KERN_INFO "About to alloc!\n");

  pe = kzalloc(sizeof(*pe), GFP_KERNEL);
  if (!pe) {
    err = -ENOMEM;
    goto err_pe_alloc;
  }
  pe->pdev = pdev;
  pci_set_drvdata(pdev, pe);
 
  printk(KERN_INFO "IO stuff!\n");

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

  
 
  led_reg = readl(pe->hw_addr + PE_REG_LEDS);
  dev_info(&pdev->dev, "led_reg = 0x%02x\n", led_reg); 

/*
  if (new_leds) {
    led_reg = (led_reg & ~PE_REG_LEDS) | (new_leds & PE_LED_MASK);
    writeb(led_reg, (pe->hw_addr + PE_REG_LEDS));
    dev_info(&pdev->dev, "new led_reg = 0x%02x\n", led_reg);
  }
*/
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
  .name = "pci_example",
  .id_table = pe_pci_tbl,
  .probe = pe_probe,
  .remove = pe_remove
};

static int __init kern_init(void)
{
  int result = 0;

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
    dev.syscall_val = test;
  }

  result = pci_register_driver(&pe_driver); 
  if (result)
  {
    printk(KERN_INFO "Bad?\n");
  }

  return result;
}


static void __exit kern_exit(void)
{
  // Clean up what we used, be a good module.
  cdev_del(&dev.cdev);
  unregister_chrdev_region(dev_node, DEV_COUNT);
  pci_unregister_driver(&pe_driver);
  printk(KERN_INFO "Unloaded.");
}

MODULE_AUTHOR("Max Schweitzer");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(kern_init);
module_exit(kern_exit);
