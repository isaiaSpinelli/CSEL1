/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/moduleparam.h> /* needed for module parameters */

static char* text= "test text";
module_param(text, charp, 0);
static int elements= 1;
module_param(elements, int, 0);

static int __init skeleton_init(void)
{
	pr_info ("Linux module skeleton loaded\n");
	printk( KERN_DEBUG "text: %s\nelements: %d\n", text, elements);
	return 0; }
static void __exit skeleton_exit(void)
{
	pr_info ("Linux module skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); 
MODULE_LICENSE ("GPL");
