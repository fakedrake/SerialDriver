#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/platform_device.h>


struct fake_serial_tty {
    /* The backend for a console. */
    struct tty_port port;
    /* Handles the communication */
    struct console console;
    spinlock_t lock;
    u32 irq;
    int opencount;
};

static struct tty_driver *fake_serial_driver;
static u32 fake_serial_count = 5;
static u32 current_fake_serial_count;
static struct fake_serial_tty *fake_serials;

static DEFINE_MUTEX(fake_serial_lock);

static struct tty_driver *fake_serial_console_device(struct console *c,
						      int *index)
{
	*index = c->index;
	return fake_serial_driver;
}

static fake_serial_console_setup(struct console *co, char *options)
{
	if((unsigned)co->index > fake_serial_count)
		return -ENODEV;

	return 0;
}

static irqreturn_t fake_serial_interrupt(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}

static int fake_srial_activate(struct tty_port *port, struct tty_struct *tty)
{
	return 0;
}

static int fake_serial_remove(struct platform_device* pdev)
{
    struct fake_serial_tty *qtty;

    mutex_lock(&fake_serial_lock);

    qtty = &fake_serials[pdev->id];
    unregister_console(&qtty->console);
    tty_unregister_device(fake_serial_driver, pdev->id);
    free_irq(qtty->irq, pdev);

    /* Refcount */
    current_fake_serial_count--;
    if(current_fake_serial_count == 0)
	fake_serial_delete_driver();

    mutex_unlock(&fake_serial_lock);
    return 0;
}


static int fake_serial_open(struct tty_struct * tty, struct file * filp)
{
    struct fake_serial *qtty = &fake_serials[tty->index];
    return tty_port_open(&qtty->port, tty, filp);
}

static void fake_serial_close(struct tty_struct * tty, struct file * filp)
{
    tty_port_close(tty->port, tty, filp);
}

static int fake_serial_write(struct tty_struct * tty,
			     const unsigned char *buf, int count)
{
    fake_serial_do_write(tty->index, buf, count);
    return count;
}

static int fake_serial_create_driver (void)
{
    int ret;
    struct tty_driver *tty;

    fake_serials = kzalloc(sizeof(*fake_serials) * fake_serial_count,
			   GFP_KERNEL);

    if(fake_serials == NULL) {
	ret = -ENOMEM;
	goto err_alloc_ttys_failed;
    };
    tty = alloc_tty_driver(fake_serial_line_count);
    if(tty == NULL) {
	ret = -ENOMEM;
	goto err_alloc_tty_driver_failed;
    }

    tty->driver_name = "codebender";
    tty->name = "ttyCBT";
    tty->type = TTY_DRIVER_TYPE_SERIAL;
    tty->subtype = SERIAL_TYPE_NORMAL;
    tty->init_termios = tty_std_termios;
    tty->flags = TTY_DRIVER_RESET_TERMIOS |	\
	TTY_DRIVER_REAL_RAW |			\
	TTY_DRIVER_DYNAMIC_DEV;
    tty_set_operations(tty, &fake_serial_port_ops);
    ret = tty_register_driver(tty);
    if(ret)
	goto err_tty_register_driver_failed;

    fake_serial_driver = tty;
    return 0;

err_tty_register_driver_failed:
    put_tty_driver(tty);
err_alloc_tty_driver_failed:
    kfree(fake_serials);
    fake_serials = NULL;
err_alloc_ttys_failed:
    return ret;
}

static void fake_serial_delete_driver(void)
{
    /* XXX: delete ports too */
    tty_unregister_driver(fake_serial_driver);
    put_tty_driver(fake_serial_driver);
    fake_serial_driver = NULL;
    kfree(fake_serials);
    fake_serials = NULL;
}


static int fake_srial_probe(struct platform_device *pdev)
{
    struct fake_serial_tty *qtty;
    int ret = -EINVAL;
    struct device *ttydev;

    mutex_lock(&fake_serial_lock);
    if(fake_serial_current_line_count == 0) {
	ret = fake_serial_create_driver();
	if(ret)
	    goto err_create_driver_failed;
    }
    fake_serial_current_line_count++;

    /* The tty we are setting up */
    qtty = &fake_serials[pdev->id];

    /* A spinlock */
    spin_lock_init(&qtty->lock);
    tty_port_init(&qtty->port);
    qtty->port.ops = &fake_serial_port_ops;
    qtty->irq = irq;

    ret = request_irq(irq, fake_serial_interrupt, IRQF_SHARED,
		      "fake_serial_tty", pdev);
    if(ret)
	goto err_request_irq_failed;


    /* Register the tty device */
    ttydev = tty_port_register_device(&qtty->port, goldfish_tty_driver,
				      pdev->id, &pdev->dev);
    if(IS_ERR(ttydev)) {
	ret = PTR_ERR(ttydev);
	goto err_tty_register_device_failed;
    }

    /* Setup a console device */
    strcpy(qtty->console.name, "ttyCBT");
    qtty->console.write = fake_serial_console_write;
    qtty->console.device = fake_serial_console_device;
    qtty->console.setup = fake_serial_console_setup;
    qtty->console.flags = CON_PRINTBUFFER;
    qtty->console.index = pdev->id;
    register_console(&qtty->console);

    mutex_unlock(&fake_serial_lock);
    return 0;

    tty_unregister_device(goldfish_tty_driver, i);
err_tty_register_device_failed:
    free_irq(irq, pdev);
err_request_irq_failed:
    goldfish_tty_current_line_count--;
    if(goldfish_tty_current_line_count == 0)
	goldfish_tty_delete_driver();
err_create_driver_failed:
    mutex_unlock(&goldfish_tty_lock);
err_unmap:
    iounmap(base);
    return ret;

}

static struct tty_operations fake_serial_ops = {
    .open = fake_serial_open,
    .close = fake_serial_close,
    .write = fake_serial_write,
};


static struct tty_port_operations fake_serial_port_ops = {
    .activate = fake_serial_activate,
    .shutdown = fake_serial_shutdown
};

static struct platform_driver fake_serial_platform_driver = {
    .probe = fake_serial_tty_probe,
    .remove = fake_serial_tty_remove,
    .driver = {
	.name = "fake_tty_tty"
    }
};

module_platform_driver(fake_serial_platform_driver);

MODULE_LICENSE("GPL v2");
