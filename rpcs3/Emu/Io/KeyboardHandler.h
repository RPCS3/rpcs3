#pragma once

#include <vector>

extern u16 cellKbCnvRawCode(u32 arrange, u32 mkey, u32 led, u16 rawcode); // (TODO: Can it be problematic to place SysCalls in middle of nowhere?)

enum KbPortStatus
{
	CELL_KB_STATUS_DISCONNECTED		= 0x00000000,
	CELL_KB_STATUS_CONNECTED		= 0x00000001,
};

enum CellKbReadMode
{
  CELL_KB_RMODE_INPUTCHAR = 0,
  CELL_KB_RMODE_PACKET    = 1,
};

enum CellKbCodeType
{
  CELL_KB_CODETYPE_RAW    = 0,
  CELL_KB_CODETYPE_ASCII  = 1,
};

enum KbLedCodes
{
	CELL_KB_LED_NUM_LOCK	= 0x00000001,
	CELL_KB_LED_CAPS_LOCK	= 0x00000002,
	CELL_KB_LED_SCROLL_LOCK = 0x00000004,
	CELL_KB_LED_COMPOSE	    = 0x00000008,
	CELL_KB_LED_KANA	    = 0x00000016,
};

enum KbMetaKeys
{
	CELL_KB_MKEY_L_CTRL	    = 0x00000001,
	CELL_KB_MKEY_L_SHIFT	= 0x00000002,
	CELL_KB_MKEY_L_ALT	    = 0x00000004,
	CELL_KB_MKEY_L_WIN	    = 0x00000008,
	CELL_KB_MKEY_R_CTRL	    = 0x00000010,
	CELL_KB_MKEY_R_SHIFT	= 0x00000020,
	CELL_KB_MKEY_R_ALT	    = 0x00000040,
	CELL_KB_MKEY_R_WIN	    = 0x00000080,
};

enum Keys
{
	// Non-ASCII Raw data
	CELL_KEYC_NO_EVENT	    = 0x00,
	CELL_KEYC_E_ROLLOVER	= 0x01,
	CELL_KEYC_E_POSTFAIL	= 0x02,
	CELL_KEYC_E_UNDEF	    = 0x03,
	CELL_KEYC_ESCAPE	    = 0x29,
	CELL_KEYC_106_KANJI		= 0x35,
	CELL_KEYC_CAPS_LOCK		= 0x39,
	CELL_KEYC_F1		    = 0x3a,
	CELL_KEYC_F2		    = 0x3b,
	CELL_KEYC_F3		    = 0x3c,
	CELL_KEYC_F4		    = 0x3d,
	CELL_KEYC_F5		    = 0x3e,
	CELL_KEYC_F6		    = 0x3f,
	CELL_KEYC_F7		    = 0x40,
	CELL_KEYC_F8		    = 0x41,
	CELL_KEYC_F9		    = 0x42,
	CELL_KEYC_F10		    = 0x43,
	CELL_KEYC_F11		    = 0x44,
	CELL_KEYC_F12		    = 0x45,
	CELL_KEYC_PRINTSCREEN	= 0x46,
	CELL_KEYC_SCROLL_LOCK	= 0x47,
	CELL_KEYC_PAUSE			= 0x48,
	CELL_KEYC_INSERT	    = 0x49,
	CELL_KEYC_HOME	        = 0x4a,
	CELL_KEYC_PAGE_UP	    = 0x4b,
	CELL_KEYC_DELETE	    = 0x4c,
	CELL_KEYC_END		    = 0x4d,
	CELL_KEYC_PAGE_DOWN		= 0x4e,
	CELL_KEYC_RIGHT_ARROW	= 0x4f,
	CELL_KEYC_LEFT_ARROW	= 0x50,
	CELL_KEYC_DOWN_ARROW	= 0x51,
	CELL_KEYC_UP_ARROW	    = 0x52,
	CELL_KEYC_NUM_LOCK	    = 0x53,
	CELL_KEYC_APPLICATION	= 0x65,
	CELL_KEYC_KANA	        = 0x88,
	CELL_KEYC_HENKAN	    = 0x8a,
	CELL_KEYC_MUHENKAN	    = 0x8b,
	
