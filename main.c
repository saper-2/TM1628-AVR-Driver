/* ****************************************************************************
   *                                                                          *
   * Name    : TM1628 demo program                                            *
   * Author  : saper_2                                                        *
   * Date    : 2013.06.29                                                     *
   * Version : 0.1                                                            *
   * License : You are free to use this code in any kind project, but at      *
   *           least send me a info that YOU are using it. I'll be happy      *
   *           for memory :)                                                  *
   *                                                                          *
   **************************************************************************** */

#include <avr/io.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
// ------------------------
#define DEBUG_MODE
// ------------------------
#include "delay.h"
#include "usart.h"
#include "tm1628.h"

/*
 -----------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------
*/
#define led_on PORTB |= 1<<0
#define led_off PORTB &= ~(1<<0)

/* ******************************************************************************************* *
 *                             Main program start here...                                      *
 * ******************************************************************************************* */ 

#define FRAME_BUFF_LEN 24
char c, frame_buff[FRAME_BUFF_LEN];
uint8_t frame_ptr=0, process_buffer=0;

#define CHAR_DEFS_7DIGIT_COUNT 40
prog_uint8_t char_defs7digit[] = {
	// {0x00..0x09}
	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, // 0..9
	// {0x0a..0x0f}
	0x77, 0x7C,	0x58, 0x5E, 0x79, 0x71, // A,b,c,d,E,F
	// {0x10..0x17}
	0x50, 0x78, 0x6E, 0x1C, 0x5C, 0x73, 0x74, 0x76,//r,t,y,u,o,p,h,H
	// {0x18..0x1f}
	0x1E, 0x38, 0x54, 0x39, 0x3F, 0x63, 0x40, 0x09, //J,L,n,C,O,deg,-,[A-D]
	// {0x20..0x27}
	0x12, 0x24, 0x49, 0x36, 0x1B, 0x2D, 0x70, 0x46	//[B-E], [C-F], [A-D-G], [B-C-E-F], [A-B-D-E], [A-C-D-F], [E-F-G], [B-C-G]
};

struct TM_DISPLAY {
	uint8_t digit[5]; // dp1 on digit[0].bit7 , dp2 on digit[2].bit7 
	uint8_t disc;
	uint8_t icons;
	uint8_t dots;
} display;
uint8_t brightness=0;

#define DIS_ICO_DVD 0x01
#define DIS_ICO_PLAY 0x02
#define DIS_ICO_PAUSE 0x04
#define DIS_ICO_CD 0x08
#define DIS_ICO_VCD 0x10
#define DIS_ICO_DTS 0x20
#define DIS_ICO_DD 0x40

void flush_frame_buff(void) {
	for (uint8_t t=0;t<FRAME_BUFF_LEN;t++) frame_buff[t] = 0;
	frame_ptr=0;
	#ifdef DEBUG_MODE
		usart_send_strP((prog_char*)PSTR("\r\n***FLUSH***\r\n"));
	#endif
}

void update_display(void) {
	uint8_t data[14], i,j, shift=0;
	for (i=0;i<14;i++) data[i]=0;
	// translating this going to be A BIG PAIN IN THE NECK -_-
	for (i=0;i<5;i++) {
		if (i==0) shift=0;
		if (i==1) shift=1;
		if (i==2) shift=7;
		if (i==3) shift=2;
		if (i==4) shift=3;
		for (j=0;j<7;j++) {
			if ((display.digit[i]>>j) & 0x01) data[j*2] |= 1<<shift;
		}
	}
	// dp1,2
	if (display.dots & 0x01) data[11] |= 0x01;
	if (display.dots & 0x02) data[2] |= 0x10;
	// icons...
	if (display.icons & DIS_ICO_DVD) data[1] |= 0x01;
	if (display.icons & DIS_ICO_PLAY) data[3] |= 0x01;
	if (display.icons & DIS_ICO_PAUSE) data[9] |= 0x01;
	if (display.icons & DIS_ICO_CD) data[7] |= 0x01;
	if (display.icons & DIS_ICO_VCD) data[0] |= 0x10;
	if (display.icons & DIS_ICO_DTS) data[8] |= 0x10;
	if (display.icons & DIS_ICO_DD) data[6] |= 0x10;
	// disc
	if (display.disc & 0x01) data[3] |= 0x02;
	if (display.disc & 0x02) data[11] |= 0x02;
	if (display.disc & 0x04) data[13] |= 0x02;
	if (display.disc & 0x08) data[7] |= 0x02;
	if (display.disc & 0x10) data[9] |= 0x02;
	if (display.disc & 0x20) data[1] |= 0x02;
	// send
	tm1628_digit_write_block(0, 14, &data[0]);
}

