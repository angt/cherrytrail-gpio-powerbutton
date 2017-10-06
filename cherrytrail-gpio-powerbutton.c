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

static unsigned int gpioButton = 381;   ///< hard coding the button gpio for this example to P9_27 (GPIO115)
static unsigned int irqNumber;          ///< Used to share the IRQ number within this file
static unsigned int numberPresses = 0;  ///< For information, store the number of button presses

/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  cht_power_button_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int __init cht_power_button_init(void)
{
   int result = 0;
   static struct input_dev *button_dev;
   printk(KERN_INFO "CHT_POWER_BUTTON: Initializing\n");


   button_dev = input_allocate_device();
   if (!button_dev)
   {
      printk(KERN_ERR "CHT_POWER_BUTTON: Not enough memory\n");
      return;
   }
   button_dev->name="GPIO_PWRBTN";
   button_dev->evbit[0] = BIT_MASK(EV_KEY);

   input_set_capability(button_dev, EV_KEY, KEY_POWER);

   if (input_register_device(button_dev))
   {
       printk(KERN_ERR "CHT_POWER_BUTTON: Failed to register device\n");
   }


   gpio_request(gpioButton, "sysfs");
   gpio_direction_input(gpioButton);
   gpio_set_debounce(gpioButton, 200);
   gpio_export(gpioButton, false);

   printk(KERN_INFO "CHT_POWER_BUTTON: The button state is currently: %d\n", gpio_get_value(gpioButton));

   irqNumber = gpio_to_irq(gpioButton);

   printk(KERN_INFO "CHT_POWER_BUTTON: The button is mapped to IRQ: %d\n", irqNumber);

   result = request_irq(irqNumber,
                        (irq_handler_t) cht_power_button_irq_handler,
                        IRQF_TRIGGER_RISING,
                        "CHT_POWER_BUTTON",
                        button_dev);

   printk(KERN_INFO "CHT_POWER_BUTTON: The interrupt request result is: %d\n", result);
   return result;
}

static void __exit cht_power_button_exit(void){
   printk(KERN_INFO "CHT_POWER_BUTTON: The button state is currently: %d\n", gpio_get_value(gpioButton));
   printk(KERN_INFO "CHT_POWER_BUTTON: The button was pressed %d times\n", numberPresses);

   free_irq(irqNumber, NULL);               // Free the IRQ number, no *dev_id required in this case
   gpio_unexport(gpioButton);               // Unexport the Button GPIO
   gpio_free(gpioButton);                   // Free the Button GPIO

   // input_unregister_device(button_dev);

   printk(KERN_INFO "CHT_POWER_BUTTON: Goodbye from the LKM!\n");
}

static irq_handler_t cht_power_button_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
   struct input_dev *button_dev = dev_id;
   printk(KERN_INFO "CHT_POWER_BUTTON: Interrupt! (button state is %d)\n", gpio_get_value(gpioButton));
   numberPresses++;

   input_event(button_dev, EV_KEY, KEY_POWER, 1);
   input_sync(button_dev);
   input_event(button_dev, EV_KEY, KEY_POWER, 0);
   input_sync(button_dev);

   return (irq_handler_t) IRQ_HANDLED;
}

module_init(cht_power_button_init);
module_exit(cht_power_button_exit);
