#pragma once
/*
    This file is part of a program that implements a Software-Defined Radio.

    Copyright (C) 2010,2011,2012,2013,2014  Richard A. Landsman
    Author may be contacted by email at PebbleSDR@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.


List of Sources, Algorithms and References, and thank-you's:
    Dttsp - GPL License
        Frank Brikle(ab2kt@arrl.net) & Bob McGwier (rwmcgwier@gmail.com)
        DTTS Microwave Society, 6 Kathleen Place, Bridgewater, NJ 08807
    PowerSDR - GPL License
        Flex Radio
        sales@flex-radio.com.
        FlexRadio Systems, 8900 Marybank Dr., Austin, TX 78750
    SDRMax - GPL License
        Philip A Covington
        p.covington@gmail.com
    FFTW - See fftw3.h for details
        Matteo Frigo, Massachusetts Institute of Technology
    PortAudio - See portaudio.h for details
        Ross Bencina, Phil Burk
        Latest version available at: http://www.portaudio.com/
    QT Framework (4.63, 5.2, ...) - dll's distributed under GPL and LPGL license from Nokia
        Source code and license details available at http://qt.nokia.com/downloads
    HIDAPI - Cross platform HIDAPI source code
        HIDAPI cross platform source files from http://www.signal11.us/oss/hidapi/
    FTDI - Cross platform USB lib for ftdi and libusb
        http://www.ftdichip.com/Drivers/D2XX.htm
    NUE-PSK
        www.nue-psk.com
        Source code examples for Goertzel and CW decoding David M.Collins, AD7JT George Heron, N2APB
    RTL-SDR
        http://sdr.osmocom.org
        rtl-sdr is developed by Steve Markgraf, Dimitri Stolnikov, and Hoernchen, with contributions by Kyle Keen, Christian Vogel and Harald Welte.
        Latest rtl-sdr source: git clone git://git.osmocom.org/rtl-sdr.git
        http://sdr.osmocom.org/trac/wiki/rtl-sdr
    CuteSDR
        Many of the new and updated algorithms in Pebble 2.0 are taken or derived from this code.
        I can not say enough good things about how much I learned and how much it helped!
        Thank you Moe.
        http://http://cutesdr.sourceforge.net/
    FLDigi
        http://www.w1hkj.com/Fldigi.html
    CocoaModem
        http://www.w7ay.net/site/Applications/cocoaModem/
    AG1LE Blog
        http://ag1le.blogspot.com/




    Books and Articles
    "Digital Signal Processing Technology", Doug Smith KF6DX, ARRL
    "Computers, Microcontrollers and DSP for the Radio Amateur", Andy Talbot G4JNT, RSGB
    "A Software Defined Radio for the Masses", Gerald Youngblood AC5OG, QEX 7/2002 - 3/2003
    "Signals, Samples and Stuff, a DSP Tutorial", Doug Smith KF6DX, QEX 1998
    "Scientists and Engineers Guide to Digital Signal Processing" Steven Smith

    Other as noted in comments

*/
