This program allows the M5Cardputer to control a TEA5767 FM receiver over I2C, providing on-screen frequency display, signal strength, and mono/stereo indicators. The user can navigate through scanned stations, switch to saved stations, and directly enter or fine-tune the frequency. Output of the TEA5767 radio module can be sent to a powered/amplified speaker or headphones. Saved stations are kept in a file on the SD card called RadioSta.TXT. The file is required for intended operations.

Essentials:
Modify your Radio preferences in the INO file and stations in the RadioSta.Txt file. Compile the INO file & load directly or to a Bin & use Launcher on Cardputer. Copy RadioSta.txt to the root directory of the SD card.  
TEA5767 Housing STL file can be found here: https://www.thingiverse.com/thing:2796647 

M5 Tape if you don't already have it!, can be found here: https://shop.m5stack.com/products/customized-m5stack-logo-adhesive-tape-buy-5-get-6?srsltid=AfmBOooPrlgnYh5shGglB8DBJKRUAlDXSjpoOiPFOAdObK3x-UXRVYGH

## Versions

This repository contains an updated sketch & Binary for the **M5Cardputer Advanced** & - **M5Cardputer** as well as the original sketch built for only the **M5Cardputer**.


## Main Features

- Control a TEA5767 FM receiver over I2C.
- Scan the FM band for stations using signal strength.
- Navigate scanned stations or saved stations.
- Save the current frequency directly to the SD card. (New)
- Delete the current saved station from the SD card. (New)
- Prevent duplicate saved-station entries. (New)
- Manually enter a frequency using the keyboard. (New)
- Fine tune up/down by 0.1 MHz with band-edge wrapping. (New)
- Toggle mute. (New)
- Toggle forced mono for weak/noisy stereo stations. (New)
- Display saved station names when the current frequency matches a saved entry. (New)
- Show temporary interaction messages such as `Saved`, `Deleted`, `Already saved`, and `Out of band`. (New)
- Store saved stations on the SD card in `RadioSta.txt`.

## SD Card Station File

Saved stations are stored in the root directory of the SD card:

```text
/RadioSta.txt
Station file format:

frequency,station name
Example:

97.50,WQOK: Hip-Hop
96.10,WBBB: Adult
91.50,WUNC: Public
101.50,WRAL: Adult
When a station is saved from the Cardputer, it is written as:

101.50,101.50
You can later edit the station name using another application.

Controls
c      Switch between scanned stations and saved stations
/      Next station
,      Previous station
Enter  Confirm displayed or manually entered frequency

.      Fine tune down by 0.1 MHz
;      Fine tune up by 0.1 MHz

0-9    Start or continue manual frequency entry
-      Decimal point during manual frequency entry

s      Rescan frequencies in scanned-stations mode
d      Save current frequency to RadioSta.txt
x      Delete current frequency from RadioSta.txt

m      Mute/unmute audio
f      Force mono / allow stereo
Display Indicators
Signal Strength: Displayed as a percentage.

Yellow below 60%
Green at 60% or higher
Stereo/Mono Status:

Ster in green for stereo
Mono in white for mono
FMon in yellow when forced mono is enabled
Mute Status:

MUTE appears in red beside Freq:
Interaction Line:

Shows saved station name when the current frequency matches a saved station.
Shows manual frequency entry such as Tune: 101.5.
Shows temporary messages such as Saved, Deleted, Already saved, and Out of band.
Hardware Connection
The TEA5767 module is connected through the Grove port.

SDA: G2
SCL: G1
VCC: Grove V
GND: Grove G
Notes
The sketch includes a local TEA5767 driver, so a separate TEA5767 Arduino library is not required. The Cardputer display, keyboard, and board support use the M5Stack Cardputer libraries.

The TEA5767’s built-in hardware seek was not used because manual stepping and signal-threshold scanning proved more predictable, especially when antenna quality varies.


Recognizing life, the author will try to address major issues, but know the author is not committed to providing support, updates, or bug fixes for this software. Users assume all responsibility for any issues arising from using or modifying this software.
