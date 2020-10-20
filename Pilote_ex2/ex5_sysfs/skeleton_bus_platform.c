/*
 * Spinelli Isaia
 * 
 * Crée un dossier "test" dans -> /sys/bus/platform/drivers/
 * en utilisant un platform_driver
 * 
 * Crée un attribut driver "test12" dans le dossier "test"
*/

/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
//#include <linux/init.h> /* needed for macros */ 
//#include <linux/kernel.h> /* needed for debugging */
#include <linux/fs.h>           /* Needed for file_operations */
//#include <linux/uaccess.h>      /* copy_(to|from)_user */
#include <linux/miscdevice.h> /*  miscdevice */

#include <linux/device.h> /*  DRIVER_ATTR */


#include <linux/platform_device.h> /* platform_device */

#define BUFFER_SIZE	 1024

char global_buffer[BUFFER_SIZE] = {0};
int buffer_size;

static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos);
            
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *ppos);


// Structure constante statique pour les différentes opérations 
const static struct file_operations skeleton_fops = {
    .owner         = THIS_MODULE,
    .read          = skeleton_read,
    .write         = skeleton_write,
};

struct miscdevice sample_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "simple_misc",
    .fops = &skeleton_fops,
    .mode = 0777,
};


ssize_t show_attr_driver (struct device_driver *drv, char * buf){
	pr_info ("show_attr_driver !\n");
	return 0;
}
ssize_t store_attr_driver (struct device_driver *drv, const char * buf, size_t count){
	pr_info ("store_attr_driver !\n");
	return count;
}
//static DRIVER_ATTR(test, S_IRUGO | S_IWUSR, show_attr_driver, store_attr_driver);
struct driver_attribute driver_attr_test = __ATTR(test12, 0664, show_attr_driver, store_attr_driver);

static int driver_probe(struct platform_device *pdev){
	return 0;
}
static int driver_remove(struct platform_device *pdev){
	return 0;
}
// Crée une repertoire "test" dans -> /sys/bus/platform/drivers/
static struct platform_driver my_platform_driver = {
	.probe = driver_probe,
	.remove = driver_remove,
	.driver = {
		.name = "test",
			},
};

/*
 * Permet de lire un fichier(buffer global) depuis l'espace utilisateur
*/
static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos)
{	
	if (buf == 0 || count < buffer_size) {
        return 0;
    }
    
     if (*ppos >= buffer_size) {
        return 0;
    }
    
    pr_info ("Lecture !\n");
    
    pr_info ("Minor = %d\n", iminor(filp->f_inode));

    
    // Copier le buffer global dans l'espace utilisateur (buf).
    if ( copy_to_user(buf, global_buffer, buffer_size) != 0 ) {
		return 0;
	}
	
	// màj de la position
    *ppos = buffer_size;

    return buffer_size;
    
    
}


/*
 * Permet d'écrire dans un fichier (global buffer) 
 * depuis l'espace utilisateur (buf)
 */
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
             size_t count, loff_t *ppos)
             
{
	
	if (count == 0 ) {
        return 0;
    }
    
    if (count >= BUFFER_SIZE) {
		printk(KERN_ERR "error ! write too much caractere ! \n");
        return 0;
    }

    *ppos = 0;
    
    pr_info ("Ecriture !\n");
    
    // Copier un bloc de données à partir de l'espace utilisateur (buf)
	// dans la mémoire alloué (global buffer)
	if ( copy_from_user(global_buffer, buf, count) != 0) { 
		return 0;
	}

	global_buffer[count] = '\0';
	
	buffer_size = count+1;

	return count;
}

static int __init skeleton_init(void)
{
	int error;

	pr_info ("Linux pilote skeleton loaded\n");
	
	buffer_size = 0;
	
	
    error = misc_register(&sample_device);
    if (error) {
        pr_err("can't misc_register :(\n");
        return error;
    }
    
    error = platform_driver_register(&my_platform_driver);
	if (error) {
		pr_err("%s: failed to register platform driver\n",__func__);
		misc_deregister(&sample_device);
		return error;	
	}

    
    error = driver_create_file(&my_platform_driver.driver, &driver_attr_test);
	if (error){
		pr_err("can't driver_create_file :(\n");
		platform_driver_unregister(&my_platform_driver);
		misc_deregister(&sample_device);
		return error;
	}

	
	return 0; 
}
	
static void __exit skeleton_exit(void)
{
	
	driver_remove_file(&my_platform_driver.driver, &driver_attr_test);
	
	platform_driver_unregister(&my_platform_driver);
			   
	misc_deregister(&sample_device);
	 
	pr_info ("Linux pilote skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); 
MODULE_LICENSE ("GPL");
