#include <RF24.h>

/*
This attribution is specified as a lack of license in the original code.
If you are the original author, please contact and make the license explicit.

With code fragments by Stanley Seow's RF24
https://github.com/stanleyseow/RF24/
*/

#include <SPI.h>
#include "printf.h"

#define RF_SETUP 0x17

// Set up nRF24L01 radio on SPI pin for CE, CSN
RF24 radio(8,9);

const unsigned short sensorId = 1;
const uint64_t basePipe = 0xF0F0F0F0E0LL;
const uint64_t readPipe = basePipe + 1 + 2*sensorId;
const uint64_t writePipe = readPipe + 1;

boolean isConfigured = false;

uint8_t *inputPins = NULL;
uint8_t *outputPins = NULL;

static const int BUF_MAX = 128;
static const uint8_t INVALID = 255;

void setup(void)
{
  Serial.begin(57600);

  printf_begin();

  radio.begin();

  // Enable this seems to work better
  radio.enableDynamicPayloads();

  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(76);
  radio.setRetries(15,15);

  radio.openWritingPipe(writePipe);
  radio.openReadingPipe(1,readPipe);

  // Dump the configuration of the rf unit for debugging
  radio.printDetails();
  delay(500);

  radio.startListening();
}

void loop(void)
{
  while (!isConfigured) {
    listenForConfiguration();
  }

  if (outputPins) {
    receiveAndApplyOutputs(outputPins);
    readSensorInputs(outputPins);
  }

  if (inputPins) {
    readSensorInputs(inputPins);
  }

  delay(100);
}

void readSensorInputs(uint8_t *pins)
{
  char buffer[BUF_MAX]="";
  sprintf(buffer, "%hu ", sensorId);

  for (int i=0; pins[i]!=INVALID; i++) {
    const uint8_t pin = pins[i];
    uint8_t value = pin > 13 ? analogRead(pin) : digitalRead(pin);

    char str[BUF_MAX]="";
    sprintf(str, "%d,%d;", pin, value);
    strcat(buffer, str);
  }

  if (strlen(buffer) == 0)
    return;

  radio.stopListening();
  delay(20);

  if ( radio.write( buffer, strlen(buffer)) ) {
    printf("SENT %s\n", buffer);
  }

  radio.startListening();
  delay(20);
}

void applyValueToPin(char *pinAndValue, uint8_t *pins)
{
  char *pin = strsep(&pinAndValue, ",");
  char *value = strsep(&pinAndValue, ",");

  uint8_t pinNum = strtol(pin, NULL,  10);
  uint8_t valueNum = strtol(value, NULL,  10);

  boolean found = false;
  for (int i=0; pins[i]!=INVALID; i++) {
    if (pinNum == pins[i]) {
      found = true;
      break;
    }
  }

  if (found) {
    printf("SET PIN %d to %d\n", pinNum, valueNum);
    digitalWrite(pinNum, valueNum == 0 ? LOW : HIGH);
  } else {
    printf("WARNING: Attempt to set PIN %d when not set to Output\n", pinNum);
  }
}

void receiveAndApplyOutputs(uint8_t *pins)
{
  while (radio.available()) {
    uint8_t length = radio.getDynamicPayloadSize();
    char *buffer = (char*)malloc(length);

    radio.read(buffer, length);
    buffer[length-1] = '\0';

    char *pinAndValue = NULL;
    while ((pinAndValue = strsep(&buffer, ":")) != NULL) {
      applyValueToPin(pinAndValue, pins);
    }

    free(buffer);
  }
}

void listenForConfiguration()
{
  delay(20);

  printf("Waiting for configuration...\n");
  while ( !radio.available())
    ;

  uint8_t length = radio.getDynamicPayloadSize();
  char *buffer = (char*)malloc(length);

  radio.read(buffer, length);
  buffer[length-1] = '\0';

  if (strlen(buffer)) {
    printf("Received Configuration of %d bytes\n", length);
    printf("%s\n", buffer);
    isConfigured = applyConfiguration(buffer);
  } else {
    printf("Warning: Got empty configuration\n");
  }

  free(buffer);
}

boolean applyConfiguration(char *config)
{
  boolean didApply = false;
  char *input = strsep(&config, ";");
  char *output = strsep(&config, ";");

  printf("INPUT %s\n", input);
  printf("OUTPUT %s\n", output);

  if (strlen(input)) {
    inputPins = parsePins(input);
    configurePins(inputPins, INPUT);
    didApply = true;
  }
  if (strlen(output)) {
    outputPins = parsePins(output);
    configurePins(outputPins, OUTPUT);
    didApply = true;
  }

  return didApply;
}

void configurePins(uint8_t *pins, uint8_t mode)
{
  const char *modeString = mode ? "OUTPUT" : "INPUT";

  for (int i=0; pins[i]!=INVALID; i++) {
    const uint8_t pin = pins[i];
    printf("Enabling %s on %d\n", modeString, pin);
    pinMode(pin, mode);
  }
}

uint8_t* parsePins(char *pins)
{
  char *pin;

  //One comma means two numbers, so +1 int
  //And another int to store INVALID to denote end
  int numCommas = 2;
  for (int i=0; pins[i]; i++)
    numCommas = pins[i]==',' ? numCommas + 1 : numCommas;

  uint8_t *pinArray = (uint8_t*) malloc(numCommas*sizeof(uint8_t));

  int i=0;
  while ((pin = strsep(&pins, ",")) != NULL) {
    uint8_t pinNum = strtol(pin, NULL,  10);
    pinArray[i++] = pinNum;
  }

  pinArray[i] = INVALID;  //Denotes the end of array
  return pinArray;
}

