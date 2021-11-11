/*****************************************************************************/
/*
 *      seradvptl_pci.c  -- Advantech multiport serial driver.
 *
 *      Copyright (C) 2017 Advantech Corporation.
 *
 *      Based on Linux 2.6.9 Kernel's drivers/serial/8250_pci.c
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
 *      Multiport Serial Driver bus for Pericom's PCI Family of UARTs (PI7C9X895X)
 *
 *   Check Release Notes for information on what has changed in the new version.
 *
 */
#include "seradvptl.h"


/*
 * Definitions for PCI support.
 */
#define FL_BASE_MASK		0x0007
#define FL_BASE0		0x0000
#define FL_BASE1		0x0001
#define FL_BASE2		0x0002
#define FL_BASE3		0x0003
#define FL_BASE4		0x0004
#define FL_GET_BASE(x)		(x & FL_BASE_MASK)

/* Use successive BARs (PCI base address registers),
   else use offset into some specified BAR */
#define FL_BASE_BARS		0x0008

/* do not assign an irq */
#define FL_NOIRQ		0x0080

/* Use the Base address register size to cap number of ports */
#define FL_REGION_SZ_CAP	0x0100

struct pci_board {
	unsigned int flags;
	unsigned int num_ports;
	unsigned int base_baud;
	unsigned int uart_offset;
	unsigned int reg_shift;
	unsigned int first_offset;
};

/*
 * init function returns:
 *  > 0 - number of ports
 *  = 0 - use board->num_ports
 *  < 0 - error
 */
struct pci_serial_quirk {
	u32	vendor;
	u32	device;
	u32	subvendor;
	u32	subdevice;
	int	(*init)(struct pci_dev *dev);
	int	(*setup)(struct pci_dev *dev, struct pci_board *board,
			 struct serial_struct *req, int idx);
	void	(*exit)(struct pci_dev *dev);
};

static void moan_device(const char *str, struct pci_dev *dev)
{
	printk(KERN_WARNING "%s: %s\n"
	       KERN_WARNING "Please send the output of lspci -vv, this\n"
	       KERN_WARNING "message (0x%04x,0x%04x,0x%04x,0x%04x), the\n"
	       KERN_WARNING "manufacturer and name of serial board or\n"
	       KERN_WARNING "modem board to rmk+serial@arm.linux.org.uk.\n",
	       pci_name(dev), str, dev->vendor, dev->device,
	       dev->subsystem_vendor, dev->subsystem_device);
}

static int
setup_port(struct pci_dev *dev, struct serial_struct *req,
	   int bar, int offset, int regshift)
{
	struct serial_private *priv = pci_get_drvdata(dev);
	unsigned long port, len;

	if (bar >= PCI_NUM_BAR_RESOURCES)
		return -EINVAL;

	if (pci_resource_flags(dev, bar) & IORESOURCE_MEM) {
		port = pci_resource_start(dev, bar);
		len =  pci_resource_len(dev, bar);

		if (!priv->remapped_bar[bar])
			priv->remapped_bar[bar] = ioremap(port, len);
		if (!priv->remapped_bar[bar])
			return -ENOMEM;

		req->io_type = UPIO_MEM;
		req->iomap_base = port + offset;
		req->iomem_base = priv->remapped_bar[bar] + offset;
		req->iomem_reg_shift = regshift;
	} else {
		port = pci_resource_start(dev, bar) + offset;
		req->io_type = UPIO_PORT;
		req->port = port;
		if (HIGH_BITS_OFFSET)
			req->port_high = port >> HIGH_BITS_OFFSET;
	}
	return 0;
}

static int
pci_seradvptl_setup(struct pci_dev *dev, struct pci_board *board,
		  struct serial_struct *req, int idx)
{
	unsigned int bar, offset = board->first_offset, maxnr;

	bar = FL_GET_BASE(board->flags);

	if ((board->num_ports == 4) && (idx == 3)) idx = 7;

	offset += idx * board->uart_offset;

