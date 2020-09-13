/*
 * Copyright (C) 2014 Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/memblock.h>
#include <linux/kmsg_dump.h>

#include <asm/irq_regs.h>

#include <plat/pmap.h>

#define TCC_DEBUG_MAGIC		0xcafeedeb

struct tcc_debug_buffer {
	u32	magic;
	u32	start;
	u32	kmsg_size;
	u8	kmsg[0];
};

static struct tcc_debug_buffer *tcc_debug_buffer;
static unsigned int tcc_debug_buffer_paddr;
static unsigned int tcc_debug_buffer_size;

static struct proc_dir_entry *proc_dir;

static int kmsg_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, tcc_debug_buffer->kmsg);
	return 0;
}

static int kmsg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, kmsg_proc_show, NULL);
}

static const struct file_operations kmsg_proc_fops = {
	.owner = THIS_MODULE,
	.open = kmsg_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int cpuinfo_proc_show(struct seq_file *m, void *v)
{
	//seq_printf(m, "CPUINFO\n");
	//printk("%s: %p\n", __func__, get_irq_regs());
	return 0;
}

static int cpuinfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpuinfo_proc_show, NULL);
}

static const struct file_operations cpuinfo_proc_fops = {
	.owner = THIS_MODULE,
	.open = cpuinfo_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int tcc_debug_panic_handler(struct notifier_block *nb,
				   unsigned long l, void *buf)
{
	//show_stack(NULL, NULL);

	//show_state();

	return 0;
}

static struct notifier_block nb_panic_block = {
	.notifier_call = tcc_debug_panic_handler,
	.priority = -1,
};

static void tcc_kmsg_dump(struct kmsg_dumper *dumper,
			  enum kmsg_dump_reason reason,
			  const char *s1, unsigned long l1,
			  const char *s2, unsigned long l2)
{
	char *ptr = tcc_debug_buffer->kmsg;

	tcc_debug_buffer->kmsg_size = l1 + l2;

	while (l1-- > 0)
		*ptr++ = *s1++;
	while (l2-- > 0)
		*ptr++ = *s2++;
	*ptr = '\0';

	tcc_debug_buffer->magic = TCC_DEBUG_MAGIC;
}

static struct kmsg_dumper tcc_dumper = {
	.dump = tcc_kmsg_dump,
};

int __init tcc_debug_procfs_init(void)
{
	struct proc_dir_entry *entry;

	proc_dir = proc_mkdir("tcc_debug", NULL);
	if (!proc_dir)
		return -ENOMEM;

	entry = proc_create("kmsg", S_IFREG | S_IRUGO, proc_dir,
			    &kmsg_proc_fops);
	if (!entry)
		return -ENOMEM;

	entry = proc_create("cpuinfo", S_IFREG | S_IRUGO, proc_dir,
			    &cpuinfo_proc_fops);
	if (!entry)
		return -ENOMEM;

	//proc_create("task", S_IFREG | S_IRUGO, proc_dir,
	//	    &task_porc_fops);

	return 0;
}

static int tcc_debug_init_buffer(unsigned int paddr, unsigned int size)
{
	tcc_debug_buffer = ioremap(paddr, size);
	tcc_debug_buffer_size -= sizeof(struct tcc_debug_buffer);

	pr_info("tcc_debug: addr=0x%x (0x%p), size=0x%x\n",
		paddr, tcc_debug_buffer, size);

	if (tcc_debug_buffer->magic == TCC_DEBUG_MAGIC) {
		pr_info("tcc_debug: found existing buffer, 0x%x@0x%p\n",
			tcc_debug_buffer->kmsg_size, tcc_debug_buffer->kmsg);
	} else {
		tcc_debug_buffer->kmsg_size = 0;
	}

	return 0;
}

#if 0
static int __init tcc_debug_setup(char *str)
{
	unsigned int paddr = 0;
	unsigned int size = 0;

	size = memparse(str, &str);
	if (size && *str == '@') {
		paddr = (unsigned int) memparse(++str, NULL);

		tcc_debug_buffer_paddr = paddr;
		tcc_debug_buffer_size = size;

		return 1;
	}
	return 0;
}
__setup("tcc_debug=", tcc_debug_setup);
#endif

static int __init tcc_debug_init(void)
{
	pmap_t pmap;

	if (!pmap_get_info("ram_console", &pmap))
		return -ENODEV;

	tcc_debug_buffer_paddr = pmap.base;
	tcc_debug_buffer_size = pmap.size;

	tcc_debug_init_buffer(tcc_debug_buffer_paddr, tcc_debug_buffer_size);

	kmsg_dump_register(&tcc_dumper);

	atomic_notifier_chain_register(&panic_notifier_list, &nb_panic_block);

	tcc_debug_procfs_init();

	return 0;
}
postcore_initcall(tcc_debug_init);
