/*
 * ambpriv.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/pm_runtime.h>
#include <linux/ambpriv_device.h>
#include "base.h"

#define to_ambpriv_driver(drv)	(container_of((drv), struct ambpriv_driver, driver))

struct device ambpriv_bus = {
	.init_name	= "ambpriv",
};

int ambpriv_add_devices(struct ambpriv_device **devs, int num)
{
	int i, ret = 0;

	for (i = 0; i < num; i++) {
		ret = ambpriv_device_register(devs[i]);
		if (ret) {
			while (--i >= 0)
				ambpriv_device_unregister(devs[i]);
			break;
		}
	}

	return ret;
}
EXPORT_SYMBOL(ambpriv_add_devices);

struct ambpriv_object {
	struct ambpriv_device ambdev;
	char name[1];
};

void ambpriv_device_put(struct ambpriv_device *ambdev)
{
	if (ambdev)
		put_device(&ambdev->dev);
}
EXPORT_SYMBOL(ambpriv_device_put);

static void ambpriv_device_release(struct device *dev)
{
	struct ambpriv_object *pa;

	pa = container_of(dev, struct ambpriv_object, ambdev.dev);
	kfree(pa->ambdev.dev.platform_data);
	kfree(pa->ambdev.resource);
	kfree(pa);
}

struct ambpriv_device *ambpriv_device_alloc(const char *name, int id)
{
	struct ambpriv_object *pa;

	pa = kzalloc(sizeof(struct ambpriv_object) + strlen(name), GFP_KERNEL);
	if (pa) {
		strcpy(pa->name, name);
		pa->ambdev.name = pa->name;
		pa->ambdev.id = id;
		device_initialize(&pa->ambdev.dev);
		pa->ambdev.dev.release = ambpriv_device_release;
	}

	return pa ? &pa->ambdev : NULL;
}
EXPORT_SYMBOL(ambpriv_device_alloc);

int ambpriv_device_add_resources(struct ambpriv_device *ambdev,
				  const struct resource *res, unsigned int num)
{
	struct resource *r;

	if (!res)
		return 0;

	r = kmemdup(res, sizeof(struct resource) * num, GFP_KERNEL);
	if (r) {
		ambdev->resource = r;
		ambdev->num_resources = num;
		return 0;
	}
	return -ENOMEM;
}
EXPORT_SYMBOL(ambpriv_device_add_resources);

int ambpriv_device_add_data(struct ambpriv_device *ambdev, const void *data,
			     size_t size)
{
	void *d;

	if (!data)
		return 0;

	d = kmemdup(data, size, GFP_KERNEL);
	if (d) {
		ambdev->dev.platform_data = d;
		return 0;
	}
	return -ENOMEM;
}
EXPORT_SYMBOL(ambpriv_device_add_data);

int ambpriv_device_add(struct ambpriv_device *ambdev)
{
	int i, ret = 0;

	if (!ambdev)
		return -EINVAL;

	if (!ambdev->dev.parent)
		ambdev->dev.parent = &ambpriv_bus;

	ambdev->dev.bus = &ambpriv_bus_type;

	if (ambdev->id != -1)
		dev_set_name(&ambdev->dev, "%s.%d", ambdev->name,  ambdev->id);
	else
		dev_set_name(&ambdev->dev, "%s", ambdev->name);

	for (i = 0; i < ambdev->num_resources; i++) {
		struct resource *p, *r = &ambdev->resource[i];

		if (r->name == NULL)
			r->name = dev_name(&ambdev->dev);

		p = r->parent;
		if (!p) {
			if (resource_type(r) == IORESOURCE_MEM)
				p = &iomem_resource;
			else if (resource_type(r) == IORESOURCE_IO)
				p = &ioport_resource;
		}

		if (p && insert_resource(p, r)) {
			printk(KERN_ERR
			       "%s: failed to claim resource %d\n",
			       dev_name(&ambdev->dev), i);
			ret = -EBUSY;
			goto failed;
		}
	}

	pr_debug("Registering ambpriv device '%s'. Parent at %s\n",
		 dev_name(&ambdev->dev), dev_name(ambdev->dev.parent));

	ret = device_add(&ambdev->dev);
	if (ret == 0)
		return ret;

 failed:
	while (--i >= 0) {
		struct resource *r = &ambdev->resource[i];
		unsigned long type = resource_type(r);

		if (type == IORESOURCE_MEM || type == IORESOURCE_IO)
			release_resource(r);
	}

	return ret;
}
EXPORT_SYMBOL(ambpriv_device_add);

void ambpriv_device_del(struct ambpriv_device *ambdev)
{
	int i;

	if (ambdev) {
		device_del(&ambdev->dev);

		for (i = 0; i < ambdev->num_resources; i++) {
			struct resource *r = &ambdev->resource[i];
			unsigned long type = resource_type(r);

			if (type == IORESOURCE_MEM || type == IORESOURCE_IO)
				release_resource(r);
		}
	}
}
EXPORT_SYMBOL(ambpriv_device_del);

int ambpriv_device_register(struct ambpriv_device *ambdev)
{
	device_initialize(&ambdev->dev);
	return ambpriv_device_add(ambdev);
}
EXPORT_SYMBOL(ambpriv_device_register);

void ambpriv_device_unregister(struct ambpriv_device *ambdev)
{
	ambpriv_device_del(ambdev);
	ambpriv_device_put(ambdev);
}
EXPORT_SYMBOL(ambpriv_device_unregister);

static int ambpriv_drv_probe(struct device *_dev)
{
	struct ambpriv_driver *drv = to_ambpriv_driver(_dev->driver);
	struct ambpriv_device *dev = to_ambpriv_device(_dev);

	return drv->probe(dev);
}

static int ambpriv_drv_remove(struct device *_dev)
{
	struct ambpriv_driver *drv = to_ambpriv_driver(_dev->driver);
	struct ambpriv_device *dev = to_ambpriv_device(_dev);

	return drv->remove(dev);
}

static void ambpriv_drv_shutdown(struct device *_dev)
{
	struct ambpriv_driver *drv = to_ambpriv_driver(_dev->driver);
	struct ambpriv_device *dev = to_ambpriv_device(_dev);

	drv->shutdown(dev);
}

int ambpriv_driver_register(struct ambpriv_driver *drv)
{
	drv->driver.bus = &ambpriv_bus_type;
	if (drv->probe)
		drv->driver.probe = ambpriv_drv_probe;
	if (drv->remove)
		drv->driver.remove = ambpriv_drv_remove;
	if (drv->shutdown)
		drv->driver.shutdown = ambpriv_drv_shutdown;

	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(ambpriv_driver_register);

void ambpriv_driver_unregister(struct ambpriv_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(ambpriv_driver_unregister);

struct ambpriv_device * __init_or_module ambpriv_create_bundle(
			struct ambpriv_driver *driver,
			struct resource *res, unsigned int n_res,
			const void *data, size_t size)
{
	struct ambpriv_device *ambdev;
	int error;

	ambdev = ambpriv_device_alloc(driver->driver.name, -1);
	if (!ambdev) {
		error = -ENOMEM;
		goto err_out;
	}

	error = ambpriv_device_add_resources(ambdev, res, n_res);
	if (error)
		goto err_pdev_put;

	error = ambpriv_device_add_data(ambdev, data, size);
	if (error)
		goto err_pdev_put;

	error = ambpriv_device_add(ambdev);
	if (error)
		goto err_pdev_put;

	error = ambpriv_driver_register(driver);
	if (error)
		goto err_pdev_del;

	return ambdev;

err_pdev_del:
	ambpriv_device_del(ambdev);
err_pdev_put:
	ambpriv_device_put(ambdev);
err_out:
	return ERR_PTR(error);
}
EXPORT_SYMBOL(ambpriv_create_bundle);

static int ambpriv_match(struct device *dev, struct device_driver *drv)
{
	struct ambpriv_device *ambdev = to_ambpriv_device(dev);
	return (strcmp(ambdev->name, drv->name) == 0);
}

#ifdef CONFIG_PM_SLEEP

static int ambpriv_pm_prepare(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (drv && drv->pm && drv->pm->prepare)
		ret = drv->pm->prepare(dev);

	return ret;
}

static void ambpriv_pm_complete(struct device *dev)
{
	struct device_driver *drv = dev->driver;

	if (drv && drv->pm && drv->pm->complete)
		drv->pm->complete(dev);
}

#else /* !CONFIG_PM_SLEEP */

