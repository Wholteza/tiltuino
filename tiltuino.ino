#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>

char asciiBanner[] = " ▄▄▄▄▄▄▄ ▄▄▄ ▄▄▄     ▄▄▄▄▄▄▄ ▄▄   ▄▄ ▄▄▄ ▄▄    ▄ ▄▄▄▄▄▄▄ \n█       █   █   █   █       █  █ █  █   █  █  █ █       █\n█▄     ▄█   █   █   █▄     ▄█  █ █  █   █   █▄█ █   ▄   █\n  █   █ █   █   █     █   █ █  █▄█  █   █       █  █ █  █\n  █   █ █   █   █▄▄▄  █   █ █       █   █  ▄    █  █▄█  █\n  █   █ █   █       █ █   █ █       █   █ █ █   █       █\n  █▄▄▄█ █▄▄▄█▄▄▄▄▄▄▄█ █▄▄▄█ █▄▄▄▄▄▄▄█▄▄▄█▄█  █▄▄█▄▄▄▄▄▄▄█";

char wifiSsid[] = "SSID";
char wifiPassword[] = "PASSWORD";

char tiltBluetoothAddress[] = "AA:BB:CC:DD:EE:FF";

String customDeviceName = "Tiltuino";
char brewfatherRootAddress[] = "log.brewfather.net";
char brewfatherCustomStreamPath[] = "/stream?id=yourpersonalid";
long sendInterval = 900000;
bool useCelsius = true;
bool useCustomDns = true;
IPAddress customDns(1, 1, 1, 1);


bool debugMode = true;

int wifiStatus = WL_IDLE_STATUS;
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
    Serial.println("starting BLE failed!");
    return false;
  }
  Serial.println("===BLUETOOTH START=======================================");
  BLE.scan();
  Serial.println("Started scanning..");
  return true;
}

bool stopBluetooth() {
  BLE.end();
  Serial.println("===BLUETOOTH STOP========================================");
}

void getNextValueFromTiltHydrometer(float reading[]) {
  BLEDevice peripheral = BLE.available();
  if (peripheral && peripheral.address() == tiltBluetoothAddress && peripheral.hasManufacturerData()) {
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
    return;
  }
  reading[0] = 0;
  reading[1] = 0;
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
    String ipAddress = IpAddressToString(WiFi.localIP());
    String requestBody = "{\n    \"name\": \"" + customDeviceName + "\",\n    \"temp\": \"" + String(reading[0]) + "\",\n    \"temp_unit\": \"" + (useCelsius ? "C" : "F") + "\",\n    \"gravity\": \"" + String(reading[1]) + "\",\n    \"gravity_unit\": \"G\"\n}";
    String request = "POST " + String(brewfatherCustomStreamPath) + " HTTP/1.1\nContent-Type: application/json\nAccept: */*\nHost: " + ipAddress + "\nAccept-Encoding: gzip, deflate, br\nConnection: keep-alive\nContent-Length: " + requestBody.length() + "\n\n" + requestBody;
    if (debugMode) {
      Serial.print("Attempting to log to: ");
      Serial.print(brewfatherRootAddress);
      Serial.println(brewfatherCustomStreamPath);
      Serial.println("===REQUEST START=======================================");
      Serial.println(request);
      Serial.println("===REQUEST END===========================================");
    }
    if (debugMode) {
      httpClient.post(brewfatherCustomStreamPath, "application/json", requestBody);
      int statusCode = httpClient.responseStatusCode();
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
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP(); // Device IP address
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void connectToAP() {
  while (wifiStatus != WL_CONNECTED) {
    if (debugMode) {
      Serial.print("Attempting to connect to wifi network: ");
      Serial.print(wifiSsid);
      Serial.print("...");
    }
    wifiStatus = WiFi.begin(wifiSsid, wifiPassword);

  }
  delay(1000);
  Serial.println("Success!");
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
  if (debugMode){
    Serial.println("===FETCHING NEW VALUE START================================");
  }
  float reading[2] = {0, 0};
  if (startBluetooth()) {
    while (reading[0] == 0 || reading[1] == 0) {
      getNextValueFromTiltHydrometer(reading);
    }
    stopBluetooth();
  }
  if (startWifi()) {
    sendReadingToBrewfather(reading);
    stopWifi();
  }
    if (debugMode){
    Serial.println("===FETCHING NEW VALUE END================================");
    Serial.println(String("===WAITING ") + String(sendInterval) + String("MS======================================"));
  }
  delay(sendInterval);
}
