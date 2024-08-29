[![Arduino Build](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/arduino.yml/badge.svg?branch=main)](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/arduino.yml)
[![Doxygen Action](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/main.yml/badge.svg)](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/main.yml)



# DIY-Sim-Racing-FFB-Pedal

# Disclaimer
This repository documents my research progress. I wanted to understand the necessary signal processing and control theory algorithms behind such a device. 

The FFB pedal is a robot and can be dangerous. Please watch [The Terminator](https://en.wikipedia.org/wiki/The_Terminator) before continuing. If not interacted with care, it may cause harm. I'm not responsible for any harm caused by this design suggestion. Use responsibly and at your own risk.

# License
Shield: [![CC BY-NC-SA 4.0][cc-by-nc-sa-shield]][cc-by-nc-sa]

This work is licensed under a
[Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License][cc-by-nc-sa].

[![CC BY-NC-SA 4.0][cc-by-nc-sa-image]][cc-by-nc-sa]

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-image]: https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg

The reason for that is, that at some point in time individuals started
- to steal the sources and binaries and sold them on the internet
- mass production of FFB pedals in their living-rooms to make money by taking parts from this project and Simucubes design files.

All that without contributing anything to this project.

# Related repos
For the sake of clarity, this project is divided into multiple repositorys:
| Description           |  Link |
:------------------------- | :------------------------- |
| Mechanical and electrical design | https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design |
| Software (firmware, SimHub plugin, ...) |https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal |

# Features
## Control of pedal parameters
To tune the pedal parameters, a SimHub plugin was developed, which communicates with the pedal over USB.

## Effects
Currently ABS, TC and RPM vibration are supported effects. The SimHub plugin communicates with the pedal and triggers game effects as parameterized.The effects and its description can be found in [wiki](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/wiki/Pedal-Effects).

## Servo tuning
The used microcontroller has software to communicate with the used iSV57 servo. Therefore, it can tune the servos PID loop and read certain servo states like position, torque, power. 

## Joystick data stream
The joystick/gamepad data is provided via three redundant channels
1) Bluetooth
2) 0V-3.3V output analog signal. Can be read by e.g. https://gp2040-ce.info/. The pin 25 was used for analog output.
3) vJoy gamecontroller (only available when SimHub runs, also need enable control map plugin).

To provide native USB HID output, development with ESP32 S3 started, it's working, but not stable yet, [see](https://github.com/espressif/arduino-esp32/issues/9582#issuecomment-2219722111).

