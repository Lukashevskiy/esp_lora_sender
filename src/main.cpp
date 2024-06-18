#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <DHT.h>
#include <ArduinoJson.h>




const int csPin = 15;          // LoRa radio chip select
const int resetPin = 16;       // LoRa radio reset
const int irqPin = 2;         // change for your board; must be a hardware interrupt pin

constexpr uint8_t DHT_PIN = D4;
constexpr uint8_t MOISURE_PIN = A0;

DHT dht(DHT_PIN, DHT11); 
float data[3]; // элемент 0 это влажность воздуха, элемент 2 темпиратура, элемент 3 влажность почвы


struct dht11_t{
  float temp;
  float moi;
};

struct moisure_t{
  float value;
};

dht11_t get_data_dht11_t(){
  return {dht.readTemperature(), dht.readHumidity()};
}

moisure_t get_data_moisure_t(){
  return {analogRead(MOISURE_PIN)};
}


String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xFF;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends


void sendMessage(String outgoing);

void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  Serial.println("LoRa Duplex");

  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  
  Serial.setTimeout(5);
  dht.begin();
}

void loop() {

  if ((millis() - lastSendTime) > interval) {

    dht11_t data_dht = get_data_dht11_t();
    data[0] = data_dht.temp;
    data[1] = data_dht.moi;
    
    if (isnan(data[0]) || isnan(data[1])){ 
      Serial.println("Не удается считать показания"); 
      return;
    }else{
      Serial.println("Показатели.");
    }

    moisure_t data_moisure = get_data_moisure_t();
    data[2] = data_moisure.value;

    if (isnan(data[2])){ 
      Serial.println("Не удается считать показания"); 
      return;
    }else{
      Serial.println("Показатели.");
    }

    StaticJsonDocument<96> doc;
    JsonObject doc_0 = doc.createNestedObject();
    doc_0["temp"] = data[0];
    doc_0["wet"] = data[1];
    // doc[1]["value"] = data[2];

    String message;
    serializeJson(doc, message);
    
    Serial.println(message.c_str());
    sendMessage(message.c_str());
    lastSendTime = millis();            // timestamp the message
  }

}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address 
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}
