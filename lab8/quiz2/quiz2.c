#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include "sbuf.h"

/* create an empty, bounded, shared FIFO buffer with n slots */
extern void sbuf_init(sbuf_t * sp, int n);

/* clean up buffer sp */
extern void sbuf_deinit(sbuf_t * sp);

/* insert item onto the rear of shared buffer sp */
extern void sbuf_insert(sbuf_t * sp, int item);

/* remove and return the first item from buffer sp */
extern int sbuf_remove(sbuf_t * sp);

#define SBUFSIZE 3
#define NUM_SBUF 1
#define NUM_THREADS 1

static struct task_struct **pthreads;
static struct task_struct **cthreads;
static volatile int exit_flag = 0, enqueue_flag = 0, dequeue_flag = 0;
sbuf_t *sbufs = NULL;

/*
 * declare three tasklets (Esc, F2, F3)
 */
static void do_enqueue_tasklet(struct tasklet_struct *unused);
static DECLARE_TASKLET(my_enqueue_tasklet, do_enqueue_tasklet);

static void do_dequeue_tasklet(struct tasklet_struct *unused);
static DECLARE_TASKLET(my_dequeue_tasklet, do_dequeue_tasklet);

static void do_exit_tasklet(struct tasklet_struct *unused);
static DECLARE_TASKLET(my_exit_tasklet, do_exit_tasklet);

/*
 * declare three tasklet functions (Esc, F2, F3)
 */
static void do_dequeue_tasklet(struct tasklet_struct *unused)
{
        pr_info("TASKLET You pressed F2\n");
        enqueue_flag = true;
}

static void do_dequeue_tasklet(struct tasklet_struct *unused)
{
        pr_info("TASKLET You pressed F3\n");
        dequeue_flag = true;
}

static void do_exit_tasklet(struct tasklet_struct *unused)
{
        pr_info("TASKLET You pressed ESC\n");
        exit_flag = true;
}


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
            pr_info("! You pressed ESC ...\n");
                tasklet_schedule(&my_exit_tasklet);
                pr_info("! You pressed ESC ...\n");

            break;
        case 0x3C:
            pr_info("! You pressed F2 ...\n");
                tasklet_schedule(&my_enqueue_tasklet);
            break;
        case 0x3D:
            pr_info("! You pressed F3 ...\n");
                tasklet_schedule(&my_dequeue_tasklet);
            break;
    }

    return IRQ_HANDLED;
}

static int producer(void *arg)
{
int val;
    while (!exit_flag) {
        if (enqueue_flag) {
            sbuf_insert(sbufs, val);
                pr_info("Producer enqueued item: %d\n",val);
                val++;
            enqueue_flag = 0;
        }
        msleep(100); // Sleep to simulate work
    }

pr_info("Producer has terminated\n");
    return 0;
}

static int consumer(void *arg)
{
        int item;
    while (!exit_flag) {
        if (dequeue_flag) {
            item = sbuf_remove(sbufs);
                pr_info("Consumer dequeued item: %d\n",item);
            dequeue_flag = 0;
        }
        msleep(100); // Sleep to simulate work
    }
        
        pr_info("Consumer has terminated\n");
    return 0;
}

static int simple_init(void)
{
        int ret;
        
        pthreads = (struct task_struct **)kmalloc(sizeof(struct task_struct *) * NUM_THREADS, GFP_KERNEL);
        cthreads = (struct task_struct **)kmalloc(sizeof(struct tast_struct *) * NUM_THREADS, GFP_KERNEL);
        sbufs = (sbuf_t *) kmalloc(sizeof(sbuf_t) * NUM_SBUF, GFP_KERNEL);

        sbuf_init(&sbufs[0], SBUFSIZE);
        
        ret = request_irq(KEYBOARD_IRQ, irq_handler, IRQF_SHARED, "keyboard_irq_handler", (void *)(irq_handler));
        
        /* create a producer */
        /* create a consumer */
        pthreads = kthread_run(producer, NULL, "producer_thread");
        if (IS_ERR(pthreads)) {
                pr_err("Failed to create pthread\n");
        } 
        else {
            pr_err("pthread created successfully\n");
        }
        
        cthreads = kthread_run(consumer, NULL, "consumer_thread");
        if (IS_ERR(pthreads)) {
            pr_err("Failed to create pthread\n");
        } 
        else {
            pr_err("pthread created successfully\n");
        }

        return ret;
}

static void simple_exit(void)
{
        if (pthreads){ 
                kthread_stop(pthreads[i]);
                pr_info("pthread stopped successfully\n");
        }
        if (cthreads){ 
                kthread_stop(cthreads[i]);
                pr_info("cthread stopped successfully\n");
        }
        /*
         * free irq, free tasklet
         */
        kfree(pthreads);
        kfree(cthreads);
        kfree(sbufs);

        tasklet_kill(&my_enqueue_tasklet);
        tasklet_kill(&my_dequeue_tasklet);
        tasklet_kill(&my_exitqueue_tasklet);
}

module_init(simple_init);       // 모듈 생성될 때 simpel_init 함수 호출
module_exit(simple_exit);       // 모듈 생성될 때 simpel_exit 함수 호출

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("KOO");


