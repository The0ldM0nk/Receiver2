//**************************************************************************************************
// Note:
//
// Make changes to this Program file at your peril
//
// Configuration changes should be made in the LoRaTracker_Settings file not here !
//
//**************************************************************************************************

#define programname "LoRaTracker_Receiver2_130817"
#define programversion "V2.1"
#define aurthorname "Stuart Robinson - www.LoRaTracker.uk"

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "Program_Definitions.h"
#include "LoRaTracker_Receiver2_Settings.h"

/*
**************************************************************************************************

  LoRaTracker Programs for Arduino

  Copyright of the author Stuart Robinson - 13/08/2017

  http://www.LoRaTracker.uk

  These programs may be used free of charge for personal, recreational and educational purposes only.

  This program, or parts of it, may not be used for or in connection with any commercial purpose without
  the explicit permission of the author Stuart Robinson.

  The programs are supplied as is, it is up to individual to decide if the programs are suitable for the
  intended purpose and free from errors.

  To do:

  Check use of Read~_Int versus Read_Uint
  At first start, no GPS lock, displays 5737km, lat = 0, lon = 0.
  Starts in search mode
  Line 1386 TRAlt = results[2].toInt();
  check if needed if (lora_RXPacketType == Info)
  check unsigned int ReadSupplyVolts()
  void DisplaySupplyVolts()
  LOG to SD fails, and receiver stops, if HAB packet incorrect

  ******************************************************************************************************
*/


//Program Variables
int ramc_CalibrationOffset;                   //adjustment for frequency in Hertz

unsigned long ramc_TrackerMode_Frequency;          //Initial default for FlightFrequency
byte ramc_TrackerMode_Bandwidth;              //used to keep Tracker mode settings
byte ramc_TrackerMode_SpreadingFactor;
byte ramc_TrackerMode_CodeRate;
byte ramc_TrackerMode_Power;

unsigned long ramc_SearchMode_Frequency;      //Initial default for FlightFrequency
byte ramc_SearchMode_Bandwidth;               //used to keep Searc mode settings
byte ramc_SearchMode_SpreadingFactor;
byte ramc_SearchMode_CodeRate;
byte ramc_SearchMode_Power;

unsigned long ramc_CommandMode_Frequency;     //Initial default for command and reporting frequency
byte ramc_CommandMode_Bandwidth;              //used to keep Command mode settings
byte ramc_CommandMode_SpreadingFactor;
byte ramc_CommandMode_CodeRate;
byte ramc_CommandMode_Power;

byte ramc_Receiver_Mode = Portable_Mode;      //start in portable mode
byte ramc_Current_config1;                    //sets the config of whats transmitted etc
byte ramc_Current_config2;                    //sets the config of whats transmitted etc
byte ramc_Current_config3;                    //sets the config of whats transmitted etc
byte ramc_Current_config4;                    //sets the config of whats transmitted etc

float TRLat;                                  //tracker transmitter co-ordinates
float TRLon;
unsigned int TRAlt;
byte TRHour;                                  //to record time of last tracker data
byte TRMin;
byte TRSec;
byte TRDay;
byte TRMonth;
byte TRYear;
unsigned int TResets;                         //keeps the count of remote tracker resets
byte TRStatus;                                //Status byte from tracker binary payload
byte TRSats;                                  //last received number of GPS satellites from tracker
unsigned int TRdirection;                    //Tracker direction in degrees
float TRdistance;
boolean Remote_GPS_Fix = false;               //set if we have received a location from the remote tracker
boolean GLONASS_Active;

float LocalLat;                               //local GPS co-ordinates
float LocalLon;
unsigned int LocalAlt;
byte LocalHour;                               //to record time of last local co-ordinates
byte LocalMin;
byte LocalSec;
byte LocalDay;
byte LocalMonth;
byte LocalYear;
boolean Local_GPS_Fix = false;                //set if there is a Local GPS fix at last check


byte Switchpress = 0;                         //set when a switch is pressed, 0 if not otherwise 1-5
byte Function_Number = 1;                     //identifiews the current function

char keypress;                                //records the keypress from serial terminal
byte modenumber;                              //records which set of LoRa settings in use, 1 = tracker, 2 = search, 3 = command, 4=bind

boolean int_guard = false;                    //used to prevent multiple interrupts
boolean SD_Found = false;                     //set if SD card found at program startup

unsigned long TrackerMode_Packets;            //running count of trackermode packets received
unsigned int SearchMode_Packets;              //running count of searchmode packets received
unsigned int TestMode_Packets;                //running count of searchmode packets received
unsigned long NoFix_Packets;                  //running count of No GPS fix packets received
unsigned long last_LocalGPSfixmS;             //records millis() at last local GPS
long fixes_till_set_time;                     //used to keep a count of how often to set system time from GPS time

unsigned int SupplyVolts;

String results[5];                            //used during decode of LMLCSVPacket


//File Includes

#include Board_Definition                     //include previously defined board file
#include Display_Library                      //include previously defined Display Library
#include Memory_Library                       //include previously defined Memory Library
#include GPS_Library                          //include previously defined GPS Library

#include <TinyGPS++.h>                        //http://arduiniana.org/libraries/tinygpsplus/
TinyGPSPlus gps;                              //Create the TinyGPS++ object
TinyGPSCustom GNGGAFIXQ(gps, "GNGGA", 5);     //custom sentences used to detect possible switch to GLONASS mode

#include <SPI.h>
#include <Wire.h>
#include <Flash.h>                             //http://arduiniana.org/libraries/flash/ 
#include <TimeLib.h>                           //https://github.com/PaulStoffregen/Time
#include <EEPROM.h>
#include "LoRa3.h"

#ifdef UseSD
#include <SdFat.h>                             //https://github.com/greiman/SdFat
SdFat SD;
File logFile;
#endif

#ifdef USE_AFSK_RTTY_Upload
#include "AFSK_RTTY.h"
#endif

#ifdef Use_NMEA_Bluetooth_Uplink
#include "Generate_NMEA.h"
#endif

#include "Binary2.h"
#include "PinChangeInterrupt.h"
#include "Receiver2_Screens.h"

#define max_functions 5                             //number of functions in main program loop


//**************************************************************************************************
// Eventally - the program itself starts ............
//**************************************************************************************************


void loop()
{
  run_function();

  if (ramc_Receiver_Mode == Terminal_Mode)
  {
    doMenu();
  }
}


void run_function()
{
  //following a key press to change function this routine runs the appropriate function
  Serial.println();
  Serial.print("Function ");
  Serial.println(Function_Number);


  delay(switch_delay);    //debounce switch
  switch (Function_Number)
  {
    case 1:
      current_screen_number = 1;
      listen_LoRa(TrackerMode, 0);
      break;

    case 2:
      current_screen_number = 2;
      listen_LoRa(SearchMode, 0);
      break;

    case 3:
      current_screen_number = 3;
      Serial.println(F("Tracker Data Display"));
      listen_LoRa(TrackerMode, 0);
      break;

    case 4:
      current_screen_number = 4;
      Serial.println(F("Local Data Display"));
      listen_LoRa(TrackerMode, 0);
      break;

    case 5:
      current_screen_number = 5;
      listen_LoRa(BindMode, 0);
      break;

    default:
      break;
  }
}


