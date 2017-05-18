/*
 * For module learning
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/uaccess.h> 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yang Yu");

#define DEVICE_NAME "chardev_test"
#define BUF_LEN 80


static dev_t dev;
struct cdev *my_cdev;
static struct class* sample_class;
static int Device_Open = 0;
static char msg[BUF_LEN];
static char *msg_Ptr;



static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);


static struct file_operations fops = {
	.owner	= THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};


static int __init init_function(void)
{
	int result;
	
	/*
	 * Require a device number
	 * /proc/devices
	 */
	result = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);

	if(result < 0) 
	{
		printk(KERN_ALERT "Failed to alloc chrdev region.\n");
		goto fail_alloc_chrdev_region;
	}
	
	printk(KERN_INFO "Module assigned major:%d, minor:%d.\n", MAJOR(dev), MINOR(dev));

	/*
	 * Register char device
	 * /sys/class
	 */
	my_cdev = cdev_alloc();
	if(my_cdev < 0)
	{
		printk(KERN_ALERT "Failed to alloc cdev.\n");
		goto fail_alloc_cdev;
	}
	
	cdev_init(my_cdev, &fops);

	result = cdev_add(my_cdev, dev, 1);
	if(result < 0) 
	{
		printk(KERN_ALERT "Failed to add cdev.\n");
		goto fail_add_cdev;
	}
	
	/*
	 * Create a class
	 * /sys/class
	 */
	sample_class = class_create(THIS_MODULE, "chardev_class");
    if(!sample_class) 
    {
		result = -EEXIST;
		printk(KERN_ERR "failed to create class\n");
		goto fail_create_class;
    }
	

    if(!device_create(sample_class, NULL, dev, NULL, "chardev_device")) 
    {
		result = -EINVAL;
		printk(KERN_ERR "failed to create device\n");
		goto fail_create_device;
    }

	return 0;
	
fail_create_device:
    class_destroy(sample_class);
fail_create_class:
    cdev_del(my_cdev);
fail_add_cdev:
fail_alloc_cdev:
    unregister_chrdev_region(dev, 1);
fail_alloc_chrdev_region:
	return result;
}

static void __exit cleanup_function(void)
{
	device_destroy(sample_class, dev);
    class_destroy(sample_class);
	cdev_del(my_cdev);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "Unregister device %s.\n", DEVICE_NAME);
}


static int device_open(struct inode *inode, struct file *filp)
{
	static int counter = 0;

	if(Device_Open)
		return -EBUSY;

	Device_Open++;
	sprintf(msg, "I already told you %d times.\n", ++counter);
	msg_Ptr = msg;
	/*
	* TODO: comment out the line below to have some fun!
	*/
	try_module_get(THIS_MODULE);

	return 0;
}

/*
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *filp)
{
	Device_Open--;

	/*
	* Decrement the usage count, or else once you opened the file, you'll never
	* get rid of the module.
	*
	* TODO: comment out the line below to have some fun!
	*/
	module_put(THIS_MODULE);

	return 0;
}

/*
 * Called when a process, which already opened the dev file, attempts to read
 * from it.
 */
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
                           char *buffer,      /* buffer to fill with data */
                           size_t length,     /* length of the buffer     */
                           loff_t *offset)
{
  /*
   * Number of bytes actually written to the buffer
   */
  int bytes_read = 0;

  /*
   * If we're at the end of the message, return 0 signifying end of file.
   */
  if (*msg_Ptr == 0)
    return 0;

  /*
   * Actually put the data into the buffer
   */
  while (length && *msg_Ptr) {
    /*
     * The buffer is in the user data segment, not the kernel segment so "*"
     * assignment won't work. We have to use put_user which copies data from the
     * kernel data segment to the user data segment.
     */
    put_user(*(msg_Ptr++), buffer++);
    length--;
    bytes_read++;
  }

  /*
   * Most read functions return the number of bytes put into the buffer
   */
  return bytes_read;
}

/*
 * Called when a process writes to dev file: echo "hi" > /dev/hello
 */
static ssize_t device_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
  printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
  return -EINVAL;
}

module_init(init_function);
module_exit(cleanup_function);


