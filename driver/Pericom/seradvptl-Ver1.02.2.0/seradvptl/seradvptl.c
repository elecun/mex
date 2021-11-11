/*****************************************************************************/
/*
 *      seradvptl.c  -- Advantech multiport serial driver.
 *
 *      Copyright (C) 2017 Advantech Corporation.
 *
 *      Based on Linux 2.6.9 Kernel's drivers/serial/8250.c
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 *      Multiport Serial Driver ports for Pericom's PCI Family of UARTs (PI7C9X895X)
 *
 *   Check Release Notes for information on what has changed in the new version.
 *
 */
#include "seradvptl.h"


/*
 * Configuration:
 *   share_irqs - whether we pass SA_SHIRQ to request_irq().  This option
 *                is unsafe when used on edge-triggered interrupts.
 */
#define SERIALADV_SHARE_IRQS 1
unsigned int share_irqs = SERIALADV_SHARE_IRQS;

#define PASS_LIMIT	256

/*
 * We default to IRQ0 for the "no irq" hack.   Some
 * machine types want others as well - they're free
 * to redefine this in their header file.
 */
#define is_real_interrupt(irq)	((irq) != 0)

/*
 * This converts from our new CONFIG_ symbols to the symbols
 * that asm/serial.h expects.  You _NEED_ to comment out the
 * linux/config.h include contained inside asm/serial.h for
 * this to work.
 */
#undef CONFIG_SERIAL_MANY_PORTS
#undef CONFIG_SERIAL_DETECT_IRQ
#undef CONFIG_SERIAL_MULTIPORT
#undef CONFIG_HUB6

#ifdef CONFIG_SERIAL_8250_DETECT_IRQ
#define CONFIG_SERIAL_DETECT_IRQ 1
#endif
#ifdef CONFIG_SERIAL_8250_MULTIPORT
#define CONFIG_SERIAL_MULTIPORT 1
#endif
#ifdef CONFIG_SERIAL_8250_MANY_PORTS
#define CONFIG_SERIAL_MANY_PORTS 1
#endif

/*
 * HUB6 is always on.  This will be removed once the header
 * files have been cleaned.
 */
#define CONFIG_HUB6 1

#include <asm/serial.h>

/*
 * SERIAL_PORT_DFNS tells us about built-in ports that have no
 * standard enumeration mechanism.   Platforms that can find all
 * serial ports via mechanisms like ACPI or PCI need not supply it.
 */
#ifndef SERIAL_PORT_DFNS
#define SERIAL_PORT_DFNS
#endif

struct irq_info {
	struct			hlist_node node;
	int			irq;
	spinlock_t		lock;	/* Protects list not the hash */
	struct list_head	*head;
};

/*
 * Here we define the port type of UART.
 */
#define PORT_ADV_895X 1

#define NR_IRQ_HASH		32	/* Can be adjusted later */
static struct hlist_head irq_lists[NR_IRQ_HASH];
static DEFINE_MUTEX(hash_mutex);	/* Used to walk the hash */

/*
 * Here we define the default xmit fifo size used for each type of UART.
 */
#define PORT_MAX_ADV 1
static const struct serialadv_config uart_config[PORT_MAX_ADV+1] = {
	{ "unknown",	1,	1,		0 },
	{ "ADVPTL895x",	128,128, UART_CAP_FIFO | UART_CAP_SLEEP | UART_CAP_EFR },
};

unsigned int serial_in(struct uart_adv_port *up, int offset)
{
	return readb(up->port.membase + offset);
}

void serial_out(struct uart_adv_port *up, int offset, int value)
{
	writeb(value, up->port.membase + offset);
}

/*
 * We used to support using pause I/O for certain machines.  We
 * haven't supported this for a while, but just in case it's badly
 * needed for certain old 386 machines, I've left these #define's
 * in....
 */
#define serial_inp(up, offset)		serial_in(up, offset)
#define serial_outp(up, offset, value)	serial_out(up, offset, value)

/*
 * FIFO support.
 */
static inline void serialadv_clear_fifos(struct uart_adv_port *p)
{
	if (p->capabilities & UART_CAP_FIFO) {
		serial_out(p, PTLSER_FCR_OFFSET, (PTLSER_FCR_FIFO_ENABLE | PTLSER_FCR_RX_FLUSH | PTLSER_FCR_TX_FLUSH));
	}
}

/*
 * IER sleep support.  UARTs which have EFRs need the "extended
 * capability" bit enabled.  Note that on XR16C850s, we need to
 * reset LCR to write to IER.
 */
static inline void serialadv_set_sleep(struct uart_adv_port *p, int sleep)
{
	if (p->capabilities & UART_CAP_SLEEP) {
		if(sleep)
		{
			serial_outp(p, PTLSER_LCR_OFFSET, 0xBF);
			serial_outp(p, PTLSER_ISR_OFFSET, 0);
			serial_outp(p, PTLSER_LCR_OFFSET, 0);
			serial_out(p, PTLSER_SMC_OFFSET, PTLSER_SMC_ENABLE);
		}
		else
		{
			serial_out(p, PTLSER_SMC_OFFSET, PTLSER_SMC_DISABLE);
		}
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
static void serialadv_stop_tx(struct uart_port *port, unsigned int tty_stop)
#else
static void serialadv_stop_tx(struct uart_port *port)
#endif
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	if (up->ier & PTLSER_IER_TX_EMPTY) {
		up->ier &= ~PTLSER_IER_TX_EMPTY;
		serial_out(up, PTLSER_IER_OFFSET, up->ier);
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
static void serialadv_start_tx(struct uart_port *port, unsigned int tty_start)
#else
static void serialadv_start_tx(struct uart_port *port)
#endif
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	if (!(up->ier & PTLSER_IER_TX_EMPTY)) {
		up->ier |= PTLSER_IER_TX_EMPTY;
		serial_out(up, PTLSER_IER_OFFSET, up->ier);
	}
}

static void serialadv_stop_rx(struct uart_port *port)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	up->ier &= ~PTLSER_IER_RX_STATUS;
	up->port.read_status_mask &= ~UART_LSR_DR;
	serial_out(up, PTLSER_IER_OFFSET, up->ier);
}

static void serialadv_enable_ms(struct uart_port *port)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	up->ier |= PTLSER_IER_MODEM_STATUS;
	serial_out(up, PTLSER_IER_OFFSET, up->ier);
}

