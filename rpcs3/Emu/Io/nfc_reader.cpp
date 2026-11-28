#include "stdafx.h"
#include "nfc_reader.h"
#include "buzzer_player.h"
#include "Utilities/StrUtil.h"

LOG_CHANNEL(nfc_log, "NFC");

nfc_reader::nfc_reader()
	: m_buzzer(std::make_unique<buzzer_player>())
{
	m_tx_buffer.reserve(300);
	m_exec_response.reserve(300);
	init_registers();
}

nfc_reader::~nfc_reader()
{
	stop_worker();
	if (m_buzzer)
	{
		m_buzzer->stop();
	}
}

void nfc_reader::set_device_name(const std::string& name)
{
	{
		std::lock_guard lock(m_device_mutex);
		if (m_device_name == name)
		{
			return;
		}
	}

	stop_worker();

	std::lock_guard exec_lock(m_exec_mutex);
	{
		std::lock_guard dev_lock(m_device_mutex);
		if (m_device_name == name)
		{
			return;
		}
		// Release the previous physical claim before re-opening a reader
		m_pcsc_active.store(false, std::memory_order_release);
		if (m_pcsc)
		{
			m_pcsc->close();
			m_pcsc.reset();
		}
		m_device_name = std::string(pcsc_driver::fake_reader_name);
	}

	if (name.empty() || name == pcsc_driver::fake_reader_name)
	{
		std::lock_guard dev_lock(m_device_mutex);
		m_device_name = name;
		return;
	}

	auto new_pcsc = std::make_shared<pcsc_driver>();
	if (!new_pcsc->open(name))
	{
		return;
	}

	std::lock_guard dev_lock(m_device_mutex);
	m_device_name = name;
	m_pcsc = std::move(new_pcsc);
	m_pcsc_active.store(true, std::memory_order_release);
}

std::shared_ptr<pcsc_driver> nfc_reader::get_pcsc() const
{
	std::lock_guard lock(m_device_mutex);
	return m_pcsc;
}

bool nfc_reader::fallback_to_fake_if_disconnected()
{
	if (!m_pcsc_active.load(std::memory_order_acquire))
	{
		return false;
	}

	const auto pcsc = get_pcsc();
	// Lock-free check: pcsc_driver::is_open() reads an atomic bool, so this never blocks on PC/SC I/O
	if (pcsc && pcsc->is_open())
	{
		return true;
	}

	// Reader was disconnected asynchronously; re-validate under m_device_mutex so concurrent set_device_name() cannot be overwritten
	std::lock_guard lock(m_device_mutex);
	if (m_pcsc == pcsc && (!m_pcsc || !m_pcsc->is_open()))
	{
		m_pcsc_active.store(false, std::memory_order_release);
		m_device_name = pcsc_driver::fake_reader_name;
	}
	return m_pcsc_active.load(std::memory_order_acquire);
}

std::string nfc_reader::get_device_name() const
{
	std::lock_guard lock(m_device_mutex);
	return m_device_name;
}

namespace
{
	// Parses PN532 InListPassiveTarget InitiatorData UID (stripping Cascade Tag 0x88 per UM0701-02 §7.3.5)
	// and checks whether the target UID matches it as a prefix.
	bool matches_type_a_uid_filter(std::span<const u8> init_data, std::span<const u8> uid)
	{
		if (init_data.empty())
		{
			return true;
		}

		std::array<u8, 10> req_uid{};
		usz req_len = 0;
		if (init_data.size() == 8 && init_data[0] == 0x88)
		{
			// Cascade Level 2: strip leading CT tag (byte 0)
			std::memcpy(req_uid.data(), init_data.data() + 1, 7);
			req_len = 7;
		}
		else if (init_data.size() == 12 && init_data[0] == 0x88 && init_data[4] == 0x88)
		{
			// Cascade Level 3: strip CT tags at byte 0 and byte 4
			std::memcpy(req_uid.data(), init_data.data() + 1, 3);
			std::memcpy(req_uid.data() + 3, init_data.data() + 5, 7);
			req_len = 10;
		}
		else
		{
			// Cascade Level 1 (4-byte UID) or partial UID prefix
			req_len = std::min(init_data.size(), req_uid.size());
			std::memcpy(req_uid.data(), init_data.data(), req_len);
		}

		if (req_len == 0)
		{
			return true;
		}
		if (uid.size() < req_len)
		{
			return false;
		}
		return std::memcmp(uid.data(), req_uid.data(), req_len) == 0;
	}

	// Appends ISO/IEC 14443-4 Type A ATS bytes ([TL] [T0] ...) to a PN532 TargetData buffer.
	void append_iso14443_4_ats(std::vector<u8>& resp, const pcsc_card_info& card_info)
	{
		if (card_info.ats.size() >= 2 && card_info.ats[0] == card_info.ats.size())
		{
			resp.insert(resp.end(), card_info.ats.begin(), card_info.ats.end());
			return;
		}

		if (!card_info.historical_bytes.empty())
		{
			// T0 high nibble = 0 (no TA1/TB1/TC1), low nibble = FSCI = 8 (FSC = 256 bytes) per ISO/IEC 14443-4 §5.2.2
			const usz hist_len = std::min<usz>(card_info.historical_bytes.size(), 253);
			resp.push_back(static_cast<u8>(2 + hist_len));
			resp.push_back(0x08);
			resp.insert(resp.end(), card_info.historical_bytes.begin(), card_info.historical_bytes.begin() + hist_len);
			return;
		}

		const std::array<u8, 6> default_ats = {0x06, 0x75, 0x77, 0x81, 0x02, 0x80};
		resp.insert(resp.end(), default_ats.begin(), default_ats.end());
	}

	// Appends ISO/IEC 14443-3B TargetData ([Tg] [ATQB(12B)] [ATTRIB_RES len] [ATTRIB_RES]) per UM0701-02 §7.3.5.
	bool append_type_b_target(std::vector<u8>& resp, u8 tg, const pcsc_card_info& card_info)
	{
		if (card_info.uid.size() < 4)
		{
			return false;
		}

		resp.push_back(tg);
		resp.push_back(0x50); // ATQB header
		resp.insert(resp.end(), card_info.uid.begin(), card_info.uid.begin() + 4);
		if (card_info.historical_bytes.size() >= 7)
		{
			// PC/SC Part 3 Type B ATR historical bytes: 4B Application Data + 3B Protocol Info (+ optional 1B MBLI/CID)
			resp.insert(resp.end(), card_info.historical_bytes.begin(), card_info.historical_bytes.begin() + 7);
		}
		else
		{
			resp.insert(resp.end(), {0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x71});
		}
		resp.push_back(0x01); // ATTRIB_RES length
		resp.push_back(card_info.historical_bytes.size() >= 8 ? card_info.historical_bytes[7] : 0x01);
		return true;
	}

	constexpr std::array<u8, 8> default_felica_pmm = {0x03, 0x00, 0x4B, 0x02, 0x4F, 0x49, 0x8A, 0x8A};

	// Canonicalizes dual-mapped CIU Page 0 SFRs (0xFFD1..0xFFEF) to XRAM addresses (0x6331..0x633E).
	constexpr u16 canonicalize_register_address(u16 address)
	{
		// SFR D1h..D6h (CIU_Command .. CIU_Error) <-> XRAM 6331h..6336h
		if (address >= 0xFFD1 && address <= 0xFFD6)
		{
			return 0x6330 + (address - 0xFFD0);
		}

		// SFR DFh (CIU_Status1) <-> XRAM 6337h
		if (address == 0xFFDF)
		{
			return 0x6337;
		}

		// SFR E9h..EFh (CIU_Status2 .. CIU_Coll) <-> XRAM 6338h..633Eh
		if (address >= 0xFFE9 && address <= 0xFFEF)
		{
			return 0x6330 + (address - 0xFFE1);
		}

		return address;
	}

	// Validates whether an SFR address (0xFF00..0xFFFF) is an accessible SFR according to
	// UM0701-02 Table 14 (List of SFR registers) and PN532/C1 Table 289.
	// Unrecognized SFR addresses result in a Syntax Error frame according to UM0701-02 §7.2.4 & §7.2.5.
	constexpr bool is_valid_sfr_address(u8 sfr_offset)
	{
		switch (sfr_offset)
		{
		// UM0701-02 Table 14 accessible SFRs
		case 0x87: // PCON
		case 0x9A: // RWL
		case 0x9B: // TWL
		case 0x9C: // FIFOFS
		case 0x9D: // FIFOFF
		case 0x9E: // SFF
		case 0x9F: // FIT
		case 0xA1: // FITEN
		case 0xA2: // FDATA
		case 0xA3: // FSIZE
		case 0xA8: // IE0
		case 0xA9: // SPIcontrol
		case 0xAA: // SPIstatus
		case 0xAB: // HSU_STA
		case 0xAC: // HSU_CTR
		case 0xAD: // HSU_PRE
		case 0xAE: // HSU_CNT
		case 0xB0: // P3
		case 0xB8: // IP0
		case 0xE8: // IEN1 / IE1
		case 0xF4: // P7CFGA
		case 0xF5: // P7CFGB
		case 0xF7: // P7
		case 0xF8: // IP1
		case 0xFC: // P3CFGA
		case 0xFD: // P3CFGB
		// Dual-mapped CIU Page 0 SFRs (PN532/C1 Table 289: D1..D6, DF, E9..EF)
		case 0xD1: // CIU_Command
		case 0xD2: // CIU_CommIEn
		case 0xD3: // CIU_DivIEn
		case 0xD4: // CIU_CommIrq
		case 0xD5: // CIU_DivIrq
		case 0xD6: // CIU_Error
		case 0xDF: // CIU_Status1
		case 0xE9: // CIU_Status2
		case 0xEA: // CIU_FIFOData
		case 0xEB: // CIU_FIFOLevel
		case 0xEC: // CIU_WaterLevel
		case 0xED: // CIU_Control
		case 0xEE: // CIU_BitFraming
		case 0xEF: // CIU_Coll
		// I2C peripheral SFRs (PN532/C1 Table 289: D8..DB)
		case 0xD8: // I2CCON
		case 0xD9: // I2CSTA
		case 0xDA: // I2CDAT
		case 0xDB: // I2CADR
			return true;
		default:
			return false;
		}
	}

	constexpr bool is_valid_register_address(u16 address)
	{
		if ((address & 0xFF00) == 0xFF00)
		{
			return is_valid_sfr_address(static_cast<u8>(address & 0xFF));
		}

		return true;
	}
}

void nfc_reader::init_registers()
{
	m_registers.clear();

	// 80C51 Special Function Registers (SFR: 0xFF80..0xFFFF)
	m_registers[0xFF87] = 0x00;       // PCON: Power Control Register
	m_registers[0xFF9A] = 0x10;       // RWL: Reception Water Level (UM0701-02 default 0x10)
	m_registers[0xFF9B] = 0x10;       // TWL: Transmission Water Level (UM0701-02 default 0x10)
	m_registers[0xFF9C] = 0x00;       // FIFOFS: Transmit FIFO Freespace
	m_registers[0xFF9D] = 0x00;       // FIFOFF: Receive FIFO Fullness
	m_registers[0xFF9E] = 0x29;       // SFF: Serial Interface FIFO Flags (FIFO_EN=0, TWLL=1, RWLH=1, RFE=1)
	m_registers[0xFF9F] = 0x00;       // FIT: FIFO Interrupt Request Flags
	m_registers[0xFFA1] = 0x00;       // FITEN: FIFO Interrupt Enable
	m_registers[0xFFA2] = 0x00;       // FDATA: FIFO Data In/Out
	m_registers[0xFFA3] = 0x00;       // FSIZE: FIFO Receive Size
	m_registers[0xFFA8] = 0x00;       // IE0: Interrupt Enable 0
	m_registers[0xFFA9] = 0x00;       // SPIcontrol: SPI Control Register
	m_registers[0xFFAA] = 0x00;       // SPIstatus: SPI Status Register
	m_registers[0xFFAB] = 0x00;       // HSU_STA: High Speed UART Status
	m_registers[0xFFAC] = 0x01;       // HSU_CTR: High Speed UART Control (soft_reset_n=1)
	m_registers[0xFFAD] = 0x18;       // HSU_PRE: High Speed UART Prescaler
	m_registers[0xFFAE] = 0x70;       // HSU_CNT: High Speed UART Counter
	m_registers[0xFFB0] = 0xC0 | m_gpio_p3;  // P3: Port 3 GPIO Data (Hardware POR: 0xFF, bits 7..6 reserved 1, bits 5..0 GPIO)
	m_registers[0xFFB8] = 0x00;       // IP0: Interrupt Priority 0
	m_registers[0xFFD8] = 0x00;       // I2CCON: I2C Control
	m_registers[0xFFD9] = 0xF8;       // I2CSTA: I2C Status (No relevant state)
	m_registers[0xFFDA] = 0x00;       // I2CDAT: I2C Data
	m_registers[0xFFDB] = 0x00;       // I2CADR: I2C Address
	m_registers[0xFFE8] = 0x00;       // IE1: Interrupt Enable 1
	m_registers[0xFFF4] = 0xFF;       // P7CFGA: Port 7 Output Configuration A (Reset default: 0xFF; bits[7:3] reserved read-as-1 | bits[2:0]=0x07 quasi-bidir; per PN532_C1 Table 60)
	m_registers[0xFFF5] = 0x00;       // P7CFGB: Port 7 Output Configuration B (Quasi-bidirectional mode reset default: 0x00 per Table 62)
	m_registers[0xFFF7] = 0xF8 | m_gpio_p7;  // P7: Port 7 GPIO Data (Hardware POR: 0xFF, bits 7..3 reserved 1, bits 2..1 GPIO, bit 0 IRQ)
	m_registers[0xFFF8] = 0x00;       // IP1: Interrupt Priority 1
	m_registers[0xFFFC] = 0xFF;       // P3CFGA: Port 3 Output Configuration A (Reset default: 0xFF; bits[7:6] reserved read-as-1 | bits[5:0]=0x3F quasi-bidir; per PN532_C1 Table 66)
	m_registers[0xFFFD] = 0x00;       // P3CFGB: Port 3 Output Configuration B (Quasi-bidirectional mode reset default: 0x00 per Table 68)

	// System / PCR / Peripheral Registers in XRAM (0x6100..0x6205)
	m_registers[0x6103] = ((m_gpio_i0i1 & 0x02) << 4) | ((m_gpio_i0i1 & 0x01) << 3); // Config I0_I1
	m_registers[0x6104] = 0x00; // Observe_testbus: Test bus observation control
	m_registers[0x6105] = 0x00; // Data_rng: Random number output register
	m_registers[0x6106] = 0x41; // Control_switch_rng: Random Number Generator control and power switch
	m_registers[0x6107] = 0x00; // GPIRQ: General Purpose Interrupt Configuration
	m_registers[0x6109] = 0x04; // LDO: Voltage regulator control and overcurrent status
	m_registers[0x610A] = 0x00; // i2c_wu_control: I2C Wakeup Control
	m_registers[0x610C] = 0x00; // Andet_control: Analog detector control
	m_registers[0x610E] = 0x00; // NFC_WI_control: NFC-WI (S2C) interface control
	m_registers[0x6200] = 0x02; // PCR CFR: Power, Clock & Reset - CPU Frequency control (0x02 = 27.12 MHz)
	m_registers[0x6201] = 0x0E; // PCR CER: Clock Enable Register (HSU, CIU, timers enabled)
	m_registers[0x6202] = 0x40; // PCR ILR: Interrupt & Latched Reset control (POR pulse latched)
	m_registers[0x6203] = 0xC0; // PCR Control: Power Control & Software Reset
	m_registers[0x6204] = 0x00; // PCR Status: Wakeup source status flags
	m_registers[0x6205] = 0x00; // PCR Wakeupen: Wakeup source interrupt enable

	// Contactless Interface Unit (CIU) Registers in XRAM (0x6301..0x633E)
	// CIU Page 1: Communication & Framing Control (0x6301..0x630E)
	m_registers[0x6301] = 0x3B; // CIU_Mode: General transmitting/receiving modes (CRCPreset: 6363h, PolSigin=1)
	m_registers[0x6302] = 0x00; // CIU_TxMode: Transmission data rate, parity and framing
	m_registers[0x6303] = 0x00; // CIU_RxMode: Reception data rate, parity and framing
	m_registers[0x6304] = 0x80; // CIU_TxControl: Logical behavior of antenna driver TX1 and TX2 (InvTx2RFon=1)
	m_registers[0x6305] = 0x00; // CIU_TxAuto: Antenna driver automated controls (100% ASK, AutoWakeUp)
	m_registers[0x6306] = 0x10; // CIU_TxSel: Transmitter analog source selection (DriverSel=01)
	m_registers[0x6307] = 0x84; // CIU_RxSel: Receiver demodulator settings and wait time (RxWait=4, UartSel=10)
	m_registers[0x6308] = 0x84; // CIU_RxThreshold: Bit decoder minimum level and collision threshold (MinLevel=8, Collevel=4)
	m_registers[0x6309] = 0x4D; // CIU_Demod: Demodulator time constants and phase settings
	m_registers[0x630A] = 0x00; // CIU_FelNFC1: FeliCa sync-byte length and minimum packet length
	m_registers[0x630B] = 0x00; // CIU_FelNFC2: FeliCa maximum packet length and time slots
	m_registers[0x630C] = 0x62; // CIU_MifNFC: ISO/IEC 14443A/Mifare framing & Miller sensitivity (SensMiller=3, TxWait=2)
	m_registers[0x630D] = 0x00; // CIU_ManualRCV: Receiver analog fine-tuning and HPCF filter selection
	m_registers[0x630E] = 0x00; // CIU_TypeB: ISO/IEC 14443B specific framing, SOF/EOF and EGT controls

	// CIU Page 2: Configuration, CRC & Timers (0x6311..0x631F)
	m_registers[0x6311] = 0xFF; // CIU_CRCResultMSB: Actual MSB of CRC coprocessor calculation
	m_registers[0x6312] = 0xFF; // CIU_CRCResultLSB: Actual LSB of CRC coprocessor calculation
	m_registers[0x6313] = 0x88; // CIU_GsNOff: Conductance of N-driver when carrier is OFF
	m_registers[0x6314] = 0x26; // CIU_ModWidth: Miller modulation pulse width setting
	m_registers[0x6315] = 0x87; // CIU_TxBitPhase: Bit-grid phase fine-tuning and clock change
	m_registers[0x6316] = 0x48; // CIU_RFCfg: Sensitivity and receiver gain (RxGain=4: 33dB typical)
	m_registers[0x6317] = 0x88; // CIU_GsNOn: Conductance of N-driver when carrier is ON
	m_registers[0x6318] = 0x20; // CIU_CWGsP: Conductance of P-driver during continuous carrier wave
	m_registers[0x6319] = 0x20; // CIU_ModGsP: Conductance of P-driver during modulation
	m_registers[0x631A] = 0x00; // CIU_TMode: Internal timer control (prescaler high bits, auto-restart)
	m_registers[0x631B] = 0x00; // CIU_TPrescaler: Internal timer prescaler lower 8 bits
	m_registers[0x631C] = 0x00; // CIU_TReloadVal_Hi: Timer reload value high byte
	m_registers[0x631D] = 0x00; // CIU_TReloadVal_Lo: Timer reload value low byte
	m_registers[0x631E] = 0x00; // CIU_TCounterVal_hi: Timer counter current value high byte
	m_registers[0x631F] = 0x00; // CIU_TCounterVal_lo: Timer counter current value low byte

	// CIU Page 3: Test, Version & Analog Observation (0x6321..0x6330)
	m_registers[0x6321] = 0x00; // CIU_TestSel1: General test signal and SIC clock selection
	m_registers[0x6322] = 0x00; // CIU_TestSel2: PRBS generator configuration and test bus routing
	m_registers[0x6323] = 0x00; // CIU_TestPinEn: Test pin output enable (D1-D7)
	m_registers[0x6324] = 0x00; // CIU_TestPinValue: Test pin output levels
	m_registers[0x6325] = 0x00; // CIU_TestBus: Internal test bus observation port
	m_registers[0x6326] = 0x40; // CIU_AutoTest: Digital self-test & non-linear receiver processing (AmpRcv=1)
	m_registers[0x6327] = 0x80; // CIU_Version: CIU Silicon revision (Product=1000b for PN532, Version=0000b)
	m_registers[0x6328] = 0x00; // CIU_AnalogTest: Analog pin AUX1/AUX2 observation controls
	m_registers[0x6329] = 0x00; // CIU_TestDAC1: Test value for internal DAC1
	m_registers[0x632A] = 0x00; // CIU_TestDAC2: Test value for internal DAC2
	m_registers[0x632B] = 0x00; // CIU_TestADC: Actual digitized value of ADC I and Q channels
	m_registers[0x632F] = 0x00; // CIU_RFlevelDet: RF level detector power-down and sensitivity
	m_registers[0x6330] = 0x00; // SIC_CLK: S2C interface clock control and test bus enable

	// CIU Page 0 in XRAM: Dual-mapped to 80C51 SFRs (0x6331..0x633E <-> 0xFFD1..0xFFEF)
	m_registers[0x6331] = 0x20; // CIU_Command (Dual-mapped: SFR D1h / 0xFFD1): Command execution register (Default: Idle)
	m_registers[0x6332] = 0x00; // CIU_CommIEn (Dual-mapped: SFR D2h / 0xFFD2): Common interrupt enable
	m_registers[0x6333] = 0x00; // CIU_DivIEn (Dual-mapped: SFR D3h / 0xFFD3): Diversity interrupt enable
	m_registers[0x6334] = 0x14; // CIU_CommIrq (Dual-mapped: SFR D4h / 0xFFD4): Common interrupt request flags (IdleIrq | ErrIrq)
	m_registers[0x6335] = 0x00; // CIU_DivIrq (Dual-mapped: SFR D5h / 0xFFD5): Diversity interrupt request flags
	m_registers[0x6336] = 0x00; // CIU_Error (Dual-mapped: SFR D6h / 0xFFD6): Error status of the last executed command
	m_registers[0x6337] = 0x21; // CIU_Status1 (Dual-mapped: SFR DFh / 0xFFDF): CRC, interrupt and FIFO status (CRCReady | LoAlert)
	m_registers[0x6338] = 0x00; // CIU_Status2 (Dual-mapped: SFR E9h / 0xFFE9): Transmitter/receiver modem state machine and Crypto1 status
	m_registers[0x6339] = 0x00; // CIU_FIFOData (Dual-mapped: SFR EAh / 0xFFEA): 64-byte FIFO buffer input/output port
	m_registers[0x633A] = 0x00; // CIU_FIFOLevel (Dual-mapped: SFR EBh / 0xFFEB): Number of bytes stored in CIU FIFO
	m_registers[0x633B] = 0x08; // CIU_WaterLevel (Dual-mapped: SFR ECh / 0xFFEC): FIFO underflow/overflow warning threshold
	m_registers[0x633C] = 0x00; // CIU_Control (Dual-mapped: SFR EDh / 0xFFED): Timer manual start/stop, Initiator/Target mode
	m_registers[0x633D] = 0x00; // CIU_BitFraming (Dual-mapped: SFR EEh / 0xFFEE): Bit-oriented frame transmission and alignment
	m_registers[0x633E] = 0xA0; // CIU_Coll (Dual-mapped: SFR EFh / 0xFFEF): Bit collision position flags (ValuesAfterColl | CollPosNotValid)

	// Firmware XRAM RF Configuration & Analog Calibration Tables (0xA0xx, UM0701-02 Table 19 & 20)
	// 106 kbps Type A profile (0xA010..0xA01A)
	m_registers[0xA010] = 0xDC; // CIU_RFCfg: Receiver sensitivity and analog front-end gain (106 kbps Type A)
	m_registers[0xA011] = 0xF4; // CIU_GsNOn: Transmitter N-channel conductance when carrier is ON (106 kbps Type A)
	m_registers[0xA012] = 0x3F; // CIU_CWGsP: Transmitter P-channel conductance for continuous carrier wave (106 kbps Type A)
	m_registers[0xA013] = 0x11; // CIU_ModGsP: Transmitter P-channel conductance during modulation (106 kbps Type A)
	m_registers[0xA014] = 0x4D; // CIU_Demod: Demodulator settings when internal RF field is active (106 kbps Type A)
	m_registers[0xA015] = 0x85; // CIU_RxThreshold: Receiver bit-decoder signal threshold / min level (106 kbps Type A)
	m_registers[0xA016] = 0x61; // CIU_Demod: Demodulator settings when internal RF field is OFF / external field active
	m_registers[0xA017] = 0xF1; // CIU_GsNOff: Transmitter N-channel conductance when carrier is OFF (106 kbps Type A)
	m_registers[0xA018] = 0x26; // CIU_ModWidth: Transmitter Miller modulation pulse width (106 kbps Type A)
	m_registers[0xA019] = 0x6A; // CIU_MifNFC: Mifare/NFC collision resolution and framing control (106 kbps Type A)
	m_registers[0xA01A] = 0x87; // CIU_TxBitPhase: Transmitter bit-grid phase fine-tuning (106 kbps Type A)

	// 212 kbps profile (Table 20: 8 bytes, 0xA01B..0xA022)
	m_registers[0xA01B] = 0x69; // CIU_RFCfg: Receiver sensitivity and analog front-end gain (212 kbps)
	m_registers[0xA01C] = 0xFF; // CIU_GsNOn: Transmitter N-channel conductance when carrier is ON (212 kbps)
	m_registers[0xA01D] = 0x3F; // CIU_CWGsP: Transmitter P-channel continuous wave conductance (212 kbps)
	m_registers[0xA01E] = 0x0E; // CIU_ModGsP: Transmitter P-channel modulation conductance (212 kbps Manchester)
	m_registers[0xA01F] = 0x41; // CIU_Demod: Demodulator settings when internal RF field is active (212 kbps)
	m_registers[0xA020] = 0x85; // CIU_RxThreshold: Receiver bit-decoder signal threshold / min level (212 kbps)
	m_registers[0xA021] = 0x61; // CIU_Demod: Demodulator settings when internal RF field is OFF / external field active
	m_registers[0xA022] = 0xF1; // CIU_GsNOff: Transmitter N-channel conductance when carrier is OFF (212 kbps)

	// 424 kbps profile driver conductance cache (0xA0CB..0xA0CE)
	m_registers[0xA0CB] = 0xFF; // CIU_GsNOn: Transmitter N-channel conductance when carrier is ON (424 kbps)
	m_registers[0xA0CC] = 0x3F; // CIU_CWGsP: Transmitter P-channel continuous wave conductance (424 kbps)
	m_registers[0xA0CD] = 0x0E; // CIU_ModGsP: Transmitter P-channel modulation conductance (424 kbps Manchester)
	m_registers[0xA0CE] = 0xF1; // CIU_GsNOff: Transmitter N-channel conductance when carrier is OFF (424 kbps)
}

