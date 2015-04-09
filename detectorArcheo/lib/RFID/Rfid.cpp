// digit 2 <-> 9
// 5V <->
// GND <->

#include "rfid.h"

//http://stackoverflow.com/questions/13169714/creating-a-library-for-an-arduino
Rfid::Rfid(int pin_in,int pin_out)
: RFIDSerial(pin_in,pin_out)
{
	RFIDSerial = SoftwareSerial(pin_in,pin_out);
	RFIDSerial.begin(9600);

}

//HEX byte read, 12 char long array as parameter
bool Rfid::RFIDReadHEX(byte *rFIDCode)
{

	int index = 0;
	boolean reading = false;
	while(RFIDSerial.available()){
		int readByte = RFIDSerial.read(); //read next available byte

		//header and footer of rfid tags
		if(readByte == 2) reading = true;
		if(readByte == 3) reading = false;
		//printf("byte read:%d", readByte);
		if(reading && readByte != 2 && readByte != 10 && readByte != 13){
			//store the tag
			rFIDCode[index] = readByte;
			index ++;
		}
	}
	if (index>0) printf("number of bytes read:%d\n\r", index);
	return (index == 12); //if 12 bytes was read, tag is OK
}
bool Rfid::RFIDRead(byte *rFIDCode) {

	byte i = 0;
	byte readByte = 0;
	byte tempByte = 0;
	byte checksum = 0;
	byte bytesread = 0;
	if(RFIDSerial.available() > 0) {

		readByte = RFIDSerial.read();
		if(readByte == 2) {                  // check for header

			bytesread = 0;

			while (bytesread < 12) {                        // read 10 digit code + 2 digit checksum
				if( RFIDSerial.available() > 0) {
					readByte = RFIDSerial.read();

					if((readByte == 0x0D)||(readByte == 0x0A)||(readByte == 0x03)||(readByte == 0x02)) { // if header or stop bytes before the 10 digit reading
						return false;                                  // stop reading
					}

					// Do Ascii/Hex conversion:
					if ((readByte >= '0') && (readByte <= '9')) {
						readByte = readByte - '0';
					} else if ((readByte >= 'A') && (readByte <= 'F')) {
						readByte = 10 + readByte - 'A';
					}

					// Every two hex-digits, add byte to code:
					if (bytesread & 1 == 1) {
						// make some space for this hex-digit by
						// shifting the previous hex-digit with 4 bits to the left:
						rFIDCode[bytesread >> 1] = (readByte | (tempByte << 4));

						if (bytesread >> 1 != 5) {                // If we're at the checksum byte,
							checksum ^= rFIDCode[bytesread >> 1];       // Calculate the checksum... (XOR)
						};
					} else {
						tempByte = readByte;                           // Store the first hex digit first...
					};

					bytesread++;                                // ready to read next digit
				}
			}

			if (bytesread == 12) {              // if 12 digit read is complete
				if(rFIDCode[5] == checksum) {
					return true;
				}
			}
		}
	}
	return false;
}
// <- fin RFIDRead

// byteArrayToString ->
String Rfid::byteArrayToString(byte *byteArray, int byteArraySize) {
	int i;
	String ret = "";
	for (i=0; i<byteArraySize; i++) {
		if(byteArray[i] < 6) ret += "0";
		ret += String(byteArray[i], HEX);
		if (i < 4) ret += " ";
	}
	return ret;
}
// <- fin byteArrayToString
