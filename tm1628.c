/* ****************************************************************************
   *                                                                          *
   * Name    : TM1628 driver - code                                           *
   * Author  : saper_2                                                        *
   * Date    : 2013.06.29                                                     *
   * Version : 0.1                                                            *
   * License : You are free to use this library in any kind project, but at   *
   *           least send me a info that YOU are using it. I'll be happy      *
   *           for memory :)                                                  *
   * About   : TM1628 is a multiplexed LED display driver with                *
   *           ability to support up to 16 keys organised in 8x2 matrix       *
   *                                                                          *
   * Bugs    : ??                                                             *
   *                                                                          *
   *                                                                          *
   **************************************************************************** */

#include <avr/io.h>
#include <inttypes.h>
#include "tm1628.h"
#include "delay.h"


// port init
void tm1628_init(void) {
	// CLK setup
	TM1628_CLK_DDR |= 1<<TM1628_CLK;
	tm1628_clk_hi;
	// STB
	TM1628_STB_DDR |= 1<<TM1628_STB;
	tm1628_stb_hi;
	// DATA
	tm1628_data_in;
	tm1628_data_hi;
	tm1628_delay(TM1628_DELAY);
	
}
// send byte to wire :)
void tm1628_send(uint8_t b) {
	tm1628_data_out;
	for (uint8_t i=0;i<8;i++) {
		tm1628_clk_lo;
		if (b&0x01) tm1628_data_hi; 
			else tm1628_data_lo;
		b >>= 1;
		tm1628_delay(TM1628_HDELAY);
		tm1628_clk_hi; // latch DATA in TM1628 (rising edge)
		tm1628_delay(TM1628_HDELAY);
	}
	tm1628_data_in;
	tm1628_data_hi;
}
// get one byte...
uint8_t tm1628_get(void) {
	uint8_t res=0x00;
	tm1628_data_in;
	tm1628_data_hi;
	tm1628_delay(TM1628_HDELAY);
	for (uint8_t i=0;i<8;i++) {
		res <<= 1;
		tm1628_clk_lo;
		tm1628_delay(TM1628_HDELAY);
		tm1628_clk_hi; // latch DATA from TM1628 on riding edge
		if (tm1628_data_get) res |= 0x01;
		tm1628_delay(TM1628_HDELAY);
	}
	tm1628_data_in;
	tm1628_data_hi;
	return res;
}

// send command
void tm1628_command(uint8_t cmd) {
	tm1628_stb_lo;
	tm1628_delay(TM1628_HDELAY);
	tm1628_send(cmd);
	tm1628_stb_hi;
	tm1628_delay(TM1628_HDELAY);
}

// send raw digit data
void tm1628_digit(uint8_t digit, uint8_t data) {
	// set address - command 3
	if (digit > 0x0d) digit=0x0d;
	tm1628_command(0xc0|(digit&0x0f));
	// send digit - fixed address mode
	tm1628_delay(TM1628_DELAY);
	tm1628_stb_lo;
	tm1628_delay(TM1628_HDELAY);
	tm1628_send(0x44); // 
	tm1628_send(data); // 
	tm1628_stb_hi;
	tm1628_delay(TM1628_HDELAY);
}

// write a few bytes at once to TM1628
void tm1628_digit_write_block(uint8_t start, uint8_t count, uint8_t *data) {
	// set address - command 3
	if (start > 0x0d) start=0x0d;
	if (count > 14) count=14;
	tm1628_command(0xc0|(start&0x0f));
	// send digit - incr address mode
	tm1628_delay(TM1628_DELAY);
	tm1628_stb_lo;
	tm1628_delay(TM1628_HDELAY);
	tm1628_send(TM1628_CMD2|TM1628_CMD2_NORMAL_MODE|TM1628_CMD2_ADR_INCR|TM1628_CMD2_WRITE); // write data, incr addr, normal mode
	for (;count>0;count--) {
		tm1628_send(*data); //
		data++; 
	}
	tm1628_stb_hi;
	tm1628_delay(TM1628_HDELAY);
}

// this function is exclusivly for my PCB from Superior DVD Player, but with little tweaks (fixing key map for your pcb) will work too
uint8_t tm1628_read_superior_keys(void) {
	uint8_t res=0, t1, t2;
	tm1628_stb_lo;
	tm1628_delay(TM1628_HDELAY);
	tm1628_send(TM1628_CMD2|TM1628_CMD2_NORMAL_MODE|TM1628_CMD2_ADR_INCR|TM1628_CMD2_READ); // 
	tm1628_delay(TM1628_DELAY);
	tm1628_get(); // 1st byte - not used, discard...
	t1 = tm1628_get(); // 2nd byte - needed - contains: Play/Pause, Standby, Forward, Rewind
	t2 = tm1628_get(); // 3rd byte - needed - contains: Stop, Eject
	tm1628_get(); // 4th byte - not used, discard...
	tm1628_stb_hi;
	tm1628_delay(TM1628_HDELAY);
	// set key bits...
	// bit map of keys: 5=Standby(0x20), 4=Eject(0x10), 3=Stop(0x08), 2=Play/Pause(0x04), 1=rewind(0x02), 0=forward(0x01) 
	if (t1&0x08) res |= 0x01;
	if (t1&0x10) res |= 0x02;
	if (t1&0x40) res |= 0x20;
	if (t1&0x80) res |= 0x04;
	if (t2&0x80) res |= 0x08;
	if (t2&0x40) res |= 0x10;
	return res;
}
