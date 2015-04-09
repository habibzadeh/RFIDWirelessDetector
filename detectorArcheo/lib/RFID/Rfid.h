/*
rfid.h - Library for RFID reader through serial port.
Released into the public domain.
*/
#ifndef Rfid_h
#define Rfid_h

#include <Arduino.h>
#include <SoftwareSerial.h>

class Rfid
{
public:
  Rfid(int pin_in,int pin_out);
  bool serialReadLine(String &dest);
  bool RFIDReadHEX(byte *rFIDCode);
  bool RFIDRead(byte *rFIDCode);
  String byteArrayToString(byte *byteArray, int byteArraySize);
private:
  SoftwareSerial RFIDSerial;
};

#endif
