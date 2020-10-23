/*
 * Spinelli Isaia
 * N device (dans le dtb) avec miscdevice
*/

/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/fs.h>           /* Needed for file_operations */
#include <linux/uaccess.h>      /* copy_(to|from)_user */
#include <linux/miscdevice.h> /*  miscdevice */
#include <linux/slab.h> /* kmalloc */
#include <linux/of.h>
#include <linux/platform_device.h>

#define BUFFER_SIZE	 1024

char *global_buffer;
int *buffer_size;
int NB_Devices=-1;


struct miscdevice* mymisc;
int offset_minor;

int countDeviceOn(struct device_node* dt_node);

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


//module_init (skeleton_init);
//module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); 
MODULE_LICENSE ("GPL");

/* Fonction probe appelée lors du "branchement" du périphérique (pdev est un poiteur sur une structure contenant toutes les informations du device détécté) */
static int skeleton_probe(struct platform_device *pdev)
{
	int retval;
	int i =0;
	struct device_node* dt_node = pdev->dev.of_node;
	pr_info ("Linux pilote skeleton : skeleton_probe\n");	
	
	NB_Devices = countDeviceOn(dt_node);
	
	pr_info("NB_Devices = %d \n", NB_Devices);
	
	// Alloue à 0 la structure misc device le nombre de device demandé
	mymisc = kzalloc(sizeof(*mymisc) * NB_Devices, GFP_KERNEL);
    if (mymisc == NULL) {
        printk(KERN_ERR "Failed to allocate memory for mymisc!\n");
        retval = -ENOMEM;
		return retval;
    }
    
    if (dt_node) {
		const char* attr = 0;
		struct device_node* child = 0;
		
		for_each_available_child_of_node(dt_node, child)
		{
			retval = of_property_read_string(child, "attribute", &attr);
			if (attr && retval == 0){
				if (strcmp (attr, "on") == 0) {
					mymisc[i].minor = MISC_DYNAMIC_MINOR;
					mymisc[i].name = child->full_name;
					mymisc[i].fops = &my_fops;
					mymisc[i].mode = 0777,
					retval = misc_register(&mymisc[i]);
					i++;
					if (retval) return retval;
				}
				

			}
		}
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

/* Fonction remove appelée lors du "débranchement" du périphérique (pdev est un poiteur sur une structure contenant toutes les informations du device retiré) */
static int skeleton_remove(struct platform_device *pdev)
{
	int i;
	pr_info ("Linux pilote skeleton : skeleton_remove\n");	
	
	for(i=0; i < NB_Devices; ++i){
		misc_deregister(&mymisc[i]);
	}
	kfree(mymisc);		
	
	kfree(global_buffer);
	
	kfree(buffer_size);
	
	return 0;
}


/* Tableau de structure of_device_id qui permet de déclarer les compatibiltés des périphériques gérés par ce module */
static const struct of_device_id mydevice_driver_id[] = {
	/* Déclare la compatibilité du périphérique géré par ce module */
    { .compatible = "mydevice" },
    { /* END */ },
};

/* Cette macro décrit les périphériques que le module peut prendre en charge (recherche de la dtb)*/
MODULE_DEVICE_TABLE(of, mydevice_driver_id);

/* Structure permettant  de représenter le driver */
static struct platform_driver myPlatform_driver = {
	/* Déclare le nom, le module et la table des compatibilités du module*/
    .driver = {
        .name = "myModulePlatform",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(mydevice_driver_id),
    },
	/* Définit la fonction probe appelée lors du branchement d'un préiphérique pris en charge par ce module */
    .probe = skeleton_probe,
	/* Définit la fonction remove appelée lors du débranchement d'un préiphérique pris en charge par ce module */
    .remove = skeleton_remove,
};

/* Macro d'assistance pour les pilotes qui ne font rien de spécial dans les fonctions init / exit. Permet d'éliminer du code (fonction init et exit) */
/* Enregistre le driver via la structure ci-dessus */
module_platform_driver(myPlatform_driver);


int countDeviceOn(struct device_node* dt_node){
	int ret;
	int count = 0;
	
	if (dt_node) {
		const char* attr = 0;
		struct device_node* child = 0;
		
		for_each_available_child_of_node(dt_node, child)
		{
			pr_info("child found: name=%s, fullname=%s\n",
			child->name,
			child->full_name);
			
			ret = of_property_read_string(child, "attribute", &attr);
			if (attr && ret == 0){
				pr_info("attribute=%s (ret=%d)\n", attr, ret);

				if (strcmp (attr, "on") == 0) {
					count++;
				}
				

			}
		}
	}
	
	return count;
}
