This library is designed to allow you to hook one or more PS2-style keyboards to your Arduino.
This is not the only PS2 library out there.  There's this one, which is really basic:

https://playground.arduino.cc/Main/PS2Keyboard

And this one, which adds some capability to translate keys into a neutral form:

https://playground.arduino.cc/Main/PS2KeyboardExt

Then this library expanded on that theme:

https://github.com/techpaul/PS2KeyRaw

And finally this one added two-way communications:

https://github.com/techpaul/PS2KeyAdvanced

With the exception of the last one, they don't support two-way communication with the PS2 - which means
you can't do self-tests, you can't set the LED's, and you can't change the scan code set of the keyboard.

With PS2KeyAdvanced, you get that two-way communication, but it does a couple of things that render it
less than ideal for the two principal use-cases.  First, it does a translation from the raw PS2 language
into an invented language.  That translation costs machine cycles and memory and doesn't really help.

There are two practical use-cases that you can get after with a PS2 keyboard attached to an Arduino:

1) You can use it to control a device without connecting it to a computer host.
2) You can use it to convert a PS2 keyboard into a USB keyboard that's used by something else (e.g. a Raspberry Pi).

For the first use-case, it's unlikely that the neutral form will directly work for what you're trying to do.
For the second use-case, you need to translate to the HID format, and the internal-only format that's provided
will just get in the way.

This library provides one class that interfaces with the PS2 keyboard, [ps2::Keyboard](https://stevebenz.github.io/firsttry/classps2_1_1_keyboard.html),
and other classes that you can choose from to translate from the language of the PS2 either to ASCII, with
[ps2::AnsiTranslator](https://stevebenz.github.io/firsttry/classps2_1_1_ansi_translator.html),
or HID/USB with [ps2::UsbTranslator](https://stevebenz.github.io/firsttry/classps2_1_1_usb_translator.html).

If you're using the keyboard as a way to control a device, a good option is to leverage the keyboard
controller itself.  If you have a remotely modern PS2-based keyboard, you can program it to provide you with
a very simple language that will allow you to directly translate from the stream of data coming from the keyboard
into behavior of your device.

If you take that approach, you'll find that this library will provide all the functionality you really need with
a bare minimum of RAM usage and code size.

There are three examples provided:

[Ps2ToUsbKeyboardAdapter](https://github.com/SteveBenz/firsttry/tree/master/examples/Ps2ToUsbKeyboardAdapter) - actually
a fully-functional program for converting PS2 keyboards to USB while allowing you to re-map keys along the way.

[Ps2KeyboardHost](https://github.com/SteveBenz/firsttry/tree/master/examples/Ps2KeyboardHost) - reads the PS2 keyboard,
converts the codes to Ascii and prints them on the Serial device.

[SelfTest](https://github.com/SteveBenz/firsttry/tree/master/examples/SelfTest) - is a test application that excercises
most of the functionality of the PS2.  If you intend to create an application based on the PS2 scancode set, this
is a great way to experiment with the settings until you find something that will work for your application.