void set_display_brightness(uint8_t level) {
	if (level < 250) {
		brightness=level&0x07;
	}
	tm1628_command(TM1628_CMD4|TM1628_CMD4_DISP_ON|(brightness&0x07));
	//eeprom_write_byte((uint8_t*)0x10, brightness&0x07);
}

uint8_t dec2bin(char c) {
	c = c-'0';
	if (c > 9) c = c - 7;
	return c;
}

int main(void) {
	//uint16_t x,y;
	//uint16_t avr_itemp;
	uint8_t t=0,i=0,j=0,x=0;//, dta[5];
	
	DDRB |= 1<<0;
	led_off;
	
	usart_config(12, USART_TX_ENABLE|USART_RX_ENABLE|USART_RX_INT_COMPLET, USART_MODE_8N1);
	// enable ints
	sei();
	usart_send_strP((prog_char*)PSTR("\r\nTM1628 test start...\r\n"));
	
	delay1ms(20);
	tm1628_init();
	
	tm1628_command(TM1628_CMD1|TM1628_CMD1_7GRID11SEG);
	tm1628_command(TM1628_CMD4|TM1628_CMD4_DISP_ON|TM1628_CMD4_DIM_1_16);
	
	usart_send_strP((prog_char*)PSTR("Display clear.....\r\n"));

	for (t=0;t<14;t++) tm1628_digit(t,0x00);

	usart_send_strP((prog_char*)PSTR("TM1628 init done. Main loop start...\r\n"));
	
	for (t=0;t<5;t++) display.digit[t]=0;
	flush_frame_buff();
	while(1) {
		t = tm1628_read_superior_keys();

		if (t>0) {
			/*if (t&KEY_MASK_PLAY) {
				usart_send_char('p'); usart_send_char(0x0d);
			}
			if (t&KEY_MASK_STOP) {
				usart_send_char('s'); usart_send_char(0x0d);
			}
			if (t&KEY_MASK_RW) {
				usart_send_char('r'); usart_send_char(0x0d);
			}			
			if (t&KEY_MASK_STANDBY) {
				usart_send_char('t'); usart_send_char(0x0d);
			}
			if (t&KEY_MASK_EJECT) {
				usart_send_char('e'); usart_send_char(0x0d);
			}
			if (t&KEY_MASK_FW) {
				usart_send_char('f'); usart_send_char(0x0d);
			}*/
			
			usart_send_strP((prog_char*)PSTR("Key press: "));
			usart_send_hex_byte(t);
			usart_send_char(' ');
			usart_send_bin_byte(t);
			usart_send_strP((prog_char*)PSTR("\r\n"));
			delay1ms(75);
		}
		
		update_display();
		delay1ms(1);
		
		// process serial buffer - each command is ended with only: CR = 0x0D = \r
		if (process_buffer == 1) {
			if (frame_buff[0] == 'd') { // set digit: d[#num 1char][char_code 1char]
				x = dec2bin(frame_buff[1]); // digit [0..4] - bigger number (5 or higher) apply char code to all digits
				if (x > 4) {
					j = dec2bin(frame_buff[2]); // code [ 0..A..F..G..Z[\]^ ] <- look on ASCII table, \..
					// numbers (from 48 to 57 ascii codes) and 
					// upper case letters (from 65 to 90 and special characters from 91 to 94 including) (numbers are decimal ascii codes)
					if (j>(CHAR_DEFS_7DIGIT_COUNT-1)) {
						i = 0;
					} else {
						i = pgm_read_byte(&char_defs7digit[j]);
					}
					for (x=0;x<5;x++) display.digit[x] = i;
				} else {
					j = dec2bin(frame_buff[2]); // code - read comment above :)
					if (j>(CHAR_DEFS_7DIGIT_COUNT-1)) {
						display.digit[x] = 0;
					} else {
						display.digit[x] = pgm_read_byte(&char_defs7digit[j]);
					}
				}
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR("Digit/char_code: "));
					usart_send_int(x);
					usart_send_char('/');
					usart_send_int(j);
					usart_send_strP((prog_char*)PSTR("\r\n"));
				#endif
			} else if (frame_buff[0] == 'i') { // set icon state: i[icon_number 1char][state 1char]
				x = dec2bin(frame_buff[1]); // icon number (0..6) - 7..9 - apply state for all icons
				j = dec2bin(frame_buff[2]); // state (0=off/1=on)
				if (x > 6) {
					if (j == 1) {
						display.icons = 0xff;
					} else {
						display.icons = 0x00;
					}
				} else {
					if (j == 1) {
						display.icons |= (1<<x);
					} else {
						display.icons &= ~(1<<x);
					}
				}
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR("Icon=state: "));
					usart_send_int(x);
					usart_send_char('=');
					usart_send_int(j);
					usart_send_strP((prog_char*)PSTR("\r\n"));
				#endif
			} else if (frame_buff[0] == 'c') { // set disc/circle state: c[piece 1char][state 1char]
				x = dec2bin(frame_buff[1]); // piece number (clockwise, piece 0 at 12hour) (0..5) , 6..9 - all pieces
				j = dec2bin(frame_buff[2]); // state (0/1)
				if (x > 5) {
					if (j == 1) {
						display.disc = 0xff;
					} else {
						display.disc = 0x00;
					}
				} else {
					if (j == 1) {
						display.disc |= (1<<x);
					} else {
						display.disc &= ~(1<<x);
					}
				}
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR("Circle_disc=state: "));
					usart_send_int(x);
					usart_send_char('=');
					usart_send_int(j);
					usart_send_strP((prog_char*)PSTR("\r\n"));
				#endif
			} else if (frame_buff[0] == 'p') { // set duble dots state: p[dotNum 1char][state 1char]
				x = dec2bin(frame_buff[1]); // dots number (0,1) - numbering from left to right | 2..9 - state for all dots
				j = dec2bin(frame_buff[2]); // state (0/1)
				if (x > 1) {
					if (j == 1) {
						display.dots = 0x03;
					} else {
						display.dots = 0x00;
					}
				} else {
					if (j == 1) {
						display.dots |= (1<<x);
					} else {
						display.dots &= ~(1<<x);
					}
				}
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR("DoubleDots=state: "));
					usart_send_int(x);
					usart_send_char('=');
					usart_send_int(j);
					usart_send_strP((prog_char*)PSTR("\r\n"));
				#endif
			} else if (frame_buff[0] == 't') { // set text + double dots: 
				// t[digit0 1char][digit1 1char][digit2 1char][digit3 1char][digit4 1char][dots0 1char][dots1 1char]
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR("TextToSet=Dots: "));
				#endif
				for (x=1;x<6;x++) {
					j = dec2bin(frame_buff[x]);
					#ifdef DEBUG_MODE
						usart_send_hex_byte(j);
						usart_send_char(' ');
					#endif
					if (j>CHAR_DEFS_7DIGIT_COUNT) {
						display.digit[x-1] = 0;
					} else {
					display.digit[x-1] = pgm_read_byte(&char_defs7digit[j]);
					}
				}
				i = dec2bin(frame_buff[6]);
				j = dec2bin(frame_buff[7]);
				if (i == 1) display.dots |= 0x01;
					else  display.dots &= ~0x01;
				if (j == 1) display.dots |= 0x02;
					else  display.dots &= ~0x02;
				#ifdef DEBUG_MODE
					usart_send_char('=');
					usart_send_int(i);
					usart_send_char(' ');
					usart_send_int(j);
					usart_send_strP((prog_char*)PSTR("\r\n"));
				#endif
			} else if (frame_buff[0] == 'a') { // set all: a[digits 5chars][dots 2chars][disc 6chars][icons 7chars] <total: 21chars>
				// dots, disc, icons - each char define state of each doubleDot/discPiece/icon allowed 0=off or 1=on
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR("AllSet [Digit:5]="));
				#endif
				for (x=1;x<6;x++) {
					j = dec2bin(frame_buff[x]);
					#ifdef DEBUG_MODE
						usart_send_hex_byte(j);
						usart_send_char(' ');
					#endif
					if (j>CHAR_DEFS_7DIGIT_COUNT) {
						display.digit[x-1] = 0;
					} else {
					display.digit[x-1] = pgm_read_byte(&char_defs7digit[j]);
					}
				}
				// dots
				i = dec2bin(frame_buff[6]);
				j = dec2bin(frame_buff[7]);
				if (i == 1) display.dots |= 0x01;
					else  display.dots &= ~0x01;
				if (j == 1) display.dots |= 0x02;
					else  display.dots &= ~0x02;
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR(",[Dots:2]="));
					usart_send_int(i);
					usart_send_char(' ');
					usart_send_int(j);
				#endif
				// disc
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR(",[disc:6]="));
				#endif
				for (x=8,j=0;j<6;x++,j++) {
					i = dec2bin(frame_buff[x]);
					if (i == 1) {
						display.disc |= (1<<j);
					} else {
						display.disc &= ~(1<<j);
					}
					#ifdef DEBUG_MODE
						usart_send_int(i);
					#endif
				}
				// icons
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR(",[icons:7]="));
				#endif
				for (x=14,j=0;j<7;x++,j++) {
					i = dec2bin(frame_buff[x]);
					if (i == 1) {
						display.icons |= (1<<j);
					} else {
						display.icons &= ~(1<<j);
					}
					#ifdef DEBUG_MODE
						usart_send_int(i);
					#endif
				}
				#ifdef DEBUG_MODE
					usart_send_strP((prog_char*)PSTR("\r\n"));
				#endif
			} else if (frame_buff[0] == 'b') { // set display brightness: b[level 1char]
				x = dec2bin(frame_buff[1]); // level [0..7]
				if (x>7) x=7;
				tm1628_command(TM1628_CMD4|TM1628_CMD4_DISP_ON|(x & 0x07));
			}
			// flush buffer
			#ifdef DEBUG_MODE
				usart_send_strP((prog_char*)PSTR("Processed frame: {"));
				usart_send_str(frame_buff);
				usart_send_strP((prog_char*)PSTR("} hex: "));
				for (t=0;t<FRAME_BUFF_LEN;t++) {
					usart_send_hex_byte(frame_buff[t]);
					usart_send_char(' ');
				}
				usart_send_strP((prog_char*)PSTR(" Len="));
				usart_send_int(frame_ptr);
				usart_send_strP((prog_char*)PSTR("\r\n"));
			// end debug
			#endif
			flush_frame_buff();
			process_buffer=0;
		}
	}
}

SIGNAL (SIG_USART_RECV) {
	char c = UDR;
	if (c == 0x0d) { // CR
		process_buffer=1;
	} else if (c == 0x1b) { // ESC
		flush_frame_buff();
	} else {
		if (frame_ptr < FRAME_BUFF_LEN) {
			frame_buff[frame_ptr++] = c;
		}
		#ifdef DEBUG_MODE
		else 
		{
			usart_send_strP((prog_char*)PSTR("\r\n**BUFF_OVERFLOW**\r\n"));
		}
		#endif
	}
}
