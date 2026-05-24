#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>



#define DEVICE_NAME "mydevice"
#define BUFFER_SIZE 1024
#define WR_VALUE _IOW('a', 'a', int32_t *)
#define RD_VALUE _IOR('a', 'b', int32_t *)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jacob");
MODULE_DESCRIPTION("Auto Character Device Driver");

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static char buffer[BUFFER_SIZE];
int32_t kernel_value = 0;
static DEFINE_MUTEX(my_mutex);
static wait_queue_head_t wait_queue;
static int data_available = 0;
static struct proc_dir_entry *proc_entry;






// open
static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device opened\n");
    return 0;
}


// close
static int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device closed\n");
    return 0;
}


// write
static ssize_t my_write(struct file *file,
                        const char __user *user_buffer,
                        size_t len,
                        loff_t *offset)
{
    mutex_lock(&my_mutex);

    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;

    if (copy_from_user(buffer, user_buffer, len))
    {
        mutex_unlock(&my_mutex);
        return -EFAULT;
    }

    printk(KERN_INFO "Written: %s\n", buffer);

    data_available = 1;

    wake_up_interruptible(&wait_queue);

    mutex_unlock(&my_mutex);

    return len;
}


// read
static ssize_t my_read(struct file *file,
                       char __user *user_buffer,
                       size_t len,
                       loff_t *offset)
{
    int bytes;

    printk(KERN_INFO "Reader going to sleep\n");

    wait_event_interruptible(wait_queue,
                             data_available != 0);

    printk(KERN_INFO "Reader woke up\n");

    mutex_lock(&my_mutex);

    bytes = strlen(buffer);

    if (*offset >= bytes)
    {
        mutex_unlock(&my_mutex);
        return 0;
    }

    if (copy_to_user(user_buffer, buffer, bytes))
    {
        mutex_unlock(&my_mutex);
        return -EFAULT;
    }

    *offset += bytes;

    data_available = 0;

    printk(KERN_INFO "Read done\n");

    mutex_unlock(&my_mutex);

    return bytes;
}

static long my_ioctl(struct file *file,
                     unsigned int cmd,
                     unsigned long arg)
{
    switch(cmd)
    {
        case WR_VALUE:

            copy_from_user(&kernel_value,
                           (int32_t*) arg,
                           sizeof(kernel_value));

            printk(KERN_INFO "IOCTL Write Value = %d\n",
                   kernel_value);

            break;

        case RD_VALUE:

            copy_to_user((int32_t*) arg,
                         &kernel_value,
                         sizeof(kernel_value));

            printk(KERN_INFO "IOCTL Read Value = %d\n",
                   kernel_value);

            break;
    }

    return 0;
}

static __poll_t my_poll(struct file *file,
                        poll_table *wait)
{
    __poll_t mask = 0;

    printk(KERN_INFO "Poll function called\n");


    poll_wait(file, &wait_queue, wait);

    if (data_available)
    {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}
static int proc_show(struct seq_file *m, void *v)
{
    seq_printf(m,
               "Character Driver Info\n");

    seq_printf(m,
               "Last Buffer: %s\n",
               buffer);

    seq_printf(m,
               "Data Available: %d\n",
               data_available);

    seq_printf(m,
               "Kernel Value: %d\n",
               kernel_value);

    return 0;
}
static int proc_open(struct inode *inode,
                     struct file *file)
{
    return single_open(file,
                       proc_show,
                       NULL);
}
static const struct proc_ops proc_fops =
{
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};


// file operations
static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .write = my_write,
    .read = my_read,
    .unlocked_ioctl = my_ioctl,
    .poll = my_poll
    
};


// init
static int __init my_init(void)
{
    mutex_init(&my_mutex);

    init_waitqueue_head(&wait_queue);

    proc_entry = proc_create("mydriver",
                         0666,
                         NULL,
                         &proc_fops);

    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);

    cdev_init(&my_cdev, &fops);

    cdev_add(&my_cdev, dev_num, 1);

    my_class = class_create(THIS_MODULE, "my_class");

    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);

    printk(KERN_INFO "Character driver loaded\n");

    return 0;
}


// exit
static void __exit my_exit(void)
{
    mutex_destroy(&my_mutex);

    proc_remove(proc_entry);
    
    device_destroy(my_class, dev_num);

    class_destroy(my_class);

    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "Character driver unloaded\n");
}


module_init(my_init);
module_exit(my_exit);

