#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>

int major = 0;
int minor = 0;
int memsize = 255;

module_param(major, int, S_IRUGO);
module_param(minor, int, S_IRUGO);
module_param(memsize, int, S_IRUGO);

MODULE_AUTHOR("Claudio Demaria");
MODULE_LICENSE("Dual BSD/GPL");

struct my_dev
{
    char *data; // Pointer to data area
    int memsize;
    struct semaphore sem; // mutual exclusion semaphore
    struct cdev cdev;     // structure for char device
};

struct my_dev my_device;

int my_open(struct inode *inode, struct file *filp)
{
    struct my_dev *dev; // a pointer to a my_dev structure
    dev = container_of(inode->i_cdev, struct my_dev, cdev);

    // update private data pointer to be re-used in other system call
    filp->private_data = dev;

    return 0;
}

int my_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t my_write(struct file *filp, const char __user *buf, size_t count,
                 loff_t *f_pos)
{
    // declare and fill a my_dev structure that points to private_data
    struct my_dev *dev = filp->private_data;

    ssize_t retval = 0; // return value

    if (down_interruptible(&dev->sem))
    {
        return -ERESTARTSYS;
    }

    // if count is bigger that memsize saturate it
    if (count > dev->memsize)
    {
        count = dev->memsize;
    }

    // copy the memory from user memory in buf to kernel memory
    if (copy_from_user(dev->data, buf, count))
    {
        retval = -EFAULT;
        goto out;
    }

    // print in the kernel log the string written
    printk(KERN_INFO "my_write: %s", dev->data);

    // update retval
    retval = count;

    // free the semaphore
out:
    up(&dev->sem);
    return retval;
}

struct file_operations my_ops = {
    .owner = THIS_MODULE,
    .write = my_write,
    .open = my_open,
    .release = my_release,
};

void my_cleanup_module(void)
{
    // fill a dev_t variable with major and minor number
    dev_t devno = MKDEV(major, minor);

    // tell the kernel to remove the cden
    cdev_del(&my_device.cdev);

    // free the memory for data exchange
    kfree(my_device.data);

    // Notify the kernel that the device with the corresponding major and minor number is no more present
    unregister_chrdev_region(devno, 1);
}

int my_init_module(void)
{
    int result, err;
    dev_t dev = 0;

    // if the major number has been passed as an input parameter, register the device
    if (major)
    {
        dev = MKDEV(major, minor);
        result = register_chrdev_region(dev, 1, "my_device");
    }

    // otherwise allocate the device in order to receive a free major number
    else
    {
        result = alloc_chrdev_region(&dev, minor, 1, "my_device");
        major = MAJOR(dev);
    }

    if (result < 0)
    {
        printk(KERN_WARNING "my_device: can't get major %d\n", major);
        return result;
    }

    // copy the parameter memsize in the structure my_dev
    my_device.memsize = memsize;

    // allocate memory for data exchange
    my_device.data = kmalloc(memsize * sizeof(char), GFP_KERNEL);
    memset(my_device.data, 0, memsize * sizeof(char));

    // Initialize semaphore
    sema_init(&my_device.sem, 1);

    // Inizialize cdev
    cdev_init(&my_device.cdev, &my_ops);
    my_device.cdev.owner = THIS_MODULE;

    // Set the corresponding file operations
    my_device.cdev.ops = &my_ops;

    // Notify to the kernel the presence of cdev
    err = cdev_add(&my_device.cdev, dev, 1);

    // Print in the log the corresponding major and minor number
    if (err)
        printk(KERN_NOTICE "Error %d adding my_device", err);
    else
        printk(KERN_NOTICE "my_device Added major: %d minor: %d", major, minor);

    return 0;
}
module_init(my_init_module);
module_exit(my_cleanup_module);
