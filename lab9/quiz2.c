#include <linux/random.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/workqueue.h>
#include <linux/ktime.h>
#include <linux/delay.h>

/*
 * 0: unsafe
 * 1: fine-grained mutex
 * 2: coarse-grained mutex
 * 3: atomic operations
 */
static int mode = 0;
module_param(mode, int, 0644);

// Declare workers statically
static struct work_struct work1, work2, work3, work4;

static volatile int main_number = 0;

static inline s64 get_current_time_in_ms(void)
{
    return ktime_to_ms(ktime_get());
}

static void number_worker(struct work_struct *unused)
{
    volatile int i;

    for (i = 0; i < 10000000; i++) {
        main_number++;
    }
}

static int __init number_init(void)
{
    int cpu = get_random_u32() % 28;
    s64 time_start, time_end;

    time_start = get_current_time_in_ms();

    switch (mode) {
    case 0:
        pr_info("number: using unprotected mode\n");
        break;
    case 1:
        pr_info("number: using fine-grained mutex\n");
        break;
    case 2:
        pr_info("number: using coarse-grained mutex\n");
        break;
    case 3:
        pr_info("number: using atomic operations\n");
        break;
    default:
        pr_info("number: unsupported mode, defaulting to unprotected mode\n");
        break;
    }

    // Initialize the work_structs
    INIT_WORK(&work1, number_worker);
    INIT_WORK(&work2, number_worker);
    INIT_WORK(&work3, number_worker);
    INIT_WORK(&work4, number_worker);

    // Queue work on [cpu, cpu + 3]
    queue_work_on(cpu, system_wq, &work1);
    queue_work_on(cpu + 1, system_wq, &work2);
    queue_work_on(cpu + 2, system_wq, &work3);
    queue_work_on(cpu + 3, system_wq, &work4);

    pr_info("number: queued 4 works on cpu%d-%d\n", cpu, cpu + 3);

    pr_info("number: flushing workers\n");
    // Flush individual workers
    flush_work(&work1);
    flush_work(&work2);
    flush_work(&work3);
    flush_work(&work4);
    pr_info("number: flushed workers\n");

    time_end = get_current_time_in_ms();

    // Print final number
    pr_info("number: main_number = %d\n", main_number);

    pr_info("number: took %lld ms\n", time_end - time_start);

    return 0;
}

static void __exit number_exit(void)
{
    pr_info("number: exiting module\n");
}

module_init(number_init);
module_exit(number_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("22SYS");

