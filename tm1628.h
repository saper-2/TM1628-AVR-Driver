/* ****************************************************************************
   *                                                                          *
   * Name    : TM1628 driver - header file                                    *
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
#ifndef _TM1628_H_
#define _TM1628_H_

#define tm1628_delay(XX) delay1us(XX) // delay function alias declaration

#define TM1628_HDELAY 5 // half-delay of CLK [us]
#define TM1628_DELAY 15 // delay between commands/etc [us]...

// hardware def.
#define TM1628_CLK_PORT PORTA
#define TM1628_CLK_DDR DDRA
#define TM1628_CLK 1

#define TM1628_DATA_PORT PORTA
#define TM1628_DATA_PIN PINA
#define TM1628_DATA_DDR DDRA
#define TM1628_DATA 2

#define TM1628_STB_PORT PORTA
#define TM1628_STB_DDR DDRA
#define TM1628_STB 0

// macros for toggling ports/directions
#define tm1628_clk_lo TM1628_CLK_PORT &= ~(1<<TM1628_CLK)
#define tm1628_clk_hi TM1628_CLK_PORT |= (1<<TM1628_CLK)
#define tm1628_stb_lo TM1628_STB_PORT &= ~(1<<TM1628_STB)
#define tm1628_stb_hi TM1628_STB_PORT |= (1<<TM1628_STB)
#define tm1628_data_out TM1628_DATA_DDR |= 1<<TM1628_DATA
#define tm1628_data_in TM1628_DATA_DDR &= ~(1<<TM1628_DATA)
#define tm1628_data_lo TM1628_DATA_PORT &= ~(1<<TM1628_DATA)
#define tm1628_data_hi TM1628_DATA_PORT |= (1<<TM1628_DATA)
#define tm1628_data_get TM1628_DATA_PIN & (1<<TM1628_DATA)

// commands defs
#define TM1628_CMD1 0x02  // display mode set
	#define TM1628_CMD1_6GRID12SEG 0x00  // 6 grids, 12 segments
	#define TM1628_CMD1_7GRID11SEG 0x01  // 7 grids, 11 segments
#define TM1628_CMD2 0x40 // data settings command
	#define TM1628_CMD2_TEST_MODE 0x08 //  test mode
	#define TM1628_CMD2_NORMAL_MODE 0x00 // normal mode
	#define TM1628_CMD2_ADR_INCR 0x00 // increment address after each byte (after command goes bytes to TM1628)
	#define TM1628_CMD2_ADR_FIXED 0x04 // fixed address
	#define TM1628_CMD2_WRITE 0x00 // write data to digits/segments
	#define TM1628_CMD2_READ 0x02 // reads data of key states
#define TM1628_CMD3 0xC0 // address settings - address=0x00..0x0d (in 4 LSB bits of command)
#define TM1628_CMD4 0x80 // display control
	#define TM1628_CMD4_DISP_OFF 0x00 // display off 
	#define TM1628_CMD4_DISP_ON 0x08 // display on
	#define TM1628_CMD4_DIM_1_16 0x00 // display brightness 1/16
	#define TM1628_CMD4_DIM_2_16 0x01 // display brightness 2/16
	#define TM1628_CMD4_DIM_4_16 0x02 // display brightness 4/16
	#define TM1628_CMD4_DIM_10_16 0x03 // display brightness 10/16
	#define TM1628_CMD4_DIM_11_16 0x04 // display brightness 11/16
	#define TM1628_CMD4_DIM_12_16 0x05 // display brightness 12/16
	#define TM1628_CMD4_DIM_13_16 0x06 // display brightness 13/16
	#define TM1628_CMD4_DIM_14_16 0x07 // display brightness 14/16



void tm1628_init(void); // init ports
void tm1628_send(uint8_t b); // send a byte
uint8_t tm1628_get(void); // get a byte from TM1628
void tm1628_command(uint8_t cmd); // send a single byte command
void tm1628_digit(uint8_t digit, uint8_t data) ; // write a raw digit data
void tm1628_digit_write_block(uint8_t start, uint8_t count, uint8_t *data); // write block to TM1628

// Bit map of keys:  5=Standby(0x20), 4=Eject(0x10), 3=Stop(0x08), 2=Play/Pause(0x04), 1=rewind(0x02), 0=forward(0x01) 
uint8_t tm1628_read_superior_keys(void); // this is a version for reading keys on PCB in Superior DVD Player
#define KEY_MASK_STBY 0x20
#define KEY_MASK_STANDBY 0x20
#define KEY_MASK_EJECT 0x10
#define KEY_MASK_STOP 0x08
#define KEY_MASK_PLAY 0x04
#define KEY_MASK_PAUSE 0x04
#define KEY_MASK_RW 0x02
#define KEY_MASK_FW 0x01

#endif   
