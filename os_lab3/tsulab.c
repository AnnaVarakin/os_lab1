#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define procfs_name "tsu"

static struct proc_dir_entry* our_proc_file = NULL;

static ssize_t profile_read(struct file* file_pointer, char __user* buffer,
    size_t buffer_length, loff_t* offset)
{
    char s[6] = "Tomsk\n";
    size_t len = sizeof(s);
    ssize_t bytes_to_copy;

    if (*offset >= len) {
        return 0;
    }
    bytes_to_copy = len - *offset;

    if (buffer_length < bytes_to_copy) {
        bytes_to_copy = buffer_length;
    }

    if (copy_to_user(buffer, s + *offset, bytes_to_copy)) {
        return -EFAULT;
    }

    *offset += bytes_to_copy;
    return bytes_to_copy;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = profile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = profile_read,
};
#endif

static int __init procfs1_init(void)
{
    our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_fops);
    if (!our_proc_file) {
        pr_err("Failed to create /proc/%s\n", procfs_name);
        return -ENOMEM; 
    }
    pr_info("Welcome to the Tomsk State University\n");
    return 0;
}

static void __exit procfs1_exit(void)
{
    if (our_proc_file) {
        proc_remove(our_proc_file);
    }
    pr_info("Tomsk State University forever!\n");
}

module_init(procfs1_init);
module_exit(procfs1_exit);

MODULE_LICENSE("GPL");