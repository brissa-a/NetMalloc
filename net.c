#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/types.h>

#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>

#define __bswap_16(x) ((unsigned short)(__builtin_bswap32(x) >> 16))

MODULE_LICENSE("Dual BSD/GPL");

#define PORT 4242
#define SERVER_ADDR "127.0.0.1"

struct task_struct *kthread = NULL;
struct mutex mutex;
struct socket *sock = NULL;

static char	*init_msg = "Init message";
static int	size_init_msg = 13;
static char	*exit_msg = "Exit message";
static int	size_exit_msg = 13;

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

static void ksocket_start(void)
{
	printk("Thread launched !");
	for (;;)
	{
		mutex_lock(&mutex);
		usleep_range(10000, 10000);
		mutex_unlock(&mutex);
		usleep_range(1000, 1000);
	}
}

static int send_msg(unsigned char *buf, int len)
{
	struct msghdr msg = {0};
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;
	
	iov.iov_base = buf;
	iov.iov_len = len;
	
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	/* msg.msg_control = NULL; */
	/* msg.msg_controllen = 0; */
	/* msg.msg_name = NULL; */
	/* msg.msg_namelen = 0; */
	/* msg.msg_flags= 0; */
	
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_sendmsg(sock,&msg,len);
	set_fs(oldfs);
	
	if (size < 0)
		printk("thug_write_to_socket: sock_sendmsg error: %d\n", size);
	
	return size;
}

static int my_connect(struct socket **s, const char *s_addr)
{
	struct sockaddr_in address = {0};
	int len = sizeof(struct sockaddr_in), ret;
	unsigned char i_addr[4] = {0};
	if ((ret = sock_create(AF_INET, SOCK_STREAM, 0, s)) < 0)
	{
		printk("echo_server: sock_create error");
		return ret;
	}
	convert_from_addr(s_addr, i_addr);
	address.sin_addr.s_addr = *(unsigned int*)i_addr;
	address.sin_port = PORT;
	address.sin_family = AF_INET;
	if ((ret = (*s)->ops->connect(sock, (struct sockaddr*)&address, len, 0)) < 0)
	{
		sock_release(*s);
		*s = NULL;
	}
	return ret;
}

static int echo_server_init(void)
{
	int ret;
	
	if ((ret = my_connect(&sock, "127.0.0.1")) < 0)
	{
		printk("echo_server: connect error");
		return ret;
	}
	mutex_init(&mutex);
	kthread = kthread_run((void *)ksocket_start, NULL, "echo-server");
	if (IS_ERR(kthread))
	{
		sock_release(sock);
		sock = NULL;
		printk(KERN_INFO "echo-server: unable to start kernel thread\n");
		kthread = NULL;
		return -ENOMEM;
	}
	printk(KERN_ALERT "echo-server is on !\n");  
	send_msg(init_msg, size_init_msg);
	return 0;
}

static void echo_server_exit(void)
{
	int err;
	
	send_msg(exit_msg, size_exit_msg);
	if (kthread == NULL)
		printk(KERN_INFO "echo-server: no kernel thread to kill\n");
	else 
	{
		mutex_lock(&mutex);
		err = kthread_stop(kthread);
		//err = kill_proc(kthread->thread->pid, SIGKILL, 1);
		
		/* wait for kernel thread to die */
		if (err < 0)
			printk(KERN_INFO "echo-server: unknown error %d while trying to terminate kernel thread\n", -err);
		else
			printk(KERN_INFO "echo-server: succesfully killed kernel thread!\n");
	}
	if (sock != NULL)
	{
		sock_release(sock);
		sock = NULL;
	}
	
	/* free allocated resources before exit */
	kthread = NULL;
	printk(KERN_ALERT "echo-server is now ending\n");
}

module_init(echo_server_init);

module_exit(echo_server_exit);
