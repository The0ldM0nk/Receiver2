//LoRaTracker_Settings.h
/*
******************************************************************************************************

LoRaTracker Programs for Arduino

Copyright of the author Stuart Robinson - 05/08/2017

http://www.LoRaTracker.uk
  
These programs may be used free of charge for personal, recreational and educational purposes only.

This program, or parts of it, may not be used for or in connection with any commercial purpose without
the explicit permission of the author Stuart Robinson.

The programs are supplied as is, it is up to individual to decide if the programs are suitable for the
intended purpose and free from errors.

To Do:


******************************************************************************************************
*/

//**************************************************************************************************
// 1) Hardware related definitions and options - specify board type here
//**************************************************************************************************

#define UseSD                                              //Select to use SD card for logging
#define Board_Definition "Receiver2_Board_Definitions.h"   //define the board definition file to use here

//**************************************************************************************************
// 2) Program Options
//**************************************************************************************************

//#define ClearConfigData                 //zero the config memory
//#define ClearAllMemory                  //Clears from start memory to end memory, normally 1kbyte, needs to be followed by ConfigureDefaults
//#define ConfigureDefaults                 //Configure settings from default program defaults, these are then stored in memory
#define ConfigureFromMemory               //Configure settings from values stored in memory, this needs to be the active mode for bind to work

#define CalibrateTone                     //comment in to have a calibrate tone at startup

#define SendBind                          //if defined at startup tracker transmitter will send a bind packet
#define write_CalibrationOffset           //if defined will write calibration constant to memory, needs to be done once only.

#define DEBUG                             //if defined debug mode used, results in more diagnostics to serial terminal    
//#define ReceiveBind                     //during flight allows for a bind to be received    



//**************************************************************************************************
// 3) Frequency settings
//**************************************************************************************************

//Tracker mode
const unsigned long TrackerMode_Frequency = 434400000;   

//Search mode
const unsigned long SearchMode_Frequency = 434300000;   

//Command mode
const unsigned long CommandMode_Frequency = 434500000;   

//Bind mode - Change this with great care !!!!!
const unsigned long BindMode_Frequency = 434100000;      

//this is the LoRa module frequency calibration offset in Hertz
const int CalibrationOffset = 500;                    


//**************************************************************************************************
// 4) LoRa settings
//**************************************************************************************************

#define LoRa_Device_in_MB1                  //if using a MikroBus based specify the socket for the LoRa Device 

//Tracker mode
const byte TrackerMode_Power = 10;
#define TrackerMode_Bandwidth BW62500
#define TrackerMode_SpreadingFactor SF8
#define TrackerMode_CodeRate CR45

//Search mode
const byte SearchMode_Power = 10;
#define SearchMode_Bandwidth BW62500
#define SearchMode_SpreadingFactor SF12
#define SearchMode_CodeRate CR45

//Command mode
const byte CommandMode_Power = 5;
#define CommandMode_Bandwidth BW62500
#define CommandMode_SpreadingFactor SF10
#define CommandMode_CodeRate CR45


//Bind mode - Change this with great care !!!!!
const byte BindMode_Power = 2;
#define BindMode_Bandwidth BW500000
#define BindMode_SpreadingFactor SF8
#define BindMode_CodeRate CR45

const byte Deviation = 0x52;    //typical deviation for tones
const byte lora_RXBUFF_Size = 128;
const byte lora_TXBUFF_Size = 128;


#define RemoteControlNode 'G'               //normally used by tracker transmitter in promiscuous_Mode
#define ControlledNode '1'                  //normally used by tracker transmitter in promiscuous_Mode
#define ThisNode ControlledNode
const boolean  Promiscuous_Mode = true;     //if set to false remote control packets from any node accepted
const int inter_Packet_delay = 500;  //allows time for receiver to be ready to see a possible reply
const byte Cmd_WaitSecs = 15;                       //number of seconds to stay in command mode  
const byte default_attempts = 5;                    //default number of times a command will attempt to be sent

