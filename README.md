# Droid Brain Program

This project is software to control an Astromech style remote-controlled droid.
While the author took inspiration from several existing projects, including:
- [PenumbraShadowMD](https://github.com/reeltwo/PenumbraShadowMD)
- [SHADOW MD](https://astromech.net/droidwiki/SHADOW_MD)
- [PADAWAN360](https://astromech.net/droidwiki/PADAWAN360)
- 
The Droid Brain has been designed and implemented from the ground-up to act as the central processing unit for a droid, accepting input stimuli from various sources, interpreting that input, and generating output commands to various actuators.  It is designed to run exclusively on ESP32 based microcontrollers and to be compiled from the PlatformIO development environment.  This is a departure from many previous projects that attempt to embed everything into a single sketch.  While adding some complexity to the build process, it also provides an opportunity to provide flexibility that has been missing up to this point.

## Key Features

- **Extensible Controller Support**:
  - Supports multiple game controllers including:
    - Single/Dual Sony Move Controllers (Bluetooth)
    - Generic PS3 Bluetooth controllers
    - Generic Xbox Bluetooth Controllers
    - Widely available Bluetooth Low Energy Ring style "VR controllers"
    - Custom ESPNow based controllers (Future implementation)
  - Unlike existing control software, which often supports only one specific controller, Droid Brain allows you to pick the controller that best suites your particular situation.

- **Actuator Control**:
  - Designed to leverage several existing solutions external actuator solutions such as:
    - Marcduino boards
    - AstroPixel boards
    - Various sound boards:
      - Teensy based Human-Cyborg Relations sound generator
      - Sparkfun MP3 Trigger
      - DF Robotics DFMini MP3 player
    - Motor drivers:
      - Sabertooth dual motor drivers
      - Syren single motor drivers
      - Widely available DRV8871 single motor drivers
      - Additional drivers coming soon...
  - Also provides direct support for:
    - Adafruit PCA9685 Servo/PWM drivers

- **Platform**:
  - Exclusively designed for ESP32 based boards.
  - Supports both standard breakout boards and the Penumbra ESP32 board.
  - The Penumbra ESP32 is required if you plan to use a controller that requires a USB/Bluetooth dongle such as the Sony, PS3 or XBox controllers.  The Penumbra ESP32 with USB Host Mode support is sold by [Astromech.net user skelmir](https://astromech.net/forums/showthread.php?43249-Penumbra-ESP32-with-integrated-USB-host-shield))

## Getting Started

To get started with the Droid Brain Program, follow the steps below:

1. **Hardware Setup**:
   - Ensure your ESP32 board is properly connected and configured.
   - If using USB Host Mode, connect the necessary Bluetooth dongle.

2. **Software Installation**:
   - Clone this repository to your local machine.
   - Install any required libraries and dependencies.

3. **Configuration**:
   - Define your input controllers and actuators in the configuration files.
   - Upload the firmware to your ESP32 board.

4. **Running the Program**:
   - Power on your droid and test the input and output functions.
   - Use the provided tools and interfaces to monitor and debug the system.

## License

This source code is open-source and can be freely used, modified, and distributed for any non-commercial purposes. For commercial use, please obtain a license from the author.

Author: Kizmit99
License: Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)

For more information, visit [Droid Brain GitHub Repository](https://github.com/kizmit99/DroidBrain).

## Contributing

Contributions are welcome! Please read the contributing guidelines and submit your pull requests.

## Contact

For questions, suggestions, or commercial licensing inquiries, please contact the author via [GitHub](https://github.com/kizmit99).
