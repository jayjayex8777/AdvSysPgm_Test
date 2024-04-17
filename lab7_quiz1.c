#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *thread_st;

// Function executed by kernel thread
static int thread_fn(void *unused)
{
        // print msg every 5 second until removing a module
        while (!kthread_should_stop()) {  // 스레드 종료 요청이 있을 때까지 반복
                pr_info("Thread running\n");
                ssleep(5);  // 5초 동안 대기
        }
    return 0;
}

// Module Initialization
static int __init init_thread(void)
{
        // create timer thread
        printk(KERN_INFO "Creating thread\n");
        thread_st = kthread_run(thread_fn, NULL, "thread_st");
   
    return 0;
}

// Module Exit
static void __exit cleanup_thread(void)
{
        // stop kernel thread
        kthread_stop(thread_st);
        printk(KERN_INFO "thread stopped\n");
}

MODULE_LICENSE("GPL");
module_init(init_thread);
module_exit(cleanup_thread);
