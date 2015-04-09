
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <avr/sleep.h>
#include <avr/power.h>

#include <SoftwareSerial.h>
// Hardware configuration
RF24 radio(9,10);                          // Set up nRF24L01 radio on SPI bus plus pins 7 & 8

SoftwareSerial midiSerial(3,4);

byte rfid_read[6]={0,0,0,0,0,0};


byte tag1[6] = {79,0,65,85,28,71};
byte tag2[6] = {79,0,64,241,176,78};
byte tag3[6] = {79,0,66,30,235,248};
byte tag4[6] = {79,0,64,152,109,250};
byte tag5[6] = {168,0,111,136,106,37};
byte tag6[6] = {79,0,64,152,104,255};

const short role_pin = 5;                 // sets the role of this unit in hardware.  Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter

const uint64_t address[2] = {0xABCDABCD71LL, 0x544d52687CLL};  // Radio pipe addresses for the 2 nodes to communicate.

// Role management

// Set up role.  This sketch uses the same software for all the nodes in this
// system.  Doing so greatly simplifies testing.  The hardware itself specifies
// which node it is.
// This is done through the role_pin
typedef enum { role_sender = 1, role_receiver } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Sender", "Receiver"};  // The debug-friendly names of those roles
role_e role;                                                            // The role of the current running sketch

boolean gotMsg = 0;                                                     // So we know when to go to sleep

