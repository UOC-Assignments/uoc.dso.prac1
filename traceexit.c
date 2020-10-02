/*
 ################################################################################
 #                                                                              #
 #                       UOC - Open University of Catalonia                     #
 #                                                                              #
 ################################################################################

 ################################################################################
 #                                                                              #
 #                         OPERATIVE SYSTEMS DESIGN (DSO)                       #
 #                             PRACTICAL ASSIGNMENT 1                           #
 #                                                                              #
 #                           STUDENT: Jordi Bericat Ruz                         #
 #                              TERM: Autumn 2020/21                            #
 #                                                                              #
 #                            FILE 1 OF 1: traceexit.c                          #
 #                                                                              #
 #                                  Version 1.1                                 #
 #                                                                              #
 ################################################################################

 ################################################################################
 #                                                                              #
 #  DESCRIPTION:                                                                #
 #                                                                              #
 #  Implement a Linux Kernel module that keeps track of every exit system call  #
 #  executed and also allows access to a summary from user space interfacing    #
 #  via procfs.                                                                 #
 #                                                                              #
 #                                                                              #
 #  IMPLEMENTATION STRATEGY:                                                    #
 #                                                                              #
 #  Exit System calls monitoring                                                #
 #                                                                              #
 #  In regards to the exit syscall codes monitoring, we only have to call the   #
 #  low-level assembler code which performs the sys_exit call associated tasks  #
 #  to a new function (new_sys_exit) and then link it to the system calls       #
 #  vector (sys_call_table). Summing-up: this way we "bypass" the original      #
 #  sys_exit call (original_sys_exit) so we can modify its behaviour (in our    #
 #  case, to keep track of every exit system call executed once the kernel      #
 #  module becomes enabled (insmod traceexit.ko).                               #
 #                                                                              #
 #  Procfs Interface implementation                                             #
 #                                                                              #
 #  The goal is to implement a communication interface between the user memory  #
 #  space and the one associated with the kernel module, so an user would be    #
 #  able to read from the shell (user memory space) the data stored in the      #
 #  physical memory space reserved to the kernel (exit syscalls counters) via   #
 #  the procfs "virtual" interface -> cat "/proc/traceexit. Writing operations  #
 #  will be performed from within the module (kernel memory space), so a        #
 #  writing interface from user to kernel space won't be needed.                #
 #                                                                              #
 #                                                                              #
 #  INPUT:                                                                      #
 #                                                                              #
 #  N/A                                                                         #
 #                                                                              #
 #                                                                              #
 #  OUTPUT:                                                                     #
 #                                                                              #
 #  Summary at /proc/traceexit showing the amount of times every exit code      #
 #  that has been invocated since the kernel module activation.                 #
 #                                                                              #
 #                                                                              #
 #  USAGE:                                                                      #
 #                                                                              #
 #  See examples/usage.txt                                                      #
 #                                                                              #
 ################################################################################


 _.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(_.~"(


 ################################################################################
 #                                                                              #
 #                       1. INCLUDES, CONSTANTS & GLOBALS                       #
 #                                                                              #
 ################################################################################ */

#include <linux/module.h> // We're doing kernel module 	work 
#include <linux/fcntl.h> // (?) 
#include <linux/init.h>  // (?) 
#include <linux/kernel.h> // We're doing kernel work
#include <linux/proc_fs.h>	// Necessary because we use the proc fs 
#include <linux/uaccess.h>	// for copy_from_user (move data from user to kernel) 
#include <asm/unistd_32.h> // syscall asm implementations (__NR_exit, etc) 

MODULE_LICENSE ("GPL");

#define BUFSIZE  100
#define PROC_FILE "traceexit" // more efficient alternative(?) -> const char *HELLO2 = "Howdy";
#define GPF_DISABLE write_cr0(read_cr0() & (~ 0x10000)) // Disable read-only protection 
#define GPF_ENABLE write_cr0(read_cr0() | 0x10000) // Enable read-only protection 

static int exit_codes_count[999];

/*

 ################################################################################
 #                                                                              #
 #                       2. PROC FS OPERATIONS IMPLEMENTATION                   #
 #                                                                              #
 ################################################################################ */ 


/* Adaptació del següent codi original: */
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Liran B.H");

//######## 2.1 create the proc file

static struct proc_dir_entry *ent;

//######## 2.2 Implement the read from user space procfs operation
 
static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	char buf[BUFSIZE];
	char str[BUFSIZE*20];
	char tmp_str[20];
	int len=0;
	int i;
	if(*ppos > 0 || count < BUFSIZE)
		return 0;

	for (i=0;i<999;i++){
		if (exit_codes_count[i]!=0){
			sprintf(tmp_str,"code %d = %d\n",i,exit_codes_count[i]);
			strcat(str,tmp_str);
		}
	}
	
   len += sprintf(buf,"%s",str); /* cal fer-ho així?*/
	if(copy_to_user(ubuf,buf,len)) /* potser posant str en comptes de buf puc eliminar linia anterior */
		return -EFAULT;
	*ppos = len;
	return len;
}

//######## 2.3 Set our custom read operation as default to access the module's
//########     process memory space	

static struct file_operations myops = 
{
	.owner = THIS_MODULE,
	.read = myread,
};

/*
 ################################################################################
 #                                                                              #
 #                       3. CUSTOM SYS EXIT IMPLEMENTATION                      #
 #                                                                              #
 ################################################################################ */

extern unsigned sys_call_table[];

asmlinkage long (*original_sys_exit)(int) = NULL;

/************* 2.  ************/

asmlinkage long
new_sys_exit(int exit_code) 
{
  exit_codes_count[exit_code]++;
  printk ("exit code %d captured at /proc/traceexit\n", exit_code);
  return original_sys_exit(exit_code);
}

/*
 ################################################################################
 #                                                                              #
 #                      			  4. MAIN                      				#
 #                                                                              #
 ################################################################################ */

static int __init
traceexit_init (void)
{

  int z;

  /* create the /proc file */
  ent=proc_create(PROC_FILE,0660,NULL,&myops);

  /* Enable syscalls */
  original_sys_exit = (asmlinkage long (*)(int))(sys_call_table[__NR_exit]);
  
  GPF_DISABLE; /* Disable read-only protection (sys_call_table is on a read-only page )*/
  sys_call_table[__NR_exit] = (unsigned) new_sys_exit;
  GPF_ENABLE; /* Enable read-only protection */

  /* init counters */

  for (z=0;z<999;z++){
	exit_codes_count[z]=0;
  }
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


/***************************************************************
****************************************************************
****  					  						  			****
****  				        z. BIBLIOGRAPHY	  				****
****  					  						  			****
****************************************************************
****************************************************************
****  					  						  			
**** 	 1. http://asm.sourceforge.net/syscall.html#p31 	
****  	 2. https://devarea.com/linux-kernel-development-creating-a-proc-file-and-interfacing-with-user-space/#.X3Kd42j7TD4		
****	 2.1. Note : to implement more complex proc entries , use the seq_file wrapper
****  	 3. https://www.linuxjournal.com/article/8110
****	 4. https://www.linuxtopia.org/online_books/Linux_Kernel_Module_Programming_Guide/x714.html				  						  	
****
***************************************************************
***************************************************************/