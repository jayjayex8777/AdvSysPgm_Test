#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/random.h>

#include "sbuf.h"

#define ITEMS 30
#define SBUFSIZE 3
#define NUM_SBUF 4
#define NUM_THREADS 2

static struct task_struct *pthreads[NUM_THREADS];
static struct task_struct *cthread;
static sbuf_t *sbufs;

static int producer(void *arg)
{
    int i, buf_index;
    for (i = 0; i < 15; i++) {
       get_random_bytes(&buf_index, sizeof(buf_index));
       
       if(buf_index < 0)
          buf_index = buf_index % NUM_SBUF;
       
       sbuf_insert(&sbufs[buf_index], i);
    }

    pr_info("Producer Done\n");
    
    return 0;
}

static int consumer(void *arg)
{
    int item, count = 0;
    while (count < ITEMS) {
        int buf_index;
        for (buf_index = 0; buf_index < NUM_SBUF; buf_index++) {
            item = sbuf_tryremove(&sbufs[buf_index]);
            if (item != -1) {
                pr_info("Consumed: %d from buffer %d\n", item, buf_index);
                count++;
                if (count >= ITEMS)
                    break;
            }
        }
    }
    return 0;
}

static int simple_init(void)
{
    int i;
    sbufs = kmalloc(NUM_SBUF * sizeof(sbuf_t), GFP_KERNEL);
    for (i = 0; i < NUM_SBUF; i++) {
        sbuf_init(&sbufs[i], SBUFSIZE);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthreads[i] = kthread_run(producer, NULL, "producer_thread_%d", i);
    }
   if(0)
    cthread = kthread_run(consumer, NULL, "consumer_thread");

    return 0;
}

static void simple_exit(void)
{
    int i;
    for (i = 0; i < NUM_SBUF; i++) {
        sbuf_deinit(&sbufs[i]);
    }
    kfree(sbufs);
   #if 0
    for (i = 0; i < NUM_THREADS; i++) {
        if (pthreads[i])
            kthread_stop(pthreads[i]);
    }
    if (cthread)
        kthread_stop(cthread);
   #endif
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("KOO");
