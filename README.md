#Pebble II SDR (Mac) 
July 1, 2016  
Copyright 2010 ... 2016  
Richard Landsman N1DDY <PebbleSDR@gmail.com>  
Licensed under GPL, see gpl.h for details, attribution, and references  
<https://github.com/oldscienceguy/PebbleSDR>  
Dedicated to SuperRatt, my first program (c 1980) for the Apple II with support for RTTY, CW, and special modes.  

---
AFEDRI showing full 2mHz bandwidth from 150kHz to 2150kHz and HiRes zoom at 850 kHz  
<img src=https://raw.github.com/oldscienceguy/PebbleSDR/master/screen_shots/Pebble%207-9-16.png width=500> 
---
##Introduction

I 'discovered' Software Defined Radio at the beginning of 2010 when I found myself between jobs and some extra time.  The experience of building my first SoftRock V9 and actually hearing something was like the first crackle from a crystal radio.  I wasn't quite sure everything was working, but it made noise!  I was hooked and soon wanted to learn about the magical DSP that made everything work.  Rocky was the first SDR program I used, and I thought Pebble - a small rock, was an appropriate name for my new program.

Pebble is now a complete SDR program, written in QT/C++.  The original version was Windows only and had only basic functionality.  This second version is substantially improved and mostly re-written.  Development for V 2 was done on a Mac and so Pebble II is Mac only for the time being.  Once stable, it will be back-ported to Windows.

There are a few design principles I tried to follow throughtout the evolution of Pebble
* ####Compact  
There are plenty of full featured SDR applications these days, but many of them take up the entire screen and present an overwhelming number of options to the average user.  I wanted something that had all the key features in a compact UI, leaving room for other programs to be visible on an average screen.
* ####Incremental 
In order to keep the UI compact, more advanced and optional functionality had to be exposed incrementally.  There when you need it, not there when you don't.  This led to features like being able to collapse the spectrum and data windows.

* ####Extensible
In addition to being extensible by virtue of being open source, I wanted to be able to keep the core functionality intact, while exploring new data modes and options.  This led to the modem plug-in architecture and eventually will include cross platform extio-like device support.

* ####Disclaimer
The source code for this project represents incremental, "drive-by" coding, with numerous experiments, re-writes, re-factoring, re-everything.  Little attempt has been made to pretty up the code and clean up comments.  In fact, in many places I have deliberately left alternative implementations and detailed comments in the code for future reference.  DSP comments in the source code should be taken with a grain of salt, especially older comments.  This has been a learning experience for me and many times I thought I understood something, only to come back later and realize I didn't have a clue!  Finally, if you have questions, want me to fix some bugs, etc , please be patient for a response from me.

* ####Credits
I knew nothing about DSP algorithms when I started writing Pebble.  Although I've collected quite a library of DSP books and articles, I learned by looking at the work of others who were kind enough to make their projects open source.  I've included all of the key projects I referenced and in some cases, derived code from, in the gpl.h file you can find in the source tree.  But I especially wanted to thank Moe Wheatley for his outstanding work in making CuteSDR <http://sourceforge.net/projects/cutesdr/> available with source.  While I had working code, Moe's code demonstrated what a professionally written DSP program should look like.  I would also like to thank Michael Ossman for his ongoing work in SDR hardware and education.  HackRF One is one of my favorite devices and Michael's SDR video tutorial <https://greatscottgadgets.com/sdr/> is worth its weight in gold - even if you don't have a HackRF.
	 

##Installation
Installation is easy, just unzip the files into a Pebble directory.  Pebble does not install and does not depend on any other libraries being previously installed, everything is in the directory.  The required OSX version is 10.7 or greater.  Development and testing have been done on OSX 10.11.

Files in the directory are:

1. Pebble: This is the application you click
2. PebbleData: This subdirectory contains all of the configuration files.  Pebble will create a new .ini file in this directory for each SDR device you configure.
3. PebbleRecordings:  This subdirectory is where any I/Q recordings you make (see below) will be stored.
4. Plugins: this subdirectory contains the plugins that Pebble loads on startup.

##Getting Started
1. Find the Device pull-down located below the LO and Mixer buttons.  Pull down the menu and select the SDR device you want to use.
2. To the right of the device pull-down is the device options button, shown with three dots.  Clicking this will bring up a screen with device options.  Options for each device are stored separately, so one a device is set up, you can switch to it quickly.
3. Make sure the Audio Output and other device options such as Sample Rate are set properly, then close the options dialog
4. Find the Power button in the lower left and click it to power-on the receiver.
5. If everything is working correctly, you will see the frequency display light up and show the default frequency for the device.
6. The size and position of the Pebble are saved on exit and restored when the application is re-opened.
7. See the Details section for each area of the UI

Tip: Always used shielded usb cables with ferrite cores whenever possible to reduce noise.  Some inexpensive usb cables do not have the shield connected, so check with ohm meter.

Tip: If no audio on Pebble 2.02 and earlier, using newer OSX - edit pebble.ini and make sure Audio output is set to core audio:built-in output


##Details (Top left to Bottom right)
###Menu  
* ####Pebble|About
Display QT version and other application information
* ####Developer|TestBench
Display TestBench window
* ####Developer|Device Info
Displays device freq range etc
* ####Help|ReadMe
Display readme.html (this file) in PebbleData folder
* ####Help|Credits
Display GPL and other credits

###Tuning Box (10 digits)
There are many ways to tune Pebble.  

1. Direct Frequency Entry: Move the cursor anywhere in the Tuning box and start typing (Numbers and Period only).  As soon as you start typing, a Frequency Entry box will pop-up.  Enter the frequency in kHz and hit the Return key or click Enter/Cancel.  If the entered frequency is within the devices's range, the display will change to reflect what you entered and the device will be set to the new frequency.
2. Up/Down buttons above each digit:  You can change each digit by clicking on the small buttons above and below each digit.
3. Up/Down arrows: Move the cursor over the digit you want to change and press the up or down arrow.
4. Mouse scroll wheel: Move the cursor over the digit you want to change and use the mouse scroll wheel or track pad to scroll up or down.
5. Click any digit to reset all lower digits to zero.  This is a quick way to reset.  

