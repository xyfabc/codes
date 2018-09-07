#include <asm/jzsoc.h>

#define GOLF
//#undef GOLF
#define KEYBOARD_MAJOR		52
//#define KBD_IRQ_PIN			115
//#define KBD_IRQ_PIN			(32*5 + 8) // PF8: 168
//#define KBD_IRQ_PIN (32*1 + 3)  // PB3 for jz4770
#define KBD_IRQ_PIN (32*0 + 5)  // PA5 for jz4775
#define IRQ_KBD				(IRQ_GPIO_0 + KBD_IRQ_PIN)

#define KBD_PWOFF_IRQ_PIN	106
#define IRQ_KBD_PWOFF		(IRQ_GPIO_0 + KBD_PWOFF_IRQ_PIN)

//#define REG_SADC_CFG		REG32(SADC_CFG)

/* DEBUG  */
#define JZ_KBD_DEBUG 1

#ifdef JZ_KBD_DEBUG
#define dbg(format, arg...) printk( ": " format "\n" , ## arg)
#else
#define dbg(format, arg...) do {} while (0)
#endif
#define err(format, arg...) printk(": " format "\n" , ## arg)
#define info(format, arg...) printk(": " format "\n" , ## arg)
#define warn(format, arg...) printk(": " format "\n" , ## arg)

/*  key value */
#define FL_Button	0xfee8	///< A mouse button; use Fl_Button + n for mouse button n.
#define FL_BackSpace	0xff08	///< The backspace key.
#define FL_Tab		0xff09	///< The tab key.
#define FL_Enter	0xff0d	///< The enter key. 
#define FL_Pause	0xff13	///< The pause key.
#define FL_Scroll_Lock	0xff14	///< The scroll lock key.
#define FL_Escape	0xff1b	///< The escape key.
#define FL_Home		0xff50	///< The home key.
#define FL_Left		0xff51	///< The left arrow key.
#define FL_Up		0xff52	///< The up arrow key.
#define FL_Right	0xff53	///< The right arrow key.
#define FL_Down		0xff54	///< The down arrow key.
#define FL_Page_Up	0xff55	///< The page-up key.
#define FL_Page_Down	0xff56	///< The page-down key.
#define FL_End		0xff57	///< The end key.
#define FL_Print	0xff61	///< The print (or print-screen) key.
#define FL_Insert	0xff63	///< The insert key. 
#define FL_Menu		0xff67	///< The menu key.
#define FL_Help		0xff68	///< The 'help' key on Mac keyboards
#define FL_Num_Lock	0xff7f	///< The num lock key.
#define FL_KP		0xff80	///< One of the keypad numbers; use FL_KP + n for number n.
#define FL_KP_Enter	0xff8d	///< The enter key on the keypad, same as Fl_KP+'\\r'.
#define FL_KP_Last	0xffbd	///< The last keypad key; use to range-check keypad.
#define FL_F		0xffbd	///< One of the function keys; use FL_F + n for function key n.
#define FL_F_Last	0xffe0	///< The last function key; use to range-check function keys.
#define FL_Shift_L	0xffe1	///< The lefthand shift key.
#define FL_Shift_R	0xffe2	///< The righthand shift key.
#define FL_Control_L	0xffe3	///< The lefthand control key.
#define FL_Control_R	0xffe4	///< The righthand control key.
#define FL_Caps_Lock	0xffe5	///< The caps lock key.
#define FL_Meta_L	0xffe7	///< The left meta/Windows key.
#define FL_Meta_R	0xffe8	///< The right meta/Windows key.
#define FL_Alt_L	0xffe9	///< The left alt key.
#define FL_Alt_R	0xffea	///< The right alt key. 
#define FL_Delete	0xffff	///< The delete key.


/*MS8885 command*/

#define SRES	0x00
#define CLINT	0x03
#define SLEEP	0x05
#define WAKE	0x06
#define CFGRD	0x33
#define CFGWR	0x30
#define CLKWD	0x35
#define CLKRD	0x36
#define MSKWR	0x39
#define MSKRD	0x3a
#define INTI2C	0x3c

//





struct key_map
{
	int key;
	int val;
};

struct key_remap
{
	int val;
	int reval;
};

struct key_remap g_key_remap[] = 
{
#if 0
	{0x11,FL_Escape},
	{0x13,FL_Enter},

	{0x41,'1'},
	{0x42,'2'},
	{0x43,'3'},
	{0x31,'4'},
	{0x32,'5'},
	{0x33,'6'},
	{0x21,'7'},
	{0x22,'8'},
	{0x23,'9'},
	{0x12,'0'},
	
	{0x18,FL_F+0},
	{0x15,FL_F+1},
	{0x16,FL_F+2},
	{0x17,FL_F+3},
	
	{0x44,FL_BackSpace},

	{0x24,FL_Up},
	{0x34,FL_Menu},
	{0x14,FL_Down},
#elif 1
	{0x44,0x11},  // ESC
	{0x42,0x13},  // OK

	{0x14,0x41},  // 1
	{0x41,0x42},  // 2
	{0x12,0x43},  // 3
	{0x24,0x31},  // 4
	{0x31,0x32},  // 5
	{0x22,0x33},  // 6
	{0x34,0x21},  // 7
	{0x21,0x22},  // 8
	{0x32,0x23},  // 9
	{0x11,0x12},  // 0
	
	{0x18,0x18},  // F0
	{0x15,0x15},  // F1
	{0x16,0x16},  // F2
	{0x17,0x17},  // F3
	
	{0x43,0x44},  // BKSP

	{0x23,0x24},  // UP
	{0x33,0x34},  // MENU
	{0x13,0x14},  // DOWN
#endif
	{0,-1}
};

/*add by dy*/
struct key_remap d_key_remap[] = 
{
#if 0			// modify, yjt, 20130821
	{0x0d,0x11},  // ESC
	{0x0f,0x13},  // OK

	{0x01,0x41},  // 1
	{0x02,0x42},  // 2
	{0x03,0x43},  // 3
	{0x04,0x31},  // 4
	{0x05,0x32},  // 5
	{0x06,0x33},  // 6
	{0x07,0x21},  // 7
	{0x08,0x22},  // 8
	{0x09,0x23},  // 9
	{0x0e,0x12},  // 0
		
	{0x0a,0x44},  // BKSP

	{0x0c,0x24},  // UP
	{0x0b,0x34},  // MENU
	{0x10,0x14},  // DOWN
#else
	{0x0d,0x41},  // ESC
	{0x0f,0x21},  // OK

	{0x01,0x44},  // 1
	{0x02,0x34},  // 2
	{0x03,0x24},  // 3
	{0x04,0x43},  // 4
	{0x05,0x33},  // 5
	{0x06,0x23},  // 6
	{0x07,0x42},  // 7
	{0x08,0x32},  // 8
	{0x09,0x22},  // 9
	{0x0e,0x31},  // 0
		
	{0x0a,0x14},  // BKSP

	{0x0c,0x12},  // UP
	{0x0b,0x13},  // MENU
	{0x10,0x11},  // DOWN
#endif
};


