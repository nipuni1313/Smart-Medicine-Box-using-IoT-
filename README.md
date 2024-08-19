# Medibox IoT Project

Welcome to the Medibox IoT project repository. This project is designed to help users manage their medication schedules effectively using IoT technology.

## Table of Contents
- [Introduction](#introduction)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [License](#license)

## Introduction
Medibox is an IoT-enabled device that helps users manage their medication schedules, monitor environmental conditions, and ensure the proper storage of medicines.

## Features
- Real-time light intensity monitoring using LDR sensors.
- Dynamic light adjustment with a servo motor to protect light-sensitive medicines.
- Customizable settings for different types of medicines via a user-friendly Node-RED dashboard.

## Installation
To install and run this project, you need to have the following tools installed:
- Arduino IDE
- Node-RED
- MQTT broker (e.g., test.mosquitto.org)

### Steps:
1. Clone the repository: `git clone https://github.com/yourusername/medibox.git`
2. Install the necessary libraries: 
   - Install the ESP32 board package in Arduino IDE.
   - Install the required Node-RED nodes.
3. Upload the code to the ESP32 and deploy the Node-RED flow.

## Usage
Once the setup is complete, you can use the Node-RED dashboard to:
- Monitor light intensity and adjust the servo motor position.
- Set medicine-specific parameters to ensure proper storage conditions.
- Receive alerts and notifications.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.
