# CEC-Tiny
Support for the HDMI CEC protocol with minimal hardware, running on an ATTINY25 processor.

I've got myself into HDMI-CEC attempting to control the power state (on/off) of a vintage TV based on the state of a HDMI signal source. I'm using a [HDMI-to-RF modulator box](https://www.amazon.com/HDMI-Converter-Modulator-Old-Transmitter/dp/B07W58PNPP/) which does a good job converting a HDMI signal into a coaxsial atenna cable which can be fed into any old TV set. The HDMI source is a Chromecaset with Google TV - it has a remote controller, so the goal has been to intercept the Google remote on/off events to drive a relay which turns the tv on and off. As I did not want to run any extra cable, the relay control voltage would be passed through the coaxial RF signal cable.

I went through a dozen or so HDMI-CEC projects and all of them were quite complex (e.g., requiring a separation of CEC-IN and CEC-OUT as separate signals) and despite several efforts I could not get any of them up an running. Then I came across the [CEC Volume Control project by Thomas Sowell](https://blog.ldtlb.com/2020/10/14/pioneer-sx950-hdmi-cec-volume.html) from which I derived the idea for my implemetation.

The code is a farily straightforward fork of [Thomas'es code](https://github.com/tsowell/avr-hdmi-cec-volume), with minor modifications:
* Changed to make it compatible with the Arduino IDE (this brings much greater flexibility in terms of hardware)
* Changed timer "ticks" to the system "micros()" function to make the code independent (to some extent) from the exact oscillator frequency. This change also makes it it easier to port to the low-end ATTINY processors, which do not have a 16-bit timer).
* Removed all serial console debugging code.
* Removed the timer interrupt handler for simplicity.
* Added a hack to force the Chromecast to send power and volume commands on the CEC bus (to do that you need to pretend you are a TV - in CEC terms). This also works with Apple TV.

The INO file does not have any dependencies nor requires any libraries. This has been tested with both external, crystal-based clock as well as with an internal oscillator. Remember to select a proper option in the IDE and "Tools -> Burn Bootloader", which will set the AVR fuses accordingly.

Thomas argues for using a crystal oscillator, as the CEC protocol is very time-sensitive. This is true, but I found it working quite well on an AVR (ATTINY) processor with an internal oscillator, after tweaking the OSCCAL variable (you need to do this for every unit). This also means that for the first device you make the crystal is really needed, to save you from debugging frustration. Please note the crystal must match the Arduino IDE settings for your board / processor. 16MHz is recommended. Once you have one board working, you may consider iterating by removing the crystal, switching to the internal oscillator and tweaking the OSCCAL variable.

The highlight of this CEC implementation is the CEC line (pin 13 on a HDMI port) is fed directly to a GPIO pin on an AVR processor. The pin is configured as a High-Z (no pull-up) input when receiving and is toggled between low output (active) and High-Z input (inactive) when transmitting. This approach reduces the necessary hardware to minimum. In fact in the most simplified version the only part needed is the ATTINY25 processor (and an LED serving as an output). I thik this can be claimed the tiniest HDMI-CEC implementation possible. The code uses ~1600 bytes of program memory and 9 bytes of variables.

To help with debugging and tweaking the ATTINY25 code, I have also preapred a separate project: the [CEC-Tiny-Pro](https://github.com/SzymonSlupik/CEC-Tiny-Pro). It is targeting a "Pro" (ATMEGA328) arduino board and provides all bells and whistles of CEC, incluiding full serial monitoring / debugging. It can be used in a fully passive mode (just listening to the CEC line, not sending anything), so that you can daisy-chain the ATTINY25 implementation with the ATMEGA implementation and use the ATMEGA as a passive HDMI-CEC scanner to monitor the behavior of the ATTINY25 (which does not have serial output).

Other than the ATTINY25 processor, there is a power status LED with a resistor. It red-glows the box gently to help troubleshooting the state / connectivity. The DC-to-RF coupling uses small inductors and 100nF capacitors to block the RF path from interference. It assumes the RF path has serial insulating capacitors already installed. Finally I opted for a serial Constant Current Regulator (CCR) - the NSI45015WT1G. It limits the current (in case of shorting) to 15mA. This can be replaced by a ~200R resistor.

Below are photos of the complete setup. The HDMI-to-RF box has a USB port, which can be used to provide power to the Chromecast, reducing the number of power bricks and cables. You may also notice I like using the SMD variants of the ATTINY and that is because I can use a simple clip, combimned with a [Tiny AVR Programmer](https://www.sparkfun.com/products/11801) to program the chip in-circuit from the Arduino IDE.

![alt text](https://github.com/SzymonSlupik/CEC-Tiny/blob/main/HDMI-to-RF%20with%20Chromecast.JPG?raw=true "HDMI-to-RF with Chromecast")
![alt text](https://github.com/SzymonSlupik/CEC-Tiny/blob/main/CEC%20Power%20-%20Idea.jpg?raw=true "CEC-based Power Control over coaxial RF cable")
![alt text](https://github.com/SzymonSlupik/CEC-Tiny/blob/main/CEC-Tiny%20inside.JPG?raw=true "CEC-Tiny inside")
![alt text](https://github.com/SzymonSlupik/CEC-Tiny/blob/main/CEC-Tiny%20in-circuit%20programming.JPG?raw=true "In-circuit programming using a clip")
![alt text](https://github.com/SzymonSlupik/CEC-Tiny/blob/main/Brionvega%20Algol.jpg?raw=true "Algol Brionvega controlled by Chromecast over HDMI-CEC")

Another proof-of-concept variation of this project is a simple CEC demonstrator. It uses only the ATTINY25 processor and 3 LEDs with no other external components. Even the LED resistors are not necessary, as the internal pull-ups are used instead. This demonstrates yet another interesting technique possible - declare a pin as input and switch it between high-Z and pull-up mode, it is sufficient to energize an LED. This demostrator uses the CEC_Tiny_Volume.INO which has added code for handling volume up/down messages.

![alt text](https://github.com/SzymonSlupik/CEC-Tiny/blob/main/CEC-Tiny.JPG?raw=true "CEC-Tiny Demonstrator")