byte doMenu()
{
  //prints the terminal menu, caqn be ignored in handheld display mode (no serial characters to input
menustart:

  byte i;
  unsigned int returnedCRC;
  Switchpress = 0;
  digitalWrite(LED1, LOW);                   //make sure LED is off
  Setup_LoRaCommandMode();

  Serial.println();
  Serial.println();
  print_last_HABpacket();
  print_TRData();
  print_LocalData();
  Serial.println();
  print_CurrentLoRaSettings();
  print_Nodes();
  Serial.println();
  Serial.println(F("1) Enable FSK RTTY"));
  Serial.println(F("2) Disable FSK RTTY"));
  Serial.println(F("3) Enable Address Strip"));
  Serial.println(F("4) Disable Address Strip"));
  Serial.println(F("5) Enable GPS Power Off"));
  Serial.println(F("6) Disable GPS Power Off"));
  Serial.println(F("7) Enable Fence Check"));
  Serial.println(F("8) Disable Fence Check"));
  Serial.println(F("9) Enable Doze Mode"));
  Serial.println(F("0) Disable Doze Mode"));
  Serial.println();
  Serial.println(F("B) Bind Receive"));
  Serial.println(F("C) Command Mode Listen"));
  Serial.println(F("D) Default Configuration"));
  Serial.println(F("L) Link Report"));
  //Serial.println(F("M) Memory Configure"));
  Serial.println(F("O) Offset for Tracker Frequency"));
  Serial.println(F("P) Packet Test"));
  Serial.println(F("R) RTTY Test"));
  Serial.println(F("S) Search Mode Listen"));
  Serial.println(F("T) Tracker Mode Listen"));

  Serial.println(F("X) Reset Tracker Transmitter"));
  Serial.println(F("Y) Clear All Memory and Settings (re-config needed)"));
  Serial.println();
  Serial.println(F("+) Increase Transmitter Frequency 1KHZ"));
  Serial.println(F("-) Reduce Transmitter Frequency 1KHZ"));
  Serial.println();
  Serial.print(F("> "));
  delay(switch_delay);
  int_guard = true;                           //allow switch press interrupts again


  while (Serial.read() != -1);                //clear serial buffer

  while ((Serial.available() == 0) && (Switchpress == 0))
  {
    //we are in menu mode so wait for a keypress or a switch press
  }

  //if there is a switch press then we should switch to portable mode and return from menu mode.

  if (Switchpress != 0)
  {
    Serial.println(F("Switch Press - Set Portable Mode"));
    Switchpress = 0;                          //clear the switch press
    ramc_Receiver_Mode = Portable_Mode;       //key was pressed so record the switch to portable mode
    return Portable_Mode;
  }

  //if we are here then there could have been a keypress and switch press at the same time

  if (Serial.available() != 0)                //double check a key has been pressed
  {
    ramc_Receiver_Mode = Terminal_Mode;       //keyboard key was pressed so ensure we are in terminal mode
  }

  keypress = Serial.read();                   //get the keypress

  Serial.println(keypress);
  Serial.println();

  if (keypress == '1')
  {
    Serial.println(F("Queue Enable FSKRTTY"));
    send_ConfigCommand(Config0, ThisNode, FSKRTTYEnable, 1);
  }

  if (keypress == '2')
  {
    Serial.println(F("Queue Disable FSKRTTY"));
    send_ConfigCommand(Config0, ThisNode, FSKRTTYEnable, 0);
  }

  if (keypress == '3')
  {
    Serial.println(F("Queue Enable Address Strip"));
    send_ConfigCommand(Config0, ThisNode, AddressStrip, 1);
  }

  if (keypress == '4')
  {
    Serial.println(F("Queue Disable Address Strip"));
    send_ConfigCommand(Config0, ThisNode, AddressStrip, 0);
  }

  if (keypress == '5')
  {
    //this is lat,long and alt at low data rate
    Serial.println(F("Queue Enable GPS Power Off"));
    send_ConfigCommand(Config0, ThisNode, GPSPowerSave, 1);
  }

  if (keypress == '6')
  {
    //this is lat,long and alt at low data rate
    Serial.println(F("Queue Disable GPS Power Off"));
    send_ConfigCommand(Config0, ThisNode, GPSPowerSave, 0);
  }

  if (keypress == '7')
  {
    Serial.println(F("Queue Enable Fence Check"));
    send_ConfigCommand(Config0, ThisNode, CheckFence, 1);
  }


  if (keypress == '8')
  {
    Serial.println(F("Queue Disable Fence Check"));
    send_ConfigCommand(Config0, ThisNode, CheckFence, 0);
  }

  if (keypress == '9')
  {
    Serial.println(F("Queue Enable Doze"));
    send_ConfigCommand(Config0, ThisNode, DozeEnable, 1);
  }

  if (keypress == '0')
  {
    Serial.println(F("Queue Disable Doze"));
    send_ConfigCommand(Config0, ThisNode, DozeEnable, 0);
  }


  if ((keypress == 'B') || (keypress == 'b'))
  {
    Function_Number = 5;                                       //bind only accepted in function 6
    listen_LoRa(BindMode, 0);
  }

  if ((keypress == 'C') || (keypress == 'c'))
  {
    listen_LoRa(CommandMode, 0);
  }



  if ((keypress == 'L') || (keypress == 'l'))
  {
    lora_TXBUFF[0] = '0';
    Serial.println(F("Queue Link Report"));
    lora_QueuedSend(0, 0, LinkReport, Broadcast, ThisNode, 10, lora_Power, default_attempts, NoStrip);  //send a Link Report Request
    delay(inter_Packet_delay);
    listen_LoRa(CommandMode, 0);                              //listen for the link budget reply
  }

  if ((keypress == 'O') || (keypress == 'o'))
  {
    enter_CalibrationOffset();
  }

  if ((keypress == 'P') || (keypress == 'p'))
  {
    Serial.println(F("Queue LoRa Packet Test"));
    send_TestRequest('0');
    listen_LoRa(TrackerMode, 0);                              //command has been sent so listen for replies
  }

  if ((keypress == 'R') || (keypress == 'r'))
  {
    Serial.println(F("Queue FSK RTTY Test Request"));
    send_TestRequest('1');
  }

  if ((keypress == 'S') || (keypress == 's'))
  {
    current_screen_number = 2;
    writescreen_2();
    listen_LoRa(SearchMode, 0);
  }

  if ((keypress == 'T') || (keypress == 't'))
  {
    current_screen_number = 1;
    writescreen_1();
    listen_LoRa(TrackerMode, 0);
  }


  if (keypress == 'D')
  {
    Serial.println(F("Configure from Defaults"));
    read_Settings_Defaults();
    write_Settings_Memory();
    Serial.println();
    Print_All_Memory();
    Serial.println();
    Print_CRC_All_Memory();
    Print_CRC_Config_Memory();
    //returnedCRC = Memory_CRC(addr_StartConfigData, addr_EndConfigData);
    //Serial.print(F("Memory CRC  "));
    //Serial.println(returnedCRC, HEX);
    Serial.println();
    delay(1500);
  }

  if (keypress == 'Y')
  {
    Clear_All_Memory();
    Serial.println();
    Print_CRC_All_Memory();
    Serial.println();
    read_Settings_Memory();
  }




  if (keypress == 'X')
  {
    load_key();
    Serial.println(F("Queue Reset Tracker Request"));
    lora_QueuedSend(0, 3, ResetTracker, Broadcast, ThisNode, 10, lora_Power, default_attempts, NoStrip);  //send a reset request
  }

  if (keypress == '+')
  {
    Serial.println(F("Increase Tracker Frequency 1KHZ"));
    i = lora_QueuedSend(0, 1, INCFreq, Broadcast, ThisNode, 10, lora_Power, default_attempts, NoStrip);
  }

  if (keypress == '-')
  {
    Serial.println(F("Decrease Tracker Frequency 1KHZ"));
    i = lora_QueuedSend(0, 1, DECFreq, Broadcast, ThisNode, 10, lora_Power, default_attempts, NoStrip);
  }

  goto menustart;                                  //all programs deserve at least one goto, if only because it works
}


void enter_CalibrationOffset()
{
  //allows calibration offset of recever to be changed from terminal keyboard
  float tempfloat;
  char tempchar;
  Serial.println(F("Enter Khz offset > "));
  while (Serial.available() == 0);
  {
    tempfloat = Serial.parseFloat();
    Serial.print("Offset = ");
    Serial.println(tempfloat, 5);
  }
  while (Serial.available() > 0)
  {
    tempchar == Serial.read();                      //clear keyboard buffer
  }
}


void Send_Bind()
{
  //transmit the bind packet
  byte index, bufferdata;
  byte packetlength = 3;  //the bind packet will contain from addr_StartConfigData to addr_EndConfigData
  unsigned int tempUint;
  load_key();                                       //loads key in bytes 0,1,2,3
  int_guard = true;
  writescreen_5();
  Serial.println(F("Queue Send Bind Packet"));

  Setup_LoRaBindMode();

  for (index = addr_StartBindData; index <= addr_EndBindData; index++)
  {
    packetlength++;
    bufferdata =  Memory_ReadByte(index);
    lora_TXBUFF[packetlength] = bufferdata;
  }

  if (lora_QueuedSend(0, packetlength, Bind, Broadcast, ThisNode, 10, lora_Power, default_attempts, NoStrip));
  //will return as true if queued send received
  {
    Print_CRC_Bind_Memory();
    writescreen_Alert3();                                    //write out bind acknowledged screen
  }

  if (keypress > 0)
  {
    ramc_Receiver_Mode = Terminal_Mode;                     //if key was pressed in queued send then make sure the menu is run
  }

}


void load_key()
{
  //loads the protection key to first locations in TX buffer
  lora_TXBUFF[0] = key0;
  lora_TXBUFF[1] = key1;
  lora_TXBUFF[2] = key2;
  lora_TXBUFF[3] = key3;
}


boolean Is_Key_Valid()
{
  //checks if protection key in RX bauffer matches

  Serial.print(F("Received Key "));
  Serial.write(lora_RXBUFF[0]);
  Serial.write(lora_RXBUFF[1]);
  Serial.write(lora_RXBUFF[2]);
  Serial.write(lora_RXBUFF[3]);
  Serial.println();

  if ( (lora_RXBUFF[0] == key0) && (lora_RXBUFF[1] == key1)  && (lora_RXBUFF[2] == key2)  && (lora_RXBUFF[3] == key3) )
  {
    Serial.println(F("Key Valid"));
    return true;
  }
  else
  {
    Serial.println(F("Key Not Valid"));
    return false;
  }
}


