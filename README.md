# ESP8266 Home Switches (10 Relay Controller)

A simple **ESP8266 (NodeMCU)** based smart home relay controller that works **completely locally** without any cloud services. The project uses **two 74HC595 shift registers** to control **10 relays** while using only **3 GPIO pins** on the ESP8266.

## Features

* Control up to **10 relays**
* Uses **2 × 74HC595 shift registers**
* Only **3 ESP8266 GPIO pins** required
* Local web dashboard
* Access using **http://homeswitches.local** (mDNS)
* Relay states saved in EEPROM
* Automatic state restoration after power failure
* Live relay status updates
* Serial logging for debugging
* No cloud or third-party server required

---

## Hardware Required

* ESP8266 NodeMCU
* 2 × 74HC595 Shift Registers
* 10-Channel Relay Module (or individual relays)
* 5V Power Supply
* Jumper Wires
* Breadboard or PCB

---

## GPIO Connections

| ESP8266 Pin | Function | 74HC595 Pin |
| ----------- | -------- | ----------- |
| D7 (GPIO13) | Data     | DS          |
| D5 (GPIO14) | Clock    | SH_CP       |
| D6 (GPIO12) | Latch    | ST_CP       |

Connect:

* Q7' (Pin 9) of the first 74HC595 → DS (Pin 14) of the second 74HC595.
* SH_CP and ST_CP are shared between both ICs.
* Connect all grounds together.

---

## Software

Libraries used:

* ESP8266WiFi
* ESP8266WebServer
* ESP8266mDNS
* EEPROM

Install the **ESP8266 Board Package** in the Arduino IDE before uploading.

---

## Configuration

Update your Wi-Fi credentials in the code:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

You can also edit the relay names:

```cpp
const char* relayNames[] = {
  "Living Room",
  "Hall",
  "Kitchen",
  ...
};
```

---

## Upload

1. Open the project in Arduino IDE.
2. Select **NodeMCU 1.0 (ESP-12E Module)**.
3. Select the correct COM port.
4. Upload the sketch.

---

## Access

After connecting to Wi-Fi, open:

```
http://homeswitches.local
```

or use the IP address shown in the Serial Monitor.

---

## Project Structure

```
HomeSwitches/
│
├── HomeSwitches.ino
├── README.md
└── Circuit_Diagram.png
```

---

## Future Improvements

* Password-protected dashboard
* MQTT support
* OTA firmware updates
* Scheduler/Timer
* Google Home & Alexa integration
* Energy monitoring

---

## License

This project is released under the MIT License.

Feel free to modify, improve, and use it in your own home automation projects.
