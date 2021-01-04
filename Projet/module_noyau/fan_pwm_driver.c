/**
 * Autĥor:	Charbon Yann & Spinelli Isaia
 * Date:	11.12.20
 * File : fan_pwm_driver.c
 * Desc. : Driver du projet CSEL1
 */

#include <linux/module.h> /* needed by all modules */
#include <linux/device.h> /*  DRIVER_ATTR */

#include <linux/thermal.h>
#include <linux/gpio.h>
#include <linux/timer.h>


#define BUFFER_SIZE	 	32
#define LED				10


static struct class *myClass ;
static dev_t devt;
struct device *myDev;


bool mode = 0;
int freq = 2;

struct thermal_zone_device *cpu_thermal;
struct thermal_zone_device *gpu_thermal;

struct timer_list timer;


// change l etat de la led 
void toggleLed(void){
	static int led_trigger = 0;
	gpio_set_value(LED, led_trigger);
	led_trigger = led_trigger ? (0):(1);
	
}

//void timer_callback(void){
void timer_callback(struct timer_list *unused){

	int temp=0, ret;
	
	toggleLed();
	mod_timer(&timer, jiffies + msecs_to_jiffies(1000 /freq));
	
	// mode automatique
	if (mode == 0){
		
		if ((ret = thermal_zone_get_temp(cpu_thermal, &temp))) {
			if (ret != -EAGAIN){
				dev_warn(&cpu_thermal->device,
					 "failed to read out thermal zone (%d)\n",
					 ret);
			}
				
			return -1;
		} else {
			
			if (temp < 35000){
				freq = 2;
			} else if (temp < 40000){
				freq = 5;
			} else if (temp < 45000){
				freq = 10;
			} else {
				freq = 20;
			}
		}
		
		
		
	}
	
}
//DEFINE_TIMER(timer, timer_callback);

// echo 1 > /sys/class/fan_pwm/device_pwm/parameters/mode
static ssize_t mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long ret;
	long mode_wr = -1;
	

    ret = kstrtol(buf, 10, &mode_wr);

	if (ret == 0) { 
		pr_info ("mode = %ld \n", mode_wr);
		mode = mode_wr;
	} else if (ret == -ERANGE) {
		printk(KERN_ERR "error ! kstrtol - overflow  ! (err = %ld)\n", ret);
		return -1;
	} else if (ret == -EINVAL ) {
		printk(KERN_ERR "error ! kstrtol - parsing error  ! (err = %ld)\n", ret);
		return -1;
	} else {
		printk(KERN_ERR "error ! kstrtol - don't know error  ! (err = %ld)\n", ret);
		return -1;
	}
	
	
	return count;
}
// cat /sys/class/fan_pwm/device_pwm/parameters/mode
static ssize_t mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	int modeASCII = mode+48;


	pr_info ("0 = mode automatic\n1 = mode manuel");
	pr_info ("Actually, mode = %d\n", mode);
	
	
	// Copier le buffer global dans l'espace utilisateur (buf).
    if ( copy_to_user(buf, (void*)&modeASCII, 1) != 0 ) {
		return 0;
	}

	return 1;

}
       
// echo 1 > /sys/class/fan_pwm/device_pwm/parameters/freq
static ssize_t freq_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{

	unsigned long ret;
	long freq_wr = -1;
	
	pr_info ("freq ! count = %ld \n", count);
    
    
    ret = kstrtol(buf, 10, &freq_wr);

	if (ret == 0) { 
		pr_info ("freq = %ld \n", freq_wr);
		freq = freq_wr;
		
	} else if (ret == -ERANGE) {
		printk(KERN_ERR "error ! kstrtol - overflow  ! (err = %ld)\n", ret);
		return -1;
	} else if (ret == -EINVAL ) {
		printk(KERN_ERR "error ! kstrtol - parsing error  ! (err = %ld)\n", ret);
		return -1;
	} else {
		printk(KERN_ERR "error ! kstrtol - don't know error  ! (err = %ld)\n", ret);
		return -1;
	}
	
	
	return count;
}
// cat > /sys/class/fan_pwm/device_pwm/parameters/freq
static ssize_t freq_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	int sizeBuf = 0;
	char freq_str[BUFFER_SIZE] = {0};

	pr_info ("freq show ! \n");
	
	sizeBuf = snprintf(freq_str, BUFFER_SIZE, "%d",freq);
		
	// Copier le buffer global dans l'espace utilisateur (buf).
    if ( copy_to_user(buf, freq_str, sizeBuf) != 0 ) {
		return 0;
	}

	return sizeBuf;
}

// cat > /sys/class/fan_pwm/device_pwm/parameters/temp_cpu
static ssize_t temp_cpu_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
		
	int sizeBuf = 0;
	char temp_str[BUFFER_SIZE] = {0};
	int temp=0, ret;
	

    ret = thermal_zone_get_temp(cpu_thermal, &temp);
    if (ret) {
        if (ret != -EAGAIN){
			dev_warn(&cpu_thermal->device,
                 "failed to read out thermal zone (%d)\n",
                 ret);
		}
            
        return -1;
    }
    
	pr_info ("temp cpu = %d\n", temp); // 42564
	
	sizeBuf = snprintf(temp_str, BUFFER_SIZE, "%d", temp);
		
	// Copier le buffer global dans l'espace utilisateur (buf).
    if ( copy_to_user(buf, temp_str, sizeBuf) != 0 ) {
		return 0;
	}

	return sizeBuf;

}
// cat > /sys/class/fan_pwm/device_pwm/parameters/temp_gpu
static ssize_t temp_gpu_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
		
	int sizeBuf = 0;
	char temp_str[BUFFER_SIZE] = {0};
	int temp=0, ret;
	
	ret = thermal_zone_get_temp(gpu_thermal, &temp);
    if (ret) {
        if (ret != -EAGAIN){
			dev_warn(&gpu_thermal->device,
                 "failed to read out thermal zone (%d)\n",
                 ret);
		}
            
        return -1;
    }
    
	pr_info ("temp gpu = %d\n", temp); // 42564
	
	sizeBuf = snprintf(temp_str, BUFFER_SIZE, "%d", temp);
		
	// Copier le buffer global dans l'espace utilisateur (buf).
    if ( copy_to_user(buf, temp_str, sizeBuf) != 0 ) {
		return 0;
	}
	
	toggleLed();
	
	return sizeBuf;

}
 
