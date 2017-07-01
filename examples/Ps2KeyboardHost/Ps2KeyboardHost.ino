#include "ps2_Keyboard.h"
#include "ps2_AnsiTranslator.h"
#include "Diagnostics.h"

static ps2::AnsiTranslator keyMapping;
static ps2::Keyboard<4,2,1,Diagnostics_> ps2Keyboard(Diagnostics);
static ps2::KeyboardLeds lastLedSent;

// the setup function runs once when you press reset or power the board
void setup() {
	pinMode(LED_BUILTIN, OUTPUT);

	ps2Keyboard.begin();
	ps2Keyboard.reset();

	// Start out with NumLock on
	keyMapping.setNumLock(true);
	lastLedSent = ps2::KeyboardLeds::Ps2LedNumLock;
	ps2Keyboard.sendLedStatus(ps2::KeyboardLeds::Ps2LedNumLock);
}

void loop() {
	ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
	if (scanCode == ps2::KeyboardOutput::garbled) {
		keyMapping.reset();
	}
	else if (scanCode != ps2::KeyboardOutput::none)
	{
		char buf[2];
		buf[1] = '\0';
		buf[0] = keyMapping.translatePs2Keycode(scanCode);
		if (buf[0] == '\r') {
			Serial.println();
		}
		else if (buf[0] >= ' ') { // Stuff < ' ' are control-characters; this example isn't clever enough to do anything with them.
			Serial.write(buf);
		}

		ps2::KeyboardLeds newLeds =
			  (keyMapping.getCapsLock() ? ps2::KeyboardLeds::Ps2LedCapsLock : ps2::KeyboardLeds::Ps2LedNone)
			| (keyMapping.getNumLock() ? ps2::KeyboardLeds::Ps2LedNumLock : ps2::KeyboardLeds::Ps2LedNone);
		if (newLeds != lastLedSent) {
			ps2Keyboard.sendLedStatus(newLeds);
			lastLedSent = newLeds;
		}
	}
}