Note: If you enter a frequency above or below the range of the device, you will hear a beep and the frequency will not change.  You can over-ride the hi/lo range in the specific device .ini file.


###Signal Strength  
Signal strength is calculated in the frequency domain and should sync with the spectrum display.  The power for each fft bin within the selected bandpass filter is checked to find the peak and average within the bandpass.  Noise is estimated by looking at the fft bins on either side of the bandpass.
Options are: 
 
* Peak: The peak power within the bandpass in dB
* Average: The average power within bandpass in dB
* Floor: The average signal outside the bandpass (noise) in dB
* SNR: The Signal to Noise ratio = Peak / Floor
* S-Units: The average power in S-Units (approx)
* Ext: For future use to display somthing else
* None: Disables the meter

###Clock  
The clock will use your computer time and timezone to show local or UTC time.  Change using the buttons to the right of the clock.

###LO or Mixer
Below the S-Meter are the LO and Mixer buttons.  When LO (local oscilator) is selected, tuning changes the frequency of the SDR device.  Changing the LO frequency will change what is displayed in the Spectrum window. When Mixer is selected, the SDR device frequency is not changed and the Spectrum display is unchanged.  Instead a local 'mixer' in the DSP code of Pebble is used to tune within the bandwidth of the SDR device.  When LO mode is selected, the local mixer is reset to zero.

You can manually switch between LO and Mixer, but Pebble will also automatically switch when it's clear which mode is desired.

1. Entering a direct frequency will always switch to LO mode.
2. Clicking on a signal in the Spectrum window will always switch to Mixer mode.

###SDR Device selector
Pebble includes a number of SDR device plugins. These are located in the 'plugins' directory and all of the valid device plugins are listed in this drop down. 

Each plugin may support a number of devices.  These are selectable once you open the Options dialog.

The settings for each device are stored in a separate file (see advanced section), so if you have multiple devices, you can quickly restore to the last settings for each one.

Note: The Device selector can not be changed when a device is on.  Power off, select new device, check options, then power back on.

###Device options  
Clicking on the Device Options button '...' opens the Option dialog.  Clicking on it while the dialog is open will close it.  This makes it easy to quickly check something and close the dialog.

The Option button also serves as a 'device status' indicator and will change from Green, to Orange, to Red if the device is not able to keep up with the selected sample rate.  Note that even though the button is Red, you still may be able to receive without data loss.  This is just an indication that there may be a problem.

Except as notes, Device options do not take effect until the device is powered off and back on.

* ####Device Selection
This drop down at the top of the Options dialog lists all of the specifi devices supported by the selected plugin.  Each device may have it's own device-specific options (see below).

* ####Audio Ouput  
Select "Core Audio built-in Output" as default.  Option options may be visible if other output devices are installed

* ####Startup
	1. Last Frequency: The last frequency and mode will be remembered when the device is powered off and will be used the next time the device is powered on
	2. Set Frequency: Enter a frequency in hz which will be used the next time the device is powered on.
	3. Device Default: A default frequency and mode will be used the next time the device is powered on.

* ####Converter Mode & Converter Offset  
Checking this box allows the use of an external up or down converter with any device.  The displayed frequency is the actual frequency after the converter, you don't have to add or subtract the converter offset.  Enter the offset for the converter if this box is checked, typically 100mHz or 120mHz.  In this mode, device high/low frequency limits are not checked so you can tune wherever the converter supports.  But you can also enter frequencies that are outside the range of the device and may see strange results.


* ####SampleRate
If the device uses audio input, these will be the sample rate of the IQ Input field (see below).  If the device supports direct I/Q input, like the rtl-sdr, the list will be device specific.

* ####DC Removal
Some SDR's will display a large spike in the center of the spectrum.  This is usually caused by a DC component in the signal.  Checking this box inserts a high pass filter that removes DC - 10hz.  This should eliminate a DC spike, while having minimal affect on any signal.  If you do experience distortion or other signal degradation, just tune the LO slightly and use the mixer to select the signal.  

* ####I/Q Order
This can be used to invert the incoming I/Q input, or select only the I or Q input for testing.

* ####I/Q Gain
This defaults to 1.0, but can be changed if the device does not have enough power or too much power in the I/Q signals.  This will be evident in the spectrum display.  This value can be changed in real time and the results observed in the Specturm display.  This option can also be used to normalize levels among difference devices so that switch between them keeps a steady level.

* ####I/Q Input
This field is only visible for devices that do not support direct I/Q input.  For devices like SoftRock, select the input audio device that the SoftRock is plugged into.  Note that some Mac's do not support stereo audio input and an external USB audio device must be used.  For devices that create their own audio device, like Funcube, select that as the input.

* ####Reset Settings
This will turn off the device and delete the .ini file which stores the device settings.  The next time the device is powered on, the ini file will automatically be re-created with default values.

* ####Device Specific options
The center of the the Setting page is reserved for device specific options and will change depending on the device selected.  See device specific section of this document.

* ####I/Q Balance
This can be used to manually adjust the gain and phase of the incoming I/Q signals if the device does not produce accurate 90 degree phase shifts.

###Frequency Information
This bar is used to change frequencies to known bands and stations.  It also is used to track frequency changes made elsewhere.  The data for this comes from three .csv files in the PebbleData directory.  Details of how to manually edit these files is in the advanced section.  

	eibi.csv: Stores station specific information
	bands.csv: Stores band categories and bands
	memory.csv: Stores user defined station information

* ####Band Category
There is a large amount of possible band information to select from.  To make this more manageable, all the band information is broken into categories.  To select a frequency, first select a band category from this drop down.
1. All
2. Ham
3. Broadcast
4. Scanner
5. Other

* ####Bands
For each band category, there is a Each Band selection of bands to choose from.  The frequency that will be switched to for each band is defined in bands.csv, along with other band details.  If no default (tune) frequency is set for the band in bands.csv, the middle of the band will be selected.

* ####Stations
For each band, there is a list of specific stations that can be selected.  This list comes from eibi.csv, whicn can be updated using the instructions in PebbleData directory.  It also comes from memory.csv, which is where user defined station information is stored.

* ####"?"
Clicking this will attempt to look up the closest station to the current frequency.

* ####Add
Clicking Add will append an entry to memory.csv.  To fill in the station details, edit memory.csv (save a backup first) using any text editor.

