/*****************************************************************************/
/*
 *      seradvptl.h  -- Advantech multiport serial driver.
 *
 *      Copyright (C) 2017 Advantech Corporation.
 *
 *      Based on Linux 2.6.9 Kernel's linux/drivers/char/8250.h
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
#include <linux/version.h>
#include "register.h"
#include "seradv_func.h"


#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
#include <linux/config.h>
#endif

#define VERSION_CODE(ver,rel,seq)	((ver << 16) | (rel << 8) | seq)

#define ADVANTECH_PTL_VER        "1.02.2.0"
#define ADVANTECH_PTL_DATE       "2019/10/16"
#define ADVANTECH_PTL_FILE_VER   "1.02.2.0"

/*
 * Debugging.
 */
#if 0
#define DEBUG_AUTOCONF(fmt...)	printk(fmt)
#else
#define DEBUG_AUTOCONF(fmt...)	do { } while (0)
#endif

#if 0
#define DEBUG_INTR(fmt...)	printk(fmt)
#else
#define DEBUG_INTR(fmt...)	do { } while (0)
#endif

/* Products definition */
#ifndef PCI_VENDOR_ID_ADVANTECH
#define PCI_VENDOR_ID_ADVANTECH				0x13FE
#endif
#define PCI_DEVICE_ID_PCI_1602UP			0x005D
#define PCI_DEVICE_ID_PCI_1604L				0x0060

/* DTR/DSR flow control support */
#ifndef CDTRDSR
#define CDTRDSR				004000000000		/* For DTR/DSR flow control */
#endif

#define NR_PORTS				256
#define	MAX_CARD_SUPPORT		16
#define PCI_NUM_BAR_RESOURCES	6

/* For fixed number */
#ifdef FIXED_NUMBER_FUNC
#define	MAX_STRING_LEN		64
#endif

#define CPR_VALUE( m, n )	((m << 3) | n)

/* ioctl commands */
#define     ADVANTECH_PTL_MAIGC             'x'
#define     ADV_READ_REG                    _IO(ADVANTECH_PTL_MAIGC, 1)
#define     ADV_WRITE_REG                   _IO(ADVANTECH_PTL_MAIGC, 2)
#define     ADV_GET_SERIAL_MODE             _IO(ADVANTECH_PTL_MAIGC, 3)


void serialadv_get_irq_map(unsigned int *map);
void serialadv_suspend_port(int line);
void serialadv_resume_port(int line);
int serialadv_init(void);
void serialadv_exit(void);
void serialadv_register_port(struct serial_struct *req, struct pci_dev *dev,
								int index, int nr_ports);
int serialadv_register_serial(struct serial_struct *req, int line);
unsigned int serial_in(struct uart_adv_port *up, int offset);
void serial_out(struct uart_adv_port *up, int offset, int value);


struct old_serial_port {
	unsigned int uart;
	unsigned int baud_base;
	unsigned int port;
	unsigned int irq;
	unsigned int flags;
	unsigned char hub6;
	unsigned char io_type;
	unsigned char *iomem_base;
	unsigned short iomem_reg_shift;
};

/*
 * This replaces serial_uart_config in include/linux/serial.h
 */
struct serialadv_config {
	const char	*name;
	unsigned int	fifo_size;
	unsigned int	tx_loadsz;
	unsigned int	flags;
};

struct serial_private {
	struct pci_dev		*dev;
	unsigned int		nr;
	void __iomem		*remapped_bar[PCI_NUM_BAR_RESOURCES];
	struct pci_serial_quirk	*quirk;
	int			uart_index[NR_PORTS];
	int			line[0];
};

#define UART_CAP_FIFO	(1 << 8)	/* UART has FIFO */
#define UART_CAP_EFR	(1 << 9)	/* UART has EFR */
#define UART_CAP_SLEEP	(1 << 10)	/* UART has IER sleep */

