/**
 * @file wifiSerialBridge.ino
 * 
 * @date   2015-06-10
 * @author Pascal Gollor (http://www.pgollor.de/cms/)
 * 
 * @brief Wifi Serial Bride
 */


#include <ESP8266WiFi.h>
#include <WiFiServer.h>
#include <EEPROM.h>
#include <WiFiUdp.h>

/**
 * @brief Connection definitions.
 * @{
 */
#define UDP_PORT 9999 ///< Listening for broadcast messages on this UDP port.
#define TCP_PORT 9999 ///< Listening for UART data on this TCP port.

#define MULTICAST_IP   "239.0.0.57" ///< Multicast IP for transmission as string.
#define MULTICAST_PORT 12345 ///< Multicast port for transmission.
IPAddress ipMulti(239, 0, 0, 57); ///< Multicast IP for transmission as tuple.
/// @}


/**
 * @brief Use this if no information can be found in the EEprom.
 * @{
 */
#define SOFTAP_SSID (char*)"esp8266_REST" ///< Software AP SSID
#define SOFTAP_PASS (char*)"esp8266esp8266" ///< Software AP password
/// @}

/// Serial input buffer size. ESP8266 has 256 byte Hardware buffer
#define SERIAL_INPUT_BUFFER_SIZE 256

/**
 * @brief verbose mode
 * 
 * - 0: none output
 * - 1: some output
 * - 2: some more output
 * - 3: many more output
 */
#define DEBUG 1

/**
 * @brief Wait for on complete line with new line character(s) at the end.
 * 
 * 0: No
 * 1: Yes
 */
#define UART_RECEIVE_HOLE_LINES 0

/**
 * @brief UDP protocol
 *
 * All strings without PROTO_HELLO have to be PROTO_START at first.
 * @{
 */

/**
 * @brief Hello string for UDP broadcasr request.
 *
 * If this string is received, the controller is going to transmit a response with connection information.
 */
#define PROTO_HELLO "wifi-serial"

#define PROTO_START "+++esp-" ///< Start String for all commands.
#define PROTO_START_LENGTH 7 ///< Length of PROTO_START string.

#define PROTO_RESET "reset" ///< Reset the controller

#define PROTO_SET_SSID "set-ssid" ///< Set AP SSID in client mode.
#define PROTO_SET_SSID_LENGTH 8 ///< Length of PROTO_SET_SSID string.

#define PROTO_SET_PASS "set-pass" ///< Set password for AP in client mode.
#define PROTO_SET_PASS_LENGTH 8 ///< Length of PROTO_SET_PASS string.
/// @}


/// macros
#define print_millis Serial.print('['); Serial.print(millis()); Serial.print("]: ")



/*
 * ---------- variables ---------
 * ---------------\/-------------
 */

String g_inputString = "";

/// wifi connection type
int8_t g_connectionType = -1;

/// broadcast server
WiFiUDP g_udp;

/// tcp server
WiFiServer g_server(TCP_PORT);

/// global package buffer
char packetBuffer[255];

/// SSID to connect to.
String g_esid;

/// Wifi passwort which used for connection.
String g_epass;



/*
 * ---------- functions ---------
 * ---------------\/-------------
 */


/**
 * @brief Test the wifi client conenction.
 * @return uint8_t 10: fail - 20: succesfull
 */
uint8_t testWifi(void)
{
#if DEBUG >= 1
	Serial.println("Waiting for Wifi to connect");
#endif

	for (uint8_t i = 0; i < 20; i++)
	{
		if (WiFi.status() == WL_CONNECTED)
		{
#if DEBUG >= 1
			Serial.println("");
			Serial.println("WiFi connected with client IP:");
			Serial.println(WiFi.localIP());
#endif

			return(20);
		}

		delay(500);

#if DEBUG >= 1
		Serial.print(WiFi.status());
#endif
	}

	//#if DEBUG >= 1
	Serial.println("Connect timed out, opening AP");
	//#endif

	return(10);
}


/**
 * @brief Write esid and epass to EEprom.
 */
static inline void writeWifiEEprom(void)
{
#if DEBUG >= 1
  Serial.println("clearing eeprom");
#endif

  for (uint8_t i = 0; i < 96; i++)
  {
    EEPROM.write(i, 0);
  }

#if DEBUG >= 1
  Serial.println("Writing ssid to eeprom.");
  Serial.println(g_esid.length());
#endif

  // set ssid
  for (uint8_t i = 0; i < g_esid.length(); i++)
  {
    EEPROM.write(i, g_esid[i]);
#if DEBUG >= 2
    Serial.print("wrote: ");
    Serial.println(g_esid[i]);
#endif
  }

#if DEBUG >= 1
  Serial.println("Writing pass to eeprom.");
  Serial.println(g_epass.length());
#endif

  // set password
  for (uint8_t i = 0; i < g_epass.length(); i++)
  {
    EEPROM.write(i + 32, g_epass[i]);
#if DEBUG >= 2
    Serial.print("wrote: ");
    Serial.println(g_epass[i]);
#endif
  }

  EEPROM.commit();
  EEPROM.end();
} // writeWifiEEprom