u8 nfc_reader::read_register_value(u16 address) const
{
	const u16 canonical_addr = canonicalize_register_address(address);
	const auto it = m_registers.find(canonical_addr);
	if (it != m_registers.end())
	{
		u8 val = it->second;
		switch (canonical_addr)
		{
		case 0x6103:
			// Bits 5 (pad_I1) and 3 (pad_I0) reflect hardware interface pins
			val = (val & ~0x28) | ((m_gpio_i0i1 & 0x02) << 4) | ((m_gpio_i0i1 & 0x01) << 3);
			break;
		case 0x6304:
			// Bit 2: CheckRF is write-only, always reads 0 (PN532/C1 Table 210)
			val &= ~0x04;
			break;
		case 0x6305:
			// Bit 2: InitialRFOn is write-only, always reads 0 (PN532/C1 Table 212)
			val &= ~0x04;
			break;
		case 0x6334:
			// Bit 7: Set1 is write-only, always reads 0 (PN532/C1 Table 184)
			val &= 0x7F;
			break;
		case 0x6335:
			// Bit 7: Set2 is write-only, always reads 0 (PN532/C1 Table 186)
			val &= 0x7F;
			break;
		case 0x633A:
			// Bit 7: FlushBuffer is write-only, always reads 0 (PN532/C1 Table 196)
			val &= 0x7F;
			break;
		case 0x633C:
			// Bits 7 (TStopNow) and 6 (TStartNow) are write-only, always read 0 (PN532/C1 Table 200)
			val &= 0x3F;
			break;
		case 0x633D:
			// Bit 7: StartSend is write-only, always reads 0 (PN532/C1 Table 202)
			val &= 0x7F;
			break;
		case 0xFF9F:
			// Bit 7: Reset is write-only, always reads 0 (PN532/C1 Table 289)
			val &= 0x7F;
			break;
		default:
			break;
		}
		return val;
	}

	return 0x00;
}

void nfc_reader::write_register_value(u16 address, u8 value)
{
	const u16 canonical_addr = canonicalize_register_address(address);

	// Discard write attempts to read-only hardware registers (PN532/C1 Table 288 & 289)
	switch (canonical_addr)
	{
	case 0x6105: // Data_rng (R: Random number output register)
	case 0x6204: // PCR Status (R: Wakeup source status flags)
	case 0x6311: // CIU_CRCResultMSB (R: CRC coprocessor MSB calculation result)
	case 0x6312: // CIU_CRCResultLSB (R: CRC coprocessor LSB calculation result)
	case 0x631E: // CIU_TCounterVal_hi (R: Timer counter current value high byte)
	case 0x631F: // CIU_TCounterVal_lo (R: Timer counter current value low byte)
	case 0x6325: // CIU_TestBus (R: Internal test bus observation port)
	case 0x6327: // CIU_Version (R: CIU Silicon revision)
	case 0x632B: // CIU_TestADC (R: ADC I/Q digitized value)
	case 0x6336: // CIU_Error / SFR D6h (R: Error status of the last executed command)
	case 0x6337: // CIU_Status1 / SFR DFh (R: CRC, interrupt and FIFO status)
	case 0xFF9C: // FIFOFS (R: Transmit FIFO Freespace)
	case 0xFF9D: // FIFOFF (R: Receive FIFO Fullness)
	case 0xFFAA: // SPIstatus (R: SPI status flags)
	case 0xFFD9: // I2CSTA (R: I2C status register)
		return;
	default:
		break;
	}

	// Write-mask and field protection for partially writable registers
	switch (canonical_addr)
	{
	case 0x6103: // Config I0_I1: pad_I1 (bit 5) & pad_I0 (bit 3) are read-only pin states
	{
		const u8 pad_bits = ((m_gpio_i0i1 & 0x02) << 4) | ((m_gpio_i0i1 & 0x01) << 3);
		value = (value & 0x87) | pad_bits;
		break;
	}
	case 0x6104: // Observe_testbus: only bit 0 (observe_ciu) is writable
	{
		value &= 0x01;
		break;
	}
	case 0x6106: // Control_switch_rng: bits 6, 4, 2, 1 writable (0x56); bit 5 sic_switch_overload is R
	{
		value = (m_registers[0x6106] & ~0x56) | (value & 0x56);
		break;
	}
	case 0x6109: // LDO: bits 4..0 are writable (0x1F), bit 5 is read-only overcurrent status
	{
		value = (m_registers[0x6109] & ~0x1F) | (value & 0x1F);
		break;
	}
	case 0x610A: // i2c_wu_control: bits 2..0 are writable (0x07)
	{
		value &= 0x07;
		break;
	}
	case 0x610C: // Andet_control: bits 5..0 are writable (0x3F), bits 7..6 are read-only
	{
		value = (m_registers[0x610C] & ~0x3F) | (value & 0x3F);
		break;
	}
	case 0x610E: // NFC_WI_control: bits 1..0 are writable (0x03), bit 3 is read-only nfc_wi_status
	{
		value = (m_registers[0x610E] & ~0x03) | (value & 0x03);
		break;
	}
	case 0x6200: // PCR CFR: bits 1..0 cpu_freq (0x03)
	{
		value &= 0x03;
		break;
	}
	case 0x6201: // PCR CER: bit 3 hsu_enable (0x08)
	{
		value = (m_registers[0x6201] & ~0x08) | (value & 0x08);
		break;
	}
	case 0x6202: // PCR ILR: bit 6 porpulse_latched is clear-only by writing 0; bits 4, 2..0 writable (0x17)
	{
		const u8 current = m_registers[0x6202];
		const u8 bit6 = (value & 0x40) ? (current & 0x40) : 0x00;
		value = bit6 | (value & 0x17);
		break;
	}
	case 0x6203: // PCR Control: bits 7..6 reserved (0xC0), bits 1..0 writable (0x03)
	{
		value = 0xC0 | (value & 0x03);
		break;
	}
	case 0x6205: // PCR Wakeupen: bit 2 reserved (0xFB)
	{
		value &= 0xFB;
		break;
	}
	case 0x6308: // CIU_RxThreshold: bit 3 reserved (0xF7)
	{
		value &= 0xF7;
		break;
	}
	case 0x630D: // CIU_ManualRCV: bit 7 reserved (0x7F)
	{
		value &= 0x7F;
		break;
	}
	case 0x630E: // CIU_TypeB: bit 5 reserved (0xDF)
	{
		value &= 0xDF;
		break;
	}
	case 0x6318: // CIU_CWGsP: bits 5..0 writable (0x3F)
	case 0x6319: // CIU_ModGsP: bits 5..0 writable (0x3F)
	{
		value &= 0x3F;
		break;
	}
	case 0x6326: // CIU_AutoTest: bits 3..0 writable (0x0F), bit 6 AmpRcv preserved
	{
		value = (m_registers[0x6326] & ~0x0F) | (value & 0x0F);
		break;
	}
	case 0x6329: // CIU_TestDAC1: bits 5..0 writable (0x3F)
	case 0x632A: // CIU_TestDAC2: bits 5..0 writable (0x3F)
	{
		value &= 0x3F;
		break;
	}
	case 0x632F: // CIU_RFlevelDet: bit 4 pd_rfleveldet writable (0x10)
	{
		value &= 0x10;
		break;
	}
	case 0x6330: // CIU_SIC_CLK_en: bits 7, 3..0 writable (0x8F)
	{
		value &= 0x8F;
		break;
	}
	case 0x6331: // CIU_Command: bits 5..0 writable (0x3F)
	{
		value &= 0x3F;
		break;
	}
	case 0x6332: // CIU_CommIEn: bits 6..0 writable (0x7F)
	{
		value &= 0x7F;
		break;
	}
	case 0x6333: // CIU_DivIEn: bits 4..0 writable (0x1F)
	{
		value &= 0x1F;
		break;
	}
	case 0x6334: // CIU_CommIrq / SFR D4h: bit 7 is Set1 (1: set bits, 0: clear bits)
	{
		if (value & 0x80)
			value = m_registers[0x6334] | (value & 0x7F);
		else
			value = m_registers[0x6334] & ~(value & 0x7F);
		break;
	}
	case 0x6335: // CIU_DivIrq / SFR D5h: bit 7 is Set2 (1: set bits, 0: clear bits)
	{
		if (value & 0x80)
			value = m_registers[0x6335] | (value & 0x1F);
		else
			value = m_registers[0x6335] & ~(value & 0x1F);
		break;
	}
	case 0x6338: // CIU_Status2 / SFR E9h: bit 7 TempSensClear is writable, bits 5..0 are read-only
	{
		value = (m_registers[0x6338] & 0x7F) | (value & 0x80);
		break;
	}
	case 0x633A: // CIU_FIFOLevel / SFR EBh: bit 7 FlushBuffer flushes FIFO, bits 6..0 read-only
	{
		if (value & 0x80)
			value = 0x00;
		else
			return;
		break;
	}
	case 0x633B: // CIU_WaterLevel / SFR ECh: bits 5..0 writable (0x3F)
	{
		value &= 0x3F;
		break;
	}
	case 0x633C: // CIU_Control / SFR EDh: bits 2..0 (RxLastBits) are read-only
	{
		value = (m_registers[0x633C] & 0x07) | (value & 0xF0);
		break;
	}
	case 0x633D: // CIU_BitFraming / SFR EEh: bit 3 reserved (0xF7)
	{
		value &= 0xF7;
		break;
	}
	case 0x633E: // CIU_Coll / SFR EFh: bit 7 ValuesAfterColl is writable, bits 5..0 are read-only
	{
		value = (m_registers[0x633E] & 0x7F) | (value & 0x80);
		break;
	}
	case 0xFF9E: // SFF: bit 7 FIFO_EN is writable, bits 6..0 are read-only flags
	{
		value = (m_registers[0xFF9E] & 0x7F) | (value & 0x80);
		break;
	}
	case 0xFF9F: // FIT: bit 7 Reset flushes flags
	{
		value = (value & 0x80) ? 0x00 : (value & 0x3F);
		break;
	}
	case 0xFFAB: // HSU_STA: bit 7 set_bit (1: set bits, 0: clear bits)
	{
		if (value & 0x80)
			value = m_registers[0xFFAB] | (value & 0x7F);
		else
			value = m_registers[0xFFAB] & ~(value & 0x7F);
		break;
	}
	case 0xFFB0: // P3: bits 7..6 reserved (0xC0), bits 5..0 are GPIO
	{
		m_gpio_p3 = value & 0x3F;
		value = 0xC0 | m_gpio_p3;
		break;
	}
	case 0xFFF7: // P7: bits 7..3 reserved (0xF8), bits 2..0 are GPIO/IRQ
	{
		m_gpio_p7 = value & 0x06;
		value = 0xF8 | (m_registers[0xFFF7] & 0x01) | (value & 0x06);
		break;
	}
	case 0xFFF4: // P7CFGA: bits 7..3 reserved (0xF8)
	{
		value = 0xF8 | (value & 0x07);
		break;
	}
	case 0xFFF5: // P7CFGB: bits 7..3 reserved (0x00)
	{
		value &= 0x07;
		break;
	}
	case 0xFFFC: // P3CFGA: bits 7..6 reserved (0xC0)
	{
		value = 0xC0 | (value & 0x3F);
		break;
	}
	case 0xFFFD: // P3CFGB: bits 7..6 reserved (0x00)
	{
		value &= 0x3F;
		break;
	}
	default:
		break;
	}

	m_registers[canonical_addr] = value;
}

usz nfc_reader::get_buffer_size() const
{
	std::lock_guard lock(m_buffer_mutex);
	return m_tx_buffer.size() - m_tx_read_pos;
}

void nfc_reader::write_buffer(std::vector<u8>& out, usz max_size)
{
	std::lock_guard lock(m_buffer_mutex);
	const usz count = std::min(max_size, m_tx_buffer.size() - m_tx_read_pos);
	if (count == 0)
	{
		return;
	}

	out.insert(out.end(), m_tx_buffer.begin() + m_tx_read_pos, m_tx_buffer.begin() + m_tx_read_pos + count);
	m_tx_read_pos += count;
}

bool nfc_reader::verify_request(std::span<const u8> in)
{
	if (in.size() < ack_frame.size())
		return false;

	if (!std::equal(start_code.begin(), start_code.end(), in.begin()))
		return false;

	if (std::equal(ack_frame.begin(), ack_frame.end(), in.begin()) ||
		std::equal(nack_frame.begin(), nack_frame.end(), in.begin())) // Further checks are not applicable for ACK and NACK, so simply return true
		return true;

	if (in.size() < sizeof(frame_header))
		return false;

	usz tfi_offset = 0; // TFI is the last byte of either header
	usz len = 0;
	u8 len_checksum = 0;
	frame_identifier tfi{};

	if (is_extended_frame(in)) // Extended Information Frame
	{
		if (in.size() < sizeof(extended_frame_header))
			return false;

		const auto header = read_from_ptr<extended_frame_header>(in);
		tfi_offset = offsetof(extended_frame_header, tfi);
		len = header.len;
		len_checksum = static_cast<u8>((len >> 8) + (len & 0xFF) + header.lcs);
		tfi = header.tfi;
	}
	else // Normal Information Frame
	{
		const auto header = read_from_ptr<frame_header>(in);
		tfi_offset = offsetof(frame_header, tfi);
		len = header.len;
		len_checksum = static_cast<u8>(header.len + header.lcs);
		tfi = header.tfi;
	}

	// len covers TFI + data, followed by DCS
	if (len < 2 || len_checksum != 0x00 || tfi != frame_identifier::to_device || in.size() < tfi_offset + len + sizeof(frame_footer::dcs))
		return false;

	// TFI + data + DCS
	u8 data_checksum = 0;
	for (usz i = tfi_offset; i < tfi_offset + len + 1; i++)
		data_checksum += in[i];

	return data_checksum == 0x00;
}

void nfc_reader::build_response(std::span<const u8> in)
{
	auto& out = m_exec_response;
	const bool strip_ambles = remove_pre_and_postamble();
	const usz total_len = in.empty() ? 0 : (in.size() + 1); // Including TFI
	const usz frame_overhead = (total_len > 0xFF ? sizeof(extended_frame_header) : sizeof(frame_header)) + sizeof(frame_footer) + 2;
	out.reserve(out.size() + (in.empty() ? (error_frame.size() + 2) : (frame_overhead + in.size())));

	if (!strip_ambles)
		out.push_back(preamble);

	if (in.empty())
	{
		out.insert(out.end(), error_frame.begin(), error_frame.end());

		if (!strip_ambles)
			out.push_back(postamble);

		return;
	}

	const auto append = [&out](const auto& value)
	{
		const auto bytes = to_bytes(value);
		out.insert(out.end(), bytes.begin(), bytes.end());
	};

	if (total_len > 0xFF) // Extended Information Frame
	{
		extended_frame_header header{};
		header.len = static_cast<u16>(total_len);
		header.lcs = static_cast<u8>(0x00 - ((total_len >> 8) + (total_len & 0xFF)));
		append(header);
	}
	else // Normal Information Frame
	{
		frame_header header{};
		header.len = static_cast<u8>(total_len);
		header.lcs = static_cast<u8>(0x00 - total_len);
		append(header);
	}

	frame_footer footer{};
	footer.dcs = static_cast<u8>(0x00 - static_cast<u8>(frame_identifier::to_host));

	out.insert(out.end(), in.begin(), in.end());
	for (const u8 data : in)
	{
		footer.dcs -= data;
	}

	append(footer);

	if (!strip_ambles)
		out.push_back(postamble);
}

nfc_reader::card_type nfc_reader::get_card_type(std::span<const u8> card)
{
	// Currently we only emulate Mifare Classic cards, so the check can be simple here; maybe more card types can be supported in the future.
	if (card.size() >= mifare_uid_size)
		return card_type::mifare_classic;

	return card_type::unknown;
}

std::optional<std::array<u8, mifare_uid_size>> nfc_reader::get_card_uid(std::span<const u8> card) const
{
	if (card.size() < mifare_uid_size)
	{
		return std::nullopt;
	}

	std::array<u8, mifare_uid_size> uid{};
	std::memcpy(uid.data(), card.data(), mifare_uid_size);
	return uid;
}