static _INLINE_ void
receive_chars(struct uart_adv_port *up, int *status)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)  // prevent compile warning
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
	struct tty_struct *tty = up->port.state->port.tty;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
	struct tty_struct *tty = up->port.info->port.tty;
#else
	struct tty_struct *tty = up->port.info->tty;
#endif
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
	struct uart_port *port = &up->port;
#endif

	unsigned char ch[128], lsr = *status;
	int i, max_count;
	char flag;

	max_count = serial_in(up, PTLSER_LRF_OFFSET);
	{
		if (likely(lsr & UART_LSR_DR))
		{
			for(i = 0; i < max_count; i++)
			{
				ch[i] = serial_inp(up, UART_RX);
			}
		}

		flag = TTY_NORMAL;
		up->port.icount.rx += max_count;

		lsr = *status;

	if (unlikely(lsr & (UART_LSR_BI | UART_LSR_PE | UART_LSR_FE | UART_LSR_OE)))
	{
		/*
		* Mask off conditions which should be ignored.
		*/
		lsr &= up->port.read_status_mask;

		if (lsr & UART_LSR_BI)
		{
			flag = TTY_BREAK;
		}
		else if (lsr & UART_LSR_PE)
		{
			flag = TTY_PARITY;
		}
		else if (lsr & UART_LSR_FE)
		{
			flag = TTY_FRAME;
		}
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
		if (uart_handle_sysrq_char(&up->port, ch, NULL))
#else
		if (uart_handle_sysrq_char(&up->port, ch))
#endif
			goto ignore_char;

		for(i = 0; i < max_count; i++)
		{
			uart_insert_char(&up->port, lsr, UART_LSR_OE, ch[i], flag);
		}

ignore_char:
		lsr = serial_inp(up, UART_LSR);
	}
	spin_unlock(&up->port.lock);
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
	tty_flip_buffer_push(&port->state->port);
#else
	tty_flip_buffer_push(tty);
#endif
	spin_lock(&up->port.lock);
	*status = lsr;
}

static _INLINE_ void transmit_chars(struct uart_adv_port *up)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
	struct circ_buf *xmit = &up->port.info->xmit;
#else
	struct circ_buf *xmit = &up->port.state->xmit;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	int tmp;
#endif
	int count, bytes_in_fifo;

	if (up->port.x_char) {
		serial_outp(up, PTLSER_THR_OFFSET, up->port.x_char);
		up->port.icount.tx++;
		up->port.x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&up->port)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
		serialadv_stop_tx(&up->port, 0);
#else
		serialadv_stop_tx(&up->port);
#endif
		return;
	}

	bytes_in_fifo = serial_in(up, PTLSER_TXC_OFFSET);
	count = up->port.fifosize - bytes_in_fifo;

	if (uart_circ_chars_pending(xmit) < count)
		count = uart_circ_chars_pending(xmit);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	if(count > 0)
	{
		//serial_out(up, UART_TX, xmit->buf[xmit->tail]);
		memcpy_toio(up->port.membase + PTLSER_FIFO_D_OFFSET, &(xmit->buf[xmit->tail]), 1);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
	}
#else
	do
	{
		// if the count is more than (tail to end of the buffer), transmit only the rest here.
		// tail+tmp&(UART_XMIT_SIZE-1) will reset the tail to the starting of the circular buffer
		if( ((xmit->tail + count) & (UART_XMIT_SIZE-1)) < xmit->tail)
		{
			tmp = UART_XMIT_SIZE - xmit->tail;
			memcpy_toio(up->port.membase + PTLSER_FIFO_D_OFFSET, &(xmit->buf[xmit->tail]), tmp);
			xmit->tail += tmp;
			xmit->tail &= (UART_XMIT_SIZE-1);
			up->port.icount.tx += tmp;
			count -= tmp;
		}
		else
		{
			memcpy_toio(up->port.membase + PTLSER_FIFO_D_OFFSET, &(xmit->buf[xmit->tail]), count);
			xmit->tail += count;
			xmit->tail &= UART_XMIT_SIZE - 1;
			up->port.icount.tx += count;
			count = 0;
		}

	}while (count > 0);
#endif

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&up->port);

	DEBUG_INTR("THRE...");

	if (uart_circ_empty(xmit))
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
		serialadv_stop_tx(&up->port, 0);
#else
		serialadv_stop_tx(&up->port);
#endif
}

static unsigned int check_modem_status(struct uart_adv_port *up)
{
	unsigned int status = serial_in(up, PTLSER_MSR_OFFSET);

	status |= up->msr_saved_flags;
	up->msr_saved_flags = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
	if (status & UART_MSR_ANY_DELTA && up->ier & UART_IER_MSI)
#else
	if (status & UART_MSR_ANY_DELTA && up->ier & UART_IER_MSI &&
	    up->port.state != NULL)
#endif
	{
		if (status & PTLSER_MSR_DELTA_RI)
			up->port.icount.rng++;
		if (status & PTLSER_MSR_DELTA_DSR)
			up->port.icount.dsr++;
		if (status & PTLSER_MSR_DELTA_DCD)
			uart_handle_dcd_change(&up->port, status & PTLSER_MSR_DELTA_DCD);
		if (status & PTLSER_MSR_DELTA_CTS)
			uart_handle_cts_change(&up->port, status & PTLSER_MSR_DELTA_CTS);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
		wake_up_interruptible(&up->port.info->delta_msr_wait);
#else
		wake_up_interruptible(&up->port.state->port.delta_msr_wait);
#endif
	}

	return status;
}

/*
 * This handles the interrupt from one port.
 */
static inline void
serialadv_handle_port(struct uart_adv_port *up)
{
	unsigned int status = serial_inp(up, PTLSER_LSR_OFFSET);

	DEBUG_INTR("status = %x...", status);

	if (status & PTLSER_LSR_RX_DATA)
		receive_chars(up, &status);
	check_modem_status(up);
	if (status & PTLSER_LSR_TX_EMPTY)
		transmit_chars(up);
}