/**
 * @brief Inline function to setup an Accesspoint.
 */
static inline void setupAP(void)
{
	g_connectionType = WIFI_STA;
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();

	delay(100);

	g_connectionType = WIFI_AP;
	WiFi.mode(WIFI_AP);
	WiFi.softAP(SOFTAP_SSID, SOFTAP_PASS);


	// show information
	Serial.print("SSID: ");
	Serial.println(SOFTAP_SSID);

	Serial.print("Password: ");
	Serial.println(SOFTAP_PASS);

	Serial.print("IP: ");
	Serial.println(WiFi.softAPIP());
}


/**
 * @brief Inline funciton to read wifi connection information from EEprom.
 */
static inline void readWifiEEprom(void)
{
	String esid;
	String epass;

	g_connectionType = WIFI_STA;
	WiFi.mode(WIFI_STA);


	EEPROM.begin(512);

#if DEBUG >= 1
	Serial.println("Reading ssid from EEprom");
#endif

	esid.reserve(32);
	for (uint8_t i = 0; i < 32; i++)
	{
		esid += char(EEPROM.read(i));

#if DEBUG >= 1
		if (i % 5 == 0)
		{
			Serial.print('.');
		}
#endif
	}

	esid.trim();

#if DEBUG >= 1
	Serial.println();
	Serial.print("SSID: ");
	Serial.println(esid.c_str());
	Serial.println("");
	Serial.println("Read password from EEprom");
#endif

	epass.reserve(64);
	for (uint8_t i = 32; i < 96; i++)
	{
		epass += char(EEPROM.read(i));

#if DEBUG >= 1
		if (i % 5 == 0)
		{
			Serial.print('.');
		}
#endif
	}

	epass.trim();

#if DEBUG >= 1
	Serial.println();
	Serial.print("password: ");
	Serial.println(epass.c_str());
	Serial.println("");
#endif

	if ( esid.length() > 1 )
	{
		// test esid
		WiFi.begin(esid.c_str(), epass.c_str());
		if ( testWifi() == 20 )
		{
			g_esid = esid.c_str();
			g_epass = epass.c_str();
		}
		else
		{
			// create AP to set connection information
			setupAP();

			g_esid = "";
			g_epass = "";
		}
	}
}


/// handle udp request
static inline void handle_udp_broad_req(void)
{
	// First we check if an UDP package comes over broadcast connection.
	int16_t packetSize = g_udp.parsePacket();

	if (packetSize == 0)
	{
		return;
	}

#if DEBUG >= 3
	Serial.print("packageSize: ");
	Serial.println(packetSize);
#endif

	// get data from client
	int16_t len = g_udp.read(packetBuffer, 255);
	if (len <= 0)
	{
		return;
	}

	// add end for string
	packetBuffer[len] = 0;
	String buffStr(packetBuffer);
#if DEBUG >= 2
	print_millis;
	Serial.print("received: ");
	Serial.println(buffStr);
#endif

	// check if string is for me?
	if (buffStr.startsWith(PROTO_HELLO))
	{
		IPAddress ip = WiFi.localIP();
		if (g_connectionType == WIFI_AP)
		{
			ip = WiFi.softAPIP();
		}

		// send information back to the client
		g_udp.beginPacket(g_udp.remoteIP(), g_udp.remotePort());
		g_udp.print(ip);
		g_udp.write(':');
		g_udp.print(TCP_PORT);
		g_udp.write(':');
		g_udp.print(MULTICAST_IP);
		g_udp.write(':');
		g_udp.print(MULTICAST_PORT);
		g_udp.write("\r\n");
		g_udp.endPacket();
	}
}


