/*
 *  chardev.c - Create an input/output character device
 */

#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/module.h>       /* Specifically, a module */
#include <linux/fs.h>
#include <asm/uaccess.h>        /* for get_user and put_user */
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/mman.h>
#include <linux/cdev.h>
#include <linux/delay.h>

#include "chardev.h"
#define DEVICE_NAME "chardev"

/*
 * Is the device open right now? Used to prevent
 * concurent access into the same device
 */
static int opened = 0;

/*
 * This is called whenever a process attempts to open the device file
 */
static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
        pr_info("device_open(%p)\n", file);
#endif

        /*
         * We don't want to talk to two processes at the same time
         */
        if (opened)
                return -EBUSY;

        opened++;
        /*
         * Initialize the message
         */
        try_module_get(THIS_MODULE);
        return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
        pr_info("device_release(%p,%p)\n", inode, file);
#endif

        /*
         * We're now ready for our next caller
         */
        opened--;

        module_put(THIS_MODULE);
        return 0;
}

/*
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */
long device_ioctl(struct file *file,    /* see include/linux/fs.h */
                  unsigned int ioctl_num,       /* number and param for ioctl */
                  unsigned long ioctl_param)
{
	msleep(100);
	pr_info("\n\n");

	switch (ioctl_num) {
	/* fill the body with cases */
		case IOCTL_PROCINFO:
		{
			struct task_struct *task = current;
			struct mm_struct *mm = task->mm;
			
			pr_info("Process ID: %d\n", task->pid);
			pr_info("Process name: %s\n", task->comm);			

			if (mm) {
				pr_info("Process code addr: 0x%lx ~ 0x%lx (Size: %lu B)\n", 
						mm->start_code, mm->end_code, 
						(mm->end_code - mm->start_code));

				pr_info("Process data addr: 0x%lx ~ 0x%lx (Size: %lu B)\n", 
						mm->start_data, mm->end_data, 
						(mm->end_data - mm->start_data));

				pr_info("Process heap addr: 0x%lx ~ 0x%lx (Size: %lu B)\n", 
						mm->start_brk, mm->brk, 
						(mm->brk - mm->start_brk) );

				pr_info("Process stack addr: 0x%lx \n", mm->start_stack);
				
			} else {
				pr_info("No memory management structure available\n");
			}
			break;
		}
		case IOCTL_CURMAP:
		{
			struct task_struct *task = current;
			struct mm_struct *mm = task->mm;
			struct vma_iterator vmi;
			struct vm_area_struct *vma;
			unsigned long total_vm_size = 0;

			if (mm) {
				pr_info("Current memory mappings:\n");
				vma_iter_init(&vmi, mm, 0);
				for_each_vma(vmi, vma) {

					char name_buf[256] = "[ anon ]";

					if (vma->vm_file) {
						char *path = d_path(&vma->vm_file->f_path, name_buf, sizeof(name_buf));
						pr_info("0x%lx %luK %c%c%c %s\n",
						vma->vm_start, (vma->vm_end - vma->vm_start) / 1024, 
						(vma->vm_flags & VM_READ) ? 'r' : '-',	(vma->vm_flags & VM_WRITE) ? 'w' : '-',
						(vma->vm_flags & VM_EXEC) ? 'x' : '-', IS_ERR(path) ? "Unknown" : path);				
					} 
					else {
						pr_info("0x%lx %luK %c%c%c %s\n",
							vma->vm_start, (vma->vm_end - vma->vm_start) / 1024,
							(vma->vm_flags & VM_READ) ? 'r' : '-',	(vma->vm_flags & VM_WRITE) ? 'w' : '-',
							(vma->vm_flags & VM_EXEC) ? 'x' : '-',	name_buf);
					}
					total_vm_size += (vma->vm_end - vma->vm_start);
				}
				pr_info("Total VMA address space: %luK\n", total_vm_size / 1024);		

				
			}
			else {
				pr_info("No memory management structure available\n");
			}

			break;
		}	
		
		case IOCTL_VMAINFO:
		{
			struct task_struct *task = current;
			struct mm_struct *mm = task->mm;
			struct vma_iterator vmi;
			struct vm_area_struct *vma;
			unsigned long total_vm_size = 0;
			unsigned long code_size = 0, data_size = 0, heap_size = 0, stack_size = 0;
			unsigned long code_start = 0, code_end = 0;
			unsigned long data_start = 0, data_end = 0;
			unsigned long heap_start = 0, heap_end = 0;
			unsigned long stack_start = 0, stack_end = 0;

			if (mm) {
				pr_info("Current memory mappings:\n");
				vma_iter_init(&vmi, mm, 0);
				for_each_vma(vmi, vma) {
					char name_buf[256] = "[ anon ]";

					if (vma->vm_file) {
						char *path = d_path(&vma->vm_file->f_path, name_buf, sizeof(name_buf));
						pr_info("0x%lx %luK %c%c%c %s\n",
						vma->vm_start, (vma->vm_end - vma->vm_start) / 1024, 
						(vma->vm_flags & VM_READ) ? 'r' : '-',	(vma->vm_flags & VM_WRITE) ? 'w' : '-',
						(vma->vm_flags & VM_EXEC) ? 'x' : '-', IS_ERR(path) ? "Unknown" : path);				
					} 
					else {
						pr_info("0x%lx %luK %c%c%c %s\n",
						vma->vm_start, (vma->vm_end - vma->vm_start) / 1024,
						(vma->vm_flags & VM_READ) ? 'r' : '-',	(vma->vm_flags & VM_WRITE) ? 'w' : '-',
						(vma->vm_flags & VM_EXEC) ? 'x' : '-',	name_buf);
					}

					total_vm_size += (vma->vm_end - vma->vm_start);

					/* Identify code segment */
					if (vma->vm_start <= mm->start_code && vma->vm_end >= mm->end_code) {
						code_size += (vma->vm_end - vma->vm_start);
						if (!code_start) code_start = vma->vm_start;
						code_end = vma->vm_end;
					}

					/* Identify data segment */
					 if (vma->vm_start <= mm->end_data && vma->vm_end >= mm->start_data) {
                                                data_size += (vma->vm_end - vma->vm_start);
                                                if (!data_start) data_start = vma->vm_start;
                                                data_end = vma->vm_end;
                                        }

					/* Identify heap segment */
					if (vma->vm_start <= mm->brk && vma->vm_end >= mm->start_brk) {
						heap_size += (vma->vm_end - vma->vm_start);
						if (!heap_start) heap_start = vma->vm_start;
						heap_end = vma->vm_end;
					}

					/* Identify stack segment */
					if (vma->vm_start <= mm->start_stack && vma->vm_end >= mm->start_stack) {
						stack_size += (vma->vm_end - vma->vm_start);
						if (!stack_start) stack_start = vma->vm_start;
						stack_end = vma->vm_end;
					}
				}

				pr_info("Total VMA address space: %luK\n", total_vm_size / 1024);
				
				pr_info("Code  : 0x%lx ~ 0x%lx (%luK)\n", code_start, code_end, code_size / 1024);
				pr_info("Data  : 0x%lx ~ 0x%lx (%luK)\n", data_start, data_end, data_size / 1024);
				pr_info("Heap  : 0x%lx ~ 0x%lx (%luK)\n", heap_start, heap_end, heap_size / 1024);
				pr_info("Stack : 0x%lx ~ 0x%lx (%luK)\n", stack_start, stack_end, stack_size / 1024);
			}
			else {
				pr_info("No memory management structure available\n");
			}

			break;
		}

		
	}
	pr_info("\n\n");

	return 0;
}

