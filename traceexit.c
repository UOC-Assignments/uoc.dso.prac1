/*###############################################################################
 #                                                                              #
 #                       UOC - Open University of Catalonia                     #
 #                                                                              #
 ################################################################################

 ################################################################################
 #                                                                              #
 #                         OPERATING SYSTEMS DESIGN (DSO)                       #
 #                             PRACTICAL ASSIGNMENT #1                          #
 #                                                                              #
 #                        STUDENT: Jordi Bericat Ruz                            #
 #                           TERM: Autumn 2020/21                               #
 #                       GIT REPO: UOC-Assignments/uoc.dso.prac1"               #
 #                    FILE 1 OF 1: traceexit.c                                  #
 #                        VERSION: 1.2                                          #
 #                                                                              #
 ################################################################################

 ################################################################################
 #                                                                              #
 #  DESCRIPTION:                                                                #
 #                                                                              #
 #  Implement a Linux Kernel module that keeps track of every exit system call  #
 #  executed and also allows access to a summary from user space (interfacing   #
 #  via procfs).                                                                #
 #                                                                              #
 #                                                                              #
 #  IMPLEMENTATION STRATEGY:                                                    #
 #                                                                              #
 #  Exit System calls monitoring                                                #
 #                                                                              #
 #  In regards to the exit syscall codes monitoring, we only have to call the   #
 #  low-level assembler code which performs the sys_exit call associated tasks  #
 #  from within a new function (new_sys_exit) and then link it to the system    #
 #  calls vector (sys_call_table). Summing-up: this way we bypass the original  #
 #  sys_exit call (original_sys_exit) so we can modify its behaviour (in our    #
 #  case, to keep track of every exit system call executed once the kernel      #
 #  module becomes enabled via insmod traceexit.ko).                            #
 #                                                                              #
 #  Procfs Interface implementation                                             #
 #                                                                              #
 #  The goal is to implement a communication interface between the user memory  #
 #  space and the one associated with the kernel module, so an user will be     #
 #  able to read from the shell (user memory space) the data stored in the      #
 #  physical memory space reserved to the kernel (exit syscalls counters) via   #
 #  the procfs "virtual" interface -> cat /proc/traceexit. Writing operations   #
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
 ##############################################################################*/

#include <linux/module.h> // We're doing kernel module work 
#include <linux/fcntl.h> // File control options -> 5. BIBLIOGRAPHY (7)
#include <linux/init.h>  // Needed for the __INIT and __EXIT Macros -> 5. BIBLIOGRAPHY (8)
#include <linux/kernel.h> // We're doing kernel work
#include <linux/proc_fs.h> // Necessary because we use the proc fs 
#include <linux/uaccess.h> // for copy_from_user (move data from user's to kernel's spaces) 
#include <asm/unistd_32.h> // system calls asm implementations (__NR_exit, etc) 

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Jordi B.R.");
MODULE_DESCRIPTION ("OPERATING SYSTEMS DESIGN - PRACTICAL ASSIGNMENT #1 - Open University of Catalonia");

#define MAX_EXIT_CODES 256 // There only are 256 exit syscall code numbers (0-255).
#define BUFSIZE  450 // Stack size = 1024Kb. Since we declare 2 buffers in the same function 
                     // stack (myread), we can pnly assign (almost) half the stack size to 
                     // each buffer (we leave a bit of space for other vars, etc)
#define PROC_FILE "traceexit" 
#define GPF_DISABLE write_cr0(read_cr0() & (~ 0x10000)) // Disable read-only protection 
#define GPF_ENABLE write_cr0(read_cr0() | 0x10000) // Enable read-only protection 

extern unsigned sys_call_table[];
static int exit_codes_count[MAX_EXIT_CODES]; 
static struct proc_dir_entry *ent;

/*###############################################################################
 #                                                                              #
 #                       2. PROC FS OPERATIONS IMPLEMENTATION                   #
 #                                                                              #
 ##############################################################################*/ 

// ######## DISCLAIMER: The code below is an adaptation from the ones found at:
// ########             5. BIBLIOGRAPHY -> (1.3), (2)  

// ######## 2.1 Implement the read from user space procfs operation
 
static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	char buf[BUFSIZE];
	char str[BUFSIZE];
	char tmp_str[20];
	int len=0;
	int i;
	if(*ppos > 0 || count < BUFSIZE)
		return 0;

	for (i=0;i<MAX_EXIT_CODES;i++){
		if (exit_codes_count[i]!=0){
			sprintf(tmp_str,"code %d = %d\n",i,exit_codes_count[i]);
			strcat(str,tmp_str);
		}
	}
	
   len += sprintf(buf,"%s",str); 
	if(copy_to_user(ubuf,buf,len)) 
		return -EFAULT;
	*ppos = len;
	return len;
}

// ######## 2.2 Set our custom read operation as default to access the module's
// ########     process memory space	

static struct file_operations myops = 
{
	.owner = THIS_MODULE,
	.read = myread,
};