/*
 * This is the serial driver's interrupt routine.
 *
 * Arjan thinks the old way was overly complex, so it got simplified.
 * Alan disagrees, saying that need the complexity to handle the weird
 * nature of ISA shared interrupts.  (This is a special exception.)
 *
 * In order to handle ISA shared interrupts properly, we need to check
 * that all ports have been serviced, and therefore the ISA interrupt
 * line has been de-asserted.
 *
 * This means we need to loop through all ports. checking that they
 * don't have an interrupt pending.
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 18)
static irqreturn_t serialadv_interrupt(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t serialadv_interrupt(int irq, void *dev_id)
#endif
{
	struct irq_info *i = dev_id;
	struct list_head *l, *end = NULL;
	int pass_counter = 0;
	struct uart_adv_port *up;
	unsigned int iir, lrf;

	DEBUG_INTR("serialadv_interrupt(%d)...", irq);

	spin_lock(&i->lock);

	l = i->head;
	do {
		up = list_entry(l, struct uart_adv_port, list);

		iir = serial_in(up, PTLSER_ISR_OFFSET);
		lrf = serial_in(up, PTLSER_LRF_OFFSET);
		if ((!(iir & PTLSER_ISR_NO_INT_PENDING)) || lrf > 0) {
			spin_lock(&up->port.lock);
			serialadv_handle_port(up);
			spin_unlock(&up->port.lock);

			end = NULL;
		} else if (end == NULL)
			end = l;

		l = l->next;

		if (l == i->head && pass_counter++ > PASS_LIMIT) {
			/* If we hit this, we're dead. */
			DEBUG_AUTOCONF(KERN_ERR "serialadv: too much work for "
				"irq%d\n", irq);
			break;
		}
	} while (l != end);

	spin_unlock(&i->lock);

	DEBUG_INTR("end.\n");
	/* FIXME! Was it really ours? */
	return IRQ_HANDLED;
}

/*
 * To support ISA shared interrupts, we need to have one interrupt
 * handler that ensures that the IRQ line has been deasserted
 * before returning.  Failing to do this will result in the IRQ
 * line being stuck active, and, since ISA irqs are edge triggered,
 * no more IRQs will be seen.
 */
static void serial_do_unlink(struct irq_info *i, struct uart_adv_port *up)
{
	spin_lock_irq(&i->lock);

	if (!list_empty(i->head)) {
		if (i->head == &up->list)
			i->head = i->head->next;
		list_del(&up->list);
	} else {
		BUG_ON(i->head != &up->list);
		i->head = NULL;
	}
	spin_unlock_irq(&i->lock);
	/* List empty so throw away the hash node */
	if (i->head == NULL) {
		hlist_del(&i->node);
		kfree(i);
	}
}

static int serial_link_irq_chain(struct uart_adv_port *up)
{
	struct hlist_head *h;
	struct hlist_node *n;
	struct irq_info *i;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	int ret, irq_flags = up->port.flags & UPF_SHARE_IRQ ? IRQF_SHARED : 0;
#else
	int ret, irq_flags = up->port.flags & UPF_SHARE_IRQ ? SA_SHIRQ : 0;
#endif

	mutex_lock(&hash_mutex);

	h = &irq_lists[up->port.irq % NR_IRQ_HASH];

	hlist_for_each(n, h) {
		i = hlist_entry(n, struct irq_info, node);
		if (i->irq == up->port.irq)
			break;
	}

	if (n == NULL) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9)
		i = kmalloc(sizeof(struct irq_info), GFP_KERNEL);
		memset(i , 0, sizeof(struct irq_info));
#else
		i = kzalloc(sizeof(struct irq_info), GFP_KERNEL);
#endif
		if (i == NULL) {
			mutex_unlock(&hash_mutex);
			return -ENOMEM;
		}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 32)
		spin_lock_init(&i->lock);
#endif
		i->irq = up->port.irq;
		hlist_add_head(&i->node, h);
	}
	mutex_unlock(&hash_mutex);

	spin_lock_irq(&i->lock);

	if (i->head) {
		list_add_tail(&up->list, i->head);
		spin_unlock_irq(&i->lock);

		ret = 0;
	} else {
		INIT_LIST_HEAD(&up->list);
		i->head = &up->list;
		spin_unlock_irq(&i->lock);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
		//irq_flags = up->port.flags & UPF_SHARE_IRQ ? IRQF_SHARED: 0;
		irq_flags |= up->port.irqflags;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
		irq_flags = up->port.flags & UPF_SHARE_IRQ ? SA_SHIRQ : 0;
#else
		irq_flags = up->port.flags & UPF_SHARE_IRQ ? IRQF_SHARED : 0;
#endif
		ret = request_irq(up->port.irq, serialadv_interrupt,
				  irq_flags, "ADVserial", i);
		if (ret < 0)
			serial_do_unlink(i, up);
	}

	return ret;
}

static void serial_unlink_irq_chain(struct uart_adv_port *up)
{
	struct irq_info *i = NULL;
	struct hlist_node *n;
	struct hlist_head *h;

	mutex_lock(&hash_mutex);

	h = &irq_lists[up->port.irq % NR_IRQ_HASH];

	hlist_for_each(n, h) {
		i = hlist_entry(n, struct irq_info, node);
		if (i->irq == up->port.irq)
			break;
	}

	BUG_ON(n == NULL);
	BUG_ON(i->head == NULL);

	if (list_empty(i->head))
		free_irq(up->port.irq, i);

	serial_do_unlink(i, up);
	mutex_unlock(&hash_mutex);
}

/*
 * This function is used to handle ports that do not have an
 * interrupt.  This doesn't work very well for 16450's, but gives
 * barely passable results for a 16550A.  (Although at the expense
 * of much CPU overhead).
 */
static void serialadv_timeout(unsigned long data)
{
	struct uart_adv_port *up = (struct uart_adv_port *)data;
	unsigned int timeout;
	unsigned int iir;

	iir = serial_in(up, PTLSER_ISR_OFFSET);
	if (!(iir & PTLSER_ISR_NO_INT_PENDING)) {
		spin_lock(&up->port.lock);
		serialadv_handle_port(up);
		spin_unlock(&up->port.lock);
	}

	timeout = up->port.timeout;
	timeout = timeout > 6 ? (timeout / 2 - 2) : 1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
    mod_timer(&up->timer.t, jiffies + timeout);
#else
	mod_timer(&up->timer, jiffies + timeout);
#endif
}

static unsigned int serialadv_tx_empty(struct uart_port *port)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;
	unsigned long flags;
	unsigned int ret;

	spin_lock_irqsave(&up->port.lock, flags);
	ret = serial_in(up, PTLSER_LSR_OFFSET) & PTLSER_LSR_TX_COMPLETE ? PTLSER_LSR_TX_COMPLETE : 0;
	spin_unlock_irqrestore(&up->port.lock, flags);

	return ret;
}

