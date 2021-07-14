#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>

char asciiBanner[] = " ▄▄▄▄▄▄▄ ▄▄▄ ▄▄▄     ▄▄▄▄▄▄▄ ▄▄   ▄▄ ▄▄▄ ▄▄    ▄ ▄▄▄▄▄▄▄ \n█       █   █   █   █       █  █ █  █   █  █  █ █       █\n█▄     ▄█   █   █   █▄     ▄█  █ █  █   █   █▄█ █   ▄   █\n  █   █ █   █   █     █   █ █  █▄█  █   █       █  █ █  █\n  █   █ █   █   █▄▄▄  █   █ █       █   █  ▄    █  █▄█  █\n  █   █ █   █       █ █   █ █       █   █ █ █   █       █\n  █▄▄▄█ █▄▄▄█▄▄▄▄▄▄▄█ █▄▄▄█ █▄▄▄▄▄▄▄█▄▄▄█▄█  █▄▄█▄▄▄▄▄▄▄█";

char wifiSsid[] = "abc";
char wifiPassword[] = "abc";

char tiltBluetoothAddress[] = "00:00:00:00:00:00";

String customDeviceName = "Tiltuino";
char hostname[] = "tiltuino";
char brewfatherRootAddress[] = "log.brewfather.net";
char brewfatherCustomStreamPath[] = "/stream?id=abc";
long sendInterval = 10000;
bool useCelsius = true;
bool useCustomDns = false;
IPAddress customDns(1, 1, 1, 1);


bool debugMode = true;
bool sendDataToBrewfather = true;

WiFiClient wifiClient;
HttpClient httpClient = HttpClient(wifiClient, brewfatherRootAddress, 80);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  if (debugMode) {
    Serial.begin(9600);
    while (!Serial) {
      ;
    }
    Serial.println(asciiBanner);
  }
}

bool startBluetooth() {
  if (!BLE.begin()) {
    if (debugMode) {
      Serial.println("starting BLE failed!");
    }
    return false;
  }
  if (debugMode) {
    Serial.println("===BLUETOOTH START=======================================");
  }
  return true;
}

bool stopBluetooth() {
  BLE.end();
  if (debugMode) {
    Serial.println("===BLUETOOTH STOP========================================");
  }
}

int getNextValueFromTiltHydrometer(float reading[]) {
  int scanningSuccessful = BLE.scan();
  if (debugMode) {
    Serial.println("Scanning " + scanningSuccessful ? "successful" : "failed");
  }
  if (scanningSuccessful = 0){
    // Reset reading and return
    reading[0] = 0;
    reading[1] = 0;
    return 0;
  }
  BLEDevice peripheral = BLE.available();
  if(debugMode){
      Serial.println("---------");
      Serial.println("Found device");
      Serial.println("Address " + peripheral.address());
      Serial.println("HasManufacturerData " + peripheral.hasManufacturerData() ? "yes" : "no");
    }
  if (peripheral.address() != tiltBluetoothAddress || peripheral.hasManufacturerData() == 0){
    // Reset reading and return
    reading[0] = 0;
    reading[1] = 0;
    return 0;
  }
  if (debugMode) {
    Serial.println("===READING START=========================================");

    Serial.print("Address: ");
    Serial.println(peripheral.address());

    Serial.print("RSSI: ");
    Serial.println(peripheral.rssi());
  }
  String manufacturerData = peripheral.manufacturerData();
  if (debugMode) {
    Serial.print("Manufacturer data: ");
    Serial.println(manufacturerData);
  }

  String temperatureHex = manufacturerData.substring(40, 44);
  String gravityHex = manufacturerData.substring(44, 48);

  char tempHexAsChar[5];
  temperatureHex.toCharArray(tempHexAsChar, 5);
  float tempFarenheit = strtol(tempHexAsChar, NULL, 16);
  float tempCelsius = (tempFarenheit - 32) * .5556;
  if (debugMode) {
    Serial.print("Temp: ");
    Serial.println(useCelsius ? tempCelsius : tempFarenheit);
  }

  char gravityAsChar[5];
  gravityHex.toCharArray(gravityAsChar, 5);
  long gravityAsDecimalTimesThousand = strtol(gravityAsChar, NULL, 16);
  float gravity = gravityAsDecimalTimesThousand / 1000.0f;
  if (debugMode) {
    Serial.print("Gravity: ");
    Serial.println(gravity, 3);
    Serial.println("===READING END===========================================");
  }

  reading[0] = useCelsius ? tempCelsius : tempFarenheit;
  reading[1] = gravity;
  return 1;
}