byte listen_LoRa(byte lmode, unsigned long listen_mS)
{
  //listen for packets in specific mode, timeout in mS, 0 = continuous wait
  unsigned long listen_endmS;
  byte GPSByte;


  switch (lmode)
  {
    case TrackerMode:
      //Serial.print(F("Tracker Mode - "));
      Setup_LoRaTrackerMode();
      break;

    case SearchMode:
      //Serial.print(F("Search Mode - "));
      Setup_LoRaSearchMode();
      break;

    case CommandMode:
      //Serial.print(F("Command Mode - "));
      Setup_LoRaCommandMode();
      break;

    case BindMode:
      //Serial.print(F("Bind Mode - "));
      Setup_LoRaBindMode();
      break;
  }

  print_CurrentLoRaSettings();
  listen_endmS = (millis() + listen_mS);                    //calculate the millis to stop listing at.

  keypress = 0;
  Switchpress = 0;

  while (Serial.read() != -1);                              //empty serial buffer

  update_screen(current_screen_number);
  Serial.println();
  print_mode(modenumber);
  Serial.println(F("Listen >"));
  int_guard = true;                                         //allow switch press ints again
  lora_RXpacketCount = 0;
  Switchpress = 0;
  keypress = 0;
  lora_RXONLoRa();

  do
  {
    check_for_Packet();

    do
    {
      GPSByte = GPS_GetByte();
      if (GPSByte != 0xFF)
      {
        gps.encode(GPSByte);
      }
    }
    while (GPSByte != 0xFF);

    check_GPSforFix();

    //now check for a timeout if set
    if (listen_mS > 0)
    {
      if (millis() >= listen_endmS)
      {
        Serial.println(F("Listen Timeout"));
        break;
      }
    }
  }
  while ((Serial.available() == 0) && (Switchpress == 0));

  if (Serial.available() != 0)
  {
    keypress = Serial.read();
  }

  if (Switchpress > 0)
  {
    Serial.println(F("Switch Press in Listen"));
    ramc_Receiver_Mode = Portable_Mode;                     //key was pressed so ensure we are in terminal mode
  }

  if (keypress > 0)
  {
    Serial.println(F("Key Press"));
    ramc_Receiver_Mode = Terminal_Mode; //key was pressed so ensure we are in terminal mode
  }

  if ( (millis() > listen_endmS)  && (listen_mS != 0) )
  {
    Serial.println(F("Timeout"));
    keypress = 1;
  }

  return keypress;
}


void print_mode(byte lmode)
{
  switch (lmode)
  {
    case TrackerMode:
      Serial.print(F("Tracker Mode "));
      break;

    case SearchMode:
      Serial.print(F("Search Mode "));
      break;

    case CommandMode:
      Serial.print(F("Command Mode "));
      break;

    case BindMode:
      Serial.print(F("Bind Mode "));
      break;
  }
}


void send_TestRequest(char cmd)
{
  //transmit a test request
  Serial.print(F("Send Test Request "));
  Serial.write(cmd);
  Serial.println();

  lora_TXBUFF[0] = cmd;
  lora_QueuedSend(0, 0, Test, Broadcast, ThisNode, 10, lora_Power, default_attempts, NoStrip);   //send a Test Request
  delay(inter_Packet_delay);
}


void send_ConfigCommand(char confignum, char configdestination, char bitnum, char bitval)
{
  //transmits a config command
  byte returnbyte;
  Setup_LoRaCommandMode();
  bitnum = bitnum + 48;
  bitval = bitval + 48;
  lora_TXBUFF[0] = bitnum;            //set the bitnum
  lora_TXBUFF[1] = bitval;            //set the bitval

  returnbyte = lora_QueuedSend(0, 1, Config0, configdestination, ThisNode, 10, lora_Power, default_attempts, NoStrip);

  if (returnbyte == 1)
  {
    Serial.println(F("Config Sent OK"));
    print_TrackerLastSNR();
    print_TrackerLastRSSI();
  }
  else
  {
    Serial.println(F("Config Send Failed"));
  }
  Serial.println();
  delay(inter_Packet_delay);
}



void print_TrackerLastSNR()
{
  int8_t temp;
  temp =   Read_Byte(0, lora_RXBUFF);
  Serial.print(F("Last Tracker Received SNR "));
  Serial.print(temp);
  Serial.println(F("dB"));
}


void print_TrackerLastRSSI()
{
  int8_t temp;
  temp = Read_Byte(1, lora_RXBUFF);
  Serial.print(F("Last Tracker Received RSSI "));
  Serial.print(temp);
  Serial.println(F("dB"));
}


byte check_for_Packet()
{
  //checks to see if a packet has arrived
  byte returnbyte;

  returnbyte = lora_readRXready2();

  if (returnbyte == 64)
  {
    digitalWrite(LED1, HIGH);
    lora_ReadPacket();
    lora_AddressInfo();
    lora_ReceptionInfo();
    Serial.println();
    process_Packet();
    digitalWrite(LED1, LOW);
    Serial.println();
    print_mode(modenumber);
    Serial.println(F("Listen >"));
    lora_RXONLoRa();                                //ready for next and clear flags
    return 1;
  }

  if (returnbyte == 96)
  {
    Serial.println(F("Packet CRC Error"));
    Serial.println();
    print_mode(modenumber);
    Serial.println(F("Listen >"));
    lora_RXONLoRa();                                //ready for next
  }

  return 0;
}



void write_HABPacketMemory(byte RXStart, byte RXEnd)
{
  //stores the HAB packet in memory, FRAM or EEPROM
  byte index, bufferdata;
  unsigned int MemAddr = addr_StartHABPayloadData;                 //address in FRAM where last received HAB payload stored

  Memory_WriteByte(MemAddr, lora_RXPacketType);
  MemAddr++;
  Memory_WriteByte(MemAddr, lora_RXDestination);
  MemAddr++;
  Memory_WriteByte(MemAddr, lora_RXSource);
  MemAddr++;
  for (index = RXStart; index <= RXEnd; index++)
  {
    bufferdata = lora_RXBUFF[index];
    Memory_WriteByte(MemAddr, bufferdata);
    MemAddr++;
  }
  Memory_WriteByte(MemAddr, 0xFF);

}


void print_FightID()
{
  //send flight ID to serila terminal
  byte index, bufferdata;
  index = 0;
  do
  {
    bufferdata = Flight_ID[index++];
    Serial.write(bufferdata);
  }
  while (bufferdata != 0);
}



void extract_HABPacket(byte passedRXStart, byte passedRXEnd)
{
  //extracts data from received HAB packets where first fields are lat,lon,alt.
  byte tempbyte;
  byte savedRXStart;

  savedRXStart = lora_RXStart;                 //save current value of lora_RXStart
  lora_RXStart = passedRXStart;                //use lora_RXStart as its global

  //Skip leading $
  do
  {
    tempbyte =  lora_RXBUFF[lora_RXStart++];
  }
  while ( tempbyte == '$');
  lora_RXStart--;

  //ID
  tempbyte = sizeof(Flight_ID);

  extract_Buffer(Flight_ID, sizeof(Flight_ID));

  print_FightID();
  Serial.print(F(" "));

  lora_RXStart = next_Comma(lora_RXStart);
  lora_RXStart = next_Comma(lora_RXStart);

  //Lat
  TRLat = extract_Float();

  //Lon
  TRLon = extract_Float();

  //Alt
  TRAlt = extract_Uint();

  print_Tracker_Location();
  Serial.println();

  TRSats = extract_Uint();
  lora_RXStart = next_Comma(lora_RXStart);
  lora_RXStart = next_Comma(lora_RXStart);

  //Reset
  TResets = extract_Uint();
  lora_RXStart = next_Comma(lora_RXStart);
  TRStatus = extract_Uint();
  lora_RXStart = savedRXStart;                    //restore lora_RXStart, just in case
}


float extract_Float()
{
  //extracts a float in ASCII format from buffer
  char temp[12];
  byte tempptr = 0;
  byte bufferdata;
  float tempfloat;
  do
  {
    bufferdata =  lora_RXBUFF[lora_RXStart++];
    temp[tempptr++] = bufferdata;
  }
  while (bufferdata != ',');
  temp[tempptr] = 0;  //terminator for string
  tempfloat = (float)atof(temp);
  return tempfloat;
}


void extract_Buffer(char * mybuffer, size_t bufSize)
{
  //extracts a buffer in ASCII format from RXbuffer
  byte index, buffersize;

  buffersize = bufSize - 1;
  for (index = 0; index <= buffersize; index++)
  {
    mybuffer[index] = lora_RXBUFF[lora_RXStart++];
    if (mybuffer[index] == ',')
    {
      break;
    }
  }
  mybuffer[index] = 0;
}


float extract_Uint()
{
  //extracts an unsigned int in ASCII format from buffer
  char temp[12];
  byte tempptr = 0;
  byte buffdata;
  unsigned int tempint;
  do
  {
    buffdata =  lora_RXBUFF[lora_RXStart++];
    temp[tempptr++] = buffdata;
  }
  while (buffdata != ',');
  temp[tempptr] = 0;  //terminator for string
  tempint = (unsigned int)atof(temp);
  return tempint;
}


byte next_Comma(byte localpointer)
{
  //skips through HAB packet (in CSV format) to next  comma
  byte bufferdata;
  do
  {
    bufferdata =  lora_RXBUFF[localpointer++];
  }
  while (bufferdata != ',');
  return localpointer;
}


/**********************************************************************
  Tracker data routines
***********************************************************************
*/

void extract_TRbinarylocationData()
{
  //extracts the binary location data from receive buffer and records date and time

  TRLat = Read_Float(0, lora_RXBUFF);
  TRLon = Read_Float(4, lora_RXBUFF);
  TRAlt = Read_UInt(8, lora_RXBUFF);
}


void record_TRtimedate()
{
  time_t t = now(); // Store the current time in time
  TRHour = hour(t);
  TRMin = minute(t);
  TRSec = second(t);
  TRDay = day(t);
  TRMonth = month(t);
  TRYear = (byte) (year(t) - 2000);
}


