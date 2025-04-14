#include <Servo.h>

// --- Constants ---
const int SERVO_PIN = 9;
const int PHOTO_PIN = A0;
const int LED_PIN = 10;

// --- Class Definitions ---

/**
 * @class PolarizerServo
 * @brief Manages the servo motor controlling the polarization filter.
 */
class PolarizerServo {
private:
  Servo servoMotor;
  int pin;
  int currentAngle;

public:
  PolarizerServo(int servoPin) : pin(servoPin), currentAngle(0) {}

  void attach() {
    servoMotor.attach(pin);
    Serial.println("Servo attached on pin " + String(pin));
  }

  void setPosition(int angle, int waitMillis = 100) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    currentAngle = angle;
    servoMotor.write(currentAngle);
    if (waitMillis > 0) {
      delay(waitMillis);
    }
  }

  void home() {
    setPosition(0, 500);
    Serial.println("Servo homed.");
  }

  int getCurrentAngle() const {
    return currentAngle;
  }
};

/**
 * @class LightSensor
 * @brief Manages the photodetector/light sensor.
 */
class LightSensor {
private:
  int pin;

public:
  LightSensor(int sensorPin) : pin(sensorPin) {}

  void setup() {
    pinMode(pin, INPUT);
    Serial.println("Light sensor configured on pin A" + String(pin - A0));
  }

  int readIntensity() {
    return analogRead(pin);
  }
};

/**
 * @class LightSource
 * @brief Manages the LED light source.
 */
class LightSource {
private:
  int pin;
  bool isOnState;

public:
  LightSource(int ledPin) : pin(ledPin), isOnState(false) {}

  void setup() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    Serial.println("Light source configured on pin " + String(pin));
  }

  void turnOn() {
    digitalWrite(pin, HIGH);
    isOnState = true;
    Serial.println("LED turned ON.");
  }

  void turnOff() {
    digitalWrite(pin, LOW);
    isOnState = false;
    Serial.println("LED turned OFF.");
  }

  bool isOn() const {
    return isOnState;
  }
};


/**
 * @class Polarimeter
 * @brief Main class orchestrating the polarimeter components and operation.
 * Sends data via Serial communication.
 */
class Polarimeter {
private:
  PolarizerServo& polarizerServo; // Reference to the servo object
  LightSensor& lightSensor;       // Reference to the sensor object
  LightSource& lightSource;       // Reference to the light source object

  // Default measurement parameters
  int defaultStartAngle = 0;
  int defaultEndAngle = 180;
  int defaultStepAngle = 1;

  /**
   * @brief Parses the 'run' command arguments.
   */
  bool parseRunCommand(String command, int& start, int& end, int& step) {
      start = defaultStartAngle; // Start with defaults
      end = defaultEndAngle;
      step = defaultStepAngle;

      command.trim();
      int spaceIndex1 = command.indexOf(' ');
      int spaceIndex2 = -1;

      if (command.length() == 0) { // No arguments, use defaults
          return true;
      }

      // Parse first argument (start angle)
      if (spaceIndex1 == -1) { // Only one argument
          start = command.toInt();
      } else {
          start = command.substring(0, spaceIndex1).toInt();
          command = command.substring(spaceIndex1 + 1);
          command.trim();
          spaceIndex2 = command.indexOf(' ');

          // Parse second argument (end angle)
          if (spaceIndex2 == -1) { // Only second argument
              end = command.toInt();
          } else {
              end = command.substring(0, spaceIndex2).toInt();
              command = command.substring(spaceIndex2 + 1);
              command.trim();
              // Parse third argument (step angle)
              if (command.length() > 0) {
                  step = command.toInt();
              }
          }
      }

      // Basic validation
      if (start < 0 || start > 180 || end < 0 || end > 180 || step < 1 || step > 180) {
          Serial.println("Error: Invalid parameters. Angles [0-180], Step [1-180].");
          return false;
      }
       if (start == end) {
           Serial.println("Error: Start and end angles cannot be the same.");
           return false;
       }

      return true;
  }

public:
  /**
   * @brief Constructor for Polarimeter. Updated to remove DataLogger.
   * @param servo Reference to the PolarizerServo object.
   * @param sensor Reference to the LightSensor object.
   * @param led Reference to the LightSource object.
   */
  Polarimeter(PolarizerServo& servo, LightSensor& sensor, LightSource& led)
    : polarizerServo(servo), lightSensor(sensor), lightSource(led) {} 

  /**
   * @brief Initializes all components of the polarimeter. Call this in setup().
   */
  void setup() {
    lightSource.setup();
    lightSensor.setup();
    polarizerServo.attach();
    polarizerServo.home(); // Home servo during setup
    lightSource.turnOn();  // Turn on LED after setup
  }

