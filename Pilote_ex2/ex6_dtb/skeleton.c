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


/* Structure permettant de mémoriser les informations importantes du module */
struct priv
{	
	char buffer[BUFFER_SIZE];
	int buffer_size;	
	struct miscdevice mymisc;
};

struct priv_data {
	int nb_devices;
	struct priv* priv;
};



int countDeviceOn(struct device_node* dt_node);

static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos);
            
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *ppos);

static int skeleton__open(struct inode* node, struct file * f);

// Structure constante statique pour les différentes opérations 
const static struct file_operations my_fops = {
    .owner         = THIS_MODULE,
    .read          = skeleton_read,
    .write         = skeleton_write,
    .open          = skeleton__open,
};



static int skeleton__open(struct inode* node, struct file * f){
	
	/* Récupération des informations du module (structure privée) */
    //struct priv *private_data ;
    //private_data = container_of(node->i_cdev, struct priv, my_cdev);
    /* Placement de la structure privée dans les data du fichier */
    //f->private_data = private_data;
    
    return 0;
}

/*
 * Permet de lire un fichier(buffer global) depuis l'espace utilisateur
*/
static ssize_t skeleton_read(struct file *filp, char __user *buf,
            size_t count, loff_t *ppos)
{	
	/* Récupération des informations du module (structure privée) */
	struct priv *private_data = container_of(filp->private_data, struct priv, mymisc);
	
    pr_info ("Lecture !\n");
    
    if (buf == 0 || count < private_data->buffer_size) {
        return 0;
    }
    
     if (*ppos >= private_data->buffer_size) {
        return 0;
    }

    // Copier le buffer global dans l'espace utilisateur (buf).
    if ( copy_to_user(buf, private_data->buffer, private_data->buffer_size) != 0 ) {
		return 0;
	}
	
	// màj de la position
    *ppos = private_data->buffer_size;

    return private_data->buffer_size;
    
}


/*
 * Permet d'écrire dans un fichier (global buffer) 
 * depuis l'espace utilisateur (buf)
 */
static ssize_t skeleton_write(struct file *filp, const char __user *buf,
             size_t count, loff_t *ppos)
             
{	
	/* Récupération des informations du module (structure privée) */
	struct priv *private_data = container_of(filp->private_data, struct priv, mymisc);
	
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
	if ( copy_from_user(private_data->buffer, buf, count) != 0) { 
		return 0;
	}

	private_data->buffer[count] = '\0';
	
	private_data->buffer_size = count+1;

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
	/* Déclaration de la structure priv */
	int retval;
	int i =0;
	struct device_node* dt_node = pdev->dev.of_node;
	pr_info ("Linux pilote skeleton : skeleton_probe\n");	

	/* Allocation mémoire kernel pour la structure priv (informations du module)*/
	struct priv_data* priv_data = devm_kzalloc(&(pdev->dev), sizeof(*priv_data), GFP_KERNEL);
	/* Si la fonction kmallos à échoué */
    if (priv_data == NULL) {
        printk(KERN_ERR "Failed to allocate memory for private data!\n");
		/* Met à jour la valeur de retour (Out of memory)*/
        return -ENOMEM;
    }
	
	priv_data->nb_devices = countDeviceOn(dt_node);
	pr_info("NB_Devices = %d \n", priv_data->nb_devices);

	/* Allocation mémoire kernel pour la structure priv (informations du module)*/
    priv_data->priv = devm_kcalloc(&(pdev->dev), priv_data->nb_devices, sizeof(struct priv), GFP_KERNEL);
	/* Si la fonction kmallos à échoué */
    if (priv_data->priv == NULL) {
        printk(KERN_ERR "Failed to allocate memory for private data!\n");
		/* Met à jour la valeur de retour (Out of memory)*/
        return -ENOMEM;
    }
	
	/* Lis le pointeur de la structure contenant les informations du module au platform_device coresspondant (permet de le get dans la fonction remove)*/
    platform_set_drvdata(pdev, priv_data);
	
	
    if (dt_node) {
		const char* attr = 0;
		struct device_node* child = 0;
		
		for_each_available_child_of_node(dt_node, child)
		{
			retval = of_property_read_string(child, "attribute", &attr);
			if (attr && retval == 0){
				if (strcmp (attr, "on") == 0) {
					priv_data->priv[i].mymisc.minor = MISC_DYNAMIC_MINOR;
					priv_data->priv[i].mymisc.name = child->full_name;
					priv_data->priv[i].mymisc.fops = &my_fops;
					priv_data->priv[i].mymisc.mode = 0777,
					retval = misc_register(&priv_data->priv[i].mymisc);
					i++;
					if (retval) return retval;
				}
			}
		}
	}
		
	return 0;
}

/* Fonction remove appelée lors du "débranchement" du périphérique (pdev est un poiteur sur une structure contenant toutes les informations du device retiré) */
static int skeleton_remove(struct platform_device *pdev)
{
	int i=0;
	/* Récupère l'adresse de la structure priv correspondant au platform_device reçu (précèdemment lié dans la fonction probe)*/
	struct priv_data *priv_data = platform_get_drvdata(pdev);
	pr_info ("Linux pilote skeleton : skeleton_remove\n");	
	
	for(i=0; i < priv_data->nb_devices; ++i){
		misc_deregister(&priv_data->priv[i].mymisc);
	}
	
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
