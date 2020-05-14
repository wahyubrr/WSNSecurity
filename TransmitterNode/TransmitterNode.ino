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

  // Initializing DHKE
  int p = 17;
  int g = 3;
  int a = 2;
  char c = 'c';
  printf("*** PRESS 'T' TO START KEY EXCHANGE\n");
  while(c != 'T') {
    c = toupper(Serial.read());
  }
  transmit(p);
  transmit(g);
  int A = ipow(g, a) % p;
  transmit(A);
  int B = receive();
  int sharedKey = ipow(B, a) % p;
  printf("SHARED KEY: %d\n", sharedKey);
  delay(1000);
}

void loop(void) {
//  int data;
//  transmit(7);
//  transmit(10);
//  data = receive();
//  data = receive();
}

int transmit(int data) {
  bool ack = 0;
  while(!ack) {
    // First, stop listening so we can talk.
    radio.stopListening();

    // Take the time, and send it.  This will block until complete
    printf("Now sending '%d'...",data);
    bool ok = radio.write( &data, sizeof(int) );

    if (ok)
      printf("ok...");
    else
      printf("failed.\n\r");

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 200 )
        timeout = true;

    // Describe the results
    if ( timeout ) {
      printf("Failed, response timed out.\n\r");
    }
    else {
      // Grab the response, compare, and send to debugging spew
      radio.read( &ack, sizeof(bool) );

      // Spew it
      printf("Got response ACK: %d\n",ack);
    }

    // Try again 1s later
    delay(1000);
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
    delay(20);
  }

  // First, stop listening so we can talk
  radio.stopListening();

  // Send the final one back.
  bool ack = true;
  radio.write( &ack, sizeof(bool) );
  printf("Sent ACK response.\n\r");

  return dataReceived;
}

int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}