  /**
   * @brief Processes incoming serial commands.
   */
  void processCommand(String command) {
    command.trim();
    Serial.println("Received command: " + command); // Echo command for clarity

    if (command.startsWith("run")) {
      int start, end, step;
      String args = command.substring(3);
      if (parseRunCommand(args, start, end, step)) {
        runMeasurement(start, end, step);
      }
    } else if (command.startsWith("led")) {
      String arg = command.substring(3);
      arg.trim();
      if (arg.equalsIgnoreCase("on")) {
        lightSource.turnOn();
      } else if (arg.equalsIgnoreCase("off")) {
        lightSource.turnOff();
      } else {
        Serial.println("Invalid LED state. Use 'on' or 'off'.");
      }
    } else if (command.equalsIgnoreCase("home")) {
      polarizerServo.home();
    } else if (command.equalsIgnoreCase("help")) {
      displayHelp();
    } else if (command.length() > 0) { // Only show error for non-empty commands
        Serial.println("Invalid command. Type 'help' for options.");
    }
  }

  /**
   * @brief Runs the polarimeter measurement sequence.
   * @param startAngle The starting angle for the measurement.
   * @param endAngle The ending angle for the measurement.
   * @param stepAngle The angle increment for each step.
   */
  void runMeasurement(int startAngle, int endAngle, int stepAngle) {

    if (!lightSource.isOn()) {
        Serial.println("Warning: LED is off. Turning it on for measurement.");
        lightSource.turnOn();
        delay(100); // Give LED time to stabilize
    }

    // Print informational messages before the actual data stream
    Serial.println("Starting measurement...");
    Serial.println("  Start Angle: " + String(startAngle));
    Serial.println("  End Angle: " + String(endAngle));
    Serial.println("  Step Angle: " + String(stepAngle));

    // Signal the start of data transmission
    Serial.println("---DATA_START---");
    // Print header for the data (useful for parsing on the computer)
    Serial.println("Angle,Intensity");

    int currentAngle = startAngle;
    int direction = (endAngle > startAngle) ? 1 : -1;

    // Ensure the servo starts at the correct position before the loop
    polarizerServo.setPosition(currentAngle, 300);

    while ((direction == 1 && currentAngle <= endAngle) || (direction == -1 && currentAngle >= endAngle)) {
      // Set position (already includes delay)
      polarizerServo.setPosition(currentAngle);

      // Read intensity
      int intensity = lightSensor.readIntensity();

      // Print data to Serial in CSV format
      Serial.print(currentAngle);
      Serial.print(",");
      Serial.println(intensity);

      // Move to the next angle
      currentAngle += (stepAngle * direction);

      // Optional delay between steps if needed for stability or slower data rate
      // delay(10);
    }

    // Signal the end of data transmission
    Serial.println("---DATA_END---");
    Serial.println("Measurement complete.");

    // Optionally home the servo after measurement
    // polarizerServo.home();
  }

  /**
   * @brief Displays help information to the serial monitor.
   * Updated to reflect Serial data output.
   */
  void displayHelp() {
    Serial.println("\n--- Polarimeter Help ---");
    Serial.println("Available commands:");
    Serial.println("  help                 - Display this help message");
    Serial.println("  run [start] [end] [step] - Run measurement. Data sent via Serial.");
    Serial.println("    start: Angle 0-180 (default: " + String(defaultStartAngle) + ")");
    Serial.println("    end:   Angle 0-180 (default: " + String(defaultEndAngle) + ")");
    Serial.println("    step:  Angle 1-180 (default: " + String(defaultStepAngle) + ")");
    Serial.println("    Example: run 10 90 5");
    Serial.println("    Example: run         (uses defaults)");
    Serial.println("  led <on|off>         - Control the LED.");
    Serial.println("    Example: led on");
    Serial.println("  home                 - Move servo to 0 degrees.");
    Serial.println("------------------------");
    Serial.println("Measurement data format (sent via Serial):");
    Serial.println("---DATA_START---");
    Serial.println("Angle,Intensity");
    Serial.println("<angle1>,<intensity1>");
    Serial.println("<angle2>,<intensity2>");
    Serial.println("...");
    Serial.println("---DATA_END---");
    Serial.println("------------------------");
  }
};

// --- Global Objects ---
// Create instances of the component classes
PolarizerServo polarizerServo(SERVO_PIN);
LightSensor    lightSensor(PHOTO_PIN);
LightSource    lightSource(LED_PIN);

// Create the main Polarimeter object, passing references to the components
Polarimeter    polarimeter(polarizerServo, lightSensor, lightSource);

// --- Arduino Standard Functions ---

void setup() {
  Serial.begin(9600); // Or a higher baud rate like 115200 for faster data transfer
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // Delay slightly to ensure Serial Monitor is ready after connection
  delay(500);

  Serial.println("\n--- Polarimeter System Initializing (Serial Output Mode) ---");

  // Initialize the polarimeter and its components
  polarimeter.setup();

  Serial.println("System Ready. Type 'help' for commands.");
}

void loop() {
  // Check if data is available on the serial port
  if (Serial.available() > 0) {
    // Read the incoming command string until newline
    String command = Serial.readStringUntil('\n');
    // Process the command using the Polarimeter object
    polarimeter.processCommand(command);
  }
}
