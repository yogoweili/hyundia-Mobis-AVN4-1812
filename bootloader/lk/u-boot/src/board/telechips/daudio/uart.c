#include <common.h>
#include <serial.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/reg_physical.h>
#include <asm/arch/uart.h>

struct uart_port_map_type uart_port_map[] = {
	/*  txd          rxd           cts         rts        fn_sel */
#if defined(CONFIG_CHIP_TCC8930)
	{0 , TCC_GPA(26), TCC_GPA(27), TCC_GPA(25), TCC_GPA(24), GPIO_FN(4) },	// UT_TXD(0)
	{1 , TCC_GPA(28), TCC_GPA(29), TCC_GPA(20), TCC_GPA(21), GPIO_FN(4) },	// UT_TXD(1)
//	{1 , TCC_GPA(13), TCC_GPA(14), NC_PORT    , NC_PORT    , GPIO_FN(7) },	// UT_TXD(1)
	{2 , TCC_GPA(24), TCC_GPA(25), TCC_GPA(23), TCC_GPA(22), GPIO_FN(5) },	// UT_TXD(2)
//	{2 , TCC_GPD(4) , TCC_GPD(5) , TCC_GPD(7) , TCC_GPD(6) , GPIO_FN(7) },	// UT_TXD(2)
	{3 , TCC_GPD(11), TCC_GPD(12), TCC_GPD(14), TCC_GPD(13), GPIO_FN(7) },	// UT_TXD(3)
//	{3 , TCC_GPD(22), TCC_GPD(23), TCC_GPD(25), TCC_GPD(24), GPIO_FN(14)},	// UT_TXD(3)
//	{3 , TCC_GPD(13), TCC_GPD(14), NC_PORT    , NC_PORT    , GPIO_FN(4) },	// UT_TXD(3)
	{4 , TCC_GPD(17), TCC_GPD(18), TCC_GPD(20), TCC_GPD(19), GPIO_FN(7) },	// UT_TXD(4)
	{5 , TCC_GPB(7) , TCC_GPB(8) , TCC_GPB(10), TCC_GPB(9) , GPIO_FN(10)},	// UT_TXD(5)
	{6 , TCC_GPB(11), TCC_GPB(12), TCC_GPB(14), TCC_GPB(13), GPIO_FN(10)},	// UT_TXD(6)
	{7 , TCC_GPB(19), TCC_GPB(20), TCC_GPB(22), TCC_GPB(21), GPIO_FN(10)},	// UT_TXD(7)
	{8 , TCC_GPB(25), TCC_GPB(26), TCC_GPB(28), TCC_GPB(27), GPIO_FN(10)},	// UT_TXD(8)
	{9 , TCC_GPC(14), TCC_GPC(15), TCC_GPC(17), TCC_GPC(16), GPIO_FN(6) },	// UT_TXD(9)
	{10, TCC_GPC(22), TCC_GPC(23), TCC_GPC(25), TCC_GPC(24), GPIO_FN(6) },	// UT_TXD(10)
//	{10, TCC_GPC(1) , TCC_GPC(2) , NC_PORT    , NC_PORT    , GPIO_FN(6) },	// UT_TXD(10)
	{11, TCC_GPC(28), TCC_GPC(29), TCC_GPC(31), TCC_GPC(30), GPIO_FN(6) },	// UT_TXD(11)
//	{11, TCC_GPC(16), TCC_GPC(17), NC_PORT    , NC_PORT    , GPIO_FN(7) },	// UT_TXD(11)
//	{11, TCC_GPC(10), TCC_GPC(11), NC_PORT    , NC_PORT    , GPIO_FN(15)},	// UT_TXD(11)
	{12, TCC_GPE(13), TCC_GPE(14), TCC_GPE(16), TCC_GPE(15), GPIO_FN(5) },	// UT_TXD(12)
//	{12, TCC_GPE(11), TCC_GPE(12), NC_PORT    , NC_PORT    , GPIO_FN(5) },	// UT_TXD(12)
//	{12, TCC_GPE(15), TCC_GPE(16), NC_PORT    , NC_PORT    , GPIO_FN(9) },	// UT_TXD(12)
//	{12, TCC_GPE(20), TCC_GPE(18), NC_PORT    , NC_PORT    , GPIO_FN(15)},	// UT_TXD(12)
	{13, TCC_GPE(30), TCC_GPE(31), TCC_GPE(29), TCC_GPE(28), GPIO_FN(5) },	// UT_TXD(13)
//	{13, TCC_GPE(28), TCC_GPE(29), NC_PORT    , NC_PORT    , GPIO_FN(6) },	// UT_TXD(13)
	{14, TCC_GPF(13), TCC_GPF(14), TCC_GPF(16), TCC_GPF(15), GPIO_FN(9) },	// UT_TXD(14)
//	{14, TCC_GPF(25), TCC_GPF(26), NC_PORT    , NC_PORT    , GPIO_FN(15)},	// UT_TXD(14)
	{15, TCC_GPF(17), TCC_GPF(18), TCC_GPF(20), TCC_GPF(19), GPIO_FN(9) },	// UT_TXD(15)
	{16, TCC_GPF(30), TCC_GPF(31), TCC_GPF(29), TCC_GPF(28), GPIO_FN(9) },	// UT_TXD(16)
	{17, TCC_GPG(11), TCC_GPG(12), TCC_GPG(14), TCC_GPG(13), GPIO_FN(3) },	// UT_TXD(17)
	{18, TCC_GPG(15), TCC_GPG(16), NC_PORT    , NC_PORT    , GPIO_FN(3) },	// UT_TXD(18)
	{19, TCC_GPG(17), TCC_GPG(19), NC_PORT    , NC_PORT    , GPIO_FN(3) },	// UT_TXD(19)
	{20, TCC_GPG(10), TCC_GPG(9) , TCC_GPG(7) , TCC_GPG(8) , GPIO_FN(3) },	// UT_TXD(20)
//	{20, TCC_GPG(14), TCC_GPG(13), NC_PORT    , NC_PORT    , GPIO_FN(5) },	// UT_TXD(20)
	{21, TCC_GPG(6) , TCC_GPG(5) , NC_PORT    , NC_PORT    , GPIO_FN(3) },	// UT_TXD(21)
//	{21, TCC_GPG(14), TCC_GPG(13), NC_PORT    , NC_PORT    , GPIO_FN(6) },	// UT_TXD(21)
	{22, TCC_GPHDMI(2), TCC_GPHDMI(3), TCC_GPHDMI(1), TCC_GPHDMI(0), GPIO_FN(3) },	// UT_TXD(22)
	{23, TCC_GPADC(4) , TCC_GPADC(5) , TCC_GPADC(2) , TCC_GPADC(3) , GPIO_FN(2) },	// UT_TXD(23)
#elif defined(CONFIG_CHIP_TCC8935S) || defined(CONFIG_CHIP_TCC8933S) || defined(CONFIG_CHIP_TCC8937S)
	{0 , TCC_GPA(7),  TCC_GPA(8),  TCC_GPA(9), TCC_GPA(10) , GPIO_FN(7) },	// UT_TXD(0)
	{1 , TCC_GPA(13), TCC_GPA(14), TCC_GPA(15), TCC_GPA(16), GPIO_FN(7) },	// UT_TXD(1)
	{2 , TCC_GPD(4) , TCC_GPD(5) , TCC_GPD(6) , TCC_GPD(7) , GPIO_FN(7) },	// UT_TXD(2)
	{3 , TCC_GPD(11), TCC_GPD(12), TCC_GPD(13), TCC_GPD(14), GPIO_FN(7)},	// UT_TXD(3)
	{4 , TCC_GPD(17), TCC_GPD(18), TCC_GPD(19), TCC_GPD(20), GPIO_FN(7)},	// UT_TXD(4)	
	{5 , TCC_GPB(0) , TCC_GPB(1) , TCC_GPB(2), TCC_GPB(3) , GPIO_FN(10)},	// UT_TXD(5)
	{6 , TCC_GPB(11), TCC_GPB(12), TCC_GPB(13), TCC_GPB(14), GPIO_FN(10)},	// UT_TXD(6)
	{7 , TCC_GPB(19), TCC_GPB(20), TCC_GPB(21), TCC_GPB(22), GPIO_FN(10)},	// UT_TXD(7)
	{8 , TCC_GPB(25), TCC_GPB(26), TCC_GPB(27), TCC_GPB(28), GPIO_FN(10)},	// UT_TXD(8)
	{11, TCC_GPC(0),  TCC_GPC(1),  TCC_GPC(2) ,  TCC_GPC(3), GPIO_FN(6) },	// UT_TXD(10)
	{12, TCC_GPC(10),  TCC_GPC(11),  TCC_GPC(12) ,  TCC_GPC(13), GPIO_FN(6) },	// UT_TXD(11)
	{13, TCC_GPE(12), TCC_GPE(13), TCC_GPE(14), TCC_GPE(15), GPIO_FN(5) },	// UT_TXD(12)
	{14, TCC_GPF(11), TCC_GPF(12), TCC_GPF(13), TCC_GPF(14), GPIO_FN(9)},	// UT_TXD(14)
	{15, TCC_GPF(25), TCC_GPF(26), TCC_GPF(27), TCC_GPF(28), GPIO_FN(9)},	// UT_TXD(16)	
	{17, TCC_GPG(0), TCC_GPG(1), TCC_GPG(2), TCC_GPG(3), GPIO_FN(3) },	// UT_TXD(17)
	{18, TCC_GPG(4), TCC_GPG(5), TCC_GPG(6), TCC_GPG(7), GPIO_FN(3) },	// UT_TXD(18)	
	{19, TCC_GPG(8), TCC_GPG(9), TCC_GPG(10), TCC_GPG(11), GPIO_FN(3) },	// UT_TXD(19)	
	{20, TCC_GPG(15), TCC_GPG(14), TCC_GPG(13), TCC_GPG(12), GPIO_FN(3) },	// UT_TXD(20)	
	{21, TCC_GPG(19), TCC_GPG(18), TCC_GPG(17), TCC_GPG(16), GPIO_FN(3) },	// UT_TXD(21)	
	{22,TCC_GPHDMI(0), TCC_GPHDMI(1), TCC_GPHDMI(2), TCC_GPHDMI(3), GPIO_FN(3) },	// UT_TXD(22)
	{23,TCC_GPADC(2), TCC_GPADC(3),  NC_PORT , NC_PORT, GPIO_FN(2) },	// UT_TXD(23)
#elif defined(CONFIG_CHIP_TCC8935) || defined(CONFIG_CHIP_TCC8933)
	{1 , TCC_GPA(13), TCC_GPA(14), NC_PORT    , NC_PORT    , GPIO_FN(7) },	// UT_TXD(1)
	{2 , TCC_GPD(4) , TCC_GPD(5) , TCC_GPD(7) , TCC_GPD(6) , GPIO_FN(7) },	// UT_TXD(2)
	{3 , TCC_GPD(11), TCC_GPD(12), TCC_GPD(13), TCC_GPD(14), GPIO_FN(14)},	// UT_TXD(3)
	{5 , TCC_GPB(7) , TCC_GPB(8) , TCC_GPB(10), TCC_GPB(9) , GPIO_FN(10)},	// UT_TXD(5)
	{6 , TCC_GPB(11), TCC_GPB(12), TCC_GPB(14), TCC_GPB(13), GPIO_FN(10)},	// UT_TXD(6)
	{7 , TCC_GPB(19), TCC_GPB(20), TCC_GPB(22), TCC_GPB(21), GPIO_FN(10)},	// UT_TXD(7)
	{8 , TCC_GPB(25), TCC_GPB(26), TCC_GPB(28), TCC_GPB(27), GPIO_FN(10)},	// UT_TXD(8)
	{11, TCC_GPC(1),  TCC_GPC(20),  NC_PORT    , NC_PORT    , GPIO_FN(6) },	// UT_TXD(10)
//	{11, TCC_GPC(10), TCC_GPC(11), NC_PORT    , NC_PORT    , GPIO_FN(15) },	// UT_TXD(11)
//	{11, TCC_GPC(16), TCC_GPC(17), NC_PORT    , NC_PORT    , GPIO_FN(7) },	// UT_TXD(11)
//	{12, TCC_GPE(13), TCC_GPE(14), TCC_GPE(15), TCC_GPE(16), GPIO_FN(5) },	// UT_TXD(12)
	{12, TCC_GPE(20), TCC_GPE(18), NC_PORT    , NC_PORT    , GPIO_FN(15) },	// UT_TXD(12)
//	{12, TCC_GPE(15), TCC_GPE(16), NC_PORT    , NC_PORT    , GPIO_FN(9)},	// UT_TXD(12)
	{13, TCC_GPE(28), TCC_GPE(27), NC_PORT    , NC_PORT    , GPIO_FN(6)},	// UT_TXD(13)
	{14, TCC_GPF(13), TCC_GPF(14), TCC_GPF(15), TCC_GPF(16), GPIO_FN(9)},	// UT_TXD(14)
	{17, TCC_GPG(11), TCC_GPG(12), TCC_GPG(14), TCC_GPG(13), GPIO_FN(3) },	// UT_TXD(17)
	{18, TCC_GPG(15), TCC_GPG(16), NC_PORT    , NC_PORT    , GPIO_FN(3) },	// UT_TXD(18)
	{19, TCC_GPG(17), TCC_GPG(19), NC_PORT    , NC_PORT    , GPIO_FN(3) },	// UT_TXD(19)
	{20, TCC_GPG(15), TCC_GPG(14), TCC_GPG(12), TCC_GPG(13), GPIO_FN(15)},	// UT_TXD(20)
//	{20, TCC_GPG(19), TCC_GPG(18), TCC_GPG(16), TCC_GPG(17), GPIO_FN(15)},	// UT_TXD(20)
//	{20, TCC_GPG(10), TCC_GPG(9) , TCC_GPG(7) , TCC_GPG(8) , GPIO_FN(3) },	// UT_TXD(20)
//	{20, TCC_GPG(14), TCC_GPG(13), NC_PORT    , NC_PORT    , GPIO_FN(5) },	// UT_TXD(20)
	{21, TCC_GPG(5) , TCC_GPG(6) , NC_PORT    , NC_PORT    , GPIO_FN(3) },	// UT_TXD(21)
//	{21,TCC_GPG(14), TCC_GPG(13), NC_PORT    , NC_PORT    , GPIO_FN(6) },	// UT_TXD(21)
	{22,TCC_GPHDMI(2), TCC_GPHDMI(3), TCC_GPHDMI(0), TCC_GPHDMI(1), GPIO_FN(3) },	// UT_TXD(22)
	{23,TCC_GPADC(4), TCC_GPADC(5), TCC_GPADC(3), TCC_GPADC(2), GPIO_FN(2) },	// UT_TXD(23)

#endif
};

unsigned int NUM_UART_PORT = ARRAY_SIZE(uart_port_map);