#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include "array_stats.h"

SYSCALL_DEFINE3(array_stats, struct array_stats __user *, stats, long __user *, data, long, size)
{
    long i;
    long val;
    long sum = 0;
    struct array_stats stats_kernel;

    // Validate size argument
    if (size <= 0) {
        return -EINVAL;
    }

    // Copy data array from user space to kernel space
    if (copy_from_user(&val, data, sizeof(long))) {
        return -EFAULT;
    }

    // Initialize min and max to the first value in data
    stats_kernel.min = val;
    stats_kernel.max = val;

    // Traverse data array and compute min, max, and sum
    for (i = 0; i < size; i++) {
        // Copy value from user space to kernel space
        if (copy_from_user(&val, &data[i], sizeof(long))) {
            return -EFAULT;
        }

        // Update min and max values
        if (val < stats_kernel.min) {
            stats_kernel.min = val;
        }
        if (val > stats_kernel.max) {
            stats_kernel.max = val;
        }

        // Update sum value
        sum += val;
    }

    // Store sum value in stats structure
    stats_kernel.sum = sum;

    // Copy stats structure from kernel space to user space
    if (copy_to_user(stats, &stats_kernel, sizeof(struct array_stats))) {
        return -EFAULT;
    }

    // Return 0 to indicate success
    return 0;
}