/********************** Setup *********************/
void setup(){

  pinMode(role_pin, INPUT);                        // set up the role pin
  digitalWrite(role_pin,HIGH);                     // Change this to LOW/HIGH instead of using an external pin
  delay(20);                                       // Just to get a solid reading on the role pin

  if ( digitalRead(role_pin) )                    // read the address pin, establish our role
    role = role_sender;
    else
      role = role_receiver;


      Serial.begin(9600);
      midiSerial.begin(31250);
      printf_begin();
      printf("\n\rRF24/examples/pingpair_irq/\n\r");
      printf("ROLE: %s\n\r",role_friendly_name[role]);

      // Setup and configure rf radio
      radio.begin();
      // Max power
      radio.setPALevel( RF24_PA_MAX ) ;

      // Min speed (for better range I presume)
      radio.setDataRate( RF24_250KBPS ) ;

      // 8 bits CRC
      radio.setCRCLength( RF24_CRC_8 ) ;

      // Disable dynamic payloads
      //write_register(DYNPD,0);

      // increase the delay between retries & # of retries
      radio.setRetries(15,15);
      radio.enableAckPayload();                         // We will be using the Ack Payload feature, so please enable it
      // Open pipes to other node for communication
      if ( role == role_sender ) {                      // This simple sketch opens a pipe on a single address for these two nodes to
        radio.openWritingPipe(address[0]);             // communicate back and forth.  One listens on it, the other talks to it.
        radio.openReadingPipe(1,address[1]);
      }else{
        radio.openWritingPipe(address[1]);
        radio.openReadingPipe(1,address[0]);
        radio.startListening();
      }
      //radio.printDetails();                              // Dump the configuration of the rf unit for debugging
      delay(50);
      attachInterrupt(0, check_radio, FALLING);          // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver
    }

    static uint32_t message_count = 0;
    volatile boolean data_received=false;
    /********************** Main Loop *********************/
    void loop() {


      if (role == role_sender)  {                        // Sender role.  Repeatedly send the current time
        unsigned long time = millis();                   // Take the time, and send it.
        printf("Now sending %lu\n\r",time);
        radio.startWrite( &time, sizeof(unsigned long) ,0);
        delay(2000);                                     // Try again soon
      }


      if(role == role_receiver){                        // Receiver does nothing except in IRQ
        if(data_received && rfid_read[0]!=0){
          //playTone();
          sendSerialRFID();
          resetRFIDData();
          //sendSerialRFID();
          data_received=false;
        }
      }
    }
    void resetRFIDData()
    {
      for(int i=0;i<6;i++){
        rfid_read[i]=0;
      }
    }

    void sendSerialRFID(){
      printf("RFID Received:");

      //  printf("%s",s);

      for(int i=0;i<6;i++){
        printf("%d -",rfid_read[i]);
      }
      printf("\n\r");
    }

    void playTone(){
      if(compareTag(rfid_read, tag1)){ // if matched tag1, do this
        //printf("tag one detected\n\r");
        midiMessage(144,50,100);

      }else if(compareTag(rfid_read, tag2)){ //if matched tag2, do this
        //printf("tag 2 detected\n\r");
        midiMessage(144,60,100);

      }else if(compareTag(rfid_read, tag3)){
        //printf("tag 3 detected\n\r");
        midiMessage(144,70,100);

      }else if(compareTag(rfid_read, tag4)){
        //  printf("tag 4 detected\n\r");
        midiMessage(144,80,100);

      }else if(compareTag(rfid_read, tag5)){
        //printf("tag 5 detected\n\r");
        midiMessage(144,85,100);

      }else if(compareTag(rfid_read, tag6)){
        //printf("tag 6 detected\n\r");
        midiMessage(144,90,100);
      }
      else{ //JUST FOR TEST! unrecognized rfid
          midiMessage(144,90,100);
      }
    }
    //send MIDI message
    void midiMessage(int command, int MIDInote, int MIDIvelocity) {
      midiSerial.write(command);//send note on or note off command
      midiSerial.write(MIDInote);//send pitch data
      midiSerial.write(MIDIvelocity);//send velocity data
      //Serial.write(command);//send note on or note off command
      //Serial.write(MIDInote);//send pitch data
      //Serial.write(MIDIvelocity);//send velocity data
      //Serial.print("cmd: ");
      Serial.write(command);
      //Serial.print(", data1: ");
      Serial.write(MIDInote);
      //Serial.print(", data2: ");
      Serial.write(MIDIvelocity);
      delay(200);
      Serial.write(128);
      Serial.write(MIDInote);
      Serial.write(MIDIvelocity);

    }
    boolean compareTag(byte one[], byte two[]){
      ///////////////////////////////////
      //compare two value to see if same,
      //strcmp not working 100% so we do this
      ///////////////////////////////////

      for(int i = 0; i < 6; i++){
        if(one[i] != two[i]) return false;
      }

      return true; //no mismatches
    }
    /********************** Interrupt *********************/

    void check_radio(void)                                // Receiver role: Does nothing!  All the work is in IRQ
    {

      bool tx,fail,rx;
      radio.whatHappened(tx,fail,rx);                     // What happened?

      if ( tx ) {
        // Have we successfully transmitted?
        if ( role == role_sender ){
        //  printf("Send:OK\n\r");
        }
        if ( role == role_receiver )
          { //printf("Ack Payload:Sent\n\r");
        }
      }

      if ( fail ) {
        // Have we failed to transmit?
        if ( role == role_sender ){   printf("Send:Failed\n\r");  }
          if ( role == role_receiver ){ printf("Ack Payload:Failed\n\r");  }
          }

          if ( rx || radio.available()){
            // Did we receive a message?

            if ( role == role_sender ) {
              // If we're the sender, we've received an ack payload
              radio.read(&message_count,sizeof(message_count));
              printf("Ack:%lu\n\r",message_count);
            }


            if ( role == role_receiver )
              {
                // If we're the receiver, we've received a time message
                //static unsigned long got_time;                  // Get this payload and dump it
                //radio.read( &got_time, sizeof(got_time) );
                data_received=true;
                radio.read( &rfid_read[0], sizeof(byte)*6);

                radio.writeAckPayload( 1, &message_count, sizeof(message_count) );  // Add an ack packet for the next time around.  This is a simple
                ++message_count;                                // packet counter

              }
            }
          }
