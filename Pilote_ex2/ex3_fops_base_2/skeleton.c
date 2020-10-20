/*
 * Spinelli Isaia
 * N device (en paramètre) avec miscdevice
*/

/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/fs.h>           /* Needed for file_operations */
#include <linux/uaccess.h>      /* copy_(to|from)_user */
#include <linux/miscdevice.h> /*  miscdevice */
#include <linux/slab.h> /* kmalloc */

#define BUFFER_SIZE	 1024

char *global_buffer;
int *buffer_size;

static int NB_Devices= 1;
module_param(NB_Devices, int, 0);


struct miscdevice* mymisc;
int offset_minor;

static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos);
            
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *ppos);


// Structure constante statique pour les différentes opérations 
const static struct file_operations my_fops = {
    .owner         = THIS_MODULE,
    .read          = skeleton_read,
    .write         = skeleton_write,
};



/*
 * Permet de lire un fichier(buffer global) depuis l'espace utilisateur
*/
static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos)
{	
    // Crée l'index du tableau en fonction du numero minor
    int idx = (offset_minor - iminor(filp->f_inode)) * BUFFER_SIZE;
    
    pr_info ("Lecture !\n");
    
    if (buf == 0 || count < buffer_size[idx]) {
        return 0;
    }
    
     if (*ppos >= buffer_size[idx]) {
        return 0;
    }

    
    // Copier le buffer global dans l'espace utilisateur (buf).
    if ( copy_to_user(buf, (global_buffer+idx), buffer_size[idx]) != 0 ) {
		return 0;
	}
	
	// màj de la position
    *ppos = buffer_size[idx];

    return buffer_size[idx];
    
}


/*
 * Permet d'écrire dans un fichier (global buffer) 
 * depuis l'espace utilisateur (buf)
 */
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
             size_t count, loff_t *ppos)
             
{	// Crée l'index du tableau en fonction du numero minor
	int idx = (offset_minor - iminor(filp->f_inode)) * BUFFER_SIZE;
    
    pr_info ("Ecriture !\n");
    
    if (count == 0 ) {
        return 0;
    }
    
    if (count >= BUFFER_SIZE) {
		printk(KERN_ERR "error ! write too much caractere ! \n");
        return 0;
    }

    *ppos = 0;
    
    
    // Copier un bloc de données à partir de l'espace utilisateur (buf)
	// dans la mémoire alloué (global buffer)
	if ( copy_from_user((global_buffer+idx), buf, count) != 0) { 
		return 0;
	}

	(global_buffer+idx)[count] = '\0';
	
	buffer_size[idx] = count+1;

	return count;
}

static int __init skeleton_init(void)
{
	int retval,i;
	char name[10] ;

	pr_info ("Linux pilote skeleton loaded : %d\n", NB_Devices);
	
	// Alloue à 0 la structure misc device le nombre de device demandé
	mymisc = kzalloc(sizeof(*mymisc) * NB_Devices, GFP_KERNEL);
    if (mymisc == NULL) {
        printk(KERN_ERR "Failed to allocate memory for mymisc!\n");
        retval = -ENOMEM;
		return retval;
    }
	
	// Initialise les devices
	for(i=0; i < NB_Devices; ++i){
		mymisc[i].minor = MISC_DYNAMIC_MINOR; // offset_minor-i;
		sprintf(name, "myMisc_%d", i);
		mymisc[i].name = name;
		mymisc[i].fops = &my_fops;
		mymisc[i].mode = 0777,
		retval = misc_register(&mymisc[i]);
		if (retval) return retval;
				
	}
	// Enregistre l'offset des numeros minor
	offset_minor = mymisc[0].minor ;
	
	// Alloue à 0 un tableau 2D x*y  
	global_buffer = kzalloc(NB_Devices * BUFFER_SIZE * sizeof(char) , GFP_KERNEL);
	if (global_buffer == NULL) {
        printk(KERN_ERR "Failed to allocate memory for global_buffer!\n");
        retval = -ENOMEM;
		return retval;
    }
    
    // Alloue à 0 l index des différents tableau
    buffer_size = kzalloc(NB_Devices * sizeof(int) , GFP_KERNEL);
	if (buffer_size == NULL) {
        printk(KERN_ERR "Failed to allocate memory for buffer_size!\n");
        retval = -ENOMEM;
		return retval;
    }
    
	return 0; 
}
	
static void __exit skeleton_exit(void)
{	 
	int i;
	pr_info ("Linux pilote skeleton unloaded\n"); 
	
	for(i=0; i < NB_Devices; ++i){
		misc_deregister(&mymisc[i]);
	}
	kfree(mymisc);		
	
	kfree(global_buffer);
	
	kfree(buffer_size);
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); 
MODULE_LICENSE ("GPL");
