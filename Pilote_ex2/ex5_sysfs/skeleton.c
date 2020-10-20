/*
 * Spinelli Isaia
 * 
 * Crée un dossier "test" dans -> /sys/bus/platform/drivers/
 * en utilisant un platform_driver
 * Crée un attribut driver "test12" dans le dossier "test"
 * 
 * Enregistre un device "/dev/simple_misc" avec miscdevice
 * 
 * Crée une clase ("test_class_2") dans sysfs 
 * Enregistre le device ("device_name") dans la class crée
 * Ajoute tous les attributs (mydrv_group) dans le sysfs 
 * 
 * */

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

/* Crée un device /dev/simple_misc */
struct miscdevice sample_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "simple_misc",
    .fops = &skeleton_fops,
    .mode = 0777,
};

static struct class *test_class ;
static dev_t test_devt;
struct device *test_dev;

// cat /sys/bus/platform/drivers/test/test12
ssize_t show_attr_driver (struct device_driver *drv, char * buf){
	pr_info ("show_attr_driver !\n");
	return 0;
}
// echo 1 > /sys/bus/platform/drivers/test/test12
ssize_t store_attr_driver (struct device_driver *drv, const char * buf, size_t count){
	pr_info ("store_attr_driver !\n");
	return count;
}
//static DRIVER_ATTR(test, S_IRUGO | S_IWUSR, show_attr_driver, store_attr_driver);
struct driver_attribute driver_attr_test = __ATTR(test12, 0664, show_attr_driver, store_attr_driver);

// jamais appelé (pas de noeud dans le DTB)
static int driver_probe(struct platform_device *pdev){
	
	pr_info ("driver_probe!\n");

	return 0;
}
// jamais appelé (pas de noeud dans le DTB)
static int driver_remove(struct platform_device *pdev){
	
	pr_info ("driver_remove!\n");

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

// cat /sys/class/test_class_2/device_name/mydrv/test_1
static ssize_t test_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	pr_info ("test_store ! \n");


	return count;
}
// echo 1 > /sys/class/test_class_2/device_name/mydrv/test_1
static ssize_t test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
		pr_info ("test_show ! \n");

	return 0;
}
        
/* attribut pour additionner shared_var d'une valeur écrite */               
static DEVICE_ATTR(test_1, 0664, test_show,
                   test_store);     
/* attribut pour decrémenter shared_var */                
static DEVICE_ATTR(test_2, 0664, test_show,
                   test_store);     
/*
 * Créez un groupe d'attributs afin que nous puissions les créer et 
 * les détruire tous en même temps.
 */            
static struct attribute *mydrv_attrs[] = {
    &dev_attr_test_1.attr,
    &dev_attr_test_2.attr,
    NULL
};

/* Permet d'avoir un nom de repertoire pour nos attributs 
 * /sys/class/test_class_2/device_name/mydrv */
static struct attribute_group mydrv_group = {
    .name = "mydrv",
    .attrs = mydrv_attrs,
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
	
	// Enregsitre le device mise (/dev/simple_misc)
    error = misc_register(&sample_device);
    if (error) {
        pr_err("can't misc_register :(\n");
        return error;
    }
    
    /* Enregistre le driver (test) dans /sys/bus/platform/drivers */
    error = platform_driver_register(&my_platform_driver);
	if (error) {
		pr_err("%s: failed to register platform driver\n",__func__);
		misc_deregister(&sample_device);
		return error;	
	}

    /* Crée l attribut "test12" (driver_attr_test) dans le dossier "test" */
    error = driver_create_file(&my_platform_driver.driver, &driver_attr_test);
	if (error){
		pr_err("can't driver_create_file :(\n");
		platform_driver_unregister(&my_platform_driver);
		misc_deregister(&sample_device);
		return error;
	}
	

    /* Crée une clase ("test_class_2") dans sysfs  */
    test_class = class_create(THIS_MODULE, "test_class_2");
        
    /* Enregistre le device ("device_name"/test_devt) dans la class 
     * ("test_class_2"/test_class) */ // sample_device
	test_dev = device_create(test_class, NULL, test_devt, NULL,
		      "device_name");
		      
	/* Ajoute tous les attributs (mydrv_group) dans le sysfs 
	 * (dans le repertoire du device: /sys/class/test_class_2/device_name/)*/
	error = sysfs_create_group(&test_dev->kobj, &mydrv_group);
    if (error) {
		
		pr_err("can't driver_create_file :(\n");

        device_destroy(test_class, test_devt);
		class_destroy(test_class);

		driver_remove_file(&my_platform_driver.driver, &driver_attr_test);
		
		platform_driver_unregister(&my_platform_driver);
				   
		misc_deregister(&sample_device);
        return error;
    }

	
	return 0; 
}
	
static void __exit skeleton_exit(void)
{
	
	 /* Retire tous les attributs du sysfs */
     sysfs_remove_group(&test_dev->kobj, &mydrv_group);
     
    // détruit le device
	device_destroy(test_class, test_devt);
	// Détruit la classe
	class_destroy(test_class);
	
	// Supprime l'attribut dans le driver platform
	driver_remove_file(&my_platform_driver.driver, &driver_attr_test);
	
	// Supprime le driver platform
	platform_driver_unregister(&my_platform_driver);
			   
	// Supprime le device misc
	misc_deregister(&sample_device);
	 
	pr_info ("Linux pilote skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); 
MODULE_LICENSE ("GPL");
