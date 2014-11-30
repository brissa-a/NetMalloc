#include <linux/init.h>
#include <linux/module.h>

#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>

#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>

static char *ip = "127.0.0.1";
module_param(ip, charp, 0);

static int port = 4242;
module_param(port, int, 0);

struct socket *sock = 0;
static unsigned long page = 0;

unsigned long *syscall_table = (unsigned long *)0xffffffff8180ae20;
asmlinkage long (*original)(unsigned int size, unsigned long *ptr);
unsigned long original_cr0;

unsigned long begin_pointer = 0;

#define SYSCALL_TO_REPLACE __NR_epoll_ctl_old
#define PAGESIZE 4096

/* NETWORK */
#define READ_ID 1
#define WRITE_ID 2
#define ALLOC_ID 3

static void convert_from_addr(const char *addr, unsigned char nbs[4])
{
	int i;
	int value;
	int pos;

	for (i = 0, pos = 0; addr[i] && pos < 4; ++i)
	{
		for (value = 0; addr[i] && addr[i] != '.'; ++i) {
			value *= 10;
			value += addr[i] - '0';
		}
		nbs[pos++] = value;
		if (!addr[i])
			return;
	}
}

static int get_data(void *buf, int len, int send)
{
	struct msghdr msg = {0};
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	if (send == 1)
		size = sock_sendmsg(sock, &msg, len);
	else
		size = sock_recvmsg(sock, &msg, len, MSG_WAITALL);
	set_fs(oldfs);

	if (size < 0)
		printk("thug_write_to_socket: sock_sendmsg error: %d\n", size);

	return size;
}

static int send_msg(void *buf, int len)
{
	return get_data(buf, len, 1);
}

static int recv_msg(void *buf, int len)
{
	return get_data(buf, len, 0);
}

static int my_connect(struct socket **s, const char *s_addr)
{
	struct sockaddr_in address = {0};
	int len = sizeof(struct sockaddr_in), ret;
	unsigned char i_addr[4] = {0};
	if ((ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, s)) < 0)
	{
		printk("echo_server: sock_create error");
		return ret;
	}
	convert_from_addr(s_addr, i_addr);
	address.sin_addr.s_addr = *(unsigned int*)i_addr;
	address.sin_port = htons(port);
	address.sin_family = AF_INET;
	if ((ret = (*s)->ops->connect(sock, (struct sockaddr*)&address, len, 0)) < 0)
	{
		sock_release(*s);
		*s = NULL;
	}
	return ret;
}

/* NETMALLOC */

static int my_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	pr_info("There is page fault !\n");
	page = get_zeroed_page(GFP_KERNEL);
	//Fill page with data get from network
	if (sock != NULL)
	{
		char id = READ_ID;

		send_msg(&id, sizeof(id));
		send_msg(&begin_pointer, sizeof(begin_pointer)); // begin pointer
		send_msg((void*)page, sizeof(unsigned long)); // from pointer

		send_msg((void*)PAGESIZE, sizeof(unsigned int));
		recv_msg((void*)page, PAGESIZE);
	}
	vmf->page = virt_to_page(page);
	return 0;
}

struct vm_operations_struct my_vm_ops = {
  .fault = my_fault
};

asmlinkage int new_netmalloc(unsigned int size, unsigned long *ptr)
{
	struct vm_area_struct *vm_area;

	printk("=============> overwrite syscall !\n");
	begin_pointer = vm_mmap(0, 0, size, PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_ANONYMOUS | MAP_PRIVATE, 0);
	vm_area = find_vma(current->mm, begin_pointer);
	vm_area->vm_ops = &my_vm_ops;
	copy_to_user(ptr, &begin_pointer, sizeof(begin_pointer));

	if (sock != NULL)
	{
		char id = ALLOC_ID;

		send_msg(&id, sizeof(id));
		send_msg(&begin_pointer, sizeof(begin_pointer));
		send_msg((char*)&size, sizeof(size));
	}
	return 0;
}

/* Module */

static unsigned long **find_syscall_table(void)
{
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *) sys_close) 
			return sct;

		offset += sizeof(void *);
	}
  
	return NULL;
}

static int __init mymodule_init(void)
{
	int ret;

	syscall_table = (void **) find_syscall_table();
	if (syscall_table == NULL)
	{
		printk(KERN_ERR"net_malloc: Syscall table is not found\n");
		return -1;
	}
	original_cr0 = read_cr0();

	write_cr0(original_cr0 & ~0x00010000);
	original = (void *)syscall_table[SYSCALL_TO_REPLACE];
	syscall_table[SYSCALL_TO_REPLACE] = (unsigned long *)new_netmalloc;
	write_cr0(original_cr0);
	printk("net_malloc: Patched! netmalloc number : %d\n", SYSCALL_TO_REPLACE);
	if ((ret = my_connect(&sock, "127.0.0.1")) < 0)
	{
		printk("netmalloc: connect error\n");
		return -ENOMEM;
	}
	return 0;
}

static void __exit mymodule_exit(void)
{
	if (sock != NULL)
	{
		sock_release(sock);
		sock = NULL;
	}
	printk("module exit\n");

	// reset syscall
	if (syscall_table != NULL)
	{
		original_cr0 = read_cr0();

		write_cr0(original_cr0 & ~0x00010000);
		syscall_table[SYSCALL_TO_REPLACE] = (unsigned long *)original;
		write_cr0(original_cr0);
	}
}

module_init(mymodule_init);
module_exit(mymodule_exit);
