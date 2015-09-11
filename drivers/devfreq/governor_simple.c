/*
 *  linux/drivers/devfreq/governor_simple.c
 *  KGSL Simple GPU Governor 
 *  Copyright (c) 2011-2013, Paul Reioux (Faux123). All rights reserved.
 */

//#include "../gpu/msm/kgsl_device.h"
#include "governor.h"
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>

#define TAG "simple: "

static int devfreq_simple_func(struct devfreq *df,
					unsigned long *freq,
					u32 *flag)
{
	struct devfreq_dev_status stat;
	int err = 0;
	//unsigned long long a, b;
	//unsigned int dfso_upthreshold = DFSO_UPTHRESHOLD;
	//unsigned int dfso_downdifferential = DFSO_DOWNDIFFERENCTIAL;
	//struct devfreq_simple_ondemand_data *data = df->data;
	unsigned long max = (df->max_freq) ? df->max_freq : UINT_MAX;
	unsigned long min = (df->min_freq) ? df->min_freq : 0;
	//int idle_stat = stat.total_time - stat.busy_time;
	//struct kgsl_device *device;
	//struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	static int default_laziness = 5;
	static int laziness;
	int level = 0;

	err = df->profile->get_dev_status(df->dev.parent, &stat);
	if (err)
		pr_err(TAG "get_dev_status failed %d\n", err);
		return err;

	if (level < 0) {
		pr_err(TAG "bad freq %ld\n", stat.current_frequency);
		return level;
	}

	/*if (data) {
		if (data->upthreshold)
			dfso_upthreshold = data->upthreshold;
		if (data->downdifferential)
			dfso_downdifferential = data->downdifferential;
	}*/
	/*if (dfso_upthreshold > 100 ||
	    dfso_upthreshold < dfso_downdifferential)
		return -EINVAL;*/

	// it's currently busy
	if (stat.total_time - stat.busy_time < 6000) {
		if (stat.current_frequency == max)
			*freq = stat.current_frequency; // already maxed, do nothing
		else if (stat.current_frequency < max)
			level = devfreq_get_freq_level(df, stat.current_frequency);
			*freq = df->profile->freq_table[level + 1];
			//kgsl_pwrctrl_pwrlevel_change(device,
					     // pwr->active_pwrlevel - 1);  bump up to next power level
	// idle case
	} else {
		if ((stat.current_frequency <= max) && (stat.current_frequency > min)) {
			if (laziness > 0) {
				/* hold off for a while */
				laziness--;
				*freq = stat.current_frequency; /* don't change anything yet */
			} else {
				level = devfreq_get_freq_level(df, stat.current_frequency);
				*freq = df->profile->freq_table[level - 1];
				//kgsl_pwrctrl_pwrlevel_change(device,
					     //wr->active_pwrlevel + 1);  above min, lower it 
				laziness = default_laziness; /* reset laziness count */
			}
		} else if (stat.current_frequency == min) {
			*freq = stat.current_frequency; /* already @ min, so do nothing */
		}
	}
	return 0;
}

static int devfreq_simple_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	switch (event) {
	case DEVFREQ_GOV_START:
		devfreq_monitor_start(devfreq);
		break;

	case DEVFREQ_GOV_STOP:
		devfreq_monitor_stop(devfreq);
		break;

	case DEVFREQ_GOV_INTERVAL:
		devfreq_interval_update(devfreq, (unsigned int *)data);
		break;

	case DEVFREQ_GOV_SUSPEND:
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		break;

	default:
		break;
	}

	return 0;
}

static struct devfreq_governor devfreq_simple = {
	.name = "simple",
	.get_target_freq = devfreq_simple_func,
	.event_handler = devfreq_simple_handler,
};

static int __init devfreq_simple_init(void)
{
	return devfreq_add_governor(&devfreq_simple);
}
subsys_initcall(devfreq_simple_init);

static void __exit devfreq_simple_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&devfreq_simple);
	if (ret)
		pr_err("%s: failed remove governor %d\n", __func__, ret);

	return;
}
module_exit(devfreq_simple_exit);
MODULE_LICENSE("GPL");

/*start of original simple governor
static int default_laziness = 5;
module_param_named(simple_laziness, default_laziness, int, 0664);

static int ramp_up_threshold = 6000;
module_param_named(simple_ramp_threshold, ramp_up_threshold, int, 0664);

static int laziness;

static int simple_governor(struct kgsl_device *device, int idle_stat)
{
	int val = 0;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	// it's currently busy 
	if (idle_stat < ramp_up_threshold) {
		if (pwr->active_pwrlevel == 0)
			val = 0; // already maxed, so do nothing 
		else if ((pwr->active_pwrlevel > 0) &&
			(pwr->active_pwrlevel <= (pwr->num_pwrlevels - 1)))
			val = -1; // bump up to next pwrlevel
	// idle case
	} else {
		if ((pwr->active_pwrlevel >= 0) &&
			(pwr->active_pwrlevel < (pwr->num_pwrlevels - 1)))
			if (laziness > 0) {
				// hold off for a while
				laziness--;
				val = 0; // don't change anything yet
			} else {
				val = 1; // above min, lower it
				// reset laziness count
				laziness = default_laziness;
			}
		else if (pwr->active_pwrlevel == (pwr->num_pwrlevels - 1))
			val = 0; // already @ min, so do nothing
	}
	return val;
}*/
