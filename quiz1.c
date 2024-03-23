#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>

int kernel_init(void)
{
    /* TODO : Write your codes */
    printk("hello\n");

    return 0;
}

void kernel_exit(void)
{
    /* TODO : Write your codes */
    printk("bye\n");
}

module_init(kernel_init);
module_exit(kernel_exit);

MODULE_LICENSE("GPL");  
