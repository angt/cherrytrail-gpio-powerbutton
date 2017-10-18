#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/input.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DUPONCHEEL Sébastien");
MODULE_DESCRIPTION("A simple GPIO based power button handler for the Cherrytrail platform");
MODULE_VERSION("0.1");

#define GPIO_CHT_PWRBTN 381
static unsigned int irqNumber;
static struct input_dev *button_dev;

static irq_handler_t  cht_power_button_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int __init cht_power_button_init(void)
{
	int err;
	printk(KERN_INFO "CHT_POWER_BUTTON: Initializing module\n");
	// Initialize power button GPIO
	if((err = gpio_request_one(GPIO_CHT_PWRBTN, GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "sysfs")))
	{
		printk(KERN_ERR "CHT_POWER_BUTTON: Failed to request GPIO pin number %d\n", GPIO_CHT_PWRBTN);
		return -ENXIO;
	};
	if((err = gpio_export(GPIO_CHT_PWRBTN, false)))
	{
		printk(KERN_ERR "CHT_POWER_BUTTON: Failed to export GPIO pin number %d\n", GPIO_CHT_PWRBTN);
		gpio_free(GPIO_CHT_PWRBTN);
		return err;
	};
	irqNumber = gpio_to_irq(GPIO_CHT_PWRBTN);
	if(!irqNumber)
	{
		printk(KERN_ERR "CHT_POWER_BUTTON: Failed to get IRQ for GPIO pin number %d\n", GPIO_CHT_PWRBTN);
		gpio_unexport(GPIO_CHT_PWRBTN);
		gpio_free(GPIO_CHT_PWRBTN);
		return -EINVAL;
	};
	// Initialize virtual input device
	if(!(button_dev = input_allocate_device()))
	{
		printk(KERN_ERR "CHT_POWER_BUTTON: Failed to allocate virtual input device\n");
		gpio_unexport(GPIO_CHT_PWRBTN);
		gpio_free(GPIO_CHT_PWRBTN);
		return -ENOMEM;
	};
	button_dev->name="GPIO_PWRBTN";
	button_dev->evbit[0] = BIT_MASK(EV_KEY);
	input_set_capability(button_dev, EV_KEY, KEY_POWER);
	// Setup IRQ handler
	err = request_threaded_irq(
			irqNumber,
			(irq_handler_t) cht_power_button_irq_handler,
			NULL,
			(IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING),
			"CHT_POWER_BUTTON",
			button_dev
	);
	if(err)
	{
		printk(KERN_ERR "CHT_POWER_BUTTON: Failed to setup handler on IRQ %d\n", irqNumber);
		input_free_device(button_dev);
		gpio_unexport(GPIO_CHT_PWRBTN);
		gpio_free(GPIO_CHT_PWRBTN);
		button_dev = NULL;
		return err;
	};
	// Register virtual input device
	if((err = input_register_device(button_dev)))
	{
		printk(KERN_ERR "CHT_POWER_BUTTON: Failed to register virtual input device\n");
		input_free_device(button_dev);
		free_irq(irqNumber, button_dev);
		gpio_unexport(GPIO_CHT_PWRBTN);
		gpio_free(GPIO_CHT_PWRBTN);
		button_dev = NULL;
		return err;
	};
	return 0;
}

static void __exit cht_power_button_exit(void)
{
	if (button_dev) {
		input_unregister_device(button_dev);
		input_free_device(button_dev);
		free_irq(irqNumber, button_dev);
		button_dev = NULL;
	}
	gpio_unexport(GPIO_CHT_PWRBTN);
	gpio_free(GPIO_CHT_PWRBTN);
}

static irq_handler_t cht_power_button_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
	struct input_dev *button_dev = dev_id;
	printk(KERN_INFO "CHT_POWER_BUTTON: Power button is %s\n", !gpio_get_value(GPIO_CHT_PWRBTN) ? "pressed" : "released");
	input_event(button_dev, EV_KEY, KEY_POWER, !gpio_get_value(GPIO_CHT_PWRBTN));
	input_sync(button_dev);
	return (irq_handler_t) IRQ_HANDLED;
}

module_init(cht_power_button_init);
module_exit(cht_power_button_exit);
