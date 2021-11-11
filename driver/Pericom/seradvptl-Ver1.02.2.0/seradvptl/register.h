// Register definition for ptlser driver
// Copyright (C) 2017 Advantech Corporation
// All rights reserved




#ifndef _REGISTER_H_
#define _REGISTER_H_



#define BIT_0		0x01
#define BIT_1		0x02
#define BIT_2		0x04
#define BIT_3		0x08
#define BIT_4		0x10
#define BIT_5		0x20
#define BIT_6		0x40
#define BIT_7		0x80



///////////////////////////////////////////////////////////////////////////////////////////
//
//  register offset

#define PTLSER_RHR_OFFSET				0x00
#define PTLSER_THR_OFFSET				0x00
#define PTLSER_IER_OFFSET				0x01
#define PTLSER_ISR_OFFSET				0x02
#define PTLSER_FCR_OFFSET				0x02
#define PTLSER_LCR_OFFSET				0x03
#define PTLSER_MCR_OFFSET				0x04
#define PTLSER_LSR_OFFSET				0x05
#define PTLSER_MSR_OFFSET				0x06
#define PTLSER_SFR_OFFSET				0x07
#define PTLSER_DLL_OFFSET				0x08
#define PTLSER_DLH_OFFSET				0x09
#define PTLSER_EFR_OFFSET				0x0A
#define PTLSER_XON1_OFFSET				0x0B
#define PTLSER_XON2_OFFSET				0x0C
#define PTLSER_XOFF1_OFFSET				0x0D
#define PTLSER_XOFF2_OFFSET				0x0E
#define PTLSER_ASR_OFFSET				0x0F
#define PTLSER_TTL_OFFSET				0x10
#define PTLSER_RTL_OFFSET				0x11
#define PTLSER_FCL_OFFSET				0x12
#define PTLSER_FCH_OFFSET				0x13
#define PTLSER_CPR_OFFSET				0x14
#define PTLSER_LRF_OFFSET				0x15
#define PTLSER_LTF_OFFSET				0x16
#define PTLSER_SCR_OFFSET				0x16
#define PTLSER_GLSR_OFFSET				0x17
#define PTLSER_GIE_OFFSET				0x18
#define PTLSER_GIS_OFFSET				0x19
#define PTLSER_ISR2_OFFSET				0x1C
#define PTLSER_RXC_OFFSET				0x1D
#define PTLSER_TXC_OFFSET				0x1E

#define PTLSER_TOV_OFFSET				0x20            // word size
#define PTLSER_TOE_OFFSET				0x23
#define PTLSER_WDC_OFFSET				0x24
#define PTLSER_WDE_OFFSET				0x25


#define PTLSER_SC1_OFFSET				0x28
#define PTLSER_SC2_OFFSET				0x29
#define PTLSER_SC3_OFFSET				0x2A
#define PTLSER_SC4_OFFSET				0x2B

#define PTLSER_SCC_OFFSET				0x2C
#define PTLSER_ACR_OFFSET				0x2F

#define PTLSER_SMC_OFFSET				0x4B

#define PTLSER_LAST_OFFSET              (PTLSER_SCC_OFFSET+1)

#define PTLSER_FIFO_D_OFFSET			0x100
#define PTLSER_FIFO_LSR_OFFSET			0x180


#define PTLSER_FIFO_SIZE				0x80
#define PTLSER_MEMORY_RANGE             0x200

///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_IER_OFFSET

#define PTLSER_IER_RX_DATA				BIT_0
#define PTLSER_IER_TX_EMPTY				BIT_1
#define PTLSER_IER_RX_STATUS			BIT_2
#define PTLSER_IER_MODEM_STATUS			BIT_3
#define PTLSER_IER_XON_XOFF				BIT_4
#define PTLSER_IER_RTS_DTR				BIT_5
#define PTLSER_IER_CTS_DSR				BIT_6


///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_ISR_OFFSET

#define PTLSER_ISR_NO_INT_PENDING		BIT_0
#define PTLSER_ISR_INT_STATUS_MASK		(BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | PTLSER_ISR_NO_INT_PENDING)


#define PTLSER_ISR_RX_DATA_ERROR		(BIT_1 | BIT_2)
#define PTLSER_ISR_RX_DATA_ERROR_BIT	(BIT_1)
#define PTLSER_ISR_RX_DATA				(BIT_2)
#define PTLSER_ISR_RX_TIMEOUT			(BIT_2 | BIT_3)
#define PTLSER_ISR_RX_TIMEOUT_BIT		(BIT_2 | BIT_3)
#define PTLSER_ISR_TX_EMPTY				(BIT_1)
#define PTLSER_ISR_MODEM_STATUS_CHANGE	0
#define PTLSER_ISR_SPECIAL_CHARACTOR	(BIT_4)
#define PTLSER_ISR_CTS_RTS_CHANGE		(BIT_5)

