# ESP32-GardenSprinklerSystem
Garden sprinkler system for automated garden watering with multiple circles. Controls vents and pump vie Web Interface.
Garden sprinkler system is intended to automate a garden irrigation system/lawn sprinkler via ESP32 Hardware. The ESP will control relays, depended on automatic mode based on schedule or manual interaction, to control water valves for different water cycles.
It also can optionally check previous rain and forecast rain via api calls to OpenWether or Meteomatics and depended on the limits activate the schedule. Timers for short/manually start of watering.
Currently, it's only possible to control one pump and two circles (with additional hardware of course! --> HIGH level relays recommended). For more pumps/circles it's currently necessary to manually extend the main code, html and JavaScript.
