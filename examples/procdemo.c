/*
 * /proc example module
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/uaccess.h>	/* copy_from_user */
#include <linux/proc_fs.h>
#include <linux/sched.h>	/* find_task_by_pid_ns */
#include <linux/seq_file.h>

MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("/proc demo");

/* /proc interface to the module */

#define PROC_ENTRY "procdemo"	/* File name */

pid_t pid = 0;

/* Read: Shows information about process "pid" */
/* Information is written using seq_printf function */
static int
read_proc (struct seq_file *m, void *p)
{
  struct task_struct *task = NULL;

  if (pid)
    {
      task = pid_task(find_vpid(pid), PIDTYPE_PID); // Get task struct from pid
//      task = find_task_by_pid_ns (pid, &init_pid_ns);
      if (!task)
	return -EINVAL;

      seq_printf (m, "Information about process %u\n", pid);
      seq_printf (m, " ppid %u\n", task->parent->pid);
      seq_printf (m, " Voluntary context switches %lu\n", task->nvcsw);
      seq_printf (m, " Involuntary context switches %lu\n", task->nivcsw);
      seq_printf (m, " Page table address %p\n",
		  (task->mm ? task->mm->pgd : NULL));
    }
  else
    seq_printf (m, "No pid defined\n");
  return 0;
}

/* Write: captures the pid of the process */
ssize_t
write_proc (struct file * f, const char __user * buff, size_t len, loff_t * o)
{
#define MAX_LEN 7
  char c[MAX_LEN + 1];

  if (len > MAX_LEN)
    return -EINVAL;

  if (copy_from_user (c, buff, len))
    return -EFAULT;

  c[len] = 0;
  if (!sscanf (c, "%d\n", &pid))
    return printk (KERN_INFO "Read pid %d\n", pid);

  return len;
}

static int
ex2_open (struct inode *inode, struct file *file)
{
  return single_open (file, read_proc, NULL);
}

static struct file_operations proc_operations = {
  .owner = THIS_MODULE,
  .open = ex2_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .write = write_proc,
  .release = single_release
};

static int __init
procdemo_init (void)
{
  /* Register /proc entry for our module */
  if (!proc_create (PROC_ENTRY, S_IFREG, NULL, &proc_operations))
      return (-1);

  printk (KERN_INFO "procdemo: Correctly installed\n Compiled at %s %s\n",
	  __DATE__, __TIME__);
  return (0);
}

static void __exit
procdemo_cleanup (void)
{
  remove_proc_entry (PROC_ENTRY, NULL); /* Unregister /proc entry for our module */
  printk (KERN_INFO "EXEMPLE: Cleanup successful\n");
}

module_init (procdemo_init);
module_exit (procdemo_cleanup);
