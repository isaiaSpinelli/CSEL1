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
// waitqueue
#include <linux/wait.h>


static wait_queue_head_t my_wait_queue;
int flag = 0;

int thread_fn_notif (void* data){
	
	while(!kthread_should_stop()){
		
		//wake_up_interruptible(&my_wait_queue);
		flag = 1;
		wake_up(&my_wait_queue);
		
		ssleep(5);
		pr_info ("call notif !");
	}
	
	return 0;
}

int thread_fn_wait (void* data){
	
	while(!kthread_should_stop()){
		pr_info ("wait notif !\n");
		
		wait_event_interruptible(my_wait_queue, flag == 1);
		flag = 0;
	}
	
	return 0;
}

static struct task_struct *thread1;
static struct task_struct *thread2;


static int __init skeleton_init(void)
{
	char name[15]="thread_wait";
	char name2[15]="thread_notif";
	
	init_waitqueue_head(&my_wait_queue);
	
	thread1 = kthread_create( thread_fn_wait, NULL, name);
	
	if((thread1))
	{
		wake_up_process(thread1);
	}
	
	thread2 = kthread_create( thread_fn_notif, NULL, name2);
	
	if((thread2))
	{
		wake_up_process(thread2);
	}
  
	return 0; 
	
}


static void __exit skeleton_exit(void)
{

	int ret;
	ret = kthread_stop(thread1);
	
	if(!ret)
		printk(KERN_INFO "Thread1 stopped");
		
	ret = kthread_stop(thread2);
	
	if(!ret)
		printk(KERN_INFO "Thread2 stopped");
		
	pr_info ("Linux module skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); MODULE_LICENSE ("GPL");
