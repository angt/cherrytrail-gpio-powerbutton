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
	int result = 0;
	printk(KERN_INFO "CHT_POWER_BUTTON: Initializing module\n");
	button_dev = input_allocate_device();
	if (!button_dev)
	{
		printk(KERN_ERR "CHT_POWER_BUTTON: Not enough memory\n");
		return result;
	}
	button_dev->name="GPIO_PWRBTN";
	button_dev->evbit[0] = BIT_MASK(EV_KEY);
	input_set_capability(button_dev, EV_KEY, KEY_POWER);
	if (input_register_device(button_dev))
	{
		printk(KERN_ERR "CHT_POWER_BUTTON: Failed to register virtual input\n");
	}
	gpio_request_one(GPIO_CHT_PWRBTN, GPIOF_DIR_IN | GPIOF_ACTIVE_LOW, "sysfs");
	gpio_set_debounce(GPIO_CHT_PWRBTN, 200);
	gpio_export(GPIO_CHT_PWRBTN, false);
	irqNumber = gpio_to_irq(GPIO_CHT_PWRBTN);
	result = request_threaded_irq(
		irqNumber,
		(irq_handler_t) cht_power_button_irq_handler,
		NULL,
		(IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING),
		"CHT_POWER_BUTTON",
		button_dev
	);
	return result;
}

static void __exit cht_power_button_exit(void)
{
	if(irqNumber)
	{
		free_irq(irqNumber, button_dev);
		gpio_unexport(GPIO_CHT_PWRBTN);
		gpio_free(GPIO_CHT_PWRBTN);
		if (button_dev)
		{
			input_unregister_device(button_dev);
			input_free_device(button_dev);
		}
	}
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