	// Raw keycodes for ASCII keys
	CELL_KEYC_A			    = 0x04,
	CELL_KEYC_B			    = 0x05,
	CELL_KEYC_C			    = 0x06,
	CELL_KEYC_D			    = 0x07,
	CELL_KEYC_E			    = 0x08,
	CELL_KEYC_F			    = 0x09,
	CELL_KEYC_G			    = 0x0A,
	CELL_KEYC_H			    = 0x0B,
	CELL_KEYC_I			    = 0x0C,
	CELL_KEYC_J			    = 0x0D,
	CELL_KEYC_K			    = 0x0E,
	CELL_KEYC_L			    = 0x0F,
	CELL_KEYC_M			    = 0x10,
	CELL_KEYC_N			    = 0x11,
	CELL_KEYC_O			    = 0x12,
	CELL_KEYC_P			    = 0x13,
	CELL_KEYC_Q			    = 0x14,
	CELL_KEYC_R			    = 0x15,
	CELL_KEYC_S			    = 0x16,
	CELL_KEYC_T			    = 0x17,
	CELL_KEYC_U			    = 0x18,
	CELL_KEYC_V			    = 0x19,
	CELL_KEYC_W			    = 0x1A,
	CELL_KEYC_X			    = 0x1B,
	CELL_KEYC_Y			    = 0x1C,
	CELL_KEYC_Z			    = 0x1D,
	CELL_KEYC_1			    = 0x1E,
	CELL_KEYC_2			    = 0x1F,
	CELL_KEYC_3			    = 0x20,
	CELL_KEYC_4			    = 0x21,
	CELL_KEYC_5			    = 0x22,
	CELL_KEYC_6			    = 0x23,
	CELL_KEYC_7			    = 0x24,
	CELL_KEYC_8			    = 0x25,
	CELL_KEYC_9			    = 0x26,
	CELL_KEYC_0			    = 0x27,
	CELL_KEYC_ENTER		    = 0x28,
	CELL_KEYC_ESC			= 0x29,
	CELL_KEYC_BS			= 0x2A,
	CELL_KEYC_TAB			= 0x2B,
	CELL_KEYC_SPACE		    = 0x2C,
	CELL_KEYC_MINUS		    = 0x2D,
	CELL_KEYC_EQUAL_101		= 0x2E,
	CELL_KEYC_ACCENT_CIRCONFLEX_106 = 0x2E,
	CELL_KEYC_LEFT_BRACKET_101	    = 0x2F,
	CELL_KEYC_ATMARK_106		    = 0x2F,
	CELL_KEYC_RIGHT_BRACKET_101	    = 0x30,
	CELL_KEYC_LEFT_BRACKET_106	    = 0x30,
	CELL_KEYC_BACKSLASH_101	        = 0x31,
	CELL_KEYC_RIGHT_BRACKET_106	    = 0x32,
	CELL_KEYC_SEMICOLON		    = 0x33,
	CELL_KEYC_QUOTATION_101	    = 0x34,
	CELL_KEYC_COLON_106		    = 0x34,
	CELL_KEYC_COMMA		        = 0x36,
	CELL_KEYC_PERIOD		    = 0x37,
	CELL_KEYC_SLASH		        = 0x38,
	//CELL_KEYC_CAPS_LOCK		    = 0x39,
	CELL_KEYC_KPAD_NUMLOCK	    = 0x53,
	CELL_KEYC_KPAD_SLASH		= 0x54,
	CELL_KEYC_KPAD_ASTERISK	    = 0x55,
	CELL_KEYC_KPAD_MINUS		= 0x56,
	CELL_KEYC_KPAD_PLUS		    = 0x57,
	CELL_KEYC_KPAD_ENTER		= 0x58,
	CELL_KEYC_KPAD_1		    = 0x59,
	CELL_KEYC_KPAD_2		    = 0x5A,
	CELL_KEYC_KPAD_3		    = 0x5B,
	CELL_KEYC_KPAD_4		    = 0x5C,
	CELL_KEYC_KPAD_5		    = 0x5D,
	CELL_KEYC_KPAD_6		    = 0x5E,
	CELL_KEYC_KPAD_7		    = 0x5F,
	CELL_KEYC_KPAD_8		    = 0x60,
	CELL_KEYC_KPAD_9		    = 0x61,
	CELL_KEYC_KPAD_0		    = 0x62,
	CELL_KEYC_KPAD_PERIOD		= 0x63,
	CELL_KEYC_BACKSLASH_106	    = 0x87,
	CELL_KEYC_YEN_106		    = 0x89,
};

