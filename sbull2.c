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
#include <linux/list.h>         /* linked list */

MODULE_LICENSE("Dual BSD/GPL");

static int sbull_major = 0;
static int hardsect_size = 512;
static int nsectors = 1024 * 1024; /* How big the drive is */

#define KERNEL_SECTOR_SIZE 512

struct sbull_dev {
    int size;
    int users;
    spinlock_t lock;
    struct gendisk *gendisk;
    struct list_head data_list; // List head
};

struct sbull_data {
    struct list_head list; // Linked list
    unsigned long sector;  // Sector number
    char data[4096];       // Data buffer
};

static struct sbull_dev device;

static inline unsigned int bio_cur_bytes(struct bio *bio) {
    if (bio_has_data(bio))
        return bio_iovec(bio).bv_len;
    else /* dataless requests such as discard */
        return bio->bi_iter.bi_size;
}

static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
                           unsigned long nsect, char *buffer, int write) {
    struct list_head *ptr;
    struct sbull_data *data;
    unsigned long offset = sector * KERNEL_SECTOR_SIZE;
    unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;

    list_for_each(ptr, &dev->data_list) {
        data = list_entry(ptr, struct sbull_data, list);
        if (data->sector == sector) {
            if (write) memcpy(data->data, buffer, nbytes);
            else memcpy(buffer, data->data, nbytes);
            return;
        }
    }

    // Sector not found, so allocate new
    data = kmalloc(sizeof(struct sbull_data), GFP_KERNEL);
    if (!data) {
        pr_err("sbull: out of memory\n");
        return;
    }
    data->sector = sector;
    if (write) memcpy(data->data, buffer, nbytes);
    list_add(&data->list, &dev->data_list);
}

static int sbull_xfer_bio(struct sbull_dev *dev, struct bio *bio) {
    struct bvec_iter iter;
    struct bio_vec bvec;
    sector_t sector = bio->bi_iter.bi_sector;

    bio_for_each_segment(bvec, bio, iter) {
        char *buffer = kmap_atomic(bvec.bv_page) + bvec.bv_offset;
        sbull_transfer(dev, sector, bio_cur_bytes(bio) >> 9, buffer, bio_data_dir(bio) == WRITE);
        sector += bio_cur_bytes(bio) >> 9;
        kunmap_atomic(buffer);
    }
    return 0; /* Always "succeed" */
}

static void sbull_make_request(struct bio *bio) {
    struct sbull_dev *dev = bio->bi_bdev->bd_disk->queue->queuedata;
    sbull_xfer_bio(dev, bio);
    bio_endio(bio);
}

static int sbull_open(struct gendisk *disk, fmode_t mode) {
    struct sbull_dev *dev = disk->private_data;
    spin_lock(&dev->lock);
    dev->users++;
    spin_unlock(&dev->lock);
    return 0;
}

static void sbull_release(struct gendisk *disk, fmode_t mode) {
    struct sbull_dev *dev = disk->private_data;
    spin_lock(&dev->lock);
    dev->users--;
    spin_unlock(&dev->lock);
}

static struct block_device_operations sbull_ops = {
    .owner = THIS_MODULE,
    .open = sbull_open,
    .release = sbull_release,
};

static void setup_device(struct sbull_dev *dev) {
    memset(dev, 0, sizeof(*dev));
    dev->size = nsectors * hardsect_size;
    spin_lock_init(&dev->lock);
    INIT_LIST_HEAD(&dev->data_list);

    dev->gendisk = blk_alloc_disk(NUMA_NO_NODE);
    if (!dev->gendisk) {
        pr_err("sbull: alloc_disk failure\n");
        return;
    }

    dev->gendisk->major = sbull_major;
    dev->gendisk->first_minor = 0;
    dev->gendisk->minors = 1;
    dev->gendisk->fops = &sbull_ops;
    dev->gendisk->queue = blk_alloc_queue(GFP_KERNEL);
    blk_queue_make_request(dev->gendisk->queue, sbull_make_request);
    dev->gendisk->private_data = dev;
    snprintf(dev->gendisk->disk_name, 32, "sbull0");
    set_capacity(dev->gendisk, nsectors);
    add_disk(dev->gendisk);
}

static int __init sbull_init(void) {
    sbull_major = register_blkdev(sbull_major, "sbull");
    if (sbull_major <= 0) {
        pr_err("sbull: unable to get major number\n");
        return -EBUSY;
    }
    setup_device(&device);
    return 0;
}

static void __exit sbull_exit(void) {
    struct sbull_dev *dev = &device;
    struct sbull_data *tmp, *next;

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
