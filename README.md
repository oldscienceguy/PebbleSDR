#Pebble II SDR (Mac) 
December 26, 2013  
Copyright 2010, 2011, 2012, 2013, 2014  
Richard Landsman N1DDY <PebbleSDR@gmail.com>  
Licensed under GPL, see gpl.h for details, attribution, and references  
<https://github.com/oldscienceguy/PebbleSDR>  
Dedicated to SuperRatt, my first program (c 1980) for the Apple II with support for RTTY, CW, and special modes.  

---
AFEDRI showing full 2mHz bandwidth from 150kHz to 2150kHz and zoom at 1150 kHz  
<img src=https://raw.github.com/oldscienceguy/PebbleSDR/master/screen_shots/AFEDRI%20150Hz%20to%202150kHz.png width=500> 
---
##Introduction

I 'discovered' Software Defined Radio at the beginning of 2010 when I found myself between jobs and some extra time.  The experience of building my first SoftRock V9 and actually hearing something was like the first crackle from a crystal radio.  I wasn't quite sure everything was working, but it made noise!  I was hooked and soon wanted to learn about the magical DSP that made everything work.  Rocky was the first SDR program I used, and I thought Pebble - a small rock, was an appropriate name for my new program.

Pebble is now a complete SDR program, written in QT/C++.  The original version was Windows only and had only basic functionality.  This second version is substantially improved and mostly re-written.  Development for V 2 was done on a Mac and so Pebble II is Mac only for the time being.  Once stable, it will be back-ported to Windows.

There are a few design principles I tried to follow throughtout the evolution of Pebble
####Compact  
There are plenty of full featured SDR applications these days, but many of them take up the entire screen and present an overwhelming number of options to the average user.  I wanted something that had all the key features in a compact UI, leaving room for other programs to be visible on an average screen.
####Incremental 
In order to keep the UI compact, more advanced and optional functionality had to be exposed incrementally.  There when you need it, not there when you don't.  This led to features like being able to collapse the spectrum and data windows.

####Extensible
In addition to being extensible by virtue of being open source, I wanted to be able to keep the core functionality intact, while exploring new data modes and options.  This led to the modem plug-in architecture and eventually will include cross platform extio-like device support.

####Disclaimer
The source code for this project represents three years of incremental, "drive-by" coding, with numerous experiments, re-writes, re-factoring, re-everything.  No attempt has been made to pretty up the code or clean up comments.  In fact, in many places I have deliberately left alternative implementations and detailed comments in the code for future reference.

####Credits
I knew nothing about DSP algorithms when I started writing Pebble.  Although I've collected quite a library of DSP books and articles, I learned by looking at the work of others who were kind enough to make their projects open source.  I've included all of the key projects I referenced and in some cases, derived code from, in the gpl.h file you can find in the source tree.  But I especially wanted to thank Moe Wheatley for his outstanding work in making CuteSDR <http://sourceforge.net/projects/cutesdr/> available with source.  While I had working code, Moe's code demonstrated what a professionally written DSP program should look like.

##Installation
Installation is easy, just unzip the files into a Pebble directory.  Pebble does not install and does not depend on any other libraries being previously installed, everything is in the directory.

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
6. See the Details section for each area of the UI

##Details (Top left to Bottom right)
###Tuning Box (10 digits)
There are many ways to tune Pebble.  

1. Direct Frequency Entry: Move the cursor anywhere in the Tuning box and start typing (Numbers and Period only).  As soon as you start typing, a Frequency Entry box will pop-up.  Enter the frequency in kHz and hit the Return key or click Enter/Cancel.  If the entered frequency is within the devices's range, the display will change to reflect what you entered and the device will be set to the new frequency.
2. Up/Down buttons above each digit:  You can change each digit by clicking on the small buttons above and below each digit.
3. Up/Down arrows: Move the cursor over the digit you want to change and press the up or down arrow.
4. Mouse scroll wheel: Move the cursor over the digit you want to change and use the mouse scroll wheel or track pad to scroll up or down.
5. Click any digit to reset all lower digits to zero.  This is a quick way to reset.

###S-Meter  
The S-Meter can be set to Instaneous or Average using the buttons to the right of the bar graph.

###Clock  
The clock will use your computer time and timezone to show local or UTC time.  Change using the buttons to the right of the clock.

