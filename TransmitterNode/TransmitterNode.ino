#include <SPI.h>
#include <AESLib.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

uint8_t sharedKey[16];
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
  int p[16] = {17, 4, 5, 5, 6, 7, 7, 9,  9, 10, 10, 11, 11, 11, 11, 13};
  int g[16] = {3, 3, 2, 3, 5, 3, 5, 2,  5, 3, 7, 2, 6, 7, 8, 7};
  int a[16] = {2, 2, 2, 3, 4, 5, 3, 3,  2, 2, 4, 2, 4, 3, 1, 3};
  char c = 'c';
  printf("*** PRESS 'T' TO START KEY EXCHANGE\n");
  while(c != 'T') {
    c = toupper(Serial.read());
  }
  for(int i = 0; i < 16; i++) {
    transmit(p[i]);
    transmit(g[i]);
    int A = ipow(g[i], a[i]) % p[i];
    transmit(A);
    int B = receive();
    sharedKey[i] = ipow(B, a[i]) % p[i];
    printf("SHARED KEY: %d\n", sharedKey[i]);
    delay(100);
  }
  for(int i = 0; i < 16; i++) {
    printf("shared key %d: %d\n",i, sharedKey[i]);
  }
}

void loop(void) {
  char c = "o";
  char data[16] = "WahyuBerlianto"; //16 chars == 16 bytes
  Serial.println();
  Serial.println("==========ASN DEMO==========");
  Serial.print("PLAIN TEXT: ");
  Serial.println(data);
  aes128_enc_single(sharedKey, data);
  Serial.print("ENCRYPTED DATA: ");
  Serial.println(data);
  Serial.println("SENDING ENCRYPTED DATA...");
  for(int i = 0; i < 16; i++) {
    transmit(data[i]);
  }
  Serial.print("ENCRYPTED DATA SENT: ");
  Serial.println();
  delay(500);

  for(int i = 0; i < 16; i++) {
    data[i] = receive();
    delay(10);
  }
  Serial.print("RECEIVED ENCRYPTED DATA: ");
  Serial.println(data);
  aes128_dec_single(sharedKey, data);
  Serial.print("DECRYPTED DATA: ");
  Serial.println(data);
  Serial.println();
  delay(500);

  char data2[16] = "SatyaPradhana"; //16 chars == 16 bytes
  Serial.print("PLAIN TEXT: ");
  Serial.println(data2);
  aes128_enc_single(sharedKey, data2);
  Serial.print("ENCRYPTED DATA: ");
  Serial.println(data2);
  Serial.println("SENDING ENCRYPTED DATA...");
  for(int i = 0; i < 16; i++) {
    transmit(data2[i]);
  }
  Serial.println("ENCRYPTED DATA SENT...");
  delay(500);
  printf("*** PRESS 'T' TO RETRY AES\n");
  while(c != 'T') {
    c = toupper(Serial.read());
  }
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
    delay(20);
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
