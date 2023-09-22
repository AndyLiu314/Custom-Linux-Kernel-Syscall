#include <linux/kernel.h>
#include <linux/syscalls.h>
#include "process_ancestors.h"
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/sched.h>

SYSCALL_DEFINE3(process_ancestors, struct process_info __user *, info_array, long, size, long __user *, num_filled)
{
    struct process_info pinfo;
    struct task_struct *task = current;
    struct task_struct *child;
    struct task_struct *sibling;
    int count = 0;

    if (size <= 0)
        return -EINVAL;
    if (!info_array || !num_filled)
        return -EFAULT;

    while (task && count < size) {
        pinfo.pid = task->pid;
        get_task_comm(pinfo.name, task);
        pinfo.state = task->state;
        pinfo.uid = task->cred->uid.val;
        pinfo.nvcsw = task->nvcsw;
        pinfo.nivcsw = task->nivcsw;

        /* count children */
        pinfo.num_children = 0;
        list_for_each_entry(child, &task->children, sibling) {
            ++pinfo.num_children;
        }

        /* count siblings */
        pinfo.num_siblings = 0;
        list_for_each_entry(sibling, &task->sibling, sibling) {
            ++pinfo.num_siblings;
        }

        /* copy to user space */
        if (copy_to_user(&info_array[count], &pinfo, sizeof(struct process_info))) {
            return -EFAULT;
        }

        task = task->parent;
        count++;
    }

    /* write the number of structs written to the array */
    if (put_user(count, num_filled)) {
        return -EFAULT;
    }

    return 0;
}
