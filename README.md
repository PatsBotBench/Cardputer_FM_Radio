This program allows the M5Cardputer to control a TEA5767 FM receiver over I2C, providing on-screen frequency display, signal strength, and mono/stereo indicators. The user can navigate through scanned stations, switch to saved stations, and fine-tune the frequency. Output of the TEA5767 radio module can be sent to a powered/amplified speaker or headphones. Saved stations are kept in a file on the SD card called RadioSta.TXT. The file is required for intended operations.

Essentials:
Modify your Radio preferences in the INO file and stations in the RadioSta.Txt file. Compile the INO file & load directly or to to a Bin & use Launcher on Cardputer. Copy RadioSta.txt to the root directory of the SD card.
TEA5767 Housing STL file can be found here: https://www.thingiverse.com/thing:2796647 
M5 Tape if you don't alreay have it!, can be found here: https://shop.m5stack.com/products/customized-m5stack-logo-adhesive-tape-buy-5-get-6?srsltid=AfmBOooPrlgnYh5shGglB8DBJKRUAlDXSjpoOiPFOAdObK3x-UXRVYGH

Pin Mapping:
I2C Configuration: The TEA5767 module is connected to the M5Cardputer via the Grove port.
Pin Assignments:
SDA: G2
SCL: G1

Key Functions:
m: Cycle between modes (Scanned Stations and Saved Stations).
/: Tune to the next saved or scanned station up the frequency band.
,: Tune to the previous saved or scanned station down the frequency band.
OK/Enter (LF key): Confirm selection of the new station after using tune keys
.: Fine-tune the frequency down by 0.1 MHz.
;: Fine-tune the frequency up by 0.1 MHz.
s: Rescan frequencies in Scanned Stations mode (ignored in Saved Stations mode).

Display Indicators:
Signal Strength: Displays the signal strength as a percentage, colored yellow for < 60% and green for ≥ 60%.
Stereo/Mono Indicator: Shows “Ster” in green for stereo signals and “Mono” in white for mono signals.
Frequency: Displays the current frequency in MHz.
Station Name: For saved stations, the station name displays at the bottom of the screen. In scanned mode, this area remains blank.

Futures:
1) Ability to save current scanned station
2) Use Wifi and also display time and/or Weather.


Recognizing life, the author will try to address major issues, but know the author is not committed to providing support, updates, or bug fixes for this software. Users assume all responsibility for any issues arising from using or modifying this software.
