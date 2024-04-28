#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#define KEYBOARD_IRQ 1

irqreturn_t irq_handler(int irq, void *dev_id)
{
    static unsigned char scancode;
    unsigned char status;

    status = inb(0x64);
    scancode = inb(0x60);

    switch (scancode)
    {
        case 0x01:
            printk(KERN_INFO "ESC key pressed\n");
            break;
        case 0x3B:
            printk(KERN_INFO "F1 key pressed\n");
            break;
        case 0x3C:
            printk(KERN_INFO "F2 key pressed\n");
            break;
    }

    return IRQ_HANDLED;
}

static int __init irq_ex_init(void)
{
    int ret;

    pr_info("irq_ex_init done\n");

    ret = request_irq(KEYBOARD_IRQ, irq_handler, IRQF_SHARED, "keyboard_irq_handler", (void *)(irq_handler));

    return ret;
}

static void __exit irq_ex_exit(void)
{    
    free_irq(KEYBOARD_IRQ, (void *)(irq_handler));
    pr_info("irq_ex_exit done\n");
}

module_init(irq_ex_init);
module_exit(irq_ex_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Intae Jun");
MODULE_DESCRIPTION("Keyboard interrupt handler module");