#undef SERIAL_DEBUG_PCI

#if defined(__i386__) && (defined(CONFIG_M386) || defined(CONFIG_M486))
#define SERIAL_INLINE
#endif

#ifdef SERIAL_INLINE
#define _INLINE_ inline
#else
#define _INLINE_
#endif

#define PROBE_RSA	(1 << 0)
#define PROBE_ANY	(~0)

#define HIGH_BITS_OFFSET ((sizeof(long)-sizeof(int))*8)

#ifdef CONFIG_SERIAL_8250_SHARE_IRQ
#define SERIAL8250_SHARE_IRQS 1
#else
#define SERIAL8250_SHARE_IRQS 0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)

#ifndef __LINUX_MUTEX_H
#define __LINUX_MUTEX_H

#include <asm/semaphore.h>

#define mutex semaphore
#define DEFINE_MUTEX(foo) DECLARE_MUTEX(foo)
#define mutex_init(foo) init_MUTEX(foo)
#define mutex_lock(foo) down(foo)
#define mutex_lock_interruptible(foo) down_interruptible(foo)
/* this function follows the spin_trylock() convention, so        *
 * it is negated to the down_trylock() return values! Be careful  */
#define mutex_trylock(foo) !down_trylock(foo)
#define mutex_unlock(foo) custom_up(foo)

/*
 * Note! This is subtle. We jump to wake people up only if
 * the semaphore was negative (== somebody was waiting on it).
 * The default case (no contention) will result in NO
 * jumps for both down() and up().
 */
static inline void custom_up(struct semaphore * sem)
{
	__asm__ __volatile__(
		"# atomic up operation\n\t"
		LOCK "incl %0\n\t"     /* ++sem->count */
		"jle 2f\n"
		"1:\n"
		LOCK_SECTION_START("")
		"2:\tcall __up_wakeup\n\t"
		"jmp 1b\n"
		LOCK_SECTION_END
		".subsection 0\n"
		:"=m" (sem->count)
		:"c" (sem)
		:"memory");
}

#endif /* __LINUX_MUTEX_H */
#define SERIAL_IO_AU      4
#define SERIAL_IO_TSI     5
#define SERIAL_IO_MEM32BE 6
#define SERIAL_IO_MEM16   7

#define UPIO_AU				(SERIAL_IO_AU)		/* Au1x00 and RT288x type IO */
#define UPIO_TSI			(SERIAL_IO_TSI)		/* Tsi108/109 type IO */
#define UPIO_MEM32BE		(SERIAL_IO_MEM32BE)	/* 32b big endian */
#define UPIO_MEM16			(SERIAL_IO_MEM16)	/* 16b little endian */

static inline int
uart_match_port(struct uart_port *port1, struct uart_port *port2)
{
    if (port1->iotype != port2->iotype)
        return 0;

    switch (port1->iotype) {
    case UPIO_PORT:
        return (port1->iobase == port2->iobase);
    case UPIO_HUB6:
        return (port1->iobase == port2->iobase) &&
               (port1->hub6   == port2->hub6);
    case UPIO_MEM:
    case UPIO_MEM16:
    case UPIO_MEM32:
    case UPIO_MEM32BE:
    case UPIO_AU:
    case UPIO_TSI:
        return (port1->mapbase == port2->mapbase);
    }
    return 0;
}

static inline void
uart_insert_char(struct uart_port *port, unsigned int status,
		 unsigned int overrun, unsigned int ch, unsigned int flag)
{
	struct tty_struct *tty = port->info->tty;

	if ((status & port->ignore_status_mask & ~overrun) == 0)
		tty_insert_flip_char(tty, ch, flag);

	/*
	 * Overrun is special.  Since it's reported immediately,
	 * it doesn't affect the current character.
	 */
	if (status & ~port->ignore_status_mask & overrun)
		tty_insert_flip_char(tty, 0, TTY_OVERRUN);
}
#endif
