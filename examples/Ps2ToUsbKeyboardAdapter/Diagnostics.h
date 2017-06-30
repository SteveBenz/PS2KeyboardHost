#pragma once
#include <stdint.h>
#include "ps2_Keyboard.h"
#include "ps2_KeyboardOutput.h"

#define SEND_NACK_SCAN_CODE 0xfa
#define SEND_LED_SCAN_CODE 0xfb
#define FAIL_REPORT 0xfc
#define ACK_SCAN_CODE 0xfd

enum FailureCode
{
	KeyUpWithNoKeyDown = 0x01,
	KeyBufferOverflow = 0x02,
	KeyUnknownKeys = 0x04,
	Ps2BufferOverflow = 0x08,
	Ps2PacketIncomplete = 0x10,
	Ps2PacketDidNotStartWithZero = 0x20,
	Ps2PacketDidNotEndWithOne = 0x40,
	Ps2PacketParityError = 0x80,
	Ps2SendPacketFrameError = 0x100,
	Ps2SendBufferOverflow = 0x200,
	Ps2KeyboardSentNack = 0x400,
	Ps2DidNotRespondWithAck = 0x800,
	ReadInterruptWhileWriting = 0x1000,
	Ps2VoteNotUnanimous = 0x2000,
};

class Diagnostics_
{
#define NUMPS2SCANSRETAINED 40
#define NUMUSBSCANSRETAINED 16
	byte ps2Scans[NUMPS2SCANSRETAINED];
	byte lastPs2ScanIndex = 0;
	byte lastUsbScanIndex = 0;
	volatile uint16_t failureCodes = 0;
public:
	void fail(FailureCode code);
	void recordScanCode(byte ps2Scancode);
	//void recordUsbSend(const KeyReport &usbSend);
	void sendReport();
	void recordDataAvailable(byte numBytes, byte *bytes);

	void setLedIndicator() {
		digitalWrite(LED_BUILTIN, (millis() & (failureCodes != 0 ? 128 : 1024)) ? HIGH : LOW);
	}

	uint16_t fails() { return failureCodes; }

	void readInterruptWhileWriting();
	void packetDidNotStartWithZero();
	void parityError();
	void packetDidNotEndWithOne();
	void packetIncomplete();
	void BufferOverflow();
	void sendFrameError();
	void nackReceived();
	void noAckReceived();

	//-------------------------------------------------------------------------------
	void sentNack();
	void sentAck();
	void sentSetLed(ps2::KeyboardLeds ledStatus);
	void returnedBadScanCode();
	void returnedScanCode(ps2::KeyboardOutput scanCode);

	void incorrectResponse(ps2::KeyboardOutput scanCode, ps2::KeyboardOutput expectedScanCode) {
		Serial.print("incorrect response: ");
		Serial.println((byte)scanCode, HEX);
	}
	void noResponse(ps2::KeyboardOutput expectedScanCode) {
		Serial.println("no response");
	}
};
extern Diagnostics_ Diagnostics;
