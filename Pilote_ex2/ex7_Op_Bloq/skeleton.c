/*
 * 
 * Spinelli Isaia
 * Miscdevice / wiatqueue / irq / poll
 * /

/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/miscdevice.h> /*  miscdevice */
// sprintf
#include  <linux/io.h>
// treahd 
#include <linux/kthread.h> 
// ssleep
#include <linux/delay.h>
// waitqueue
#include <linux/wait.h>
// irq
#include <linux/interrupt.h>
// gpio irq
#include <linux/gpio.h>

// poll
#include <linux/poll.h>

unsigned int irq0 = 0;
unsigned int irq1 = 0;
unsigned int irq2 = 0;

int buttonPressed = 0;
wait_queue_head_t button_queue;

#define KEY0 0
#define KEY1 2
#define KEY2 3
#define MY_DEV_NAME "my_device"


static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos);
            
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *ppos);
			
unsigned int skeleton_poll(struct file * file,
poll_table * pt);


// Structure constante statique pour les différentes opérations 
const static struct file_operations my_fops = {
    .owner         = THIS_MODULE,
    .read          = skeleton_read,
    .write         = skeleton_write,
    .poll		= skeleton_poll,
};


struct miscdevice sample_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "button_nano",
    .fops = &my_fops,
    .mode = 0777,
};

irqreturn_t short_interrupt(int irq, void *dev_id)
{
	
	
	int irqNum = irq & 0x3;
	
	switch(irqNum){
		
			case 0:
				pr_info ("K1 is pressed ! \n"); 
				break;
			case 2:
				pr_info ("K2 is pressed ! \n"); 
				break;
			case 3:
				pr_info ("K3 is pressed ! \n"); 
				break;
			default :
				pr_info ("Short_interrupt ! (%d)\n", irq); 
				break;
			
	}
	
	buttonPressed = 1;
	wake_up(&button_queue);
	
	return IRQ_HANDLED;
}

static int __init skeleton_init(void)
{
	
	int ret;
	
	printk("module: button interrupt example.\n");
	
	init_waitqueue_head(&button_queue);
	
	ret = misc_register(&sample_device);
    if (ret) {
        pr_err("can't misc_register :(\n");
        return ret;
    }

	
	if(gpio_is_valid(KEY0) == 0){
		printk(KERN_ERR "gpio0 is not valid\n");
		return -1;
	} 
	if(gpio_request(KEY0, "KEY0") < 0) {
		printk(KERN_ERR "gpio request is not valid\n");
		return -1;
	} 
	if( (irq0 = gpio_to_irq(KEY0)) < 0 ){
		printk(KERN_ERR "gpio irq is not valid\n");
		return -1;
	} 
	
	
	if(gpio_is_valid(KEY1) == 0){
		printk(KERN_ERR "gpio1 is not valid\n");
		return -1;
	} 
	if(gpio_request(KEY1, "KEY0") < 0) {
		printk(KERN_ERR "gpio request is not valid\n");
		return -1;
	} 
	if( (irq1 = gpio_to_irq(KEY1)) < 0 ){
		printk(KERN_ERR "gpio irq is not valid\n");
		return -1;
	} 
	
	if(gpio_is_valid(KEY2) == 0){
		printk(KERN_ERR "gpio2 is not valid\n");
		return -1;
	} 
	if(gpio_request(KEY2, "KEY0") < 0) {
		printk(KERN_ERR "gpio request is not valid\n");
		return -1;
	}
	if( (irq2 = gpio_to_irq(KEY2)) < 0 ){
		printk(KERN_ERR "gpio irq is not valid\n");
		return -1;
	} 
	
	ret = request_irq ( irq0, (irq_handler_t) short_interrupt, IRQF_SHARED, "pushbutton_irq_handler", MY_DEV_NAME );
	if (ret != 0) {
		printk(KERN_ERR "error code: %d\n", ret);
	}
	ret = request_irq ( irq1, (irq_handler_t) short_interrupt, IRQF_SHARED, "pushbutton_irq_handler", MY_DEV_NAME );
	if (ret != 0) {
		printk(KERN_ERR "error code: %d\n", ret);
	}
	ret = request_irq ( irq2, (irq_handler_t) short_interrupt, IRQF_SHARED, "pushbutton_irq_handler", MY_DEV_NAME );
	if (ret != 0) {
		printk(KERN_ERR "error code: %d\n", ret);
	}
  
	return 0; 
	
}


static void __exit skeleton_exit(void)
{
	gpio_free(KEY0);
	gpio_free(KEY1);
	gpio_free(KEY2);
	
	free_irq(irq0, MY_DEV_NAME);
	free_irq(irq1, MY_DEV_NAME);
	free_irq(irq2, MY_DEV_NAME);
	
	misc_deregister(&sample_device);
	
	pr_info ("Linux module skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); MODULE_LICENSE ("GPL");

unsigned int skeleton_poll(struct file * file,
poll_table * pt){
	unsigned int mask = 0;
	
	if (buttonPressed) mask |= POLLIN | POLLPRI;
	buttonPressed = 0;
	
	poll_wait(file, &button_queue, pt);
	
	return mask;
}


/*
 * Permet de lire un fichier(buffer global) depuis l'espace utilisateur
*/
static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos)
{	
   
    pr_info ("Pourrait retourner des informations aux utilisateurs (read wich key)\n");

    return 0;
    
}


/*
 * Permet d'écrire dans un fichier (global buffer) 
 * depuis l'espace utilisateur (buf)
 */
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
             size_t count, loff_t *ppos)
             
{	
    pr_info ("Pourrait recevoir des informations des utilisateurs (disable irq) \n");
    
	return count;
}
