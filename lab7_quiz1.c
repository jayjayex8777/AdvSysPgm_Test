#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *thread_st;
static int example_thread;
// Function executed by kernel thread
static int thread_fn(void *unused)
{
        // print msg every 5 second until removing a module
        while (!kthread_should_stop()) {  // 스레드 종료 요청이 있을 때까지 반복
                pr_info("Thread running\n");
                ssleep(5);  // 5초 동안 대기
        }
    printk(KERN_INFO "Thread stopping\n");
    return 0;
}

// Module Initialization
static int __init init_thread(void)
{
        // create timer thread
        printk(KERN_INFO "Creating thread\n");
        example_thread = kthread_run(thread_fn, NULL, "example_thread");
        if (IS_ERR(example_thread)) {
                printk(KERN_ALERT "Failed to create the thread, returned %ld\n", PTR_ERR(example_thread));
        return PTR_ERR(example_thread);
        }
    
    return 0;
}

// Module Exit
static void __exit cleanup_thread(void)
{
        // stop kernel thread
        printk(KERN_INFO "Example Module: Exiting\n");
        if (example_thread) {
                kthread_stop(example_thread);
                printk(KERN_INFO "Example Thread: Successfully stopped\n");
        }
}

MODULE_LICENSE("GPL");
module_init(init_thread);
module_exit(cleanup_thread);