#define ambpriv_pm_prepare		NULL
#define ambpriv_pm_complete		NULL

#endif /* !CONFIG_PM_SLEEP */

#ifdef CONFIG_SUSPEND

int __weak ambpriv_pm_suspend(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->suspend)
		ret = drv->pm->suspend(dev);

	return ret;
}

int __weak ambpriv_pm_suspend_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->suspend_noirq)
		ret = drv->pm->suspend_noirq(dev);

	return ret;
}

int __weak ambpriv_pm_resume(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->resume)
		ret = drv->pm->resume(dev);

	return ret;
}

int __weak ambpriv_pm_resume_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->resume_noirq)
		ret = drv->pm->resume_noirq(dev);

	return ret;
}

#else /* !CONFIG_SUSPEND */

#define ambpriv_pm_suspend		NULL
#define ambpriv_pm_resume		NULL
#define ambpriv_pm_suspend_noirq	NULL
#define ambpriv_pm_resume_noirq	NULL

#endif /* !CONFIG_SUSPEND */

#ifdef CONFIG_HIBERNATION

static int ambpriv_pm_freeze(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->freeze)
		ret = drv->pm->freeze(dev);

	return ret;
}

static int ambpriv_pm_freeze_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->freeze_noirq)
		ret = drv->pm->freeze_noirq(dev);

	return ret;
}

