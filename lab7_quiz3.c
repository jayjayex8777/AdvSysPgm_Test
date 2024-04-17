#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/random.h>

#include "sbuf.h"

/* create an empty, bounded, shared FIFO buffer with n slots */
extern void sbuf_init(sbuf_t * sp, int n);

/* clean up buffer sp */
extern void sbuf_deinit(sbuf_t * sp);

/* insert item onto the rear of shared buffer sp */
extern void sbuf_insert(sbuf_t * sp, int item);

/* remove and return the first item from buffer sp */
extern int sbuf_remove(sbuf_t * sp);

/* if there is items in buffer, remove and return the first item from buffer sp.
   if not, return -1, immediately */
extern int sbuf_tryremove(sbuf_t * sp);

#define ITEMS 30
#define SBUFSIZE 3
#define NUM_SBUF 4
#define NUM_THREADS 2

static struct task_struct **pthreads;
static struct task_struct **cthreads;
sbuf_t *sbufs = NULL;

static int producer(void *arg)
{
        // insert 0-14 integers to a queue
        // the queue is randomly seledted
}

static int consumer(void *arg)
{
        // remove 30 items and print each item
}

static int simple_init(void)
{
        // init 4 sbuf
        // create 2 producer, 1 consumer
}

static void simple_exit(void)
{
        // deinit sbuf
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("KOO");
