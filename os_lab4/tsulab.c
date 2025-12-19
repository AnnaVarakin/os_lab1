#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/math64.h>

#define procfs_name "tsulab"

static struct proc_dir_entry* our_proc_file = NULL;

static u64 calculate_gagarin_orbits(void)
{
    struct timespec64 now;
    u64 seconds_since_landing;
    u64 orbits;
    ktime_get_real_ts64(&now);

    seconds_since_landing = now.tv_sec - (-275328300LL);

    if (now.tv_sec < -275328300LL) {
        return 0;
    }
    orbits = div64_u64(seconds_since_landing, 6480);
    return orbits;
}

static ssize_t profile_read(struct file* file_pointer, char __user* buffer,
    size_t buffer_length, loff_t* offset)
{
    char s[256];  
    int len;
    ssize_t bytes_to_copy;
    u64 orbits;
    orbits = calculate_gagarin_orbits();

    len = scnprintf(s, sizeof(s),
        "If Yuri Gagarin had not landed on April 12, 1961,\n"
        "he would have made approximately %llu orbits around the Earth by today.\n",
        orbits);

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