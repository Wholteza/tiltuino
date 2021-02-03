# Tiltuino

Tiltuino is a project that allows reading of tilt hydrometer v2 temperature and gravity data via BLE and log it to the Brewfather API. The only hardware needed is an Arduino 33 IOT.

![](./serial.png)

## How is data read?

The tilt hydrometer v2 makes use of ibeacon that communicates through bluetooth low energy. Instead of using the Service and Characteristic method of publishing data to the subscribers it broadcasts a major and minor value in the manufacturer data. Those two values are extracted and converted into decimal values.

## Getting started

### Requirements

The sketch depends on my fork of the official ArduinoBLE library. This fork adds a method to read the manufacturer data from bluetooth devices. The code for that was taken from an open pull request on the official libraries pull request section that has been ignored for a year. Link is included in the bottom of the readme.

Clone [Wholteza/ArduinoBLE](https://github.com/Wholteza/ArduinoBLE) and place it in your arduino IDE's library directory. Make sure that you don't have the official ArduinoBLE library in there from before.

Install ArduinoHttpClient and WiFiNINA from the Library Manager in the Arduino IDE.

### Steps

1. Clone this repository.
2. Replace the variables in the sketch:

- tiltBluetoothAddress: Can be obtained by downloading a BLE scanning app to your phone. Keep the tilt hydrometer near and tilt it a bit so it starts broadcasting. The device should show up as an "ibeacon". Copy the address from that device.
- brewfatherCustomStreamPath: Can be obtained in your brewfather settings under "Power-ups". Enable it and copy the url.
- customDeviceName: Choose a name, will be shown in brewfather.
- sendInterval: Interval in ms, should not be less than 900000 for brewfather.
- debugMode: Logs all actions to serial.

3. Upload it to the Arduino 33 IOT.
4. Keep your hydrometer near and tilt it a bit so it starts broadcasting.
5. Device should show up in brewfather device section with the name you selected.

- If not: Use the debug mode to see if some part fails.

## Resources

- [ArduinoBLE Repository](https://github.com/arduino-libraries/ArduinoBLE)
- [ArduinoBLE Manufacturer data pull request](https://github.com/arduino-libraries/ArduinoBLE/pull/53)
- [Brewfather custom device documentation](https://docs.brewfather.app/integrations/custom-stream)
- [Old arduino forum post of someone who wanted to do a simular thing](https://forum.arduino.cc/index.php?topic=626200.0)
