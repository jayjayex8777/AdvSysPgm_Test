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
 * concurrent access into the same device
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

			if (mm) {
				pr_info("Current memory mappings:\n");
				struct vma_iterator vmi;
				struct vm_area_struct *vma;
				unsigned long total_vm_size = 0;
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

				// Print ranges and sizes of code, data, heap, and stack
				pr_info("Process code addr: 0x%lx ~ 0x%lx (Size: %luK)\n", 
						mm->start_code, mm->end_code, 
						(mm->end_code - mm->start_code) / 1024);

				pr_info("Process data addr: 0x%lx ~ 0x%lx (Size: %luK)\n", 
						mm->start_data, mm->end_data, 
						(mm->end_data - mm->start_data) / 1024);

				pr_info("Process heap addr: 0x%lx ~ 0x%lx (Size: %luK)\n", 
						mm->start_brk, mm->brk, 
						(mm->brk - mm->start_brk) / 1024 );

				pr_info("Process stack addr: 0x%lx (Top of stack)\n", mm->start_stack);

				
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