###LO or Mixer
Below the S-Meter are the LO and Mixer buttons.  When LO (local oscilator) is selected, tuning changes the frequency of the SDR device.  Changing the LO frequency will change what is displayed in the Spectrum window. When Mixer is selected, the SDR device frequency is not changed and the Spectrum display is unchanged.  Instead a local 'mixer' in the DSP code of Pebble is used to tune within the bandwidth of the SDR device.  When LO mode is selected, the local mixer is reset to zero.

You can manually switch between LO and Mixer, but Pebble will also automatically switch when it's clear which mode is desired.

1. Entering a direct frequency will always switch to LO mode.
2. Clicking on a signal in the Spectrum window will always switch to Mixer mode.

###SDR Device selector
A number of SDR devices are directly supported by Pebble and can be selected with this pull down.  The settings for each device are stored in a separate file (see advanced section), so if you have multiple devices, you can quickly restore to the last settings for each one.

Note: The Device selector can not be changed when a device is on.  Power off, select new device, check options, then power back on.

###Device options  
Except as notes, Device options do not take effect until the device is powered off and back on.

###Audio Ouput  
Select "Core Audio built-in Output" as default.  Option options may be visible if other output devices are installed

###Startup
1. Last Frequency: The last frequency and mode will be remembered when the device is powered off and will be used the next time the device is powered on
2. Set Frequency: Enter a frequency in hz which will be used the next time the device is powered on.
3. Device Default: A default frequency and mode will be used the next time the device is powered on.

###SampleRate
If the device uses audio input, these will be the sample rate of the IQ Input field (see below).  If the device supports direct I/Q input, like the rtl-sdr, the list will be device specific.

###I/Q Order
This can be used to invert the incoming I/Q input, or select only the I or Q input for testing.

###I/Q Gain
This defaults to 1.0, but can be changed if the device does not have enough power or too much power in the I/Q signals.  This will be evident in the spectrum display.  This value can be changed in real time and the results observed in the Specturm display.  This option can also be used to normalize levels among difference devices so that switch between them keeps a steady level.

###I/Q Input
This field is only visible for devices that do not support direct I/Q input.  For devices like SoftRock, select the input audio device that the SoftRock is plugged into.  Note that some Mac's do not support stereo audio input and an external USB audio device must be used.  For devices that create their own audio device, like Funcube, select that as the input.

###Test Bench
If this box is checked, a diagnostic Test Bench is displayed along side the main Pebble window when the device is powered on.  This is mostly used for debugging device and DSP code.

###Device Specific options
The center of the the Setting page is reserved for device specific options and will change depending on the device selected.  See device specific section of this document.

###I/Q Balance
This can be used to manually adjust the gain and phase of the incoming I/Q signals if the device does not produce accurate 90 degree phase shifts.

###Frequency Information
This bar is used to change frequencies to known bands and stations.  It also is used to track frequency changes made elsewhere.  The data for this comes from three .csv files in the PebbleData directory.  Details of how to manually edit these files is in the advanced section.  

1. eibi.csv: Stores station specific information
2. bands.csv: Stores band categories and bands
3. memory.csv: Stores user defined station information

###Band Category
There is a large amount of band information that can be selected from.  To make this more manageable, all the band information is broken into categories.  To select a frequency, first select a band category from this drop down.
1. All
2. Ham
3. Broadcast
4. Scanner
5. Other

###Bands
For each band category, there is a Each Band selection of bands to choose from.  The frequency that will be switched to for each band is defined in bands.csv, along with other band details.

###Stations
For each band, there is a list of specific stations that can be selected.  This list comes from eibi.csv, whicn can be updated using the instructions in PebbleData directory.  It also comes from memory.csv, which is where user defined station information is stored.

###"?"
Clicking this will attempt to look up the closest station to the current frequency.

###Add
Clicking Add will append an entry to memory.csv.  To fill in the station details, edit memory.csv (save a backup first) using any text editor.

###Spectrum
The Spectrum and Waterfall display has a great deal of hidden functionality. 
 
* Moving the cursor in the display also displays the frequency and power (db) in the lower right corner immediately below the display
* The selected bandpass filter is shown as a shaded area to the left, right or both sides of the frequency line.  Note that this can be hard to see when the device bandwidth is very large, since the bandpass is very small relative to it.  In the waterfall display, the same information is displayed in the cursor bar below the display.
* Clicking anywhere in the Spectrum or Waterfall will switch to Mixer mode and change the frequency accordingly.
* Right clicking (Option Click) anywhere in the Spectrum or Waterfall will change the LO frequency.
* Clicking Up or Down changes the frequency in 100hz increments.  Very useful for fine tuning data signals.
* Clicking Left or Right arrows changes the frequency in 10hz increments.
* Scrolling Up or Down changes the frequench in 1kHz increments.
* Scrolling Left or Right changes the frequency in 100hz increments.
* Changing the frequency using Arrows or Scrolling also will automatically reset the LO when scrolling off either end of the spectrum.  This provides an easy to use continuous tuning mode.

