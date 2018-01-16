#ifndef _linux_printk_h
#define _linux_printk_h

#include <linux/kern_levels.h>

#ifdef __cplusplus
extern "C" {
#endif

int printk (const char *fmt, ...);

#ifndef pr_fmt
#define pr_fmt(fmt)	fmt
#endif

#define pr_emerg(fmt, ...)	printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#define pr_alert(fmt, ...)	printk(KERN_ALERT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_crit(fmt, ...)	printk(KERN_CRIT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_err(fmt, ...)	printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warning(fmt, ...)	printk(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warn			pr_warning
#define pr_notice(fmt, ...)	printk(KERN_NOTICE pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info(fmt, ...)	printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
