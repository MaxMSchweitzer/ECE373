#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("Dual BSD/GPL");

static int __init hello_init(void)
{
  printk(KERN_INFO "This is a hello message!\n");
  return 0;
}

static void __exit hello_exit(void)
{
  printk(KERN_INFO "Alas, poor kernel.\n");
  return;
}

module_init(hello_init);
module_exit(hello_exit);