static unsigned int serialadv_get_mctrl(struct uart_port *port)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;
	unsigned char status;
	unsigned int ret;

	status = check_modem_status(up);

	ret = 0;
	if (status & PTLSER_MSR_DCD)
		ret |= TIOCM_CAR;
	if (status & PTLSER_MSR_RI)
		ret |= TIOCM_RNG;
	if (status & PTLSER_MSR_DSR)
		ret |= TIOCM_DSR;
	if (status & PTLSER_MSR_CTS)
		ret |= TIOCM_CTS;

	return ret;
}

static void serialadv_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;
	unsigned char mcr = 0;

	if (mctrl & TIOCM_RTS)
		mcr |= PTLSER_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= PTLSER_MCR_DTR;
	if (mctrl & TIOCM_OUT1)
		mcr |= PTLSER_MCR_OUTPUT1;
	if (mctrl & TIOCM_OUT2)
		mcr |= PTLSER_MCR_OUTPUT2;
	if (mctrl & TIOCM_LOOP)
		mcr |= PTLSER_MCR_INTERNAL_LOOPBACK;

	mcr = (mcr & up->mcr_mask) | up->mcr_force | up->mcr;

	serial_out(up, PTLSER_MCR_OFFSET, mcr);
}

static void serialadv_break_ctl(struct uart_port *port, int break_state)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;
	unsigned long flags;

	spin_lock_irqsave(&up->port.lock, flags);
	if (break_state == -1)
		up->lcr |= PTLSER_LCR_BREAK;
	else
		up->lcr &= ~PTLSER_LCR_BREAK;
	serial_out(up, PTLSER_LCR_OFFSET, up->lcr);
	spin_unlock_irqrestore(&up->port.lock, flags);
}

static int serialadv_startup(struct uart_port *port)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;
	unsigned long flags;
	int retval;
	unsigned char efr;

	up->capabilities = uart_config[up->port.type].flags;
	up->mcr = 0;

	/*
	 * Clear the FIFO buffers and disable them.
	 * (they will be reeanbled in set_termios())
	 */
	serialadv_clear_fifos(up);

	/*
	 * Clear the interrupt registers.
	 */
	(void) serial_inp(up, PTLSER_LSR_OFFSET);
	(void) serial_inp(up, PTLSER_RHR_OFFSET);
	(void) serial_inp(up, PTLSER_ISR_OFFSET);
	(void) serial_inp(up, PTLSER_MSR_OFFSET);

	/*
	 * At this point, there's no way the LSR could still be 0xff;
	 * if it is, then bail out, because there's likely no UART
	 * here.
	 */
	if (!(up->port.flags & UPF_BUGGY_UART) &&
	    (serial_inp(up, PTLSER_LSR_OFFSET) == 0xff)) {
		printk("ttyAP%d: LSR safety check engaged!\n", up->port.line);
		return -ENODEV;
	}

	/*
	 * If the "interrupt" for this port doesn't correspond with any
	 * hardware interrupt, we use a timer-based system.  The original
	 * driver used to do this with IRQ0.
	 */
	if (!is_real_interrupt(up->port.irq)) {
		unsigned int timeout = up->port.timeout;

		timeout = timeout > 6 ? (timeout / 2 - 2) : 1;

		up->timer.data = (unsigned long)up;
		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
        mod_timer(&up->timer.t, jiffies + timeout);
#else
		mod_timer(&up->timer, jiffies + timeout);
#endif
	} else {
		retval = serial_link_irq_chain(up);
		if (retval)
			return retval;
	}

	/*
	 * Now, initialize the UART
	 */
	serial_outp(up, PTLSER_LCR_OFFSET, PTLSER_LCR_WORD_WIDTH_8);

	spin_lock_irqsave(&up->port.lock, flags);
	if (up->port.flags & UPF_FOURPORT) {
		if (!is_real_interrupt(up->port.irq))
			up->port.mctrl |= TIOCM_OUT1;
	} else
		/*
		 * Most PC uarts need OUT2 raised to enable interrupts.
		 */
		if (is_real_interrupt(up->port.irq))
			up->port.mctrl |= TIOCM_OUT2;

	serialadv_set_mctrl(&up->port, up->port.mctrl);
	spin_unlock_irqrestore(&up->port.lock, flags);

	/*
	 * Finally, enable interrupts.  Note: Modem status interrupts
	 * are set via set_termios(), which will be occurring imminently
	 * anyway, so we don't enable them here.
	 */
	up->ier = UART_IER_RLSI | UART_IER_RDI;
	serial_outp(up, PTLSER_IER_OFFSET, up->ier);

	if (up->port.flags & UPF_FOURPORT) {
		unsigned int icp;
		/*
		 * Enable interrupts on the AST Fourport board
		 */
		icp = (up->port.iobase & 0xfe0) | 0x01f;
		outb_p(0x80, icp);
		(void) inb_p(icp);
	}

	efr = serial_in(up, PTLSER_EFR_OFFSET);
	serial_out(up, PTLSER_EFR_OFFSET, efr | PTLSER_EFR_ENHANCE_MODE);

	serial_out(up, PTLSER_FCL_OFFSET, 16);
	serial_out(up, PTLSER_FCH_OFFSET, 110);

	/*
	 * And clear the interrupt registers again for luck.
	 */
	(void) serial_inp(up, PTLSER_LSR_OFFSET);
	(void) serial_inp(up, PTLSER_RHR_OFFSET);
	(void) serial_inp(up, PTLSER_ISR_OFFSET);
	(void) serial_inp(up, PTLSER_MSR_OFFSET);

	return 0;
}

static void serialadv_shutdown(struct uart_port *port)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;
	unsigned long flags;

	/*
	 * Disable interrupts from this port
	 */
	up->ier = 0;
	serial_outp(up, PTLSER_IER_OFFSET, 0);

	spin_lock_irqsave(&up->port.lock, flags);
	if (up->port.flags & UPF_FOURPORT) {
		/* reset interrupts on the AST Fourport board */
		inb((up->port.iobase & 0xfe0) | 0x1f);
		up->port.mctrl |= TIOCM_OUT1;
	} else
		up->port.mctrl &= ~TIOCM_OUT2;

	serialadv_set_mctrl(&up->port, up->port.mctrl);
	spin_unlock_irqrestore(&up->port.lock, flags);

	/*
	 * Disable break condition and FIFOs
	 */
	serial_out(up, PTLSER_LCR_OFFSET, serial_inp(up, PTLSER_LCR_OFFSET) & ~PTLSER_LCR_BREAK);
	serialadv_clear_fifos(up);

	/*
	 * Read data port to reset things, and then unlink from
	 * the IRQ chain.
	 */
	(void) serial_in(up, PTLSER_RHR_OFFSET);

	if (!is_real_interrupt(up->port.irq))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
        del_timer_sync(&up->timer.t);
