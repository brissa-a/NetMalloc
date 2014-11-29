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

static unsigned long page = 0;

static int __init mymodule_init(void)
{
	return 0;
}

static int my_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	pr_info("There is page fault !");
	if (page == 0)
	{
		page = get_zeroed_page(GFP_KERNEL);
	}
	vmf->page = virt_to_page(page);
	return 0;
}

struct vm_operations_struct my_vm_ops = {
	.fault = my_fault
};

SYSCALL_DEFINE2(net_malloc, unsigned int, size, unsigned long *, ptr)
{

	struct vm_area_struct *vm_area;
	addr = vm_mmap(0, 0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0);
	vm_area = find_vma(current->mm, addr);
	vm_area->vm_ops = &my_vm_ops;
	copy_to_user(ptr, &addr, sizeof(addr));
	printk("syscall called: %p\n", addr);
	return 0;
}

static void __exit mymodule_exit(void)
{
	pr_info("mymodule: exit\n");
	return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