///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_FCR_OFFSET

#define PTLSER_FCR_FIFO_ENABLE			BIT_0
#define PTLSER_FCR_RX_FLUSH				BIT_1
#define PTLSER_FCR_TX_FLUSH				BIT_2
#define PTLSER_FCR_DMA_ENABLE			BIT_3
#define PTLSER_FCR_TX_TRIGGER_MASK		(BIT_4 | BIT_5)
#define PTLSER_FCR_TX_TRIGGER			BIT_4
#define PTLSER_FCR_RX_TRIGGER_MAKS		(BIT_6 | BIT_7)
#define PTLSER_FCR_RX_TRIGGER			BIT_6


///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_LCR_OFFSET

#define PTLSER_LCR_WORD_WIDTH_MASK		(BIT_0 | BIT_1)
#define PTLSER_LCR_WORD_WIDTH_5			0
#define PTLSER_LCR_WORD_WIDTH_6			BIT_0
#define PTLSER_LCR_WORD_WIDTH_7			BIT_1
#define PTLSER_LCR_WORD_WIDTH_8			(BIT_0 | BIT_1)

#define PTLSER_LCR_STOP_BIT				BIT_2

#define PTLSER_LCR_PARITY_MASK			(BIT_3 | BIT_4 | BIT_5)
#define PTLSER_LCR_PARITY_NONE			0
#define PTLSER_LCR_PARITY_ODD			BIT_3
#define PTLSER_LCR_PARITY_EVEN			(BIT_3 | BIT_4)
#define PTLSER_LCR_PARITY_MARK			(BIT_3 |         BIT_5)
#define PTLSER_LCR_PARITY_SPACE			(BIT_3 | BIT_4 | BIT_5)


#define PTLSER_LCR_BREAK				BIT_6
#define PTLSER_LCR_DIVISOR_LATCH		BIT_7


///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_MCR_OFFSET

#define PTLSER_MCR_DTR					BIT_0
#define PTLSER_MCR_RTS					BIT_1
#define PTLSER_MCR_OUTPUT1				BIT_2
#define PTLSER_MCR_OUTPUT2				BIT_3
#define PTLSER_MCR_INTERNAL_LOOPBACK	BIT_4
#define PTLSER_MCR_XON_ANY				BIT_5
//#define PTLSER_MCR_BRG_PRESCALE			BIT_7


///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_LSR_OFFSET

#define PTLSER_LSR_RX_DATA				BIT_0
#define PTLSER_LSR_RX_OVERRUN			BIT_1
#define PTLSER_LSR_RX_PARITY_ERROR		BIT_2
#define PTLSER_LSR_RX_FRAME_ERROR		BIT_3
#define PTLSER_LSR_RX_BREAK				BIT_4
#define PTLSER_LSR_TX_EMPTY				BIT_5
#define PTLSER_LSR_TX_COMPLETE			BIT_6
#define PTLSER_LSR_RX_DATA_ERROR		BIT_7

///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_MSR_OFFSET

#define PTLSER_MSR_DELTA_CTS			BIT_0
#define PTLSER_MSR_DELTA_DSR			BIT_1
#define PTLSER_MSR_DELTA_RI				BIT_2
#define PTLSER_MSR_DELTA_DCD			BIT_3
#define PTLSER_MSR_CTS					BIT_4
#define PTLSER_MSR_DSR					BIT_5
#define PTLSER_MSR_RI					BIT_6
#define PTLSER_MSR_DCD					BIT_7


///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_SFR_OFFSET

#define PTLSER_SFR_FORCE_TX				BIT_0
#define PTLSER_SFR_AUTO_DSR_DTR			BIT_1
#define PTLSER_SFR_IXON_ANY				BIT_4
#define PTLSER_SFR_950					BIT_5
#define PTLSER_SFR_PRESCALE_MASK		(BIT_6 | BIT_7)
#define PTLSER_SFR_PRESCALE_16			0
//#define PTLSER_SFR_PRESCALE_8			BIT_6
//#define PTLSER_SFR_PRESCALE_4			BIT_7
#define PTLSER_SFR_LSR_FIFO_COUNT		BIT_6
#define PTLSER_SFR_TX_FIFO_COUNT		BIT_7


