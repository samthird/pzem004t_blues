#include <PZEM004Tv30.h>
#include <Notecard.h>
#include <Wire.h>

#define usbSerial Serial
#define PRODUCT_UID "product_uid_goes_here"
#define myProductID PRODUCT_UID

Notecard notecard;

PZEM004Tv30 pzem(Serial2, 16, 17);


typedef struct{
  float voltage;
  float current;
  float power;
  float energy;
  float frequency;
  float powerFactor;
  const char* datetime;
  const char* sensor_ID;
} energyReading;

energyReading readings[10];

int counter = 0;


String apikey = "super-secret-api-key";

String sensor_id = "PZEM004t_001";

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600);
#ifdef usbSerial
    delay(2500);
    usbSerial.begin(115200);
    notecard.setDebugOutputStream(usbSerial);
#endif

    // Initialize the physical I/O channel to the Notecard
#ifdef txRxPinsSerial
    notecard.begin(txRxPinsSerial, 9600);
#else
    Wire.begin();
    notecard.begin();
#endif

  J *req = notecard.newRequest("hub.set");
  
  if (myProductID[0]) {
        JAddStringToObject(req, "product", myProductID);
  }

  JAddStringToObject(req, "mode", "continuous");

  notecard.sendRequest(req);

  delay(30 * 1000);

  req = NoteNewRequest("card.time");
  notecard.sendRequest(req);
}

void loop() {

  float voltage;
  float current;
  float power;
  float energy;
  float frequency;
  float pf;

  if(counter < 10){

    // Check if the data is valid
    if(isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency) || isnan(pf)){
        Serial.println("Error reading voltage");
    }

    else{
      // Read the data from the sensor
      float voltage = pzem.voltage();
      float current = pzem.current();
      float power = pzem.power();
      float energy = pzem.energy();
      float frequency = pzem.frequency();
      float pf = pzem.pf();
  
      energyReading temp = {voltage, current, power, energy, frequency, pf};
      readings[counter] = temp;
    }
  }
  if(counter == 9){
    J *req = notecard.newRequest("note.add");
    if(req != NULL){
      JAddBoolToObject(req, "sync", true);
      J *bodyArray = JCreateArray();
      for(int i = 0; i < 10; i++){
        J *bodyObject = JCreateObject();
        JAddNumberToObject(bodyObject, "voltage", round2(readings[i].voltage));
        JAddNumberToObject(bodyObject, "current", round2(readings[i].current));
        JAddNumberToObject(bodyObject, "power", round2(readings[i].power));
        JAddNumberToObject(bodyObject, "energy", round2(readings[i].energy));
        JAddNumberToObject(bodyObject, "frequency", round2(readings[i].frequency));
        JAddNumberToObject(bodyObject, "powerFactor", round2(readings[i].powerFactor));
        JAddStringToObject(bodyObject, "sendorID", readings[i].sensor_ID);
        JAddStringToObject(bodyObject, "datetime", readings[i].datetime);

        JAddItemToArray(bodyArray, bodyObject);
      }
      JAddItemToObject(req, "body", bodyArray);
    }
    notecard.sendRequest(req);
    counter = 0;
  }
  Serial.println();
  delay(5000);
}

double round2(double value) {
   return (int)(value * 100 + 0.5) / 100.0;
}
