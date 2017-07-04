This library is designed to allow you to hook one or more PS2-style keyboards to your Arduino.

It's not the only PS2 library out there.  There's this one, which is really basic:

https://playground.arduino.cc/Main/PS2Keyboard

This, which adds some capability to translate keys into a neutral form:

https://playground.arduino.cc/Main/PS2KeyboardExt

This library expanded on that theme:

https://github.com/techpaul/PS2KeyRaw

And finally this one added two-way communications:

https://github.com/techpaul/PS2KeyAdvanced


With the exception of the last one, they don't support two-way communication with the PS2 - which means
you can't do self-tests, you can't set the LED's, and you can't change the scan code set of the keyboard.

With PS2KeyAdvanced, you get that two-way communication, but it does a couple of things that render it
less than ideal for the two principal use-cases.  First, it does a translation from the raw PS2 language
into an invented language.  That translation costs machine cycles and memory and doesn't really help.

There are really two use-cases that you can get after with a PS2 keyboard attached to an Arduino:

1) You can use it to control a device without connecting it to a computer host.
2) You can use it to convert a PS2 keyboard into a USB keyboard that's used by something else (e.g. a Raspberry Pi).

For the first use-case, it's unlikely that the neutral form will directly work for what you're trying to do.
For the second use-case, you need to translate to the HID format, and the internal-only format that's provided
will just get in the way.

This library provides one class that interfaces with the PS2 keyboard, [[]]] and other sets of classes that
you can choose from to translate from the language of the PS2 either to ASCII, HID/USB, or the format invented
in PS2KeyAdvanced.

However, if you're using the keyboard as a way to control a device, a good option is to leverage the keyboard
controller itself.  If you have a remotely modern PS2-based keyboard, you can program it to provide you with
a very simple language that will allow you to directly translate from the stream of data coming from the keyboard
into behavior of your device.

If you take that approach, you'll find that this library will provide all the functionality you really need with
a bare minimum of RAM usage and code size.