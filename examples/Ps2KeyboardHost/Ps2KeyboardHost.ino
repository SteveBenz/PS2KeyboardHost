#include "ps2_Keyboard.h"
#include "ps2_AnsiTranslator.h"
#include "Diagnostics.h"

static ps2::AnsiTranslator keyMapping;
static ps2::Keyboard<4,2,1,Diagnostics_> ps2Keyboard(Diagnostics);

// the setup function runs once when you press reset or power the board
void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	ps2Keyboard.begin();
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
		Serial.write(buf);
	}
}
