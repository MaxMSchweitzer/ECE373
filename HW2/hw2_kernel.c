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

#define DEV_COUNT 2
#define DEV_NAME "HW2-Max"

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
  char temp[20];
 

  printk(KERN_INFO "In read");
 
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

  

  // Convert from int to char, so that it plays nicely with user space
  // assuming it's a char array.
  snprintf(temp, sizeof(int),"%d", dev.syscall_val);

  // Send it to the user.
  if (copy_to_user(buf, temp, strlen(temp)))
  {
    printk(KERN_ERR "!buf\n");
    result = -EFAULT;
    goto out;
  }

  result = sizeof(int);
  *offset += len;

  // If we made it here, we passed it back okay. Probably.
  printk(KERN_INFO "User was passed %d\n", dev.syscall_val);

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
  if (copy_from_user(kern_buf, buf, len))
  {
    result = -EFAULT;
    goto mem_out;
  }

  result = len;

  // Convert from char into int and assign it to our value internally.
  val = kstrtol(kern_buf, 10, ptr_result);
  dev.syscall_val = *ptr_result;
  // Since the test variable is our easy way to interface a parameter with syscall_val,
  // make sure that stays current.
  test = *ptr_result;

  *offset = 0;

  // We should be good then.
  printk(KERN_INFO "User wrote %d\n", dev.syscall_val);

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

  return result;
}


static void __exit kern_exit(void)
{
  // Clean up what we used, be a good module.
  cdev_del(&dev.cdev);
  unregister_chrdev_region(dev_node, DEV_COUNT);
  printk(KERN_INFO "Unloaded.");
}

MODULE_AUTHOR("Max Schweitzer");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(kern_init);
module_exit(kern_exit);