//Protected Command Settings
const char key0 = 'L';                               //Used to restrict access to some commands
const char key1 = 'o';
const char key2 = 'R';
const char key3 = 'a';

 
//**************************************************************************************************
// 5) GPS Options
//**************************************************************************************************

#define GPS_in_MB2                          //if using a MikroBus board specify the socket for the GPS
//#define DebugNoGPS                        //test mode, does not use GPS used test location instead
//#define TestLocation                      //uses test locations as defined in Flight_Settings

#define WhenNoGPSFix LeaveOn                //What to do with GPS power when there is no fix at ends of wait period (LeaveOn or LeaveOff)
#define GPSPowerControl Enabled             //Some tracker boards can remove the power form the GPS

const unsigned int GPSBaud = 9600;
const byte WaitGPSFixSeconds = 30;                //in flight mode time to wait for a new GPS fix 
const unsigned long FixisoldmS = 10000;                    //if location has not updated in this number of mS, assume GPS has lost fix
const unsigned int GPSFixes = 100;                        //number of GPS fixes between setting system clock from GPS   
const byte GPS_attempts = 3;                      //number of times the sending of GPS config will be attempted.
const unsigned int GPS_WaitAck_mS = 2000;                   //number of mS to wait for an ACK response from GPS
const unsigned int GPSShutdownTimemS = 1900;              //Software backup mode takes around 1.9secs to power down, used in maHr calculation
const byte GPS_Reply_Size = 12;             //size of GPS reply buffer

const float TestLatitude = 51.48230;                       
const float TestLongitude = -3.18136;
const float TestAltitude = 48;

#define GPS_Library "UBLOX_SerialGPS.h"            //define the GPS program routines to use here
//#define GPS_Library "UBLOX_I2CGPS.h"           //define the GPS program routines to use here
//#define GPS_Library "No_GPS.h"           //define the GPS program routines to use here


//**************************************************************************************************
// 6) Which Memory to use for storage
//**************************************************************************************************

//#include "I2CFRAM_MB85RC16PNF.h"
#define Memory_Library "I2CFRAM_MB85RC16PNF.h" 

//**************************************************************************************************
// 7) Display Settings 
//**************************************************************************************************

const byte contrast5110 = 55;

#define Display_Library "Display_5110.h"                    //define the display Library file to use here
//#define Display_Library "Display_ST7735.h"                    //define the display Library file to use here
//#define Display_Library "Display_SD1306_AVR.h"                    //define the display Library file to use here

//#include "Display_5110.h"                                 //define file to load for screen
//#include Display_Library

//**************************************************************************************************
// 8) FSK RTTY Settings
//**************************************************************************************************

const unsigned int FSKRTTYbaudDelay = 9900;         //delay for baud rate for FSK RTTY, 19930 for 50baud, 9900 for 100baud, 4930 for 200baud (100baud was 9800)  
const byte FSKRTTYRegshift = 6;                     //value to write to frequency offset register for RTTY mark shift
//const byte const_FSKRTTYnoshift = 99;             //value to write to frequency offset register for RTTY space shift
const byte FSKRTTYpips = 5;                         //number of FSK lead in pips
const int  FSKRTTYleadin = 500;                     //number of ms for FSK constant lead in tone
//const byte Cmd_WaitSecs = 15;                     //number of seconds to stay in command mode    



//**************************************************************************************************
// 9) AFSK RTTY Options
//**************************************************************************************************

#define USE_AFSK_RTTY_Upload
const int AFSKrttybaud = 1465;               //delay in uS x 2 for 1 bit and 300baud. Decode range in FLDIGI 1420 to 1510  
const int afskleadinmS = 500;                //number of ms for AFSK constant lead in tone
const int tonehighHz = 1000;                 //high tone in Hertz 
const int tonelowHz = 650;                   //low tone in Hertz   



//**************************************************************************************************
// 10) Bluetooth Options
//**************************************************************************************************

#define Use_NMEA_Bluetooth_Uplink
#define BluetoothBaud 9600 
const byte Bluetooth_Buff_Size = 128;         //size of buffer for NMEA output


