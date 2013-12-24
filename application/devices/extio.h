#ifndef EXTIO_H
#define EXTIO_H
//From HDSDR.DE
//---------------------------------------------------------------------------
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"

#ifndef LC_ExtIO_TypesH
#define LC_ExtIO_TypesH

// specification from http://www.sdradio.eu/weaksignals/bin/Winrad_Extio.pdf
// linked referenced from http://www.weaksignals.com/

// function implemented by Winrad / HDSDR; see enum extHWstatusT below
typedef int               (* pfnExtIOCallback)  (int cnt, int status, float IQoffs, void *IQdata);

// mandatory functions, which have to be implemented by ExtIO DLL
typedef bool    (__stdcall * pfnInitHW)         (char *name, char *model, int& hwtype);
typedef bool    (__stdcall * pfnOpenHW)         (void);
typedef void    (__stdcall * pfnCloseHW)        (void);
typedef int     (__stdcall * pfnStartHW)        (int extLOfreq);
typedef void    (__stdcall * pfnStopHW)         (void);
typedef void    (__stdcall * pfnSetCallback)    (pfnExtIOCallback funcptr);
typedef int     (__stdcall * pfnSetHWLO)        (long extLOfreq); //what will happen when extLOfreq > 2GHz???
typedef int     (__stdcall * pfnGetStatus)      (void);

// optional functions, which can be implemented by ExtIO DLL
// for performance reasons prefer not implementing rather then implementing empty functions
//   especially for RawDataReady
typedef long    (__stdcall * pfnGetHWLO)        (void);
typedef long    (__stdcall * pfnGetHWSR)        (void);
typedef void    (__stdcall * pfnRawDataReady)   (long samprate, void *Ldata, void *Rdata, int numsamples);
typedef void    (__stdcall * pfnShowGUI)        (void);
typedef void    (__stdcall * pfnHideGUI)        (void);
typedef void    (__stdcall * pfnTuneChanged)    (long tunefreq);
typedef long    (__stdcall * pfnGetTune)        (void);
typedef void    (__stdcall * pfnModeChanged)    (char mode);
typedef char    (__stdcall * pfnGetMode)        (void);
typedef void    (__stdcall * pfnIFLimitsChanged)(long lowfreq, long highfreq);
typedef void    (__stdcall * pfnFiltersChanged) (int loCut, int hiCut, int pitch);  // lo/hiCut relative to tuneFreq
typedef void    (__stdcall * pfnMuteChanged)    (bool muted);
typedef void    (__stdcall * pfnGetFilters)     (int& loCut, int& hiCut, int& pitch);

// optional functions, which can be implemented by ExtIO DLL
// following functions may get called from HDSDR 2.13 and above

typedef void    (__stdcall * pfnVersionInfo)    (const char * progname, int ver_major, int ver_minor);  // called after successful InitHW() if exists

typedef int     (__stdcall * pfnGetAttenuators) (int idx, float * attenuation);  // fill in attenuation
                             // use positive attenuation levels if signal is amplified (LNA)
                             // use negative attenuation levels if signal is attenuated
                             // sort by attenuation: use idx 0 for highest attenuation / most damping
                             // this functions is called with incrementing idx
                             //    - until this functions return != 0 for no more attenuator setting
typedef int     (__stdcall * pfnGetActualAttIdx)(void);                          // returns -1 on error
typedef int     (__stdcall * pfnSetAttenuator)  (int idx);                       // returns != 0 on error


typedef int     (__stdcall * pfnSetModeRxTx)    (int modeRxTx);
typedef int     (__stdcall * pfnActivateTx)     (int magicA, int magicB);
typedef int     (__stdcall * pfnDeactivateBP)   (int deactivate);  // deactivate all bandpass filter to allow "bandpass undersampling"
                             // deactivate == 1 to deactivate all bandpass filters of hardware
                             // deactivate == 0 to reactivate automatic bandpass selection depending on frequency

// future functions ..
//typedef int     (__stdcall * pfnGetSamplerates) (int idx, double * samplerate);  // fill in possible samplerates
                             // this functions is called with incrementing idx
                             //    - until this functions return != 0 for no more samplerates
//typedef int     (__stdcall * pfnSetSamplerate)  (int idx);                       // returns != 0 on error


// hwtype codes to be set with pfnInitHW
// Please ask Alberto di Bene (i2phd@weaksignals.com) for the assignment of an index code
// for cases different from the above.
typedef enum
{
    exthwNone =0
  , exthwSDR14
  , exthwSDRX
  , exthwUSBdata16  // the hardware does its own digitization and the audio data are returned to Winrad
                    // via the callback device. Data must be in 16-bit  (short) format, little endian.
  , exthwSCdata     // The audio data are returned via the (S)ound (C)ard managed by Winrad. The external
                    // hardware just controls the LO, and possibly a preselector, under DLL control.
  , exthwUSBdata24  // the hardware does its own digitization and the audio data are returned to Winrad
                    // via the callback device. Data are in 24-bit  integer format, little endian.
  , exthwUSBdata32  // the hardware does its own digitization and the audio data are returned to Winrad
                    // via the callback device. Data are in 32-bit  integer format, little endian.
  , exthwUSBfloat32 // the hardware does its own digitization and the audio data are returned to Winrad
                    // via the callback device. Data are in 32-bit  float format, little endian.
  , exthwHPSDR      // for HPSDR only!
} extHWtypeT;