///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_EFR_OFFSET

#define PTLSER_EFR_INBAND_RX_MODE_MASK		(BIT_0 | BIT_1)
#define PTLSER_EFR_INBAND_RX_MODE_DISABLE	0
#define PTLSER_EFR_INBAND_RX_MODE_2			BIT_0
#define PTLSER_EFR_INBAND_RX_MODE_1			BIT_1
#define PTLSER_EFR_INBAND_RX_MODE_DEPEND	(BIT_0 | BIT_1)

#define PTLSER_EFR_INBAND_TX_MODE_MASK		(BIT_2 | BIT_3)
#define PTLSER_EFR_INBAND_TX_MODE_DISABLE	0
#define PTLSER_EFR_INBAND_TX_MODE_2			BIT_2
#define PTLSER_EFR_INBAND_TX_MODE_1			BIT_3

#define PTLSER_EFR_ENHANCE_MODE				BIT_4
#define PTLSER_EFR_X_DETECT_ENABLE			BIT_5
#define PTLSER_EFR_AUTO_RTS					BIT_6
#define PTLSER_EFR_AUTO_CTS					BIT_7



///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_ASR_OFFSET

#define PTLSER_ASR_TX_DISABLE				BIT_0
#define PTLSER_ASR_REMOTE_TX_DISABLE		BIT_1
#define PTLSER_ASR_X_DECTECT				BIT_2		//Status bit
#define PTLSER_ASR_LSR_FIFO_RESET			BIT_3



///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_CPR_OFFSET

#define PTLSER_CPR_N_MASK				(BIT_0 | BIT_1 | BIT_2)
#define PTLSER_CPR_N_OFFSET				0
#define PTLSER_CPR_N_( x )				( x << PTLSER_CPR_N_OFFSET)

#define PTLSER_CPR_M_MASK				(BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7)
#define PTLSER_CPR_M_OFFSET				3
#define PTLSER_CPR_M_( x )				( x << PTLSER_CPR_M_OFFSET)


///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_SCR_OFFSET

#define PTLSER_SCR_16			0
#define PTLSER_SCR_15			0x01
#define PTLSER_SCR_14			0x02
#define PTLSER_SCR_13			0x03
#define PTLSER_SCR_12			0x04
#define PTLSER_SCR_11			0x05
#define PTLSER_SCR_10			0x06
#define PTLSER_SCR_9			0x07
#define PTLSER_SCR_8			0x08
#define PTLSER_SCR_7			0x09
#define PTLSER_SCR_6			0x0A
#define PTLSER_SCR_5			0x0B
#define PTLSER_SCR_4			0x0C
#define PTLSER_SCR_MASK			0x0F








///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_ISR2_OFFSET

#define PTLSER_ISR2_NO_INT_PENDING		BIT_0
#define PTLSER_ISR2_INT_STATUS_MASK		(BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_6 | BIT_7)



#define PTLSER_ISR2_RX_DATA_ERROR		(BIT_1)
#define PTLSER_ISR2_RX_DATA				(BIT_2)
#define PTLSER_ISR2_RX_TIMEOUT			(BIT_3)
#define PTLSER_ISR2_TX_EMPTY			(BIT_4)
#define PTLSER_ISR2_MODEM_STATUS_CHANGE	(BIT_5)
#define PTLSER_ISR2_SPECIAL_CHARACTOR	(BIT_6)
#define PTLSER_ISR2_CTS_RTS_CHANGE		(BIT_7)


#define PTLSER_ISR2_RX_MASK				(BIT_1 | BIT_2 | BIT_3)


///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_WDE_OFFSET

#define PTLSER_WDE_ENABLE				(BIT_1)



///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_SCC_OFFSET


#define PTLSER_SCC_SC1_ENABLE			(BIT_0)
#define PTLSER_SCC_SC2_ENABLE			(BIT_1)
#define PTLSER_SCC_SC3_ENABLE			(BIT_2)
#define PTLSER_SCC_SC4_ENABLE			(BIT_3)

#define PTLSER_SCC_SC1_EATEN			(BIT_4)
#define PTLSER_SCC_SC2_EATEN			(BIT_5)
#define PTLSER_SCC_SC3_EATEN			(BIT_6)
#define PTLSER_SCC_SC4_EATEN			(BIT_7)

///////////////////////////////////////////////////////////////////////////////////////////
//
//	PTLSER_SMC_OFFSET


#define PTLSER_SMC_DISABLE				0x00
#define PTLSER_SMC_ENABLE				0x01

#endif
