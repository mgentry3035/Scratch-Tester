# Automated Scratch Test Rig (Arduino + Nextion Interface)

This project controls an **automated scratch test apparatus** using an **Arduino Mega 2560**, **Nextion HMI display**, **two stepper motors**, and a **servo-actuated pin**.  
It allows manual jog control, automatic test execution, and an emergency stop (E-Stop) via a touchscreen interface.

---

## 🧠 Overview

The system performs a repeatable **scratch test** by:
1. Moving a servo-controlled pin **down onto the surface**.
2. Driving the Y-axis stepper a set **scratch length**.
3. Lifting the pin.
4. Returning the carriage to the start.
5. Incrementally moving the X-axis for the next test line.

Manual control is also available for positioning and homing.

---

## ⚙️ Hardware Setup

| Component | Description | Pin Connections (Arduino Mega) |
|------------|-------------|-------------------------------|
| **Stepper X** | Moves the test carriage laterally | STEP → Pin 2, DIR → Pin 5 |
| **Stepper Y** | Moves the pin carriage forward/backward | STEP → Pin 3, DIR → Pin 6 |
| **Stepper Enable Pin** | Global enable for drivers | Pin 8 |
| **Servo (Pin Control)** | Raises/lowers scratch pin | Pin 16 |
| **Limit Switches** | Define axis limits | X Stop → Pin 9, Y Stop → Pin 10 |
| **Nextion Display** | User interface | TX → Arduino RX3, RX → Arduino TX3 |
| **Manual Limit Switches** | (Optional) movement enable buttons | Left (40), Right (39), Forward (38), Back (41) |

> ⚠️ **Important:** All limit switches are configured with `INPUT_PULLUP`.  
> They read **HIGH when not pressed** and **LOW when pressed**.

---

## 🧩 Libraries Used

Make sure you have these Arduino libraries installed:

- [**Nextion Library**](https://github.com/itead/ITEADLIB_Arduino_Nextion)
- [**AccelStepper**](https://www.airspayce.com/mikem/arduino/AccelStepper/)
- [**Servo**](https://www.arduino.cc/en/Reference/Servo)
- [**ezButton**](https://github.com/ArduinoGetStarted/ezButton)

You can install them through **Arduino IDE → Tools → Manage Libraries**.

---

## 💻 Code Functionality

### 1. **Manual Control**
Buttons on the Nextion screen (Forward, Back, Left, Right) move the X and Y axes for positioning.  
Movement is continuous while the button is pressed and stops when released.

### 2. **Servo Control**
- **Pin Down Button**: Lowers the pin.
- **Release**: Raises the pin.

### 3. **Home Function (optional)**
Moves both axes until they hit the limit switches, then sets the origin `(0, 0)`.

### 4. **Automated Scratch Test**
Triggered by pressing **Start**:
1. Reads scratch length from Nextion slider.
2. Lowers the pin.
3. Moves Y-axis the specified distance.
4. Lifts the pin.
5. Returns to the start.
6. Moves X-axis by `finalXDIST` for the next test.
7. Returns to main page.

If the **Stop** button is pressed:
- Motion halts immediately.
- Pin is lifted.
- Test aborts safely.

---

## 🧠 Key Adjustable Parameters

| Variable | Default | Description |
|-----------|----------|-------------|
| `xStepsPerMm` | 8 | Step resolution of X-axis motor |
| `yStepsPerMm` | 40 | Step resolution of Y-axis motor |
| `scratchSpacing` | 10 mm | X-axis distance between tests |
| `initTestSPD` | -960 | Forward test speed (negative = direction) |
| `retTestSPD` | 1000 | Return speed |
| `startXSPD` | 500 | X-axis shift speed |
| `finalXDIST` | 80 | X-axis distance after test |
| `scratchLength` | 50 | Default scratch distance (mm) |

Modify these in the source file to tune behavior.

---

## 🧰 Setup Instructions

1. **Wire all components** according to the pin table above.
2. **Upload the code** to your Arduino Mega 2560.
3. **Load the Nextion `.HMI` file** (if provided) to your display.
4. **Power the setup** (ensure your stepper drivers are properly powered and grounded).
5. **Open Serial Monitor** at `115200` baud to view debug messages.
6. **Use the Nextion display** to jog motors, home axes, and start tests.

---

## 🔍 Debugging Notes

- If steppers move in the **wrong direction**, flip the sign of `xPosAdjust`/`yPosAdjust` or use `stepperX.setPinsInverted(true, false, false)`.
- If the display doesn’t respond, verify **Serial3 wiring and baud rate (9600)**.
- If a limit switch immediately halts motion, ensure **it’s normally open** and **wired to GND**.

---

## 🧯 Safety / E-Stop

The **Stop button** on the Nextion immediately:
- Lifts the pin (servo).
- Stops all motion.
- Returns to the main page.

The code also checks for physical limit-switch triggers to prevent overtravel.

---

## 🧩 Future Improvements

- Add non-blocking stepper movement (state machine instead of while loops)
- Implement automatic homing sequence on startup
- Save configuration parameters via EEPROM
- Add test logging via SD card

---

## 📄 License

This code is provided for educational and laboratory automation purposes.  
Modify freely for personal, academic, or internal research use.

---

## 👤 Author

**Michael Gentry**  
Developed for an Arduino Mega-based automation platform using Nextion HMI control.  
Includes contributions to servo control, motion coordination, and user interface logic.

---