std::span<const u8> nfc_reader::get_card_uid_bytes(std::span<const u8> card, std::array<u8, 7>& out_uid) const
{
	const auto card_t = get_card_type(card);
	if (card_t == card_type::mifare_ultralight)
	{
		// MIFARE Ultralight (MF0ICU1): 7-byte double size UID
		// Page 0 (bytes 0..2) contains SN0..SN2 (byte 3 is BCC0)
		// Page 1 (bytes 4..7) contains SN3..SN6
		if (card.size() < 8)
		{
			return {};
		}

		out_uid = {card[0], card[1], card[2], card[4], card[5], card[6], card[7]};
		return out_uid;
	}
	else if (card_t == card_type::iso14443_4)
	{
		// ISO/IEC 14443-4 Type A (e.g. MIFARE DESFire 7-byte UID or 4-byte UID)
		if (card.size() >= 7)
		{
			std::memcpy(out_uid.data(), card.data(), 7);
			return out_uid;
		}
	}

	// MIFARE Classic 1K / 4K: 4-byte single size UID from Block 0 (bytes 0..3)
	const auto uid4 = get_card_uid(card);
	if (!uid4)
	{
		return {};
	}

	std::memcpy(out_uid.data(), uid4->data(), mifare_uid_size);
	return std::span<const u8>(out_uid.data(), mifare_uid_size);
}

nfc_reader::mifare_block_info nfc_reader::get_mifare_block_info(u8 block)
{
	mifare_block_info info{};
	if (block < 128)
	{
		// Sectors 0..31: 4 blocks per sector (Mifare Classic 1K / 4K lower sectors)
		info.sector = block / 4;
		info.block_rel = block % 4;
		info.trailer_block = info.sector * 4 + 3;
		info.is_trailer = (info.block_rel == 3);
		info.cond_index = info.block_rel;
	}
	else
	{
		// Sectors 32..39: 16 blocks per sector (Mifare Classic 4K upper sectors)
		info.sector = static_cast<u8>(32 + (block - 128) / 16);
		info.block_rel = static_cast<u8>((block - 128) % 16);
		info.trailer_block = static_cast<u8>(128 + (info.sector - 32) * 16 + 15);
		info.is_trailer = (info.block_rel == 15);

		// NXP MF1S703x Table 5: 15 data blocks grouped into 3 access condition sets
		if (info.block_rel < 5)
			info.cond_index = 0;
		else if (info.block_rel < 10)
			info.cond_index = 1;
		else if (info.block_rel < 15)
			info.cond_index = 2;
		else
			info.cond_index = 3; // Block 15 is Sector Trailer
	}
	return info;
}

std::array<u8, mifare_block_size> nfc_reader::get_block_data(std::span<const u8> card, u8 block) const
{
	std::array<u8, mifare_block_size> data{};
	const usz offset = static_cast<usz>(block) * mifare_block_size;
	const usz total_blocks = is_4k_card(card) ? mifare_4k_blocks : mifare_1k_blocks;

	if (offset + mifare_block_size <= card.size())
	{
		std::memcpy(data.data(), card.data() + offset, mifare_block_size);
	}
	else if (block < total_blocks)
	{
		// Beyond currently provided card data (card.size() < 1K or card.size() < 4K):
		// Simulate empty sector with default transport Sector Trailer
		const auto info = get_mifare_block_info(block);
		if (info.is_trailer)
		{
			// Default transport Sector Trailer
			std::fill_n(data.begin(), 6, 0xFF);     // Key A: FF FF FF FF FF FF
			data[6] = 0xFF;                          // Access Bits Byte 6
			data[7] = 0x07;                          // Access Bits Byte 7
			data[8] = 0x80;                          // Access Bits Byte 8
			data[9] = 0x69;                          // GPB
			std::fill_n(data.begin() + 10, 6, 0xFF); // Key B: FF FF FF FF FF FF
		}
		else
		{
			// Default Data Block: 16 zero bytes
			data.fill(0x00);
		}
	}

	return data;
}

nfc_reader::access_bits nfc_reader::get_access_bits(const std::array<u8, mifare_block_size>& trailer, u8 cond_index)
{
	u8 b6 = trailer[6];
	u8 b7 = trailer[7];
	u8 b8 = trailer[8];

	// If the entire sector trailer is uninitialized (all zeros), treat as default factory transport configuration (FF 07 80)
	const bool is_uninitialized = std::all_of(trailer.begin(), trailer.end(), [](u8 b) { return b == 0x00; });
	if (is_uninitialized)
	{
		b6 = 0xFF;
		b7 = 0x07;
		b8 = 0x80;
	}

	// Non-inverted bits
	const u8 c1 = (b7 >> (4 + cond_index)) & 1;
	const u8 c2 = (b8 >> cond_index) & 1;
	const u8 c3 = (b8 >> (4 + cond_index)) & 1;

	// Inverted bits
	const u8 inv_c1 = (b6 >> cond_index) & 1;
	const u8 inv_c2 = (b6 >> (4 + cond_index)) & 1;
	const u8 inv_c3 = (b7 >> cond_index) & 1;

	// Redundancy check: each non-inverted bit must be the inverse of the inverted bit
	const bool valid = (c1 != inv_c1) && (c2 != inv_c2) && (c3 != inv_c3);

	return {c1, c2, c3, valid};
}

std::array<u8, mifare_block_size> nfc_reader::format_value_block(s32 value, u8 addr)
{
	std::array<u8, mifare_block_size> b{};
	write_to_ptr<le_t<s32>>(b, 0, value);
	write_to_ptr<le_t<s32>>(b, 4, ~value);
	write_to_ptr<le_t<s32>>(b, 8, value);

	const u8 inv_a = static_cast<u8>(~addr);
	b[12] = addr;
	b[13] = inv_a;
	b[14] = addr;
	b[15] = inv_a;

	return b;
}

bool nfc_reader::get_value_from_block(std::span<const u8> block, s32& out_value, u8& out_addr)
{
	if (block.size() < mifare_block_size)
		return false;

	for (usz i = 0; i < 4; ++i)
	{
		if (block[i] != static_cast<u8>(~block[4 + i]))
			return false;
		if (block[i] != block[8 + i])
			return false;
	}

	if (block[12] != static_cast<u8>(~block[13]) || block[12] != block[14] || block[13] != block[15])
		return false;

	out_value = read_from_ptr<le_t<s32>>(block, 0);
	out_addr = block[12];
	return true;
}

void nfc_reader::reset_card_session()
{
	m_authenticated_sector = -1;
	m_authenticated_key = auth_key::none;
	m_transfer_buffer_valid = false;
	m_transfer_buffer.fill(0);
}

void nfc_reader::reset_target_state()
{
	reset_card_session();
	m_target_state = 0x00;
	m_initiator_state = initiator_target_state::released;
	if (const auto pcsc = get_pcsc())
	{
		pcsc->invalidate_card_cache();
	}
}

void nfc_reader::build_dep_activation_response(u8 res, bool include_tg, bool is_pcsc, std::span<const u8> card)
{
	if (!is_pcsc && !card.empty() && get_card_type(card) == card_type::nfcip_dep && card.size() >= 10)
	{
		m_initiator_state = initiator_target_state::selected;
		std::vector<u8> resp = {res, static_cast<u8>(status::success)};
		if (include_tg)
		{
			resp.push_back(0x01); // Tg: Logical target number
		}
		resp.insert(resp.end(), card.begin(), card.begin() + 10); // NFCID3t (10B)
		resp.insert(resp.end(), {0x00, 0x00, 0x00, 0x0E, 0x00});  // DIDt, BSt, BRt, TO, PPt
		build_response(resp);
		return;
	}

	build_response({res, static_cast<u8>(status::timeout)});
}

nfc_reader::mifare_check_result nfc_reader::check_mifare_block_access(std::span<const u8> card, u8 block, bool allow_trailer, bool allow_block_0) const
{
	const usz total_blocks = is_4k_card(card) ? mifare_4k_blocks : mifare_1k_blocks;
	if (block >= total_blocks)
	{
		return {.error = status::timeout};
	}

	const auto info = get_mifare_block_info(block);

	// Must be authenticated to the target block's sector
	if (m_authenticated_sector != info.sector || m_authenticated_key == auth_key::none)
	{
		return {.error = status::mifare_auth_error};
	}

	// Block 0 (Manufacturer Block) is read-only
	if (!allow_block_0 && block == 0)
	{
		return {.error = status::mifare_auth_error};
	}

	// Value operations cannot target Sector Trailer
	if (!allow_trailer && info.is_trailer)
	{
		return {.error = status::mifare_auth_error};
	}

	const auto trailer = get_block_data(card, info.trailer_block);
	// In MIFARE Classic (MF1S503x Section 8.6.2), if access bits for any block are corrupt, the entire sector is irreversibly locked
	access_bits bits{};
	for (u8 cond_idx = 0; cond_idx < 4; ++cond_idx)
	{
		const auto cond = get_access_bits(trailer, cond_idx);
		if (!cond.valid)
		{
			return {.error = status::mifare_auth_error};
		}
		if (cond_idx == info.cond_index)
		{
			bits = cond;
		}
	}

	const usz offset = static_cast<usz>(block) * mifare_block_size;
	if (!allow_block_0 && (offset + mifare_block_size > card.size()))
	{
		return {.error = status::mifare_auth_error};
	}

	const u8 c = (bits.c1 << 2) | (bits.c2 << 1) | bits.c3;
	return {info, bits, c, offset, status::success};
}

void nfc_reader::ensure_worker_started_locked()
{
	if (m_worker_running.load(std::memory_order_acquire))
	{
		return;
	}

	if (m_worker_thread.joinable())
	{
		m_worker_thread.join();
	}

	m_worker_stop.store(false, std::memory_order_release);
	m_worker_running.store(true, std::memory_order_release);
	m_worker_thread = std::thread([this]()
	{
		worker_loop();
	});
}

void nfc_reader::start_worker()
{
	std::lock_guard lifecycle_lock(m_worker_lifecycle_mutex);
	ensure_worker_started_locked();
}

void nfc_reader::stop_worker()
{
	std::lock_guard lifecycle_lock(m_worker_lifecycle_mutex);
	if (!m_worker_running.load(std::memory_order_acquire) && !m_worker_thread.joinable())
	{
		return;
	}

	{
		std::lock_guard lock(m_task_mutex);
		m_worker_stop.store(true, std::memory_order_release);
		m_task_queue.clear();
		m_task_seq.store(0, std::memory_order_release);
		m_task_cv.notify_all();
	}

	if (m_worker_thread.joinable())
	{
		m_worker_thread.join();
	}
	m_worker_running.store(false, std::memory_order_release);
}

void nfc_reader::worker_loop()
{
	while (true)
	{
		worker_task task;
		{
			std::unique_lock lock(m_task_mutex);
			m_task_cv.wait(lock, [this]()
			{
				return m_worker_stop.load(std::memory_order_acquire) || !m_task_queue.empty();
			});
			if (m_worker_stop.load(std::memory_order_acquire) && m_task_queue.empty())
			{
				break;
			}

			// Only skip obsolete non-mutating polling/status requests when newer commands arrived
			const auto extract_cmd_byte = [](std::span<const u8> frame) -> u8
			{
				const usz hdr_off = is_extended_frame(frame) ? sizeof(extended_frame_header) : sizeof(frame_header);
				return (frame.size() > hdr_off) ? frame[hdr_off] : 0xFF;
			};
			while (m_task_queue.size() > 1 && m_task_queue.front().id < m_task_seq.load(std::memory_order_acquire))
			{
				const u8 front_cmd = extract_cmd_byte(m_task_queue.front().frame);
				if (front_cmd != static_cast<u8>(command_code::get_general_status) &&
					front_cmd != static_cast<u8>(command_code::in_list_passive_target) &&
					front_cmd != static_cast<u8>(command_code::in_auto_poll))
				{
					break;
				}
				m_task_queue.pop_front();
			}

			task = std::move(m_task_queue.front());
			m_task_queue.pop_front();
		}

		{
			std::lock_guard exec_lock(m_exec_mutex);
			execute_command(task.frame, task.card);

			{
				std::lock_guard buffer_lock(m_buffer_mutex);
				// Check m_task_seq while holding m_buffer_mutex so process_request cannot clear/increment between check and write
				if (task.id == m_task_seq.load(std::memory_order_acquire) && !m_exec_response.empty())
				{
					if (m_response_offset == no_response_offset && m_tx_read_pos == m_tx_buffer.size())
					{
						// Host already consumed ACK; hand off response buffer without copying or losing capacity
						m_tx_buffer.clear();
						m_tx_buffer.swap(m_exec_response);
						m_tx_read_pos = 0;
						m_response_offset = 0;
					}
					else
					{
						m_response_offset = m_tx_buffer.size();
						m_tx_buffer.insert(m_tx_buffer.end(), m_exec_response.begin(), m_exec_response.end());
					}
				}
				m_exec_response.clear();
			}
		}
	}
}

void nfc_reader::dispatch_async_command(u64 task_id, std::span<const u8> frame, std::span<u8> card)
{
	std::lock_guard lifecycle_lock(m_worker_lifecycle_mutex);
	ensure_worker_started_locked();
	{
		std::lock_guard lock(m_task_mutex);
		m_task_queue.push_back({
			task_id,
			std::vector<u8>(frame.begin(), frame.end()),
			card
		});
		m_task_cv.notify_one();
	}
}

void nfc_reader::process_request(const std::vector<u8>& in, std::span<u8> card)
{
	if (m_buzzer)
	{
		m_buzzer->check_device();
	}

	const std::span<const u8> frame = find_frame(in);

	if (!verify_request(frame))
	{
		return;
	}

	// In case NACK is received, the pending response should be retransmitted.
	// So only clear the pending response buffer when a new legitimate request or ACK is received.
	if (std::equal(nack_frame.begin(), nack_frame.end(), frame.begin()))
	{
		std::lock_guard lock(m_buffer_mutex);
		if (m_response_offset < m_tx_buffer.size())
		{
			m_tx_read_pos = m_response_offset; // Only retransmit the actual response; do not retransmit ACK
		}
		else
		{
			m_tx_buffer.clear();
			m_tx_read_pos = 0;
			m_response_offset = no_response_offset;
		}
		return;
	}

	// Host ACK confirms completed commands or aborts any in-flight command (PN532 UM0701-02 §6.2.1.4)
	if (std::equal(ack_frame.begin(), ack_frame.end(), frame.begin()))
	{
		std::lock_guard lock(m_buffer_mutex);
		++m_task_seq;
		m_tx_buffer.clear();
		m_tx_read_pos = 0;
		m_response_offset = no_response_offset;
		return;
	}

	const bool is_pcsc = fallback_to_fake_if_disconnected();
	const auto enqueue_async = [&]()
	{
		// Allocate task_id while holding m_buffer_mutex so superseded checks cannot race with buffer reset
		u64 task_id = 0;
		{
			std::lock_guard lock(m_buffer_mutex);
			task_id = ++m_task_seq;
			m_tx_buffer.clear();
			m_tx_read_pos = 0;
			m_response_offset = no_response_offset;
			append_ack_to_tx();
		}
		dispatch_async_command(task_id, frame, card);
	};

	if (!is_pcsc)
	{
		// Avoid blocking the USB/emulator thread if the PC/SC worker is still unwinding after an unplug
		std::unique_lock exec_lock(m_exec_mutex, std::defer_lock);
		if (m_worker_running.load(std::memory_order_acquire))
		{
			if (!exec_lock.try_lock())
			{
				enqueue_async();
				return;
			}
		}
		else
		{
			exec_lock.lock();
		}

		// Pure software emulation (Fake mode) is instantaneous (< 1 microsecond)
		execute_command(frame, card);
		std::lock_guard lock(m_buffer_mutex);
		m_tx_buffer.clear();
		m_tx_read_pos = 0;
		append_ack_to_tx();
		if (!m_exec_response.empty())
		{
			m_response_offset = m_tx_buffer.size();
			m_tx_buffer.insert(m_tx_buffer.end(), m_exec_response.begin(), m_exec_response.end());
		}
		else
		{
			m_response_offset = no_response_offset;
		}
		m_exec_response.clear();
		return;
	}

	// Real hardware PC/SC reader mode:
	// 1. Immediately acknowledge the verified frame to the host (host will read ACK first without delay)
	// 2. Asynchronously execute the PC/SC command on worker thread without blocking the thread calling this function
	enqueue_async();
}