static int ambpriv_pm_thaw(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->thaw)
		ret = drv->pm->thaw(dev);

	return ret;
}

static int ambpriv_pm_thaw_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->thaw_noirq)
		ret = drv->pm->thaw_noirq(dev);

	return ret;
}

static int ambpriv_pm_poweroff(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->poweroff)
		ret = drv->pm->poweroff(dev);

	return ret;
}

static int ambpriv_pm_poweroff_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->poweroff_noirq)
		ret = drv->pm->poweroff_noirq(dev);

	return ret;
}

static int ambpriv_pm_restore(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->restore)
		ret = drv->pm->restore(dev);

	return ret;
}

static int ambpriv_pm_restore_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->restore_noirq)
		ret = drv->pm->restore_noirq(dev);

	return ret;
}

#else /* !CONFIG_HIBERNATION */

#define ambpriv_pm_freeze		NULL
#define ambpriv_pm_thaw		NULL
#define ambpriv_pm_poweroff		NULL
#define ambpriv_pm_restore		NULL
#define ambpriv_pm_freeze_noirq	NULL
#define ambpriv_pm_thaw_noirq		NULL
#define ambpriv_pm_poweroff_noirq	NULL
#define ambpriv_pm_restore_noirq	NULL

#endif /* !CONFIG_HIBERNATION */

#ifdef CONFIG_PM_RUNTIME

int __weak ambpriv_pm_runtime_suspend(struct device *dev)
{
	return pm_generic_runtime_suspend(dev);
};

int __weak ambpriv_pm_runtime_resume(struct device *dev)
{
	return pm_generic_runtime_resume(dev);
};

int __weak ambpriv_pm_runtime_idle(struct device *dev)
{
	return pm_generic_runtime_idle(dev);
};

#else /* !CONFIG_PM_RUNTIME */

#define ambpriv_pm_runtime_suspend NULL
#define ambpriv_pm_runtime_resume NULL
#define ambpriv_pm_runtime_idle NULL

#endif /* !CONFIG_PM_RUNTIME */

static const struct dev_pm_ops ambpriv_dev_pm_ops = {
	.prepare = ambpriv_pm_prepare,
	.complete = ambpriv_pm_complete,
	.suspend = ambpriv_pm_suspend,
	.resume = ambpriv_pm_resume,
	.freeze = ambpriv_pm_freeze,
	.thaw = ambpriv_pm_thaw,
	.poweroff = ambpriv_pm_poweroff,
	.restore = ambpriv_pm_restore,
	.suspend_noirq = ambpriv_pm_suspend_noirq,
	.resume_noirq = ambpriv_pm_resume_noirq,
	.freeze_noirq = ambpriv_pm_freeze_noirq,
	.thaw_noirq = ambpriv_pm_thaw_noirq,
	.poweroff_noirq = ambpriv_pm_poweroff_noirq,
	.restore_noirq = ambpriv_pm_restore_noirq,
	.runtime_suspend = ambpriv_pm_runtime_suspend,
	.runtime_resume = ambpriv_pm_runtime_resume,
	.runtime_idle = ambpriv_pm_runtime_idle,
};

struct bus_type ambpriv_bus_type = {
	.name		= "ambpriv",
	.match		= ambpriv_match,
	.pm		= &ambpriv_dev_pm_ops,
};
EXPORT_SYMBOL(ambpriv_bus_type);

int __init ambpriv_bus_init(void)
{
	int error;

	error = device_register(&ambpriv_bus);
	if (error)
		return error;
	error = bus_register(&ambpriv_bus_type);
	if (error)
		device_unregister(&ambpriv_bus);
	return error;
}

core_initcall(ambpriv_bus_init);