void print_TRData()
{
  //prints the last received tracker location data
  Serial.print("Last Tracker Fix ");
  Serialprint_addleadingZero(TRHour);
  Serial.print(":");
  Serialprint_addleadingZero(TRMin);
  Serial.print(":");
  Serialprint_addleadingZero(TRSec);
  Serial.print("  ");
  Serialprint_addleadingZero(TRDay);
  Serial.print("/");
  Serialprint_addleadingZero(TRMonth);
  Serial.print("/");
  Serialprint_addleadingZero(TRYear);

  Serial.print("  ");
  Serial.print(TRLat, 5);
  Serial.print("  ");
  Serial.print(TRLon, 5);
  Serial.print("  ");
  Serial.println(TRAlt);
}


void save_TRData()
{
  //writes the last received tracker location data to memory
  Memory_WriteFloat(addr_TRLat, TRLat);
  Memory_WriteFloat(addr_TRLon, TRLon);
  Memory_WriteUInt(addr_TRAlt, TRAlt);
  Memory_WriteByte(addr_TRYear, TRYear);
  Memory_WriteByte(addr_TRMonth, TRMonth);
  Memory_WriteByte(addr_TRDay, TRDay);
  Memory_WriteByte(addr_TRHour, TRHour);
  Memory_WriteByte(addr_TRMin, TRMin);
  Memory_WriteByte(addr_TRSec, TRSec);
}


void read_TRData()
{
  //read stored tracker location data from memory
  TRLat = Memory_ReadFloat(addr_TRLat);
  TRLon = Memory_ReadFloat(addr_TRLon);
  TRAlt = Memory_ReadUInt(addr_TRAlt);
  TRHour = Memory_ReadByte(addr_TRHour);
  TRMin = Memory_ReadByte(addr_TRMin);
  TRSec = Memory_ReadByte(addr_TRSec);
  TRDay = Memory_ReadByte(addr_TRDay);
  TRMonth = Memory_ReadByte(addr_TRMonth);
  TRYear = Memory_ReadByte(addr_TRYear);
}


void clear_TRData()
{
  //sets the tracker location data to zero
  TRLat = 0;
  TRLon = 0;
  TRAlt = 0;
  TRHour = 0;
  TRMin = 0;
  TRSec = 0;
  TRDay = 0;
  TRMonth = 0;
  TRYear = 0;
}


/**********************************************************************
  Local data routines
***********************************************************************
*/

void record_LocalData()
{
  //gets the current local location fix data and time
  time_t t = now(); // Store the current time in time
  LocalLat = gps.location.lat();
  LocalLon = gps.location.lng();
  LocalAlt = gps.altitude.meters();
  LocalHour = hour(t);
  LocalMin = minute(t);
  LocalSec = second(t);
  LocalDay = day(t);
  LocalMonth = month(t);
  LocalYear =  (byte) (year(t) - 2000);
}



void print_LocalData()
{
  //prints the local location fix data and time
  Serial.print("Last Local Fix ");
  Serialprint_addleadingZero(LocalHour);
  Serial.print(":");
  Serialprint_addleadingZero(LocalMin);
  Serial.print(":");
  Serialprint_addleadingZero(LocalSec);
  Serial.print("  ");
  Serialprint_addleadingZero(LocalDay);
  Serial.print("/");
  Serialprint_addleadingZero(LocalMonth);
  Serial.print("/");
  Serialprint_addleadingZero(LocalYear);
  Serial.print("  ");
  Serial.print(LocalLat, 5);
  Serial.print("  ");
  Serial.print(LocalLon, 5);
  Serial.print("  ");
  Serial.println(LocalAlt);

}


void save_LocalData()
{
  //writes local GPS location data to memory
  Memory_WriteFloat(addr_LocalLat, LocalLat);              //save tracker lat in non volatile memory
  Memory_WriteFloat(addr_LocalLon, LocalLon);              //save tracker lon in non volatile memory
  Memory_WriteUInt(addr_LocalAlt, LocalAlt);                //save tracker lon in non volatile memory
  Memory_WriteByte(addr_LocalYear, LocalYear);             //add current date and time from local GPS
  Memory_WriteByte(addr_LocalMonth, LocalMonth);
  Memory_WriteByte(addr_LocalDay, LocalDay);
  Memory_WriteByte(addr_LocalHour, LocalHour);
  Memory_WriteByte(addr_LocalMin, LocalMin);
  Memory_WriteByte(addr_LocalSec, LocalSec);
}


void read_LocalData()
{
  //reads local GPS location data from memory
  LocalLat = Memory_ReadFloat(addr_LocalLat);
  LocalLon = Memory_ReadFloat(addr_LocalLon);
  LocalAlt = Memory_ReadUInt(addr_LocalAlt);
  LocalHour = Memory_ReadByte(addr_LocalHour);
  LocalMin = Memory_ReadByte(addr_LocalMin);
  LocalSec = Memory_ReadByte(addr_LocalSec);
  LocalDay = Memory_ReadByte(addr_LocalDay);
  LocalMonth = Memory_ReadByte(addr_LocalMonth);
  LocalYear = Memory_ReadByte(addr_LocalYear);
}


void clear_LocalData()
{
  //sets the current local location data to zero
  LocalLat = 0;
  LocalLon = 0;
  LocalAlt = 0;
  LocalHour = 0;
  LocalMin = 0;
  LocalSec = 0;
  LocalHour = 0;
  LocalMin = 0;
  LocalSec = 0;
}

void print_Tracker_Location()
{
  //prints the tracker location data only
  Serial.print(TRLat, 6);
  Serial.print(F(","));
  Serial.print(TRLon, 6);
  Serial.print(F(","));
  Serial.print(TRAlt);
}


void print_packet_HEX(byte RXStart, byte RXEnd)
{
  //prints contents of received packet as hexadecimal
  byte index, bufferdata;
  Serial.print(lora_RXPacketType, HEX);
  Serial.print(F(" "));
  Serial.print(lora_RXDestination, HEX);
  Serial.print(F(" "));
  Serial.print(lora_RXSource, HEX);
  Serial.print(F(" "));

  for (byte index = RXStart; index <= RXEnd; index++)
  {
    bufferdata = lora_RXBUFF[index];
    if (bufferdata < 0x10)
    {
      Serial.print(F("0"));
    }
    Serial.print(bufferdata, HEX);
    Serial.print(F(" "));
  }

}


