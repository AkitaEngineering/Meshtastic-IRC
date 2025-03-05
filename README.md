# Meshtastic IRC Bridge

This project provides a bridge between the Meshtastic mesh network and an IRC (Internet Relay Chat) server. It allows users to send and receive messages between Meshtastic devices and an IRC channel.

## Features

* **Bi-directional Communication:** Sends messages from Meshtastic to IRC and from IRC to Meshtastic.
* **Configuration File:** Loads WiFi and IRC settings from a JSON file stored on SPIFFS.
* **Robust Connection Handling:** Implements WiFi reconnection and IRC ping/pong to maintain stable connections.
* **Message Queues:** Uses message queues to prevent message loss during network interruptions.
* **Rate Limiting:** Implements rate limiting for IRC messages to avoid server bans.
* **Error Handling:** Includes basic error handling for file operations, WiFi, and IRC connections.
* **Default Configuration:** Creates a default configuration file if one is not found.

## Requirements

* Lolin D1 Mini (ESP8266) or compatible board
* Meshtastic compatible device
* Arduino IDE with ESP8266 board support
* Required Arduino Libraries:
    * `Meshtastic`
    * `IRCClient`
    * `ArduinoJson`

## Installation

1.  **Install Arduino IDE and ESP8266 Board Support:**
    * If you haven't already, install the Arduino IDE and add support for the ESP8266 board.
2.  **Install Required Libraries:**
    * Open the Arduino IDE Library Manager (Sketch -> Include Library -> Manage Libraries...).
    * Search for and install the following libraries:
        * `Meshtastic`
        * `IRCClient`
        * `ArduinoJson`
3.  **Download the Code:**
    * Download or clone this repository to your computer.
4.  **Configure WiFi and IRC Settings:**
    * In the Arduino IDE, open the `Meshtastic_IRC_Bridge.ino` file.
    * Create a `config.json` file in the `data` folder of your Arduino project directory. The contents of the file should be similar to the following, replacing the placeholder values with your actual settings:

    ```json
    {
      "ssid": "YOUR_WIFI_SSID",
      "password": "YOUR_WIFI_PASSWORD",
      "ircServer": "irc.akita.chat",
      "ircPort": 6667,
      "ircNickname": "MeshtasticBot",
      "ircUsername": "MeshtasticBot",
      "ircRealname": "Meshtastic IRC Bridge",
      "ircChannel": "#meshtastic"
    }
    ```

    * Alternatively, you can upload the sketch without the config file. It will create a default file, which you can then edit on the device's SPIFFS.
5.  **Upload the Code and Configuration:**
    * Connect your Lolin D1 Mini to your computer.
    * In the Arduino IDE, select your board and port.
    * Go to Tools -> ESP8266 Sketch Data Upload and upload the `config.json` file to SPIFFS.
    * Upload the `Meshtastic_IRC_Bridge.ino` sketch to your board.
6.  **Monitor Serial Output:**
    * Open the Serial Monitor in the Arduino IDE to monitor the connection status and messages.

## Usage

* Once the device is connected to WiFi and IRC, messages sent to the specified IRC channel will be forwarded to the Meshtastic network, and messages sent from Meshtastic devices will be forwarded to the IRC channel.
* The messages will be formatted to indicate their origin (\[IRC] or \[Meshtastic]).

## Future Improvements

* Web-based configuration interface.
* OTA (Over-the-Air) updates.
* More robust error handling and logging.
* Message filtering and formatting options.
* Support for multiple IRC channels.
* Implement security features like credential encryption.
* Add a visual status indicator using an LED.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues to suggest improvements or report bugs.

## License

This project is licensed under the MIT License.
