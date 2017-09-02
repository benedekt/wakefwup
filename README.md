# wakefwup
A small tool to "wake up" BeBoB based FireWire M-Audio soundcards on Mac. 
It makes use the research of Takashi Sakamoto of the FFADO project.

As Takashi found out, the official driver sends some magic bytes to the bootloader
of the card, and that activates the main firmware, and enables the audio functionality.

But, if the card was not plugged in at boot, the official driver wakes up the device,
and then crashes the kernel. On the other hand, if it was present at boot, it just wakes it up,
then starts to use it.

I think, this is a protection feature, planted in the "wake up" function in the driver:
It tries to wake up the device, and then checks if it was present at boot, if it was, we are good.
It keeps running with the audio initiating functions. 
(This paragraph is all speculation, I didn't have the time to disassemble the module --> on the TODO list.)

But, if we wake up the device manually, it wont crash the system: we can hot-plug our card! :)

Do this at your on risk!, because M-Audio says that hot-plugging is dangerous.

Yes, it is. It WAS in 2003-2004 with the Oxford controllers, and the 400 type, 6-pin cables.
I. 	The intel based macs, especially with the FW 800 don't use that problematic IC. We are safe.
II.	Connection angle. There is no such problem on type 800 cables, again, but if you use a separate
	power cord, and a 4 pin cable, you are safe too from your own stupidity if you plug your cable in
	with an extreme angle.
