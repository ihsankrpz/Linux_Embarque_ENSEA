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
	{ 0xd1b09e08, "module_layout" },
	{ 0xb68673cf, "single_release" },
	{ 0xbf9840d6, "seq_read" },
	{ 0x79e93ec9, "seq_lseek" },
	{ 0xe2b24b7c, "remove_proc_entry" },
	{ 0x7c32d0f0, "printk" },
	{ 0x61fa6f7a, "proc_create" },
	{ 0x9b9e8bf0, "seq_printf" },
	{ 0xff2ebbe0, "single_open" },
	{ 0xbdfb6dbb, "__fentry__" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