enum CellKbMappingType
{
	CELL_KB_MAPPING_101,
	CELL_KB_MAPPING_106,
	CELL_KB_MAPPING_106_KANA,
	CELL_KB_MAPPING_GERMAN_GERMANY,
	CELL_KB_MAPPING_SPANISH_SPAIN,
	CELL_KB_MAPPING_FRENCH_FRANCE,
	CELL_KB_MAPPING_ITALIAN_ITALY,
	CELL_KB_MAPPING_DUTCH_NETHERLANDS,
	CELL_KB_MAPPING_PORTUGUESE_PORTUGAL,
	CELL_KB_MAPPING_RUSSIAN_RUSSIA,
	CELL_KB_MAPPING_ENGLISH_UK,
	CELL_KB_MAPPING_KOREAN_KOREA,
	CELL_KB_MAPPING_NORWEGIAN_NORWAY,
	CELL_KB_MAPPING_FINNISH_FINLAND,
	CELL_KB_MAPPING_DANISH_DENMARK,
	CELL_KB_MAPPING_SWEDISH_SWEDEN,
	CELL_KB_MAPPING_CHINESE_TRADITIONAL,
	CELL_KB_MAPPING_CHINESE_SIMPLIFIED,
	CELL_KB_MAPPING_SWISS_FRENCH_SWITZERLAND,
	CELL_KB_MAPPING_SWISS_GERMAN_SWITZERLAND,
	CELL_KB_MAPPING_CANADIAN_FRENCH_CANADA,
	CELL_KB_MAPPING_BELGIAN_BELGIUM,
	CELL_KB_MAPPING_POLISH_POLAND,
	CELL_KB_MAPPING_PORTUGUESE_BRAZIL,
};

static const u32 CELL_KB_MAX_KEYBOARDS = 127;
static const u32 CELL_KB_MAX_PORT_NUM = 7;
static const u32 CELL_KB_MAX_KEYCODES = 62;

struct KbInfo
{
	u32 max_connect;
	u32 now_connect;
	u32 info;
	u8 status[CELL_KB_MAX_KEYBOARDS];
};

struct CellKbData
{
	u32 led;
	u32 mkey;
	s32 len;
	u16 keycode[CELL_KB_MAX_KEYCODES];

	CellKbData()
		: led(0)
		, mkey(0)
		, len(0)
	{ // (TODO: Set initial state of led)
	}
};

struct CellKbConfig
{
	u32 arrange;
	u32 read_mode;
	u32 code_type;

	CellKbConfig()
		: arrange(CELL_KB_MAPPING_106)
		, read_mode(CELL_KB_RMODE_INPUTCHAR)
		, code_type(CELL_KB_CODETYPE_ASCII)
	{
	}
};

struct KbButton
{
	u32 m_keyCode;
	u32 m_outKeyCode;
	bool m_pressed;

	KbButton(u32 keyCode, u32 outKeyCode)
		: m_pressed(false)
		, m_keyCode(keyCode)
		, m_outKeyCode(outKeyCode)
	{
	}
};

struct Keyboard
{
	CellKbData m_data;
	CellKbConfig m_config;
	std::vector<KbButton> m_buttons;

	Keyboard()
		: m_data()
		, m_config()
	{
	}
};

class KeyboardHandlerBase
{
protected:
	KbInfo m_info;
	std::vector<Keyboard> m_keyboards;

public:
	virtual void Init(const u32 max_connect)=0;
	virtual void Close()=0;

	void Key(const u32 code, bool pressed)
	{
		for(Keyboard& keyboard : m_keyboards)
		{
			for(KbButton& button : keyboard.m_buttons)
			{
				if(button.m_keyCode != code)
					continue;

				CellKbData& data = keyboard.m_data;
				CellKbConfig& config = keyboard.m_config;

				if (pressed)
				{
					// Meta Keys
					if (code == 308 || code == 307 || code == 306 || 
					    code == 393 || code == 396 || code == 394)
					{
						data.mkey |= button.m_outKeyCode;
					}
					else
					{
						// Led Keys
						if (code == 364) data.led ^= CELL_KB_LED_NUM_LOCK;
						if (code == 311) data.led ^= CELL_KB_LED_CAPS_LOCK;
						if (code == 365) data.led ^= CELL_KB_LED_SCROLL_LOCK;

						u16 kcode;
						if (config.code_type == CELL_KB_CODETYPE_RAW)
						{
							kcode = button.m_outKeyCode;
						}
						else //config.code_type == CELL_KB_CODETYPE_ASCII
						{
							kcode =  cellKbCnvRawCode(config.arrange, data.mkey, data.led, button.m_outKeyCode);
						}
						data.keycode[data.len % CELL_KB_MAX_KEYCODES] = kcode;
						data.len++;
					}
				}

				if (!pressed)
				{
					// Meta Keys
					if (code == 308 || code == 307 || code == 306 || 
					    code == 393 || code == 396 || code == 394)
					{
						data.mkey &= ~button.m_outKeyCode;
					}
				}

			}
		}
	}

	KbInfo& GetInfo() { return m_info; }
	std::vector<Keyboard>& GetKeyboards() { return m_keyboards; }
	std::vector<KbButton>& GetButtons(const u32 keyboard) { return m_keyboards[keyboard].m_buttons; }
	CellKbData& GetData(const u32 keyboard) { return m_keyboards[keyboard].m_data; }
	CellKbConfig& GetConfig(const u32 keyboard) { return m_keyboards[keyboard].m_config; }
};