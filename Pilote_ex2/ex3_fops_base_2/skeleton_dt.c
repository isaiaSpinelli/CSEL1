/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/slab.h>

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

static int NB_Devices= 1;
module_param(NB_Devices, int, 0);

struct mydata {
	char* buffer;
	struct miscdevice* mymisc;
	int offset_minor;
	struct platform_device *pdev;
};


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

static int skeleton_probe(struct platform_device *pdev)
{

    
    //dev_info(&pdev->dev, "Registered\n");

   /* Déclaration de la structure priv */
    struct mydata *mydata_priv;
    struct miscdevice *miscDevice;
    int i;
    int retval;
    char name[9] ;
    
	pr_info ("Linux pilote skeleton loaded\n");
	
	if (NB_Devices <= 0){
		pr_err("Nombre de device error !\n");
		return -1;
	}
	
	
	
	/* Allocation mémoire kernel pour la structure priv (informations du module)*/
    mydata_priv = kmalloc(sizeof(*mydata_priv), GFP_KERNEL);
    if (mydata_priv == NULL) {
        printk(KERN_ERR "Failed to allocate memory for private data!\n");
		/* Met à jour la valeur de retour (Out of memory)*/
        retval = -ENOMEM;
		/* Fin de la fonction et retourne rc */
        goto kmalloc_fail;
    }
    
    platform_set_drvdata(pdev, mydata_priv);
    mydata_priv->pdev = pdev;
    
    mydata_priv->mymisc = kmalloc(sizeof(miscDevice) * NB_Devices, GFP_KERNEL);
    if (mydata_priv->mymisc == NULL) {
        printk(KERN_ERR "Failed to allocate memory for mymisc!\n");
        retval = -ENOMEM;
		/* Fin de la fonction et retourne rc */
        goto kmalloc_fail_mymisc;
    }
	
	mydata_priv->offset_minor = MISC_DYNAMIC_MINOR;
	for(i=0; i < NB_Devices; ++i){
		mydata_priv->mymisc[i].minor = mydata_priv->offset_minor+i;
		
		sprintf(name, "myMisc_%d", i);
		mydata_priv->mymisc[i].name = name;
		mydata_priv->mymisc[i].fops = &my_fops;
		retval = misc_register(&mydata_priv->mymisc[i]);
		if (retval) return retval;
				
	}
    
		
	/* Lis le pointeur de la structure contenant les informations du module au platform_device coresspondant (permet de le get dans la fonction remove)*/
    platform_set_drvdata(pdev, mydata_priv);
    
    
	//result = alloc_chrdev_region(&sample_dev_t, 0, NB_Devices, "sample-cdev");
	
	
	printk( KERN_DEBUG "Nombre elements: %d\n", NB_Devices);
	
	/* Retourne 0 si la fonction probe c'est correctement effectué */
    return 0;
    
kmalloc_fail_mymisc:
	 kfree(mydata_priv);
kmalloc_fail:
	/* Retourne le numéro de l'erreur */
    return retval;

    

}

static int skeleton_remove(struct platform_device *pdev)
{
	int i;
    struct mydata *mydata_priv = platform_get_drvdata(pdev);
    
    pr_info ("Linux pilote Unregistered skeleton\n"); 
	
	
	for(i=0; i < NB_Devices; ++i){
		misc_deregister(&mydata_priv->mymisc[i]);
	}
	kfree(mydata_priv->mymisc);		
	
	kfree(mydata_priv);
	
	return 0;
}

/* Structure permettant  de représenter le driver */
static struct platform_driver skeleton_driver = {
	/* Déclare le nom, le module et la table des compatibilités du module*/
    .driver = {
        .name = "ex2_misc_s",
        .owner = THIS_MODULE,
        //.of_match_table = of_match_ptr(skeleton_driver_id),
    },
	/* Définit la fonction probe appelée lors du branchement d'un préiphérique pris en charge par ce module */
    .probe = skeleton_probe,
	/* Définit la fonction remove appelée lors du débranchement d'un préiphérique pris en charge par ce module */
    .remove = skeleton_remove,
};

/* Macro d'assistance pour les pilotes qui ne font rien de spécial dans les fonctions init / exit. Permet d'éliminer du code (fonction init et exit) */
/* Enregistre le driver via la structure ci-dessus */
module_platform_driver(skeleton_driver);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); 
MODULE_LICENSE ("GPL");


/*
 * Permet de lire un fichier(buffer global) depuis l'espace utilisateur
*/
static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos)
{	
    
    pr_info ("Lecture !\n");
    
    return 0;
    
    
}


/*
 * Permet d'écrire dans un fichier (global buffer) 
 * depuis l'espace utilisateur (buf)
 */
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
             size_t count, loff_t *ppos)
             
{
    
    pr_info ("Ecriture !\n");
    


	return 0;
}
