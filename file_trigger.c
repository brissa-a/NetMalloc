#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>

/* Module */
static char *ip = "127.0.0.1"; module_param(ip, charp, 0);
static int port = 4242; module_param(port, int, 0);

/* Filesystem */
static const struct address_space_operations myfs_aops = {
	.readpage	= simple_readpage,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
};

//myfs_aops
struct inode *myfs_get_inode(struct super_block *sb,
				const struct inode *dir, umode_t mode, dev_t unused)
{
	struct inode * inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, mode);
		inode->i_mapping->a_ops = &myfs_aops;
		mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
		mapping_set_unevictable(inode->i_mapping);
		inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
		switch (mode & S_IFMT) {
		default:
			break;
		case S_IFREG:
			break;
		case S_IFDIR:
			/* directory inodes start off with i_nlink == 2 (for "." entry) */
			inc_nlink(inode);
			break;
		case S_IFLNK:
			break;
		}
	}
	return inode;
}

static const struct super_operations myfs_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
	.show_options	= generic_show_options,
};

//myfs_ops & myfs_getinode
int myfs_fill_super(struct super_block *sb, void *unused, int silent)
{
	struct inode *inode;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_CACHE_SIZE;
	sb->s_blocksize_bits	= PAGE_CACHE_SHIFT;
	sb->s_magic		= 0xBEEF;
	sb->s_op		= &myfs_ops;
	sb->s_time_gran		= 1;

	inode = myfs_get_inode(sb, NULL, 0755 || S_IFDIR, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

//myfs_fill_super
struct dentry *myfs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, myfs_fill_super);
}

static void myfs_kill_sb(struct super_block *sb)
{
	kill_litter_super(sb);
}

//myfs_mount && myfs_kill_sb
static struct file_system_type myfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "myfs",
	.mount		= myfs_mount,
	.kill_sb	= myfs_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
};

//myfs_fs_type
static int __init init_myfs_fs(void) {
	static unsigned long once;
	int err;

	pr_info("myfs: intialized");
	if (test_and_set_bit(0, &once))//Only one fs initialisation
		return 0;
	err = register_filesystem(&myfs_fs_type);
	return err;

}

static int __init mymodule_init(void)
{	
	pr_info("mymodule: runned with %s:%d\n", ip, port);
	init_myfs_fs();
	return 0;
}

static void __exit mymodule_exit(void)
{
	pr_info("mymodule: exit\n");
	unregister_filesystem(&myfs_fs_type);
	return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
