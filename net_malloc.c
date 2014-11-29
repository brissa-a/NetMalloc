#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/syscalls.h>

/* Module */
static char *ip = "127.0.0.1";
module_param(ip, charp, 0);

static int port = 4242;
module_param(port, int, 0);

/*
static int __init mymodule_init(void)
{
	return 0;
}
*/

SYSCALL_DEFINE2(net_malloc, unsigned int, size, unsigned long *, ptr)
{
	*ptr = vm_mmap(0, 0, 50, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS, 0);
	return 0;
}

/*
static void __exit mymodule_exit(void)
{
	pr_info("mymodule: exit\n");
	return;
}
*/

/*
module_init(mymodule_init);
module_exit(mymodule_exit);
*/

MODULE_LICENSE("GPL");