/* attribut pour régler le mode */               
static DEVICE_ATTR(mode, 0664, mode_show,
                   mode_store);     
/* attribut pour régler la fréquence */                
static DEVICE_ATTR(freq, 0664, freq_show,
                   freq_store);  
/* attribut pour lire la temp du cpu */                
static DEVICE_ATTR(temp_cpu, 0444, temp_cpu_show,
                   NULL);  
/* attribut pour lire la temp du gpu */                
static DEVICE_ATTR(temp_gpu, 0444, temp_gpu_show,
                   NULL);  
/*
 * Créez un groupe d'attributs afin que nous puissions les créer et 
 * les détruire tous en même temps.
 */            
static struct attribute *mydrv_attrs[] = {
    &dev_attr_mode.attr,
    &dev_attr_freq.attr,
	&dev_attr_temp_cpu.attr,
    &dev_attr_temp_gpu.attr,
    NULL
};

/* Permet d'avoir un nom de repertoire pour nos attributs 
 * /sys/class/fan_pwm/device_pwm/parameters/ */
static struct attribute_group mydrv_group = {
    .name = "parameters",
    .attrs = mydrv_attrs,
};

static int __init skeleton_init(void)
{
	int error;

	pr_info ("Linux pilote skeleton loaded\n");
	
	
	// ---- SET ZONE CPU TEMP -----
	cpu_thermal = thermal_zone_get_zone_by_name("cpu-thermal");
	error = (uintptr_t)cpu_thermal;
		
	if (error == -EINVAL){
		printk(KERN_ERR "error ! thermal_zone_get_zone_by_name - invalid paramenters  ! (err = %d)\n", error);
		return -1;

	} else if (error == -ENODEV ){
		printk(KERN_ERR "error ! thermal_zone_get_zone_by_name - not found  ! (err = %d)\n", error);
		return -1;
	}else if (error == -EEXIST ){
		printk(KERN_ERR "error ! thermal_zone_get_zone_by_name - multiple match  ! (err = %d)\n", error);
		return -1;
	}
	
	
	// ---- SET ZONE GPU TEMP -----
	gpu_thermal = thermal_zone_get_zone_by_name("gpu_thermal");
	error = (uintptr_t)gpu_thermal;
		
	if (error == -EINVAL){
		printk(KERN_ERR "error ! thermal_zone_get_zone_by_name - invalid paramenters  ! (err = %d)\n", error);
		return -1;

	} else if (error == -ENODEV ){
		printk(KERN_ERR "error ! thermal_zone_get_zone_by_name - not found  ! (err = %d)\n", error);
		return -1;
	}else if (error == -EEXIST ){
		printk(KERN_ERR "error ! thermal_zone_get_zone_by_name - multiple match  ! (err = %d)\n", error);
		return -1;
	}
	
	
	// ---- SET STATUS LED (10) -----
	pr_debug ("SET STATUS LED \n");
	error = gpio_is_valid(LED) ;
	if(error == 0) {
		printk(KERN_ERR "error ! gpio_is_valid -  ! (err = %d)\n", error);
		return -1;
	}
	error = gpio_request(LED, "LED") ;
	if(error < 0) {
		printk(KERN_ERR "error ! gpio_request - ! (err = %d)\n", error);
		return -1;
	}
	gpio_direction_output(LED, 0 );
	
	
	// ---- SET TIMER -----
	pr_debug ("SET TIMER  \n");
	timer_setup(&timer, timer_callback, 0);
	mod_timer(&timer, jiffies + msecs_to_jiffies(1000 / freq));


    /* Crée une clase dans sysfs  */
    myClass = class_create(THIS_MODULE, "fan_pwm");
        
    /* Enregistre le device ("device_name") dans la class */
	myDev = device_create(myClass, NULL, devt, NULL,
		      "device_pwm");
		      
	/* Ajoute tous les attributs (mydrv_group) dans le sysfs 
	 * (dans le repertoire du device: /sys/class/myClass/device_pwm/)*/
	error = sysfs_create_group(&myDev->kobj, &mydrv_group);
    if (error) {
		
		pr_err("can't driver_create_file :(\n");

        device_destroy(myClass, devt);
		class_destroy(myClass);

        return error;
    }

	
	return 0; 
}
	
static void __exit skeleton_exit(void)
{
	
	 /* Retire tous les attributs du sysfs */
     sysfs_remove_group(&myDev->kobj, &mydrv_group);
     
    // détruit le device
	device_destroy(myClass, devt);
	// Détruit la classe
	class_destroy(myClass);
	
	// Libère le timer
	del_timer(&timer);
	
	// libère la LED
	gpio_free(LED);
	
	
	pr_info ("Linux pilote skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); 
MODULE_LICENSE ("GPL");