// status codes for pfnExtIOCallback; used when cnt < 0
typedef enum
{
  // only processed/understood for SDR14
    extHw_Disconnected        = 0     // SDR-14/IQ not connected or powered off
  , extHw_READY               = 1     // IDLE / Ready
  , extHw_RUNNING             = 2     // RUNNING  => not disconnected
  , extHw_ERROR               = 3     // ??
  , extHw_OVERLOAD            = 4     // OVERLOAD => not disconnected

  // for all extIO's
  , extHw_Changed_SampleRate  = 100   // sampling speed has changed in the external HW
  , extHw_Changed_LO          = 101   // LO frequency has changed in the external HW
  , extHw_Lock_LO             = 102
  , extHw_Unlock_LO           = 103
  , extHw_Changed_LO_Not_TUNE = 104   // CURRENTLY NOT YET IMPLEMENTED
                                      // LO freq. has changed, Winrad must keep the Tune freq. unchanged
                                      // (must immediately call GetHWLO() )
  , extHw_Changed_TUNE        = 105   // a change of the Tune freq. is being requested.
                                      // Winrad must call GetTune() to know which value is wanted
  , extHw_Changed_MODE        = 106   // a change of demod. mode is being requested.
                                      // Winrad must call GetMode() to know the new mode
  , extHw_Start               = 107   // The DLL wants Winrad to Start
  , extHw_Stop                = 108   // The DLL wants Winrad to Stop
  , extHw_Changed_FILTER      = 109   // a change in the band limits is being requested
                                      // Winrad must call GetFilters()

  // Above status codes are processed with Winrad 1.32.
  //   All Winrad derivation like WRplus, WinradF, WinradHD and HDSDR should understand them,
  //   but these do not provide version info with VersionInfo(progname, ver_major, ver_minor).

  , extHw_Mercury_DAC_ON      = 110   // enable audio output on the Mercury DAC when using the HPSDR
  , extHw_Mercury_DAC_OFF     = 111   // disable audio output on the Mercury DAC when using the HPSDR
  , extHw_PC_Audio_ON         = 112   // enable audio output on the PC sound card when using the HPSDR
  , extHw_PC_Audio_OFF        = 113   // disable audio output on the PC sound card when using the HPSDR

  , extHw_Audio_MUTE_ON       = 114   // the DLL is asking Winrad to mute the audio output
  , extHw_Audio_MUTE_OFF      = 115   // the DLL is asking Winrad to unmute the audio output

  // Above status codes are processed with Winrad 1.33 and HDSDR
  //   Winrad 1.33 and HDSDR still do not provide their version with VersionInfo()


  // Following status codes are processed when VersionInfo delivers
  //  0 == strcmp(progname, "HDSDR") && ( ver_major > 2 || ( ver_major == 2 && ver_minor >= 13 ) )

  // all extHw_XX_SwapIQ_YYY callbacks shall be reported after each OpenHW() call
  , extHw_RX_SwapIQ_ON        = 116   // additionaly swap IQ - this does not modify the menu point / user selection
  , extHw_RX_SwapIQ_OFF       = 117   //   the user selected swapIQ is additionally applied
  , extHw_TX_SwapIQ_ON        = 118   // additionaly swap IQ - this does not modify the menu point / user selection
  , extHw_TX_SwapIQ_OFF       = 119   //   the user selected swapIQ is additionally applied


  // Following status codes (for I/Q transceivers) are processed when VersionInfo delivers
  //  0 == strcmp(progname, "HDSDR") && ( ver_major > 2 || ( ver_major == 2 && ver_minor >= 13 ) )

  , extHw_TX_Request          = 120   // DLL requests TX mode / User pressed PTT
                                      //   exciter/transmitter must wait until SetModeRxTx() is called!
  , extHw_RX_Request          = 121   // DLL wants to leave TX mode / User released PTT
                                      //   exciter/transmitter must wait until SetModeRxTx() is called!
  , extHw_CW_Pressed          = 122   // User pressed  CW key
  , extHw_CW_Released         = 123   // User released CW key
  , extHw_PTT_as_CWkey        = 124   // handle extHw_TX_Request as extHw_CW_Pressed in CW mode
                                      //  and   extHw_RX_Request as extHw_CW_Released
  , extHw_Changed_ATT         = 125   // Attenuator changed => call GetActualAttIdx()

} extHWstatusT;

// codes for pfnSetModeRxTx:
typedef enum
{
    extHw_modeRX  = 0
  , extHw_modeTX  = 1
} extHw_ModeRxTxT;


#endif /* LC_ExtIO_TypesH */

#endif // EXTIO_H
