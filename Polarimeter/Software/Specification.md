# Polarimeter System Specification

**Version:** 1.0
**Date:** 2025-04-13

## 1. User-Level Specification

### 1.1 Overview

This document specifies a prototype polarimeter controlled by an Arduino Nano. The system measures light intensity as a function of the rotation angle of a polarizing filter. It is designed to be controlled and monitored via a USB connection to a host computer using a serial terminal or custom software.

### 1.2 Features

* **Automated Measurement Scan:** Rotates a servo-controlled polarizing filter through a specified angular range and records light intensity at defined steps.
* **Configurable Scan Parameters:** Users can define the start angle, end angle, and step size for the measurement scan via serial commands.
    * Default Range: 0 to 180 degrees.
    * Default Step Size: 1 degree.
    * Customizable Range: Any start/end angle between 0 and 180 degrees.
    * Customizable Step Size: Any step size between 1 and 180 degrees.
* **Manual Servo Control:** Allows the user to return the rotating polarizer to its home position (0 degrees).
* **Light Source Control:** Allows the user to manually turn the internal LED light source on or off.
* **Serial Data Output:** Transmits measurement data (angle, intensity) to the connected computer via the USB serial connection in a simple, parseable format (CSV).
* **Command-Line Interface:** Users interact with the system by sending text-based commands through a serial terminal.
* **Help Command:** Provides users with a list of available commands and their syntax.

### 1.3 User Interaction

The system is operated by connecting it to a computer via USB and using a serial terminal application (like the Arduino Serial Monitor, PuTTY, Tera Term, etc.) configured with the correct COM port and baud rate (e.g., 9600).

The user sends commands as text strings terminated by a newline character. Key commands include:

* `help`: Displays available commands.
* `run [start] [end] [step]`: Initiates a measurement scan. Arguments are optional; defaults are used if omitted.
* `led <on|off>`: Controls the LED state.
* `home`: Moves the servo to the 0-degree position.

The system provides feedback messages via the serial connection, confirming actions, reporting errors, and indicating measurement progress.

### 1.4 Data Output

During a `run` command, after initial status messages, the system outputs measurement data via the serial connection. The data stream is formatted as follows:

1.  Start Marker: `---DATA_START---`
2.  Header Line: `Angle,Intensity`
3.  Data Lines: `<angle>,<intensity>` (one line per measurement step)
4.  End Marker: `---DATA_END---`

This format allows easy parsing and import into data analysis software (e.g., spreadsheets, Python scripts).

## 2. Technical Specification

### 2.1 Hardware Components

* **Microcontroller:** Arduino Nano (or compatible board with an ATmega328P).
* **Rotational Actuator:** Standard hobby servo motor (capable of 0-180 degree rotation).
    * Connected to: Digital Pin 9 (PWM capable).
* **Light Source:** Red LED (or other specified wavelength).
    * Connected to: Digital Pin 10.
* **Light Detector:** Photodiode or Phototransistor connected as an analog sensor.
    * Connected to: Analog Pin A0.
* **Polarizers:**
    * One fixed linear polarizing film.
    * One linear polarizing film mounted to the servo motor shaft.
* **Power Supply:** Appropriate power source for Arduino and servo (typically 5V via USB or external supply).
* **Connectivity:** USB Type-B or Mini-B (depending on Arduino model) for connection to a host computer.

### 2.2 Software Design

#### 2.2.1 Architecture

The Arduino firmware is implemented in C++ using an Object-Oriented Programming (OOP) approach to enhance modularity, maintainability, and readability. Core functionalities are encapsulated within distinct classes.

#### 2.2.2 Core Classes

* **`PolarizerServo`:** Manages all interactions with the servo motor, including initialization (`attach`), setting position (`setPosition`), and homing (`home`). Encapsulates the `Servo.h` library usage.
* **`LightSensor`:** Handles reading the analog value from the photodetector (`readIntensity`) and pin setup (`setup`).
* **`LightSource`:** Controls the LED state (`turnOn`, `turnOff`, `isOn`) and pin setup (`setup`).
* **`Polarimeter`:** The main orchestrator class.
    * Holds references to the `PolarizerServo`, `LightSensor`, and `LightSource` objects.
    * Initializes the system (`setup`).
    * Processes incoming serial commands (`processCommand`).
    * Executes the measurement sequence (`runMeasurement`), coordinating the servo, sensor, and serial output.
    * Provides user help (`displayHelp`).
    * Parses command arguments (`parseRunCommand`).

#### 2.2.3 Control Flow

* **`setup()`:** Initializes Serial communication, creates and initializes instances of all component classes via the `Polarimeter` object's `setup()` method (which calls the setup methods of its components), homes the servo, and turns on the LED.
* **`loop()`:** Continuously checks for incoming serial data. If a complete command (terminated by newline) is received, it passes the command string to the `Polarimeter` object's `processCommand` method for handling.

### 2.3 Communication Protocol

#### 2.3.1 Interface

* Serial communication over USB connection.

#### 2.3.2 Baud Rate

* 9600 bps (configurable in code, but 9600 is the default set). 8 data bits, no parity, 1 stop bit (8N1).

#### 2.3.3 Command Structure

Commands are case-insensitive for keywords (`run`, `led`, `home`, `help`) but case-sensitive for arguments like `on`/`off`. Commands must be terminated with a newline character (`\n`).

* `help`: No arguments.
* `run`: Optional arguments: `[start_angle] [end_angle] [step_angle]`. Integers expected.
    * Defaults: start=0, end=180, step=1.
    * Validation: 0 <= angles <= 180, 1 <= step <= 180, start != end.
* `led`: Argument: `on` or `off`.
* `home`: No arguments.

#### 2.3.4 Data Output Format (during `run`)

Data is transmitted via Serial after the `run` command is initiated and parameters are confirmed.


<Informational messages, e.g., "Starting measurement...">
---DATA_START---
Angle,Intensity
<angle_value_1>,<intensity_value_1>
<angle_value_2>,<intensity_value_2>
...
<angle_value_N>,<intensity_value_N>
---DATA_END---
<Completion message, e.g., "Measurement complete.">


* `<angle_value>`: Integer representing the servo angle (0-180).
* `<intensity_value>`: Integer representing the raw analog reading from the photodetector (0-1023).