//****************************************************************************************************
// 11) Program Default Option settings
//    This section determins which options are on or off by default, this is the default_config byte
//    Take care here.......... 
//**************************************************************************************************

#define OptionOff 0
#define OptionOn 1

const char option_SearchEnable = OptionOn;
const char option_TXEnable = OptionOn;
const char option_FSKRTTYEnable = OptionOn;
const char option_CheckFence = OptionOn;
const char option_ShortPayloadEnable = OptionOff;
const char option_RepeatEnable = OptionOff;
const char option_AddressStrip = OptionOff;
const char option_GPSPowerSave = OptionOn;      

#define option_SearchEnable_SUM (option_SearchEnable*1)
#define option_FSKRTTYEnable_SUM (option_FSKRTTYEnable*4)
#define option_CheckFence_SUM (option_CheckFence*8)
#define option_ShortPayloadEnable_SUM (option_ShortPayloadEnable*16)
#define option_RepeatEnable_SUM (option_RepeatEnable*32)
#define option_AddressStrip_SUM (option_AddressStrip*64)
#define option_GPSPowerSave_SUM (option_GPSPowerSave*128)

const byte Default_config1 = (option_SearchEnable_SUM+ option_FSKRTTYEnable_SUM+option_CheckFence_SUM+option_ShortPayloadEnable_SUM+option_RepeatEnable_SUM+option_AddressStrip_SUM+option_GPSPowerSave_SUM);
const byte Default_config2 = 0;                //currently unused   
const byte Default_config3 = 0;                //currently unused 
const byte Default_config4 = 0;                //currently unused 
const byte const_Default_config = 196;         //Phew, the default config can always be set manually .............


//**************************************************************************************************
// 12) Miscellaneous program settings
//**************************************************************************************************

const unsigned int switch_delay = 1000;             //delay in mS after a switch press before another can take place


//**************************************************************************************************
// 13) Unique calibration settings for each Board
//    These settings are not part of a bind operation
//**************************************************************************************************

const int kelvin_offset = 312;              //if processor self read of temperature reports high, increase this number
const float temp_conversion_slope = 1.0;    //defines the rate of change between low and high temperatures
const long  adc_constant = 1109860;         //if processor self read of its supply voltage reports high reduce this number 



//**************************************************************************************************
// 14) HAB2 settings
//**************************************************************************************************

char Flight_ID[15] = "MYHAB";
const float west_fence = -32;
const float east_fence = 45;
#define SleepSecs 10                      //sleep time in seconds after each TX loop 
#define outside_fence_Sleep_seconds 600   //if outside the fence settings, how long to sleep before next GPS check
const byte Output_len_max = 125;          //maximum length for built HAB payload
#define PayloadArraySize 20               //Maximum number of fields when parsing long HAB payload 



//**************************************************************************************************
// 15) Locator2 Settings
//**************************************************************************************************

//#define RCPulseMode                        //select if RC servo pulse reading to be used for lost detection 
#define Timeout
#define MinsToLost 40                        //minutes before lost mode automatically engaged.
#define TXDelaySecs 15                       //delay in seconds between position transmissions, needed to ensure duty cycle limits kept, normally 10%
#define TXLostDelaySecs 15                   //delay in seconds between lost mode transmissions
#define Output_len_max 126                   //max length of outgoing packet

//Program constants
const int pulseerrorlimit = 100;             //number of RC error pulses needed to trigger lost condition, 255 max
const int holddifference = 30;               //if differance between two RC pulses is less than this, then hold is assumed
const int RCpulseshort = 750;                //in uS, lower than this RC pulse is assumed not to be valid
const int RCpulselong = 2500;                //in uS, higher than this RC pulse is assumed not to be valid
const unsigned long GPSerrorLimit = 129000;  //number of times around loop with no characters before an error is flagged, around 5seconds
const byte inc_on_error = 5;                 //number to increase pulse error count on a fail
const byte dec_on_OK = 10;                   //number to decrease pulse error count on a pass