#else
		del_timer_sync(&up->timer);
#endif
	else
		serial_unlink_irq_chain(up);
}

static unsigned int serialadv_get_divisor(struct uart_adv_port *up, unsigned int baud_rate)
{
	unsigned long input_frequency;	/* 14.7456MHz                                */
	unsigned char sample_clock;		/* sample_clock = 16 - SCR                  */
	unsigned char SCR = 0;			/* SCR: 0x0 ~ 0xc                            */
	unsigned char M = 0x1;			/* M: 0x1 ~ 0x2                              */
	unsigned char N = 0x0;			/* N: 0x0 ~ 0x7                              */
	unsigned long divisor;			/* Divisor = (inputFrequency/baud)/Prescaler */
	unsigned char prescaler;		/* 2^(M-1) * (sampleClock + N)               */

	unsigned char data, sfr;
	unsigned char best_SCR;
	unsigned char best_M;
	unsigned char best_N;
	unsigned long best_divisor;
	long          desired_baud, best_baud_rate, diff_baud, diff_curr_baud;

	input_frequency   = 14745600;
	desired_baud      = baud_rate;
	best_baud_rate    = 0;
	best_SCR          = SCR;
	best_M            = M;
	best_N            = N;
	best_divisor      = 0;

	sfr = serial_in(up, PTLSER_SFR_OFFSET);

	for(M = 0x1; M <= 0x2; M++)
	{
		for(SCR = 0x0; SCR <= 0xc; SCR++)
		{
			for(N = 0x0; N <= 0x7; N++)
			{
				sample_clock = 16 - SCR;
				prescaler   = ((M == 0x1)? 0x1 : 0x2) * (sample_clock + N);
				if(prescaler == 0)
				{
					continue;
				}

				divisor = (input_frequency / desired_baud) / prescaler;

				if(divisor == 0)
				{
					continue;
				}
				baud_rate = input_frequency / (divisor * prescaler);

				diff_curr_baud = baud_rate - desired_baud;
				if(diff_curr_baud < 0)
				{
					diff_curr_baud = 0 - diff_curr_baud;
				}

				diff_baud = best_baud_rate - desired_baud;
				if(diff_baud < 0)
				{
					diff_baud = 0 - diff_baud;
				}

				if(diff_baud > diff_curr_baud)
				{
					best_SCR          = SCR;
					best_M            = M;
					best_N            = N;
					best_divisor      = divisor;

					best_baud_rate  = baud_rate;
				}

				if(best_baud_rate == desired_baud)
				{
					goto done;
				}
			}
		}
	}

done:
	serial_out(up, PTLSER_SFR_OFFSET, PTLSER_SFR_950);

	// SC
	serial_out(up, PTLSER_SCR_OFFSET, best_SCR);

	// DLL
	serial_out(up, PTLSER_DLL_OFFSET, (unsigned char)(best_divisor & 0x00ff));

	// DLH
	serial_out(up, PTLSER_DLH_OFFSET, (unsigned char)((best_divisor & 0xff00) >> 8));

	// CPR
	serial_out(up, PTLSER_CPR_OFFSET, (unsigned char)(CPR_VALUE(best_M, best_N)));

	serial_out(up, PTLSER_SFR_OFFSET, sfr | (PTLSER_SFR_950 | PTLSER_SFR_LSR_FIFO_COUNT | PTLSER_SFR_TX_FIFO_COUNT));

	// Fifo size
	if(best_baud_rate <= 9600){
		data = 0x10;
	}
	else {
		data = 0x60;
	}

	DEBUG_AUTOCONF("best_baud_rate = %ld\n", best_baud_rate);

	serial_out(up, PTLSER_RTL_OFFSET, (unsigned char)data);
	serial_out(up, PTLSER_TTL_OFFSET, (unsigned char)up->tx_loadsz);

	return best_divisor;
}

static void serial_set_ctsrts(struct uart_port *port, int turn_on)
{
	unsigned char old_efr, old_mcr;
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	old_efr = serial_in(up, PTLSER_EFR_OFFSET);
	old_mcr = serial_in(up, PTLSER_MCR_OFFSET);

	if(turn_on)
	{
		old_efr |= (PTLSER_EFR_AUTO_RTS | PTLSER_EFR_AUTO_CTS);
		old_mcr |= PTLSER_MCR_RTS;
	}
	else
	{
		old_efr &= ~(PTLSER_EFR_AUTO_RTS | PTLSER_EFR_AUTO_CTS);
		/* Force RTS# output to a HIGH (default) */
		old_mcr &= ~PTLSER_MCR_RTS;
	}

	serial_out(up, PTLSER_EFR_OFFSET, old_efr);
	serial_out(up, PTLSER_MCR_OFFSET, old_mcr);

	return;
}

static void serial_set_dtrdsr(struct uart_port *port, int turn_on)
{
	unsigned char old_sfr, old_mcr;
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	old_sfr = serial_in(up, PTLSER_SFR_OFFSET);
	old_mcr = serial_in(up, PTLSER_MCR_OFFSET);

	if(turn_on)
	{
		old_sfr |= PTLSER_SFR_AUTO_DSR_DTR;
		old_mcr |= PTLSER_MCR_DTR;
	}
	else
	{
		old_sfr &= ~PTLSER_SFR_AUTO_DSR_DTR;
		/* Force DTR# output to a HIGH (default) */
		old_mcr &= ~PTLSER_MCR_DTR;
	}

	serial_out(up, PTLSER_SFR_OFFSET, old_sfr);
	serial_out(up, PTLSER_MCR_OFFSET, old_mcr);
}

static void serial_set_xonxoff(struct uart_port *port, int turn_on)
{
	unsigned char old_efr;
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	old_efr = serial_in(up, PTLSER_EFR_OFFSET);

	if(turn_on)
	{
		serial_out(up, PTLSER_XON1_OFFSET,  0x11); //Initializing XON1
		serial_out(up, PTLSER_XOFF1_OFFSET, 0x13); //Initializing XOFF1

		old_efr |= (PTLSER_EFR_INBAND_RX_MODE_1 | PTLSER_EFR_INBAND_TX_MODE_1);
	}
	else
	{
		old_efr &= ~(PTLSER_EFR_INBAND_RX_MODE_1 | PTLSER_EFR_INBAND_TX_MODE_1);
	}

	serial_out(up, PTLSER_EFR_OFFSET, old_efr);
}

