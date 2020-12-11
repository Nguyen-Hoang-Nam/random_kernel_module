#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_AUTHOR "Nguyen Hoang Nam & Luong Duc Trung"
#define DRIVER_DESCRIPTION "Create chracter device for user space to open and read random number"
#define BUFFER_LENGTH (128 * sizeof(uint64_t))
#define RANDOM_NAME "xoshrio256"

static int random_init(void) __init;
static void random_fill_buffer(char *, size_t);
static uint64_t random_xoshiro256(void);
static void random_seed(void);
static int random_open(struct inode *, struct file *);
static int random_release(struct inode *, struct file *);
static ssize_t random_read(struct file *, char *, size_t, loff_t *);
static ssize_t random_write(struct file *, const char *, size_t, loff_t *);
static void random_exit(void) __exit;

static struct file_operations random_fops = {
	.owner = THIS_MODULE,
	.read = random_read,
	.write = random_write,
	.open = random_open,
	.release = random_release
};

static struct miscdevice random_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = RANDOM_NAME,
	.fops = &random_fops,
	.mode = S_IRUGO,
};

uint64_t random_state[4];

static int device_open;
static char random_buffer[BUFFER_LENGTH];
static char *random_buffer_pointer; //  pointer to current random_buffer position

/*
 * Implement Xoshiro256+ to generate random number
 * Reference https://en.wikipedia.org/wiki/Xorshift
*/
static uint64_t random_xoshiro256(void)
{
	uint64_t* s = random_state;
	uint64_t const result = s[0] + s[3];
	uint64_t const t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;
	s[3] = rol64(s[3], 45);

	return result;
}

static void random_fill_buffer(char *random_buffer_pointer, size_t length)
{
	uint64_t random_number;

	while(length) {
		random_number = random_xoshiro256();
		*((uint64_t *) random_buffer_pointer) = random_number;

		random_buffer_pointer += sizeof(uint64_t);
		length -= sizeof(uint64_t);
	}
}

static void random_seed(void)
{
	uint64_t i;

	for (i = 0; i < 4; i++) {
		random_state[i] = i + 1;
	}
}

static int random_open(struct inode *inode, struct file *file)
{
	if (device_open) {
		return -EBUSY;
	}

	device_open++;

	try_module_get(THIS_MODULE);

	return 0;
}

static int random_release(struct inode *inode, struct file *file)
{
	device_open--;

	module_put(THIS_MODULE);

	return 0;
}

static ssize_t random_read(struct file *file, char __user *buffer, size_t length, loff_t *off)
{
	ssize_t bytes_read = length;
	ssize_t bytes_to_copy = 0;

	while (length) {
		if (random_buffer_pointer == random_buffer + sizeof(random_buffer)) {
			random_fill_buffer(random_buffer, sizeof(random_buffer));
			random_buffer_pointer = random_buffer;
		}

		bytes_to_copy = min(length, sizeof(random_buffer) - (random_buffer_pointer - random_buffer));
		if(copy_to_user(buffer, random_buffer_pointer, bytes_to_copy)) {
			return -EFAULT;
		}

		random_buffer_pointer += bytes_to_copy;
		buffer += bytes_to_copy;
		length -= bytes_to_copy;
	}

	return bytes_read;
}

static ssize_t random_write(struct file *file, const char *buffer, size_t length, loff_t *off)
{
	pr_alert("%s: write operation on /dev/%s not supported\n", RANDOM_NAME, RANDOM_NAME);
	return -EINVAL;
}

static int __init random_init(void)
{
	int ret;

	ret = misc_register(&random_miscdev);

	if (ret) {
		pr_err("%s: misc_register failed %d", RANDOM_NAME, ret);
	}

	pr_info("%s: registered\n", RANDOM_NAME);
	pr_info("%s: created device file /dev/%s\n", RANDOM_NAME, RANDOM_NAME);

	device_open = 0;

	random_seed();

	random_fill_buffer(random_buffer, sizeof(random_buffer));
	random_buffer_pointer = random_buffer;

	return 0;
}

static void __exit random_exit(void)
{
	misc_deregister(&random_miscdev);

	pr_info("%s: unregistered\n", RANDOM_NAME);
	pr_info("%s: deleted /dev/%s\n", RANDOM_NAME, RANDOM_NAME);

}

module_init(random_init);
module_exit(random_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("testdevice");
