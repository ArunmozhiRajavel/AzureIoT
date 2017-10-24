// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
// This example only works with Arduino IDE 1.6.8 or later.

#include <ESP8266WiFi.h>
#include <time.h>
#include "command_center.h"
#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

const char ssid[] = "InfyIoT"; //  your WiFi SSID (name)
const char pass[] = "20072117";    // your WiFi password (use for WPA, or use as key for WEP)
const char connectionString[] = "HostName=InfyIoTHub.azure-devices.net;DeviceId=InfyTempCheck;SharedAccessKey=f1mLnJDGNL08+Wvn183ETJpu4o27ZyQkmvaEY+wEzDI=";

const char* mqtt_server = "52.163.243.255";
bool updateFirmware = 0;
HTTPClient http;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int count = 0;
void callback(char* topic, byte* payload, unsigned int length);

int status = WL_IDLE_STATUS;

///////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  initSerial();
  initWifi();
  initTime();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // This function doesn't exit.
  checkForUpdate();
  //command_center_run();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void initSerial() {
  // Start serial and initialize stdout
  Serial.begin(115200);
  Serial.setDebugOutput(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void initWifi() {
  // Attempt to connect to Wifi network:
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  status = WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to wifi");
  Serial.println("New firmware v7.4 - initial check");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void initTime() {
  time_t epochTime;

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  while (true) {
    epochTime = time(NULL);

    if (epochTime == 0) {
      Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
      delay(2000);
    } else {
      Serial.print("Fetched NTP epoch time is: ");
      Serial.println(epochTime);
      break;
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
    updateFirmware = 1;
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    updateFirmware = 0;
  }

}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("firmwareupdate");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void checkForUpdate() {
  reconnect();
  if (!client.connected())
  {
    Serial.println("Connecting to MQTT server");
    reconnect();
  }
  else
  {
    client.loop();
    delay(1000);
    if (updateFirmware) {
      client.publish("firmwareupdate", "0", true);
      Serial.println("Updating firmware");
      digitalWrite(LED_BUILTIN, LOW);
      t_httpUpdate_return ret = ESPhttpUpdate.update("http://s3.ap-south-1.amazonaws.com/iotci/Fimware/firmware.bin", "v1");
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;
          
        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK");
          client.publish("firmwareupdate", "2");
          digitalWrite(LED_BUILTIN, HIGH);
          delay(50);
          updateFirmware = 0;
          command_center_run();
          //ESP.restart();
          break;
      }
    }
    else
    {
      Serial.println("No update found");
      if (count > 5) {
        client.disconnect();
        command_center_run();
      }
      else
      {
        count++;
      }
    }
  }
}


