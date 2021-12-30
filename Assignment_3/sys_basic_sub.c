#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>

asmlinkage int sys_basic_sub(int num1, int num2, int *result)
{
	int val = num1 - num2;
        
	if (put_user(val, result) < 0)
                printk("The subtraction operation failed!\n");
        else
                printk("The subtraction operation success!\n");

        return 0;
}

SYSCALL_DEFINE3(basic_sub, int, num1, int, num2, int *, result)
{
        return sys_basic_sub(num1, num2, result);
}

