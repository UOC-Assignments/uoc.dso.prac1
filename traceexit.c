#include<linux/module.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/sched.h>
#include<linux/uaccess.h>
#include<linux/fs.h>
#include<linux/seq_file.h>
#include<linux/slab.h>

#define BUFSIZE  100
#define PROC_FILE "traceexit" //const char *HELLO2 = "Howdy";
#define GPF_DISABLE write_cr0(read_cr0() & (~ 0x10000)) /* Disable read-only protection */
#define GPF_ENABLE write_cr0(read_cr0() | 0x10000) /* Enable read-only protection */

/******* 1. PROC FS OPERATIONS IMPLEMENTATION **********/

static char *str = NULL;

static int my_proc_show(struct seq_file *m,void *v){
	//seq_printf(m,"hello from proc file\n");
	seq_printf(m,"%s\n",str);
	return 0;
}

static ssize_t my_proc_write(struct file* file,const char __user *buffer,size_t count,loff_t *f_pos){
	char *tmp = kzalloc((count+1),GFP_KERNEL);
	if(!tmp)return -ENOMEM;
	if(copy_from_user(tmp,buffer,count)){
		kfree(tmp);
		return EFAULT;
	}
	kfree(str);
	str=tmp;
	return count;
}

static int my_proc_open(struct inode *inode,struct file *file){
	return single_open(file,my_proc_show,NULL);
}

/* Comment this to disable user interfacing from shell */
static struct file_operations my_fops={
	.owner = THIS_MODULE,
	.open = my_proc_open,
	.release = single_release,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = my_proc_write
};

/*********************** END (1) ************************/

/******* 2. CUSTOM EXIT SYSCALL IMPLEMENTATION **********/

extern unsigned sys_call_table[];

asmlinkage long (*original_sys_exit)(int) = NULL;

/* new syscall */
asmlinkage long
new_sys_exit(int exit_code) 
{
  /*TO-DO: Read from /proc/traceexit in order to check if "code" exists. If so,
  we read its counter value, we increment it by 1 and then we write it back 
  to the /proc/traceexit file corresponding "code" field. */
  
  /* 2.1. Read from /proc/traceexit */

  // myread() .....

  /* 2.2. check if code exists & update count value */

  /* 2.3. Write count value back to /proc/traceexit */

  // TO-DO: Implement sys_write system call- > Accessing  
  // /proc/traceexit will trigger the write operation implementation ( myWrite() ) 
  char write_ubuf[BUFSIZE];
  int c = 0;
  int exit_counter = 0;
  c = strlen(write_ubuf);

  sscanf(write_ubuf,"%d %d",&exit_code,&exit_counter);
  my_proc_write(PROC_FILE,write_ubuf,c,1000);

  /* 2.4. Close file, print message and execute exit syscall */

  printk ("exit code %d captured at /proc/traceexit\n", exit_code);
  return original_sys_exit(exit_code);
}

/*********************** END (2) ************************/

/****************** 3. MODULE OPERATIONS ****************/

static int __init traceexit_init(void){

   /* create the /proc file */
   struct proc_dir_entry *entry;
   entry = proc_create(PROC_FILE,0777,NULL,&my_fops);
   if(!entry){
		return -1;	
   }else{
		printk(KERN_INFO "create proc file successfully\n");
		printk (KERN_INFO "Exit syscall \"bypass\" correctly installed\n Compiled at %s %s\n", __DATE__, __TIME__);
		printk (KERN_NOTICE "exit syscall captured");
   }

   /* Enable custom exit syscall */

   original_sys_exit = (asmlinkage long (*)(int))(sys_call_table[__NR_exit]);
   
   GPF_DISABLE; /* Disable read-only protection (sys_call_table is on a read-only page )*/
   sys_call_table[__NR_exit] = (unsigned) new_sys_exit;
   GPF_ENABLE; /* Enable read-only protection */
 
   printk (KERN_NOTICE "exit syscall captured");
   printk (KERN_INFO "Correctly installed\n Compiled at %s %s\n", __DATE__,
          __TIME__);
   return (0);

}

static void __exit traceexit_exit(void){
  /* Remove procfs entry */
	remove_proc_entry(PROC_FILE,NULL);

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

module_init(traceexit_init);
module_exit(traceexit_exit);
MODULE_LICENSE("GPL");


/*********************** END (3) ************************/