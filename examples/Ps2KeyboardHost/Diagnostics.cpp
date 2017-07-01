#include "Diagnostics.h"
#include "ps2_Keyboard.h"

static void sendUsb(byte code)
{
	// TODO!
}

static byte hexToHidCode[16] = { 0x27,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x04,0x05,0x06,0x07,0x08,0x09 };

static void sendHexDigit(byte b)
{
	sendUsb(hexToHidCode[b&0x0f]);
}

static void sendHex(byte b)
{
	sendHexDigit(b >> 4);
	sendHexDigit(b & 0xf);
}

void Diagnostics_::fail(FailureCode code) {
	failureCodes |= code;
	recordScanCode(FAIL_REPORT);
	recordScanCode(code >> 8);
	recordScanCode(code & 0xff);
}

void Diagnostics_::recordDataAvailable(byte numBytes, byte *bytes)
{
	recordScanCode(SEND_LED_SCAN_CODE);
	recordScanCode(numBytes);
	for (int i = 0; i < numBytes; ++i)
	{
		recordScanCode(bytes[i]);
	}
}


void Diagnostics_::sendReport()
{
	int failureCodesAtStart = failureCodes;
	sendUsb(0x0);
	sendUsb(0x2f);
	sendUsb(0x2f);

	sendHex(failureCodesAtStart >> 8);
	sendHex(failureCodesAtStart & 0xff);
	sendUsb(0x2d);
	for (int i = 0; i < NUMPS2SCANSRETAINED; ++i)
	{
		if (i > 0)
			sendUsb(0x2c); // space
		sendHex(ps2Scans[(lastPs2ScanIndex + 1 + i) % NUMPS2SCANSRETAINED]);
	}
	sendUsb(0x2d);
	//for (int i = 0; i < NUMUSBSCANSRETAINED; ++i)
	//{
	//	if (i > 0)
	//		sendUsb(0x2c); // space
	//	sendUsbReport(usbReports[(lastUsbScanIndex + 1 + i) % NUMUSBSCANSRETAINED]);
	//}

	sendUsb(0x30);
	sendUsb(0x30);

	failureCodes = 0;
}

void Diagnostics_::recordScanCode(byte scanCode)
{
	lastPs2ScanIndex = (1 + lastPs2ScanIndex) % NUMPS2SCANSRETAINED;
	ps2Scans[lastPs2ScanIndex] = scanCode;
}

void Diagnostics_::readInterruptWhileWriting() {
	fail(FailureCode::ReadInterruptWhileWriting);
}
void Diagnostics_::packetDidNotStartWithZero() {
	fail(FailureCode::Ps2PacketDidNotStartWithZero);
}
void Diagnostics_::parityError() {
	fail(FailureCode::Ps2PacketParityError);
}
void Diagnostics_::packetDidNotEndWithOne() {
	fail(FailureCode::Ps2PacketDidNotEndWithOne);
}
void Diagnostics_::packetIncomplete() {
	fail(FailureCode::Ps2PacketIncomplete);
}
void Diagnostics_::BufferOverflow() {
	fail(FailureCode::Ps2BufferOverflow);
}
void Diagnostics_::sendFrameError() {
	fail(FailureCode::Ps2SendPacketFrameError);
}
void Diagnostics_::nackReceived() {
	fail(FailureCode::Ps2KeyboardSentNack);
}
void Diagnostics_::noAckReceived() {
	fail(FailureCode::Ps2DidNotRespondWithAck);
}

//-------------------------------------------------------------------------------
void Diagnostics_::sentNack() {
	recordScanCode(SEND_NACK_SCAN_CODE);
}
void Diagnostics_::sentAck() {
	recordScanCode(ACK_SCAN_CODE);
}

void Diagnostics_::sentSetLed(ps2::KeyboardLeds ledStatus) {
	recordScanCode(SEND_LED_SCAN_CODE);
	recordScanCode((byte)ledStatus);
}
void Diagnostics_::returnedBadScanCode()
{
	recordScanCode((byte)ps2::KeyboardOutput::garbled);
}
void Diagnostics_::returnedScanCode(ps2::KeyboardOutput scanCode)
{
	recordScanCode((byte)scanCode);
}

Diagnostics_ Diagnostics;