	maxnr = (pci_resource_len(dev, bar) - board->first_offset) /
		(8 << board->reg_shift);

	return setup_port(dev, req, bar, offset, board->reg_shift);
}

/*
 * Master list of serial port init/setup/exit quirks.
 * This does not describe the general nature of the port.
 * (ie, baud base, number and location of ports, etc)
 *
 * This list is ordered alphabetically by vendor then device.
 * Specific entries must come before more generic entries.
 */
static struct pci_serial_quirk pci_serial_quirks[] = {
	/*
	 * Advantech
	 */
	{
		.vendor		= PCI_VENDOR_ID_ADVANTECH,
		.device		= PCI_DEVICE_ID_PCI_1602UP,
		.subvendor	= PCI_ANY_ID,
		.setup		= pci_seradvptl_setup,
		.subdevice	= PCI_ANY_ID,
	},

	{
		.vendor		= PCI_VENDOR_ID_ADVANTECH,
		.device		= PCI_DEVICE_ID_PCI_1604L,
		.subvendor	= PCI_ANY_ID,
		.setup		= pci_seradvptl_setup,
		.subdevice	= PCI_ANY_ID,
	}
};

static inline int quirk_id_matches(u32 quirk_id, u32 dev_id)
{
	return quirk_id == PCI_ANY_ID || quirk_id == dev_id;
}

static struct pci_serial_quirk *find_quirk(struct pci_dev *dev)
{
	struct pci_serial_quirk *quirk;

	for (quirk = pci_serial_quirks; ; quirk++)
		if (quirk_id_matches(quirk->vendor, dev->vendor) &&
		    quirk_id_matches(quirk->device, dev->device) &&
		    quirk_id_matches(quirk->subvendor, dev->subsystem_vendor) &&
		    quirk_id_matches(quirk->subdevice, dev->subsystem_device))
		 	break;
	return quirk;
}

static _INLINE_ int
get_pci_irq(struct pci_dev *dev, struct pci_board *board, int idx)
{
	if (board->flags & FL_NOIRQ)
		return 0;
	else
		return dev->irq;
}

/*
 * This is the configuration table for all of the PCI serial boards
 * which we support.  It is directly indexed by the pci_board_num_t enum
 * value, which is encoded in the pci_device_id PCI probe table's
 * driver_data member.
 *
 * The makeup of these names are:
 *  pbn_bn{_bt}_n_baud
 *
 *  bn   = PCI BAR number
 *  bt   = Index using PCI BARs
 *  n    = number of serial ports
 *  baud = baud rate
 *
 * Please note: in theory if n = 1, _bt infix should make no difference.
 * ie, pbn_b0_1_115200 is the same as pbn_b0_bt_1_115200
 */
enum pci_board_num_t {
	pbn_seradvptl_1port = 0,
	pbn_seradvptl_2port,
	pbn_seradvptl_4port,
	pbn_seradvptl_8port,
};

/*
 * uart_offset - the space between channels
 * reg_shift   - describes how the UART registers are mapped
 *               to PCI memory by the card.
 * For example IER register on SBS, Inc. PMC-OctPro is located at
 * offset 0x10 from the UART base, while UART_IER is defined as 1
 * in include/linux/serial_reg.h,
 * see first lines of serial_in() and serial_out() in 8250.c
*/

static struct pci_board pci_boards[] = {
	/*
	 * Advantech ICOM PI7C9X895[1248] Uno/Dual/Quad/Octal UART
	 */
	[pbn_seradvptl_1port] = {
		.flags		= FL_BASE1,
		.num_ports	= 1,
		.base_baud	= 921600,
		.uart_offset= 0x200,
	},
	[pbn_seradvptl_2port] = {
		.flags		= FL_BASE1,
		.num_ports	= 2,
		.base_baud	= 921600,
		.uart_offset= 0x200,
	},
	[pbn_seradvptl_4port] = {
		.flags		= FL_BASE1,
		.num_ports	= 4,
		.base_baud	= 921600,
		.uart_offset= 0x200,
	},
	[pbn_seradvptl_8port] = {
		.flags		= FL_BASE1,
		.num_ports	= 8,
		.base_baud	= 921600,
		.uart_offset= 0x200,
	},
};

