/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/moduleparam.h> /* needed for module parameters */

// sprintf
#include  <linux/io.h>
// treahd 
#include <linux/kthread.h> 
// ssleep
#include <linux/delay.h>

int thread_fn (void* data){
	
	while(!kthread_should_stop()){
		pr_info ("Call in thread ! /n");
		ssleep(5);
	}
	
	return 0;
}

static struct task_struct *thread1;


static int __init skeleton_init(void)
{
	char name[8]="thread1";
	
	thread1 = kthread_create( thread_fn, NULL, name);
	
	if((thread1))
	{
		printk(KERN_INFO "in if");
		wake_up_process(thread1);
	}
	printk(KERN_INFO "out if");
  
	return 0; 
	
}


static void __exit skeleton_exit(void)
{

	int ret;
	ret = kthread_stop(thread1);
	
	if(!ret)
		printk(KERN_INFO "Thread stopped");
	pr_info ("Linux module skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); MODULE_LICENSE ("GPL");
