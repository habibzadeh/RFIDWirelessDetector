#include <Arduino.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include "Rfid.h"
void setup();
void loop();
void sendSerialRFID();
void resetReader();
void check_radio(void);
#line 1 "src/detector.ino"

//#include <SPI.h>
//#include "nRF24L01.h"
//#include "RF24.h"
//#include "printf.h"
//#include <avr/sleep.h>
//#include <avr/power.h>

//#include "Rfid.h"

// Hardware configuration
RF24 radio(9,10);                          // Set up nRF24L01 radio on SPI bus plus pins 7 & 8
//pin 7 must be connected to D0
Rfid rfid(7,8);
int RFIDResetPin=0;

const short role_pin = 5;                 // sets the role of this unit in hardware.  Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter

const uint64_t address[2] = {0xABCDABCD71LL, 0x544d52687CLL};  // Radio pipe addresses for the 2 nodes to communicate.
byte rfid_read[12];

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
      printf_begin();
      pinMode(RFIDResetPin, OUTPUT);
      digitalWrite(RFIDResetPin, HIGH);
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
      radio.printDetails();                              // Dump the configuration of the rf unit for debugging
      delay(50);
      attachInterrupt(0, check_radio, FALLING);          // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver
    }

    static uint32_t message_count = 0;

    boolean new_rfid_received=false;
    /********************** Main Loop *********************/
    void loop() {

      if (role == role_sender)  {
        //new_rfid_received = rfid.RFIDRead(&rfid_read[0]);
        int i =0;
        /*  while(Serial.available() > 0)
        {
        byte readByte = Serial.read();
        rfid_read[i]= readByte;
        i++;
      }
      sendSerialRFID();
      delay(1000);
      // 2 52 70 48 48 52 48 70 49 66 48 52 69 13 10 3
      //RFID Received:2-52-70-48-48-52-49-53-53-49-67-52-55-13-10-3-
      */
      //new_rfid_received = rfid.RFIDReadHEX(&rfid_read[0]);
      new_rfid_received = rfid.RFIDRead(&rfid_read[0]);
      if(new_rfid_received){
        resetReader();
        sendSerialRFID();
        //radio.startWrite( &rfid_read[0], sizeof(byte)*6 ,0);
        delay(100);
        radio.flush_tx();
        new_rfid_received=false;
      }
      //received = rfid.RFIDRead(&rfid_read[0]);
      //  unsigned long time = millis();                   // Take the time, and send it.
      //radio.startWrite( &time, sizeof(unsigned long) ,0);

    }


    if(role == role_receiver){                        // Receiver does nothing except in IRQ
    }
  }
  void sendSerialRFID(){
    printf("RFID Received:");
    for(int i=0;i<12;i++){
      printf("%x",rfid_read[i]);
      printf("-");
    }
    printf("\n\r");

  }

  void resetReader(){
    ///////////////////////////////////
    //Reset the RFID reader to read again.
    ///////////////////////////////////
    digitalWrite(RFIDResetPin, LOW);
    digitalWrite(RFIDResetPin, HIGH);
    delay(500);
  }

  /********************** Interrupt *********************/

  void check_radio(void)
  {

    bool tx,fail,rx;
    radio.whatHappened(tx,fail,rx);                     // What happened?

    if ( tx ) {                                         // Have we successfully transmitted?
      if ( role == role_sender ){   printf("Send:OK\n\r"); }
        if ( role == role_receiver ){ printf("Ack Payload:Sent\n\r"); }
        }

        if ( fail ) {                                       // Have we failed to transmit?
          if ( role == role_sender ){   printf("Send:Failed\n\r");  }
            if ( role == role_receiver ){ printf("Ack Payload:Failed\n\r");  }
            }

            if ( rx || radio.available()){                      // Did we receive a message?

              if ( role == role_sender ) {                      // If we're the sender, we've received an ack payload
                radio.read(&message_count,sizeof(message_count));
                printf("Ack:%lu\n\r",message_count);
              }


              if ( role == role_receiver ) {                    // If we're the receiver, we've received a time message
                static unsigned long got_time;                  // Get this payload and dump it
                radio.read( &got_time, sizeof(got_time) );
                printf("Got payload %lu\n\r",got_time);
                radio.writeAckPayload( 1, &message_count, sizeof(message_count) );  // Add an ack packet for the next time around.  This is a simple
                ++message_count;                                // packet counter

              }
            }
          }