void process_Packet()
{
  //process and decide what to do with received packet
  unsigned int index, tempint;
  byte tempbyte, tempbyte1, ptr;
  int signedint;
  unsigned int returnedCRC;
  int8_t tempchar;

  digitalWrite(LED1, HIGH);
  //Serial.println(F("Process Packet"));

  if (lora_RXPacketType == LocationBinaryPacket)
  {
    Remote_GPS_Fix = true;

    if (modenumber == SearchMode)
    {
      SearchMode_Packets++;
    }

    if (modenumber == TrackerMode)
    {
      TrackerMode_Packets++;
    }

    extract_TRbinarylocationData();
    record_TRtimedate();
    save_TRData();

    print_Tracker_Location();
    TRStatus = Read_Byte(10, lora_RXBUFF);
    Serial.print(F(","));
    print_AllBits(TRStatus);
    Serial.println();

#ifdef UseSD
    SD_WriteBinarypacket_Log();
#endif

    if (Local_GPS_Fix)
    {
      distanceto();
      directionto();
    }

    update_screen(current_screen_number);              //and update the appropriate screen
    digitalWrite(LED1, LOW);

#ifdef Use_NMEA_Bluetooth_Uplink
    send_NMEA(TRLat, TRLon, TRAlt);                    //Send position to Bluetooth, sends two NMEA strings
    Serial.println();
#endif

    display_fix_Status();
    return;
  }


  if (lora_RXPacketType == HABPacket)
  {
    Remote_GPS_Fix = true;
    TrackerMode_Packets++;

    lora_RXBuffPrint(PrintASCII);                      //print packet contents as ASCII
    Serial.println();

#ifdef UseSD
    SD_WriteHABpacket_Log();
#endif

    write_HABPacketMemory(lora_RXStart, lora_RXEnd);
    extract_HABPacket(lora_RXStart, lora_RXEnd);

    record_TRtimedate();
    save_TRData();

    if (Local_GPS_Fix)
    {
      distanceto();
      directionto();
    }

    update_screen(current_screen_number);                //and update the appropriate screen
    digitalWrite(LED1, LOW);

#ifdef Use_NMEA_Bluetooth_Uplink
    send_NMEA(TRLat, TRLon, TRAlt);                      //Send position to Bluetooth
    Serial.println();
#endif

#ifdef USE_AFSK_RTTY_Upload
    Serial.print(F("AFSK RTTY Upload "));
    Serial.println();
    start_AFSK_RTTY();

    for (index = 0; index <= 3; index++)
    {
      SendAFSKRTTY('$');
    }

    for (index = lora_RXStart; index <= lora_RXEnd; index++)
    {
      SendAFSKRTTY(lora_RXBUFF[index]);
    }
    SendAFSKRTTY(13);
    SendAFSKRTTY(10);
    end_AFSK_RTTY();
#endif

    display_fix_Status();
    return;
  }


  if (lora_RXPacketType == Testpacket)
  {
    Serial.print(F("TestPacket "));
    TestMode_Packets++;
    lora_RXBuffPrint(PrintASCII);                        //print packet contents as ASCII
    Serial.println();
    writescreen_7();
    return;
  }


  if (lora_RXPacketType == Bind )
  {
    if ((Function_Number == 5) && Is_Key_Valid())      //only accept incoming bind request when in function 5
    {

      ptr = 4;                                         //bind packet has 4 bytes of key
      Serial.println(F("Tracker Bind Received"));

      for (index = addr_StartBindData; index <= addr_EndBindData; index++)
      {
        tempbyte = lora_RXBUFF[ptr++];
        Memory_WriteByte(index, tempbyte);
      }

      Print_All_Memory();
      read_Settings_Memory();
      Print_All_Memory();

      print_Powers();
      Print_CRC_Bind_Memory();

      tempint = (lora_RXBUFF[lora_RXEnd] * 256) + (lora_RXBUFF[lora_RXEnd - 1]);

      Serial.print(F("Transmitted CRC "));
      Serial.println(tempint, HEX);

      if (returnedCRC == tempint)
      {
        Serial.println();
        Serial.println(F("Bind Accepted"));
        Serial.println();
        writescreen_Alert2();
      }
      else
      {
        Serial.println();
        Serial.println(F("Bind Rejected"));
        Serial.println();
        writescreen_Alert7();
      }
    }
    else
    {
      Serial.println(F("Bind Ignored - Wrong Key or Function"));
    }
    return;

  }


  if (lora_RXPacketType == Wakeup)
  {
    //digitalWrite(LED1, LOW);
    Serial.write(7);                                       //print a bell
    Serial.println();
    return;
  }


  if (lora_RXPacketType == NoFix)
  {
    writescreen_Alert4();
    NoFix_Packets++;
    Serial.print(F("No Tracker GPS Fix "));
    Serial.println(NoFix_Packets);
    return;
  }

  if (lora_RXPacketType == NoGPS)
  {
    writescreen_Alert4();
    Serial.print(F("Tracker GPS Error "));
    return;
  }


  if (lora_RXPacketType == PowerUp)
  {
    //digitalWrite(LED1, LOW);
    tempint = Read_UInt(2, lora_RXBUFF);
    writescreen_Alert5(tempint);
    Serial.print(F("Power Up "));
    Serial.print(tempint);
    Serial.println(F("mV"));
    return;
  }

  if (lora_RXPacketType == Info)
  {
    Serial.println();
    Serial.print(F("Tracker Last Packet Reception  SNR,"));
    tempchar = Read_Byte(0, lora_RXBUFF);
    Serial.print(tempchar);
    Serial.print(F("dB"));
    tempchar = Read_Byte(1, lora_RXBUFF);
    Serial.print(F("  RSSI,"));
    Serial.print(tempchar);
    Serial.println(F("dB"));
    Serial.print(F("Battery,"));
    tempint = Read_Int(2, lora_RXBUFF);
    Serial.print(tempint);
    Serial.print(F("mV,TRStatus,"));
    tempbyte = (byte) Read_Int(4, lora_RXBUFF);
    print_AllBits(tempbyte);
    Serial.println();
    Serial.println();
    return;
  }

  if (lora_RXPacketType == ClearToSendCommand)
  {
    return;                                 //do nothing
  }

  if (lora_RXPacketType == ClearToSend)
  {
    return;                                 //do nothing
  }


  if ((lora_RXPacketType == LMLCSVPacket) || (lora_RXPacketType == LMLCSVPacket_Repeated)) //is a short payload, direct or repeated
  {
    Remote_GPS_Fix = 1;

    if (modenumber == SearchMode)
    {
      SearchMode_Packets++;
    }

    if (modenumber == TrackerMode)
    {
      TrackerMode_Packets++;
    }

    FillPayloadArray(lora_RXStart, lora_RXEnd);
    TRLat = convertstring(results[0]);
    TRLon = convertstring(results[1]);
    TRAlt = results[2].toInt();

    record_TRtimedate();
    save_TRData();

    print_Tracker_Location();
    Serial.println();

    if (Local_GPS_Fix)
    {
      distanceto();
      directionto();
    }

    update_screen(current_screen_number);              //and update the appropriate screen
  }


  if (lora_RXPacketType == Sensor1)
  {
    Serial.println(F("Sensor1 Packet"));
    process_Sensor1();
  }


}


void process_Sensor1()
{
  Serial.println(F("Process Sensor Data"));
  Serial.flush();
  float lTemperature, lHumidity, lPressure, lAltitude;

  lTemperature = Read_Float(0, lora_RXBUFF);
  lHumidity = Read_Float(4, lora_RXBUFF);
  lPressure = Read_Float(8, lora_RXBUFF);
  lAltitude = Read_Float(12, lora_RXBUFF);

  Serial.print("Temperature: ");
  Serial.print(lTemperature, 2);
  Serial.println(" degrees C");

  Serial.print("%RH: ");
  Serial.print(lHumidity, 2);
  Serial.println(" %");

  Serial.print("Pressure: ");
  Serial.print(lPressure, 2);
  Serial.println(" Pa");

  Serial.print("Altitude: ");
  Serial.print(lAltitude, 2);
  Serial.println("m");
}




void FillPayloadArray(byte llora_RXStart, byte llora_RXEnd)
{
  //fill the payload array from the CSV data in the packet
  byte i = 0;
  byte j = 0;
  byte lvar1;
  String tempstring = "";

  for (i = llora_RXStart; i <= llora_RXEnd; i++)
  {
    lvar1 = lora_RXBUFF[i];

    if (lvar1 == ',')
    {
      results[j] = tempstring;
      j++;
      tempstring = "";
      if (j > PayloadArraySize)
      {
        Serial.print(F("ERROR To many Fields"));
        Serial.println(j);
        break;
      }
    }
    else
    {
      tempstring = tempstring + char(lvar1);
    }
  }

}


float convertstring(String inString)
{
  //convert the string in the payload buffer to a float
  char buf[20];
  inString.toCharArray(buf, inString.length());
  float val = atof(buf);
  return val;
}



void print_AllBits(byte myByte)
{
  //prints bits of byte as binary
  for (byte mask = 0x80; mask; mask >>= 1) {
    if (mask  & myByte)
      Serial.print('1');
    else
      Serial.print('0');
  }
}



void display_fix_Status()
{
  //display fix status of local GPS and tracker
  long tempfixes;

  if (Local_GPS_Fix)
  {
    Serial.println(F("Local GPS Fix"));
  }
  else
  {
    Serial.println(F("No Local GPS Fix"));
  }

  tempfixes = GPSFixes - fixes_till_set_time;
  Serial.print(F("GPS Fixes Since Time Set "));
  Serial.println(tempfixes);

  if (bitRead(TRStatus, GPSFix))
  {
    Serial.println(F("Tracker GPS fix"));
  }
  else
  {
    Serial.println(F("No Tracker GPS fix"));
  }
}


void print_CurrentLoRaSettings()
{
  //prints the current LoRa settings, reads device registers
  float tempfloat;
  int tempint;
  byte regdata;
  unsigned int bw;

  tempfloat = lora_GetFreq();
  Serial.print(tempfloat, 3);
  Serial.print(F("MHz  ("));

  tempint = ramc_CalibrationOffset;
  tempfloat = (float)(tempint / 1000);
  Serial.print(tempfloat, 1);

  Serial.print(F("Khz)"));

  regdata = lora_Read(lora_RegModemConfig1);
  regdata = regdata & 0xF0;
  bw = lora_returnbandwidth(regdata);
  Serial.print(F("  BW"));
  Serial.print(bw);

  regdata = lora_Read(lora_RegModemConfig2);
  regdata = (regdata & 0xF0) / 16;
  Serial.print(F("  SF"));
  Serial.print(regdata);

  regdata = lora_Read(lora_RegModemConfig1);
  regdata = regdata & B00001110;
  regdata = regdata / 2; //move right one
  regdata = regdata + 4;

  Serial.print(F("  CR4/"));
  Serial.print(regdata);

  regdata = lora_Read(lora_RegModemConfig3);
  regdata = regdata & B00001000;
  Serial.print(F("  LDROPT_"));
  if (regdata == 8)
  {
    Serial.print(F("ON"));
  }
  else
  {
    Serial.print(F("OFF"));
  }

  regdata = lora_Read(lora_RegPaConfig);
  regdata = regdata - 0xEE;

  Serial.print(F("  Power "));
  Serial.print(regdata);
  Serial.print(F("dBm"));
  Serial.println();
}


void system_Error()
{
  //there is a error, likley no LoRa device found, cannot continue
  while (1)
  {
    digitalWrite(LED1, HIGH);
    delay(50);
    digitalWrite(LED1, LOW);
    delay(50);
  }
}


void led_Flash(unsigned int flashes, unsigned int dealymS)
{
  //flash LED to show tracker is alive
  unsigned int index;

  for (index = 1; index <= flashes; index++)
  {
    digitalWrite(LED1, HIGH);
    delay(dealymS);
    digitalWrite(LED1, LOW);
    delay(dealymS);
  }
}


