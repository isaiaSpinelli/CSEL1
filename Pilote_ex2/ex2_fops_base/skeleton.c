/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/fs.h>           /* Needed for file_operations */
#include <linux/uaccess.h>      /* copy_(to|from)_user */
#include <linux/miscdevice.h> /*  miscdevice */

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

	
	return 0; 
}
	
static void __exit skeleton_exit(void)
{
	misc_deregister(&sample_device);
	 
	pr_info ("Linux pilote skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); 
MODULE_LICENSE ("GPL");
