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

#define ITEMS 30
#define SBUFSIZE 3
#define NUM_SBUF 1
#define NUM_THREADS 1

static struct task_struct *pthreads;
static struct task_struct *cthreads;

sbuf_t *sbufs = NULL;

static int producer(void *arg)
{
	// insert 0-29 integers
	int i;

	for (i = 0; i < ITEMS; i++) {
		sbuf_insert(sbufs, i);
		pr_info("Inserted %d\n", i);
	}
	
	pr_info("Producer Done");
	
	return 0;
}

static int consumer(void *arg)
{
	// remove 30 items and print each item
	int i, item;

	for (i = 0; i < ITEMS; i++) {
		item = sbuf_remove(sbufs);
		pr_info("Removed %d\n", item);
	}

	pr_info("Consumer Done");
	
	return 0;
}

static int simple_init(void)
{
	// init 1 sbuf
	// create 1 producer, 1 consumer
	sbufs = kmalloc(sizeof(sbuf_t), GFP_KERNEL);

	if (!sbufs) {
		pr_err("Failed to allocate memory for sbufs\n");
		return -ENOMEM;
	}
	sbuf_init(sbufs, SBUFSIZE);

	pthreads = kthread_run(producer, NULL, "producer_thread");
	cthreads = kthread_run(consumer, NULL, "consumer_thread");

	return 0;
}

static void simple_exit(void)
{
	// deinit sbuf
   if (pthreads) {
        printk(KERN_INFO "Stopping producer thread\n");
        if (kthread_stop(pthreads) != -EINTR) {
            printk(KERN_INFO "Producer thread stopped successfully\n");
        }
    }
    if (sbufs) {
        sbuf_deinit(sbufs);
        kfree(sbufs);
        printk(KERN_INFO "FIFO buffer freed\n");
    }
    if (cthreads) {
        printk(KERN_INFO "Stopping consumer thread\n");
        if (kthread_stop(cthreads) != -EINTR) {
            printk(KERN_INFO "Consumer thread stopped successfully\n");
        }
    }



    printk(KERN_INFO "Exiting sbuf example module\n");
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("KOO");