void read_Settings_Defaults()
{
  //To ensure the program routines are as common as possible betweeen transmitter and receiver
  //this receiver program uses constants in RAM copied from memory in the same way as the transmitter.
  //There are some exceptions, where the local programs need to use a setting unique to the particular
  //receiver.
  Serial.print(F("Configuring Settings from Defaults - "));
  ramc_CalibrationOffset = CalibrationOffset;
  ramc_TrackerMode_Frequency = TrackerMode_Frequency;
  ramc_CommandMode_Frequency = CommandMode_Frequency;
  ramc_SearchMode_Frequency = SearchMode_Frequency;
  ramc_Current_config1 = Default_config1;
  ramc_Current_config2 = Default_config2;
  ramc_Current_config3 = Default_config3;
  ramc_Current_config4 = Default_config4;
  ramc_TrackerMode_Bandwidth = TrackerMode_Bandwidth;
  ramc_TrackerMode_SpreadingFactor = TrackerMode_SpreadingFactor;
  ramc_TrackerMode_CodeRate = TrackerMode_CodeRate;
  ramc_CommandMode_Bandwidth = CommandMode_Bandwidth;
  ramc_CommandMode_SpreadingFactor = CommandMode_SpreadingFactor;
  ramc_CommandMode_CodeRate = CommandMode_CodeRate;
  ramc_SearchMode_Bandwidth = SearchMode_Bandwidth;
  ramc_SearchMode_SpreadingFactor = SearchMode_SpreadingFactor;
  ramc_SearchMode_CodeRate = SearchMode_CodeRate;
  ramc_TrackerMode_Power = TrackerMode_Power;
  ramc_SearchMode_Power = SearchMode_Power;
  ramc_CommandMode_Power = CommandMode_Power;
  Serial.println(F(" Done"));
}


void read_Settings_Memory()
{
  //To ensure the program routines are as common as possible betweeen transmitter and receiver
  //this receiver program uses constants in RAM copied from memory in the same way as the transmitter.
  //There are some exceptions, where the local programs need to use a setting unique to the particular
  //receiver.
  Serial.print(F("Configuring Settings from Memory"));
  ramc_CalibrationOffset = Memory_ReadInt(addr_CalibrationOffset);
  ramc_TrackerMode_Frequency = Memory_ReadULong(addr_TrackerMode_Frequency);
  ramc_CommandMode_Frequency = Memory_ReadULong(addr_CommandMode_Frequency);
  ramc_SearchMode_Frequency = Memory_ReadULong(addr_SearchMode_Frequency);
  ramc_Current_config1 = Memory_ReadByte(addr_Default_config1);
  ramc_Current_config2 = Memory_ReadByte(addr_Default_config2);
  ramc_Current_config3 = Memory_ReadByte(addr_Default_config3);
  ramc_Current_config4 = Memory_ReadByte(addr_Default_config4);
  ramc_TrackerMode_Bandwidth = Memory_ReadByte(addr_TrackerMode_Bandwidth);
  ramc_TrackerMode_SpreadingFactor = Memory_ReadByte(addr_TrackerMode_SpreadingFactor);
  ramc_TrackerMode_CodeRate = Memory_ReadByte(addr_TrackerMode_CodeRate);
  ramc_CommandMode_Bandwidth = Memory_ReadByte(addr_CommandMode_Bandwidth);
  ramc_CommandMode_SpreadingFactor = Memory_ReadByte(addr_CommandMode_SpreadingFactor);
  ramc_CommandMode_CodeRate = Memory_ReadByte(addr_CommandMode_CodeRate);
  ramc_SearchMode_Bandwidth = Memory_ReadByte(addr_SearchMode_Bandwidth);
  ramc_SearchMode_SpreadingFactor = Memory_ReadByte(addr_SearchMode_SpreadingFactor);
  ramc_SearchMode_CodeRate = Memory_ReadByte(addr_SearchMode_CodeRate);
  ramc_TrackerMode_Power = Memory_ReadByte(addr_TrackerMode_Power);
  ramc_SearchMode_Power = Memory_ReadByte(addr_SearchMode_Power);
  ramc_CommandMode_Power = Memory_ReadByte(addr_CommandMode_Power);
  Serial.println(F(" - Done"));
}

void write_Settings_Memory()
{
  //To ensure the program routines are as common as possible betweeen transmitter and receiver
  //this receiver program uses constants in RAM copied from memory in the same way as the transmitter.
  //There are some exceptions, where the local programs need to use a setting unique to the particular
  //receiver..
  Serial.print(F("Writing RAM Settings to Memory"));
  Memory_Set(addr_StartConfigData, addr_EndConfigData, 0);          //clear memory area first
  Memory_WriteInt(addr_CalibrationOffset, ramc_CalibrationOffset);
  Memory_WriteULong(addr_TrackerMode_Frequency, ramc_TrackerMode_Frequency);
  Memory_WriteULong(addr_CommandMode_Frequency, ramc_CommandMode_Frequency);
  Memory_WriteULong(addr_SearchMode_Frequency, ramc_SearchMode_Frequency);
  Memory_WriteByte(addr_Default_config1, ramc_Current_config1);
  Memory_WriteByte(addr_Default_config2, ramc_Current_config2);
  Memory_WriteByte(addr_Default_config3, ramc_Current_config3);
  Memory_WriteByte(addr_Default_config4, ramc_Current_config4);
  Memory_WriteByte(addr_TrackerMode_Bandwidth, ramc_TrackerMode_Bandwidth);
  Memory_WriteByte(addr_TrackerMode_SpreadingFactor, ramc_TrackerMode_SpreadingFactor);
  Memory_WriteByte(addr_TrackerMode_CodeRate, ramc_TrackerMode_CodeRate);
  Memory_WriteByte(addr_CommandMode_Bandwidth, ramc_CommandMode_Bandwidth);
  Memory_WriteByte(addr_CommandMode_SpreadingFactor, ramc_CommandMode_SpreadingFactor);
  Memory_WriteByte(addr_CommandMode_CodeRate, ramc_CommandMode_CodeRate);
  Memory_WriteByte(addr_SearchMode_Bandwidth, ramc_SearchMode_Bandwidth);
  Memory_WriteByte(addr_SearchMode_SpreadingFactor, ramc_SearchMode_SpreadingFactor);
  Memory_WriteByte(addr_SearchMode_CodeRate, ramc_SearchMode_CodeRate);
  Memory_WriteByte(addr_TrackerMode_Power, ramc_TrackerMode_Power);
  Memory_WriteByte(addr_SearchMode_Power, ramc_SearchMode_Power);
  Memory_WriteByte(addr_CommandMode_Power, ramc_CommandMode_Power);
  Serial.println(F(" - Done"));
}


void print_Powers()
{
  //print the mode power settings, sort of a check that a data transfer has gone OK
  byte memorydata;
  memorydata = Memory_ReadByte(addr_TrackerMode_Power);
  Serial.print(F("Tracker_Power "));
  Serial.println(memorydata);
  memorydata = Memory_ReadByte(addr_SearchMode_Power);
  Serial.print(F("Search_Power "));
  Serial.println(memorydata);
  memorydata = Memory_ReadByte(addr_CommandMode_Power);
  Serial.print(F("Command_Power "));
  Serial.println(memorydata);
}


void SD_WriteHABpacket_Log()
{
  //write HAB packet to SD card
  byte index;

  if (SD_Found)
  {
    Serial.println();
    Serial.print("Log to SD ");
    logFile.write(lora_RXSource);
    logFile.print(",");
    SD_addtimeanddate_Log();

    logFile.print(",");
    logFile.write(lora_RXPacketType);
    logFile.write(lora_RXDestination);
    logFile.write(lora_RXSource);

    for (index = lora_RXStart; index <= lora_RXEnd; index++)
    {
      logFile.write(lora_RXBUFF[index]);
    }
    logFile.write(13);
    logFile.write(10);
    logFile.flush();
  }
  else
  {
    Serial.println("No SD card for Logging");
  }
}




void SD_WriteBinarypacket_Log()
{
  //send the location data in binary packet to the log
  if (SD_Found)
  {
    Serial.println();
    Serial.print("Log to SD ");
    logFile.write(lora_RXSource);
    logFile.print(",");
    SD_addtimeanddate_Log();
    logFile.print(",");
    SD_addLatLonAlt_Log();

    logFile.write(13);
    logFile.write(10);
    logFile.flush();
    Serial.println(" - Flushed file");
  }
  else
  {
    Serial.println("No SD card for Logging");
  }
}



boolean setup_SDLOG()
{
  //checks if the SD card is present and can be initialised

  Serial.print("SD card...");

  if (!SD.begin(SD_CS))
  {
    Serial.println("Failed, or not present");
    SD_Found = false;
    writescreen_Alert6();
    return false;                         //don't do anything more:
  }

  Serial.print("Initialized OK");
  SD_Found = true;

  char filename[] = "Track000.txt";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logFile = SD.open(filename, FILE_WRITE);
      break;                            // leave the loop!
    }
  }

  Serial.print("...Writing to ");
  Serial.print("Track0");
  Serial.write(filename[6]);
  Serial.write(filename[7]);
  Serial.print(F(".txt"));
  return true;
}



void printlog_addleadingZero(byte temp)
{
  if (temp < 10)
  {
    logFile.print("0");
  }
  logFile.print(temp);
}


void Serialprint_addleadingZero(byte temp)
{
  if (temp < 10)
  {
    Serial.print("0");
  }
  Serial.print(temp);
}


