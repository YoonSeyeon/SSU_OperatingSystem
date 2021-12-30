#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>

asmlinkage int sys_basic_mod(int num1, int num2, int *result)
{
	int val = num1 % num2;

	if (put_user(val, result) < 0)
		printk("The moduler operation failed!\n");
	else
		printk("The moduler operation success!\n");

        return 0;
}

SYSCALL_DEFINE3(basic_mod, int, num1, int, num2, int *, result)
{
        return sys_basic_mod(num1, num2, result);
}
