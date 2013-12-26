#Pebble II SDR (Mac)

##Introduction

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
A number of SDR devices are directly supported by Pebble and can be selected with this pull down.  The settings for each device are stored in a separate file (see advanced section), so if you have multiple devices, you can quickly restore

###Device options

###Frequency Category

###Frequency Range

###Station ID

###Memory

###Spectrum Display

###Waterfall Display

###No display (CPU saving)

###Spectrum dB range

###Spectrum Zoom

###Data Modems

###AGC

###Squelch

###Volume / Mute

###Record

###Modes

###Bandwidth

###Automatic Noise Filter (ANF)

###Noise Blanker - Spike

###Noice Blanker - Interference

##Device Specifics


##Advanced
###PebbleData directory

###Device ini files

###Bands.csv, Memory.csv, EIBI.csv

###PebbleRecordings directory

###Plugins directory