void SD_addtimeanddate_Log()
{
  printlog_addleadingZero(TRHour);
  logFile.print(":");
  printlog_addleadingZero(TRMin);
  logFile.print(":");
  printlog_addleadingZero(TRSec);
  logFile.print(",");
  printlog_addleadingZero(TRDay);
  logFile.print("/");
  printlog_addleadingZero(TRMonth);
  logFile.print("/");
  printlog_addleadingZero(TRYear);
}


void SD_addLatLonAlt_Log()
{
  logFile.print(TRLat, 6);
  logFile.print(",");
  logFile.print(TRLon, 6);
  logFile.print(",");
  logFile.print(TRAlt, 6);
  logFile.write(13);
  logFile.write(10);
}


void switch_press()
{
  //interrupt routine called when switch is pressed
  byte index;

  if (!int_guard)                         //unless the guard is true ignore interrupt
  {
    Serial.print("Switch Interrupt Ignored");
    return;
  }

  delay(100);

  int_guard = false;                      //disable multiple interrupts having any affect

  if (!digitalRead(SWITCH1))
  {
    Serial.println(F("Switch1"));
    Switchpress = SWITCH1;
    Function_Number++;
  }
  else if (!digitalRead(SWITCH_U))
  {
    Serial.println(F("Switch U"));
    Switchpress = SWITCH_U;
    Function_Number--;
  }
  else if (!digitalRead(SWITCH_D))
  {
    Serial.println(F("Switch D"));
    Switchpress = SWITCH_D;
    Function_Number++;
  }
  else
  {
    Serial.println(F("ERROR - No Switch pressed"));
  }

  ramc_Receiver_Mode == Portable_Mode;    //switch pressed so switch to portable mode

  if (Function_Number > max_functions)
  {
    Function_Number = 1;
  }

  if (Function_Number == 0)
  {
    Function_Number = max_functions;
  }

  digitalWrite(LED1, HIGH);              //Flash LED and de-bounce a bit

  for (index = 0; index >= 100; index++)
  {
    delayMicroseconds(1000);
  }

  digitalWrite(LED1, LOW);

  ramc_Receiver_Mode == Portable_Mode;    //switch pressed so switch to portable mode

}

//*******************************************************************************************************
// Memory Routines
//*******************************************************************************************************

void Clear_Config_Memory()
{
  //clears the memory used for config
  Serial.print(F("Clearing Config Memory"));
  Memory_Set(addr_StartConfigData, addr_EndConfigData, 0);
  Serial.println(F(" - Done"));
}


void Clear_All_Memory()
{
  //clears the whole of memory, normally 1kbyte
  Serial.print(F("Clearing All Memory"));
  Memory_Set(addr_StartMemory, addr_EndMemory, 0);
  Serial.println(F(" - Done"));
}


void Print_All_Memory()
{
  //prints the memory used for storing configuration settings
  byte memory_LLoopv1;
  byte memory_LLoopv2;
  unsigned int memory_Laddr = 0;
  byte memory_Ldata;
  unsigned int CRC;
  Serial.println(F("Print All Memory"));
  Serial.print(F("Lcn    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F"));
  Serial.println();

  for (memory_LLoopv1 = 0; memory_LLoopv1 <= 63; memory_LLoopv1++)
  {
    Serial.print(F("0x"));
    Serial.print(memory_LLoopv1, HEX);                       //print the register number
    Serial.print(F("0  "));
    for (memory_LLoopv2 = 0; memory_LLoopv2 <= 15; memory_LLoopv2++)
    {
      memory_Ldata = Memory_ReadByte(memory_Laddr);
      if (memory_Ldata < 0x10) {
        Serial.print("0");
      }
      Serial.print(memory_Ldata, HEX);                       //print the register number
      Serial.print(F(" "));
      memory_Laddr++;
    }
    Serial.println();
  }

}


void Print_CRC_All_Memory()
{
  unsigned int returnedCRC = Memory_CRC(addr_StartMemory, addr_EndMemory);
  Serial.print(F("CRC_All_Memory "));
  Serial.println(returnedCRC, HEX);
}


void Print_CRC_Config_Memory()
{
  unsigned int returnedCRC = Memory_CRC(addr_StartConfigData, addr_EndConfigData);
  Serial.print(F("CRC_Config_Memory "));
  Serial.println(returnedCRC, HEX);
}

void Print_CRC_Bind_Memory()
{
  unsigned int returnedCRC = Memory_CRC(addr_StartBindData, addr_EndBindData);
  Serial.print(F("CRC_Bind_Memory "));
  Serial.println(returnedCRC, HEX);
}


//*******************************************************************************************************


void print_Nodes()
{
  //prints current node of this device
  Serial.print(F("This Node "));
  Serial.write(ThisNode);
  Serial.println();
}


void check_GPSforFix()
{
  //check GPS for new fix
  if (gps.location.isUpdated() && gps.altitude.isUpdated())          //check the fix is updated
  {
    digitalWrite(LED2, HIGH);

    last_LocalGPSfixmS = millis();

    Local_GPS_Fix = true;
    GLONASS_Active = false;

    record_LocalData();
    save_LocalData();
    fixes_till_set_time--;

    if (fixes_till_set_time <= 0)
    {
      fixes_till_set_time = GPSFixes;
      settime_GPS();
    }

    distanceto();
    directionto();

    update_screen(current_screen_number);
    digitalWrite(LED2, LOW);

  }
  else
  {

    if (GNGGAFIXQ.age() < 2000)                     //check to see if GLONASS has gone active
    {
      Serial.println(F("GLONASS Active !"));
      GLONASS_Active = true;
      setstatusByte(GLONASSisoutput, 1);
      GPS_Setup();
      writescreen_Alert1();
      delay(1000);
      update_screen(current_screen_number);
    }
    else
    {
      GLONASS_Active = false;
      setstatusByte(GLONASSisoutput, 0);
    }

    if (gps.location.age() > FixisoldmS)
    {
      Local_GPS_Fix = false;
    }

  }

}


void directionto()
{
  //using remote and local lat and long calculate direction in degrees
  TRdirection = (int) TinyGPSPlus::courseTo(LocalLat, LocalLon, TRLat, TRLon);
}


void distanceto()
{
  //using remote and local lat and long calculate distance in metres
  TRdistance = TinyGPSPlus::distanceBetween(LocalLat, LocalLon, TRLat, TRLon);
}


void print_last_HABpacket()
{
  //prints last received HAB packet to serial terminal
  byte memorydata;
  unsigned int address;
  address = addr_StartHABPayloadData;

  Serial.print("Last HAB Packet  ");
  do
  {
    memorydata = Memory_ReadByte(address);
    if ((memorydata == 0xFF) || (address >= addr_EndPayloadData))
    {
      break;
    }
    Serial.write(memorydata);
    address++;
  }
  while (true);
  Serial.println();
}


void setup_interrupts()
{
  attachInterrupt(digitalPinToInterrupt(SWITCH1), switch_press, CHANGE);   //This is a hardware interrupt
  attachPCINT(digitalPinToPCINT(SWITCH_U), switch_press, CHANGE);
  attachPCINT(digitalPinToPCINT(SWITCH_D), switch_press, CHANGE);

}

void Setup_LoRaTrackerMode()
{
  //sets LoRa modem to Tracker mode
  lora_SetFreq(ramc_TrackerMode_Frequency, ramc_CalibrationOffset);
  lora_SetModem2(ramc_TrackerMode_Bandwidth, ramc_TrackerMode_SpreadingFactor, ramc_TrackerMode_CodeRate, Explicit);  //Setup the LoRa modem parameters
  lora_Power = ramc_TrackerMode_Power;
  modenumber = TrackerMode;
}


void Setup_LoRaSearchMode()
{
  //sets LoRa modem to Search mode
  lora_SetFreq(ramc_SearchMode_Frequency, ramc_CalibrationOffset);
  lora_SetModem2(ramc_SearchMode_Bandwidth, ramc_SearchMode_SpreadingFactor, ramc_SearchMode_CodeRate, Explicit);  //Setup the LoRa modem parameters
  lora_Power = ramc_SearchMode_Power;
  modenumber = SearchMode;
}

void Setup_LoRaCommandMode()
{
  //sets LoRa modem to Command mode
  lora_SetFreq(ramc_CommandMode_Frequency, ramc_CalibrationOffset);
  lora_SetModem2(ramc_CommandMode_Bandwidth, ramc_CommandMode_SpreadingFactor, ramc_CommandMode_CodeRate, Explicit);  //Setup the LoRa modem parameters
  lora_Power = ramc_CommandMode_Power;
  modenumber = CommandMode;
}


void Setup_LoRaBindMode()
{
  //sets LoRa modem to Bind mode
  lora_SetFreq(BindMode_Frequency, ramc_CalibrationOffset);
  lora_SetModem2(BindMode_Bandwidth, BindMode_SpreadingFactor, BindMode_CodeRate, Explicit); //Setup the LoRa modem parameters
  lora_Power = BindMode_Power;
  modenumber = BindMode;
}


void display_frequency()
{
  //display current set frequency of LoRa device
  float freq_temp;
  freq_temp = lora_GetFreq();
  Serial.print(F("Set to Frequency "));
  Serial.print(freq_temp, 3);
  Serial.println(F("MHz"));
}



