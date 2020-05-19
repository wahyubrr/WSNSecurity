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
//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.
//

// The various roles supported by this sketch
typedef enum { role_ping_out = 1, role_pong_back } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};

// The role of the current running sketch
role_e role = role_pong_back;

void setup(void)
{
  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();
  printf("Receiving Node...\n");

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

  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);

  //
  // Start listening
  //

  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.printDetails();

  //Initializing DHKE
  int p[16], g[16], b[16] = {3, 4, 3, 4, 2, 2, 1, 2,  3, 3, 3, 3, 2, 2, 2, 2};
  for(int i = 0; i < 16; i++) {
    printf("Waiting for p...\n");
    p[i] = receive();
    printf("Waiting for g...\n");
    g[i] = receive();
    int A = receive();
    int B = ipow(g[i], b[i]) % p[i];
    transmit(B);
    sharedKey[i] = ipow(A, b[i]) % p[i];
    printf("SHARED KEY: %d\n", sharedKey[i]);
  }
  for(int i = 0; i < 16; i++) {
    printf("shared key %d: %d\n",i, sharedKey[i]);
  }
}

void loop(void) {
  char c = "o";
  char data[16];
  
  Serial.println();
  Serial.println("==========AES DEMO==========");
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

  char data1[16] = "FahmiArdiansyah"; //16 chars == 16 bytes
  Serial.print("PLAIN TEXT: ");
  Serial.println(data1);
  aes128_enc_single(sharedKey, data1);
  Serial.print("ENCRYPTED DATA: ");
  Serial.println(data1);
  Serial.println("SENDING ENCRYPTED DATA...");
  for(int i = 0; i < 16; i++) {
    transmit(data1[i]);
  }
  Serial.println("ENCRYPTED DATA SENT...");
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
  delay(500);
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
      printf("Got response ACK: %d with round-trip delay of %dms\n",ack, millis()-started_waiting_at);
    }

    // Try again 1s later
    delay(20);
  }
}

int receive() {
  radio.startListening();
  bool incoming = 0;
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
    printf("Got payload '%d' with size of %d bytes ",dataReceived, sizeof(dataReceived));

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
