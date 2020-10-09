/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/moduleparam.h> /* needed for module parameters */

// sprintf
#include  <linux/io.h>

void *reg_chip_ID;
void *reg_temp;
void *reg_MAC;


static int __init skeleton_init(void)
{
	const unsigned long phys_addr_ID = 0x01c14200;
	const unsigned long phys_addr_Temp = 0x01c25080;
	const unsigned long phys_addr_MAC = 0x01c30050;
	
	int tempRaw = 0;
	int tempC = 0;
	unsigned char macChar[8];
	
	int i=0;
	uint32_t chipID[4];
	
	pr_info ("Linux module skeleton loaded\n");
	
	reg_chip_ID = ioremap ( phys_addr_ID, 4 * 32);
	if (reg_chip_ID == NULL){
		printk(KERN_ERR "reg_chip_ID is NULL !\n");
		return -1;
	}
			
	reg_temp = ioremap ( phys_addr_Temp, 1 * 32);
	if (reg_temp == NULL){
		printk(KERN_ERR "reg_chip_ID is NULL !\n");
		return -1;
	}
	reg_MAC = ioremap ( phys_addr_MAC, 2 * 32);
	if (reg_MAC == NULL){
		printk(KERN_ERR "reg_chip_ID is NULL !\n");
		return -1;
	}
	
	
	// Lecture et affichage Temperature
	ioread32_rep(reg_temp, &tempRaw, 1);
	tempC = -1191 * tempRaw / 10 + 223000;
	pr_info ("Temps = %.2d [mC]\n", tempC);
	
	// Lecture et affichage MAC addresse
	ioread64_rep(reg_MAC, macChar, 1);
	pr_info ("MAC addresse = %02X:%02X:%02X:%02X:%02X:%02X\n", macChar[4],macChar[5],macChar[6],macChar[7],macChar[0],macChar[1]);	
	
	// Lecture et affichage CHIP ID
	for (i=0; i < 4; ++i){
		ioread32_rep(reg_chip_ID+(4*i), chipID+i, 1);
	}
	pr_info ("CHIP ID = %u - %u - %u - %u\n", chipID[0],chipID[1],chipID[2],chipID[3]);	

	
	return 0; 
	
}


static void __exit skeleton_exit(void)
{


	pr_info ("Linux module skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); MODULE_LICENSE ("GPL");