void display_frequencies_memory()
{
  //display the frequncies set in memory, good check to see if all is well with memory
  unsigned long freq_temp;
  freq_temp = Memory_ReadULong(addr_TrackerMode_Frequency);
  Serial.println(F("Frequencies stored in Memory"));
  freq_temp = Memory_ReadULong(addr_TrackerMode_Frequency);
  Serial.print(F("TrackerMode "));
  Serial.println(freq_temp);
  freq_temp = Memory_ReadULong(addr_SearchMode_Frequency);
  Serial.print(F("SearchMode  "));
  Serial.println(freq_temp);
  freq_temp = Memory_ReadULong(addr_CommandMode_Frequency);
  Serial.print(F("CommandMode "));
  Serial.println(freq_temp);
  Serial.println();

}


void print_system_timedate()
{
  time_t t = now();                     //Store the current time in time
  Serial.print(F("System Time "));
  Serialprint_addleadingZero(hour(t));
  Serial.print(F(":"));
  Serialprint_addleadingZero(minute(t));
  Serial.print(F(":"));
  Serialprint_addleadingZero(second(t));
  Serial.print(F(" "));
  Serial.print(day(t));
  Serial.print(F("/"));
  Serial.print(month(t));
  Serial.print(F("/"));
  Serial.print((year(t) - 2000));
  Serial.println();
}


void print_GPS_timedate()
{
  Serialprint_addleadingZero(gps.time.hour());
  Serial.print(F(":"));
  Serialprint_addleadingZero(gps.time.minute());
  Serial.print(F(":"));
  Serialprint_addleadingZero(gps.time.second());
  Serial.print(F("  "));
  Serial.print(gps.date.day());
  Serial.print(F("/"));
  Serial.print(gps.date.month());
  Serial.print(F("/"));
  Serial.print((gps.date.year() - 2000));
}


void settime_GPS()
{
  Serial.print(F("Set time and date from GPS  "));
  print_GPS_timedate();
  Serial.println();
  setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
}


unsigned int ReadSupplyVolts()
{
  //relies on 1V1 internal reference and 91K & 11K resistor divider
  //returns supply in mV @ 10mV per AD bit read
  unsigned int temp;
  byte i;
  SupplyVolts = 0;

  analogReference(INTERNAL1V1);
  for (i = 0; i <= 2; i++)                      //sample AD 3 times
  {
    temp = analogRead(SupplyAD);
    SupplyVolts = SupplyVolts + temp;
  }
  SupplyVolts = (unsigned int) ((SupplyVolts / 3) * ADMultiplier);
  return SupplyVolts;
}


void DisplaySupplyVolts()
{
  //get and display supply volts on terminal or monitor
  Serial.print(F("Supply Volts "));
  Serial.print(SupplyVolts);
  Serial.println(F("mV"));
}



void setstatusByte(byte bitnum, byte bitval)
{
  //program the status byte
#ifdef Debug
  if (bitval)
  {
    Serial.print(F("Set Status Bit "));
  }
  else
  {
    Serial.print(F("Clear Status Bit "));
  }

  Serial.println(bitnum);
#endif

  if (bitval == 0)
  {
    bitClear(TRStatus, bitnum);
  }
  else
  {
    bitSet(TRStatus, bitnum);
  }
}


void softReset()
{
  asm volatile ("  jmp 0");
}


void Check_for_Clear()
{
  //now lets provide a manual way to clear memory
  //need to activate SWITCH_U\SWITCH2 only, then press SWITCH1
  byte i;
  byte switchcount = 0;

  if (!digitalRead(SWITCH_U) && digitalRead(SWITCH1))
  {
    Serial.println();
    Serial.println(F("Check for Clear"));
    Serial.println();

    for (i = 0; i <= 60; i++)
    {
      digitalWrite(LED1, HIGH);
      delay(20);
      if (!digitalRead(SWITCH_U))
      {
        switchcount++;
      }
      digitalWrite(LED1, LOW);
      delay(20);
    }
  }
  if ((switchcount > 20) && (switchcount < 80))
  {
    Serial.println("Memory Clear Selected");
    writescreen_Alert8();
    Clear_All_Memory();
    read_Settings_Defaults();
    write_Settings_Memory();
    softReset();
  }

}


/*if (!digitalRead(SWITCH_U) && digitalRead(SWITCH1))
  {
  Serial.println("Memory Clear Selected");
  Serial.println("Press SWITCH1");
  writescreen_9();
  digitalWrite(LED2, HIGH);
  for (index = 0; index <= 100; index++)
  {
    if (!digitalRead(SWITCH1))
    {

    }
    delay(10);
  }
  digitalWrite(LED2, LOW);
  }


  }

*/

void setup()
{
  //needs no explanation I hope ...............

  unsigned int returnedCRC;
  int tempint;
  byte index;

  pinMode(LED1, OUTPUT);                    //setup pin for PCB LED
  led_Flash(2, 250);

  Serial.begin(38400);                       //setup Serial console ouput
  Serial.println();
  Serial.println();
  Serial.println(F(programname));
  Serial.println(F(programversion));
  Serial.println(F(aurthorname));
  ReadSupplyVolts();
  DisplaySupplyVolts();

  pinMode(SWITCH1, INPUT_PULLUP);            //setup switches
  pinMode(SWITCH_L, INPUT_PULLUP);
  pinMode(SWITCH_R, INPUT_PULLUP);
  pinMode(SWITCH_U, INPUT_PULLUP);
  pinMode(SWITCH_D, INPUT_PULLUP);

  pinMode(MEMORY_CS, OUTPUT);                //setup pin for memory chip select

  pinMode(LED2, OUTPUT);                    //setup pin for PCB LED
  pinMode(LED3, OUTPUT);                    //setup pin for Pin 13 (SCk) LED
  pinMode(GPSPOWER, OUTPUT);
  pinMode(DISP_CS, OUTPUT);
  digitalWrite(DISP_CS, HIGH);


  GPS_On(UseGPSPowerControl);

  SPI.begin();                               //initialize SPI:
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  pinMode(lora_NReset, OUTPUT);              //LoRa Device reset line
  pinMode (lora_NSS, OUTPUT);               //LoRa Device select line
  digitalWrite(lora_NSS, HIGH);
  digitalWrite(lora_NReset, HIGH);
  delay(1);

  Memory_Start();

  Display_Setup();

  Check_for_Clear();

#ifdef ClearAllMemory
  writescreen_9();
  Clear_All_Memory();
  Serial.println();
  Print_All_Memory();
  Serial.println();
#endif


#ifdef ClearConfigData
  Clear_Config_Memory();
#endif


#ifdef ConfigureDefaults
  read_Settings_Defaults();
  write_Settings_Memory();
  Serial.println();
#endif

#ifdef ConfigureFromMemory
  read_Settings_Memory();
#endif


#ifdef write_CalibrationOffset
  Memory_WriteInt(addr_CalibrationOffset, CalibrationOffset);
  Serial.print(F("Write Calibration Offset to Memory "));
  tempint = Memory_ReadInt(addr_CalibrationOffset);
  Serial.println(tempint);
#endif

  Print_CRC_All_Memory();
  Print_CRC_Config_Memory();
  Print_CRC_Bind_Memory();

  read_TRData();
  read_LocalData();

  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));   //redo faster SPI as changed by display_setup

  if (!lora_CheckDevice())
  {
    Serial.println("LoRa Device Error");
    led_Flash(40, 50);
  }

  lora_Setup();

#ifdef Use_NMEA_Bluetooth_Uplink
  Bluetooth_Serial_Setup();
#endif

  print_Nodes();
  Serial.println();

  Setup_LoRaTrackerMode();

  Serial.println();
  lora_Print();
  Serial.println();

  display_frequency();

  writescreen_8(ramc_TrackerMode_Frequency, ramc_SearchMode_Frequency, ramc_CommandMode_Frequency, SupplyVolts);

  digitalWrite(LED1, HIGH);                   //turn on LED
  lora_Tone(1000, 3000, 10);                  //Transmit an FM tone, 1000hz, 1000ms, 10dBm
  digitalWrite(LED1, LOW);                    //LED is off
  delay(1000);

  update_screen(3);

  lora_RXONLoRa();

#ifdef UseSD
  setup_SDLOG();                               //setup SD and delay a bit to ensure any pending ints cleared
#endif

  Serial.println();

#ifdef NMEAUplink
  Bluetooth_Serial_Setup();
#endif

  Serial.println();
  print_last_HABpacket();
  print_TRData();
  print_LocalData();
  Serial.println();

  GPS_Setup();

#ifdef UBLOX
  if (!GPS_CheckNavigation())                            //Check that UBLOX GPS is in Navigation model 6
  {
    Serial.println();
    GPS_Config_Error = true;
    setstatusByte(GPSError, 1);
  }
#endif



  fixes_till_set_time = 0;
  setTime(0, 0, 0, 1, 1, 0); // Another way to set
  print_system_timedate();
  Serial.println();
  display_frequencies_memory();

  int_guard = true;                              //set the guard variable to after attachInterrupt
  setup_interrupts();

#ifdef Use_NMEA_Bluetooth_Uplink
  Bluetooth_Serial.println("Bluetooth Active");
  Serial.println("LoRaTracker Receiver2 - Bluetooth Active");
#endif

}


