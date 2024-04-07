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
        char *buf;
        spinlock_t lock;

        struct gendisk *gendisk;
};

static struct sbull_dev device;

static inline unsigned int bio_cur_bytes(struct bio *bio)
{
        if (bio_has_data(bio))
                return bio_iovec(bio).bv_len;
        else /* dataless requests such as discard */
                return bio->bi_iter.bi_size;
}

static void sbull_transfer(struct sbull_dev *dev, unsigned long sector,
                           unsigned long nsect, char *buffer, int write)
{
        unsigned long offset = sector * KERNEL_SECTOR_SIZE;
        unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;

        if ((offset + nbytes) > dev->size) {
                pr_err("Beyond-end write (%ld %ld)\n", offset, nbytes);
                return;
        }

        // TODO: Write code here
        // What is buffer, offset, nbytes, write?
        // Hint: The Linux kernel often uses 'int' as 'bool'
        if (write) /* transfer user data to the device */{                
                memcpy(dev->buf + offset, buffer, nbytes);
                pr_info ("Writing %d bytes from 0x%lx\n",nbytes,offset);
        }
        else /* transfer data in the device to user buffer */{
                memcpy(buffer, dev->buf + offset, nbytes);
                pr_info ("Reading %d bytes from 0x%lx\n",nbytes,offset);
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
        dev->buf = vmalloc(dev->size);  /* Allocate DRAM for RamDrive */
        if (dev->buf == NULL) {
                pr_info("vmalloc failure.\n");
                return;
        }
        spin_lock_init(&dev->lock);     /* Initialize spinlock */

        /* gendisk structure */
        dev->gendisk = blk_alloc_disk(NUMA_NO_NODE);
        if (!dev->gendisk) {
                pr_info("alloc_disk failure\n");
                goto out_vfree;
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

        set_capacity(dev->gendisk, nsectors * (hardsect_size / KERNEL_SECTOR_SIZE));

        ret = add_disk(dev->gendisk);
        if (ret != 0) {
                pr_err("Failed to add sbull device: %d\n", ret);
                goto out_vfree;
        }

        return;

 out_vfree:
        if (dev->buf)
                vfree(dev->buf);
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

        return 0;
}

static void sbull_exit(void)
{
        struct sbull_dev *dev = &device;

        if (dev->gendisk) {
                del_gendisk(dev->gendisk);
                put_disk(dev->gendisk);
        }
        if (dev->buf)
                vfree(dev->buf);

        unregister_blkdev(sbull_major, "sbull");
}

module_init(sbull_init);
module_exit(sbull_exit);






