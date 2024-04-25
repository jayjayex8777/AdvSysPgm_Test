#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "sbuf.h"

/* create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t * sp, int n)
{
        sp->buf = kmalloc(n * sizeof(int), GFP_KERNEL);
        sp->n = n;              /* Buffer holds max of n items */
        sp->front = sp->rear = 0;       /* Empty buffer iff front == rear */
        sema_init(&sp->mutex, 1);       /* Binary semaphore for locking */
        sema_init(&sp->slots, n);       /* Initially, buf has n empty slots */
        sema_init(&sp->items, 0);       /* Initially, buf has zero data items */
}

/* clean up buffer sp */
void sbuf_deinit(sbuf_t * sp)
{
        kfree(sp->buf);
}

/* insert item onto the rear of shared buffer sp */
void sbuf_insert(sbuf_t * sp, int item)
{
        down(&sp->slots);       /* Wait for available slot */
        down(&sp->mutex);       /* Lock the buffer */
        sp->buf[(++sp->rear) % (sp->n)] = item; /* Insert the item */
        up(&sp->mutex);         /* Unlock the buffer */
        up(&sp->items);         /* Announce available item */
}

/* remove and return the first item from buffer sp */
int sbuf_remove(sbuf_t * sp)
{
        int item;
        down(&sp->items);       /* Wait for available item */
        down(&sp->mutex);       /* Lock the buffer */
        item = sp->buf[(++sp->front) % (sp->n)];        /* Remove the item */
        up(&sp->mutex);         /* Unlock the buffer */
        up(&sp->slots);         /* Announce available slot */
        return item;
}

int sbuf_tryremove(sbuf_t * sp)
{
        int item;
        int rc;
        rc = down_trylock(&sp->items);
        if (rc == 1) {
                return -1;
        }
        down(&sp->mutex);       /* Lock the buffer */
        item = sp->buf[(++sp->front) % (sp->n)];        /* Remove the item */
        up(&sp->mutex);         /* Unlock the buffer */
        up(&sp->slots);         /* Announce available slot */
        return item;
}

static int simple_init(void)
{
        pr_info("Loading sbuf\n");
        return 0;
}

static void simple_exit(void)
{
        pr_info("Removing sbuf\n");
}

module_init(simple_init);
module_exit(simple_exit);

EXPORT_SYMBOL(sbuf_init);
EXPORT_SYMBOL(sbuf_deinit);
EXPORT_SYMBOL(sbuf_insert);
EXPORT_SYMBOL(sbuf_remove);
EXPORT_SYMBOL(sbuf_tryremove);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("KOO");