static void serial_set_xon_any(struct uart_port *port, int turn_on)
{
	unsigned char old_sfr;
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	old_sfr = serial_in(up, PTLSER_SFR_OFFSET);

	if(turn_on)
	{
		old_sfr |= PTLSER_SFR_IXON_ANY;
	}
	else
	{
		old_sfr &= ~PTLSER_SFR_IXON_ANY;
	}

	serial_out(up, PTLSER_SFR_OFFSET, old_sfr);
}

static void
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
serialadv_set_termios(struct uart_port *port, struct termios *termios,
		       struct termios *old)
#else
serialadv_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old)
#endif
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;
	unsigned char cval;
	unsigned long flags;
	unsigned int baud = 0, quot = 0;

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		cval = PTLSER_LCR_WORD_WIDTH_5;
		break;
	case CS6:
		cval = PTLSER_LCR_WORD_WIDTH_6;
		break;
	case CS7:
		cval = PTLSER_LCR_WORD_WIDTH_7;
		break;
	default:
	case CS8:
		cval = PTLSER_LCR_WORD_WIDTH_8;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		cval |= UART_LCR_STOP;
	if (termios->c_cflag & PARENB)
		cval |= UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		cval |= UART_LCR_EPAR;
#ifdef CMSPAR
	if (termios->c_cflag & CMSPAR)
		cval |= UART_LCR_SPAR;
#endif

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, 921600);
	if((port->flags & UPF_SPD_MASK) != UPF_SPD_CUST)
	{
		quot = serialadv_get_divisor(up, baud);
	}

	if(((termios->c_iflag) & IXOFF)&&((termios->c_iflag) & IXON))
	{
		serial_set_xonxoff(port, 1);
	}
	else
	{
		serial_set_xonxoff(port, 0);
	}

	if((termios->c_iflag) & IXANY)
	{
		serial_set_xon_any(port, 1);
	}
	else
	{
		serial_set_xon_any(port, 0);
	}

	serial_set_ctsrts(port, 0);
	serial_set_dtrdsr(port, 0);
	if(termios->c_cflag & CRTSCTS)
	{
		serial_set_ctsrts(port, 1);
	}
	else if(termios->c_cflag & CDTRDSR)
	{
		serial_set_dtrdsr(port, 1);
	}

	/*
	 * Ok, we're now changing the port state.  Do it with
	 * interrupts disabled.
	 */
	spin_lock_irqsave(&up->port.lock, flags);

	/*
	 * Update the per-port timeout.
	 */
	if(!((baud == 38400) && ((port->flags & UPF_SPD_MASK) == UPF_SPD_CUST)))
	{
		uart_update_timeout(port, termios->c_cflag, baud);
	}

	up->port.read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		up->port.read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		up->port.read_status_mask |= UART_LSR_BI;

	/*
	 * Characteres to ignore
	 */
	up->port.ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		up->port.ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		up->port.ignore_status_mask |= UART_LSR_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			up->port.ignore_status_mask |= UART_LSR_OE;
	}

	/*
	 * ignore all characters if CREAD is not set
	 */
	if ((termios->c_cflag & CREAD) == 0)
		up->port.ignore_status_mask |= UART_LSR_DR;

	/*
	 * modem status interrupts
	 */
	up->ier &= ~PTLSER_IER_MODEM_STATUS;
	if (UART_ENABLE_MS(&up->port, termios->c_cflag))
		up->ier |= PTLSER_IER_MODEM_STATUS;

	serial_out(up, PTLSER_IER_OFFSET, up->ier);

	serial_outp(up, PTLSER_LCR_OFFSET, cval);		/* set DLAB */
	up->lcr = cval;					/* Save LCR */

	serialadv_set_mctrl(&up->port, up->port.mctrl);

	if(up->serial_mode == SERIAL_MODE_RS485ORRS422S)
	{
		serialadv_set_auto485(up, 1, 0);
	}
	else
	{
		serialadv_set_auto485(up, 0, 1);
	}

	spin_unlock_irqrestore(&up->port.lock, flags);
}

struct advioctl_rw_reg {
	unsigned char reg;
	unsigned char regvalue;
};
/*
 * This function is used to handle Advantech Device specific ioctl calls
 * The user level application should have defined the above ioctl
 * commands with the above values to access these ioctls and the
 * input parameters for these ioctls should be struct advioctl_rw_reg
 * The Ioctl functioning is pretty much self explanatory here in the code,
 * and the register values should be between 0 to XR_17V35X_EXTENDED_RXTRG
 */

static int
serialadv_ioctl(struct uart_port *port, unsigned int cmd, unsigned long arg)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;
	int ret = -ENOIOCTLCMD;
	struct advioctl_rw_reg ioctlrwarg;

	switch (cmd)
	{
		case ADV_READ_REG:
			if(copy_from_user(&ioctlrwarg, (void *)arg, sizeof(ioctlrwarg)))
				return -EFAULT;
			ioctlrwarg.regvalue = serial_in(up, ioctlrwarg.reg);
			if (copy_to_user((void *)arg, &ioctlrwarg, sizeof(ioctlrwarg)))
				return -EFAULT;
			DEBUG_INTR(KERN_INFO "serialadv_ioctl read reg[0x%02x]=0x%02x \n",ioctlrwarg.reg,ioctlrwarg.regvalue);
			ret = 0;
			break;

		case ADV_WRITE_REG:
			if(copy_from_user(&ioctlrwarg, (void *)arg, sizeof(ioctlrwarg)))
				return -EFAULT;
			serial_out(up, ioctlrwarg.reg, ioctlrwarg.regvalue);
			DEBUG_INTR(KERN_INFO "serialadv_ioctl write reg[0x%02x]=0x%02x \n",ioctlrwarg.reg,ioctlrwarg.regvalue);
			ret = 0;
			break;

		// serial have RS232/RS422/RS485 mode, get it
		case ADV_GET_SERIAL_MODE:
			DEBUG_INTR(KERN_INFO"enter ADV_GET_SERIAL_MODE\n");
			if(copy_to_user((void *)arg, &up->serial_mode, sizeof(int)))
				return -EFAULT;
			ret = 0;
			break;
	}

	return ret;
}

static void
serialadv_pm(struct uart_port *port, unsigned int state,
	      unsigned int oldstate)
{
	struct uart_adv_port *p = (struct uart_adv_port *)port;

	serialadv_set_sleep(p, state != 0);

	if (p->pm)
		p->pm(port, state, oldstate);
}

