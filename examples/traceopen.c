#include <linux/module.h>
#include <linux/fcntl.h>
#include <asm/unistd_32.h> /* __NR_open */

MODULE_LICENSE ("GPL");

extern unsigned sys_call_table[];

asmlinkage long (*original_sys_open)(const char __user *, int, umode_t) = NULL;

/* new syscall */
asmlinkage long
new_sys_open(const char __user *name, int flags, umode_t mode)
{
  printk ("file=%s\n", &name);
  return ( original_sys_open(name, flags, mode) );
}

#define GPF_DISABLE write_cr0(read_cr0() & (~ 0x10000)) /* Disable read-only protection */
#define GPF_ENABLE write_cr0(read_cr0() | 0x10000) /* Enable read-only protection */

static int __init
traceopen_init (void)
{

  original_sys_open = (asmlinkage long (*)(const char __user *, int, umode_t))(sys_call_table[__NR_open]);

  GPF_DISABLE; /* Disable read-only protection (sys_call_table is on a read-only page )*/
  sys_call_table[__NR_open] = (unsigned) new_sys_open;
  GPF_ENABLE; /* Enable read-only protection */

  printk (KERN_NOTICE "Open captured");
  printk (KERN_INFO "Correctly installed\n Compiled at %s %s\n", __DATE__,
          __TIME__);
  return (0);
}

static void __exit
traceopen_exit (void)
{
  /* Restore previous state */
  if (sys_call_table[__NR_open] == (unsigned) new_sys_open) {
    GPF_DISABLE; /* Disable read-only protection (sys_call_table is on a read-only page )*/
    sys_call_table[__NR_open] = (unsigned) original_sys_open;
    GPF_ENABLE; /* Enable read-only protection */

    printk (KERN_NOTICE "Open restored\n");
  }
  else 
    printk (KERN_ALERT "Unexpected error\n");
}

module_init (traceopen_init);
module_exit (traceopen_exit);
