# Wi-Fi Controlled Smart Surveillance Robot

## Files
- `ESP32_CAM_Robot.ino` — ESP32-CAM Wi-Fi, camera, web server and L298N motor control.
- `webpage.h` — separate HTML/CSS/JavaScript interface served by the ESP32-CAM.
- `Arduino_Uno_Obstacle.ino` — independent HC-SR04 obstacle warning using one shared D6 output for LED and buzzer.

## ESP32-CAM setup
1. Open `ESP32_CAM_Robot.ino` in Arduino IDE.
2. Keep `webpage.h` in the same sketch folder as the `.ino` file.
3. Replace `YOUR_WIFI_NAME` and `YOUR_WIFI_PASSWORD` with your 2.4 GHz Wi-Fi credentials.
4. Select the correct ESP32-CAM board and enable PSRAM if your board package exposes that option.
5. Upload the sketch. Open Serial Monitor at 115200 baud.
6. Open the printed webpage IP address in a browser on the same Wi-Fi network.

## ESP32-CAM to L298N
| ESP32-CAM | L298N |
|---|---|
| GPIO12 | IN1 |
| GPIO13 | IN2 |
| GPIO14 | IN3 |
| GPIO15 | IN4 |
| GND | GND |

- Leave ENA and ENB jumpers installed for full-speed control.
- Connect the motors to OUT1/OUT2 and OUT3/OUT4.
- Power the motors from a suitable external motor supply.
- Join the external supply negative, L298N GND and ESP32-CAM GND.
- Do not power the motors from the ESP32-CAM 3.3V pin.

## Arduino Uno obstacle warning
| Component | Uno pin |
|---|---|
| HC-SR04 TRIG | D9 |
| HC-SR04 ECHO | D10 |
| Buzzer positive | D6 |
| LED anode through 220 ohm resistor | D6 |
| Buzzer negative and LED cathode | GND |

Upload `Arduino_Uno_Obstacle.ino` to the Uno. The LED and buzzer turn on together when the measured distance is 20 cm or less.

**Important:** The Uno is independent. It does not communicate with the ESP32-CAM and does not automatically stop the robot motors.

## Important compatibility and safety notes
- This camera pin map is for the AI Thinker ESP32-CAM style board. Confirm your actual board and camera wiring before uploading.
- The sketch uses `pin_sccb_sda` and `pin_sccb_scl`, as used by newer ESP32 Arduino camera configurations. If your installed ESP32 core reports these members do not exist, the core may use the older names `pin_sscb_sda` and `pin_sscb_scl`; change only those two field names.
- GPIO12 and GPIO15 are boot-strapping pins on ESP32. If boot/upload problems occur, temporarily disconnect the L298N input wires from those pins during upload and startup.
- The combined buzzer and LED load on Uno D6 must stay within the Arduino pin's current limit. If the buzzer draws too much current, use a transistor driver; do not drive a high-current buzzer directly from D6.
- Test the robot with its wheels lifted first. Keep hands clear of moving parts.
- Wi-Fi control works only when the ESP32-CAM and the phone/computer are on the same reachable network.
