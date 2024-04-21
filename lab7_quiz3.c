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

static struct task_struct *pthreads[NUM_THREADS];
static struct task_struct *cthread;
sbuf_t *sbufs = NULL;
static int thread_ids[NUM_THREADS];

static int buf_val_array[30] = {0,};
static int buf_val_count[30] = {0,};
static int test_val[30] = {-1,};

static int producer(void *arg)
{
	int i, buf_index;
	int thread_id = *(int*)arg;

	for (i = 0; i < 15; i++) {
		get_random_bytes(&buf_index, sizeof(buf_index));

		if(buf_index < 0){
			buf_index = - buf_index;
		}

		buf_index = buf_index % NUM_SBUF; 		

		sbuf_insert(&sbufs[buf_index], i);
#if 0
		pr_info("Inserting %d by producer_thread[%d]\n",i,thread_id);
		for(int j=0;j<4;j++){
			pr_info("Buffer Queue[%d] : ",j);
			for(int k=0; k<3; k++){
				pr_info(" %d ",sbufs[j].buf[k]);
			}
			pr_info("\n");
		}
		pr_info("\n");
#endif
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
				pr_info("Consumed item %d = %d from queue %d\n",count,item,buf_index);
				buf_val_array[count] = item;
				count++;

				if (count >= ITEMS)
					break;
			}
		}	
	}

	pr_info("Consumer Done\n");

#if 1
	for(count=0; count<ITEMS; count++){
		buf_val_count[buf_val_array[count]]++;		
	}
	for (count=0; count<15; count++){
		pr_info("buf val %d = evicted %d times\n",count,buf_val_count[count]);
		
	}
#endif
	
#if 1	
	for(int j=0;j<4;j++){
		pr_info("Buffer Queue[%d] : ",j);
		
		for(int k=0; k<3; k++){
			pr_info(" %d ",sbufs[j].buf[k]);
		}
		pr_info("\n");
	}
	pr_info("\n");
#endif	
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
		thread_ids[i] = i;
		pthreads[i] = kthread_run(producer, &thread_ids[i], "producer_thread_%d", i);
	}

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
	pr_info("sbufs freed all\n");
	
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
