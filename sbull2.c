#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>
#include <linux/list.h>

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
    struct list_head data_list; /* the head of the data list */
};

struct sbull_sector {
    struct list_head list;
    unsigned long sector;
    char *data;
};

static struct sbull_dev device;

static inline unsigned int bio_cur_bytes(struct bio *bio) {
    if (bio_has_data(bio))
        return bio_iovec(bio).bv_len;
    else /* dataless requests such as discard */
        return bio->bi_iter.bi_size;
}

/* Function to find or create a sector */
static struct sbull_sector *sbull_get_sector(struct sbull_dev *dev, unsigned long sector) {
    struct sbull_sector *blk;
    list_for_each_entry(blk, &dev->data_list, list) {
        if (blk->sector == sector) return blk;
    }

    /* Not found, so allocate a new sector */
    blk = kmalloc(sizeof(*blk), GFP_KERNEL);
    if (!blk) return NULL;

    blk->data = kmalloc(KERNEL_SECTOR_SIZE, GFP_KERNEL);
    if (!blk->data) {
        kfree(blk);
        return NULL;
    }

    blk->sector = sector;
    memset(blk->data, 0, KERNEL_SECTOR_SIZE);
    list_add(&blk->list, &dev->data_list);
    return blk;
}

static void sbull_transfer(struct sbull_dev *dev, unsigned long sector, unsigned long nsect, char *buffer, int write) {
    unsigned long offset = sector * KERNEL_SECTOR_SIZE;
    unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;

    if ((offset + nbytes) > dev->size) {
        pr_err("Beyond-end write (%ld %ld)\n", offset, nbytes);
        return;
    }

    while (nbytes > 0) {
        struct sbull_sector *blk = sbull_get_sector(dev, sector);
        if (!blk) {
            pr_err("Out of memory at sector %lu\n", sector);
            return;
        }

        if (write) memcpy(blk->data, buffer, KERNEL_SECTOR_SIZE);
        else memcpy(buffer, blk->data, KERNEL_SECTOR_SIZE);

        sector++;
        buffer += KERNEL_SECTOR_SIZE;
        nbytes -= KERNEL_SECTOR_SIZE;
    }
}

static int sbull_xfer_bio(struct sbull_dev *dev, struct bio *bio) {
    struct bio_vec bvec;
    struct bvec_iter iter;
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
    int status = sbull_xfer_bio(dev, bio);
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
    memset(dev, 0, sizeof(struct sbull_dev));
    dev->size = nsectors * hardsect_size;
    spin_lock_init(&dev->lock);
    INIT_LIST_HEAD(&dev->data_list);

    dev->gendisk = blk_alloc_disk(NUMA_NO_NODE);
    if (!dev->gendisk) {
        pr_info("alloc_disk failure\n");
        return;
    }

    dev->gendisk->major = sbull_major;
    dev->gendisk->first_minor = 0;
    dev->gendisk->fops = &sbull_ops;
    dev->gendisk->queue = blk_alloc_queue(GFP_KERNEL);
    blk_queue_make_request(dev->gendisk->queue, sbull_make_request);
    dev->gendisk->queue->queuedata = dev;
    snprintf(dev->gendisk->disk_name, 32, "sbull%d", 0);
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
    struct sbull_sector *blk, *next;

    list_for_each_entry_safe(blk, next, &dev->data_list, list) {
        list_del(&blk->list);
        kfree(blk->data);
        kfree(blk);
    }

    if (dev->gendisk) {
        del_gendisk(dev->gendisk);
        put_disk(dev->gendisk);
    }
    unregister_blkdev(sbull_major, "sbull");
}

module_init(sbull_init);
module_exit(sbull_exit);

