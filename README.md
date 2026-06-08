Smart Dustbin IoT

Project Overview
Smart Dustbin IoT is an intelligent waste management system developed using ESP32. The system automatically opens the lid when a user approaches, monitors the waste level in real time, sends notifications when the bin is full, and provides remote monitoring through a web dashboard.

Features
- Automatic lid opening using PIR sensor
- Real-time waste level monitoring using ultrasonic sensor
- Servo motor controlled lid operation
- Full bin detection
- Telegram notification when the bin is full
- Firebase real-time database integration
- Web dashboard for monitoring and control
- Data logging to Google Sheets

Hardware Components
- ESP32
- Ultrasonic Sensor (HC-SR04)
- PIR Motion Sensor
- Servo Motor

Technologies Used
- Arduino IDE (C++)
- HTML
- CSS
- JavaScript
- Firebase Realtime Database
- Netlify
- Telegram Bot API
- Google Sheets
- Google Apps Script

System Workflow
1. User approaches the dustbin.
2. PIR sensor detects motion.
3. ESP32 checks the bin level using the ultrasonic sensor.
4. If the bin is not full, the lid opens automatically.
5. Data is uploaded to Firebase.
6. Dashboard displays the latest status.
7. When the bin becomes full:
   - The lid remains closed.
   - A Telegram notification is sent.
   - The event is recorded in Google Sheets.
8. After the bin is emptied, the system resumes normal operation.

Project Outcomes
- Automated waste management process
- Real-time monitoring and notifications
- Improved waste collection efficiency
- Practical implementation of IoT technologies