void nfc_reader::execute_command(std::span<const u8> frame, std::span<u8> card)
{
	std::lock_guard lock(m_exec_mutex);
	m_exec_response.clear();

	const bool card_tapped = !card.empty();
	const auto pcsc = get_pcsc();
	const bool is_pcsc = pcsc && pcsc->is_open();

	if (!card_tapped && !is_pcsc)
	{
		reset_target_state();
	}

	const bool is_extended = is_extended_frame(frame);
	const usz data_offset = is_extended ? sizeof(extended_frame_header) : sizeof(frame_header);
	const usz data_len = (is_extended ? static_cast<usz>(read_from_ptr<extended_frame_header>(frame).len) : static_cast<usz>(read_from_ptr<frame_header>(frame).len)) - 1; // Excluding TFI
	const std::span<const u8> payload = frame.subspan(data_offset, data_len); // data_offset + data_len boundary check is done in verify_request()

	const auto command = static_cast<command_code>(::at32(payload, 0));
	const u8 res = response_code(command);

	const auto are_sector_access_bits_valid = [](const std::array<u8, mifare_block_size>& trailer) -> bool
	{
		for (u8 cond_idx = 0; cond_idx < 4; ++cond_idx)
		{
			if (!get_access_bits(trailer, cond_idx).valid)
			{
				return false;
			}
		}
		return true;
	};

	switch (command)
	{
	case command_code::diagnose:
	{
		// -------------------------------------------------------------------------
		// 7.2.1 Diagnose (0x00)
		// Performs internal self-diagnosis and communication testing routines.
		//
		// Expected Request Frame Parameters:
		//   Length: >= 2 bytes
		//   Data:   [0] 0x00 (Command Code)
		//           [1] NumTst (1 byte):
		//                 0x00: Communication Line Test
		//                 0x01: ROM Test
		//                 0x02: RAM Test
		//                 0x04: Polling Test to Target
		//                 0x05: Echo Back Test
		//                 0x06: Attention Request Test / Card Presence Detection
		//                 0x07: Antenna Test
		//           [2..n] InParam: Test-specific parameters
		//
		// Expected Response Frame Parameters:
		//   Length: Variable (1 to 263 bytes payload; no response for NumTst=0x05)
		//   Data:   [0] 0x01 (Response Code)
		//           [1..m] OutParam: Test-specific result bytes:
		//                 0x00: Echoes NumTst + InParam (m = InParam.size() + 1)
		//                 0x01, 0x02: 0x00 (OK) or 0xFF (NG) (1 byte)
		//                 0x04: Number of polling fails (1 byte, 0..128)
		//                 0x05: Infinite loop, no response frame returned
		//                 0x06: 0x00 (Target present) or non-zero (NG) (1 byte)
		//                 0x07: 0x00 (Antenna detected) or non-zero (NG) (1 byte)
		// -------------------------------------------------------------------------
		if (payload.size() < 2)
		{
			build_response();
			break;
		}

		const u8 num_tst = payload[1];
		switch (num_tst)
		{
		case 0x00: // Communication Line Test: Echo back NumTst + InParam
		{
			if (payload.size() > 264)
			{
				build_response();
				break;
			}

			std::vector<u8> resp = {res};
			resp.insert(resp.end(), payload.begin() + 1, payload.end());
			build_response(resp);
			break;
		}
		case 0x01: // ROM Test: 0x00 = OK
		case 0x02: // RAM Test: 0x00 = OK
		{
			if (payload.size() != 2)
			{
				build_response();
				break;
			}
			build_response({res, 0x00});
			break;
		}
		case 0x04: // Polling Test to Target (0x01 = 212k, 0x02 = 424k)
		{
			if (payload.size() != 3 || (payload[2] != 0x01 && payload[2] != 0x02))
			{
				build_response();
				break;
			}

			build_response({res, 0x00});
			break;
		}
		case 0x05: // Echo Back Test (requires ReplyDelay, CIU_TxMode, CIU_RxMode)
		{
			if (payload.size() != 5)
			{
				build_response();
				break;
			}

			write_register_value(0x6302, payload[3]);
			write_register_value(0x6303, payload[4]);
			// Runs until next host command; no response frame is sent
			m_exec_response.clear();
			break;
		}
		case 0x06:
		{
			// UM0701-02 Section 7.2.1: Tests if a previously activated target is still present.
			if (payload.size() != 2)
			{
				build_response();
				break;
			}
			// For DEP targets: sends ATN (Attention Request) and expects the same response.
			// For ISO/IEC 14443-4 cards: sends R(NACK) and checks for a valid reply.
			// Returns 0x00 if the target is present; otherwise, returns an error code.
			const bool target_active = (m_initiator_state != initiator_target_state::released) &&
				(card_tapped || (is_pcsc && pcsc->get_card_info().present));
			build_response({res, static_cast<u8>(target_active ? status::success : status::timeout)});
			break;
		}
		case 0x07: // Antenna Test: 0x00 = Antenna OK
		{
			// UM0701-02 Section 7.2.1: Threshold used for antenna detection applied in Andet_Control (@610C)
			if (payload.size() != 3)
			{
				build_response(); // Syntax error: requires exactly 1 threshold parameter
				break;
			}

			write_register_value(0x610C, payload[2]);
			build_response({res, 0x00});
			break;
		}
		default:
		{
			build_response(); // Unknown test number -> syntax error
			break;
		}
		}
		break;
	}
	case command_code::get_firmware_version:
	{
		// -------------------------------------------------------------------------
		// 7.2.2 GetFirmwareVersion (0x02)
		// Retrieves the IC model, embedded firmware version, and supported standards.
		//
		// Expected Request Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x02 (Command Code)
		//
		// Expected Response Frame Parameters:
		//   Length: 5 bytes
		//   Data:   [0] 0x03 (Response Code)
		//           [1] IC (0x32 for PN532)
		//           [2] Ver (Firmware major version, e.g. 0x01)
		//           [3] Rev (Firmware minor revision, e.g. 0x06 for v1.6)
		//           [4] Support bitmask (0x07: bit 0=ISO14443A, bit 1=ISO14443B, bit 2=ISO18092)
		// -------------------------------------------------------------------------
		if (payload.size() != 1)
		{
			build_response();
			break;
		}

		build_response(to_bytes(get_firmware_version_response{}));
		break;
	}
	case command_code::get_general_status:
	{
		// -------------------------------------------------------------------------
		// 7.2.3 GetGeneralStatus (0x04)
		// Inspects current situation of the PN532: last error, RF field, targets, SAM.
		//
		// Expected Request Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x04 (Command Code)
		//
		// Expected Response Frame Parameters:
		//   Length: 5 bytes (0 targets) to 13 bytes (2 targets)
		//   Data:   [0] 0x05 (Response Code)
		//           [1] Err (Latest error code, cleared after reading)
		//           [2] Field (0x01 if external RF field present, else 0x00)
		//           [3] NbTg (Number of targets currently controlled: 0, 1, or 2)
		//           For each target (4 bytes each):
		//             [+0] Tg (Logical target number: 1 or 2)
		//             [+1] BrRx (Rx bit rate: 0x00=106k, 0x01=212k, 0x02=424k)
		//             [+2] BrTx (Tx bit rate: 0x00=106k, 0x01=212k, 0x02=424k)
		//             [+3] Type (Modulation: 0x00=Mifare/ISO14443A, 0x10=FeliCa, etc.)
		//           [Last] SAM status (Status of SAM connection and CLAD line)
		// -------------------------------------------------------------------------
		if (payload.size() != 1)
		{
			build_response();
			break;
		}

		const u8 last_err = m_last_error;
		m_last_error = 0x00; // Cleared after reading per UM0701-02 §7.2.3

		if (is_pcsc)
		{
			if (m_initiator_state != initiator_target_state::released)
			{
				const auto info = pcsc->get_card_info();
				if (info.present)
				{
					const u8 br = info.is_felica ? ((info.felica_speed == 2) ? 0x02 : 0x01) : 0x00;
					const u8 type = info.is_felica ? 0x10 : (info.is_type_b ? 0x03 : 0x00);
					build_response({
						res,
						last_err,
						0x00,
						0x01,
						0x01,
						br,
						br,
						type,
						0x00
					});
					break;
				}
			}
			build_response({
				res,
				last_err,
				0x00,
				0x00,
				0x00
			});
			break;
		}

		if (card_tapped && m_initiator_state != initiator_target_state::released)
		{
			const auto card_t = get_card_type(card);
			u8 br_rx = 0x00; // 106 kbps
			u8 br_tx = 0x00; // 106 kbps
			u8 type = 0x00;  // Mifare / ISO14443-3A / ISO14443-4

			if (card_t == card_type::felica)
			{
				br_rx = 0x01; // 212 kbps default for FeliCa
				br_tx = 0x01; // 212 kbps default for FeliCa
				type = 0x10;  // FeliCa / ISO18092 passive 212/424 kbps
			}
			else if (card_t == card_type::jewel)
			{
				type = 0x02; // Innovision Jewel tag
			}

			build_response({
				res,
				last_err, // Err: Latest error detected, cleared after reading
				0x00,     // Field: No external RF field detected per UM0701-02 §7.2.3 (initiator passive mode)
				0x01,     // NbTg: 1 target present
				0x01,     // Tg 1: Target logical number 1
				br_rx,    // BrRx 1
				br_tx,    // BrTx 1
				type,     // Type 1
				0x00      // SAM status
			});
		}
		else
		{
			// No targets present, no external RF field
			build_response({
				res,
				last_err, // Err: Latest error detected, cleared after reading
				0x00,     // Field: No external RF field
				0x00,     // NbTg: 0 targets
				0x00      // SAM status
			});
		}
		break;
	}
	case command_code::read_register:
	{
		// -------------------------------------------------------------------------
		// 7.2.4 ReadRegister (0x06)
		// Reads internal registers (SFRs: 0xFF00-0xFFFF, CIU: 0x6300-0x633F).
		//
		// Expected Request Frame Parameters:
		//   Length: 1 + 2*n bytes (n >= 1)
		//   Data:   [0] 0x06 (Command Code)
		//           [1..2*n] ADR_H ADR_L pairs (16-bit big-endian addresses)
		//
		// Expected Response Frame Parameters:
		//   Length: 1 + n bytes
		//   Data:   [0] 0x07 (Response Code)
		//           [1..n] VAL1..VALn (Register values)
		// -------------------------------------------------------------------------
		if (payload.size() < 3 || ((payload.size() - 1) % 2 != 0))
		{
			build_response();
			break;
		}

		const usz num_addrs = (payload.size() - 1) / 2;

		// UM0701-02 §7.2.4: Unknown SFR address results in a syntax error.
		bool syntax_error = false;
		for (usz i = 0; i < num_addrs; ++i)
		{
			const u16 addr = (static_cast<u16>(payload[1 + 2 * i]) << 8) | payload[2 + 2 * i];
			if (!is_valid_register_address(addr))
			{
				syntax_error = true;
				break;
			}
		}

		if (syntax_error)
		{
			build_response();
			break;
		}

		std::vector<u8> resp = {res};
		resp.reserve(1 + num_addrs);

		for (usz i = 0; i < num_addrs; ++i)
		{
			const u16 addr = (static_cast<u16>(payload[1 + 2 * i]) << 8) | payload[2 + 2 * i];
			resp.push_back(read_register_value(addr));
		}

		build_response(resp);
		break;
	}
	case command_code::write_register:
	{
		// -------------------------------------------------------------------------
		// 7.2.5 WriteRegister (0x08)
		// Overwrites internal SFR or CIU registers.
		//
		// Expected Request Frame Parameters:
		//   Length: 1 + 3*n bytes (n >= 1)
		//   Data:   [0] 0x08 (Command Code)
		//           [1..3*n] ADR_H ADR_L VAL triplets (16-bit address and 8-bit value)
		//
		// Expected Response Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x09 (Response Code)
		// -------------------------------------------------------------------------
		if (payload.size() < 4 || ((payload.size() - 1) % 3 != 0))
		{
			build_response();
			break;
		}

		const usz num_triplets = (payload.size() - 1) / 3;

		// Validate all addresses before applying writes (atomic rejection per UM0701-02 §7.2.5)
		bool syntax_error = false;
		for (usz i = 0; i < num_triplets; ++i)
		{
			const u16 addr = (static_cast<u16>(payload[1 + 3 * i]) << 8) | payload[2 + 3 * i];
			if (!is_valid_register_address(addr))
			{
				syntax_error = true;
				break;
			}
		}

		if (syntax_error)
		{
			build_response(); // Syntax error: unknown SFR address
			break;
		}

		for (usz i = 0; i < num_triplets; ++i)
		{
			const u16 addr = (static_cast<u16>(payload[1 + 3 * i]) << 8) | payload[2 + 3 * i];
			const u8 val = payload[3 + 3 * i];
			write_register_value(addr, val);
		}

		// Different from what PN532 manual indicates, the application expects an extra byte of 0x00 following the response code,
		// and RC-S620/S Commend Reference Manual also indicates the same (the response should be D5 09 00).
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::read_gpio:
	{
		// -------------------------------------------------------------------------
		// 7.2.6 ReadGPIO (0x0C)
		// Reads current port state of P3, P7, and interface mode lines I0/I1.
		//
		// Expected Request Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x0C (Command Code)
		//
		// Expected Response Frame Parameters:
		//   Length: 4 bytes
		//   Data:   [0] 0x0D (Response Code)
		//           [1] P3: Port 3 GPIO status (bits 0..5: P30..P35)
		//           [2] P7: Port 7 GPIO status (bit 1: P71, bit 2: P72)
		//           [3] I0I1: Interface selection lines (bit 0: I0, bit 1: I1)
		// -------------------------------------------------------------------------
		if (payload.size() != 1)
		{
			build_response();
			break;
		}

		read_gpio_response resp{};
		resp.p3 = m_gpio_p3;
		resp.p7 = m_gpio_p7;
		resp.i0i1 = m_gpio_i0i1;
		build_response(to_bytes(resp));
		break;
	}
	case command_code::write_gpio:
	{
		// -------------------------------------------------------------------------
		// 7.2.7 WriteGPIO (0x0E)
		// Applies output values to general-purpose ports P3 and P7 simultaneously.
		//
		// Expected Request Frame Parameters:
		//   Length: 3 bytes
		//   Data:   [0] 0x0E (Command Code)
		//           [1] P3 (Bit 7: Val=1 validates, bits 0..5: P30..P35 output value)
		//           [2] P7 (Bit 7: Val=1 validates, bit 1: P71, bit 2: P72 output value)
		//
		// Expected Response Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x0F (Response Code)
		// -------------------------------------------------------------------------
		if (payload.size() != 3)
		{
			build_response();
			break;
		}

		const u8 p3_in = payload[1];
		const u8 p7_in = payload[2];

		// Vendor buzzer control command multiplexed onto WriteGPIO (P3 = 0x08, P7 = mode)
		if (p3_in == 0x08 && m_buzzer)
		{
			// Type 0x20~0x3F, 0x40~0x5F, ..., 0x80~0x9F (primarily used range), ..., and 0xE0~0xFF behave the same as 0x00~0x1F
			const u8 type = p7_in & 0x1F;
			switch (type)
			{
			case 0x01:
			{
				m_buzzer->play_beep_1();
				break;
			}
			case 0x02:
			{
				m_buzzer->play_beep_3();
				break;
			}
			case 0x03:
			{
				m_buzzer->play_periodic_burst();
				break;
			}
			case 0x04:
			{
				m_buzzer->play_short_long_alert();
				break;
			}
			case 0x05:
			{
				m_buzzer->play_descending_alert();
				break;
			}
			case 0x06:
			{
				m_buzzer->play_chromatic_siren();
				break;
			}
			case 0x07:
			{
				m_buzzer->play_arpeggio();
				break;
			}
			case 0x09:
			case 0x13:
			{
				m_buzzer->play_scale();
				break;
			}
			case 0x0A:
			{
				m_buzzer->play_click();
				break;
			}
			case 0x0B:
			case 0x0C:
			case 0x0D:
			case 0x0E:
			case 0x0F:
			{
				m_buzzer->play_count(type - 0x0B);
				break;
			}
			case 0x14:
			{
				m_buzzer->play_high_scale();
				break;
			}
			case 0x1F:
			{
				m_buzzer->play_count(0);
				break;
			}
			default:
			{
				m_buzzer->stop();
				break;
			}
			}
		}
		else
		{
			if (p3_in & 0x80) // Bit 7: Val validates P30..P35
			{
				write_register_value(0xFFB0, p3_in & 0x3F);
			}

			if (p7_in & 0x80) // Bit 7: Val validates P71, P72
			{
				write_register_value(0xFFF7, p7_in & 0x06);
			}
		}

		build_response({res});
		break;
	}
	case command_code::set_serial_baud_rate:
	{
		// -------------------------------------------------------------------------
		// 7.2.8 SetSerialBaudRate (0x10)
		// Configures HSU (High Speed UART) interface baud rate.
		//
		// Expected Request Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x10 (Command Code)
		//           [1] BR (0x00: 9600, 0x01: 19200, 0x02: 38400, 0x03: 57600,
		//                   0x04: 115200 (default), 0x05: 230400, 0x06: 460800,
		//                   0x07: 921600, 0x08: 1288000)
		//
		// Expected Response Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x11 (Response Code)
		//   Note: The ACK and response frame are returned at the current baud rate
		//         before the baud rate switch takes effect.
		// -------------------------------------------------------------------------
		if (payload.size() != 2 || payload[1] > 0x08)
		{
			build_response();
			break;
		}

		build_response({res});
		break;
	}
	case command_code::set_parameters:
	{
		// -------------------------------------------------------------------------
		// 7.2.9 SetParameters (0x12)
		// Configures internal firmware flags and behavior.
		//
		// Expected Request Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x12 (Command Code)
		//           [1] Flags bitmask:
		//                 Bit 0: fNADUsed (Use NAD in DEP / ISO14443-4 PCD)
		//                 Bit 1: fDIDUsed (Use DID/CID in DEP / ISO14443-4 PCD)
		//                 Bit 2: fAutomaticATR_RES (Auto-generate ATR_RES in Target mode)
		//                 Bit 3: RFU (Must be set to 0 for PN532; Must be set to 1 for RC-S620/S)
		//                 Bit 4: fAutomaticRATS (Auto-generate RATS in ISO14443-4 PCD mode)
		//                 Bit 5: fISO14443-4_PICC (Enable ISO14443-4 PICC emulation)
		//                 Bit 6: fRemovePrePostAmble (Suppress Preamble and Postamble)
		//                 Bit 7: RFU (Must be set to 0)
		//
		// Expected Response Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x13 (Response Code)
		// -------------------------------------------------------------------------
		if (payload.size() != 2)
		{
			build_response();
			break;
		}

		m_parameters_flags.store(payload[1], std::memory_order_release);
		build_response({res});
		break;
	}
	case command_code::sam_configuration:
	{
		// -------------------------------------------------------------------------
		// 7.2.10 SAMConfiguration (0x14)
		// Configures the internal data flow switch for SAM (Security Access Module).
		//
		// Requires further hardware implementation (Physical SAM interface and secure element communication)
		//
		// Expected Request Frame Parameters:
		//   Length: 2 to 4 bytes
		//   Data:   [0] 0x14 (Command Code)
		//           [1] Mode:
		//                 0x01: Normal Mode (SAM not used, default)
		//                 0x02: Virtual Card (PN532+SAM seen as one contactless card)
		//                 0x03: Wired Card (Host controller accesses SAM directly)
		//                 0x04: Dual Card (PN532 Target and SAM both visible)
		//           [2] Timeout (optional for Modes 1/3/4; mandatory for Mode 2):
		//                 Timeout value in units of 50ms (0x00 = no timeout, max 12.75s)
		//           [3] IRQ (optional):
		//                 0x00: P70_IRQ pin remains High
		//                 0x01: P70_IRQ pin driven by PN532 (default)
		//
		// Expected Response Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x15 (Response Code)
		// -------------------------------------------------------------------------
		if (payload.size() < 2 || payload.size() > 4)
		{
			build_response();
			break;
		}

		// UM0701-02 §7.2.10: Mode must be 0x01..0x04; Timeout is mandatory for Virtual Card mode (0x02)
		if (payload[1] < 0x01 || payload[1] > 0x04)
		{
			build_response(); // Syntax error: incorrect Mode value
			break;
		}

		if (payload[1] == 0x02 && payload.size() < 3)
		{
			build_response(); // Syntax error: Timeout parameter missing in Virtual Card mode
			break;
		}

		if (payload.size() >= 4 && payload[3] > 0x01)
		{
			build_response(); // Syntax error: IRQ parameter must be 0x00 or 0x01
			break;
		}

		m_sam_mode = payload[1];
		build_response({res});
		break;
	}
	case command_code::power_down:
	{
		// -------------------------------------------------------------------------
		// 7.2.11 PowerDown (0x16)
		// Puts the PN532 and RF frontend into power-down mode to save energy.
		//
		// Expected Request Frame Parameters:
		//   Length: 2 to 3 bytes
		//   Data:   [0] 0x16 (Command Code)
		//           [1] WakeUpEnable bitmask:
		//                 Bit 0: INT0 (P32), Bit 1: INT1 (P33)
		//                 Bit 3: RFLevelDetector
		//                 Bit 4: HSU (5th rising edge)
		//                 Bit 5: SPI (NSS low)
		//                 Bit 6: GPIO
		//                 Bit 7: I2C (address match)
		//           [2] GenerateIRQ (optional): 0x01 asserts P70_IRQ on RF wake up
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x17 (Response Code)
		//           [1] Status (0x00 = Success)
		// -------------------------------------------------------------------------
		if (payload.size() < 2 || payload.size() > 3 || (payload[1] & 0x04) != 0 || (payload.size() == 3 && payload[2] > 0x01))
		{
			build_response();
			break;
		}

		if (is_pcsc)
		{
			pcsc->release_target(m_authenticated_sector >= 0);
		}

		reset_target_state();
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::reset:
	{
		// -------------------------------------------------------------------------
		// 8.2.5 Reset (0x18) - Sony RC-S620/S Specification
		// Transitions operating mode back to Mode 0 (Idle / Standby) and clears target context.
		//
		// Expected Request Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x18 (Command Code)
		//           [1] Function (0x01 fixed for RC-S620/S)
		//
		// Expected Response Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x19 (Response Code)
		// -------------------------------------------------------------------------
		if (payload.size() != 2 || payload[1] != 0x01)
		{
			build_response();
			break;
		}

		if (is_pcsc)
		{
			pcsc->release_target(m_authenticated_sector >= 0);
		}

		reset_target_state();
		build_response({res});
		break;
	}
	case command_code::rf_configuration:
	{
		// -------------------------------------------------------------------------
		// 7.3.1 RFConfiguration (0x32)
		// Configures RF analog registers, timeouts, retries, and field status.
		//
		// Expected Request Frame Parameters:
		//   Length: Variable (2 to 13 bytes)
		//   Data:   [0] 0x32 (Command Code)
		//           [1] CfgItem (Configuration Item ID):
		//                 0x01: RF Field (1B: AutoRFCA, RF on/off)
		//                 0x02: Various timings (3B: RFU, fATR_RES_Timeout, fRetryTimeout)
		//                 0x04: MaxRtyCOM (1B: Max retry count in non-DEP communications)
		//                 0x05: MaxRetries (3B: MxRtyATR, MxRtyPSL, MxRtyPassiveActivation)
		//                 0x0A: Analog 106k Type A (11B CIU register values)
		//                 0x0B: Analog 212/424k (8B CIU register values)
		//                 0x0C: Analog Type B (3B CIU register values)
		//                 0x0D: Analog 212/424/848k ISO14443-4 (9B CIU register values)
		//                 0x81: Additional wait time for Target ID acquisition (Sony RC-S620/S extension, 1B)
		//                 0x82: DEP timeout parameters (Sony RC-S620/S extension, 3B: gbyAtrResTo, gbyRtox, gbyTargetStoTo)
		//           [2..n] ConfigurationData: Specific to CfgItem
		//
		// Expected Response Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x33 (Response Code)
		// -------------------------------------------------------------------------
		if (payload.size() < 2)
		{
			build_response();
			break;
		}

		const u8 cfg_item = payload[1];
		switch (cfg_item)
		{
		case 0x01: // RF field on/off & AutoRFCA (1 byte)
		{
			if (payload.size() != 3)
			{
				build_response();
				break;
			}

			const u8 rf_field = payload[2];
			// Bit 0: RF on/off -> controls CIU_TxControl (0x6304) Tx1RFEn and Tx2RFEn (bits 1:0)
			if (rf_field & 0x01)
			{
				write_register_value(0x6304, read_register_value(0x6304) | 0x03);
			}
			else
			{
				write_register_value(0x6304, read_register_value(0x6304) & ~0x03);
			}

			// Bit 1: AutoRFCA -> controls CIU_TxAuto (0x6305) CAOn and InitialRFOn (bits 3:2)
			if (rf_field & 0x02)
			{
				write_register_value(0x6305, read_register_value(0x6305) | 0x0C);
			}
			else
			{
				write_register_value(0x6305, read_register_value(0x6305) & ~0x0C);
			}

			build_response({res});
			break;
		}
		case 0x02: // Various timings (3 bytes: RFU, fATR_RES_Timeout, fRetryTimeout)
		{
			if (payload.size() != 5)
			{
				build_response();
				break;
			}

			// UM0701-02 Section 7.3.1: Syntax error if timing value > 0x10
			if (payload[3] > 0x10 || payload[4] > 0x10)
			{
				build_response();
				break;
			}

			m_atr_res_timeout = payload[3];
			m_retry_timeout = payload[4];
			build_response({res});
			break;
		}
		case 0x04: // MaxRtyCOM (1 byte)
		{
			if (payload.size() != 3)
			{
				build_response();
				break;
			}

			m_max_rty_com = payload[2];
			build_response({res});
			break;
		}
		case 0x05: // MaxRetries (3 bytes: MxRtyATR, MxRtyPSL, MxRtyPassiveActivation)
		{
			if (payload.size() != 5)
			{
				build_response();
				break;
			}

			m_mx_rty_atr = payload[2];
			m_mx_rty_psl = payload[3];
			m_mx_rty_passive_activation = payload[4];
			build_response({res});
			break;
		}
		case 0x0A: // Analog 106k Type A (11 bytes)
		{
			if (payload.size() != 13)
			{
				build_response();
				break;
			}

			// Table 19: 11 bytes of analog settings for 106 kbps type A
			write_register_value(0x6316, payload[2]);  // CIU_RFCfg
			write_register_value(0x6317, payload[3]);  // CIU_GsNOn
			write_register_value(0x6318, payload[4]);  // CIU_CWGsP
			write_register_value(0x6319, payload[5]);  // CIU_ModGsP
			write_register_value(0x6309, payload[6]);  // CIU_Demod (when own RF is On)
			write_register_value(0x6308, payload[7]);  // CIU_RxThreshold
			write_register_value(0x6313, payload[9]);  // CIU_GsNOff
			write_register_value(0x6314, payload[10]); // CIU_ModWidth
			write_register_value(0x630C, payload[11]); // CIU_MifNFC
			write_register_value(0x6315, payload[12]); // CIU_TxBitPhase

			// Also synchronize to XRAM 106 kbps analog configuration table (0xA010..0xA01A)
			// Note: payload[8] (Table 19 Byte 7, "CIU_Demod when own RF is Off") is NOT written
			// to CIU register 0x6309 here — the physical register holds the RF-On value from
			// payload[6]. The RF-Off value is only cached in XRAM (0xA016) via this loop and
			// applied at runtime by PN532 firmware when the RF field is switched off.
			for (usz i = 0; i < 11; ++i)
			{
				write_register_value(::narrow<u16>(0xA010 + i), payload[2 + i]);
			}

			build_response({res});
			break;
		}
		case 0x0B: // Analog 212/424 kbps (8 bytes)
		{
			if (payload.size() != 10)
			{
				build_response();
				break;
			}

			// Table 20: 8 bytes of analog settings for 212/424 kbps
			write_register_value(0x6316, payload[2]);  // CIU_RFCfg
			write_register_value(0x6317, payload[3]);  // CIU_GsNOn
			write_register_value(0x6318, payload[4]);  // CIU_CWGsP
			write_register_value(0x6319, payload[5]);  // CIU_ModGsP
			write_register_value(0x6309, payload[6]);  // CIU_Demod (when own RF is On)
			write_register_value(0x6308, payload[7]);  // CIU_RxThreshold
			write_register_value(0x6313, payload[9]);  // CIU_GsNOff

			// Synchronize to XRAM 212/424 kbps analog configuration table (0xA01B..0xA022)
			// Note: payload[8] (Table 20 Byte 7, "CIU_Demod when own RF is Off") is NOT written
			// to CIU register 0x6309 here — the physical register holds the RF-On value from
			// payload[6]. The RF-Off value is only cached in XRAM (0xA022) via this loop and
			// applied at runtime by PN532 firmware when the RF field is switched off.
			for (usz i = 0; i < 8; ++i)
			{
				write_register_value(::narrow<u16>(0xA01B + i), payload[2 + i]);
			}

			// Synchronize shared 424 kbps driver conductance cache (0xA0CB..0xA0CE)
			write_register_value(0xA0CB, payload[3]); // CIU_GsNOn
			write_register_value(0xA0CC, payload[4]); // CIU_CWGsP
			write_register_value(0xA0CD, payload[5]); // CIU_ModGsP
			write_register_value(0xA0CE, payload[9]); // CIU_GsNOff

			build_response({res});
			break;
		}
		case 0x0C: // Analog Type B (3 bytes)
		{
			if (payload.size() != 5)
			{
				build_response();
				break;
			}

			// Table 21: 3 bytes of analog settings for Type B
			write_register_value(0x6317, payload[2]); // CIU_GsNOn
			write_register_value(0x6319, payload[3]); // CIU_ModGsP
			write_register_value(0x6308, payload[4]); // CIU_RxThreshold

			build_response({res});
			break;
		}
		case 0x0D: // Analog 212/424/848 kbps ISO/IEC14443-4 (9 bytes)
		{
			if (payload.size() != 11)
			{
				build_response();
				break;
			}

			// Table 22: 9 bytes of analog settings (RxThreshold, ModWidth, MifNFC for 212k, 424k, 848k)
			// Apply the 212 kbps active settings to current CIU registers
			write_register_value(0x6308, payload[2]); // CIU_RxThreshold (212k)
			write_register_value(0x6314, payload[3]); // CIU_ModWidth (212k)
			write_register_value(0x630C, payload[4]); // CIU_MifNFC (212k)

			build_response({res});
			break;
		}
		case 0x81: // Additional wait time for Target ID acquisition (Sony RC-S620/S extension, 1 byte)
		{
			if (payload.size() != 3)
			{
				build_response();
				break;
			}

			// Extra wait time after RF On before sending Target ID acquisition command (e.g. Polling)
			// Wait time = 0.1 * ConfigurationData + 5.7 ms
			m_rf_on_wait_time = payload[2];
			build_response({res});
			break;
		}
		case 0x82: // DEP timeout parameters (Sony RC-S620/S extension, 3 bytes)
		{
			if (payload.size() != 5)
			{
				build_response();
				break;
			}

			// Byte 1: gbyAtrResTo (0x00..0x0E), Byte 2: gbyRtox (0x01..0x3B), Byte 3: gbyTargetStoTo (0x00..0x0E)
			if (payload[2] > 0x0E || payload[3] < 0x01 || payload[3] > 0x3B || payload[4] > 0x0E)
			{
				build_response();
				break;
			}

			m_dep_atr_res_to = payload[2];
			m_dep_rtox = payload[3];
			m_dep_target_sto_to = payload[4];
			build_response({res});
			break;
		}
		default:
		{
			build_response(); // Syntax error: invalid CfgItem
			break;
		}
		}
		break;
	}
	case command_code::in_data_exchange:
	{
		// -------------------------------------------------------------------------
		// 7.3.8 InDataExchange (0x40)
		// Supports high-level protocol data exchanges with activated targets.
		// Handles Mifare Classic 1K / 4K authentication (Key A / Key B), Access Condition checking,
		// and read/write operations with asymmetric sector support (4-block / 16-block sectors).
		//
		// Expected Request Frame Parameters:
		//   Length: 2 to 264 bytes
		//   Data:   [0] 0x40 (Command Code)
		//           [1] Tg: Target logical number (Bit 6 is MI = More Information for chaining)
		//           [2..n] DataOut: Command payload formatted by card type:
		//                   - MIFARE: [Cmd] [Addr] [Data0..15]
		//                       0x60/0x61: Authenticate with Key A/B (6B Key + 4B UID)
		//                       0x30: Read 16 bytes
		//                       0xA0: Write 16 bytes
		//                       0xA2: Write 4 bytes
		//                       0xC1: Incrementation
		//                       0xC0: Decrementation
		//                       0xB0: Transfer
		//                       0xC2: Restore
		//                   - ISO/IEC 14443-4: C-APDU (up to 261 bytes)
		//                   - FeliCa: [Len] [Cmd] [Data...]
		//                   - Jewel: [Cmd] [Addr] [Data1..8]
		//
		// Expected Response Frame Parameters:
		//   Length: 2 to 264 bytes
		//   Data:   [0] 0x41 (Response Code)
		//           [1] Status: Bit 7=NAD present, Bit 6=MI (chaining), Bits 0..5=Error code:
		//                 0x00: Success
		//                 0x01: Timeout
		//                 0x14: Mifare authentication error
		//                 0x27: Target not active / invalid context
		//           [2..m] DataIn: Received response payload (e.g. 16B Mifare block, R-APDU)
		// -------------------------------------------------------------------------
		if (payload.size() < 2 || payload.size() > 264)
		{
			build_response();
			break;
		}

		// Validate Tg (bits 0..5): PN532 only holds target 1 in this emulation session.
		// If target is invalid / not active, return Context Error (0x27)
		if ((payload[1] & 0x3F) != 0x01)
		{
			build_response({res, static_cast<u8>(status::context_error)});
			break;
		}

		if (is_pcsc)
		{
			const auto card_info = pcsc->get_card_info(false);
			if (!card_info.present || m_initiator_state == initiator_target_state::released)
			{
				reset_target_state();
				m_last_error = static_cast<u8>(status::context_error);
				build_response({res, static_cast<u8>(status::context_error)});
				break;
			}

			if (m_initiator_state == initiator_target_state::deselected)
			{
				pcsc->connect_card();
				m_initiator_state = initiator_target_state::selected;
			}

			const std::span<const u8> data_out = payload.subspan(2);
			if (data_out.empty())
			{
				build_response({res, static_cast<u8>(status::success)});
				break;
			}

			const u8 cmd = data_out[0];
			const bool is_auth_cmd = (cmd == static_cast<u8>(mifare_cmd::auth_key_a) || cmd == static_cast<u8>(mifare_cmd::auth_key_b)) &&
				!(cmd == 0x60 && data_out.size() == 1);
			const bool is_read_cmd = (cmd == static_cast<u8>(mifare_cmd::read_16_bytes) && data_out.size() == 2);
			const bool is_write_cmd = (cmd == static_cast<u8>(mifare_cmd::write_16_bytes) || cmd == static_cast<u8>(mifare_cmd::write_4_bytes));
			const bool is_valid_write_len = (cmd == static_cast<u8>(mifare_cmd::write_16_bytes) && data_out.size() == 18) ||
				(cmd == static_cast<u8>(mifare_cmd::write_4_bytes) && data_out.size() == 6);

			if (card_info.is_felica)
			{
				std::vector<u8> felica_resp;
				if (pcsc->felica_pass_thru(data_out, felica_resp))
				{
					std::vector<u8> resp = {res, static_cast<u8>(status::success)};
					resp.insert(resp.end(), felica_resp.begin(), felica_resp.end());
					build_response(resp);
				}
				else
				{
					build_response({res, static_cast<u8>(status::timeout)});
				}
				break;
			}
			else if (is_auth_cmd)
			{
				if (data_out.size() < 8)
				{
					build_response();
					break;
				}

				// Non-MIFARE cards (e.g. EMV credit cards, Type B, DESFire) and MIFARE Ultralight/NTAG (SAK=0x00)
				// do not support Crypto1 auth. Reject immediately instead of leaking into T=CL or stalling on RF.
				if (!card_info.is_mifare || (card_info.sak_valid && card_info.sak == 0x00))
				{
					reset_card_session();
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
					break;
				}

				const u8 block = data_out[1];
				const bool is_key_a = (cmd == static_cast<u8>(mifare_cmd::auth_key_a));
				std::span<const u8, 6> key(data_out.subspan(2, 6).data(), 6);
				if (pcsc->mifare_authenticate(block, is_key_a, key))
				{
					m_authenticated_sector = get_mifare_block_info(block).sector;
					m_authenticated_key = is_key_a ? auth_key::key_a : auth_key::key_b;
					build_response({res, static_cast<u8>(status::success)});
				}
				else
				{
					reset_card_session();
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
				}
				break;
			}
			else if (is_read_cmd)
			{
				if (!card_info.is_mifare)
				{
					// Do not send raw 2-byte MIFARE read frames to ISO 14443-4 cards (e.g. credit cards)
					build_response({res, static_cast<u8>(status::timeout)});
					break;
				}

				const u8 block = data_out[1];
				std::vector<u8> block_data;
				if (pcsc->mifare_read(block, block_data) ||
					(card_info.sak_valid && card_info.sak == 0x00 && pcsc->part3_pass_thru(data_out, block_data)))
				{
					std::vector<u8> resp = {res, static_cast<u8>(status::success)};
					resp.insert(resp.end(), block_data.begin(), block_data.end());
					build_response(resp);
				}
				else
				{
					build_response({res, static_cast<u8>(status::timeout)});
				}
				break;
			}
			else if (card_info.is_iso14443_4 && !(card_info.is_mifare && is_valid_write_len))
			{
				std::vector<u8> rapdu;
				if (pcsc->tcl_pass_thru(data_out, rapdu))
				{
					std::vector<u8> resp = {res, static_cast<u8>(status::success)};
					resp.insert(resp.end(), rapdu.begin(), rapdu.end());
					build_response(resp);
				}
				else
				{
					build_response({res, static_cast<u8>(status::timeout)});
				}
				break;
			}
			else if (card_info.is_mifare && is_valid_write_len)
			{
				const u8 block = data_out[1];
				const std::span<const u8> write_bytes = data_out.subspan(2);
				std::vector<u8> part3_resp;
				if (pcsc->mifare_write(block, write_bytes) ||
					(card_info.sak_valid && card_info.sak == 0x00 && pcsc->part3_pass_thru(data_out, part3_resp)))
				{
					build_response({res, static_cast<u8>(status::success)});
				}
				else
				{
					build_response({res, static_cast<u8>(status::timeout)});
				}
				break;
			}
			else if (card_info.is_mifare && is_write_cmd)
			{
				build_response(); // UM0701-02 §7.3.8: Syntax error on malformed MIFARE write length
				break;
			}
			else if (!card_info.is_mifare && data_out.size() >= 2 && data_out[0] == data_out.size())
			{
				std::vector<u8> felica_resp;
				if (pcsc->felica_pass_thru(data_out, felica_resp))
				{
					std::vector<u8> resp = {res, static_cast<u8>(status::success)};
					resp.insert(resp.end(), felica_resp.begin(), felica_resp.end());
					build_response(resp);
				}
				else
				{
					build_response({res, static_cast<u8>(status::timeout)});
				}
				break;
			}
			else
			{
				std::vector<u8> card_resp;
				if (pcsc->part3_pass_thru(data_out, card_resp))
				{
					std::vector<u8> resp = {res, static_cast<u8>(status::success)};
					resp.insert(resp.end(), card_resp.begin(), card_resp.end());
					build_response(resp);
				}
				else
				{
					build_response({res, static_cast<u8>(status::timeout)});
				}
				break;
			}
		}

		if (!card_tapped || m_initiator_state == initiator_target_state::released)
		{
			reset_target_state();
			m_last_error = static_cast<u8>(status::context_error);
			build_response({res, static_cast<u8>(status::context_error)});
			break;
		}

		if (m_initiator_state == initiator_target_state::deselected)
		{
			// UM0701-02 §7.3.8: Automatically re-select deselected target before data exchange
			m_initiator_state = initiator_target_state::selected;
		}

		const auto card_t = get_card_type(card);

		if (card_t == card_type::iso14443_4 || card_t == card_type::iso14443_3b)
		{
			const std::span<const u8> data_out = (payload.size() > 2) ? payload.subspan(2) : std::span<const u8>{};
			if (!data_out.empty() && (data_out[0] == static_cast<u8>(mifare_cmd::auth_key_a) || data_out[0] == static_cast<u8>(mifare_cmd::auth_key_b)) &&
				!(data_out[0] == 0x60 && data_out.size() == 1))
			{
				reset_card_session();
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}
			// UM0701-02 §7.3.8: ISO/IEC 14443-4 / ISO/IEC 14443-3B C-APDU exchange -> R-APDU response
			build_response({res, static_cast<u8>(status::success), 0x90, 0x00});
			break;
		}

		if (card_t == card_type::felica)
		{
			// UM0701-02 §7.3.8: FeliCa data exchange
			build_response({res, static_cast<u8>(status::success)});
			break;
		}

		if (card_t == card_type::nfcip_dep)
		{
			// UM0701-02 §7.3.8: DEP data exchange
			build_response({res, static_cast<u8>(status::success)});
			break;
		}

		if (card_t == card_type::jewel)
		{
			// UM0701-02 §7.3.8: Jewel tag data exchange
			build_response({res, static_cast<u8>(status::success)});
			break;
		}

		if (payload.size() < sizeof(in_data_exchange_request))
		{
			build_response(); // Syntax error: frame too short for Mifare command and block (UM0701-02 §7.3.8)
			break;
		}

		const auto req = read_from_ptr<in_data_exchange_request>(payload);
		const usz total_blocks = is_4k_card(card) ? mifare_4k_blocks : mifare_1k_blocks;

		switch (static_cast<u8>(req.mifare))
		{
		case static_cast<u8>(mifare_cmd::auth_key_a):
		case static_cast<u8>(mifare_cmd::auth_key_b):
		{
			if (payload.size() != sizeof(mifare_auth_request))
			{
				build_response(); // Syntax error: invalid auth request frame size
				break;
			}

			if (get_card_type(card) == card_type::mifare_ultralight)
			{
				// MIFARE Ultralight (MF0ICU1) does not support Crypto1 authentication commands (0x60 / 0x61).
				// Card remains mute, leading to RF transmission timeout.
				build_response({res, static_cast<u8>(status::timeout)});
				break;
			}

			const auto auth_req = read_from_ptr<mifare_auth_request>(payload);
			const u8 block = auth_req.block;

			// Verify block boundary for 1K (64 blocks) or 4K (256 blocks)
			if (block >= total_blocks)
			{
				reset_card_session();
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			const auto info = get_mifare_block_info(block);
			const auto trailer = get_block_data(card, info.trailer_block);

			// MF1S503x Section 8.6.2: Corrupt access bits lock the entire sector
			if (!are_sector_access_bits_valid(trailer))
			{
				reset_card_session();
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			const auto trailer_bits = get_access_bits(trailer, 3);

			const auto card_uid = get_card_uid(card);
			if (!card_uid || auth_req.uid != *card_uid)
			{
				reset_card_session();
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			const bool is_key_a = (auth_req.mifare == mifare_cmd::auth_key_a);
			const u8 c_trailer = (trailer_bits.c1 << 2) | (trailer_bits.c2 << 1) | trailer_bits.c3;
			// Table 6: In configurations 000, 001, 010, Key B is readable as data and cannot authenticate
			const bool key_can_auth = is_key_a || (c_trailer != 0 && c_trailer != 1 && c_trailer != 2);

			bool auth_success = false;
			if (key_can_auth)
			{
				std::array<u8, 6> expected_key{};
				std::memcpy(expected_key.data(), trailer.data() + (is_key_a ? 0 : 10), 6);

				// Allow factory default FF..FF when trailer key bytes are uninitialized (all zeros)
				const bool is_zero_key = std::all_of(expected_key.begin(), expected_key.end(), [](u8 b) { return b == 0x00; });
				constexpr std::array<u8, 6> default_transport_key{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

				if (auth_req.key == expected_key || (is_zero_key && auth_req.key == default_transport_key))
				{
					auth_success = true;
					m_authenticated_sector = info.sector;
					m_authenticated_key = is_key_a ? auth_key::key_a : auth_key::key_b;
				}
			}

			if (auth_success)
			{
				build_response({res, static_cast<u8>(status::success)});
			}
			else
			{
				reset_card_session();
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
			}
			break;
		}
		case static_cast<u8>(mifare_cmd::read_16_bytes):
		{
			if (payload.size() != sizeof(in_data_exchange_request))
			{
				build_response(); // Syntax error: invalid read request frame size
				break;
			}

			// MIFARE Ultralight (MF0ICU1 §8.5.1): unauthenticated read of 4 pages (16 bytes) with page roll-over
			if (get_card_type(card) == card_type::mifare_ultralight)
			{
				const u8 page = req.block;
				if (page >= 16 || card.size() < 64)
				{
					build_response({res, static_cast<u8>(status::timeout)});
					break;
				}

				in_data_exchange_response resp{};
				resp.status = static_cast<u8>(status::success);
				for (usz i = 0; i < 4; ++i)
				{
					const usz src_page = (page + i) % 16;
					std::memcpy(resp.block.data() + i * 4, card.data() + src_page * 4, 4);
				}
				build_response(to_bytes(resp));
				break;
			}

			const auto check = check_mifare_block_access(card, req.block, true, true);
			if (!check.ok())
			{
				build_response({res, static_cast<u8>(check.error)});
				break;
			}

			if (!check.info.is_trailer)
			{
				// Data Block: Table 7 access conditions
				// Key A: can read if C in {0, 1, 2, 4, 6}
				// Key B: can read if C in {0, 1, 2, 3, 4, 5, 6} (Never if C == 7)
				bool can_read = false;
				if (m_authenticated_key == auth_key::key_a)
				{
					can_read = (check.c == 0 || check.c == 1 || check.c == 2 || check.c == 4 || check.c == 6);
				}
				else if (m_authenticated_key == auth_key::key_b)
				{
					can_read = (check.c != 7);
				}

				if (!can_read)
				{
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
					break;
				}

				const auto block_data = get_block_data(card, req.block);
				in_data_exchange_response resp{};
				resp.status = static_cast<u8>(status::success);
				std::memcpy(resp.block.data(), block_data.data(), mifare_block_size);
				build_response(to_bytes(resp));
			}
			else
			{
				// Sector Trailer: Table 6 access conditions
				// Key A: NEVER readable
				// Access Bits: readable by Key A or Key B in all valid states
				// Key B: readable by Key A or Key B only if C in {0, 1, 2}
				const auto trailer = get_block_data(card, check.info.trailer_block);
				in_data_exchange_response resp{};
				resp.status = static_cast<u8>(status::success);

				// Key A is always blanked out (zeros)
				std::fill_n(resp.block.begin(), 6, 0x00);

				// Access bits (bytes 6..9)
				std::memcpy(resp.block.data() + 6, trailer.data() + 6, 4);

				// Key B (bytes 10..15): readable only if C in {0, 1, 2}
				if (check.c <= 2)
				{
					std::memcpy(resp.block.data() + 10, trailer.data() + 10, 6);
				}
				else
				{
					std::fill_n(resp.block.begin() + 10, 6, 0x00);
				}

				build_response(to_bytes(resp));
			}
			break;
		}
		case static_cast<u8>(mifare_cmd::write_16_bytes):
		{
			if (payload.size() != 4 + mifare_block_size)
			{
				build_response(); // Syntax error: invalid write 16 bytes frame size
				break;
			}

			// MIFARE Ultralight Compatibility Write (MF0ICU1 §8.5.2):
			// Unauthenticated write of 16 bytes, but only the first 4 bytes are written to the page.
			// The remaining 12 bytes are ignored. Pages 0 and 1 are read-only.
			if (get_card_type(card) == card_type::mifare_ultralight)
			{
				const u8 page = req.block;
				const usz page_offset = static_cast<usz>(page) * 4;
				if (page < 2 || page >= 16 || page_offset + 4 > card.size())
				{
					build_response({res, static_cast<u8>(status::timeout)});
					break;
				}

				std::memcpy(card.data() + page_offset, payload.data() + 4, 4);
				build_response({res, static_cast<u8>(status::success)});
				break;
			}

			const auto check = check_mifare_block_access(card, req.block, true, false);
			if (!check.ok())
			{
				build_response({res, static_cast<u8>(check.error)});
				break;
			}

			if (!check.info.is_trailer)
			{
				// Table 7 Data block write:
				// C=0 (000): Key A | B
				// C=3 (011): Key B
				// C=4 (100): Key B
				// C=6 (110): Key B
				// C=1, 2, 5, 7: Never
				bool can_write = false;
				if (m_authenticated_key == auth_key::key_a)
				{
					can_write = (check.c == 0);
				}
				else if (m_authenticated_key == auth_key::key_b)
				{
					can_write = (check.c == 0 || check.c == 3 || check.c == 4 || check.c == 6);
				}

				if (!can_write)
				{
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
					break;
				}

				std::memcpy(card.data() + check.offset, payload.data() + 4, mifare_block_size);
				build_response({res, static_cast<u8>(status::success)});
			}
			else
			{
				// Table 6 Sector trailer write:
				// Key A write: C in {0, 1} with Key A; C in {3, 4} with Key B
				// Access bits write: C in {3, 5} with Key B; C == 1 with Key A
				// Key B write: C in {0, 1} with Key A; C in {3, 4} with Key B
				bool can_write_key_a = false;
				bool can_write_access = false;
				bool can_write_key_b = false;

				if (m_authenticated_key == auth_key::key_a)
				{
					can_write_key_a = (check.c == 0 || check.c == 1);
					can_write_access = (check.c == 1);
					can_write_key_b = (check.c == 0 || check.c == 1);
				}
				else if (m_authenticated_key == auth_key::key_b)
				{
					can_write_key_a = (check.c == 3 || check.c == 4);
					can_write_access = (check.c == 3 || check.c == 5);
					can_write_key_b = (check.c == 3 || check.c == 4);
				}

				if (!can_write_key_a && !can_write_access && !can_write_key_b)
				{
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
					break;
				}

				if (can_write_key_a)
				{
					std::memcpy(card.data() + check.offset, payload.data() + 4, 6);
				}
				if (can_write_access)
				{
					std::memcpy(card.data() + check.offset + 6, payload.data() + 10, 4);

					// Sector corruption simulation: MIFARE Classic writes invalid access bits into EEPROM (write succeeds),
					// but all blocks become permanently inaccessible and the active auth session is immediately lost.
					if (!are_sector_access_bits_valid(get_block_data(card, check.info.trailer_block)))
					{
						reset_card_session();
					}
				}
				if (can_write_key_b)
				{
					std::memcpy(card.data() + check.offset + 10, payload.data() + 14, 6);
				}

				build_response({res, static_cast<u8>(status::success)});
			}
			break;
		}
		case static_cast<u8>(mifare_cmd::write_4_bytes):
		{
			// MIFARE Ultralight (MF0ICU1) only; MIFARE Classic returns timeout
			if (payload.size() != 8)
			{
				build_response();
				break;
			}

			if (get_card_type(card) != card_type::mifare_ultralight)
			{
				build_response({res, static_cast<u8>(status::timeout)});
				break;
			}

			const u8 page = req.block;
			const usz page_offset = static_cast<usz>(page) * 4;

			// Pages 0..1 are read-only; upper bound is 16 pages
			if (page < 2 || page >= 16 || page_offset + 4 > card.size())
			{
				build_response({res, static_cast<u8>(status::timeout)});
				break;
			}

			std::memcpy(card.data() + page_offset, payload.data() + 4, 4);
			build_response({res, static_cast<u8>(status::success)});
			break;
		}
		case static_cast<u8>(mifare_cmd::increment):
		case static_cast<u8>(mifare_cmd::decrement):
		{
			if (payload.size() != 8)
			{
				build_response();
				break;
			}

			const auto check = check_mifare_block_access(card, req.block, false, false);
			if (!check.ok())
			{
				build_response({res, static_cast<u8>(check.error)});
				break;
			}

			const bool is_inc = (payload[2] == 0xC1);
			const bool can_do = is_inc
				? ((m_authenticated_key == auth_key::key_a && check.c == 0) ||
				   (m_authenticated_key == auth_key::key_b && (check.c == 0 || check.c == 6)))
				: (check.c == 0 || check.c == 1 || check.c == 6);

			if (!can_do)
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			s32 current_val = 0;
			u8 current_addr = 0;
			if (!get_value_from_block(std::span<const u8>(card.data() + check.offset, mifare_block_size), current_val, current_addr))
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			const s32 operand = read_from_ptr<le_t<s32>>(payload, 4);
			const s32 new_val = static_cast<s32>(is_inc
				? (static_cast<u32>(current_val) + static_cast<u32>(operand))
				: (static_cast<u32>(current_val) - static_cast<u32>(operand)));

			m_transfer_buffer = format_value_block(new_val, current_addr);
			m_transfer_buffer_valid = true;

			build_response({res, static_cast<u8>(status::success)});
			break;
		}
		case static_cast<u8>(mifare_cmd::restore):
		{
			// Accepts 4 bytes (no operand) or 8 bytes (with 4 zero operand bytes, libnfc compatibility)
			if (payload.size() != 4 && payload.size() != 8)
			{
				build_response();
				break;
			}

			const auto check = check_mifare_block_access(card, req.block, false, false);
			if (!check.ok())
			{
				build_response({res, static_cast<u8>(check.error)});
				break;
			}

			// Table 7: Restore allowed by Key A or Key B when C in {0, 1, 6}
			if (check.c != 0 && check.c != 1 && check.c != 6)
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			s32 current_val = 0;
			u8 current_addr = 0;
			if (!get_value_from_block(std::span<const u8>(card.data() + check.offset, mifare_block_size), current_val, current_addr))
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			std::memcpy(m_transfer_buffer.data(), card.data() + check.offset, mifare_block_size);
			m_transfer_buffer_valid = true;

			build_response({res, static_cast<u8>(status::success)});
			break;
		}
		case static_cast<u8>(mifare_cmd::transfer):
		{
			if (payload.size() != 4)
			{
				build_response();
				break;
			}

			if (!m_transfer_buffer_valid)
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			const auto check = check_mifare_block_access(card, req.block, false, false);
			if (!check.ok())
			{
				build_response({res, static_cast<u8>(check.error)});
				break;
			}

			// Table 7: Transfer allowed by Key A or Key B when C in {0, 1, 6}
			if (check.c != 0 && check.c != 1 && check.c != 6)
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			std::memcpy(card.data() + check.offset, m_transfer_buffer.data(), mifare_block_size);

			// In MIFARE Classic hardware (MF1S503x §8.6.2.1), the transfer buffer retains its content
			// until overwritten by another Increment, Decrement or Restore command.
			build_response({res, static_cast<u8>(status::success)});
			break;
		}
		default:
		{
			// UM0701-02 §7.3.8: If Cmd value is incorrect, the PN532 returns a Syntax Error.
			build_response();
			break;
		}
		}
		break;
	}
	case command_code::in_communicate_thru:
	{
		// -------------------------------------------------------------------------
		// 7.3.9 InCommunicateThru (0x42)
		// Direct RF raw transceiver data exchange without high-level protocol handling.
		// PN532 automatically calculates CRC for transmission and validates CRC on reception.
		//
		// Requires further hardware implementation (Raw RF contactless data transmit/receive)
		//
		// Expected Request Frame Parameters:
		//   Length: 1 to 265 bytes
		//   Data:   [0] 0x42 (Command Code)
		//           [1..n] DataOut: Raw RF payload (max 264 bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 to 265 bytes
		//   Data:   [0] 0x43 (Response Code)
		//           [1] Status: RF transaction status (0x00 = Success, 0x01 = Timeout)
		//           [2..m] DataIn: Raw data received from target (CRC stripped)
		// -------------------------------------------------------------------------
		if (payload.size() < 1 || payload.size() > 265)
		{
			build_response();
			break;
		}

		if (is_pcsc)
		{
			const auto card_info = pcsc->get_card_info(false);
			if (!card_info.present)
			{
				build_response({res, static_cast<u8>(status::timeout)});
				break;
			}

			const std::span<const u8> data_out = payload.subspan(1);
			std::vector<u8> card_resp;
			const bool ok = card_info.is_felica
				? pcsc->felica_pass_thru(data_out, card_resp)
				: pcsc->part3_pass_thru(data_out, card_resp);
			if (ok)
			{
				std::vector<u8> resp = {res, static_cast<u8>(status::success)};
				resp.insert(resp.end(), card_resp.begin(), card_resp.end());
				build_response(resp);
			}
			else
			{
				build_response({res, static_cast<u8>(status::timeout)});
			}
			break;
		}

		if (!card_tapped)
		{
			build_response({res, static_cast<u8>(status::timeout)});
		}
		else
		{
			build_response({res, static_cast<u8>(status::success)});
		}
		break;
	}
	case command_code::in_deselect:
	{
		// -------------------------------------------------------------------------
		// 7.3.10 InDeselect (0x44)
		// Deselects target(s) while retaining target context in PN532 internal memory.
		// Sends SLP_REQ / DSL_REQ / S(DESELECT) over the RF field.
		//
		// Expected Request Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x44 (Command Code)
		//           [1] Tg: Target logical number (0x00 = all targets, 0x01..0x02 = specific)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x45 (Response Code)
		//           [1] Status: 0x00 = Success, 0x27 = Unknown target
		// -------------------------------------------------------------------------
		if (payload.size() < 2 || payload.size() > 3 || payload[1] > 0x02)
		{
			build_response();
			break;
		}

		if (is_pcsc)
		{
			// Preserve PC/SC card handle and cached card info (UM0701-02 §7.3.10 keeps target context for InSelect)
			reset_card_session();
			m_initiator_state = initiator_target_state::deselected;
			build_response({res, static_cast<u8>(status::success)});
			break;
		}

		if (!card_tapped || m_initiator_state == initiator_target_state::released)
		{
			build_response({res, static_cast<u8>(status::context_error)});
			break;
		}

		reset_card_session();
		m_initiator_state = initiator_target_state::deselected;
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::in_jump_for_psl:
	case command_code::in_jump_for_dep:
	{
		// -------------------------------------------------------------------------
		// 7.3.4 InJumpForPSL (0x46)
		// Activates a target in active or passive mode for subsequent PSL parameter negotiation.
		//
		// Requires further hardware implementation (RF P2P PSL Initiator activation)
		//
		// Expected Request Frame Parameters:
		//   Length: 4 to 67 bytes
		//   Data:   [0] 0x46 (Command Code)
		//           [1] ActPass: 0x00=Passive mode, 0x01=Active mode
		//           [2] BR: Baud Rate (0x00=106k, 0x01=212k, 0x02=424k)
		//           [3] Next: Bitmask (Bit 0: PassiveInitiatorData, Bit 1: NFCID3i, Bit 2: Gi)
		//           [4..n] Optional parameters:
		//                    PassiveInitiatorData (4B ID or 5B Polling request payload)
		//                    NFCID3i (10B initiator identifier)
		//                    Gi (0..48B general bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: 18 to 65 bytes (success) or 2 bytes (failure)
		//   Data:   [0] 0x47 (Response Code)
		//           [1] Status: 0x00 = Success, 0x01 = Timeout
		//           [2] Tg: Logical target number (0x01)
		//           [3..12] NFCID3t: 10 bytes target NFCID3
		//           [13] DIDt, [14] BSt, [15] BRt, [16] TO, [17] PPt, [18..m] Gt
		// -------------------------------------------------------------------------
		// 7.3.3 InJumpForDEP (0x56)
		// Activates a target in active or passive mode directly for DEP exchanges.
		// Handles RF collision resolution and ATR_REQ/ATR_RES internally.
		//
		// Requires further hardware implementation (RF P2P DEP Initiator activation)
		//
		// Expected Request Frame Parameters:
		//   Length: 4 to 67 bytes
		//   Data:   [0] 0x56 (Command Code)
		//           [1] ActPass: 0x00=Passive mode, 0x01=Active mode
		//           [2] BR: Baud Rate (0x00=106k, 0x01=212k, 0x02=424k)
		//           [3] Next: Bitmask (Bit 0: PassiveInitiatorData, Bit 1: NFCID3i, Bit 2: Gi)
		//           [4..n] Optional parameters:
		//                    PassiveInitiatorData (4B ID or 5B Polling request payload)
		//                    NFCID3i (10B initiator identifier)
		//                    Gi (0..48B general bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: 18 to 65 bytes (success) or 2 bytes (failure)
		//   Data:   [0] 0x57 (Response Code)
		//           [1] Status: 0x00 = Success, 0x01 = Timeout
		//           [2] Tg: Logical target number (0x01)
		//           [3..12] NFCID3t: 10 bytes target NFCID3
		//           [13] DIDt, [14] BSt, [15] BRt, [16] TO, [17] PPt, [18..m] Gt
		// -------------------------------------------------------------------------
		if (payload.size() < 4)
		{
			build_response();
			break;
		}

		const u8 act_pass = payload[1];
		const u8 br = payload[2];
		const u8 next = payload[3];

		if (act_pass > 0x01 || br > 0x02 || (next & 0xF8) != 0)
		{
			build_response();
			break;
		}

		// Passive 212/424 kbps requires PassiveInitiatorData (UM0701-02 §7.3.3/7.3.4)
		if (act_pass == 0x00 && br > 0 && !(next & 0x01))
		{
			build_response();
			break;
		}

		usz expected_len = 4;
		if (next & 0x01)
		{
			expected_len += (br == 0) ? 4 : 5;

			if (br > 0)
			{
				if (payload.size() < expected_len)
				{
					build_response();
					break;
				}
				const u8 tsn = payload[8]; // payload[4..8] is PassiveInitiatorData
				if (tsn != 0x00 && tsn != 0x01 && tsn != 0x03 && tsn != 0x07 && tsn != 0x0F)
				{
					build_response();
					break;
				}
			}
		}

		if (next & 0x02)
		{
			expected_len += 10; // NFCID3i
		}

		if (payload.size() < expected_len || payload.size() > expected_len + 48)
		{
			build_response();
			break;
		}

		build_dep_activation_response(res, true, is_pcsc, card);
		break;
	}
	case command_code::in_list_passive_target:
	{
		// -------------------------------------------------------------------------
		// 7.3.5 InListPassiveTarget (0x4A)
		// Detects and activates up to MaxTg passive targets (ISO14443A/B, FeliCa, Jewel).
		// Emulates Mifare Classic 1K (SAK=0x08, ATQA=0x0004) or 4K (SAK=0x18, ATQA=0x0002).
		//
		// Expected Request Frame Parameters:
		//   Length: 3 to 14 bytes
		//   Data:   [0] 0x4A (Command Code)
		//           [1] MaxTg: Maximum number of targets to initialize (1 or 2; Jewel=1)
		//           [2] BrTy: Baud rate and modulation type (0x00 = 106 kbps Type A)
		//           [3..n] InitiatorData (optional)
		//
		// Expected Response Frame Parameters:
		//   Length: Variable (2 bytes when NbTg=0, 12 bytes for 1 Type A target)
		//   Data:   [0] 0x4B (Response Code)
		//           [1] NbTg: Number of initialized targets (0..2)
		//           TargetData: Tg(1B) SENS_RES(2B) SEL_RES(1B) NFCIDLen(1B) NFCID1(NFCIDLen)
		// -------------------------------------------------------------------------
		if (payload.size() < 3 || payload[1] == 0 || payload[1] > 2 || payload[2] > 4 || (payload[2] == 4 && payload[1] > 1))
		{
			build_response();
			break;
		}

		const u8 in_list_br_ty = payload[2];
		if (in_list_br_ty == 0x01 || in_list_br_ty == 0x02) // FeliCa 212/424 kbps: mandatory 5 bytes InitiatorData
		{
			if (payload.size() != 8)
			{
				build_response(); // Syntax error: missing or invalid InitiatorData
				break;
			}
			const u8 tsn = payload[7]; // payload[3..7] is InitiatorData
			if (tsn != 0x00 && tsn != 0x01 && tsn != 0x03 && tsn != 0x07 && tsn != 0x0F)
			{
				build_response(); // Syntax error: bad TSN
				break;
			}
		}
		else if (in_list_br_ty == 0x03) // ISO14443-3B: mandatory AFI (1B), optional Polling Method (1B)
		{
			if (payload.size() < 4 || payload.size() > 5)
			{
				build_response(); // Syntax error: AFI parameter missing or extra data
				break;
			}
		}
		else if (in_list_br_ty == 0x04) // Innovision Jewel: InitiatorData not used
		{
			if (payload.size() != 3)
			{
				build_response(); // Syntax error: unexpected InitiatorData
				break;
			}
		}

		if (is_pcsc)
		{
			const auto card_info = pcsc->get_card_info();
			if (!card_info.present)
			{
				reset_target_state();
				build_response({res, 0x00});
				break;
			}

			if (in_list_br_ty == 0x00) // 106 kbps Type A
			{
				if (card_info.is_felica || card_info.is_type_b)
				{
					build_response({res, 0x00});
					break;
				}

				if (card_info.uid.size() < mifare_uid_size)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				const std::span<const u8> target_uid{card_info.uid};

				if (payload.size() > 3 && !matches_type_a_uid_filter(payload.subspan(3), target_uid))
				{
					build_response({res, 0x00});
					break;
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;

				std::vector<u8> resp = {
					res,
					0x01, // NbTg
					0x01, // Tg
					static_cast<u8>(card_info.atqa & 0xFF),
					static_cast<u8>((card_info.atqa >> 8) & 0xFF),
					card_info.sak,
					static_cast<u8>(target_uid.size())
				};
				resp.insert(resp.end(), target_uid.begin(), target_uid.end());

				if ((m_parameters_flags.load(std::memory_order_relaxed) & (1 << 4)) && card_info.is_iso14443_4)
				{
					append_iso14443_4_ats(resp, card_info);
				}

				build_response(resp);
			}
			else if (in_list_br_ty == 0x01 || in_list_br_ty == 0x02) // FeliCa 212 / 424 kbps
			{
				if (!card_info.is_felica)
				{
					build_response({res, 0x00});
					break;
				}

				const u8 sys_msb = payload[4];
				const u8 sys_lsb = payload[5];
				const u8 req_code = payload[6];
				const u8 tsn = payload[7];
				const u16 req_sys_code = static_cast<u16>((sys_msb << 8) | sys_lsb);

				std::vector<u8> pol_res;
				const std::array<u8, 6> poll_cmd = {0x06, 0x00, sys_msb, sys_lsb, req_code, tsn};
				if (pcsc->felica_pass_thru(poll_cmd, pol_res) && (pol_res.size() == 18 || pol_res.size() == 20) && pol_res[1] == 0x01)
				{
					reset_card_session();
					m_initiator_state = initiator_target_state::selected;

					std::vector<u8> resp = {
						res,
						0x01, // NbTg: 1
						0x01  // Tg: 1
					};
					resp.insert(resp.end(), pol_res.begin(), pol_res.end());
					build_response(resp);
					break;
				}

				if (req_sys_code != 0xFFFF && (card_info.system_code == 0xFFFF || req_sys_code != card_info.system_code))
				{
					build_response({res, 0x00});
					break;
				}

				if (card_info.uid.size() < 8)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;

				const bool include_syscode = (req_code != 0x00);
				const u8 pol_res_len = include_syscode ? 0x14 : 0x12;

				std::vector<u8> resp = {
					res,
					0x01,        // NbTg: 1
					0x01,        // Tg: 1
					pol_res_len, // POL_RES len: 18 or 20 bytes
					0x01         // Response code
				};

				resp.insert(resp.end(), card_info.uid.begin(), card_info.uid.begin() + 8);

				if (card_info.pmm.size() >= 8)
				{
					resp.insert(resp.end(), card_info.pmm.begin(), card_info.pmm.begin() + 8);
				}
				else
				{
					resp.insert(resp.end(), default_felica_pmm.begin(), default_felica_pmm.end());
				}

				if (include_syscode)
				{
					resp.push_back(static_cast<u8>((card_info.system_code >> 8) & 0xFF));
					resp.push_back(static_cast<u8>(card_info.system_code & 0xFF));
				}

				build_response(resp);
			}
			else if (in_list_br_ty == 0x03) // ISO/IEC 14443-3B (106 kbps)
			{
				if (!card_info.is_type_b)
				{
					build_response({res, 0x00});
					break;
				}

				if (card_info.uid.size() < 4)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				std::vector<u8> resp = {res, 0x01};
				if (!append_type_b_target(resp, 0x01, card_info))
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;
				build_response(resp);
			}
			else
			{
				build_response({res, 0x00});
			}
			break;
		}

		if (card_tapped)
		{
			const auto card_t = get_card_type(card);

			if (in_list_br_ty == 0x00) // 106 kbps Type A
			{
				if (card_t != card_type::mifare_classic && card_t != card_type::mifare_ultralight && card_t != card_type::iso14443_4)
				{
					// Non-Type A card does not respond to 106 kbps Type A polling
					build_response({res, 0x00});
					break;
				}

				std::array<u8, 7> uid_buf{};
				const auto uid_bytes = get_card_uid_bytes(card, uid_buf);
				if (uid_bytes.empty())
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				if (payload.size() > 3 && !matches_type_a_uid_filter(payload.subspan(3), uid_bytes))
				{
					// Card UID does not match requested UID: no target detected
					build_response({res, 0x00});
					break;
				}

				if (card_t == card_type::mifare_ultralight)
				{
					reset_card_session(); // Reset Crypto1/auth state for new card activation (real PN532 resets implicitly during anti-collision)
					m_initiator_state = initiator_target_state::selected;

					// MIFARE Ultralight: SENS_RES=0x0044 (wire: 0x44, 0x00), SEL_RES=0x00, UID length=7
					std::vector<u8> resp = {
						res,
						0x01,                             // NbTg: 1 target
						0x01,                             // Tg: Target 1
						0x44, 0x00,                       // SENS_RES (ATQA) on wire: 44 00 (double size UID)
						0x00,                             // SEL_RES (SAK) for Ultralight
						static_cast<u8>(uid_bytes.size()) // NFCIDLength: 7 bytes
					};
					resp.insert(resp.end(), uid_bytes.begin(), uid_bytes.end());
					build_response(resp);
				}
				else if (card_t == card_type::iso14443_4)
				{
					reset_card_session();
					m_initiator_state = initiator_target_state::selected;

					// ISO/IEC 14443-4 Type A (e.g. MIFARE DESFire): SENS_RES=0x0344 (wire: 0x44, 0x03), SAK=0x20
					// Per UM0701-02 §7.3.5 Page 116 & 118, if the target indicates ISO/IEC 14443-4 compliance,
					// the PN532 performs RATS activation and appends the ATS response frame [ATSLength, ...]
					// ATS is only appended if fAutomaticRATS (bit 4 of m_parameters_flags) is set (UM0701-02 §7.2.9 p.87)
					std::vector<u8> resp = {
						res,
						0x01,                             // NbTg: 1 target
						0x01,                             // Tg: Target 1
						0x44, 0x03,                       // SENS_RES (ATQA) on wire: 44 03
						0x20,                             // SEL_RES (SAK: ISO/IEC 14443-4 PICC compliant)
						static_cast<u8>(uid_bytes.size()) // NFCIDLength
					};
					resp.insert(resp.end(), uid_bytes.begin(), uid_bytes.end());

					if (m_parameters_flags.load(std::memory_order_relaxed) & (1 << 4))
					{
						// ATS response (6 bytes: TL=0x06, T0=0x75, TA1=0x77, TB1=0x81, TC1=0x02, T1=0x80)
						const std::array<u8, 6> ats = {0x06, 0x75, 0x77, 0x81, 0x02, 0x80};
						resp.insert(resp.end(), ats.begin(), ats.end());
					}

					build_response(resp);
				}
				else
				{
					if (uid_bytes.size() < mifare_uid_size)
					{
						reset_target_state();
						build_response({res, 0x00});
						break;
					}

					reset_card_session();
					m_initiator_state = initiator_target_state::selected;

					in_list_passive_target_response resp{};
					std::memcpy(resp.nfcid.data(), uid_bytes.data(), mifare_uid_size);

					if (is_4k_card(card))
					{
						resp.sens_res = {0x02, 0x00}; // SENS_RES (ATQA) for Mifare Classic 4K (UM0701-02 §7.3.5 Page 120)
						resp.sel_res  = 0x18;         // SEL_RES (SAK) for Mifare Classic 4K
					}
					else
					{
						resp.sens_res = {0x04, 0x00}; // SENS_RES (ATQA) for Mifare Classic 1K (UM0701-02 §7.3.5 Page 120)
						resp.sel_res  = 0x08;         // SEL_RES (SAK) for Mifare Classic 1K
					}

					build_response(to_bytes(resp));
				}
			}
			else if (in_list_br_ty == 0x01 || in_list_br_ty == 0x02) // FeliCa 212 / 424 kbps
			{
				if (card_t != card_type::felica)
				{
					// Mifare/Type A card cannot respond to FeliCa 212/424 kbps polling
					build_response({res, 0x00});
					break;
				}

				if (card.size() < 8)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				std::array<u8, 8> idm{};
				std::array<u8, 8> pmm = default_felica_pmm;
				std::memcpy(idm.data(), card.data(), 8);
				if (card.size() >= 16)
				{
					std::memcpy(pmm.data(), card.data() + 8, 8);
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;

				// UM0701-02 §7.3.5 Page 116 & 120: FeliCa POL_RES format (18 or 20 bytes)
				// TargetData: [Tg(1B)] [POL_RES len(1B)] [0x01] [NFCID2t(8B)] [Pad(8B)] [SystemCode(2B)]
				std::vector<u8> resp = {
					res,
					0x01,       // NbTg: 1 target
					0x01,       // Tg: Target 1
					0x14,       // POL_RES length: 20 bytes (2 + 8 + 8 + 2)
					0x01        // Response code byte
				};
				resp.insert(resp.end(), idm.begin(), idm.end());
				resp.insert(resp.end(), pmm.begin(), pmm.end());
				resp.insert(resp.end(), {0xFF, 0xFF}); // System Code (2B)
				build_response(resp);
			}
			else if (in_list_br_ty == 0x04) // Innovision Jewel (106 kbps)
			{
				if (card_t != card_type::jewel)
				{
					// Non-Jewel card does not respond to Innovision Jewel polling
					build_response({res, 0x00});
					break;
				}

				const auto uid = get_card_uid(card);
				if (!uid)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;

				// UM0701-02 §7.3.5 Page 117 & 120: Jewel response format
				// TargetData: [Tg(1B)] [SENS_RES(2B)] [JEWELID(4B)]
				std::vector<u8> resp = {
					res,
					0x01,       // NbTg: 1 target
					0x01,       // Tg: Target 1
					0x04, 0x00  // SENS_RES (ATQA) for Jewel/Topaz per UM0701-02 §7.3.5 Page 121 Example 4
				};
				resp.insert(resp.end(), uid->begin(), uid->end());
				build_response(resp);
			}
			else if (in_list_br_ty == 0x03) // ISO/IEC 14443-3B (106 kbps)
			{
				if (card_t != card_type::iso14443_3b)
				{
					// Non-Type B card does not respond to ISO/IEC 14443-3B polling
					build_response({res, 0x00});
					break;
				}

				const auto uid = get_card_uid(card);
				if (!uid)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;

				// UM0701-02 §7.3.5 Page 116 & 121: ISO/IEC 14443-3B TargetData format
				// TargetData: [Tg(1B)] [ATQB(12B)] [ATTRIB_RES length(1B)] [ATTRIB_RES(1B...)]
				// ATQB: 50h, 4B PUPI, 4B Application Data, 3B Protocol Info
				// ATTRIB_RES length: 0x01, ATTRIB_RES: 0x01
				std::vector<u8> resp = {
					res,
					0x01,       // NbTg: 1 target
					0x01,       // Tg: Target 1
					0x50        // ATQB header
				};
				resp.insert(resp.end(), uid->begin(), uid->end()); // PUPI (4B)
				resp.insert(resp.end(), {0x00, 0x00, 0x00, 0x00}); // Application Data (4B)
				resp.insert(resp.end(), {0x00, 0x81, 0x71});       // Protocol Info (3B)
				resp.push_back(0x01);                              // ATTRIB_RES length (1B)
				resp.push_back(0x01);                              // ATTRIB_RES (1B)
				build_response(resp);
			}
			else
			{
				// Unsupported BrTy
				build_response({res, 0x00});
			}
		}
		else
		{
			build_response({res, 0x00}); // No targets found
		}
		break;
	}
	case command_code::in_psl:
	{
		// -------------------------------------------------------------------------
		// 7.3.7 InPSL (0x4E)
		// Changes bit rates with an activated TPE target (PSL_REQ) or ISO14443-4 target (PPS).
		//
		// Requires further hardware implementation (RF parameter selection PSL/PPS)
		//
		// Expected Request Frame Parameters:
		//   Length: 4 bytes
		//   Data:   [0] 0x4E (Command Code)
		//           [1] Tg: Logical target number (1 or 2)
		//           [2] BRit: Baud rate Initiator -> Target (0x00=106k, 0x01=212k, 0x02=424k)
		//           [3] BRti: Baud rate Target -> Initiator (0x00=106k, 0x01=212k, 0x02=424k)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x4F (Response Code)
		//           [1] Status: 0x00 = Success, 0x26 = Not allowed, 0x27 = Context error
		// -------------------------------------------------------------------------
		if (payload.size() != 4 || payload[1] < 0x01 || payload[1] > 0x02 || payload[2] > 0x02 || payload[3] > 0x02)
		{
			build_response();
			break;
		}

		if (card_tapped && m_initiator_state != initiator_target_state::released && payload[1] == 0x01)
		{
			build_response({res, static_cast<u8>(status::success)});
		}
		else
		{
			build_response({res, static_cast<u8>(status::context_error)});
		}
		break;
	}
	case command_code::in_atr:
	{
		// -------------------------------------------------------------------------
		// 7.3.6 InATR (0x50)
		// Activates a target previously found by InListPassiveTarget in passive mode using ATR_REQ.
		//
		// Requires further hardware implementation (RF P2P DEP ATR activation)
		//
		// Expected Request Frame Parameters:
		//   Length: 3 to 61 bytes
		//   Data:   [0] 0x50 (Command Code)
		//           [1] Tg: Target logical number
		//           [2] Next: Bitmask (Bit 0: NFCID3i, Bit 1: Gi)
		//           [3..n] NFCID3i (10B, optional), Gi (0..48B, optional)
		//
		// Expected Response Frame Parameters:
		//   Length: 17 to 64 bytes (success) or 2 bytes (failure)
		//   Data:   [0] 0x51 (Response Code)
		//           [1] Status: 0x00 = Success, 0x01 = Timeout, 0x27 = Unknown target
		//           [2..11] NFCID3t: 10 bytes target NFCID3
		//           [12] DIDt, [13] BSt, [14] BRt, [15] TO, [16] PPt, [17..m] Gt
		// -------------------------------------------------------------------------
		if (payload.size() < 3 || payload[1] < 0x01 || payload[1] > 0x02 || (payload[2] & 0xFC) != 0)
		{
			build_response();
			break;
		}

		const u8 atr_next = payload[2];
		const usz atr_min_len = 3 + ((atr_next & 0x01) ? 10 : 0);
		if (payload.size() < atr_min_len || payload.size() > atr_min_len + 48)
		{
			build_response();
			break;
		}

		const bool has_valid_target = is_pcsc
			? pcsc->get_card_info().present
			: card_tapped;
		if (!has_valid_target || m_initiator_state == initiator_target_state::released || payload[1] != 0x01)
		{
			build_response({res, static_cast<u8>(status::context_error)});
		}
		else
		{
			build_dep_activation_response(res, false, is_pcsc, card);
		}
		break;
	}
	case command_code::in_release:
	{
		// -------------------------------------------------------------------------
		// 7.3.11 InRelease (0x52)
		// Releases target(s) and erases their context from PN532 internal memory.
		// Sends HLTA / HLTB / DESELECT / RLS_REQ and switches off RF field if no more targets.
		//
		// Expected Request Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x52 (Command Code)
		//           [1] Tg: Target logical number (0x00 = all targets, 0x01..0x02 = specific)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x53 (Response Code)
		//           [1] Status: 0x00 = Success
		// -------------------------------------------------------------------------
		if (payload.size() < 2 || payload.size() > 3 || payload[1] > 0x02)
		{
			build_response();
			break;
		}

		if (is_pcsc)
		{
			pcsc->release_target(m_authenticated_sector >= 0);
		}

		reset_target_state();
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::in_select:
	{
		// -------------------------------------------------------------------------
		// 7.3.12 InSelect (0x54)
		// Re-selects a previously deselected target.
		// Sends WUPA/SELECT for Type A, WUPB/ATTRIB for Type B, or WUP_REQ for DEP.
		//
		// Expected Request Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x54 (Command Code)
		//           [1] Tg: Target logical number (1 or 2)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x55 (Response Code)
		//           [1] Status: 0x00 = Success, 0x27 = Context error / unknown target
		// -------------------------------------------------------------------------
		if (payload.size() != 2 || payload[1] < 0x01 || payload[1] > 0x02)
		{
			build_response();
			break;
		}

		if (is_pcsc)
		{
			if (m_initiator_state != initiator_target_state::released && payload[1] == 0x01 && pcsc->get_card_info().present)
			{
				m_initiator_state = initiator_target_state::selected;
				build_response({res, static_cast<u8>(status::success)});
			}
			else
			{
				reset_target_state();
				build_response({res, static_cast<u8>(status::context_error)});
			}
			break;
		}

		if (card_tapped && m_initiator_state != initiator_target_state::released)
		{
			m_initiator_state = initiator_target_state::selected;
			build_response({res, static_cast<u8>(status::success)});
		}
		else
		{
			build_response({res, static_cast<u8>(status::context_error)});
		}
		break;
	}
	case command_code::rf_regulation_test:
	{
		// -------------------------------------------------------------------------
		// 7.3.2 RFRegulationTest (0x58)
		// Radio regulation continuous transmission test with PRBS15 pseudo-random sequence.
		//
		// Requires further hardware implementation (Continuous RF transmission for radio regulation testing)
		//
		// Expected Request Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x58 (Command Code)
		//           [1] TxMode:
		//                 Bits 2..0: TxFraming (00: Mifare, 10: FeliCa)
		//                 Bits 6..4: TxSpeed (000: 106k, 001: 212k, 010: 424k, 011: 848k)
		//
		// Expected Response Frame Parameters:
		//   This command transmits continuously and never terminates on its own; no response frame is sent.
		// -------------------------------------------------------------------------
		// UM0701-02 §7.3.2: TxMode Bits 6..4 TxSpeed <= 3, Bits 1..0 TxFraming (bit 0 == 0), Bits 7, 3, 2 are 0
		if (payload.size() != 2 || (payload[1] & 0x8D) != 0 || ((payload[1] >> 4) & 0x07) > 3)
		{
			build_response();
			break;
		}

		m_exec_response.clear();
		break;
	}
	case command_code::in_auto_poll:
	{
		// -------------------------------------------------------------------------
		// 7.3.13 InAutoPoll (0x60)
		// Automatically polls for targets across specified types and periods.
		// Emulates Mifare Classic 1K (SAK=0x08) or 4K (SAK=0x18) when card_tapped == true.
		//
		// Expected Request Frame Parameters:
		//   Length: 4 to 18 bytes
		//   Data:   [0] 0x60 (Command Code)
		//           [1] PollNr: Number of polling cycles (0x01..0xFE, 0xFF=Endless)
		//           [2] Period: Polling interval in units of 150ms (0x01..0x0F)
		//           [3] Type 1: Mandatory first target type (0x00=Generic 106kA, 0x10=Mifare)
		//           [4..n] Type 2..N: Optional additional target types
		//
		// Expected Response Frame Parameters:
		//   Length: Variable (2 bytes if no targets found, 13 bytes for 1 Mifare target)
		//   Data:   [0] 0x61 (Response Code)
		//           [1] NbTg: Number of targets found (0..2)
		//           TargetData: Type(1B) Ln(1B) Tg(1B) SENS_RES(2B) SEL_RES(1B) NFCIDLen(1B) NFCID1
		// -------------------------------------------------------------------------
		if (payload.size() < 4 || payload.size() > 18 || payload[1] == 0x00 || payload[2] == 0x00 || payload[2] > 0x0F)
		{
			build_response();
			break;
		}

		const auto is_valid_autopoll_type = [](u8 t) {
			switch (t)
			{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: // Generic passive
			case 0x10: case 0x11: case 0x12:                         // Mifare / FeliCa
			case 0x20: case 0x23:                                     // ISO/IEC 14443-4A / 4B
			case 0x40: case 0x41: case 0x42:                         // DEP passive
			case 0x80: case 0x81: case 0x82:                         // DEP active
				return true;
			default:
				return false;
			}
		};

		bool has_invalid_type = false;
		for (usz i = 3; i < payload.size(); ++i)
		{
			if (!is_valid_autopoll_type(payload[i]))
			{
				has_invalid_type = true;
				break;
			}
		}

		if (has_invalid_type)
		{
			build_response(); // Syntax error: invalid Type value
			break;
		}

		const auto card_t = card_tapped ? get_card_type(card) : card_type::unknown;

		const bool polls_mifare = std::any_of(payload.begin() + 3, payload.end(), [](u8 t) {
			return t == 0x00 || t == 0x10;
		});
		const bool polls_tcl_a = std::any_of(payload.begin() + 3, payload.end(), [](u8 t) {
			return t == 0x00 || t == 0x20;
		});
		const bool polls_type_a = polls_mifare || polls_tcl_a;

		auto it_felica = std::find_if(payload.begin() + 3, payload.end(), [](u8 t)
		{
			return t == 0x01 || t == 0x11 || t == 0x02 || t == 0x12;
		});
		const bool polls_felica = (it_felica != payload.end());

		auto it_type_b = std::find_if(payload.begin() + 3, payload.end(), [](u8 t)
		{
			return t == 0x03 || t == 0x23;
		});
		const bool polls_type_b = (it_type_b != payload.end());

		auto it_dep = std::find_if(payload.begin() + 3, payload.end(), [](u8 t)
		{
			return t == 0x00 || t == 0x01 || t == 0x02 ||
				t == 0x40 || t == 0x41 || t == 0x42 ||
				t == 0x80 || t == 0x81 || t == 0x82;
		});
		const bool polls_dep = (it_dep != payload.end());

		if (is_pcsc)
		{
			const auto card_info = pcsc->get_card_info();
			if (!card_info.present)
			{
				reset_target_state();
				build_response({res, 0x00});
				break;
			}

			if (card_info.is_felica && polls_felica)
			{
				if (card_info.uid.size() < 8)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;

				const u8 felica_type = (*it_felica == 0x02 || *it_felica == 0x12 || (*it_felica != 0x11 && card_info.felica_speed == 2)) ? 0x12 : 0x11;
				// UM0701-02 §7.3.13 Page 145 & 149: AutoPollTargetData for FeliCa
				// Fixed polling uses ReqCode=0x00, so POL_RES length is 18 bytes (0x12): [0x01] [NFCID2t(8B)] [Pad(8B)]
				const u8 pol_res_len = 0x12; // 18 bytes: 0x01 + 8B IDm + 8B PMm
				const u8 ln = static_cast<u8>(1 + pol_res_len); // Tg(1B) + POL_RES(18B) = 19 (0x13)

				std::vector<u8> resp = {
					res,
					0x01,        // NbTg: 1
					felica_type, // Target type: FeliCa 212 kbps (0x11) or 424 kbps (0x12)
					ln,          // Ln 1: Length of AutoPollTargetData (19 bytes)
					0x01,        // Logical Number Tg = 1
					pol_res_len, // POL_RES length: 18 bytes
					0x01         // Response code: 0x01
				};
				resp.insert(resp.end(), card_info.uid.begin(), card_info.uid.begin() + 8);

				if (card_info.pmm.size() >= 8)
				{
					resp.insert(resp.end(), card_info.pmm.begin(), card_info.pmm.begin() + 8);
				}
				else
				{
					resp.insert(resp.end(), default_felica_pmm.begin(), default_felica_pmm.end());
				}

				build_response(resp);
				break;
			}
			else if (card_info.is_type_b && polls_type_b)
			{
				if (card_info.uid.size() < 4)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				const u8 type_b_type = (*it_type_b == 0x23 || (*it_type_b == 0x03 && card_info.is_iso14443_4)) ? 0x23 : 0x03;
				std::vector<u8> resp = {res, 0x01, type_b_type, 0x00};
				const usz payload_start = resp.size();
				if (!append_type_b_target(resp, 0x01, card_info))
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;
				resp[3] = static_cast<u8>(resp.size() - payload_start);

				build_response(resp);
				break;
			}
			else if (!card_info.is_felica && !card_info.is_type_b &&
				((card_info.is_iso14443_4 && polls_tcl_a) || (card_info.is_mifare && polls_mifare) || (!card_info.is_iso14443_4 && !card_info.is_mifare && polls_type_a)))
			{
				if (card_info.uid.size() < mifare_uid_size)
				{
					reset_target_state();
					build_response({res, 0x00});
					break;
				}

				reset_card_session();
				m_initiator_state = initiator_target_state::selected;

				const u8 uid_len = static_cast<u8>(card_info.uid.size());
				const bool auto_rats = (m_parameters_flags.load(std::memory_order_relaxed) & (1 << 4)) != 0;
				const bool report_iso14443_4 = card_info.is_iso14443_4 && polls_tcl_a &&
					(auto_rats || !card_info.is_mifare || !polls_mifare);

				if (report_iso14443_4)
				{
					std::vector<u8> resp = {
						res,
						0x01,    // NbTg: 1
						0x20,    // Target type: ISO/IEC 14443-4 Type A
						0x00,    // Ln 1 (filled below)
						0x01,    // Logical Number Tg = 1
						static_cast<u8>(card_info.atqa & 0xFF),
						static_cast<u8>((card_info.atqa >> 8) & 0xFF),
						card_info.sak,
						uid_len
					};
					resp.insert(resp.end(), card_info.uid.begin(), card_info.uid.end());
					if (auto_rats)
					{
						append_iso14443_4_ats(resp, card_info);
					}
					resp[3] = static_cast<u8>(resp.size() - 4);
					build_response(resp);
					break;
				}

				const u8 ln = static_cast<u8>(1 + 2 + 1 + 1 + uid_len); // Tg(1B) + SENS_RES(2B) + SEL_RES(1B) + NFCIDLength(1B) + UID

				std::vector<u8> resp = {
					res,
					0x01,    // NbTg: 1
					0x10,    // Target type: Mifare
					ln,      // Ln 1: Length of AutoPollTargetData
					0x01,    // Logical Number Tg = 1
					static_cast<u8>(card_info.atqa & 0xFF),
					static_cast<u8>((card_info.atqa >> 8) & 0xFF),
					card_info.sak,
					uid_len  // NFCID1 length
				};
				resp.insert(resp.end(), card_info.uid.begin(), card_info.uid.end());

				build_response(resp);
				break;
			}
			else
			{
				build_response({res, 0x00});
				break;
			}
		}

		if (card_tapped && polls_type_a && (card_t == card_type::mifare_classic || card_t == card_type::mifare_ultralight || card_t == card_type::iso14443_4))
		{
			std::array<u8, 7> uid_buf{};
			const auto uid_bytes = get_card_uid_bytes(card, uid_buf);
			if (uid_bytes.empty())
			{
				reset_target_state();
				build_response({res, 0x00});
				break;
			}

			reset_card_session(); // Reset Crypto1/auth state for new card activation (real PN532 resets implicitly during anti-collision)
			m_initiator_state = initiator_target_state::selected;
			const bool is_4k = is_4k_card(card);

			if (card_t == card_type::iso14443_4)
			{
				// ISO/IEC 14443-4 Type A (e.g. MIFARE DESFire)
				// UM0701-02 §7.3.13 Page 145: AutoPollTargetData for ISO14443-4 is identical to TargetData in §7.3.5
				const std::array<u8, 6> ats = {0x06, 0x75, 0x77, 0x81, 0x02, 0x80};
				const u8 nfcid_len = static_cast<u8>(uid_bytes.size());
				const u8 ln = static_cast<u8>(1 + 2 + 1 + 1 + nfcid_len + ats.size()); // Tg + SENS_RES + SEL_RES + NFCIDLen + UID + ATS

				std::vector<u8> resp = {
					res,
					0x01,       // NbTg: 1 target found
					0x20,       // Type 1: ISO/IEC 14443-4 Type A
					ln,         // Ln 1: Length of AutoPollTargetData
					0x01,       // Logical Number Tg = 1
					0x44, 0x03, // SENS_RES (ATQA) on wire
					0x20,       // SEL_RES (SAK: ISO/IEC 14443-4 PICC)
					nfcid_len   // NFCID1 length
				};
				resp.insert(resp.end(), uid_bytes.begin(), uid_bytes.end());
				resp.insert(resp.end(), ats.begin(), ats.end());
				build_response(resp);
			}
			else
			{
				const u8 sak = (card_t == card_type::mifare_ultralight) ? 0x00 : (is_4k ? 0x18 : 0x08);
				const u8 atqa_byte0 = (card_t == card_type::mifare_ultralight) ? 0x44 : (is_4k ? 0x02 : 0x04);
				const u8 atqa_byte1 = 0x00;
				const u8 nfcid_len = static_cast<u8>(uid_bytes.size());
				const u8 ln = static_cast<u8>(1 + 2 + 1 + 1 + nfcid_len); // Tg(1B) + SENS_RES(2B) + SEL_RES(1B) + NFCIDLength(1B) + UID

				std::vector<u8> resp = {
					res,
					0x01,                   // NbTg: 1 target found
					0x10,                   // Type 1: Mifare
					ln,                     // Ln 1: Length of AutoPollTargetData
					0x01,                   // Logical Number Tg = 1
					atqa_byte0, atqa_byte1, // SENS_RES (ATQA) on wire: 04 00 / 02 00 / 44 00 (UM0701-02 Page 150)
					sak,                    // SEL_RES (SAK)
					nfcid_len               // NFCID1 length (4 or 7 bytes)
				};
				resp.insert(resp.end(), uid_bytes.begin(), uid_bytes.end());
				build_response(resp);
			}
		}
		else if (card_tapped && polls_felica && card_t == card_type::felica)
		{
			if (card.size() < 8)
			{
				reset_target_state();
				build_response({res, 0x00});
				break;
			}

			reset_card_session();
			m_initiator_state = initiator_target_state::selected;

			const u8 felica_type = (*it_felica == 0x02 || *it_felica == 0x12) ? 0x12 : 0x11;
			std::array<u8, 8> idm{};
			std::array<u8, 8> pmm = default_felica_pmm;
			std::memcpy(idm.data(), card.data(), 8);
			if (card.size() >= 16)
			{
				std::memcpy(pmm.data(), card.data() + 8, 8);
			}

			// UM0701-02 §7.3.13 Page 145 & 149: AutoPollTargetData for FeliCa
			// [Tg(1B)] [POL_RES length(1B)] [0x01] [NFCID2t(8B)] [Pad(8B)] (19 bytes total)
			std::vector<u8> resp = {
				res,
				0x01,         // NbTg: 1 target found
				felica_type,  // Type 1: 0x11 (212 kbps) or 0x12 (424 kbps)
				0x13,         // Ln 1: 19 bytes of AutoPollTargetData
				0x01,         // Logical Number Tg = 1
				0x12,         // POL_RES length = 18 bytes
				0x01          // Response code
			};
			resp.insert(resp.end(), idm.begin(), idm.end());
			resp.insert(resp.end(), pmm.begin(), pmm.end());
			build_response(resp);
		}
		else if (card_tapped && polls_type_b && card_t == card_type::iso14443_3b)
		{
			const auto uid = get_card_uid(card);
			if (!uid)
			{
				reset_target_state();
				build_response({res, 0x00});
				break;
			}

			reset_card_session();
			m_initiator_state = initiator_target_state::selected;

			const u8 type_b_type = 0x23; // UM0701-02 §7.3.13 Page 144: Specific type for ISO/IEC 14443-4B
			const u8 ln = 15;            // Tg(1B) + ATQB(12B) + ATTRIB_RES len(1B) + ATTRIB_RES(1B)

			// UM0701-02 §7.3.13 Page 145: Non-DEP AutoPollTargetData is identical to TargetData in §7.3.5
			std::vector<u8> resp = {
				res,
				0x01,        // NbTg: 1 target found
				type_b_type, // Type 1: 0x23 (Passive 106 kbps ISO/IEC 14443-4B)
				ln,          // Ln 1: 15 bytes
				0x01,        // Logical Number Tg = 1
				0x50         // ATQB header
			};
			resp.insert(resp.end(), uid->begin(), uid->end()); // PUPI (4B)
			resp.insert(resp.end(), {0x00, 0x00, 0x00, 0x00}); // Application Data (4B)
			resp.insert(resp.end(), {0x00, 0x81, 0x71});       // Protocol Info (3B)
			resp.push_back(0x01);                              // ATTRIB_RES length
			resp.push_back(0x01);                              // ATTRIB_RES
			build_response(resp);
		}
		else if (card_tapped && polls_dep && card_t == card_type::nfcip_dep)
		{
			const auto uid = get_card_uid(card);
			if (!uid || card.size() < mifare_uid_size + 10)
			{
				reset_target_state();
				build_response({res, 0x00});
				break;
			}

			reset_card_session();
			m_initiator_state = initiator_target_state::selected;

			// UM0701-02 §7.3.13 Page 145: Generic types (0x00, 0x01, 0x02) map to specific DEP types (0x40, 0x41, 0x42)
			u8 dep_type = 0x40;
			if (it_dep != payload.end())
			{
				if (*it_dep == 0x01)
				{
					dep_type = 0x41;
				}
				else if (*it_dep == 0x02)
				{
					dep_type = 0x42;
				}
				else if (*it_dep != 0x00)
				{
					dep_type = *it_dep;
				}
			}

			// UM0701-02 §7.3.13 Page 145 & 150:
			// For DEP compliant passive targets, AutoPollTargetData is a concatenation of
			// InListPassiveTarget (§7.3.5) and InATR (§7.3.6):
			// Tg(1B) + SENS_RES(2B) + SEL_RES(1B) + NFCIDLength(1B) + NFCID1(4B) +
			// NFCID3t(10B) + DIDt(1B) + BSt(1B) + BRt(1B) + TO(1B) + PPt(1B)
			// Total length = 1 + 2 + 1 + 1 + 4 + 10 + 1 + 1 + 1 + 1 + 1 = 24 (0x18) bytes.
			const std::span<const u8> nfcid3t = card.subspan(mifare_uid_size, 10);
			const u8 ln = 0x18; // 24 bytes

			std::vector<u8> resp = {
				res,
				0x01,       // NbTg: 1 target found
				dep_type,   // Type 1: DEP type (e.g. 0x40)
				ln,         // Ln 1: 24 bytes
				0x01,       // Logical Number Tg = 1
				0x00, 0x08, // SENS_RES (NFCIP-1 DEP 106 kbps passive target)
				0x40,       // SEL_RES (NFC Forum / ISO 18092 DEP mode)
				0x04        // NFCID1 length: 4 bytes
			};
			resp.insert(resp.end(), uid->begin(), uid->end());       // NFCID1t (4B)
			resp.insert(resp.end(), nfcid3t.begin(), nfcid3t.end()); // NFCID3t (10B)
			resp.push_back(0x00);                                    // DIDt
			resp.push_back(0x00);                                    // BSt
			resp.push_back(0x00);                                    // BRt
			resp.push_back(0x09);                                    // TO
			resp.push_back(0x01);                                    // PPt
			build_response(resp);
		}
		else
		{
			build_response({res, 0x00}); // No targets found
		}
		break;
	}
	case command_code::tg_get_data:
	case command_code::tg_get_initiator_command:
	{
		// -------------------------------------------------------------------------
		// 7.3.16 TgGetData (0x86)
		// Target mode: retrieves data received from initiator (DEP or ISO14443-4 PICC).
		// Automatically handles supervisory frames (S(TO)REQ / S(WTX)) to prevent timeouts.
		//
		// Requires further hardware implementation (Target mode RF reception of DEP / ISO/IEC 14443-4 data)
		//
		// Expected Request Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x86 (Command Code)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 to 264 bytes
		//   Data:   [0] 0x87 (Response Code)
		//           [1] Status: Bit 7=NAD present, Bit 6=MI (chaining), Bits 0..5=Error code
		//           [2..n] DataIn: Received payload bytes (up to 262 bytes)
		// -------------------------------------------------------------------------
		// 7.3.19 TgGetInitiatorCommand (0x88)
		// Target mode: retrieves raw RF command packet directly from the initiator.
		// (Counterpart of InCommunicateThru on initiator side; no protocol handling).
		//
		// Requires further hardware implementation (Raw Target RF reception from external Initiator)
		//
		// Expected Request Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x88 (Command Code)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 to 264 bytes
		//   Data:   [0] 0x89 (Response Code)
		//           [1] Status: 0x00 = Success, Error code on failure
		//           [2..n] InCommand: Raw command bytes received from initiator (CRC stripped)
		// -------------------------------------------------------------------------
		if (payload.size() != 1)
		{
			build_response();
			break;
		}

		if (m_target_state != 0x01 && m_target_state != 0x81)
		{
			build_response({res, static_cast<u8>(status::invalid_state)});
			break;
		}

		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::tg_get_target_status:
	{
		// -------------------------------------------------------------------------
		// 7.3.21 TgGetTargetStatus (0x8A)
		// Queries the current target state (IDLE, ACTIVATED, DESELECTED, RELEASED) and baud rate.
		//
		// Expected Request Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x8A (Command Code)
		//
		// Expected Response Frame Parameters:
		//   Length: 3 bytes
		//   Data:   [0] 0x8B (Response Code)
		//           [1] State:
		//                 0x00: TG_IDLE / TG_RELEASED
		//                 0x01: TG_ACTIVATED (NFCIP-1 target activated)
		//                 0x02: TG_DESELECTED
		//                 0x80: PICC_RELEASED (ISO14443-4 PICC released)
		//                 0x81: PICC_ACTIVATED (ISO14443-4 PICC activated)
		//                 0x82: PICC_DESELECTED
		//           [2] BRit: Baud rate in TG_ACTIVATED state:
		//                 Bits 6..4: Initiator speed (000:106k, 001:212k, 010:424k)
		//                 Bits 2..0: Target speed (000:106k, 001:212k, 010:424k)
		// -------------------------------------------------------------------------
		if (payload.size() != 1)
		{
			build_response();
			break;
		}

		tg_get_target_status_response resp{};
		resp.state = m_target_state;
		resp.brit = 0x00;
		build_response(to_bytes(resp));
		break;
	}
	case command_code::tg_init_as_target:
	{
		// -------------------------------------------------------------------------
		// 7.3.14 TgInitAsTarget (0x8C)
		// Configures the PN532 as a target (DEP, ISO14443-4 PICC, or FeliCa target).
		// Activates hardware AutoColl to handle polling and anti-collision automatically
		// until the first valid command frame is received.
		//
		// Requires further hardware implementation (RF Target / PICC AutoColl emulation for ISO/IEC 18092 DEP, ISO/IEC 14443-4 PICC, FeliCa)
		//
		// Expected Request Frame Parameters:
		//   Length: 37 to 132 bytes
		//   Data:   [0] 0x8C (Command Code)
		//           [1] Mode: Bit 0=PassiveOnly, Bit 1=DEPOnly, Bit 2=PICCOnly
		//           [2..7] MifareParams (6B): SENS_RES (2B), NFCID1t (3B), SEL_RES (1B)
		//           [8..25] FeliCaParams (18B): NFCID2t (8B), PAD (8B), SystemCode (2B)
		//           [26..35] NFCID3t (10B): Target NFCID3 for ATR_RES
		//           [36] LEN_Gt, [37..] Gt: General bytes (max 47 bytes)
		//           [..] LEN_Tk, [..] Tk: Historical bytes for ATS (max 48 bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: Variable (at least 3 bytes)
		//   Data:   [0] 0x8D (Response Code)
		//           [1] Mode:
		//                 Bits 1..0: Framing (00:Mifare, 01:Active, 10:FeliCa)
		//                 Bit 2: DEP target activated
		//                 Bit 3: ISO14443-4 PICC activated
		//                 Bits 6..4: Baud rate (000:106k, 001:212k, 010:424k)
		//           [2..n] InitiatorCommand: First frame received from Initiator (e.g. ATR_REQ, RATS)
		// -------------------------------------------------------------------------
		// UM0701-02 §7.3.14: Validate minimum length and parameters
		// Minimum: 1B Cmd + 1B Mode + 6B MifareParams + 18B FeliCaParams + 10B NFCID3t + 1B LEN_Gt + 1B LEN_Tk = 38
		if (payload.size() < 38)
		{
			build_response();
			break;
		}

		// DEPOnly (bit 1) and PICCOnly (bit 2) cannot both be set
		// UM0701-02 §7.3.14: PICCOnly is set to 1 whereas fISO14443-4_PICC is set to 0 (SetParameters) -> Syntax Error
		if (((payload[1] & 0x06) == 0x06) || (((payload[1] & 0x04) != 0) && !(m_parameters_flags.load(std::memory_order_relaxed) & (1 << 5))))
		{
			build_response(); // Syntax error: invalid Mode combination or fISO14443-4_PICC disabled
			break;
		}

		{
			const u8 len_gt = payload[36];
			if (len_gt > 47 || payload.size() < static_cast<usz>(38 + len_gt))
			{
				build_response(); // Syntax error: LEN_Gt exceeds 47 bytes or data truncated
				break;
			}

			const u8 len_tk = payload[37 + len_gt];
			if (len_tk > 48 || payload.size() != static_cast<usz>(38 + len_gt + len_tk))
			{
				build_response(); // Syntax error: LEN_Tk exceeds 48 bytes or incorrect payload size
				break;
			}
		}

		// Per UM0701-02 §7.3.14, PN532 enters AutoColl listening and returns D5 8D only when an external Initiator activates it.
		// Without an external RF Initiator, remain unactivated and return no fabricated InitiatorCommand frame.
		m_target_state = (payload[1] & 0x04) ? 0x80 : 0x00;
		m_exec_response.clear();
		break;
	}
	case command_code::tg_set_data:
	case command_code::tg_response_to_initiator:
	{
		// -------------------------------------------------------------------------
		// 7.3.17 TgSetData (0x8E)
		// Target mode: transmits response data to initiator in DEP or ISO14443-4 PICC mode.
		// Transmits complete payload in a single frame or as final frame of chained transfer.
		//
		// Requires further hardware implementation (Target mode RF transmission of DEP / ISO/IEC 14443-4 response data)
		//
		// Expected Request Frame Parameters:
		//   Length: 1 to 263 bytes
		//   Data:   [0] 0x8E (Command Code)
		//           [1..n] DataOut: Response payload to transmit (max 262 bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x8F (Response Code)
		//           [1] Status: 0x00 = Success, Error code on failure
		// -------------------------------------------------------------------------
		// 7.3.20 TgResponseToInitiator (0x90)
		// Target mode: transmits raw RF response packet directly to the initiator.
		// (Counterpart of InCommunicateThru on target side; no protocol handling).
		//
		// Requires further hardware implementation (Raw Target RF transmission to external Initiator)
		//
		// Expected Request Frame Parameters:
		//   Length: 1 to 263 bytes
		//   Data:   [0] 0x90 (Command Code)
		//           [1..n] TgResponse: Raw response payload (max 262 bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x91 (Response Code)
		//           [1] Status: 0x00 = Success, Error code on failure
		// -------------------------------------------------------------------------
		if (payload.size() < 1 || payload.size() > 263)
		{
			build_response();
			break;
		}

		if (m_target_state != 0x01 && m_target_state != 0x81)
		{
			build_response({res, static_cast<u8>(status::invalid_state)});
			break;
		}

		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::tg_set_general_bytes:
	case command_code::tg_set_meta_data:
	{
		// -------------------------------------------------------------------------
		// 7.3.15 TgSetGeneralBytes (0x92)
		// Target mode: sets General Bytes (Gt) when fAutomaticATR_RES=0, then triggers ATR_RES.
		//
		// Expected Request Frame Parameters:
		//   Length: 1 to 48 bytes
		//   Data:   [0] 0x92 (Command Code)
		//           [1..n] Gt: General bytes for ATR_RES (max 47 bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x93 (Response Code)
		//           [1] Status: 0x00 = Success
		// -------------------------------------------------------------------------
		// 7.3.18 TgSetMetaData (0x94)
		// Target mode: transmits intermediate chained packet (>262B) with More Information (MI)=1.
		// Only supported in DEP mode.
		//
		// Requires further hardware implementation (Target RF DEP chained packet transmission with More Information bit)
		//
		// Expected Request Frame Parameters:
		//   Length: 1 to 263 bytes
		//   Data:   [0] 0x94 (Command Code)
		//           [1..n] DataOut: Chained packet payload (max 262 bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: 2 bytes
		//   Data:   [0] 0x95 (Response Code)
		//           [1] Status: 0x00 = Success
		// -------------------------------------------------------------------------
		const usz max_len = (command == command_code::tg_set_general_bytes) ? 48u : 263u;
		if (payload.size() > max_len || m_target_state == 0x81)
		{
			build_response();
			break;
		}

		if (m_target_state != 0x01)
		{
			build_response({res, static_cast<u8>(status::invalid_state)});
			break;
		}

		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::communicate_thru_ex:
	{
		// -------------------------------------------------------------------------
		// Sony RC-S620/S Section 8.2.11 CommunicateThruEX (0xA0)
		// Direct RF raw transceiver data exchange extension for FeliCa (212/424 kbps).
		//
		// Transmits DataOut directly as an RF command packet to the Target at 212/424 kbps,
		// and returns the received RF response packet as DataIn to the host controller.
		//
		// Expected Request Frame Parameters:
		//   Length: >= 3 bytes (up to 258 bytes in normal/extended frame)
		//   Data:   [0]    0xA0 (Command Code)
		//           [1..2] Timeout (2 bytes, little-endian, unit: 0.5 ms)
		//           [3..n] DataOut: Command data sent to Target (0 to 255 bytes)
		//
		// Expected Response Frame Parameters:
		//   Length: >= 2 bytes
		//   Data:   [0]    0xA1 (Response Code)
		//           [1]    Status: RF transaction status
		//                  0x00 = Success (正常終了)
		//                  0x01 = Timeout (RF 通信中にタイムアウトエラーが発生)
		//                  0x02 = CRC Error (RF 通信中にCRC エラーが発生)
		//                  0x0D = Temperature anomaly (温度異常が発生)
		//                  0x31 = Target mode RF error (Target モードでRF 通信中にRF エラー)
		//                  0x35 = Response length exceeded (受信RF レスポンスがデータ長より大きい)
		//           [2..m] DataIn: Response data received from Target (0 to 255 bytes)
		// -------------------------------------------------------------------------
		if (payload.size() < 3 || payload.size() > 258)
		{
			build_response();
			break;
		}

		const u16 timeout = read_from_ptr<le_t<u16>>(payload, 1);
		const std::span<const u8> data_out = payload.subspan(3);

		// RC-S620/S §8.2.11 Page 61-62: When DataOut is empty, RF command transmission is skipped
		// and RC-S620/S waits for an RF response from the Target.
		// Since passive FeliCa targets never transmit without first receiving an RF command,
		// reception inevitably times out (Status 0x01). No PC/SC call is needed.
		if (data_out.empty())
		{
			build_response({res, static_cast<u8>(status::timeout)});
			break;
		}

		// RC-S620/S §8.2.11 Page 62: When Timeout is 0000h, transmit DataOut without waiting for an RF reply
		// and return Status 0x01 (Timeout). PC/SC Escape 0xF3 uses reader-side FeliCa turnaround timing (~2.4 ms).
		if (is_pcsc)
		{
			const auto card_info = pcsc->get_card_info(false);
			if (!card_info.present || !card_info.is_felica)
			{
				build_response({res, static_cast<u8>(status::timeout)});
				break;
			}

			if (timeout == 0x0000)
			{
				std::vector<u8> discard;
				pcsc->felica_pass_thru(data_out, discard);
				build_response({res, static_cast<u8>(status::timeout)});
				break;
			}

			std::vector<u8> felica_resp;
			if (pcsc->felica_pass_thru(data_out, felica_resp))
			{
				std::vector<u8> resp = {res, static_cast<u8>(status::success)};
				resp.insert(resp.end(), felica_resp.begin(), felica_resp.end());
				build_response(resp);
			}
			else
			{
				build_response({res, static_cast<u8>(status::timeout)});
			}
			break;
		}

		// CommunicateThruEX is dedicated to 212/424 kbps FeliCa (ISO/IEC 18092 / JIS X 6319-4) communication.
		// If no card is tapped or the tapped card is not FeliCa (e.g. MIFARE Classic 1K/4K operates on
		// ISO/IEC 14443 Type A 106 kbps), the RF communication receives no response, resulting in a timeout.
		if (timeout == 0x0000 || !card_tapped || get_card_type(card) != card_type::felica)
		{
			build_response({res, static_cast<u8>(status::timeout)});
			break;
		}

		// Future implementation: process FeliCa command packet in data_out and return RF response in DataIn.
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	default:
	{
		nfc_log.error("NFC: Unhandled reader command: 0x%02X", static_cast<u8>(command));
		build_response(); // Respond with an error frame
		break;
	}
	}
}