static void serialadv_release_port(struct uart_port *port)
{
	return;
}

static int serialadv_request_port(struct uart_port *port)
{
	return 0;
}

static void serialadv_config_port(struct uart_port *port, int flags)
{
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	up->capabilities = uart_config[up->port.type].flags;
	return;
}

static int
serialadv_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	struct uart_adv_port *up = (struct uart_adv_port *)port;

	if(ser->custom_divisor > 0)
		serialadv_get_divisor(up, ser->baud_base/ser->custom_divisor);

	return ret;
}

static const char *
serialadv_type(struct uart_port *port)
{
	int type = port->type;

	if (type >= ARRAY_SIZE(uart_config))
		type = 0;
	return uart_config[type].name;
}

static struct uart_ops serialadv_pops = {
	.tx_empty	= serialadv_tx_empty,
	.set_mctrl	= serialadv_set_mctrl,
	.get_mctrl	= serialadv_get_mctrl,
	.stop_tx	= serialadv_stop_tx,
	.start_tx	= serialadv_start_tx,
	.stop_rx	= serialadv_stop_rx,
	.enable_ms	= serialadv_enable_ms,
	.break_ctl	= serialadv_break_ctl,
	.startup	= serialadv_startup,
	.shutdown	= serialadv_shutdown,
	.set_termios	= serialadv_set_termios,
	.pm				= serialadv_pm,
	.type			= serialadv_type,
	.release_port	= serialadv_release_port,
	.request_port	= serialadv_request_port,
	.config_port	= serialadv_config_port,
	.verify_port	= serialadv_verify_port,
	.ioctl			= serialadv_ioctl,
};

static struct uart_adv_port serialadv_ports[NR_PORTS];

#define SERIALADV_CONSOLE	NULL

#ifdef FIXED_NUMBER_FUNC
static struct uart_driver serialadv_reg[MAX_CARD_SUPPORT];
static char adv_dev_name[MAX_CARD_SUPPORT][MAX_STRING_LEN];
static char adv_driver_name[MAX_CARD_SUPPORT][MAX_STRING_LEN];
unsigned int old_method_cnt = 0;
#else
static struct uart_driver serialadv_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "serialadv",
	.dev_name		= "ttyAP",
	.major			= 0,
	.minor			= 0,
	.nr			= NR_PORTS,
};
#endif

/*
 * serial8250_register_port and serial8250_unregister_port allows for
 * 16x50 serial ports to be configured at run-time, to support PCMCIA
 * modems and PCI multiport cards.
 */
static struct uart_adv_port *serialadv_find_match_or_unused(struct uart_port *port)
{
	int i;

	/*
	 * First, find a port entry which matches.
	 */
	for (i = 0; i < NR_PORTS; i++)
		if (uart_match_port(&serialadv_ports[i].port, port))
		{
			serialadv_ports[i].port.line = i;
			return &serialadv_ports[i];
		}

	/*
	 * We didn't find a matching entry, so look for the first
	 * free entry.  We look for one which hasn't been previously
	 * used (indicated by zero iobase).
	 */
	for (i = 0; i < NR_PORTS; i++)
		if (serialadv_ports[i].port.type == PORT_UNKNOWN &&
		    serialadv_ports[i].port.iobase == 0
			&& serialadv_ports[i].port.membase == 0)
			{
				serialadv_ports[i].port.line = i;
				return &serialadv_ports[i];
			}

	/*
	 * That also failed.  Last resort is to find any entry which
	 * doesn't have a real port associated with it.
	 */
	for (i = 0; i < NR_PORTS; i++)
		if (serialadv_ports[i].port.type == PORT_UNKNOWN)
		{
			serialadv_ports[i].port.line = i;
			return &serialadv_ports[i];
		}

	return NULL;
}

