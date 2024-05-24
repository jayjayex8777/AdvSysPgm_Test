
#include <linux/random.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

/*
 * 0: unsafe
 * 1: fine-grained mutex
 * 2: coarse-grained mutex
 * 3: atomic operations
 */
static int mode = 0;
module_param(mode, int, 0644);

// TODO: Declare workers statically here or in number_init() (Q1)

static volatile int main_number = 0;
// TODO: Declare an atomic number (Q3)

// TODO: Declare a mutex (Q2)

static inline s64 get_current_time_in_ms(void)
{
        return ktime_to_ms(ktime_get());
}

static void number_worker(struct work_struct *unused)
{
        volatile int i;

        // TODO: Fill code (Q2)
        for (i = 0; i < 10000000; i++) {
                // TODO: Fill code (Q2)

                // TODO: if mode is atomic, use atomic operation (Q3)
                main_number++;

                // TODO: Fill code (Q2)
        }
        // TODO: Fill code (Q2)
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
        }

        // TODO: Queue work on [cpu, cpu + 3] using system_wq (Q1)
        pr_info("number: queued 4 works on cpu%d-%d\n", cpu, cpu + 3);

        pr_info("number: flushing workers\n");
        // TODO: Flush queued workers (Q1)
        pr_info("number: flushed workers\n");

        time_end = get_current_time_in_ms();

        // TODO: if mode is atomic, use atomic operation (Q3)
        pr_info("number: main_number = %d\n", main_number);

        pr_info("number: took %lld ms\n", time_end - time_start);

        return 0;
}

static void __exit number_exit(void)
{
}

module_init(number_init);
module_exit(number_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("22SYS");
