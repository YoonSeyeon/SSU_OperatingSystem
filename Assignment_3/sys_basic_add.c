#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>

asmlinkage int sys_basic_add(int num1, int num2, int *result)
{
	int val = num1 + num2;

        if (put_user(val, result) < 0)
                printk("The addition operation failed!\n");
        else
                printk("The addition operation success!\n");

        return 0;
}

SYSCALL_DEFINE3(basic_add, int, num1, int, num2, int *, result)
{
        return sys_basic_add(num1, num2, result);
}

