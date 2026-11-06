#include "stdafx.h"
#include "nfc_reader.h"

LOG_CHANNEL(usio_log, "USIO");

nfc_reader::nfc_reader()
{
	init_registers();
}

namespace
{
	// Canonicalizes dual-mapped register addresses to their standard XRAM addresses.
	// In the PN532 architecture, the 14 CIU Page 0 registers are mapped to both:
	//   1. XRAM space (0x6331..0x633E, contiguous with base 0x6330 + idx)
	//   2. 80C51 SFR space (0xFFD1..0xFFD6, 0xFFDF, 0xFFE9..0xFFEF, partitioned to fit 80C51 SFR slots)
	// Reference: PN532/C1 Product Data Sheet Section 8.6.23 & Section 8.7 (Table 288 & Table 289)
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
}

void nfc_reader::init_registers()
{
	m_registers.clear();

	// -------------------------------------------------------------------------
	// 80C51 Special Function Registers (SFR: 0xFF80..0xFFFF)
	// Reference: PN532/C1 Product Data Sheet Section 8.7.2 (Table 289) & UM0701-02 Table 14
	// (Note: Dual-mapped CIU Page 0 SFRs 0xFFD1..0xFFEF are handled by canonicalize_register_address)
	// -------------------------------------------------------------------------
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
	m_registers[0xFFF4] = 0x07;       // P7CFGA: Port 7 Output Configuration A (Quasi-bidirectional mode reset default: 0x07 per UM0701-02 p.77 & Table 60)
	m_registers[0xFFF5] = 0x00;       // P7CFGB: Port 7 Output Configuration B (Quasi-bidirectional mode reset default: 0x00 per Table 62)
	m_registers[0xFFF7] = 0xF8 | m_gpio_p7;  // P7: Port 7 GPIO Data (Hardware POR: 0xFF, bits 7..3 reserved 1, bits 2..1 GPIO, bit 0 IRQ)
	m_registers[0xFFF8] = 0x00;       // IP1: Interrupt Priority 1
	m_registers[0xFFFC] = 0x3F;       // P3CFGA: Port 3 Output Configuration A (Quasi-bidirectional mode reset default: 0x3F per Table 66)
	m_registers[0xFFFD] = 0x00;       // P3CFGB: Port 3 Output Configuration B (Quasi-bidirectional mode reset default: 0x00 per Table 68)

	// -------------------------------------------------------------------------
	// System / PCR / Peripheral Registers in XRAM (0x6100..0x6205)
	// Reference: PN532/C1 Product Data Sheet Section 8.7.1 (Table 288)
	// -------------------------------------------------------------------------
	m_registers[0x6103] = ((m_gpio_i0i1 & 0x02) << 4) | ((m_gpio_i0i1 & 0x01) << 3); // Config I0_I1: Pad I0 and I1 input/output configuration and interface selection
	m_registers[0x6104] = 0x00; // Observe_testbus: Test bus observation control
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

	// -------------------------------------------------------------------------
	// Contactless Interface Unit (CIU) Registers in XRAM (0x6301..0x633E)
	// Reference: PN532/C1 Product Data Sheet Section 8.6.23 & Section 8.7.1 (Table 288)
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Firmware XRAM RF Configuration & Analog Calibration Tables (0xA0xx)
	// Stored in MCU internal XRAM by NXP ROM firmware to cache analog profile settings
	// (Corresponds to UM0701-02 Table 19 & Table 20 parameter profiles)
	// -------------------------------------------------------------------------
	// 106 kbps Type A profile (Table 19: 11 bytes, 0xA010..0xA01A)
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

	// -------------------------------------------------------------------------
	// Read-Only Register Protection
	// Discard write attempts to pure read-only hardware registers.
	// Reference: PN532/C1 Product Data Sheet Section 8.7 (Table 288 & 289)
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Write-Mask and Field Protection for Partially Writable Registers
	// -------------------------------------------------------------------------
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
	case 0x6202: // PCR ILR: bits 6, 4, 2..0 writable (0x57)
	{
		value = (m_registers[0x6202] & ~0x57) | (value & 0x57);
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
	usz size = 0;

	if (m_send_ack)
		size += ack_frame.size() + (m_remove_pre_and_postamble ? 0 : 2);

	if (m_send_response)
		size += m_response.size();

	return size;
}

void nfc_reader::write_buffer(std::vector<u8>& out)
{
	if (m_send_ack)
	{
		write_ack(out);
		m_send_ack = false;
	}

	if (m_send_response)
	{
		out.insert(out.end(), m_response.begin(), m_response.end());
		m_send_response = false;
	}
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

void nfc_reader::build_response(const std::vector<u8>& in)
{
	auto& out = m_response;

	if (!m_remove_pre_and_postamble)
		out.push_back(preamble);

	if (in.empty())
	{
		out.insert(out.end(), error_frame.begin(), error_frame.end());

		if (!m_remove_pre_and_postamble)
			out.push_back(postamble);

		return;
	}

	const auto append = [&out](const auto& value)
	{
		const auto bytes = to_bytes(value);
		out.insert(out.end(), bytes.begin(), bytes.end());
	};

	const usz total_len = in.size() + 1; // Including TFI

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

	for (const auto& data : in)
	{
		out.push_back(data);
		footer.dcs -= data;
	}

	append(footer);

	if (!m_remove_pre_and_postamble)
		out.push_back(postamble);
}

std::array<u8, mifare_uid_size> nfc_reader::get_card_uid(std::span<const u8> card) const
{
	std::array<u8, mifare_uid_size> uid{0x01, 0x02, 0x03, 0x04};
	if (card.size() >= mifare_uid_size)
	{
		std::memcpy(uid.data(), card.data(), mifare_uid_size);
	}
	return uid;
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

	// If access bits are completely uninitialized (00 00 00), treat as default factory transport configuration (FF 07 80)
	if (b6 == 0x00 && b7 == 0x00 && b8 == 0x00)
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
	const auto bits = get_access_bits(trailer, info.cond_index);
	if (!bits.valid)
	{
		return {.error = status::mifare_auth_error};
	}

	const usz offset = static_cast<usz>(block) * mifare_block_size;
	if (!allow_block_0 && (offset + mifare_block_size > card.size()))
	{
		return {.error = status::mifare_auth_error};
	}

	const u8 c = (bits.c1 << 2) | (bits.c2 << 1) | bits.c3;
	return {info, bits, c, offset, status::success};
}

void nfc_reader::process_request(const std::vector<u8>& in, std::span<u8> card, bool card_tapped)
{
	const std::span<const u8> frame = find_frame(in);

	if (!verify_request(frame))
		return;

	// In case NACK is received, the pending response should be retransmitted.
	// So only clear the pending response buffer when a new legitimate request or ACK is received.
	if (std::equal(nack_frame.begin(), nack_frame.end(), frame.begin()))
	{
		m_send_response = true; // Only retransmit the actual response; do not retransmit ACK
		return;
	}

	m_response.clear();
	m_send_ack = false;
	m_send_response = false;

	if (!card_tapped)
	{
		reset_target_state();
	}

	// Do not respond to ACK
	if (std::equal(ack_frame.begin(), ack_frame.end(), frame.begin()))
		return;

	const bool is_extended = is_extended_frame(frame);
	const usz data_offset = is_extended ? sizeof(extended_frame_header) : sizeof(frame_header);
	const usz data_len = (is_extended ? static_cast<usz>(read_from_ptr<extended_frame_header>(frame).len) : static_cast<usz>(read_from_ptr<frame_header>(frame).len)) - 1; // Excluding TFI
	const std::span<const u8> payload = frame.subspan(data_offset, data_len); // data_offset + data_len boundary check is done in verify_request()

	const auto command = static_cast<command_code>(::at32(payload, 0));
	const u8 res = response_code(command);

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
			build_response(); // Syntax error: missing NumTst
			break;
		}

		const u8 num_tst = payload[1];
		switch (num_tst)
		{
		case 0x00: // Communication Line Test: Echo back NumTst + InParam
		{
			std::vector<u8> resp = {res};
			resp.insert(resp.end(), payload.begin() + 1, payload.end());
			build_response(resp);
			break;
		}
		case 0x01: // ROM Test: 0x00 = OK
		case 0x02: // RAM Test: 0x00 = OK
		{
			build_response({res, 0x00});
			break;
		}
		case 0x04:
		{
			// Continuous RF polling test to target (212k or 424k)
			// UM0701-02 Section 7.2.1: Parameter Length: 1 (0x01 = 212 kbps, 0x02 = 424 kbps)
			if (payload.size() < 3)
			{
				build_response(); // Syntax error: missing baud rate parameter
				break;
			}

			// Returns number of transmission/reception failures (0x00 = 0 failures out of 128)
			build_response({res, 0x00});
			break;
		}
		case 0x05:
		{
			// Requires further hardware implementation (Continuous RF target echo-back test)
			// Runs infinitely until next host command; no response frame is sent
			m_send_response = false;
			break;
		}
		case 0x06:
		{
			// UM0701-02 Section 7.2.1: Tests if a previously activated target is still present.
			// For DEP targets: sends ATN (Attention Request) and expects the same response.
			// For ISO/IEC 14443-4 cards: sends R(NACK) and checks for a valid reply.
			// Returns 0x00 if the target is present; otherwise, returns an error code.
			build_response({res, static_cast<u8>(card_tapped ? status::success : status::timeout)});
			break;
		}
		case 0x07: // Antenna Test: 0x00 = Antenna OK
		{
			// UM0701-02 Section 7.2.1: Threshold used for antenna detection applied in Andet_Control (@610C)
			if (payload.size() < 3)
			{
				build_response(); // Syntax error: missing threshold parameter
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
		if (card_tapped)
		{
			// Target 1 active at 106kbps Type A, external RF field detected
			build_response({
				res,
				0x00, // Err: No error
				0x01, // Field: External RF field detected
				0x01, // NbTg: 1 target present
				0x01, // Tg 1: Target logical number 1
				0x00, // BrRx 1: 106 kbps
				0x00, // BrTx 1: 106 kbps
				0x00, // Type 1: Mifare / ISO14443-3A
				0x00  // SAM status
			});
		}
		else
		{
			// No targets present, no external RF field
			build_response({
				res,
				0x00, // Err: No error
				0x00, // Field: No external RF field
				0x00, // NbTg: 0 targets
				0x00  // SAM status
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
			build_response(); // Syntax error: invalid length or incomplete address pair
			break;
		}

		const usz num_addrs = (payload.size() - 1) / 2;
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
			build_response(); // Syntax error: invalid length or incomplete triplet
			break;
		}

		const usz num_triplets = (payload.size() - 1) / 3;
		for (usz i = 0; i < num_triplets; ++i)
		{
			const u16 addr = (static_cast<u16>(payload[1 + 3 * i]) << 8) | payload[2 + 3 * i];
			const u8 val = payload[3 + 3 * i];
			write_register_value(addr, val);
		}

		build_response({res, 0x00});
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
		if (payload.size() < 3)
		{
			build_response();
			break;
		}

		const u8 p3_in = payload[1];
		const u8 p7_in = payload[2];

		if (p3_in & 0x80) // Bit 7: Val validates P30..P35
		{
			write_register_value(0xFFB0, p3_in & 0x3F);
		}

		if (p7_in & 0x80) // Bit 7: Val validates P71, P72
		{
			write_register_value(0xFFF7, p7_in & 0x06);
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
		if (payload.size() < 2 || payload[1] > 0x08)
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
		//                 Bit 4: fAutomaticRATS (Auto-generate RATS in ISO14443-4 PCD mode)
		//                 Bit 5: fISO14443-4_PICC (Enable ISO14443-4 PICC emulation)
		//                 Bit 6: fRemovePrePostAmble (Suppress Preamble and Postamble)
		//
		// Expected Response Frame Parameters:
		//   Length: 1 byte
		//   Data:   [0] 0x13 (Response Code)
		// -------------------------------------------------------------------------
		if (payload.size() < 2)
		{
			build_response();
			break;
		}

		const u8 flags = payload[1];
		m_parameters_flags = flags;
		m_remove_pre_and_postamble = (flags & (1 << 6)) != 0;

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
		if (payload.size() < 2)
		{
			build_response();
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
		if (payload.size() < 2)
		{
			build_response();
			break;
		}

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
			if (payload.size() < 3)
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
			if (payload.size() < 5)
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
			if (payload.size() < 3)
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
			if (payload.size() < 5)
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
			if (payload.size() < 13)
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
				write_register_value(0xA010 + i, payload[2 + i]);
			}

			build_response({res});
			break;
		}
		case 0x0B: // Analog 212/424 kbps (8 bytes)
		{
			if (payload.size() < 10)
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
				write_register_value(0xA01B + i, payload[2 + i]);
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
			if (payload.size() < 5)
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
			if (payload.size() < 11)
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
			if (payload.size() < 3)
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
			if (payload.size() < 5)
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
		if (payload.size() < 2 || !card_tapped)
		{
			reset_target_state();
			build_response({res, static_cast<u8>(status::timeout)});
			break;
		}

		if (payload.size() >= sizeof(in_data_exchange_request))
		{
			const auto req = read_from_ptr<in_data_exchange_request>(payload);
			const usz total_blocks = is_4k_card(card) ? mifare_4k_blocks : mifare_1k_blocks;

			switch (static_cast<u8>(req.mifare))
			{
			case static_cast<u8>(mifare_cmd::auth_key_a):
			case static_cast<u8>(mifare_cmd::auth_key_b):
			{
				if (payload.size() < sizeof(mifare_auth_request))
				{
					reset_card_session();
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
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
				const auto trailer_bits = get_access_bits(trailer, 3); // Condition index 3 for Sector Trailer

				// Verify Access Bits integrity in Sector Trailer
				if (!trailer_bits.valid)
				{
					reset_card_session();
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
					break;
				}

				// Verify Card UID
				const auto card_uid = get_card_uid(card);
				if (auth_req.uid != card_uid)
				{
					reset_card_session();
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
					break;
				}

				bool auth_success = false;
				if (auth_req.mifare == mifare_cmd::auth_key_a)
				{
					// Key A is stored in bytes 0..5 of the sector trailer
					std::array<u8, 6> expected_key_a{};
					std::memcpy(expected_key_a.data(), trailer.data(), 6);

					// If trailer Key A is all zeros (e.g. uninitialized emulator card), allow factory default FF..FF as well
					const bool is_zero_key = std::all_of(expected_key_a.begin(), expected_key_a.end(), [](u8 b) { return b == 0x00; });
					constexpr std::array<u8, 6> default_transport_key{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

					if (auth_req.key == expected_key_a || (is_zero_key && auth_req.key == default_transport_key))
					{
						auth_success = true;
						m_authenticated_sector = info.sector;
						m_authenticated_key = auth_key::key_a;
					}
				}
				else // mifare_cmd::auth_key_b
				{
					// Table 6: In configurations 000, 010, 001, Key B is readable as data
					// and cannot be used for authentication.
					const u8 c_trailer = (trailer_bits.c1 << 2) | (trailer_bits.c2 << 1) | trailer_bits.c3;
					const bool key_b_can_auth = (c_trailer != 0 && c_trailer != 1 && c_trailer != 2);

					if (key_b_can_auth)
					{
						// Key B is stored in bytes 10..15 of the sector trailer
						std::array<u8, 6> expected_key_b{};
						std::memcpy(expected_key_b.data(), trailer.data() + 10, 6);

						const bool is_zero_key = std::all_of(expected_key_b.begin(), expected_key_b.end(), [](u8 b) { return b == 0x00; });
						constexpr std::array<u8, 6> default_transport_key{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

						if (auth_req.key == expected_key_b || (is_zero_key && auth_req.key == default_transport_key))
						{
							auth_success = true;
							m_authenticated_sector = info.sector;
							m_authenticated_key = auth_key::key_b;
						}
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
				if (payload.size() < 4 + mifare_block_size)
				{
					build_response({res, static_cast<u8>(status::mifare_auth_error)});
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
					// Access bits write: C in {0, 1, 3, 4} with Key B; C == 1 with Key A
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
						can_write_access = (check.c == 0 || check.c == 1 || check.c == 3 || check.c == 4);
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
			// -------------------------------------------------------------------------
			// Write 4 bytes (0xA2): Exclusive to MIFARE Ultralight (MF0ICU1).
			// MIFARE Classic (1K/4K) does not support command 0xA2.
			// Sending 0xA2 to a MIFARE Classic card returns an error (NACK / timeout).
			// -------------------------------------------------------------------------
			if (get_card_type(card) != card_type::mifare_ultralight)
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			if (payload.size() < 8)
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			const u8 page = req.block; // In Ultralight, req.block is the page number
			const usz page_offset = static_cast<usz>(page) * 4;

			// Pages 0 and 1 are read-only (serial number and internal)
			if (page < 2 || page_offset + 4 > card.size())
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			std::memcpy(card.data() + page_offset, payload.data() + 4, 4);
			build_response({res, static_cast<u8>(status::success)});
			break;
		}
		case static_cast<u8>(mifare_cmd::increment):
		case static_cast<u8>(mifare_cmd::decrement):
		{
			if (payload.size() < 8)
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

			const u8 cmd_byte = payload[2];
			bool can_do = false;
			if (cmd_byte == 0xC1)
			{
				if (m_authenticated_key == auth_key::key_a)
					can_do = (check.c == 0);
				else if (m_authenticated_key == auth_key::key_b)
					can_do = (check.c == 0 || check.c == 6);
			}
			else // 0xC0
			{
				can_do = (check.c == 0 || check.c == 1 || check.c == 6);
			}

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
			const s32 new_val = static_cast<s32>((cmd_byte == 0xC1)
				? (static_cast<u32>(current_val) + static_cast<u32>(operand))
				: (static_cast<u32>(current_val) - static_cast<u32>(operand)));

			m_transfer_buffer = format_value_block(new_val, current_addr);
			m_transfer_buffer_valid = true;

			build_response({res, static_cast<u8>(status::success)});
			break;
		}
		case static_cast<u8>(mifare_cmd::restore):
		{
			const auto check = check_mifare_block_access(card, req.block, false, false);
			if (!check.ok())
			{
				build_response({res, static_cast<u8>(check.error)});
				break;
			}

			// Table 7: Restore allowed if C in {0, 1, 6}
			const bool can_restore = (check.c == 0 || check.c == 1 || check.c == 6);
			if (!can_restore)
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
			if (payload.size() < 4)
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
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

			// Access rules (Table 7): Transfer allowed if C in {0, 1, 6}
			const bool can_transfer = (check.c == 0 || check.c == 1 || check.c == 6);
			if (!can_transfer)
			{
				build_response({res, static_cast<u8>(status::mifare_auth_error)});
				break;
			}

			std::memcpy(card.data() + check.offset, m_transfer_buffer.data(), mifare_block_size);

			m_transfer_buffer_valid = false;
			build_response({res, static_cast<u8>(status::success)});
			break;
		}
	default:
			{
				// Requires further hardware implementation (NFC-DEP, FeliCa, ISO/IEC 14443-4 APDU, Jewel)
				build_response({res, static_cast<u8>(status::success)});
				break;
			}
			}
		}
		else
		{
			build_response({res, static_cast<u8>(status::success)});
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
		reset_target_state();
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::in_jump_for_psl:
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
		if (card_tapped)
		{
			build_response({
				res,
				static_cast<u8>(status::success),
				0x01,                                                       // Tg
				0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // NFCID3t (10B)
				0x00,                                                       // DIDt
				0x00,                                                       // BSt
				0x00,                                                       // BRt
				0x0E,                                                       // TO
				0x00                                                        // PPt
			});
		}
		else
		{
			build_response({res, static_cast<u8>(status::timeout)});
		}
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
		if (card_tapped)
		{
			in_list_passive_target_response resp{};
			const auto uid = get_card_uid(card);
			std::memcpy(resp.nfcid.data(), uid.data(), mifare_uid_size);

			if (is_4k_card(card))
			{
				resp.sens_res = 0x0002; // SENS_RES (ATQA) for Mifare Classic 4K
				resp.sel_res  = 0x18;   // SEL_RES (SAK) for Mifare Classic 4K
			}
			else
			{
				resp.sens_res = 0x0004; // SENS_RES (ATQA) for Mifare Classic 1K
				resp.sel_res  = 0x08;   // SEL_RES (SAK) for Mifare Classic 1K
			}

			build_response(to_bytes(resp));
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
		if (card_tapped)
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
		if (card_tapped)
		{
			build_response({
				res,
				static_cast<u8>(status::success),
				0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // NFCID3t (10B)
				0x00,                                                       // DIDt
				0x00,                                                       // BSt
				0x00,                                                       // BRt
				0x0E,                                                       // TO
				0x00                                                        // PPt
			});
		}
		else
		{
			build_response({res, static_cast<u8>(status::timeout)});
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
		if (card_tapped)
		{
			build_response({res, static_cast<u8>(status::success)});
		}
		else
		{
			build_response({res, static_cast<u8>(status::context_error)});
		}
		break;
	}
	case command_code::in_jump_for_dep:
	{
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
		if (card_tapped)
		{
			build_response({
				res,
				static_cast<u8>(status::success),
				0x01,                                                       // Tg
				0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // NFCID3t (10B)
				0x00,                                                       // DIDt
				0x00,                                                       // BSt
				0x00,                                                       // BRt
				0x0E,                                                       // TO
				0x00                                                        // PPt
			});
		}
		else
		{
			build_response({res, static_cast<u8>(status::timeout)});
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
		m_send_response = false;
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
		if (card_tapped)
		{
			const auto uid = get_card_uid(card);
			const bool is_4k = is_4k_card(card);
			const u8 sak = is_4k ? 0x18 : 0x08;
			const u8 atqa_hi = 0x00;
			const u8 atqa_lo = is_4k ? 0x02 : 0x04;

			build_response({
				res,
				0x01,                                           // NbTg: 1 target found
				0x10,                                           // Type 1: Mifare
				0x09,                                           // Ln 1: 9 bytes of TargetData
				0x01,                                           // Logical Number Tg = 1
				atqa_hi, atqa_lo,                               // SENS_RES (ATQA) in big-endian MSB first (Footnote 11)
				sak,                                            // SEL_RES (SAK)
				mifare_uid_size,                                // NFCID1 length (4 bytes)
				uid[0], uid[1], uid[2], uid[3]                  // NFCID1 (UID) from card data
			});
		}
		else
		{
			build_response({res, 0x00}); // No targets found
		}
		break;
	}
	case command_code::tg_get_data:
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
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::tg_get_initiator_command:
	{
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
		m_target_state = 0x01; // TG_ACTIVATED
		// Emulate activation as DEP passive 106kbps target receiving mock ATR_REQ:
		build_response({
			res,
			0x04,                                                                   // Mode: 106kbps Mifare framing, DEP target activated
			0xD4, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A // Mock Initiator command (ATR_REQ)
		});
		break;
	}
	case command_code::tg_set_data:
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
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::tg_response_to_initiator:
	{
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
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::tg_set_general_bytes:
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
		build_response({res, static_cast<u8>(status::success)});
		break;
	}
	case command_code::tg_set_meta_data:
	{
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
			build_response(); // Syntax error: missing Timeout parameter or DataOut exceeds 255 bytes
			break;
		}

		[[maybe_unused]] const u16 timeout = read_from_ptr<le_t<u16>>(payload, 1);
		[[maybe_unused]] const std::span<const u8> data_out = payload.subspan(3);

		// CommunicateThruEX is dedicated to 212/424 kbps FeliCa (ISO/IEC 18092 / JIS X 6319-4) communication.
		// If no card is tapped or the tapped card is not FeliCa (e.g. MIFARE Classic 1K/4K operates on
		// ISO/IEC 14443 Type A 106 kbps), the RF communication receives no response, resulting in a timeout.
		if (!card_tapped || get_card_type(card) != card_type::felica)
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
		usio_log.warning("Unhandled NFC reader command: 0x%02X", static_cast<u8>(command));
		build_response(); // Respond with an error frame
		break;
	}
	}

	m_send_ack = true;
	m_send_response = !m_response.empty();
}
