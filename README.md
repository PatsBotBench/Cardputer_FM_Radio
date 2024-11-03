M5 Cardputer FM Radio
This program allows the M5Cardputer to control a TEA5767 FM receiver over I2C, providing on-screen frequency display, signal strength, and mono/stereo indicators. The user can navigate through scanned stations, switch to saved stations, and fine-tune the frequency. Output of the TEA5767 radio module can be sent to a powered/amplified speaker or headphones.

Essentials:
Modify your Radio preferences in the INO file and stations in the RadioSta.Txt file. Compile the INO file & load directly or to to a Bin & use Launcher.
TEA5767 Housing STL file can be found here: https://www.thingiverse.com/thing:2796647

The user can navigate through scanned stations, switch to saved stations, and fine-tune the frequency. Saved stations are kept in a file on the TF card called RadioSta.TXT. The file is required for intended operations.

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

Configuration Variables

startupMode (RadioMode):
Type: RadioMode enum (SCANNED or SAVED)
Purpose: Sets the initial mode of the radio at startup.
Default Value: SAVED

maxStationNameLength:
Type: int
Purpose: Limits the number of characters displayed for a station name. Helps maintain a consistent display.
Default Value: 20

frequency:
Type: double
Purpose: Represents the starting frequency or the current frequency being tuned.
Default Value: 87.50 (MHz, typically the lower bound of the FM band)

topFrequency:
Type: double
Purpose: Sets the upper frequency limit for FM scanning.
Default Value: 108.00 (MHz, typically the upper bound of the FM band)

intervalFreq:
Type: double
Purpose: Frequency step for tuning and scanning.
Default Value: 0.1 (MHz, common FM frequency increment)

minSignalLevel:
Type: short
Purpose: Sets the minimum signal strength required to consider a station valid during scanning.
Default Value: 9 (on a scale where higher values represent stronger signals)

scanDelay:
Type: unsigned long
Purpose: Determines the delay between frequency steps during the scanning process.
Default Value: 100 (milliseconds)

debounceDelay:
Type: const unsigned long
Purpose: Debounce delay to prevent multiple key presses being registered.
Default Value: 200 (milliseconds)

statusUpdateInterval:
Type: unsigned long
Purpose: Sets the interval for updating the signal strength and stereo status display.
Default Value: 120000 (milliseconds or 120 seconds)

stationsFile:
Type: const char*
Purpose: File path on the SD card for loading/saving station lists.
Default Value: "/RadioSta.txt"

displaySignalStrength:
Type: bool
Purpose: Toggles display of signal strength percentage on the screen.
Default Value: true

displayStereoStatus:
Type: bool
Purpose: Toggles display of stereo/mono indicator on the screen.
Default Value: true

Futures
1) Ability to save current scanned station
2) Use Wifi and also display time and/or Weather.