/*
 * Given a complete unknown PCI device, try to use some heuristics to
 * guess what the configuration might be, based on the pitiful PCI
 * serial specs.  Returns 0 on success, 1 on failure.
 */
static int __devinit
serial_pci_guess_board(struct pci_dev *dev, struct pci_board *board)
{
	int num_iomem, num_port, first_port = -1, i;

	/*
	 * If it is not a communications device or the programming
	 * interface is greater than 6, give up.
	 *
	 * (Should we try to make guesses for multiport serial devices
	 * later?)
	 */
	if ((((dev->class >> 8) != PCI_CLASS_COMMUNICATION_SERIAL) &&
	     ((dev->class >> 8) != PCI_CLASS_COMMUNICATION_MODEM)) ||
	    (dev->class & 0xff) > 6)
		return -ENODEV;

	num_iomem = num_port = 0;
	for (i = 0; i < PCI_NUM_BAR_RESOURCES; i++) {
		if (pci_resource_flags(dev, i) & IORESOURCE_IO) {
			num_port++;
			if (first_port == -1)
				first_port = i;
		}
		if (pci_resource_flags(dev, i) & IORESOURCE_MEM)
			num_iomem++;
	}

	/*
	 * If there is 1 or 0 iomem regions, and exactly one port,
	 * use it.  We guess the number of ports based on the IO
	 * region size.
	 */
	if (num_iomem <= 1 && num_port == 1) {
		board->flags = first_port;
		board->num_ports = pci_resource_len(dev, first_port) / 8;
		return 0;
	}

	/*
	 * Now guess if we've got a board which indexes by BARs.
	 * Each IO BAR should be 8 bytes, and they should follow
	 * consecutively.
	 */
	first_port = -1;
	num_port = 0;
	for (i = 0; i < PCI_NUM_BAR_RESOURCES; i++) {
		if (pci_resource_flags(dev, i) & IORESOURCE_IO &&
		    pci_resource_len(dev, i) == 8 &&
		    (first_port == -1 || (first_port + num_port) == i)) {
			num_port++;
			if (first_port == -1)
				first_port = i;
		}
	}

	if (num_port > 1) {
		board->flags = first_port | FL_BASE_BARS;
		board->num_ports = num_port;
		return 0;
	}

	return -ENODEV;
}

static inline int
serial_pci_matches(struct pci_board *board, struct pci_board *guessed)
{
	return
	    board->num_ports == guessed->num_ports &&
	    board->base_baud == guessed->base_baud &&
	    board->uart_offset == guessed->uart_offset &&
	    board->reg_shift == guessed->reg_shift &&
	    board->first_offset == guessed->first_offset;
}

/*
 * Probe one serial board.  Unfortunately, there is no rhyme nor reason
 * to the arrangement of serial ports on a PCI card.
 */
