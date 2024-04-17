#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *thread_st;
// Function executed by kernel thread
static int thread_fn(void *unused)
{
        // print msg every 5 second until removing a module
}

// Module Initialization
static int __init init_thread(void)
{
        // create timer thread
}

// Module Exit
static void __exit cleanup_thread(void)
{
        // stop kernel thread
}

MODULE_LICENSE("GPL");
module_init(init_thread);
module_exit(cleanup_thread);