bool startWifi() {
  if (WiFi.status() == WL_NO_MODULE) {
    if (debugMode) {
      Serial.println("Communication with the wifi module failed, stalling..");
    }
    return false;
  }
  if (debugMode) {
    Serial.println("===WIFI START============================================");
  }
  connectToAP();
  if (debugMode) {
    printWifiwifiStatus();
  }
  return true;
}

void stopWifi() {
  WiFi.end();
  if (debugMode) {
    Serial.println("===WIFI STOP=============================================");
  }
}

void sendReadingToBrewfather(float reading[]) {
  if (WiFi.status() == WL_CONNECTED) {
    httpClient = HttpClient(wifiClient, brewfatherRootAddress, 80);
    String requestBody = "{\n    \"name\": \"" + customDeviceName + "\",\n    \"temp\": \"" + String(reading[0]) + "\",\n    \"temp_unit\": \"" + (useCelsius ? "C" : "F") + "\",\n    \"gravity\": \"" + String(reading[1]) + "\",\n    \"gravity_unit\": \"G\"\n}";
    String request = "POST " + String(brewfatherCustomStreamPath) + " HTTP/1.1\nContent-Type: application/json\nAccept: */*\nHost: " + brewfatherRootAddress + "\nAccept-Encoding: gzip, deflate, br\nConnection: keep-alive\nContent-Length: " + requestBody.length() + "\n\n" + requestBody;
    if (debugMode) {
      Serial.print("Attempting to log to: ");
      Serial.print(brewfatherRootAddress);
      Serial.println(brewfatherCustomStreamPath);
      Serial.println("===REQUEST START=======================================");
      Serial.println(request);
      Serial.println("===REQUEST END===========================================");
    }
    int statusCode = 0;
    if (sendDataToBrewfather) {
      httpClient.post(brewfatherCustomStreamPath, "application/json", requestBody); 
      statusCode = httpClient.responseStatusCode();
    }
    if (debugMode && sendDataToBrewfather) {
      Serial.println("===RESPONSE START========================================");
      Serial.print("Status code: ");
      Serial.println(statusCode);
      String response = httpClient.responseBody();
      Serial.print("Body: ");
      Serial.println(response);
      Serial.println("===RESPONSE END==========================================");
    }
  }
}

void printWifiwifiStatus() {
  if (debugMode) {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
  
    IPAddress ip = WiFi.localIP(); // Device IP address
    Serial.print("IP Address: ");
    Serial.println(ip);
  }
}

void connectToAP() {
  while (WiFi.status() != WL_CONNECTED) {
    if (debugMode) {
      Serial.print("Attempting to connect to wifi network: ");
      Serial.print(wifiSsid);
      Serial.println("...");
    }
    WiFi.setHostname(hostname);
    WiFi.begin(wifiSsid, wifiPassword);
  }
  delay(1000);
  if (debugMode) {
    Serial.println("Success!");
  }
  if (useCustomDns) {
    WiFi.setDNS(customDns);
    if (debugMode) {
      Serial.print("DNS Configured: ");
      Serial.println(IpAddressToString(customDns));
    }
  }
  wifiClient = WiFiClient();
}

String IpAddressToString(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}

void loop() {
  if (debugMode) {
    Serial.println("===FETCHING NEW VALUE START================================");
  }
  float reading[2] = {0, 0};
  if (startBluetooth()) {
    int successfulReading = 0;
    while (successfulReading == 0) {
      successfulReading = getNextValueFromTiltHydrometer(reading);
      if (successfulReading == 0){
        delay(1000);
      }
    }
    stopBluetooth();
  }
  if (startWifi()) {
    sendReadingToBrewfather(reading);
    stopWifi();
  }
  if (debugMode) {
    Serial.println("===FETCHING NEW VALUE END================================");
    Serial.println(String("===WAITING ") + String(sendInterval) + String("MS======================================"));
  }
  delay(sendInterval);
}
