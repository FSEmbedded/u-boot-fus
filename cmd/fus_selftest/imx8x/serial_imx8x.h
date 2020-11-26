#ifndef __SERIAL_FUS_IMX8X_H__
#define __SERIAL_FUS_IMX8X_H__

// iMX8X UART Init
#include <asm/mach-imx/sci/sci.h> // SCU Functions
#include <asm/arch/lpcg.h> // Clocks
#include <asm/arch/iomux.h> // IOMUX
#include <asm/arch/imx8-pins.h> // Pins
#include <asm/arch/imx-regs.h> // LPUART_BASE_ADDR
#include <common.h> // DECLARE_GLOBAL_DATA_PTR
#include <stdio_dev.h> // struct stdio_dev

DECLARE_GLOBAL_DATA_PTR; // gd for SCU Handle

#define DEBUG_PORT 2

// Must be in right order !!!
static char * serial_ports[] = {
		"LPUART0",
		"LPUART1",
		"LPUART2",
		"LPUART3",
		NULL
};

struct uart_tx_iomux {
	sc_pad_t pad;
	uint8_t mux;
};

static struct uart_tx_iomux uart_rx_pads[] = {
		{SC_P_UART0_RX,		0},
		{SC_P_UART1_RX,		0},
		{SC_P_UART2_RX,		0},
		{SC_P_SCU_GPIO0_00,	3},
};

static struct uart_tx_iomux uart_tx_pads[] = {
		{SC_P_UART0_TX,		0},
		{SC_P_UART1_TX,		0},
		{SC_P_UART2_TX,		0},
		{SC_P_SCU_GPIO0_01,	3},
};

static struct uart_tx_iomux uart_tx_gpio_pads[] = {
		{SC_P_UART0_TX,		4},
		{SC_P_UART1_TX,		4},
		{SC_P_UART2_TX,		4},
		{SC_P_SCU_GPIO0_01,	0},
};

//---Address-Operations---//

#define U32(X)      ((uint32_t) (X))
#define READ4(A) *((volatile uint32_t*)(A))
#define DATA4(A, V) *((volatile uint32_t*)(A)) = U32(V)
#define SET_BIT4(A, V) *((volatile uint32_t*)(A)) |= U32(V)
#define CLR_BIT4(A, V) *((volatile uint32_t*)(A)) &= ~(U32(V))

//---LPUART Control-Register---//

#define LPUART_OFFSET		0x10000
#define LPUART_STAT_OFFSET	0x14
#define LPUART_CTRL_OFFSET	0x18

#define LPUART(id)			LPUART_BASE + (id*LPUART_OFFSET)

#define LPUART_STAT(id)		LPUART(id) + LPUART_STAT_OFFSET
#define LPUART_STAT_TDRE	(1 << 23)

#define LPUART_CTRL(id)		LPUART(id) + LPUART_CTRL_OFFSET
#define LPUART_CTRL_TE		(1 << 19)
#define LPUART_CTRL_RE		(1 << 18)
#define LPUART_CTRL_LOOPS	(1 <<  7)

//---Functions for selftest---//



#endif // __SERIAL_FUS_IMX8X_H__