static int __devinit
pciserialadv_init_one(struct pci_dev *dev, const struct pci_device_id *ent)
{
	struct serial_private *priv;
	struct pci_board *board, tmp;
	struct pci_serial_quirk *quirk;
	struct serial_struct serial_req;
	int rc, nr_ports, i;

	if (ent->driver_data >= ARRAY_SIZE(pci_boards)) {
		printk(KERN_ERR "pci_init_one: invalid driver_data: %ld\n",
			ent->driver_data);
		return -EINVAL;
	}

	board = &pci_boards[ent->driver_data];

	rc = pci_enable_device(dev);
	if (rc)
		return rc;

	/*
	 * We matched an explicit entry.  If we are able to
	 * detect this boards settings with our heuristic,
	 * then we no longer need this entry.
	 */
	memcpy(&tmp, &pci_boards[pbn_seradvptl_1port], sizeof(struct pci_board));
	rc = serial_pci_guess_board(dev, &tmp);
	if (rc == 0 && serial_pci_matches(board, &tmp))
		moan_device("Redundant entry in serial pci_table.",
				dev);

	nr_ports = board->num_ports;

	/*
	 * Find an init and setup quirks.
	 */
	quirk = find_quirk(dev);

	/*
	 * Run the new-style initialization function.
	 * The initialization function returns:
	 *  <0  - error
	 *   0  - use board->num_ports
	 *  >0  - number of ports
	 */
	if (quirk->init) {
		rc = quirk->init(dev);
		if (rc < 0)
			goto disable;
		if (rc)
			nr_ports = rc;
	}

	priv = kmalloc(sizeof(struct serial_private) +
		       sizeof(unsigned int) * nr_ports,
		       GFP_KERNEL);
	if (!priv) {
		rc = -ENOMEM;
		goto deinit;
	}

	memset(priv, 0, sizeof(struct serial_private) +
			sizeof(unsigned int) * nr_ports);

	priv->quirk = quirk;
	pci_set_drvdata(dev, priv);

	for (i = 0; i < nr_ports; i++) {
		memset(&serial_req, 0, sizeof(serial_req));
		serial_req.flags = UPF_SHARE_IRQ;
		serial_req.baud_base = board->base_baud;
		serial_req.irq = get_pci_irq(dev, board, i);
		if (quirk->setup(dev, board, &serial_req, i))
			break;
#ifdef SERIAL_DEBUG_PCI
		printk("Setup PCI port: port %x, irq %d, type %d\n",
		       serial_req.port, serial_req.irq, serial_req.io_type);
#endif
		serialadv_register_port(&serial_req, dev, i, nr_ports);
		priv->line[i] = i;
		priv->uart_index[i] = serial_req.line;
		if (priv->line[i] < 0) {
			printk(KERN_WARNING "Couldn't register serial port %s: %d\n", pci_name(dev), priv->line[i]);
			break;
		}
		printk(KERN_WARNING "init_one_advpciserialcard line:%d\n",priv->line[i]);
	}

	priv->nr = i;

	return 0;

 deinit:
	if (quirk->exit)
		quirk->exit(dev);
 disable:
	pci_disable_device(dev);
	return rc;
}

