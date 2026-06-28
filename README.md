ESP8266 Tactical Telemetry Hub and Asynchronous Clock

Overview
A custom-built, hand-soldered IoT desktop device built on the ESP8266 . This project serves as a dashboard, merging real-time NTP chronometry, local environmental states (temperature/humidity), and a lightweight asynchronous web server for remote configuration. It features a completely custom, capacitive-touch-driven UI and an integrated micro-arcade physics engine to demonstrate high-speed OLED rendering.

Core Features
 Asynchronous Network Stack: Hosts a lightweight local web server allowing users to set alarms and countdown timers via a browser interface, without interrupting the main hardware loop.
 Environmental Telemetry: Integrates a DHT11 sensor to constantly poll and display local ambient temperature and humidity data.
 Capacitive Touch UI: Navigates the internal state machine (Clock -> Timer -> Game) entirely via dual capacitive touch inputs, eliminating mechanical switch degradation.
 Lithium Power Management: Powered by a 3.7V 2700mAh Li-ion cell routed through a voltage booster. Includes a voltage divider and  a custom software-side battery averaging algorithm to filter out voltage drops caused by Wi-Fi RF spikes.
 Integrated Physics Engine: Features "Cyber-Rex," a custom-coded, 60-FPS micro-arcade game running natively on the ESP8266, utilizing basic gravity and collision math.

Hardware Stack
Microcontroller: ESP8266 (NodeMCU/Wemos)
Display: 0.96" SSD1306 OLED (I2C)
Sensors: DHT11 (Temp/Humidity), 2x Capacitive Touch Modules
Power: 3.7V 2700mAh Li-ion + 5V Boost Converter
Audio: Standard 5V Piezo Buzzer

V1 Notes & Constraints & Future Improvements
This repository represents the V1 prototype. The following structural constraints have been identified for V2 optimization:
Power Draw: The V1 architecture continuously polls the Wi-Fi radio and drives the OLED at maximum brightness, resulting in an ~8-hour battery lifecycle. V2 will require deep-sleep implementation and interrupt-driven wake cycles to extend battery life.
Chassis Ergonomics: The physical placement of the capacitive sensors relative to the display creates a UI bottleneck during rapid inputs (e.g., during the arcade mode). V2 requires a redesigned, 3D-printed enclosure to separate the oled screen and touch sensors properly.