###Spectrum Display Selector
1. Spectrum: The entire spectrum is displayed (see Span below).  The height the spectrum indicates the power (db) of the signal.
2. Waterfall Display: A standard waterfall is displayed.  Colors indicate the power (db) of the signal.
3. No Display: This turns off the spectrum and collapses the display to save room.  This can be used when running on a system that does not have enough CPU to keep everything updated.

###Spectrum dB range
This can be used to increase or decrease the resolution of the display, especially when looking at the Waterfall.  The default range is -120db (black) to -50db (red).  If the Waterfall is washed out, increase the max db displayed to -30.  If no signals are visible, decrease the max db displayed to -70.  Easier to see when you try it than to explain

###Spectrum Span
This is one of my favorite features and makes devices that support large bandwidth, like the rtl-sdr and AFEDRI, more useful.  The problem with large bandwidths is that it can be difficult or impossible to see close signals.  This can always be addressed by powering off the device, changing to a lower sample rate, and restarting, but that's very inconvenient.  The Span control lets you smoothly zoom on the selected frequency and see a second Waterfall or Spectrum display with the smaller bandwidth and higher resolution.  

The zooming is very smooth, but as you get closer to the maximum you will suddenly see finer detail and resolution as the internal FFT switches to a separate zoom mode.  Even with a 2mHz bandwidth, a 32khz span can be resolved.

The zoomed display always has the selected frequency in the center.  Clicking in the zoomed display changes the frequency.

Try sliding the Span control back and forth on a busy part of the band to see how it works.

###Data Modems
Pebble support plugin data modems.  This feature is under development and only the Morse modem is currently functional.  Data modems can be independently developed and distributed without requiring new builds of Pebble.  Pebble looks in the 'plugins' directory on startup to build the menu.

###No Data
This collapses the modem display if previously open

###Band Data
This is a special modem (not in plugins) that will be used to display extended band and station information.  In this release it is only used in FM Stereo to display RDS station information.

###Example Modem
This is a 'Hello World' plugin for testing

###Morse Modem
Make sure you are in the correct CWU or CWL mode and tune until you get maximum levels in the bar graph.  Detected WPM is dynamically updated and displayed.  WPM can be locked by checking the box.  Output can be temporarily turned off and on to 'freeze' the display if needed.  Just because, you can select character only display, dot-dash only display, or mixed.  Reset button puts everything back to defaults in case internal tracking algorithms get confused.

###RTTY Modem
Not functional

###WWV Modem
Not functional

###AGC
Fast, Medium, Slow, Long, Off.  Level can be adjusted with slider next to drop down.  Acts as a kind of RF gain if needed.

###Squelch
Non-sophisticated based on volume levels.

###Volume / Mute
As expected

###Power
As expected

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
###SoftRock Ensemble & 2M, 4M, 6M
(TBW)

###SoftRock V9-ABPF
(TBW)

###SoftRock Lite II
(TBW)

###FiFi
Fifi has an onboard sound device that must be selected as the Pebble I/Q input.  In the device preferences, chose "Core Audio:UDA1361".  Make sure you set the Input level in the Mac Audio settings as well.  

In order to run the FiFi rockprog configuration utility I had to first install MacPorts popt using "sudo port install popt"

I've also found that updating the firmware with a Mac doesn't work.  It looks like it updated, but the LED doesn't flash and there's no activity.  Always update firmware with Windows, even Parallels works.


###Elektor SDR
(TBW)

###RFSpace SDR-IQ
(TBW)

###RFSpace SDR-IP & AFEDRI
I use a direct network cable, not wifi, when connecting to this device at the full 2mhz bandwidth.  I set my Mac to a fixed IP of 10.0.1.101 and the AFEDRI to 10.0.1.100.

You will get a "Allow network connections?" box the first time you run any new version.  This is standard Mac protection.

###HPSDR-USB
(TBW)

###HPSDR-Network (Not supported yet)

###FunCube Pro
(TBW)

###FunCube Pro Plus
(TBW)

###File
(TBW)

###RTL2832 Family
(TBW)

##Advanced
###PebbleData directory

###Device ini files

###Bands.csv, Memory.csv, EIBI.csv

###PebbleRecordings directory

###Plugins directory

##Source Code notes
(TBW)