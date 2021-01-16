/****************************************************************************************
 *
 * @File Name   : lct_tp_palm.c
 * @Author      : yanglintao
 * @E-mail      : <yanglintao@longcheer.com>
 * @Create Time : 2019-12-26 11:15:12
 * @Description : Enable/Disable touchpad.
 *
 ****************************************************************************************/

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

/*
 * DEFINE CONFIGURATION
 ****************************************************************************************
 */
#define TP_PALM_NAME          "tp_palm"
#define TP_PALM_LOG_ENABLE
#define TP_PALM_TAG           "LCT_TP_PALM"

#ifdef TP_PALM_LOG_ENABLE
#define TP_LOGW(log, ...) printk(KERN_WARNING "[%s] %s (line %d): " log, TP_PALM_TAG, __func__, __LINE__, ##__VA_ARGS__)
#define TP_LOGE(log, ...) printk(KERN_ERR "[%s] %s ERROR (line %d): " log, TP_PALM_TAG, __func__, __LINE__, ##__VA_ARGS__)
#else
#define TP_LOGW(log, ...) {}
#define TP_LOGE(log, ...) {}
#endif

/*
 * DATA STRUCTURES
 ****************************************************************************************
 */
typedef int (*tp_palm_cb_t) (bool enable_tp);

typedef struct lct_tp {
	bool enable_tp_palm_flag;
	struct proc_dir_entry *proc_entry_tp;
	tp_palm_cb_t pfun;
} lct_tp_t;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
static lct_tp_t *lct_tp_p;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
// --- proc ---
static int lct_creat_proc_tp_entry(void);
static ssize_t lct_proc_tp_palm_read(struct file *file, char __user *buf,
				     size_t size, loff_t *ppos);
static ssize_t lct_proc_tp_palm_write(struct file *file,
				      const char __user *buf, size_t size,
				      loff_t *ppos);
static const struct file_operations lct_proc_tp_palm_fops = {
	.read = lct_proc_tp_palm_read,
	.write = lct_proc_tp_palm_write,
};

int init_lct_tp_palm(tp_palm_cb_t callback)
{
	if (NULL == callback) {
		TP_LOGE("callback is NULL!\n");
		return -EINVAL;
	}

	TP_LOGW("Initialization tp_palm node!\n");
	lct_tp_p = kzalloc(sizeof(lct_tp_t), GFP_KERNEL);
	if (IS_ERR_OR_NULL(lct_tp_p)) {
		TP_LOGE("kzalloc() request memory failed!\n");
		return -ENOMEM;
	}
	lct_tp_p->pfun = callback;
	lct_tp_p->enable_tp_palm_flag = true;

	if (lct_creat_proc_tp_entry() < 0) {
		kfree(lct_tp_p);
		return -EPERM;
	}

	return 0;
}

EXPORT_SYMBOL(init_lct_tp_palm);

void uninit_lct_tp_palm(void)
{
	TP_LOGW("uninit /proc/%s ...\n", TP_PALM_NAME);
	if (IS_ERR_OR_NULL(lct_tp_p))
		return;
	if (lct_tp_p->proc_entry_tp != NULL) {
		remove_proc_entry(TP_PALM_NAME, NULL);
		lct_tp_p->proc_entry_tp = NULL;
		TP_LOGW("remove /proc/%s\n", TP_PALM_NAME);
	}
	kfree(lct_tp_p);
	return;
}

EXPORT_SYMBOL(uninit_lct_tp_palm);

void set_lct_tp_palm_status(bool en)
{
	lct_tp_p->enable_tp_palm_flag = en;
}

EXPORT_SYMBOL(set_lct_tp_palm_status);

bool get_lct_tp_palm_status(void)
{
	return lct_tp_p->enable_tp_palm_flag;
}

EXPORT_SYMBOL(get_lct_tp_palm_status);

static int lct_creat_proc_tp_entry(void)
{
	lct_tp_p->proc_entry_tp =
	    proc_create_data(TP_PALM_NAME, 0444, NULL, &lct_proc_tp_palm_fops,
			     NULL);
	if (IS_ERR_OR_NULL(lct_tp_p->proc_entry_tp)) {
		TP_LOGE("add /proc/tp_palm error \n");
		return -EPERM;
	}
	TP_LOGW("/proc/tp_palm is okay!\n");

	return 0;
}

static ssize_t lct_proc_tp_palm_read(struct file *file, char __user *buf,
				     size_t size, loff_t *ppos)
{
	ssize_t cnt = 0;
	char *page = NULL;

	if (*ppos)
		return 0;

	page = kzalloc(128, GFP_KERNEL);
	if (IS_ERR_OR_NULL(page))
		return -ENOMEM;

	cnt =
	    snprintf(page, PAGE_SIZE, "%s",
		     (lct_tp_p->enable_tp_palm_flag ? "1\n" : "0\n"));

	cnt = simple_read_from_buffer(buf, size, ppos, page, cnt);
	if (*ppos != cnt)
		*ppos = cnt;
	TP_LOGW("Touchpad Palm status : %s", page);

	kfree(page);
	return cnt;
}

static ssize_t lct_proc_tp_palm_write(struct file *file,
				      const char __user *buf, size_t size,
				      loff_t *ppos)
{
	int ret;
	ssize_t cnt = 0;
	char *page = NULL;
	unsigned int input = 0;

	page = kzalloc(128, GFP_KERNEL);
	if (IS_ERR_OR_NULL(page))
		return -ENOMEM;
	cnt = simple_write_to_buffer(page, 128, ppos, buf, size);
	if (cnt <= 0)
		return -EINVAL;
	if (sscanf(page, "%u", &input) != 1)
		return -EINVAL;

	if (input > 0) {
		//if (lct_tp_p->enable_tp_palm_flag)
		//	goto exit;
		TP_LOGW("Enbale Touchpad Palm...\n");
		ret = lct_tp_p->pfun(false);
		if (ret) {
			TP_LOGW("Enable Touchpad Palm Failed! ret=%d\n", ret);
			goto exit;
		}
		lct_tp_p->enable_tp_palm_flag = true;
	} else {
		//if (!lct_tp_p->enable_tp_palm_flag)
		//	goto exit;
		TP_LOGW("Disable Touchpad Palm...\n");
		ret = lct_tp_p->pfun(true);
		if (ret) {
			TP_LOGW("Disable Touchpad Palm Failed! ret=%d\n", ret);
			goto exit;
		}
		lct_tp_p->enable_tp_palm_flag = false;
	}
	TP_LOGW("Set Touchpad Palm successfully!\n");

exit:
	kfree(page);
	return cnt;
}

MODULE_DESCRIPTION("Touchpad Palm Controller Driver");
MODULE_LICENSE("GPL");
