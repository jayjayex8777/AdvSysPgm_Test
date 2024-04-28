
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
#define KEYBOARD_IRQ 1

static struct task_struct *pthreads;
static struct task_struct *cthreads;
static volatile int exit_flag = 0, enqueue_flag = 0, dequeue_flag = 0;
sbuf_t *sbufs = NULL;

static struct workqueue_struct *my_workqueue;
static struct work_struct my_enqueue_work;
static struct work_struct my_dequeue_work;
static struct work_struct my_exit_work;

static void do_enqueue_work(struct work_struct *work)
{
    pr_info("WORK You pressed F2\n");
    enqueue_flag = 1;
}

static void do_dequeue_work(struct work_struct *work)
{
    pr_info("WORK You pressed F3\n");
    dequeue_flag = 1;
}

static void do_exit_work(struct work_struct *work)
{
    pr_info("WORK You pressed ESC\n");
    exit_flag = 1;
}


irqreturn_t irq_handler(int irq, void *dev_id)
{
    static unsigned char scancode;
    unsigned char status;
    
    status = inb(0x64);
    scancode = inb(0x60);

    switch (scancode)
    {
        case 0x01:
            pr_info("! You pressed ESC ...\n");
	queue_work(my_workqueue, &my_exit_work);
            break;

        case 0x3C:
            pr_info("! You pressed F2 ...\n");
            queue_work(my_workqueue, &my_enqueue_work);
            break;

        case 0x3D:
            pr_info("! You pressed F3 ...\n");
            queue_work(my_workqueue, &my_dequeue_work);
            break;
    }

    return IRQ_HANDLED;
}

static int producer(void *arg)
{
    int val;
    
    while (!kthread_should_stop()) {
        
        if (enqueue_flag) {
            sbuf_insert(sbufs, val);
            
            pr_info("Producer enqueued item: %d\n",val);
            
            val++;
            
            enqueue_flag = 0;
        }
        
        msleep(100);

        if(exit_flag)
            break;
    }
    
    pr_info("Producer has terminated\n");
    
    return 0;
}

static int consumer(void *arg)
{
    int item;
    
    while (!kthread_should_stop()) {
    
        if (dequeue_flag) {
            item = sbuf_remove(sbufs);
            pr_info("Consumer dequeued item: %d\n",item);
            dequeue_flag = 0;
        }

        msleep(100);

        if(exit_flag)
            break;
    }
        
    pr_info("Consumer has terminated\n");
    
    return 0;
}

static int simple_init(void)
{
    int ret;

    sbufs = (sbuf_t *) kmalloc(sizeof(sbuf_t) * NUM_SBUF, GFP_KERNEL);
    sbuf_init(&sbufs[0], SBUFSIZE);
    
    my_workqueue = create_workqueue("my_workqueue");
    INIT_WORK(&my_enqueue_work, do_enqueue_work);
    INIT_WORK(&my_dequeue_work, do_dequeue_work);
    INIT_WORK(&my_exit_work, do_exit_work);
        
    ret = request_irq(KEYBOARD_IRQ, irq_handler, IRQF_SHARED, "keyboard_irq_handler", (void *)(irq_handler));
        
    /* create a producer */
    /* create a consumer */
    pthreads = kthread_run(producer, NULL, "producer_thread");
    if (IS_ERR(pthreads)) {
        pr_err("Failed to create pthread\n");
    } 
    else {
        pr_info("pthread created successfully\n");
    }
        
    cthreads = kthread_run(consumer, NULL, "consumer_thread");
    if (IS_ERR(cthreads)) {
        pr_err("Failed to create cthread\n");
    } 
    else {
        pr_info("cthread created successfully\n");
    }

    return ret;
}

static void simple_exit(void)
{
    if (pthreads){
        if(!exit_flag){
            kthread_stop(pthreads);
            pr_info("pthread stopped successfully\n");
        }
        else 
            pr_info("pthread already stopped\n");
        }
    
    if (cthreads){ 
        if(!exit_flag){
            kthread_stop(cthreads);  
            pr_info("cthread stopped successfully\n");
        }
        else 
            pr_info("cthread already stopped\n");
    }

    /*
    * free irq, free tasklet
    */ 
    free_irq(KEYBOARD_IRQ, (void *)(irq_handler));
    pr_info("KBD IRQ Freed\n");

    destroy_workqueue(my_workqueue);
    pr_info("my workqueue destroyed\n");
        
    if(sbufs){
        kfree(sbufs);
        pr_info("sbuf freed\n");
          sbufs = NULL;
    }
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");


MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("Intae Jun");