static void __devexit pciserialadv_remove_one(struct pci_dev *dev)
{
	struct serial_private *priv = pci_get_drvdata(dev);

	pci_set_drvdata(dev, NULL);

	if (priv) {
		struct pci_serial_quirk *quirk;
		int i;

		for (i = 0; i < PCI_NUM_BAR_RESOURCES; i++) {
			if (priv->remapped_bar[i])
				iounmap(priv->remapped_bar[i]);
			priv->remapped_bar[i] = NULL;
		}

		/*
		 * Find the exit quirks.
		 */
		quirk = find_quirk(dev);
		if (quirk->exit)
			quirk->exit(dev);

		pci_disable_device(dev);

		kfree(priv);
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
static int pciserialadv_suspend_one(struct pci_dev *dev, u32 state)
#else
static int pciserialadv_suspend_one(struct pci_dev *dev, pm_message_t state)
#endif
{
	struct serial_private *priv = pci_get_drvdata(dev);

	if (priv) {
		int i;

		for (i = 0; i < priv->nr; i++)
			serialadv_suspend_port(priv->line[i]);
	}
	return 0;
}

static int pciserialadv_resume_one(struct pci_dev *dev)
{
	struct serial_private *priv = pci_get_drvdata(dev);

	if (priv) {
		int i;

		/*
		 * Ensure that the board is correctly configured.
		 */
		if (priv->quirk->init)
			priv->quirk->init(dev);

		for (i = 0; i < priv->nr; i++)
			serialadv_resume_port(priv->line[i]);
	}
	return 0;
}

static struct pci_device_id serialadv_pci_tbl[] = {
	/*
	 * Advantech PI7C9X895[1248] Uno/Dual/Quad/Octal UART
	 */
	{	PCI_VENDOR_ID_ADVANTECH, PCI_DEVICE_ID_PCI_1602UP,	PCI_ANY_ID, PCI_ANY_ID,	0, 0, pbn_seradvptl_2port },
	{	PCI_VENDOR_ID_ADVANTECH, PCI_DEVICE_ID_PCI_1604L,	PCI_ANY_ID, PCI_ANY_ID,	0, 0, pbn_seradvptl_2port },

	{ 0, }
};

static struct pci_driver serialadv_pci_driver = {
	.name		= "ADVPTLserial",
	.probe		= pciserialadv_init_one,
	.remove		= __devexit_p(pciserialadv_remove_one),
	.suspend	= pciserialadv_suspend_one,
	.resume		= pciserialadv_resume_one,
	.id_table	= serialadv_pci_tbl,
};

struct pci_dev	*pci_dev_tbl[MAX_CARD_SUPPORT];

static int __init serialadv_pci_init(void)
{
	int i, b, n;
#ifdef CONFIG_PCI
	struct pci_dev	*pdev=NULL;
#endif

	printk("\n");
	printk("==========================================================="
			"====\n");
	printk("Advantech PCI (PI7C9X895X) Serial Device Drivers.\n");
	printk("Product V%s [%s]\nFile V%s\n",
		ADVANTECH_PTL_VER, ADVANTECH_PTL_DATE, ADVANTECH_PTL_FILE_VER);
	printk("Supports: RS232/422/485 auto detection and setting\n");
	printk("Devices:  ICOM: PCI-1602UP, PCI-1604L\n"
			);
	printk("Advantech Industrial Automation Group.\n");
	printk("==========================================================="
			"====\n");

	/* Avoid a compiling warning, remove this later */
	serialadv_pci_driver.id_table = serialadv_pci_tbl;

	serialadv_init();
	/* start finding PCI board here */
	n = (sizeof(serialadv_pci_tbl) / sizeof(serialadv_pci_tbl[0])) - 1;
	b = 0;

	while (b < n) {
#if (LINUX_VERSION_CODE < VERSION_CODE(2,6,21))
		pdev = pci_find_device(serialadv_pci_tbl[b].vendor, serialadv_pci_tbl[b].device, pdev);
#else
		pdev = pci_get_device(serialadv_pci_tbl[b].vendor, serialadv_pci_tbl[b].device, pdev);
#endif
		if(pdev==NULL){
			b++;
			continue;
		}

		if(pci_enable_device(pdev)) {
			continue;
		}

		pciserialadv_init_one(pdev, &serialadv_pci_tbl[b]);

		for(i = 0; i < MAX_CARD_SUPPORT; i++) {
			if(pci_dev_tbl[i] == NULL) {
				pci_dev_tbl[i] = pdev;
				break;
			}
		}
	}

	return 0;
}

static void serialadv_pci_exit(void)
{
	int i, j;
	struct serial_private *priv;
	struct pci_serial_quirk *quirk;

	serialadv_exit();

	for(j = 0; j < MAX_CARD_SUPPORT; j++)
	{
		if(pci_dev_tbl[j] == NULL)
		{
			continue;
		}

		priv = pci_get_drvdata(pci_dev_tbl[j]);
		for (i = 0; i < PCI_NUM_BAR_RESOURCES; i++) {
			if (priv->remapped_bar[i])
				iounmap(priv->remapped_bar[i]);
			priv->remapped_bar[i] = NULL;
		}

		/*
		 * Find the exit quirks.
		 */
		if(priv->dev == NULL)
		{
			continue;
		}

		quirk = find_quirk(priv->dev);
		if (quirk->exit)
			quirk->exit(priv->dev);

		kfree(priv);
	}

	printk("Uninstall Advantech PCI (PI7C9X895X) Serial Device Drivers successfully.\n");

	return;
}

module_init(serialadv_pci_init);
module_exit(serialadv_pci_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Advantech PCI specific serial driver for PI7C9X895X");
MODULE_VERSION(ADVANTECH_PTL_VER);

MODULE_DEVICE_TABLE(pci, serialadv_pci_tbl);
