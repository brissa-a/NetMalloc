#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
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

SYSCALL_DEFINE1(net_malloc, unsigned int, size)
{
	pr_info("mymodule: net_malloc called");
/*
	int toto;	
	pr_info("mymodule: runned with %s:%d\n", ip, port);
	toto = get_unmapped_area(0, 0, 50, 0, 0);
	pr_info("mymodule: RETURN: %d\n", toto);
	vm_mmap(0, toto, 50, 0, 0, 0);
*/
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
