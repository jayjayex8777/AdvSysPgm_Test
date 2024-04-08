


/*
 * Sample disk driver, from the beginning.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/timer.h>
#include <linux/types.h>        /* size_t */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/hdreg.h>        /* HDIO_GETGEO */
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>  /* invalidate_bdev */
#include <linux/bio.h>
#include <linux/list.h>

MODULE_LICENSE("Dual BSD/GPL");

static int sbull_major = 0;
static int hardsect_size = 512;
static int nsectors = 1024 * 1024;      /* How big the drive is */

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE      512

/*
 * The internal representation of our device.
 */
struct sbull_dev {
        int size;
        int users;
        spinlock_t lock;

        struct gendisk *gendisk;
        struct list_head data_list;
};

static struct sbull_dev device;

struct sbull_list {
    struct list_head list;
    unsigned long idx;
    char buf[4096];
};
// TODO: You can declare global variables too

static inline unsigned int bio_cur_bytes(struct bio *bio)
{
        if (bio_has_data(bio))
                return bio_iovec(bio).bv_len;
        else /* dataless requests such as discard */
                return bio->bi_iter.bi_size;
}

static struct sbull_list* sbull_find_list(struct sbull_dev *dev, unsigned long sector) {
    struct sbull_list *tmp;
    list_for_each_entry(tmp, &dev->data_list, list) {
        if (tmp->idx == sector) return tmp;
    }
    return NULL;
}

static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
                           unsigned long nsect, char *buffer, int write)
{
        unsigned long offset = sector * KERNEL_SECTOR_SIZE;
        unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;
        struct sbull_list *tmp;

        if ((offset + nbytes) > dev->size) {
                pr_err("Beyond-end write (%ld %ld)\n", offset, nbytes);
                return;
        }

        // TODO: Memory allocation, data structure management, kmalloc, memcpy, etc...
        tmp = sbull_find_list(dev, sector);
        if (!tmp && write) {
                tmp = kmalloc(sizeof(struct sbull_list), GFP_KERNEL);
                if (!tmp) {
                    pr_err("sbull: Out of memory\n");
            return;
        }
        memset(tmp->buf, 0, 4096);
        tmp->idx = sector;
        list_add_tail(&tmp->list, &dev->data_list);
    }

    if (tmp) {
        if (write){
			pr_info("Writing %ld bytes on the memory(%p) for idx =%ld\n",nbytes,tmp,tmp->idx);
			memcpy(tmp->buf, buffer, nbytes);
        }
        else{
            memcpy(buffer, tmp->buf, nbytes);
			pr_info("Reading %ld bytes on the memory(%p) for idx =%ld\n",nbytes,tmp,tmp->idx);
        }
    }
}

/*
 * Transfer a single BIO.
 */
static int sbull_xfer_bio(struct sbull_dev *dev, struct bio *bio)
{
        struct bvec_iter iter;
        struct bio_vec bvec;
        sector_t sector = bio->bi_iter.bi_sector;

        // Process each and every segment
        bio_for_each_segment(bvec, bio, iter) {
                // Map a kernel page for I/O
                char *buffer = kmap_atomic(bvec.bv_page) + bvec.bv_offset;
                // Read from or write to the buffer
                sbull_transfer(dev, sector, bio_cur_bytes(bio) >> 9, buffer, bio_data_dir(bio) == WRITE);
                sector += bio_cur_bytes(bio) >> 9;
                // Free the mapped kernel page
                kunmap_atomic(buffer);
        }

        return 0;               /* Always "succeed" */
}

/*
 * The direct make request version.
 */
static void sbull_make_request(struct bio *bio)
{
        struct sbull_dev *dev = bio->bi_bdev->bd_disk->queue->queuedata;
        int status;

        status = sbull_xfer_bio(dev, bio);
        bio_endio(bio);
}

/*
 * Open and close.
 */
/* changed open and release function */

static int sbull_open(struct gendisk *disk, fmode_t mode)
{
        struct sbull_dev *dev = disk->private_data;

        spin_lock(&dev->lock);
        dev->users++;
        spin_unlock(&dev->lock);
        
        return 0;
}

static void sbull_release(struct gendisk *disk)
{
        struct sbull_dev *dev = disk->private_data;

        spin_lock(&dev->lock);
        dev->users--;
        spin_unlock(&dev->lock);
}

/*
 * The device operations structure.
 */
static struct block_device_operations sbull_ops = {
        .open = sbull_open,
        .release = sbull_release,
        .submit_bio = sbull_make_request,
};

/*
 * Set up our internal device.
 */
static noinline void setup_device(struct sbull_dev *dev)
{
        int ret;

        memset(dev, 0, sizeof(struct sbull_dev));
        dev->size = nsectors * hardsect_size;
        spin_lock_init(&dev->lock);     /* Initialize spinlock */
        
        INIT_LIST_HEAD(&dev->data_list);
        
        /* gendisk structure */
        dev->gendisk = blk_alloc_disk(NUMA_NO_NODE);
        if (!dev->gendisk) {
                pr_info("alloc_disk failure\n");
                return;
        }

        dev->gendisk->queue->queuedata = dev;

        /*
         * And the gendisk structure.
         */
        dev->gendisk->major = sbull_major;
        dev->gendisk->first_minor = 0;
        dev->gendisk->minors = 1;
        dev->gendisk->fops = &sbull_ops;
        dev->gendisk->private_data = dev;       /* register the private data structure */

        snprintf(dev->gendisk->disk_name, 32, "sbull0");

        /*
         * To ensure that we always get PAGE_SIZE aligned
         * and n*PAGE_SIZED sized I/O requests.
         */
        blk_queue_physical_block_size(dev->gendisk->queue, PAGE_SIZE);
        blk_queue_logical_block_size(dev->gendisk->queue, 1 << 12);
        blk_queue_io_min(dev->gendisk->queue, PAGE_SIZE);

        set_capacity(dev->gendisk, nsectors * (hardsect_size / KERNEL_SECTOR_SIZE));

        ret = add_disk(dev->gendisk);
        if (ret != 0) {
                pr_err("Failed to add sbull device: %d\n", ret);
        }

        return;
}

static int __init sbull_init(void)
{
        /*
         * Get registered.
         */
        sbull_major = register_blkdev(0, "sbull");
        if (sbull_major <= 0) {
                pr_err("sbull: unable to get major number\n");
                return -EBUSY;
        }

        setup_device(&device);

        // TODO: You can add data structure initialization here if needed

        return 0;
}

static void sbull_exit(void)
{
        struct sbull_dev *dev = &device;
        struct sbull_list *tmp, *next;


  list_for_each_entry_safe(tmp, next, &dev->data_list, list) {
        list_del(&tmp->list);
        kfree(tmp);
    }

    del_gendisk(dev->gendisk);
    put_disk(dev->gendisk);
    unregister_blkdev(sbull_major, "sbull");
}

module_init(sbull_init);
module_exit(sbull_exit);

