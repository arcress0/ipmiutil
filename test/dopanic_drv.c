/* dopanic.c 
 *
 * Cause a panic in a loadable driver.
 */
#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */

static int device_open(void *inode, 
		       void *file)
{
  printk (KERN_DEBUG "device_open(%p,%p)\n", inode, file);
  return 0;
}

static int device_release(void *inode, 
			  void *file)
{
  printk ("device_release(%p,%p)\n", inode, file);
  return 0;
}


static int device_read(void *file,
    char *buffer,    /* The buffer to fill with data */
    int length,   /* The length of the buffer */
    int *offset)  /* Our offset in the file */
{
    return 0;
}

static int device_write(void *file,
    const char *buffer,    /* The buffer */
    int length,   /* The length of the buffer */
    int *offset)  /* Our offset in the file */
{
  return -1;
}

/* Initialize the module */
int init_module()
{
  panic("dopanic: init_module calls panic");
  return 0;
}


/* Cleanup - unregister the appropriate file from /proc */
void cleanup_module()
{
}  

