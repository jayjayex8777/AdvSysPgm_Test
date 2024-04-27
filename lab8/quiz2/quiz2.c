#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/delay.h>

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
        /*
         * This variables are static because they need to be
         * accessible (through pointers) to the bottom half routine.
         */

        static unsigned char scancode;
        unsigned char status;

        /*
         * Read keyboard status
         */
        return IRQ_HANDLED;
}

static int producer(void *arg)
{
        while (1) {
                /*
                 * exit or insert
                 */
        }
}

static int consumer(void *arg)
{
        while (1) {
                /*
                 * exit or remove
                 */
        }

}

static int simple_init(void)
{

        pthreads = (struct task_struct **)kmalloc(sizeof(struct task_struct *) * NUM_THREADS, GFP_KERNEL);
        cthreads = (struct task_struct **)kmalloc(sizeof(struct tast_struct *) * NUM_THREADS, GFP_KERNEL);
        sbufs = (sbuf_t *) kmalloc(sizeof(sbuf_t) * NUM_SBUF, GFP_KERNEL);

        sbuf_init(&sbufs[0], SBUFSIZE);
        /* create a producer */
        /* create a consumer */

        return 0;
}

static void simple_exit(void)
{
        /*
         * free irq, free tasklet
         */
        kfree(pthreads);
        kfree(cthreads);
        kfree(sbufs);
}

module_init(simple_init);       // 모듈 생성될 때 simpel_init 함수 호출
module_exit(simple_exit);       // 모듈 생성될 때 simpel_exit 함수 호출

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("KOO");


