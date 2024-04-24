/*
 *   irq_ex1.c: An interrupt handler example. This code binds itself to `IRQ` 1, which
 *   is the IRQ of the keyboard controlled under Intel architectures. Then, when it
 *   receives a keyboard interrupt, it reads the keyboard's status and the scan code,
 *   which is the value return by the keyboard. And then, it puts information about
 *   Key that pressed. This example only has 3 key: ESC, F1 and F2
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("22SYS");

irqreturn_t irq_handler(int irq, void *dev_id)
{
        /*
         * This variables are static because they need to be
         * accessible (through pointers) to the bottom half routine.
         * ESC: 0x01
         * F2: 0x3C
         * F3: 0x3D
         */

        static unsigned char scancode;
        unsigned char status;

        /*
         * Read keyboard status
         */

        return IRQ_HANDLED;
}

/*
 * Initialize the module - register the IRQ handler
 */
static int __init irq_ex_init(void)
{
        /*
         * Request IRQ 1, the keyboard IRQ, to go to our irq_handler.
         */
}
static void __exit irq_ex_exit(void)
{
        /*
         * Free IRQ 1
         */
}

module_init(irq_ex_init);
module_exit(irq_ex_exit);


