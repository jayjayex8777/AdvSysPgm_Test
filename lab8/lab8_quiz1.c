#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("22SYS");
MODULE_DESCRIPTION("Keyboard interrupt handler module");

#define KEYBOARD_IRQ 1

/*
 * IRQ handler - This function will be called on keyboard interrupts
 */
irqreturn_t irq_handler(int irq, void *dev_id)
{
    static unsigned char scancode;
    unsigned char status;

    // Read keyboard status and scancode from the keyboard controller
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

/*
 * Module initialization
 */
static int __init irq_ex_init(void)
{
    int ret;

    pr_info("irq_ex_init done\n");
    // Request shared IRQ 1 for the keyboard
    ret = request_irq(KEYBOARD_IRQ, irq_handler, IRQF_SHARED, "keyboard_irq_handler", (void *)(irq_handler));

    return ret;
}

/*
 * Module cleanup
 */
static void __exit irq_ex_exit(void)
{
    // Free the IRQ and pass the same argument as in request_irq()
    free_irq(KEYBOARD_IRQ, (void *)(irq_handler));
    pr_info("irq_ex_exit done\n");
}

module_init(irq_ex_init);
module_exit(irq_ex_exit);