###Spectrum
The Spectrum and Waterfall display has a great deal of hidden functionality. 
 
* Moving the cursor in the display also displays the frequency and power (db) in the lower right corner immediately below the display
* The selected bandpass filter is shown as a shaded area to the left, right or both sides of the frequency line.  Note that this can be hard to see when the device bandwidth is very large, since the bandpass is very small relative to it.  In the waterfall display, the same information is displayed in the cursor bar below the display.
* Clicking anywhere in the Spectrum or Waterfall will switch to Mixer mode and change the frequency accordingly.
* Right clicking (Option Click) anywhere in the Spectrum or Waterfall will change the LO frequency.
* Clicking Up or Down changes the frequency in 100hz increments.  Very useful for fine tuning data signals.
* Clicking Left or Right arrows changes the frequency in 10hz increments.
* Scrolling Up or Down changes the frequency in 1kHz increments.
* Scrolling Left or Right changes the frequency in 100hz increments.
* Changing the mixer frequency using Arrows or Scrolling also will automatically reset the LO when scrolling off either end of the spectrum.  This provides an easy to use continuous tuning mode.

* ####Spectrum Display Selector
1. Spectrum: The entire spectrum is displayed.  The height the spectrum indicates the power (db) of the signal.
2. Waterfall Display: A standard waterfall is displayed.  Colors indicate the power (db) of the signal.
3. Spectrum/Spectrum: The spectrum window is split into a top and bottom, separated by an adjustable splitter.  Grab the splitter and drag up or down to change the sizes of the two windows.  The upper window can also be zoomed on the currently selected frequency, to show finer granularity.  See Spectrum Zoom below
4. Spectrum/Waterfall: Spectrum on top, waterfall on bottom
5. Waterfall/Waterfall: Waterfall on top, waterfall on bottom
6. No Display: This turns off the spectrum and collapses the display to save room.  This can be used when running on a system that does not have enough CPU to keep everything updated.

* ####Spectrum Max db (FS)
This sets the full scale value of the spectrum display, effectively increasing or decreasing the db resolution.  The default is 'auto' which looks at the peak signal in the spectrum and sets the maximum to 10db above the peak.  You can also select specific values from 0 to -50db.

* ####Spectum Min db (BS)
This can be used to set the base or floor value for the spectrum display.  The default is 'auto' which looks at the average signal in the spectrum and sets the minimum db to 10db below the average.  You can also select specific values from -70 to -120db.


* ####Spectrum Refresh (x/sec) spinner
This can be used to speed up or slow down the spectrum display to match your preferences or the band being monitored.  The range is 0 to 30, but 10 is usually a good default.  Faster updates == more CPU, so if your computer is running 'hot' try slowing down the update rate.
Note: If set to Frz, the spectrum display is temporarily frozen and won't update again until you change to another rate.

* ####Spectrum Zoom (only in split window display)
This is one of my favorite features and makes devices that support large bandwidth, like the rtl-sdr and AFEDRI, more useful.  The problem with large bandwidths is that it can be difficult or impossible to see close signals.  This can always be addressed by powering off the device, changing to a lower sample rate, and restarting, but that's very inconvenient.  The Zoom control lets you smoothly zoom on the selected frequency and see a second Waterfall or Spectrum display with the smaller bandwidth and higher resolution in the top display.  

The zoomed display always has the selected frequency in the center.  Clicking on another signal in the zoomed display changes the frequency so that signal is now centered in the zoom display.

Clicking the Hi-Res button switches the upper window to use a different FFT with more resolution, but lower bandwidth.


###Data Modems (WIP)
Pebble support plugin data modems.  This feature is under development and only the Morse modem is currently functional.  Data modems can be independently developed and distributed without requiring new builds of Pebble.  Pebble looks in the 'plugins' directory on startup to build the menu.

A Data Modem opens a new panel, below the spectrum, which is controlled by the plugin.  The panel can be resized by grabbing and dragging the horizontal divider - same as the spectrum divider.

* ####No Data
This collapses the modem display if previously open

* ####Band Data
This is a special modem (not in plugins) that will be used to display extended band and station information.  In this release it is only used in FM Stereo to display RDS station information.

* ####Example Modem
This is a 'Hello World' plugin for testing

* ####Morse Modem (WIP)
This is still a work in process and has problems in high noise environments.

Make sure you are in the correct CWU or CWL mode.  Detected WPM is dynamically updated and displayed, within selected ranges.  
	* 5 to 60 wpm
	* 40 to 120 wpm
	* 100 to 200 wpm

Output can be temporarily turned off and on to 'freeze' the display if needed.  You can select character only display, dot-dash only display, or mixed.  The Clear button will erase everything in the display.  

If you want to save text, press Freeze, select and copy what you want to save, then press Freeze again to display any text that was received while you were copying.

See MorseGenDevice for generating test and practice code samples up to 200wpm.

Mac Tip: If you are using the phone out on your rig to get audio into a program like fldigi, you won't be able to hear the audio because the rigs speaker is cut out when you plug the cable in.  A very handy utility from RogueAmobea called LineIn solves this problem nicely.  https://www.rogueamoeba.com/freebies/

* ####RTTY Modem
Not functional

* ####WWV Modem
Not functional

###AGC
Fast, Medium, Slow, Long, Off.  The AGC threshold level can be adjusted with the slider next to drop down.  When AGC is OFF, this acts as an IF gain.

###Squelch
Based on relative signal strength.

###Volume / Mute
Adjusts local volume from zero to whatever the computer volume is set to.  This does not change the computer volume level.

###Power
Toggle 'power' on/off.

###Record
Click to record whatever is currently displayed.  A wav file will automatically be created in the PebbleRecordings directory.  Pebble recordings store all the incomgin I/Q data.  When played back (Device/File) the result is exactly as when recorded.  Pebble also stored other data such as mode so if you record CW, you get CW when you play it back.  These files are stored as .wav files, but since they have I/Q data you will just hear noise if you play them back with other software.

###Modes
As expected

###Bandwidth
These options change according to the Mode selected.

###Automatic Noise Filter (ANF)
Try it

###Noise Blanker - Spike
Try it

###Noice Blanker - Interference
Try it.

