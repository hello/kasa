
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-vid.h>

#include <plat/adc.h>

static ssize_t show_temper(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 adc_value;
	struct platform_device *pdev = to_platform_device(dev);
	struct ambarella_adc_controller *pd = (struct ambarella_adc_controller *)pdev->dev.platform_data;
	u32 channel = to_sensor_dev_attr(attr)->index;
	adc_value = pd->read_channel(channel);
	if(pd->temper_curve){
		return sprintf(buf, "temperature: %d degree on adc_ch%d\n",pd->temper_curve(adc_value),channel);
	}
	return sprintf(buf, "adc_ch%d value: 0x%x\n",channel,adc_value);
}


static SENSOR_DEVICE_ATTR(adc0, S_IRUGO | S_IWUSR, show_temper,NULL, 0);
static SENSOR_DEVICE_ATTR(adc1, S_IRUGO | S_IWUSR, show_temper,NULL, 1);
static SENSOR_DEVICE_ATTR(adc2, S_IRUGO | S_IWUSR, show_temper,NULL, 2);
static SENSOR_DEVICE_ATTR(adc3, S_IRUGO | S_IWUSR, show_temper,NULL, 3);
static SENSOR_DEVICE_ATTR(adc4, S_IRUGO | S_IWUSR, show_temper,NULL, 4);
static SENSOR_DEVICE_ATTR(adc5, S_IRUGO | S_IWUSR, show_temper,NULL, 5);
static SENSOR_DEVICE_ATTR(adc6, S_IRUGO | S_IWUSR, show_temper,NULL, 6);
static SENSOR_DEVICE_ATTR(adc7, S_IRUGO | S_IWUSR, show_temper,NULL, 7);
static SENSOR_DEVICE_ATTR(adc8, S_IRUGO | S_IWUSR, show_temper,NULL, 8);
static SENSOR_DEVICE_ATTR(adc9, S_IRUGO | S_IWUSR, show_temper,NULL, 9);
static SENSOR_DEVICE_ATTR(adc10, S_IRUGO | S_IWUSR, show_temper,NULL, 10);
static SENSOR_DEVICE_ATTR(adc11, S_IRUGO | S_IWUSR, show_temper,NULL, 11);

static struct device_attribute *amb_adc_attributes[] = {
	&sensor_dev_attr_adc0.dev_attr,
	&sensor_dev_attr_adc1.dev_attr,
	&sensor_dev_attr_adc2.dev_attr,
	&sensor_dev_attr_adc3.dev_attr,
	&sensor_dev_attr_adc4.dev_attr,
	&sensor_dev_attr_adc5.dev_attr,
	&sensor_dev_attr_adc6.dev_attr,
	&sensor_dev_attr_adc7.dev_attr,
	&sensor_dev_attr_adc8.dev_attr,
	&sensor_dev_attr_adc9.dev_attr,
	&sensor_dev_attr_adc10.dev_attr,
	&sensor_dev_attr_adc11.dev_attr,
	NULL
};

static void ambarella_adc_remove_file(struct platform_device *pdev,u32 number)
{
	u32 i = 0;
	struct ambarella_adc_controller *pd = (struct ambarella_adc_controller *)pdev->dev.platform_data;

	if(number != 0){
		for(i=0;i<number;i++){
			if((1 << i) & pd->adc_temper_channel)
				device_remove_file(&pdev->dev, amb_adc_attributes[i]);
		}
	}
}

static int ambarella_adc_temper_probe(struct platform_device *pdev)
{
	int	retval = 0;
	u32	i = 0;
	u32	total_channel_number = 0;
	struct device *hwmon;
	struct ambarella_adc_controller *pd = (struct ambarella_adc_controller *)pdev->dev.platform_data;

	if ((pd == NULL) ||
		(pd->read_channel == NULL) ||
		(pd->reset == NULL) ||
		(pd->open == NULL) ||
		(pd->close == NULL) ||
		(pd->get_channel_num == NULL) ) {
		dev_err(&pdev->dev, "Platform data is NULL!\n");
		retval = -ENXIO;
		return retval;
	}

	total_channel_number = pd->get_channel_num();
	if((pd->adc_temper_channel && (1 << total_channel_number)) > (1 << total_channel_number)){
		printk(KERN_ERR "adc number not support!\n");
		retval = -EPERM;
		return retval;
	}

	pd->open();

	hwmon = kzalloc(sizeof(struct device), GFP_KERNEL);
	if (!hwmon)
		return -ENOMEM;

	for(i=0; i < total_channel_number;i++){
		if((1 << i) & pd->adc_temper_channel){
			retval = device_create_file(&pdev->dev, amb_adc_attributes[i]);
			if (retval){
				printk(KERN_ERR "ambarella_adc_temper_probe create file adc%d failed!\n",i);
				goto ERROR;
			}
		}
	}

	hwmon = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hwmon)) {
		printk(KERN_ERR "error register hwmon device ambarella adc temper\n");
		retval = PTR_ERR(hwmon);
		goto ERROR;
	}

	platform_set_drvdata(pdev, hwmon);
	return retval;

ERROR:
	ambarella_adc_remove_file(pdev,i);
	kfree(hwmon);
	return retval;
}

static int ambarella_adc_temper_remove(struct platform_device *pdev)
{
	int	retval = 0;
	u32	total_channel_number = 0;
	struct device *hwmon = platform_get_drvdata(pdev);
	struct ambarella_adc_controller *pd = (struct ambarella_adc_controller *)pdev->dev.platform_data;

	pd->close();

	total_channel_number = pd->get_channel_num();
	ambarella_adc_remove_file(pdev,total_channel_number);

	hwmon_device_unregister(hwmon);

	kfree(hwmon);

	return retval;
}

static struct platform_driver ambarella_adc_temper_driver = {
	.probe		= ambarella_adc_temper_probe,
	.remove		= ambarella_adc_temper_remove,
	.driver		= {
		.name	= "ambarella-adc-temper",
		.owner	= THIS_MODULE,
	},
};

static int __init ambarella_adc_temper_init(void)
{
	int				retval = 0;

	retval = platform_driver_register(&ambarella_adc_temper_driver);
	if (retval)
		printk(KERN_ERR "Register ambarella_adc_temper_driver failed %d!\n",
			retval);

	return retval;
}

static void __exit ambarella_adc_temper_exit(void)
{
	platform_driver_unregister(&ambarella_adc_temper_driver);
}

module_init(ambarella_adc_temper_init);
module_exit(ambarella_adc_temper_exit);

MODULE_DESCRIPTION("Ambarella Media Processor temperature hardware monitor Driver");
MODULE_AUTHOR("Bingliang Hu, <blhu@ambarella.com>");
MODULE_LICENSE("GPL");

