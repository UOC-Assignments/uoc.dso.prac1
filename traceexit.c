#include <linux/module.h>
#include <linux/fcntl.h> /* (?) */
#include <linux/init.h>  /* (?) */
#include <linux/moduleparam.h> /* (?) */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
#include <linux/uaccess.h>	/* for copy_from_user */
#include <asm/unistd_32.h> /* __NR_exit */

#define BUFSIZE  100
#define PROC_FILE "traceexit" //const char *HELLO2 = "Howdy";
/***************************************************************
****************************************************************
****  					  						  			****
****  				        DOCUMENTATION	  				****
****  					  						  			****
****************************************************************
****************************************************************
****  					  						  			****
**** 	 1. http://asm.sourceforge.net/syscall.html#p31 	
****  	 2. https://devarea.com/linux-kernel-development-creating-a-proc-file-and-interfacing-with-user-space/#.X3Kd42j7TD4		
****	 2.1. Note : to implement more complex proc entries , use the seq_file wrapper
****  	 3. https://www.linuxjournal.com/article/8110
****	 4. https://www.linuxtopia.org/online_books/Linux_Kernel_Module_Programming_Guide/x714.html				  						  	
****
***************************************************************
***************************************************************/

MODULE_LICENSE ("GPL");

/************ 1. PROC FS OPERATIONS IMPLEMENTATION ************/

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Liran B.H");
 
/*create the proc file*/

static int exit_codes_count[1000]; 


static struct proc_dir_entry *ent;

 
static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	char buf[BUFSIZE];
	int len=0;
	if(*ppos > 0 || count < BUFSIZE)
		return 0;
	len += sprintf(buf,"exit code = %d\n",exit_codes_count[1]);
	//len += sprintf(buf + len,"exit count = %d\n",2);
	
	if(copy_to_user(ubuf,buf,len))
		return -EFAULT;
	*ppos = len;
	return len;
}
 
static struct file_operations myops = 
{
	.owner = THIS_MODULE,
	.read = myread,
	//.write = mywrite,
};

/************************** END (1) *************************/

extern unsigned sys_call_table[];

asmlinkage long (*original_sys_exit)(int) = NULL;

/************* 2. CUSTOM SYS EXIT IMPLEMENTATION ************/
asmlinkage long
new_sys_exit(int exit_code) 
{
  exit_codes_count[1]=exit_code;
  printk ("exit code %d captured at /proc/traceexit\n", exit_code);
  return original_sys_exit(exit_code);
}

/************************** END (2) *************************/

/************************* 3. MAIN **************************/

#define GPF_DISABLE write_cr0(read_cr0() & (~ 0x10000)) /* Disable read-only protection */
#define GPF_ENABLE write_cr0(read_cr0() | 0x10000) /* Enable read-only protection */

static int __init
traceexit_init (void)
{

  /* create the /proc file */
  ent=proc_create(PROC_FILE,0660,NULL,&myops);

  /* Enable syscalls */
  original_sys_exit = (asmlinkage long (*)(int))(sys_call_table[__NR_exit]);
  
  GPF_DISABLE; /* Disable read-only protection (sys_call_table is on a read-only page )*/
  sys_call_table[__NR_exit] = (unsigned) new_sys_exit;
  GPF_ENABLE; /* Enable read-only protection */

  printk (KERN_NOTICE "exit syscall captured");
  printk (KERN_INFO "Correctly installed\n Compiled at %s %s\n", __DATE__,
          __TIME__);
  return (0);
}

static void __exit
traceexit_exit (void)
{
  /* Remove procfs entry */
  proc_remove(ent);

  /* Restore previous state */
  if (sys_call_table[__NR_exit] == (unsigned) new_sys_exit) {
    GPF_DISABLE; /* Disable read-only protection (sys_call_table is on a read-only page )*/
    sys_call_table[__NR_exit] = (unsigned) original_sys_exit;
    GPF_ENABLE; /* Enable read-only protection */

    printk (KERN_NOTICE "exit syscall restored\n");
  }
  else 
    printk (KERN_ALERT "Unexpected error\n");
}

module_init (traceexit_init);
module_exit (traceexit_exit);