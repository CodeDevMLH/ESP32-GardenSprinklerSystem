# ESP32-GardenSprinklerSystem
Garden sprinkler system is intended to automate a garden irrigation system/lawn sprinkler with ESP32 Hardware. The ESP will control relays, depended on automatic mode based on schedule or manual interaction, to control water valves for different water cycles.
It also can optionally check previous rain and forecast rain via api calls to OpenWether or Meteomatics and depended on the limits activate the schedule. Timers for short/manually start of watering.

# Adaption
Currently, it's only possible to control one pump and two circles (with additional hardware of course! --> HIGH level relays recommended). For more pumps/circles it's currently necessary to manually extend the main code, html and JavaScript.
Feel free to modify, share and contribute
The web interface is currently only in german available, feel free to translate it or even add a language selection button!

# Hardware
The project builds up on the ESP32-Hardware and is currently not running on any other microcontrollers.
The ESP triggers relays with a HIGH level on the output GPIO. So I recommend HIGH level relays to control the Pump and magnetic valve (commonly 24V AC)

# To-Do
- nice sucess/error/info/confirm alerts
- all Serial.prints also on WebSerial
- and probably much more...
