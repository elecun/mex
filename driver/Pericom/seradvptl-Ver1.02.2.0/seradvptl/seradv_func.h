/*****************************************************************************/
/*
 *      seradv_func.h  -- Advantech multiport serial driver functions header.
 *
 *      Copyright (C) 2017 Advantech Corporation.
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
 *      Multiport Serial Driver custom functions for Pericom's PCI Family
 *      of UARTs (PI7C9X895X)
 *
 *   Check Release Notes for information on what has changed in the new version.
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_reg.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/byteorder.h>

#include <linux/serial_core.h>
#include "linux/version.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
#include <linux/config.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#include <linux/serialP.h>
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0)
#define __devinitdata
#define __devinit
#define __devexit
#define __devexit_p
#endif

#ifndef ALPHA_KLUDGE_MCR
#define ALPHA_KLUDGE_MCR 0
#endif

/*
 * Fixed number Switch
 */
#define FIXED_NUMBER_FUNC

/*
 * serial mode, RS232, RS422 Master, RS485/RS422 Slave
 */
#define SERIAL_MODE_UNKNOWN         0
#define SERIAL_MODE_RS232           1
#define SERIAL_MODE_RS422M          2
#define SERIAL_MODE_RS485ORRS422S   3


/*
 *  SPI bus, to get run mode(RS232/422/485)
 */
#define GPIO_OUTPUT_LVL_REG		0x50
#define GPIO_INPUT_SEL_REG		0x52
#define GPIO_SELECT_REG			0x53

#define EEPROM_CTL_REG			0xDC
#define GPIO_CS_BIT				0x01
#define GPIO_CK_BIT				0x02
#define GPIO_SI_BIT				0x04
#define GPIO_SO_BIT				0x08

#define IO_EXPAND_GPIO_A		0x12
#define IO_EXPAND_GPIO_B		0x13
#define SPI_READ_CMD			0x41


struct uart_adv_port {
	struct uart_port	port;
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
	/*typedef*/ struct legacy_timer_emu {
	struct timer_list t;
	void (*function)(unsigned long);
	unsigned long data;
}timer;
#else
	struct timer_list	timer;		/* "no irq" timer */
#endif
	struct list_head	list;		/* ports on this IRQ */
	unsigned int		capabilities;	/* port capabilities */
	unsigned int		tx_loadsz;	/* transmit fifo load size */
	unsigned short		rev;
	unsigned char		acr;
	unsigned char		ier;
	unsigned char		lcr;
	unsigned char		mcr;
	unsigned char		mcr_mask;	/* mask of user bits */
	unsigned char		mcr_force;	/* mask of forced bits */
	unsigned char		lsr_break_flag;

	/*
	 * We provide a per-port pm hook.
	 */
	void			(*pm)(struct uart_port *port,
				      unsigned int state, unsigned int old);

	unsigned char		msr_saved_flags;

	int				index;
	unsigned char	serial_mode;		/* RS232, RS422 or RS485 */
	unsigned char	board_id;			/* Board ID: 0x0 - 0xf */
	unsigned short	device_id;			/* Device ID */

#ifdef FIXED_NUMBER_FUNC
	int				current_card_nr;
	int				fix_number_index;
#endif
};

int serialadv_get_run_mode(struct uart_adv_port *up);
int serialadv_set_auto485(struct uart_adv_port *up, int turn_on, int rs485_active_low);
int serialadv_get_board_id(struct uart_adv_port *up);