##Device Specifics

If you have any problems with a device or modem plugin, or just don't want to see it in the list because you don't have the device, just move the file from the plugins directory to any other directory.  Pebble will load whatever is in the plugins directory, so the next time you start Pebble, you'll see the change.

###SoftRock Ensemble & 2M, 4M, 6M
(TBW)

###SoftRock V9-ABPF
My mac does not have a built in stereo sound card.  I've found the ASUS Xonar U5 card to work well.  It supports sample rates up to 192ksps and is not very expensive.

###SoftRock Lite II
(TBW)

###FiFi (Softrock Family)
Fifi has an onboard sound device that must be selected as the Pebble I/Q input.  In the device preferences, chose "UDA1361 Input".  You may have to use the Mac Audio Midi Setup tool (in applications/utilities) to set the sample rate to 192k (16bit).  

In order to run the FiFi rockprog configuration utility I had to first install MacPorts popt using "sudo port install popt"

I've also found that updating the firmware with a Mac doesn't work.  It looks like it updated, but the LED doesn't flash and there's no activity.  Always update firmware with Windows, even Parallels works.


###Elektor SDR
Basic FTDI based plugin.  See Mac Mavericks issue in SDR-IQ

###RFSpace SDR-IQ
Note there is a conflict with FTDI driver that Pebble uses (D2XX) and the built in FTDI driver that is included with OSX 10.9 (Mavericks).  There is a way to temporarily disable the Mavericks driver 

	Open a terminal window and type
		sudo kextunload -b com.apple.driver.AppleUSBFTDI
	or run this Mac ScriptEditor in the PebbleData folder
		unload_osx_ftdi_driver.scpt
	
but it has to be re-done after every startup.

Minimum firmware is 1.05

6/15 Install SpectraVue 3.36 to get latest 1.07 firmware

###AFEDRI USB  
Same as SDR-IQ, but with higher sample rate option

###RFSpace SDR-IP & AFEDRI Network
I use a direct network cable, not wifi, when connecting to this device at the full 2mhz bandwidth.  I set my Mac to a fixed IP of 10.0.1.0 using DHCP Manual in Network settings and the AFEDRI to 10.0.1.100.

You will get a "Allow network connections?" box the first time you run any new version.  This is standard Mac protection.

###HPSDR (OZY & Metis)
Note: Use either Ozy (USB) or Metis (TCP), NOT both.

The HPSDR plugin supports sampling rates of up to 384k (with Ozy 2.5 firmware) or Metis.  Supported firmware is in the PebbleData directory and will be automatically uploaded to Ozy as needed. 

Three startup choices

* Auto Discovery: Tries USB an if that fails, tries TCP auto-discovery.  Best choice unless you have a special situation
* Ozy: Will only check for Ozy USB
* Metis: Will use the specific IP/Port entered.  If the IP is blank, then will use Metis auto-discovery

HPSDR suggested slots ()Slot J6 is next to power connector, J1 is furthest away)

* J6 Empty
* J5 Mercury
* J4
* J3 Penelope if installed
* J2 Ozy (USB)
* J1 Metis (TCP)

Tested Firmware.  Other firmware versions can be uploaded by manually editing the HPSDR.ini file.

* Metis 2.6b
* Ozy 2.5
* Mercury 3.4
* Use HPSDRBootLoader on Windows machine with direct network connection.  I had problems with JTAG mode over WiFi.

###FunCube Pro
Plugin complete pending doc

###FunCube Pro Plus
Plugin complete, pending doc

###Wav File SDR
Device Plugin that reads .wav files which have IQ data recorded.  These files are produced by most SDR applications using the 'record' feature and numerous examples can be found on the internet.  Pebble recordings also store LO and mode information in the .wav file.  When you open a Pebble recording, Pebble will show act as if the original SDR device was active.  The last directory is stored in the .ini file for convenience in reopening files.  The default directory is PebbleRecordings.

###RTL2832 Family
Device Plugin that uses rtl-sdr library and wrapper code to work with all rtl-sdr supported devices.  Supports full 2mhz sample rate.  Each USB device has an RTL2832 USB controller and a tuner chip.  Each tuner chip has its own frequency limits and performance. See www.rtl-sdr.com for information and measurements on each tuner.

The two most popular tuner chips are:

* E4000: Direct conversion and may show center spike.  Higher sensitivity at lower frequencies.
* R820: Fewer birdies and higher sensitivity and higer frequencies

USB and TCP Options

* RTL2832 Sample Rate.  The RTL sample rate should be set to a rate where there is no data loss.  It must be >= to the Pebble sample rate.  
* Tuner Gain with preset options. 
	The Tuner gain is set automatically if 'Tuner AGC' is selected.  
	Chose a preset gain option
	NOTE: Tuner AGC may produce multiple aliases.  Try a fixed Tuner gain if this occurs.
	
