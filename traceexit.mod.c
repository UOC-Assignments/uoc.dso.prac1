#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xc4837400, "module_layout" },
	{ 0xa6a4a3fe, "single_release" },
	{ 0x93fe6cf1, "seq_read" },
	{ 0xb7873091, "seq_lseek" },
	{ 0xf4396a30, "remove_proc_entry" },
	{ 0x4d0ee1ca, "sys_call_table" },
	{ 0xca1756b6, "proc_create" },
	{ 0x37a0cba, "kfree" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x818d7bdd, "seq_printf" },
	{ 0x1e6a522d, "single_open" },
	{ 0xe445e0e7, "printk" },
	{ 0x20c55ae0, "sscanf" },
	{ 0xd0d8621b, "strlen" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "F7ACBFAF0BEF645FD13396D");