#ifdef FIXED_NUMBER_FUNC
void init_adv_uart_struct(void)
{
	int i;

	/*
	 * We do not init "dev_name", "nr" here, these will be
	 * initiated after getting boardID.
	 */
	for(i = 0; i < MAX_CARD_SUPPORT; i++)
	{
		serialadv_reg[i].owner		= THIS_MODULE;
		serialadv_reg[i].driver_name	= "serialadv";
		serialadv_reg[i].major		= 0;
		serialadv_reg[i].minor		= 0 + i;
		serialadv_reg[i].cons			= SERIALADV_CONSOLE;
	}

	return;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
static void legacy_timer_func(struct timer_list *t)
{
	struct legacy_timer_emu *lt = from_timer(lt, t, t);
	lt->function(lt->data);
}
#endif

void serialadv_register_port(struct serial_struct *req, struct pci_dev *dev,
								int index, int nr_ports)
{
	struct uart_adv_port *up, tmp_up;
#ifdef FIXED_NUMBER_FUNC
	int i, ret;
	char tmp_string[MAX_STRING_LEN], tmp_dev_name[MAX_STRING_LEN];
#else
	struct uart_driver *drv = &serialadv_reg;
#endif

	tmp_up.port.iobase   = req->port;
	tmp_up.port.mapbase  = req->iomap_base;
	up = serialadv_find_match_or_unused(&tmp_up.port);
	if(up)
	{
		up->port.ops = &serialadv_pops;
		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	    timer_setup(&up->timer.t, legacy_timer_func, 0);
#else
		init_timer(&up->timer);
#endif
		up->timer.function = serialadv_timeout;
		up->port.iobase   = req->port;
		up->port.membase  = req->iomem_base;
		up->port.irq      = req->irq;
		up->port.uartclk  = 14745600;
		up->port.fifosize = 0x80;
		up->port.regshift = req->iomem_reg_shift;
		up->port.iotype   = req->io_type;
		up->port.flags    = req->flags;
		up->port.mapbase  = req->iomap_base;
		up->port.type     = PORT_ADV_895X;
		up->index         = index;
		up->capabilities  = uart_config[up->port.type].flags;
		up->tx_loadsz     = uart_config[up->port.type].tx_loadsz;
		up->device_id     = dev->device;

		if (share_irqs)
			up->port.flags |= UPF_SHARE_IRQ;

		if (HIGH_BITS_OFFSET)
			up->port.iobase |= (long) req->port_high << HIGH_BITS_OFFSET;

		/*
		 * ALPHA_KLUDGE_MCR needs to be killed.
		 */
		up->mcr_mask = ~ALPHA_KLUDGE_MCR;
		up->mcr_force = ALPHA_KLUDGE_MCR;

		/*
		 * Get run mode(RS232/RS422/RS485)
		 */
		up->serial_mode = serialadv_get_run_mode(up);

		/*
		 * Get board ID
		 */
		up->board_id = serialadv_get_board_id(up);

#ifdef FIXED_NUMBER_FUNC
		up->current_card_nr = nr_ports;
		if(up->board_id == 0x0)
		{
			/*
			 * Use old naming methods "ttyAP*"
			 * WARNING: DO NOT try to modify this value "state" anywhere,
			 * this is private value for kernel
			 */
			if(serialadv_reg[0].state == NULL)
			{
				/*
				 * The first member serialadv_reg[0] is reserved for "ttyAP*"
				 */
				memset(adv_dev_name[0], 0, sizeof(adv_dev_name[0]));
				strncpy(adv_dev_name[0], "ttyAP", sizeof(adv_dev_name[0]) - 1);
				serialadv_reg[0].dev_name = adv_dev_name[0];
				serialadv_reg[0].nr = NR_PORTS;

				uart_register_driver(&serialadv_reg[0]);
			}

			/*
			 * Add this port to first member
			 */
			up->port.line = old_method_cnt++;
			up->fix_number_index = 0;
			ret = uart_add_one_port(&serialadv_reg[0], &up->port);
		}
		else
		{
			/*
			 * Enable fixed number.
			 * Fill temp buffer, "ttyB" + boardID + "P" + port_num
			 */
			memset(tmp_dev_name, 0, sizeof(tmp_dev_name));
			strncpy(tmp_dev_name, "ttyB", sizeof(tmp_dev_name) - 1);
			sprintf(tmp_string, "%02d", up->board_id);
			strncat(tmp_dev_name, tmp_string,
					 sizeof(tmp_dev_name) - strlen(tmp_dev_name));
			strncat(tmp_dev_name, "P",
					 sizeof(tmp_dev_name) - strlen(tmp_dev_name));

			/*
			 * The first member is reserved for "ttyAP*", begin to loop from 1
			 */
			for(i = 1; i < MAX_CARD_SUPPORT; i++)
			{
				/*
				 * Try to find filled member out first
				 */
				if(serialadv_reg[i].state != NULL
					&& strlen(serialadv_reg[i].dev_name) > 0
					&& strlen(tmp_dev_name) > 0)
				{
					if(strcmp(serialadv_reg[i].dev_name, tmp_dev_name) == 0)
					{
						up->port.line = index;
						up->fix_number_index = i;
						ret = uart_add_one_port(&serialadv_reg[i], &up->port);
						DEBUG_AUTOCONF("add port: %s, ret: 0x%x\n", serialadv_reg[i].dev_name, ret);
						break;
					}
					DEBUG_AUTOCONF("add port failed: %s\n", serialadv_reg[i].dev_name);
				}

				/*
				 * Try to find& init the first uninitial member at this step
				 */
				if(serialadv_reg[i].state == NULL)
				{
					/*
					 * Re-write index for port number
					 */
					serialadv_reg[i].nr = nr_ports;

					/*
					 * Set device_name, "ttyB" + boardID + "P" + port_num
					 */
					memset(adv_dev_name[i], 0, sizeof(adv_dev_name[i]));
					strncpy(adv_dev_name[i], tmp_dev_name, ARRAY_SIZE(tmp_dev_name));
					serialadv_reg[i].dev_name = adv_dev_name[i];

					memset(adv_driver_name[i], 0, sizeof(adv_driver_name[i]));
					sprintf(adv_driver_name[i], "ADVserialBID");
					strncat(adv_driver_name[i], tmp_string,
							 sizeof(adv_driver_name[i]) - strlen(adv_driver_name[i]));
					serialadv_reg[i].driver_name = adv_driver_name[i];

					ret = uart_register_driver(&serialadv_reg[i]);

					up->port.line = index;
					up->fix_number_index = i;
					ret = uart_add_one_port(&serialadv_reg[i], &up->port);
					break;
				}
			}
		}
#else
		uart_add_one_port(drv, &up->port);
#endif

	}
	else
	{
		printk(KERN_INFO "No match entry.\n");
	}
}

/**
 *	serialadv_suspend_port - suspend one serial port
 *	@line:  serial line number
 *      @level: the level of port suspension, as per uart_suspend_port
 *
 *	Suspend one serial port.
 */
void serialadv_suspend_port(int line)
{
	//uart_suspend_port(&serialadv_reg, &serialadv_ports[line].port);
}

/**
 *	serialadv_resume_port - resume one serial port
 *	@line:  serial line number
 *      @level: the level of port resumption, as per uart_resume_port
 *
 *	Resume one serial port.
 */
void serialadv_resume_port(int line)
{
	//uart_resume_port(&serialadv_reg, &serialadv_ports[line].port);
}

int serialadv_init(void)
{
	int ret;

	DEBUG_AUTOCONF(KERN_INFO "IRQ sharing %sabled\n", share_irqs ? "en" : "dis");

#ifdef FIXED_NUMBER_FUNC
	ret = 0;
#else
	ret = uart_register_driver(&serialadv_reg);
#endif

	return ret;
}

void serialadv_exit(void)
{
	int i;

#ifdef FIXED_NUMBER_FUNC
	int current_index;

	/*
	 * Unregister ports one by one
	 */
	for(i = 0; i < NR_PORTS; i++)
	{
		current_index = serialadv_ports[i].fix_number_index;

		if((serialadv_reg[current_index].state != NULL) && (serialadv_reg[current_index].nr > 0))
		{
			if (serialadv_ports[i].port.iobase != 0
				|| serialadv_ports[i].port.membase != 0)
			{
				uart_remove_one_port(&serialadv_reg[current_index], &serialadv_ports[i].port);
			}
		}
	}

	for(i = 0; i < MAX_CARD_SUPPORT; i++)
	{
		if(serialadv_reg[i].state != NULL)
		{
			uart_unregister_driver(&serialadv_reg[i]);
		}
	}
#else
	for (i = 0; i < NR_PORTS; i++)
	{
		if (serialadv_ports[i].port.iobase != 0
			|| serialadv_ports[i].port.membase != 0)
		{
			uart_remove_one_port(&serialadv_reg, &serialadv_ports[i].port);
		}
	}

	uart_unregister_driver(&serialadv_reg);
#endif

	return;
}

module_param(share_irqs, uint, 0644);