* Frequency Correction: Tune to a frequency with a known station, like 162.450 mhz NOAA, and use up/down spinners until station is centered.
* Sampling Mode
	* Normal (Frequency range depends on tuner chip)
	* Direct I (Antenna or preamp connected to I pin of ADC)  
	This is for devices like the HF Direct Sampling mod from KNOCK (http://www.easy-kits.com/).  Tuning range is 0 - 28.8.
	* Direct Q (Antenna or preamp connected to Q pin of ADC)
* RTL AGC  
	Check to enable RTL2832 AGC mode (Different than tuner AGC)
* Offset Tuning: Reduces center spike with certain tuners.  Ignored in Direct mode and with R820 tuners
	
TCP Only Options  

* IP Address
* IP Port

If your computer is losing samples (you'll hear choppy audio), try a lower RTL sample rate and/or a lower Pebble sample rate.

To test TCP, run rtl_tcp which defaults to 127.0.0.1:1234.  Make sure IP and Port in Pebble are set to same.

###SDRPlay

SDRPlay BandWidth and IF mode are restricted by sample rate.  Every time you change the sample rate, these selections are checked and reset if necessary.  Like all devices, you must re-start the device for the new sample rate to take effect.

AGC is WIP

Total Gain Reduction


###Ghpsdr3 Servers

WIP

Online servers can be found at <http://qtradio.napan.ca/qtradio/qtradio.pl>

###HackRF One
This is a high speed, wide band SDR.  Supported sample rates for the device are documented as 8msps to 20msps, but blogs and other sources indicate 2msps and up is supported  A decimate factor can be set which reduces the sample rate to something that Pebble can handle.    

(From HackRF wiki <https://github.com/mossmann/hackrf/wiki/FAQ>)  
A good default setting to start with is RF=0 (off), IF=16, baseband=16. Increase or decrease the IF and baseband gain controls roughly equally to find the best settings for your situation. Turn on the RF amp if you need help picking up weak signals. If your gain settings are too low, your signal may be buried in the noise. If one or more of your gain settings is too high, you may see distortion (look for unexpected frequencies that pop up when you increase the gain) or the noise floor may be amplified more than your signal is.

###Morse Generator  
This device simulates incoming morse code and can be used for testing the Morse Code plugin or for practice.  It took on a life of its own ...

There are five generators that can be enabled, each generator output a signal on the specified frequency with the specified options.  You can tune to a signal by using the Mixer (LO is locked and can't be changed) to simulate changing between different stations.  You can also turn on the Pebble Recorder to generate an IQ .wav file that can be used in other programs if desired.

Typical uses  

*  Morse code practice the old fashioned way
*  Testing decoder with different wpm options to see how it adapts to changing signals
*  Testing decoder with close or overlapping signals to see how it distinguishes each one
*  Testing decoder with fading or fist generated signals to see how adaptable timing is
*  Testing decoder with selectable noise levels

Options  
Options are changed in real-time if Pebble is On.  You can open the SDR options page and change any of the below and see the instant result.

* Enable: Turns the generator on/off.  You will see the signal pop-up or disappear.
* Source:
	* Sample1: Canned text
	* Sample2: Canned text
	* Sample3: Canned text
	* File: Load user selectable file.  Several example files can be found in the PebbleData directory.
	* Words: Canned list of common Morse words
	* Abbreviations: Canned list of common Morse abbreviations
	* Table: Dumps the internal Morse table for testing
* Filename: Displays the selected file to be used if Source:File chosen
* File Browse: Displays a file selection dialog
* Frequency: Sets the RF frequency of the generator, plus or minus from zero center.  1000 will generate a sigal 1k up from center. -1000 will generate a signal 1k down from center.
* WPM: Selectable from 5 to 80
* dB: Output level of the signal
* Fade: The output signal level is varied +/- 10db from the selected output level
* Fist: Not implemented yet.  Will introduce variable timing like a typical hand sent Morse signal.
* Noise: Allows selectable level of Gausian noise to be added to the signal.
* Presets: Up to 5 presets can be saved (based on current options) or loaded.  Allows quick changes between common sets of options.	

The sample morse files (300USAqso-1,-2,-3) are courtesy of Jim Weir via G4FON's excellent CW trainer site <http://www.g4fon.net/CW%20Trainer.htm>.

###AirSpy  
WIP  

##SDRGarage

WIP

This is an experimental server application that uses the same device plugins as Pebble.  The concept is to support any SDR device and multiple network protocols.  The first protocol is rtl-tcp because the first device driver is rtl2832.


##Advanced
###PebbleData directory

###Device ini files

###Bands.csv, Memory.csv, EIBI.csv

###PebbleRecordings directory

###Plugins directory

##Source Code
###Build Instructions 
If you need to rebuild from source... 

1. Install QT 5.5.1 (I try to keep up with each QT release) at www.qt.io/downloads 
2. Open pebbleII.pro
3. Edit 'Projects' in QT and add make install as an additional build step in both the Debug and Release configurations.  This will copy all the necessary files into a directory that contains all the files.  Pebble should not require any other software to be installed in order to run in the directory created by make install (MacDebug or MacRelease).
4. Download & install FTDI D2XX drivers from http://www.ftdichip.com/Drivers/D2XX.htm  
	V 1.1.0 4/25/11   
	V 1.2.2 10/30/12
5. Download and install libusb drivers from libusb.info  
	./configure  
	make  
	sudo make install  
	files are in usr/local/lib  
6. Download PortAudio from portaudio.com , open terminal  
	Get latest SVN and make sure configure.in has 10.8 and 10.9 switch statment  
	
		autoreconf -if  
		./configure  
		make  
		
	portaudio directory is at same level as PebbleII  
	if autoreconf not found  
	
		sudo port install automake autoconf libtool #macports
		
7. Download FFTW from <http://github.com/FFTW/fftw3>  
	Current version 3.3.4 as of 2/2016  
	open terminal  
	
		export CFLAGS=-mmacosx-version-min=10.7
		./configure  
		make
		sudo make install  
	
	Header is in api directory, lib is in ./libs directory  
	Or prebuilt fftw from <http://pdb.finkproject.org>	
8. Edit pebbleqt.pri  
	Define FFT library to use (uncomment)  
	Define Audio library to use (uncomment)  
	Define the Mac OSX version you are using  
9. Install latest XCode and all command line tools. NOTE: If you get build errors that look like OS problems, make sure you have the laest update and manually install the command line tools that go with the version of XCode and Mac OS you have.
10. Specific device plugins, like rtl2832, have their own pre-build installation requirements.  See the .pro file for each device

Select the Pebble II project (contains all the sub projects) and build/clean from the menu.  Then select build/Run Qmake to update all the make files from the .pro files.  Finally select build/Build All.  If you have any problems with PebbleLib, select the PebbleLib project and build in first.

Mavericks / XCode 5 does not include a GDB debugger in the command line tools as in previous releases.  QT Creator will work with the new LLVM, but you may need to use GDB for some reason. To install GDB (actually ggdb) using macports:

	sudo port install gdb

If you need to run cmake to build some external utilities and it's not found
	
	sudo port install cmake
		
If you are developing for Windows on a Mac using Parallels you may need to enable UNC paths in order to access files on the Mac side    
To support UNC paths for Parallels, Regedit HKEY_CURRENTUSER/Software Microsoft/Command Processor and add the value DisableUNCCheck REG_DWORD and set the value to 0 x 1 (Hex).  
Make sure to update project version if Qt version is updated in QtCreator
Exit code 0x0c0000315 if "Release DLLs" not copied to Debug and Release dirs

Other useful mac tools  
- XCode Merge (File Diff)  
- Tower GIT UI  
- Mouser MD editor  
- Sublime Text editor

####SDRPlay Plugin Build
Install the latest .so and .h files from sdrplay.com, then copy to the plugins/sdrplay/miricsapi directory.  Files are in /usr/local/include and lib  

Source code for API library: 

####HackRF Plugin Build  
You do not need to build the HackRF toolset to build Pebble, but you will need it to update firmware or run test programs.  Get the latest source from <https://github.com/mossmann/hackrf>.  The hackrf.c and hackrf.h files can be found in this tree and updated in Pebble if necessary.  To build HackRF tools (from GitHub md file)  

	cd host  
	mkdir build  
	cd build  
	cmake ../ -DINSTALL_UDEV_RULES=ON  
	make  
	sudo make install  
	
If you get updated hackrf.c, you will need to make the following changes.

	//#include <libusb.h>
	#include "../d2xx/libusb/libusb/libusb.h"

####AirSpy Plugin Build
Get latest library source from https://github.com/airspy/host/  

####RTL2832 Plugin Build  
Get latest from <http://sdr.osmocom.org/trac/wiki/rtl-sdr>
Libraries are copied from /usr/local/lib

	autoreconf -i
	./configure
	make
	sudo make install
	sudo ldconfig

####PulseAudio 8.0 Build (testing)
See: <https://www.freedesktop.org/wiki/Software/PulseAudio/>  

<http://stackoverflow.com/questions/11912815/compiling-pulseaudio-on-mac-os-x-with-coreservices-h>  

Clone git from git://cgit.freedesktop.org/pulseaudio/pulseaudio/

Testing for use with fldigi

	sudo port selfupdate  
	
	sudo port install autoconf automake intltool libtool libsndfile speex-devel gdbm liboil json-c
	
	#missing ltdl.h
	export CFLAGS="-I/usr/local/include"
	
	#Make sure --with-mac-sysroot points to installed SDK
	./autogen.sh --prefix=/usr/local --disable-jack --disable-hal --disable-bluez --disable-avahi --with-mac-sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/ --with-mac-version-min=10.7 --disable-dbus  
	
	make
	
	#Alternative (not tested) to get 'official' version
	sudo port install PulseAudio

####GNU Radio (testing)
<http://gnuradio.org/redmine/projects/gnuradio/wiki>
<http://gnuradio.org/redmine/projects/gnuradio/wiki/GNURadioLiveDVD>  
Best installed in a Parallels VM for quick access

	sudo port install gnuradio
	
gnuradio-companion

Hardware installation <https://gnuradio.org/redmine/projects/gnuradio/wiki/Hardware>  
Osmocom has all the sdr hardware source blocks of interest.
Edit /opt/local/etc/gnuradio/conf.d/grc.conf  
Add :/usr/local/share/gnuradio/grc/blocks to global_blocks_path 

	gnuradio-config-info #check status

<http://sdr.osmocom.org/trac/wiki/GrOsmoSDR> 

	port install gr-osmosdr
	
or  
To build (This almost works, but I get osmosdr module not found errors in GNURadio) 
 
	mkdir build
	cd build/
	cmake ../ -DCMAKE_INSTALL_PREFIX=/opt/local -DENABLE_NONFREE=TRUE -DENABLE_SOAPY=OFF
	
	#Examine cmake output and check for enabled/disabled modules
	
	make
	sudo make install
	sudo ldconfig
	
	sudo make uninstall #To remove all components
	
	#To build API documentation
	cd build/
	cmake ../ -DENABLE_DOXYGEN=1
	make -C docs

Osmocom Reference (here for convenience)
--   * Python support
--   * Osmocom IQ Imbalance Correction
--   * sysmocom OsmoSDR
--   * FUNcube Dongle
--   * FUNcube Dongle Pro+
--   * IQ File Source
--   * Osmocom RTLSDR
--   * RTLSDR TCP Client
--   * Ettus USRP Devices
--   * Osmocom MiriSDR (SDRPlay)
--   * HackRF Jawbreaker
--   * nuand bladeRF
--   * RFSPACE Receivers
FCD Source

Argument	Notes
fcd=<device-index>	0-based device identifier, optional
device=hw:2	overrides the audio device
type=2	selects the dongle type, 1 for Classic, 2 for Pro+
The "device" argument overrides the audio device used by the underlying driver to access the dongle's IQ sample stream.

The "type" argument selects the dongle type, 1 for Classic, 2 for Pro+.

IQ File Source

Argument	Notes
file=<path-to-file-name>	
freq=<frequency>	Center frequency in Hz, accepts scientific notation
rate=<sampling-rate>	Mandatory, in samples/s, accepts scientific notation
repeat=true|false	Default is true
throttle=true|false	Throttle flow of samples, default is true
OsmoSDR Source

Argument	Notes
osmosdr=<device-index>	0-based device identifier
buffers=<number-of-buffers>	Default is 32
buflen=<length-of-buffer>	Default is 256kB, must be multiple of 512
RTL-SDR Source

Argument	Notes
rtl=<device-index>	0-based device identifier OR serial number
rtl_xtal=<frequency>	Frequency (Hz) used for the RTL chip, accepts scientific notation
tuner_xtal=<frequency>	Frequency (Hz) used for the tuner chip, accepts scientific notation
buffers=<number-of-buffers>	Default is 32
buflen=<length-of-buffer>	Default is 256kB, must be multiple of 512
direct_samp=0|1|2	Enable direct sampling mode on the RTL chip. 0: Disable, 1: use I channel, 2: use Q channel
offset_tune=0|1	Enable offset tune mode for E4000 tuners
NOTE: use rtl_eeprom -s to program your own serial number to the device

NOTE: if you don't specify rtl_xtal/tuner_xtal, the underlying driver will use 28.0MHz

RTL-SDR TCP Source

Argument	Notes
rtl_tcp=<hostname>:<port>	hostname defaults to "localhost", port to "1234"
psize=<payload-size>	Default is 16384 bytes
direct_samp=0|1|2	Enable direct sampling mode on the RTL chip 0=Off, 1=I-ADC input enabled, 2=Q-ADC input enabled
offset_tune=0|1	Enable offset tune mode for E4000 tuners
Miri Source

Argument	Notes
miri=<device-index>	0-based device identifier
buffers=<number-of-buffers>	Default is 32
SDRplay Source

The sdrplay source uses a precompiled (closed source) library available from ​http://www.sdrplay.com/api_drivers.html to interface with the hardware. To enable this nonfree driver you have to call cmake with the additional parameter -DENABLE_NONFREE=TRUE

sdrplay	Use this argument without a value
UHD Source / Sink

Argument	Notes
uhd	Use this argument without a value
nchan=<channel-count>	For multichannel USRP configurations use the subdev parameter to specify stream mapping
subdev=<subdev-spec>	Examples: "A:0", "B:0", "A:0 B:0" when nchan=2. Refer original ettus documentation on this
lo_offset=<frequency>	Offset frequency in Hz, must be within daughterboard bandwidth. Accepts scientific notation
Additional argument/value pairs will be passed to the underlying driver, for more information see ​specifying the subdevice and ​common device identifiers in the Ettus documentation.

bladeRF Source / Sink

Arguments that affect both the source & sink (i.e., the underlying device), when applied to either are marked bold.

Argument	Notes
bladerf[=instance|serial]	Selects the specified bladeRF device by a 0-indexed "device instance" count or by the device's serial number. 3 or more characters from the serial number are required. If 'instance' or 'serial' are not specified, the first available device is used.
fpga=<'/path/to/the/bitstream.rbf'>	Load the FPGA bitstream from the specified file. This is required only once after powering the bladeRF on. If the FPGA is already loaded, this argument is ignored, unless 'fpga-reload=1' is specified.
fpga-reload=1	Force the FPGA to be reloaded. Requires fpga=<bitrstream> to be provided to have any effect.
buffers=<count>	Number of sample buffers to use. Increasing this value may alleviate transient timeouts, with the trade-off of added latency. This must be greater than the 'transfers' parameter. Default=32
buflen=<count>	Length of a sample buffer, in *samples* (not bytes). This must be a multiple of 1024. Default=4096
transfers=<count>	Number of in-flight sample buffer transfers. Defaults to one half of the 'buffers' count.
stream_timeout_ms=<timeout>	Specifies the timeout for the underlying sample stream. Default=3000.
loopback=<mode>	Configure the device for the specified loopback mode (disabled, baseband, or RF). See the libbladeRF documentation for descriptions of these available options: none, bb_txlpf_rxvga2, bb_txlpf_rxlpf, bb_txvga1_rxvga2, bb_txvga1_rxlpf, rf_lna1, rf_lna2, rf_lna3. The default mode is 'none'.
verbosity=<level>	Controls the verbosity of output written to stderr from libbladeRF. The available options, from least to most verbose are: silent, critical, error, warning, info, debug, verbose. The default level is determined by libbladeRF.
xb200[=filter]	Automatic filter selection will be enabled if no value is given to the xb200 parameter. Otherwise, a specific filter may be selected per the list presented below.
The following values are valid for the xb200 parameter:
"custom"  : custom band
"50M"     :  50MHz band
"144M"    : 144MHz band
"222M"    : 222MHz band
"auto3db" : Select fiterbank based on -3dB filter points
"auto"    : Select filerbank based on -1dB filter points (default)


gr-osmosdr <-> bladeRF gain mappings

Sink:
BB Gain: TX VGA1 [-35, -4]
IF Gain: N/A
RF Gain: TX VGA2 [0, 25]

Source:
RF Gain: LNA Gain {0, 3, 6}
IF Gain: N/A
BB Gain: : RX VGA1 + RX VGA2 [5, 60]
HackRF Source / Sink

Argument	Notes
hackrf=<device-index>	0-based device identifier OR serial number
bias=0|1	Disable or enable antenna bias voltage in receive mode (source)
bias_tx=0|1	Disable or enable antenna bias voltage in transmit mode (sink)
buffers=<number-of-buffers>	Default is 32
You can specify either hackrf=0 and hackrf=1 to select by a device index, or the serial number (or an unique suffix of a serial number), hackrf=f00d and hackrf=1234abba. hackrf_info lists multiple devices and their serial numbers. Device selection by serial number tail ("hackrf=beeff00d") requires updated hackrf firmware. The firmware changes have been in the hackrf git master, but there's no official firmware binary published yet (02.06.2015).

Beware of a little catch, there are some examples floating on the net with "hackrf=1" as the device argument. Device index numbers are 0-based (like with rtlsdr and other drivers), so you'll have to use hackrf=0 if you only have a single device attached, hackrf=1 would be the second device. Before this patch the hackrf gr-osmosdr driver did not care about the parameter at all.

Transmit support has been verified by using the crc-mmbTools DAB sdr transmitter.

RFSPACE Source

Argument	Notes
sdr-iq[=<serial-port>]	Optional parameter, serial-port defaults to the serial port (like /dev/ttyUSB0) used by first detected SDR-IQ
sdr-ip[=<hostname>][:<port>]	Optional parameters, hostname defaults to "localhost", port to "50000" or the first detected SDR-IP
netsdr[=<hostname>][:<port>]	Optional parameters, hostname defaults to "localhost", port to "50000" or the first detected NetSDR
nchan=<channel-count>	Optional parameter for NetSDR, must be 1 or 2
The SDR-IP/NetSDR discovery protocol (UDP broadcast) is implemented, thus specifying the ip & port should not be neccessary. Note: for the receiver to operate properly it is required that the UDP packets (port 50000) carrying the sample data can reach your PC, therefore configure your firewall/router/etc. accordingly...

The ftdi_sio driver is being used for SDR-IQ. It creates a character device of the form:

crw-rw---- 1 root dialout 188, 0 Dec 19 22:14 /dev/ttyUSB0

To be able to open the device without root permissions add yourself to the "dialout" group or do a "chmod 666 /dev/ttyUSB0" after pluggin in.

AirSpy? Source

airspy	Use this argument without a value
bias=1|0	Enable or disable DC bias at the antenna input

Apps
osmocom_fft
osmocom_siggen

	
####Inspectrum (testing)
<http://github.com/miek/inspectrum>  
IQ file analysis.  Supports .cf32, .cfile, .cs32, .cu32

	sudo port install fftw-3-single cmake pkgconfig qt5
	mkdir build
	cd build
	cmake ..
	make
	sudo make install #Optional
	./inspectrum
	
###Device Calibration
SDR devices are calibrated to normalize the output and IQ order, making it easy to switch between them.  As of 3/1/16, the following technique is used
  
1. A Red Pitya is used as a signal generator to produce a 10mhz sine wave signal at .0005vpp and fed to the device under test.
2. normalizeIQGain is set to 1.0 in the device code
3. ProcessIQ in receiver.cpp has the signalSpectrum.analyze() call uncommented
4. Tune LO to 10mhz and observe debug output to see what the max db in the spectrum is.  This will correspond to the signal generator output.
5. Compare the db with a target db of -60
6. Change normalizeIQGain in the device code to reflect the necessary adjustment.  For example, to reduce by 10db  

	normalizeIQGain = DB::dbToAmplitude(-10);  

###How to publish a release  

1. Create a new local tag using Tower, ie Pebble_2_0_3
2. Publish the tag to github using Tower
3. In github, create a new release and select the tag just created.  This will create source downloads
    automatically
4. Create a zip file from the local release directory and upload it to github in the release edit form
5. Edit any other comments in the github release edit form
6. In github, publish the release.

###Keyboard Shortcuts
Keyboard shortcuts are provided primarily for use by external tuning devices like the Contour ShuttleExpress.  An example settings file is provided in the PebbleData directory.

Key | Action    |Key|Action
--- | ---------- --- ------
J   | Down 1 hZ  | U |Up 1 hZ
H   | Down 10hZ  | Y |Up 10 hZ
G   | Down 100hZ | T |Up 100 hZ
F   | Down 1khZ  | R |Up 1 khZ
D   | Down 10khZ | E |Up 10khZ
S   | Down 100khZ| W |Up 100khZ
A   | Down 1mhZ  | Q |Up 1mhZ 
    |            |   |
P   | Power
M   | Mute
L   | LO Mode




###Coding Style
Basically a mess right now and needs to be cleaned up.

QT Editor settings

* Use tabs only
* Tab size 4
* Indent case statements

###Internal

There are three main classes that coordinate to do all the work.

**Receiver**: Responsible for all DSP processing, audio, demodulation, mixer, filters and Pebble settings.  Plugins have call-backs to this class for IQ and Audio handling.

**RecieverWidget**: Responsible for all UI, SDR device listing and selection (updates global->sdr).  Communicates to Receiver and Device (sdr*) when UI changes

**DeviceInterface Plugins**:  All device specific interaction.  Device specific settings.  Uses Receiver call-backs to process IQ and audio.  Uses DeviceInterface Get/Set function to get or set 
functionality.

##References

####GNU Licences
<http://www.gnu.org/licenses>  
  
####Dttsp - GPL License
Frank Brikle(ab2kt@arrl.net) & Bob McGwier (rwmcgwier@gmail.com)
DTTS Microwave Society, 6 Kathleen Place, Bridgewater, NJ 08807  
<http://dttsp.sourceforge.net>

####PowerSDR - GPL License
Flex Radio
sales@flex-radio.com.
FlexRadio Systems, 8900 Marybank Dr., Austin, TX 78750  
<http://www.flexradio.com>

####SDRMax - GPL License
Philip A Covington
p.covington@gmail.com  
<http://www.srl-llc.com/qs1r_latest/>

####FFTW - See fftw3.h for details
Matteo Frigo, Massachusetts Institute of Technology  
<http://www.fftw.org/>

####PortAudio - See portaudio.h for details
Ross Bencina, Phil Burk  
<http://http://www.portaudio.com>

####QT Framework (4.63, 5.2, 5.6 ...)
dll's distributed under GPL and LPGL license from qt.io  
Source code and license details available at  
<http://qt.io>

####HIDAPI - Cross platform HIDAPI source code
HIDAPI cross platform source files from  
<http://www.signal11.us/oss/hidapi/>

####FTDI - Cross platform USB lib for ftdi and libusb
<http://www.ftdichip.com/Drivers/D2XX.htm>

####NUE-PSK
Source code examples for Goertzel and CW decoding David M.Collins, AD7JT George Heron, N2APB  
<http://www.nue-psk.com>  

####RTL-SDR 
rtl-sdr is developed by Steve Markgraf, Dimitri Stolnikov, and Hoernchen, with contributions by Kyle Keen, Christian Vogel and Harald Welte.  
Latest rtl-sdr source: git clone git://git.osmocom.org/rtl-sdr.git  
<http://sdr.osmocom.org>    
<http://www.rtl-sdr.com>  
 
####CuteSDR
Many of the new and updated algorithms in Pebble 2.0 are taken or derived from this code.
I can not say enough good things about how much I learned and how much it helped!
Thank you Moe.
<http://http://cutesdr.sourceforge.net/>

####FLDigi
<http://www.w1hkj.com/Fldigi.html>

####CocoaModem  
<http://www.w7ay.net/site/Applications/cocoaModem/>

####AG1LE Blog
<http://ag1le.blogspot.com/>

####Sound Effects
<http://www.soundjay.com/beep-sounds-1.html>

####HIDAPI
Mac and win functions for accessing HID devices
<http://www.signal11.us/oss/hidapi/>

####SDRPlay
<http://www.sdrplay.com> 

####HackRF One  
<https://github.com/mossmann/hackrf>

####Good article on FFT with source code
<http://librow.com/articles/article-10>

####Other Books and Articles
* "Digital Signal Processing Technology", Doug Smith KF6DX, ARRL
* "Computers, Microcontrollers and DSP for the Radio Amateur", Andy Talbot G4JNT, RSGB
* "A Software Defined Radio for the Masses", Gerald Youngblood AC5OG, QEX 7/2002 - 3/2003
* "Signals, Samples and Stuff, a DSP Tutorial", Doug Smith KF6DX, QEX 1998
* "Scientists and Engineers Guide to Digital Signal Processing" Steven Smith
* "An Introduction to HF Software Defined Radio" Andrew Barron
* "Software Receiver Design" C Richard Johnson ...
* "Modern Communications Receiver Design and Technology" Cornell Drentea
* Other as noted in comments