## Pedals in action
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/i2e1ukc1ylA/0.jpg)](https://www.youtube.com/watch?v=i2e1ukc1ylA)

More pedal action examples can be found in the Discord.


# Contributions
A lot of awesome devs have helped this project grow. Just to name a few:

- [tjfenwick](https://github.com/tjfenwick) started the project with an initial implementation.
- [tcfshcrw](https://github.com/tcfshcrw) helped to elevate the Simhub plugin to its current form, added a ton of pedal effects, hardware and discord support, good guy and much more.
- [MichaelJFr](https://github.com/MichaelJFr) helped with refactoring the code at the beginning of this project. Fruitful discussions let to the implementation of the control-loop strategies.
- [Ibakha](https://github.com/Ibakha) Discord channel CEO.


# Wiki
Detailed descriptions of certain aspects can be found on the dedicated [Wiki page](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/wiki):


# Discord
A [Discord](https://discord.gg/j8QhD5hCv7) server has been created to allow joint research.


# Hardware
During the development of this project, PCBs to hold the electric components were developed, see below <br>
<img src="https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design/blob/main/Wiring/Esp32_V3/PCB_assembled.jpg" width="400"> .

Also a (mostly) 3d printable mechanical design was designed and optimized to withstand the high forces of this application, see below <br>
<img src="https://github.com/user-attachments/assets/f1a54fd9-5949-4dc0-b573-b34a77b52dd7" width="400"> .


Please refer to the https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/tree/main?tab=readme-ov-file#related-repos section to access the design files.

<br>
<br>
<br>

Examples other awesome DIYers have done are listed below:

| Design           |  Link |
:------------------------- | :-------------------------
|<img src="https://user-images.githubusercontent.com/17485523/231913569-695fcab1-f0bb-4af6-8d90-b1bfaece13bc.png" height="200">  |  [Tjfenwick's design](https://github.com/tjfenwick/DIY-Sim-Racing-Active-Pedal)|
|<img src="https://user-images.githubusercontent.com/79850208/261399337-b313371c-9262-416d-a131-44fa269f9557.png" height="200">  |  [Bjoes design](https://github.com/Bjoes/DIY-Active-pedal-mechanical-design)|
|<img src="https://media.printables.com/media/prints/557527/images/4471984_0fbfebf6-7b91-47dd-9602-44a6c7e8b851/thumbs/inside/1600x1200/png/screenshot-2023-08-19-150158.webp" height="200">  |  [GWiz's design](https://www.printables.com/de/model/557527-simucube-style-active-pedal/files)|
|<img src="https://cdn.thingiverse.com/assets/14/7d/56/cd/03/large_display_9d83a9a8-2c8a-4940-b9ce-b4ae4f9674c6.jpg" height="200">  | [shf90's design](https://www.thingiverse.com/thing:6414587)|


# Software

## ESP32 code

### Architecture
A Doxygen report of the sources can be found [here](https://chrgri.github.io/DIY-Sim-Racing-FFB-Pedal/Arduino/html/index.html).

### Install ESP32 driver
The drivers can be found here [here](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).

### Firmware generation and flashing
Firmware can be built and flashed via VS Code. Prebuilt binaries can be flashed e.g. via ESP32 webflasher.

#### Built from source (via VS Code)
See this [guide](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/wiki/VScode-IDE-setup).

#### Flash prebuilt binaries via web flasher
The binaries are available [here](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/releases). They can be flashed via the ESP [webflasher](https://esp.huhn.me/). Another [Webflasher](https://nabucasa.github.io/esp-web-flasher/).
##### ESP32
Memory address            |  File
:-------------------------:|:-------------------------:
| 0x1000 | bootloader.bin |
| 0x8000 | partitions.bin | 
| 0xe000 | boot_app0.bin |
| 0x10000 | firmware.bin |

##### ESP32S3
Memory address            |  File
:-------------------------:|:-------------------------:
| 0x0000 | bootloader.bin |
| 0x8000 | partitions.bin | 
| 0xe000 | boot_app0.bin |
| 0x10000 | firmware.bin |

## iSV57T-130 servo config tuning
The iSV57T allows parameter tuning via its RS232 interface. To tune the servo towards this application, I executed the following [steps](StepperParameterization/StepperTuning.md).

With the current [PCB](Wiring/Esp32_V3) design, the ESP can directly communicate with the iSV57T servo. Manual tuning as described before isn't necessary anymore. A description of the steps I undertook to decode the communication protocol can be found on the Disord server. Additional features such as sensorless homing and lost-step recovery were developed and integrated with the help of this communication.

## SimHub plugin:
The SimHub plugin was designed to communicate with the ESP to (a) modify the pedal configuration, e.g. the force vs. travel parameterization and (b) to trigger effects such as ABS oscillations.  

![image](SimHubPlugin/Images/Plugin-UI.png)

To install the plugin, the plugin [DiyActivePedal.dll](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/releases) has to be copied to the SimHub directory, e.g. C:/Program Files (x86)/SimHub

# Steps after flashing the firmware
The pedal will not move initially after flashing. One has to open the SimHub plugin, connect to the pedal, and send a config with non-zero PID values.
Recommended PID values are:

```
P=0.2-0.4
I=50-150
D=0
```

After sending the initial config, power cycling of the pedal is necessary. The pedal should move afterward.


# Error handling
## Pedal doesn't move after initial setup
1. Make sure, that you follow the above instructions. The default PID values are set to 0 thus the pedal will not move. You have to send non-zero PID values and restart the pedal to observe pedal travel.
2. Open the serial monitor in Arduino IDE, set the baud rate to 921600, and restart the pedal. You should see some debug info. Make a screenshot and kindly ask the Discord server for help.

## Bluetooth doesn't show gamepad data
Install DirectX 9

## The serial monitor shows a message "Couldn't load config from EPROM due to version mismatch"
Install a SimHub plugin matching the ESP firmware you installed and send a config to the pedal.

## The com port showed access denied or can not connect
Check the arduino plugin scan setting, please use scan only specfiec port as below.<br>
<img src="Images/ArduinoPlugin_0.png" width="800">



# Todo

ESP code:
- [ ] Add automatic system identification of pedal response
- [ ] Add model-predictive-control to the ESP code for the improved pedal response
- [x] Add field to invert motor and losdcell direction
- [x] send joystick data to simhub plugin and provide data as vJoy gamecontroller
- [x] allow effects to move stepper beyond configured max/min position, but not the measured homing positions
- [x] Optimize iSV57 communication
  - [ ] Let the communication task run from the beginning of the setup routine
  - [x] Read pedal state every cycle (currently, the pedal performance is degraded)

      
SimHub plugin:
- [ ] Send SimHub data via wifi to ESP 
- [x] GUI design improvements for the SimHub plugin 
- [x] JSON deserialization make compatible with older revisions
- [ ] include the types header file and use it
- [ ] Make use of effects from the ShakeIt plugin
- [ ] add OTA update for esp firmware
- [x] automatic serial monitor update
- [ ] serial plotter
- [x] add different abs effect patterns, e.g. sawtooth
- [x] make effects proportional to force or travel selectable by dropdown menu
      
Misc:
- [ ] Create a video describing the build progress and the features
- [ ] Add Doxygen + Graphviz to the project to automatically generate documentation, architectural design overview, etc.
