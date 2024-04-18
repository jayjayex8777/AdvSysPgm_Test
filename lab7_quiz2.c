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

static struct task_struct *producer_thread;
static struct task_struct *consumer_thread;

sbuf_t *sbufs = NULL;

#if 0
static int producer(void *arg)
{
	// insert 0-29 integers
	int i;

	for (i = 0; i < ITEMS; i++) {
		sbuf_insert(sbufs, i);
		pr_info("inserted item %d to buf %d\n", item, *(sbufs->buf));
	}
	
	pr_info("Producer Done");
	
	return 0;
}

static int consumer(void *arg)
{
    int i, item;
	
    for (i = 0; i < ITEMS; i++) {
        item = sbuf_remove(sbufs);  // 아이템 제거
        pr_info("Removed item %d from buf %d\n", i, *(sbufs->buf));
    }
    
	pr_info("Consumer Done");
	
    return 0;

}
#else
static int producer(void *arg)
{
	// insert 0-29 integers
	int i;

	while(!kthread_should_stop()){
	

	}
	
	pr_info("Producer Done");
	
	return 0;
}

static int consumer(void *arg)
{
    int i, item;
	
	while(!kthread_should_stop()){
	

	}
    
	pr_info("Consumer Done");
	
    return 0;

}

#endif
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

	producer_thread = kthread_run(producer, NULL, "producer_thread");
	consumer_thread = kthread_run(consumer, NULL, "consumer_thread");

	return 0;
}

static void simple_exit(void)
{
	// deinit sbuf
    if (sbufs) {
        sbuf_deinit(sbufs);
        kfree(sbufs);
        pr_info("buffer freed\n");
    }
	
	if(producer_thread){
		kthread_stop(producer_thread);
		pr_info("producer_thread_stopped successfully\n");
	}
	
	if(consumer_thread){
		kthread_stop(consumer_thread);
		pr_info("producer_thread_stopped successfully\n");
	}
		
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("KOO");

