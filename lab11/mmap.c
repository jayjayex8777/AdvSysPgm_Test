define DEV_NAME "mmap_test"
#define pr_fmt(fmt) DEV_NAME ": " fmt

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>           /* mmap related stuff */

/* Memory mapping: provides user programs with direct access to device memory
 * Mapped area must be multiple of PAGE_SIZE, and starting address aligned to
 * PAGE_SIZE
 *
 * syscall: mmap(caddr_t addr, size_t len, int ptro, int flags, int fd, off_t off)
 * file operation: int (*mmap)(struct file *f, struct vm_area_struct *vma)
 *
 * Driver needs to: build page tables for address range, and replace vma->vm_ops
 * Building page tables:
 *    - all at once: remap_page_range
 *    - one page at a time: nopage method. Finds correct page for address, and
 *      increments its reference cout. Must be implemented if driver supports
 *      mremap syscall
 *
 * fields in struct vm_area_struct:
 *     unsigned long vm_start, vm_end: virtual address range covered
 *                   by VMA
 *     struct file *vm_file: file associated with VMA
 *     struct vm_operations_struct *vm_ops: functions that kernel
 *                   will invoke to operate in VMA
 *     void *vm_private_data: used by driver to store its own information
 *
 * VMA operations:
 *     void (*open)(struct vm_area_struct *vma): invoked when a new reference
 *                   to the VMA is made, except when the VMA is first created,
 *                   when mmap is called
 *     struct page *(*nopage)(struct vm_area_struct *area,
 *                            unsigned long address, int write_access):
 *                   invoked by page fault handler when process tries to access
 *                   valid page in VMA, but not currently in memory
 */

/* nopage is called the first time a memory area is accessed which is not in memory,
 * it does the actual mapping between kernel and user space memory
 */
static vm_fault_t mmap_fault(struct vm_fault *vmf)
{
        struct page *page;
        char *mem;
        unsigned long address = (unsigned long)vmf->address;

        /* is the address valid? */
        if (address > vmf->vma->vm_end) {
                pr_info("invalid address");
                return VM_FAULT_SIGBUS;
        }

        /* the data is in vma->vm_private_data */
        mem = (char *)vmf->vma->vm_private_data;
        if (!mem) {
                pr_info("no data");
                return VM_FAULT_SIGBUS;
        }

        // TODO: Write data to the page
        pr_info("%s: returning \"%s\"\n", __func__, mem);
        msleep(100);

        /* get the page */
        page = virt_to_page(mem);

        /* increment the reference count of this page */
        get_page(page);
        /* type is the page fault type */
        vmf->page = page;
        return 0;
}

static struct vm_operations_struct mmap_vm_ops = {
        .fault = mmap_fault,
};

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
        vma->vm_ops = &mmap_vm_ops;
        vm_flags_set(vma, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
        /* assign the file private data to the vm private data */
        vma->vm_private_data = filp->private_data;
        return 0;
}

static int my_close(struct inode *inode, struct file *filp)
{
        char *mem = (char *)filp->private_data;
        pr_info("closing file, data = \"%s\"\n", mem);
        free_page((unsigned long)mem);
        filp->private_data = NULL;
        return 0;
}

static int my_open(struct inode *inode, struct file *filp)
{
        char *mem;

        pr_info("opening file\n");

        /* obtain new memory */
        mem = (char *)get_zeroed_page(GFP_KERNEL);
        /* assign this info struct to the file */
        filp->private_data = mem;

        return 0;
}

static const struct proc_ops my_fops = {
        .proc_open = my_open,
        .proc_release = my_close,
        .proc_mmap = my_mmap,
};

static int __init mmap_test_module_init(void)
{
        proc_create(DEV_NAME, 0, NULL, &my_fops);
        return 0;
}

static void __exit mmap_test_module_exit(void)
{
        remove_proc_entry(DEV_NAME, NULL);
}

module_init(mmap_test_module_init);
module_exit(mmap_test_module_exit);
MODULE_AUTHOR("Jose Medina");
MODULE_LICENSE("GPL");

