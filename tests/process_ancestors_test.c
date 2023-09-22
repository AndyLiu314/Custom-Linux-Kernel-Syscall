#include "process_ancestors.h"
#include <linux/sched.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/list.h>

#define _PROCESS_ANCESTORS_ 438

/**
 * Prototypes
 */
static void do_syscall_working(long size, long *num_filled);
static void test_internal(_Bool success, int lineNum, char *argStr);
static void test_print_summary(void);

int main(int argc, char *argv[]) {
    fork();
    fork();
    //long* num;

    do_syscall_working(4,4);
    test_print_summary();
    return 0;
}

static int numTests = 0;
static int numTestPassed = 0;

static int current_syscall_test_num = 0;
static int last_syscall_test_num_failed = -1;
static int num_syscall_tests_failed = 0;

#define TEST(arg) test_internal((arg), __LINE__, #arg)

// Actual function used to check success/failure:
static void test_internal(_Bool success, int lineNum, char *argStr) {
  numTests++;
  if (!success) {
    if (current_syscall_test_num != last_syscall_test_num_failed) {
      last_syscall_test_num_failed = current_syscall_test_num;
      num_syscall_tests_failed++;
    }
    printf("-------> ERROR %4d: test on line %d failed: %s\n", numTestPassed,
           lineNum, argStr);
  } else {
    numTestPassed++;
  }
}

static void test_print_summary(void) {
  printf("\nExecution finished.\n");
  printf("%4d/%d tests passed.\n", numTestPassed, numTests);
  printf("%4d/%d tests FAILED.\n", numTests - numTestPassed, numTests);
  printf("%4d/%d unique sys-call testing configurations FAILED.\n",
         num_syscall_tests_failed, current_syscall_test_num);
}

static long find_pid(int position){
    struct task_struct *task = current;
    for (int i = 0; i < position; i++){
        task = task->parent;
    }

    return task->pid;
}

static char find_name(int position){
    struct task_struct *task = current;
    char name[ANCESTOR_NAME_LEN];
    for (int i = 0; i < position; i++){
        task = task->parent;
    }
    get_task_comm(name, task);
    return name;
}

static long find_state(int position){
    struct task_struct *task = current;
    for (int i = 0; i < position; i++){
        task = task->parent;
    }

    return task->state;
}

static long find_uid(int position){
    struct task_struct *task = current;
    for (int i = 0; i < position; i++){
        task = task->parent;
    }

    return task->cred->uid.val;
}

static long find_nvcsw(int position){
    struct task_struct *task = current;
    for (int i = 0; i < position; i++){
        task = task->parent;
    }

    return task->nvcsw;
}

static long find_nivcsw(int position){
    struct task_struct *task = current;
    for (int i = 0; i < position; i++){
        task = task->parent;
    }

    return task->nivcsw;
}

static long find_num_children(int position){
    struct task_struct *task = current;
    struct task_struct *child;
    struct task_struct *sibling;
    long num_children = 0;

    for (int i = 0; i < position; i++){
        task = task->parent;
    }

    list_for_each_entry(child, &task->children, sibling) {
        ++num_children;
    }

    return num_children;
}

static long find_num_siblings(int position){
    struct task_struct *task = current;
    struct task_struct *child;
    struct task_struct *sibling;
    long num_siblings = 0;

    for (int i = 0; i < position; i++){
        task = task->parent;
    }

    list_for_each_entry(sibling, &task->sibling, sibling) {
        ++num_siblings;
    }

    return num_siblings;
}

static int do_syscall(struct process_info *info_array, long size, long *num_filled) {
  current_syscall_test_num++;
  printf("\nTest %d: ..Diving to kernel level\n", current_syscall_test_num);
  int result = syscall(_PROCESS_ANCESTORS_, info_array, size, num_filled);
  int my_errno = errno;
  printf("..Rising to user level w/ result = %d", result);
  if (result < 0) {
    printf(", errno = %d", my_errno);
  } else {
    my_errno = 0;
  }

  printf("\n");
  return my_errno;
}

static void do_syscall_working(long size, long *num_filled) {
    struct process_info* info_array;
    int result = do_syscall(&info_array, size, num_filled);

    /*printf("Fields: ID = %ld, name = %s\n, state = %ld\n, uid = %ld\n, nvcsw = %ld\n, nivcsw = %ld\n, numchildren = %ld\n, num_siblings = %ld\n", 
    info_array->pid, task->name, task->state, task->uid, task->nvcsw, task->nivcsw, task->num_children, task->num_siblings);
    */

   TEST(result == 0);
    for (int i = 0; i < size; i++){
        TEST(info_array[i].pid == find_pid(i));
        TEST(info_array[i].name == find_name(i));
        TEST(info_array[i].state == find_state(i));
        TEST(info_array[i].uid == find_uid(i));
        TEST(info_array[i].nvcsw == find_nvcsw(i));
        TEST(info_array[i].nivcsw == find_nivcsw(i));
        TEST(info_array[i].num_children == find_num_children(i));
        TEST(info_array[i].num_siblings == find_num_siblings(i));
    }
}
