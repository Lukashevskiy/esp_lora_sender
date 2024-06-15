#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <DHT.h>
#include <string>
#include <ArduinoJson.h>

enum class SENSORS{
  DHT11,
  MOISURE_SENSOR
};
const int csPin = 15;          // LoRa radio chip select
const int resetPin = 16;       // LoRa radio reset
const int irqPin = 2;         // change for your board; must be a hardware interrupt pin

constexpr uint8_t DHT_PIN = D4;
constexpr uint8_t MOISURE_PIN = A0;

DHT dht(DHT_PIN, DHT11); 
float data[3]; // элемент 0 это влажность воздуха, элемент 2 темпиратура, элемент 3 влажность почвы

template<SENSORS T, typename... ReturnArgs>
std::tuple<ReturnArgs...> do_data_collect(){}

template<>
std::tuple<float> do_data_collect<SENSORS::MOISURE_SENSOR>(){
  
  auto data = std::make_tuple(analogRead(MOISURE_PIN));

  return data;
}

template<>
std::tuple<float, float> do_data_collect<SENSORS::DHT11>(){

  auto data = std::make_tuple(dht.readHumidity(), dht.readTemperature());
  
  return data;
}



String outgoing;              // outgoing message

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xFF;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

String get_name(byte sender) {
  String sender_name = "";
  switch (sender) {
    case 0xAA:
      sender_name = "Медведь";
      break;
    case 0xBB:
      sender_name = "Краб";
      break;
    case 0xCC:
      sender_name = "Камчатка";
      break;
    case 0xDD:
      sender_name = "Hello";
      break;
    default:
      sender_name = "Некто";
  }
  return sender_name;
}
void sendMessage(String outgoing);
void onReceive(int packetSize);

void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  Serial.println("LoRa Duplex");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  Serial.println("Я " + get_name(localAddress) + ", a это наш чат.");
  
  Serial.setTimeout(5);
  dht.begin();

}

void loop() {

  if ((millis() - lastSendTime) > interval) {
    std::tie(data[0], data[1]) = do_data_collect<SENSORS::DHT11, float, float>();

    if (isnan(data[0]) || isnan(data[1])){ 
      Serial.println("Не удается считать показания"); 
      return;
    }else{
      Serial.println("Показатели.");
    }

    std::tie(data[2]) = do_data_collect<SENSORS::MOISURE_SENSOR, float>();
    if (isnan(data[2])){ 
      Serial.println("Не удается считать показания"); 
      return;
    }else{
      Serial.println("Показатели.");
    
    }

    std::string message = std::to_string(data[0]) + " " + std::to_string(data[1]) + " " + std::to_string(data[2]);
    Serial.println(message.c_str());
    sendMessage(message.c_str());
      //Serial.println("Me: " + message);
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

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    //Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
//  if (recipient != localAddress && recipient != 0xAA) {
//    Serial.println("This message is not for me.");
//    return;                             // skip rest of function
//  }

  // if message is for this device, or broadcast, print details:
  String sender_name = get_name(sender);

  Serial.print(sender_name + ": " + incoming);
//  Serial.println("Sent to: 0x" + String(recipient, HEX));
//  Serial.println("Message ID: " + String(incomingMsgId));
//  Serial.println("Message length: " + String(incomingLength));
//  Serial.println("Message: " + incoming);
//  Serial.println("RSSI: " + String(LoRa.packetRssi()));
//  Serial.println("Snr: " + String(LoRa.packetSnr()));
  //Serial.println();
}