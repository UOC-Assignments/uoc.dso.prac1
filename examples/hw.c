/*
 * Hello World Module
 */

#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hello world!");

static int __init hw_init(void)
{
	printk(KERN_INFO "Hello world!\n");
	return (0);
}

static void __exit hw_exit(void)
{
	printk(KERN_INFO "Bye world!\n");
}

module_init(hw_init);
module_exit(hw_exit);
