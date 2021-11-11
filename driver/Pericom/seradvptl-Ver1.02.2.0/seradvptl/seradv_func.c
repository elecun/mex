/*****************************************************************************/
/*
 *      seradv_func.c  -- Advantech multiport serial driver functions.
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
#include "seradvptl.h"


void serialadv_spi_delay(void)
{
	int idex = 10;

	while(idex-- > 0)
	{
		;
	}

}

int serialadv_spi_start(struct uart_adv_port *up)
{
	serial_out(up, GPIO_SELECT_REG, 0xff);		// set GPIO pins direction to output
	serial_out(up, GPIO_OUTPUT_LVL_REG, 0x0);	// set CS to low

	serialadv_spi_delay();

	return 0;
}

int serialadv_spi_stop(struct uart_adv_port *up)
{
	serial_out(up, GPIO_SELECT_REG, 0x00);		// set GPIO pins direction to input
	serial_out(up, GPIO_OUTPUT_LVL_REG, 0xff);	// set CS to high
	serialadv_spi_delay();

	return 0;
}

int serialadv_spi_write_to_slave(struct uart_adv_port *up, unsigned char data)
{
	int idex     = 0;
	int send_bit = 0;
	unsigned char reg_value = 0;

	for(idex = 7; idex >= 0; idex--)
	{
		send_bit = data&(1<<idex)?1:0;
		serial_out(up, GPIO_OUTPUT_LVL_REG, 0);					// CK low
		serialadv_spi_delay();
		reg_value = send_bit? GPIO_SI_BIT:0;
		serial_out(up, GPIO_OUTPUT_LVL_REG, reg_value|GPIO_CK_BIT);	// write EEDI bit, and CK high
		serialadv_spi_delay();
		serial_out(up, GPIO_OUTPUT_LVL_REG, 0);					// CK low
		serialadv_spi_delay();
	}

	return 0;
}

int serialadv_spi_read_from_slave(struct uart_adv_port *up, unsigned char *data)
{
	int idex     = 0;
	int recv_bit = 0;
	unsigned char reg_value = 0;

	if (!data)
	{
		return -EPERM;
	}

	*data = 0;
	for (idex = 7; idex >= 0; idex--)
	{
		serial_out(up, GPIO_OUTPUT_LVL_REG, 0);				// CK low
		serialadv_spi_delay();
		serial_out(up, GPIO_OUTPUT_LVL_REG, GPIO_CK_BIT);	// CK high
		serialadv_spi_delay();

		reg_value = serial_in(up, GPIO_OUTPUT_LVL_REG);		// read EEDO bit

		recv_bit = (reg_value&GPIO_SO_BIT)?0x1:0x0;
		if(recv_bit)
		{
			*data |= recv_bit<<idex;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int serialadv_get_run_mode(struct uart_adv_port *up)
{
	unsigned char	serial_mode = SERIAL_MODE_UNKNOWN;

	if(up->device_id == PCI_DEVICE_ID_PCI_1604L)
	{
		serial_mode = SERIAL_MODE_RS232;
	}
	else
	{
		int idex;
		unsigned char	run_mode = 0, tmp_run_mode = 0;
		unsigned long	index = 0;

		for(idex = 0; idex < 10; idex++)
		{
			serialadv_spi_start(up);
			serialadv_spi_write_to_slave(up, SPI_READ_CMD);
			serialadv_spi_write_to_slave(up, IO_EXPAND_GPIO_A);
			serialadv_spi_read_from_slave(up, &tmp_run_mode);
			serialadv_spi_stop(up);

			if((tmp_run_mode == run_mode) && (tmp_run_mode != 0))
			{
				break;
			}
			run_mode = tmp_run_mode;
		}

		index = up->index;

		if(run_mode & (1<<(index*2)))
		{
			serial_mode = SERIAL_MODE_RS232;
		}
		else if((!(run_mode & (1<<(index*2)))) && (run_mode & (1<<(index*2+1))))
		{
			serial_mode = SERIAL_MODE_RS485ORRS422S;

		}
		else if((!(run_mode & (1<<(index*2)))) && !(run_mode & (1<<(index*2+1))))
		{
			serial_mode = SERIAL_MODE_RS422M;
		}
		else
		{
			serial_mode = SERIAL_MODE_UNKNOWN;
		}
	}

	if(serial_mode == SERIAL_MODE_RS232)
	{
		printk("Serial mode: RS232\n");
	}
	else if(serial_mode == SERIAL_MODE_RS485ORRS422S)
	{
		printk("Serial mode: RS485 or RS422 Slave\n");
	}
	else if(serial_mode == SERIAL_MODE_RS422M)
	{
		printk("Serial mode: RS422 Master\n");
	}
	else
	{
		printk("Serial mode: Unknown\n");
	}

	return serial_mode;
}

int serialadv_set_auto485(struct uart_adv_port *up, int turn_on, int rs485_active_low)
{
	unsigned char sfr, acr;

	/*
	 * Read current SFR/ACR register values before modifying.
	 */
	sfr = serial_in(up, PTLSER_SFR_OFFSET);		/* Read current SFR register value */
	acr = serial_in(up, PTLSER_ACR_OFFSET);		/* Read current ACR register value */

	if(turn_on)
	{
		sfr |= 0x4;					/* Enable auto485, set bit[2] to 1 in SFR */
		DEBUG_AUTOCONF("Auto485 enabled\n");
	}
	else
	{
		sfr &= 0xfb;				/* Disable auto485, set bit[2] to 0 in SFR */
		DEBUG_AUTOCONF("Auto485 disabled\n");
	}

	if(rs485_active_low)
	{
		/*
		 * Set direction control.
		 * LOW when Transmiting, HIGH when Recieving.
		 * Set bit[5] to 0 in ACR.
		 */
		acr &= 0xdf;
		DEBUG_AUTOCONF("Auto485 set LOW when Transmiting\n");
	}
	else
	{
		/*
		 * Set direction control.
		 * HIGH when Transmiting, LOW when Recieving.
		 * Set bit[5] to 1 in ACR.
		 */
		acr |= 0x20;
		DEBUG_AUTOCONF("Advptl Auto485 set HIGH when Transmiting\n");
	}

	serial_out(up, PTLSER_SFR_OFFSET, sfr);		/* Write SFR register value */
	serial_out(up, PTLSER_ACR_OFFSET, acr);		/* Write ACR register value */

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int serialadv_get_board_id(struct uart_adv_port *up)
{
	int board_id;
	unsigned char reg_value;

	if(up->device_id == PCI_DEVICE_ID_PCI_1604L)
	{
		board_id = 0x0;
	}
	else
	{
		serial_out(up, GPIO_SELECT_REG, 0);					// set GPIO pins direction to input
		reg_value = serial_in(up, GPIO_OUTPUT_LVL_REG);		// read GPIO;
		DEBUG_AUTOCONF("Advptl reg_value: 0x%2x\n", reg_value);

		board_id = (reg_value >> 4) & 0x0f;
	}
	printk("Board ID: 0x%X\n", board_id);

	return board_id;
}
