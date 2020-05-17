#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

int sharedKey;
//int pow(int a, int b);
//int transmit(int data);
//int receive(int data);
//int request_dhke(int privateKey, int p, int g);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup(void) {
  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();
  printf("Transmitting Node...");

  //
  // Setup and configure rf radio
  //

  radio.begin();
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MIN);
  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  //radio.setPayloadSize(8);

  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  //
  // Start listening
  //

  //radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.printDetails();

}

void loop(void) {
  int data;
  for(;;) {
    data = receive();
    Serial.print("Data received: ");
    Serial.println(data);
  }
}

int receive() {
  radio.startListening();
  bool incoming = 0;
  printf("waiting for data...\n");
  while(!incoming) {
    if(radio.available()) {
      incoming = 1;
    }
  }
  // Dump the payloads until we've gotten everything
  int dataReceived;
  bool done = false;
  while (!done) {
    // Fetch the payload, and see if this was the last one.
    done = radio.read( &dataReceived, sizeof(int) );

    // Spew it
    printf("Got payload '%d'...",dataReceived);

    // Delay just a little bit to let the other unit
    // make the transition to receiver
  }

  // First, stop listening so we can talk
  radio.stopListening();

  return dataReceived;
}
