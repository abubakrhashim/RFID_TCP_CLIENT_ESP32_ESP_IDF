ESP32 RFID-Based Authentication with TCP Communication
This project implements an ESP32-based TCP server with RFID authentication using the RC522 RFID reader. The ESP32 connects to a Wi-Fi network, allowing a client to send user credentials (name and password) that are written to an RFID MIFARE card. These cards can later be used for authentication by reading and verifying the stored credentials.

Key Features
Wi-Fi TCP Server: The ESP32 establishes a TCP connection, enabling a client (e.g., a computer) to send user data remotely.
RFID Authentication: The ESP32 communicates with the MFRC522 RFID reader/writer to manage usernames and passwords stored on MIFARE cards.
User Enrollment: Clients can send user details (name, password) via the TCP connection, and these details are written to the RFID card.
Authentication Feedback: Green LED lights up on successful authentication, while a red LED indicates a failure.
Keep-Alive Mechanism: Ensures stable TCP communication between the client and server.
Components
ESP32: Serves as the Wi-Fi server and manages all RFID operations.
MFRC522 RFID Reader: Used to read and write data to/from MIFARE RFID cards.
LEDs: Visual indicators for authentication status (Green for success, Red for failure).
Wi-Fi: ESP32 connects to the local network, allowing remote client communication.
How It Works
TCP Communication:
The ESP32 sets up a TCP server and listens for incoming client connections. A client (e.g., a computer) connects to the ESP32 using the server's IP address and sends user credentials (name and password) via the TCP connection.

RFID Enrollment:
When a client sends credentials, the ESP32 writes them to a MIFARE RFID card via the MFRC522 reader. The card stores the username and password securely.

Authentication:
The system continuously checks for an RFID card presented to the reader. When a card is detected, the ESP32 reads the stored credentials from the card and compares the password with the one provided by the client.

LED Indicators:

Green LED: Authentication success, the user is verified.
Red LED: Authentication failure, credentials do not match.
Keep-Alive Mechanism:
A keep-alive system ensures that the TCP communication between the ESP32 server and client remains stable during long sessions.

Setup Instructions
1. Hardware
ESP32 Development Board
MFRC522 RFID Reader
MIFARE RFID Cards
LEDs (Green and Red)
Resistors for LEDs
Jumper wires and breadboard
2. Software
ESP-IDF (ESP32 Development Framework)
Code editor (e.g., VSCode)
MFRC522 Library for RFID (SPI-based)
Wi-Fi credentials setup for ESP32
3. Wiring
Connect the MFRC522 RFID Reader to ESP32 as follows:
SDA → GPIO 21
SCK → GPIO 18
MOSI → GPIO 23
MISO → GPIO 19
IRQ → Not connected
GND → GND
RST → GPIO 22
3.3V → 3.3V
Connect the Green and Red LEDs to GPIO pins on the ESP32 with appropriate resistors.
4. Build and Flash
Clone this repository.
Set up the ESP32 development environment with ESP-IDF.
Compile and flash the code to the ESP32 using idf.py build and idf.py flash.
5. Usage
Power on the ESP32 and connect it to a Wi-Fi network.
Open a TCP client (e.g., Telnet or a custom client) on your computer.
Connect to the ESP32 using its IP address and the designated port.
Send user credentials to enroll them on the RFID card.
Present the RFID card to the reader for authentication.
Check the LED status for verification.
Future Improvements
Add support for multiple RFID cards.
Implement a web-based interface for easier user management.
Add encryption for secure communication between client and server.
License
This project is licensed under the MIT License - see the LICENSE file for details.
