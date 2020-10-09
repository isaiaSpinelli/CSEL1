/* skeleton.c */
#include <linux/module.h> /* needed by all modules */
#include <linux/init.h> /* needed for macros */ 
#include <linux/kernel.h> /* needed for debugging */
#include <linux/moduleparam.h> /* needed for module parameters */

#include <linux/string.h>
// sprintf
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>

struct element {
	int id;
	char* texte;
	struct list_head node;
};

// definition of the global list
static LIST_HEAD (my_list);

// allocate on element and add it at the tail of the list
void alloc_ele (int id, char* texte) {
	struct element* ele;
	ele = kzalloc(sizeof(*ele), GFP_KERNEL); // create a new element
	if (ele != NULL)
	ele->id = id;
	ele->texte = texte;
	list_add_tail(&ele->node, &my_list); // add element at the end of the list
}


static char* text= "dummy help";
module_param(text, charp, 0);
static int elements= 1;
module_param(elements, int, 0);

static int __init skeleton_init(void)
{
	int i;
	
	pr_info ("Linux module skeleton loaded\n");
	pr_info ("text: %s\nelements: %d\n", text, elements);
	
	
	for(i=0; i < elements; ++i){
		alloc_ele(i, text);
		
	}
	
	
	return 0; }
static void __exit skeleton_exit(void)
{
	struct element* ele;
	list_for_each_entry(ele, &my_list, node) {// iterate over the whole list
		pr_info ("Element id : %d | texte = %s\n",  ele->id, ele->texte); 
		kfree(ele);
	}

	pr_info ("Linux module skeleton unloaded\n"); 
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Isaia Spinelli <isaia.spinelli@hes-so.ch>");
MODULE_DESCRIPTION ("Module skeleton"); MODULE_LICENSE ("GPL");