/*###############################################################################
 #                                                                              #
 #                       3. CUSTOM SYS EXIT IMPLEMENTATION                      #
 #                                                                              #
 ##############################################################################*/

// ######## DISCLAIMER: The code below is an adaptation from the ones found at:
// ########             5. BIBLIOGRAPHY -> (1.2)  

// ######## 3.1 - We declare a pointer to the function (routine), which will be used
// ########       to point to the original sys_exit routine asm instructions 

asmlinkage long (*original_sys_exit)(int) = NULL;

// ######## 3.2 - Now, in order to bypass the original sysexit routine and be able to
// ########       modify its behaviour we should create a new function which will 
// ########       implement the sysexit codes monitoring, as well as executing
// ########       the original instructions related to the original exit system call:

asmlinkage long new_sys_exit(int exit_code) 
{
// ######## 3.3 - We increment by 1 the value contained in the "exit_code" 
// ########       position of exit_codes_count[], hence every element on the array 
// ########       will be storing one (and only one) exit code's counter; that is,
// ########       the one corresponding to the index number:   

  exit_codes_count[exit_code]++;

  printk ("exit code %d captured at /proc/traceexit\n", exit_code);

// ######## 3.4 - Once we "hijacked" our code into the new sys_exit call (so we can monitor
// ########       the system exit events), we should call the function that actually invoque 
// ########       the original sysexit routine instructions:
     
  return original_sys_exit(exit_code);
}

/*###############################################################################
 #                                                                              #
 #                                 4. INIT / EXIT                               #
 #                                                                              #
 ##############################################################################*/

// ######## DISCLAIMER: The code below is an adaptation from the ones found at:
// ########             5. BIBLIOGRAPHY -> (1.2), (1.3)  

// ######## 4.1 - Module INIT Function

static int __init traceexit_init (void)
{
  int z; //We move it here because this -> BIBLIOGRAPHY (6)

// ######## 4.1.1 - Create the proc file

  ent=proc_create(PROC_FILE,0660,NULL,&myops);

// ######## 4.1.2 - Link sys_exit call original asm code to original_sys_exit (pointer to function)
  
  original_sys_exit = (asmlinkage long (*)(int))(sys_call_table[__NR_exit]);
  
// ######## 4.1.3 - Disable read-only protection (sys_call_table is on a read-only page )*/
  
  GPF_DISABLE; 

// ######## 4.1.4 - Link our custom definition of sys_exit (new_sys_exit) in the 
// ########         __NR_Exit entry of the system calls vector (sys_call_table).
 
  sys_call_table[__NR_exit] = (unsigned) new_sys_exit;

// ######## 4.1.5 - Enable read-only protection

  GPF_ENABLE; 

// ######## 4.1.6 - Init sys_exit counters

  for (z=0;z<MAX_EXIT_CODES;z++){
	exit_codes_count[z]=0;
  }

  printk (KERN_NOTICE "exit syscall captured");
  printk (KERN_INFO "Correctly installed\n Compiled at %s %s\n", __DATE__,
          __TIME__);
  return (0);
}

// ######## 4.2 - Module INIT Function

static void __exit traceexit_exit (void)
{

// ######## 4.2.1 - Remove procfs entry

  proc_remove(ent);

// ######## 4.2.2 - Restore previous state

  if (sys_call_table[__NR_exit] == (unsigned) new_sys_exit) {

// ######## 4.2.3 - Disable read-only protection (sys_call_table is on a read-only page )

    GPF_DISABLE; 

// ######## 4.2.4 - Link back the original definition of sys_exit 
// ########         (original_sys_exit) in the __NR_Exit entry of the system calls
// ########         vector (sys_call_table).

    sys_call_table[__NR_exit] = (unsigned) original_sys_exit;

// ######## 4.2.5 - Enable read-only protection 

    GPF_ENABLE; 

    printk (KERN_NOTICE "exit syscall restored\n");
  }
  else 
    printk (KERN_ALERT "Unexpected error\n");
}

module_init (traceexit_init);
module_exit (traceexit_exit);

/*###############################################################################
 #                                                                              #
 #                            5. BIBLIOGRAPHY / SOURCES                         #
 #                                                                              #
 ################################################################################ 
 					  						  			
     1. Examples provided into the DSO Practical Assignment #1:
	 1.1. examples/hw.c (example1)
	 1.2. examples/procdemo.c (example2)
	 1.3. examples/traceopen.c (example3)
     2. https://devarea.com/linux-kernel-development-creating-a-proc-file-and-interfacing-with-user-space/#.X3Kd42j7TD4		
     3. https://www.linuxjournal.com/article/8110
     4. https://www.linuxtopia.org/online_books/Linux_Kernel_Module_Programming_Guide/x714.html
     5. http://asm.sourceforge.net/syscall.html#p31 
     6. https://stackoverflow.com/questions/13291353/iso-c90-forbids-mixed-declarations-and-code-in-c
     7. https://man7.org/linux/man-pages/man0/fcntl.h.0p.html
     8. http://www.iitk.ac.in/LDP/LDP/lkmpg/2.4/html/x281.htm			  						  	

*/