/// handle tcp request
static inline void handle_tcp_req()
{
	WiFiClient client = g_server.available();

	// return if no client is connected
	if (!client)
	{
		return;
	}

	// wait for data from client
#if DEBUG >= 2
	Serial.println("New connection. Wait for data.");
#endif
	while(!client.available())
	{
		delay(1);
	}

	// get first line
	String req = client.readStringUntil('\n');
	if (!req.endsWith("\r"))
	{
		return;
	}
	client.flush();

	// Some debug output.
#if DEBUG >= 2
	Serial.print("Date Received From: ");
	Serial.print(client.remoteIP());
	Serial.print(":");
	Serial.println(client.remotePort());
	Serial.print("data: ");
	Serial.println(req);
#endif


	// Check if string is an command for me?
	if (req.startsWith(PROTO_START))
	{
		req = req.substring(PROTO_START_LENGTH);
		req.trim();

		// ... Reset command
		if(req.startsWith(PROTO_RESET))
		{
#if DEBUG >= 1
			Serial.println("reset esp8266");
#endif
		}
		// ... set SSID
		else if(req.startsWith(PROTO_SET_SSID))
		{
			req = req.substring(PROTO_SET_SSID_LENGTH + 1);

#if DEBUG >= 1
			Serial.print("Set ssid to: ");
			Serial.println(req);
#endif

			g_esid = req;
		}
		// ... set password for SSID
		else if(req.startsWith(PROTO_SET_PASS))
		{
			req = req.substring(PROTO_SET_PASS_LENGTH + 1);

#if DEBUG >= 1
			Serial.print("Set passwortd to: ");
			Serial.println(req);
#endif

			g_epass = req;
			writeWifiEEprom();
		}
	}
	else
	{
		req += '\n';

		// Transmit data over serial connection
		Serial.print(req);
	}

	// Send data back to client.
	client.print(req);
} // handle_tcp_req


/**
 * @brief Transmit data as Multicast packet.
 * @param buff String buffer pointer.
 */
void multi_transmit(String* buff)
{
#if DEBUG >= 3
	Serial.print("Transmit ");
	Serial.print(buff->length());
	Serial.println(" byte.");
#endif

	if (g_connectionType == WIFI_AP)
	{
		g_udp.beginPacketMulticast(ipMulti, MULTICAST_PORT, WiFi.softAPIP(), 1);
	}
	else
	{
		g_udp.beginPacketMulticast(ipMulti, MULTICAST_PORT, WiFi.localIP(), 1);
	}

	g_udp.print(*buff);
	g_udp.endPacket();

	// clear buffer
	*buff = "";
} // multi_transmit



/// handle UART request
static inline void handle_uart_req()
{
	uint16_t counter = 0;

	// return if no data available
	if (Serial.available() == 0)
	{
		return;
	}

#if DEBUG >= 3
	Serial.print("Serial data available: ");
	Serial.println(Serial.available());
#endif

	// check for serial data
	while (Serial.available())
	{
		counter++;
		g_inputString += (char)Serial.read();

#if DEBUG >= 4
		Serial.println(in);
#endif

#if UART_RECEIVE_HOLE_LINES == 1
		if (in == '\n')
		{
			break;
		}
#endif // UART_RECEIVE_HOLE_LINES == 1

		if (counter == SERIAL_INPUT_BUFFER_SIZE)
		{
			multi_transmit(&g_inputString);
			counter = 0;
			break;
		}

		//delay(1);
	}

#if UART_RECEIVE_HOLE_LINES == 0
	if (counter > 0)
	{
		multi_transmit(&g_inputString);
	}
#else // UART_RECEIVE_HOLE_LINES == 0

	// Check new line characters.
	if (g_inputString != "" && g_inputString.endsWith("\r\n"))
	{
#if DEBUG >= 1
		print_millis;
		Serial.print("got from uart: ");
		Serial.println(g_inputString);
#endif

		// transmit
		uart_transmit(&g_inputString);
	}
#endif // UART_RECEIVE_HOLE_LINES == 0
} // handle_uart_req


/// main setup
void setup()
{
	g_inputString.reserve(SERIAL_INPUT_BUFFER_SIZE);

	// initialize serial connection
	Serial.begin(115200);
	Serial.setDebugOutput(false);
	Serial.println("\r\n\r\n");

	delay(10);

	// use EEprom Start address
	EEPROM.begin(512);

	// reserve space
	g_esid.reserve(32);
	g_epass.reserve(64);

	// try to read ssid and passwort from eeprom
	readWifiEEprom();


	g_udp.begin(UDP_PORT);
#if DEBUG >= 2
	Serial.println("Udp server for broadcast information started.");
#endif

	g_server.begin();
#if DEBUG >= 2
	Serial.println("Tcp server started.");
#endif


#if DEBUG >= 3
	Serial.print("rx enabled for uart: ");
	Serial.println(Serial.isRxEnabled());

	Serial.print("tx enabled for uart: ");
	Serial.println(Serial.isTxEnabled());

	Serial.println("-- WifI info --");
	WiFi.printDiag(Serial);
	Serial.println("-- WifI info --");
#endif
}


/// main loop
void loop()
{
	// handle udp request
	handle_udp_broad_req();

	// time for internal wifi management
	delay(10);

	// handle tcp request
	handle_tcp_req();

	// time for internal wifi management
	delay(10);

	// handle uart request
	handle_uart_req();

	delay(10);
}