/* Module Declarations */

/*
 * This structure will hold the functions to be called
 * when a process does something to the device we
 * created. Since a pointer to this structure is kept in
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions.
 */
struct file_operations chardev_fops = {
        .unlocked_ioctl = device_ioctl,
        .open = device_open,
        .release = device_release,      /* a.k.a. close */
};

static struct class *chardev_class;
static struct cdev cdev;
#define chardev_major 295
#define chardev_minor 0

void chardev_cleanup_module(void)
{
        device_destroy(chardev_class, MKDEV(chardev_major, chardev_minor));
        unregister_chrdev_region(MKDEV(chardev_major, chardev_minor), 1);
        cdev_del(&cdev);
        class_destroy(chardev_class);
}

int chardev_init_module(void)
{
        struct device *chardev_reg_device;
        int ret;

        chardev_class = class_create(DEVICE_NAME);
        if (IS_ERR(chardev_class)) {
                ret = PTR_ERR(chardev_class);
                pr_warn("Failed to register class chardev\n");
                goto error0;
        }

        cdev_init(&cdev, &chardev_fops);
        cdev.owner = THIS_MODULE;
        ret = cdev_add(&cdev, MKDEV(chardev_major, chardev_minor), 1);
        if (ret) {
                pr_warn("Failed to add cdev for /dev/chardev\n");
                goto error1;
        }

        ret = register_chrdev_region(MKDEV(chardev_major, chardev_minor), 1, DEVICE_NAME);
        if (ret < 0) {
                pr_warn("can't get major/minor %d/%d\n", chardev_major, chardev_minor);
                goto error2;
        }

        chardev_reg_device = device_create(chardev_class, NULL, MKDEV(chardev_major, chardev_minor), NULL, DEVICE_NAME);

        if (IS_ERR(chardev_reg_device)) {
                pr_warn("Failed to create chardev device\n");
                goto error3;
        }

        return 0;

 error3:
        unregister_chrdev_region(MKDEV(chardev_major, chardev_minor), 1);
 error2:
        cdev_del(&cdev);
 error1:
        class_destroy(chardev_class);
 error0:

        return -EINVAL;
}

module_init(chardev_init_module);
module_exit(chardev_cleanup_module);

MODULE_LICENSE("GPL");
