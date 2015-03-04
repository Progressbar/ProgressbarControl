/*
 Progressbar Control panel
 Controls devices in Progressbar Hackerspace
 Created by M. Habovštiak
 Thanks to David A. Mellis for providing Webserver example
 */

#include <SPI.h>
#include <Ethernet.h>
#include <solight.h>
#include <avr/pgmspace.h>

// ======== Pins ========
Solight solight(9);

// Lights
const int mainLightFront = 8;
const int mainLightBack = 7;
const int labLightLeft = 6;
const int labLightRight = 5;

// Switches
const int mainSwitch = A0;
const int labSwitchLeft = A1;
const int labSwitchRight = A2;

// ======== TCP/IP ========
// Addresses
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,223,24);

// Port
EthernetServer server(80);

// ======== Code ========

typedef void (*StaticSender)(EthernetClient &);
typedef void (*ParamSender)(EthernetClient &, const char *);

// Holds information about static resource
class StaticResource {
	public:
		// Constructor just stores the pointers
		StaticResource(const char *path, StaticSender rs) :
			mPath(path),
			mSender(rs) {
		}

		// Checks whether path matches and calls the StaticSender if it does
		bool match(const char *path, EthernetClient &client) {
			if(strcmp(path, mPath)) return false;
			mSender(client);
			return true;
		}
	private:
		const char *mPath;
		StaticSender mSender;
};

// Holds information about parametrized resource
class ParamResource {
	public:
		// Constructor just stores the pointers
		ParamResource(const char *prefix, ParamSender rs) :
			mPrefix(prefix),
			mSender(rs) {
		}

		// Checks whether path matches and calls the ParamSender if it does
		bool match(const char *path, EthernetClient &client) {
			size_t i = 0;
			for(; mPrefix[i] && path[i]; ++i) {
				if(mPrefix[i] != path[i])
					return false;
			}

			if(mPrefix[i]) return false;

			mSender(client, path + i);
			return true;
		}
	private:
		const char *mPrefix;
		ParamSender mSender;
};

// Maps pin number to string
class PinMap {
	public:
		PinMap(const char *prefix, int pin) :
			mPrefix(prefix),
			mPin(pin) {
		}

		// If prefix matches, fills pin and tail variables and returns true
		bool match(const char *path, int &pin, const char *&tail) {
			size_t i = 0;
			for(; mPrefix[i] && path[i]; ++i) {
				if(mPrefix[i] != path[i])
					return false;
			}

			if(mPrefix[i]) return false;

			pin = mPin;
			tail = path + i;
			return true;
		}
	private:
		const char *mPrefix;
		int mPin;
};

// Timer variables
unsigned long countdownStart;
unsigned long countdownMillis;
bool countdownTimer = false;

// Last states of the switches
bool lastMain;
bool lastLabLeft;
bool lastLabRight;

// Standard Arduino setup function
void setup()
{
	// Setup pins
	pinMode(mainLightFront, OUTPUT);
	pinMode(mainLightBack, OUTPUT);
	pinMode(labLightLeft, OUTPUT);
	pinMode(labLightRight, OUTPUT);

	// Input pins specified for clarity
	pinMode(mainSwitch, INPUT);
	pinMode(labSwitchLeft, INPUT);
	pinMode(labSwitchRight, INPUT);

	// Setup pullup resistors
	digitalWrite(mainSwitch, HIGH);
	digitalWrite(labSwitchLeft, HIGH);
	digitalWrite(labSwitchRight, HIGH);

	// Let voltage stabilize
	delay(100);

	// Read current state
	lastMain = digitalRead(mainSwitch);
	lastLabLeft = digitalRead(labSwitchLeft);
	lastLabRight = digitalRead(labSwitchRight);

	// Setup server
	Ethernet.begin(mac, ip);

	// Setup debugging
	server.begin();
#ifdef DEBUG_ENABLE
	Serial.begin(9600);
	Serial.println("Server initialized.");
#endif
}

// Sends basic HTTP header
void sendHeader(EthernetClient &client, const char *response = "200 OK") {
	client.print("HTTP/1.1 ");
	client.println(response);
	client.println("Content-Type: text/html");
	client.println("Connection: close");
	client.println();
}

// Sends 307 (temporary) redirect to /
void redirect(EthernetClient &client) {
	client.println("HTTP/1.1 307 Temporary Redirect");
	client.println("Connection: close");
	client.println("Location: /");
	client.println();
}

// Sends Error 400 response
void send400(EthernetClient &client) {
	sendHeader(client, "400 Bad request");

	client.println("Bad request");
}

// Sends Error 404 response
void send404(EthernetClient &client) {
	sendHeader(client, "404 Not found");

	client.println("Not found");
}

// Reads string from PROGMEM and sends to client
void sendPMString(EthernetClient &client, uint_farptr_t str) {
	char buf[129];
	buf[128] = 0;
	size_t slen = strlen_PF(str), pos = 0;
	while(pos < slen) {
		strncpy_PF(buf, str + pos, 128);
		client.print(buf);
		pos += 128;
	}
}

// Sends main page
void sendIndex(EthernetClient &client) {
	sendHeader(client);

	// The page is stored in PROGMEM instead of RAM, because it wouldn't fit to RAM
	static const char index[] PROGMEM = {
		"<html>"
			"<head>"
				"<title>Progressbar control panel</title>"
			"</head>"
			"<body>"
			"<h1>Progressbar control panel</h1>"
			"<h2>Lights</h2>"
			"Front <a href=\"lights/main/front/On\">on</a>|<a href=\"lights/main/front/Off\">off</a><br>"
			"Back <a href=\"lights/main/back/On\">on</a>|<a href=\"lights/main/back/Off\">off</a><br>"
			"Left <a href=\"lights/lab/left/On\">on</a>|<a href=\"lights/lab/left/Off\">off</a><br>"
			"Right <a href=\"lights/lab/right/On\">on</a>|<a href=\"lights/lab/right/Off\">off</a><br>"
			"<h2>Sockets</h2>"
			"A <a href=\"sockets/A/On\">on</a>|<a href=\"sockets/A/Off\">off</a><br>"
			"B <a href=\"sockets/B/On\">on</a>|<a href=\"sockets/B/Off\">off</a><br>"
			"C <a href=\"sockets/C/On\">on</a>|<a href=\"sockets/C/Off\">off</a><br>"
			"D <a href=\"sockets/D/On\">on</a>|<a href=\"sockets/D/Off\">off</a><br>"
			"E <a href=\"sockets/E/On\">on</a>|<a href=\"sockets/E/Off\">off</a><br>"
			"<h2>The red button</h2>"
			"Shutdown the Universe <a href=\"everythingOff\">now</a> | <a href=\"everythingOff/60\">after one minute</a> | <a href=\"everythingOff/300\">after five minutes</a><br>"
			"</body>"
		"</html>"
	};

	sendPMString(client, (uint_farptr_t)index);
}

// Parses int in str, returns last position (0 means invalid char)
size_t parseInt(const char *str, uint16_t &num) {
	size_t i = 0;
	num = 0;

	for(; str[i] != '/' && str[i] != 0; ++i) {
		if(str[i] < '0' || str[i] > '9') {
			return 0;
		}
		num *= 10;
		num += str[i] - '0';
	}

	return i;
}

// Kill switch
void turnEverythingOff() {
	digitalWrite(mainLightFront, HIGH);
	digitalWrite(mainLightFront, LOW);
	digitalWrite(mainLightBack, HIGH);
	digitalWrite(mainLightBack, LOW);
	digitalWrite(labLightLeft, HIGH);
	digitalWrite(labLightLeft, LOW);
	digitalWrite(labLightRight, HIGH);
	digitalWrite(labLightRight, LOW);
	solight.control(31, 'A', 0);
	solight.control(31, 'B', 0);
	solight.control(31, 'C', 0);
	solight.control(31, 'D', 0);
	solight.control(31, 'E', 0);
}

void everythingOff(EthernetClient &client) {
	turnEverythingOff();
	redirect(client);
}

void everythingOffTimer(EthernetClient &client, const char *param) {
	uint16_t seconds;
	size_t i = parseInt(param, seconds);

	if(!i || param[i]) {
		send404(client);
		return;
	}

	countdownStart = millis();
	countdownMillis = seconds * 1000;
	countdownTimer = true;

	static const char head[] PROGMEM = {
		"<html>"
			"<head>"
				"<title>Countdown</title>"
			"</head>"
			"<body>"
				"<h1>The Universe will shut down in <span id=\"secs\"></span> seconds</h1>"
				"<script type=\"text/javascript\">"
	};
	static const char tail[] PROGMEM = {
					"function tmr() {"
						"if(secs) --secs;"
						"document.getElementById(\"secs\").innerHTML = secs;"
					"}"

					"document.getElementById(\"secs\").innerHTML = secs;"
					"window.setInterval(tmr, 1000);"
				"</script>"
			"</body>"
		"</html>"
	};

	sendPMString(client, (uint_farptr_t)head);
	client.print("secs = ");
	client.print(param);
	client.print(";");
	sendPMString(client, (uint_farptr_t)tail);
}

void lights(EthernetClient &client, const char *param) {
	static PinMap pins[] = {
		PinMap("main/front/", mainLightFront),
		PinMap("main/back/", mainLightBack),
		PinMap("lab/left/", labLightLeft),
		PinMap("lab/right/", labLightRight)
	};

	int pin = -1;
	const char *action = NULL;

	for(int i = 0; i < sizeof((pins))/sizeof(PinMap); ++i) {
		pins[i].match(param, pin, action);
	}

	if(pin < 0) {
		send404(client);
		return;
	}

	if(!strcmp(action, "On")) {
		digitalWrite(pin, HIGH);
	} else 
	if(!strcmp(action, "Off")) {
		digitalWrite(pin, LOW);
	} else {
		send404(client);
		return;
	}

	redirect(client);
}

void sockets(EthernetClient &client, const char *param) {
	if(param[0] >= 'A' && param[0] <= 'E' && param[1] == '/') {
		if(!strcmp(param + 2, "On")) {
			solight.control(31, param[0], 1);
			redirect(client);
		} else
		if(!strcmp(param + 2, "Off")) {
			solight.control(31, param[0], 0);
			redirect(client);
		} else
		send404(client);
	} else {
		uint16_t socknum;
		size_t i = parseInt(param, socknum);

		if(!i) {
			send404(client);
			return;
		}

		for(; param[i] != '/' && param[i] != 0; ++i) {
			if(param[i] < '0' || param[i] > '1') {
				send404(client);
				return;
			}
			socknum *= 10;
			socknum += param[i] - '0';
		}

		if(!strcmp(param + i, "/On")) {
			solight.control(socknum, 1);
			redirect(client);
		} else
		if(!strcmp(param + i, "/Off")) {
			solight.control(socknum, 0);
			redirect(client);
		} else {
			send404(client);
		}
	}
}

StaticResource resTable[] = {
	StaticResource("/", &sendIndex),
	StaticResource("/everythingOff", &everythingOff)
};

ParamResource categories[] = {
	ParamResource("/lights/", &lights),
	ParamResource("/sockets/", &sockets),
	ParamResource("/everythingOff/", &everythingOffTimer)
};

// Reads first line of HTTP request and extracts the path of max length = len
// Method mut be GET, HTTP version is discarded
// TODO: support other methods, don't discard the version
bool parsePath(EthernetClient &client, char *path, size_t len) {
	const char stateMachine[] = "GET ";
	size_t s = 0, pp = 0;
	// TODO: remove the while loop and replace it with something asynchronous
	while (client.connected()) {
		if (client.available()) {
			char c = client.read();
			// s > 4 marks end of path parsing, so it just waits for '\n'
			if(s > 4) {
				if(c == '\n') return true;
				continue;
			}
			// stateMachine ends with '\0' 
			// it's content must match beginning of the request
			if(stateMachine[s]) {
			       if(stateMachine[s] != c) {
#ifdef DEBUG_ENABLE
				       Serial.println("Bad method");
#endif
				       return false;
			       }
				++s;
			} else {
				if(pp >= len) {
#ifdef DEBUG_ENABLE
					Serial.println("Too long");
#endif
					return false;
				}
				if(c == ' ') {
					path[pp] = 0;
					s = 5;
				} else {
					path[pp] = c;
					++pp;
				}
			}
		}
	}
#ifdef DEBUG_ENABLE
	Serial.println("Client disconnected");
#endif
	return false;
}

// Waits for header blank line discarding the data
void receiveHeaderEnd(EthernetClient &client) {
	bool currentLineIsBlank = true;
	// TODO: remove the loop
	while (client.connected()) {
		if (client.available()) {
			char c = client.read();
			if (c == '\n' && currentLineIsBlank) {
				break;
			}
			if (c == '\n') {
				currentLineIsBlank = true;
			} 
			else if (c != '\r') {
				currentLineIsBlank = false;
			}
		}
	}
}

// Finds the resource specified by path and sends it to the client
bool sendResponse(EthernetClient &client, const char *path) {
	for(int i = 0; i < sizeof(resTable)/sizeof(StaticResource); ++i) {
		if(resTable[i].match(path, client)) return true;
	}
	for(int i = 0; i < sizeof(categories)/sizeof(ParamResource); ++i) {
		if(categories[i].match(path, client)) return true;
	}
	return false;
}

// Handles the client
void receiveClient(EthernetClient &client) {
	char path[128];
	if(!parsePath(client, path, sizeof(path))) {
		receiveHeaderEnd(client);
		send400(client);
	} else {
#ifdef DEBUG_ENABLE
		Serial.print("Path: ");
		Serial.println(path);
#endif
		receiveHeaderEnd(client);
		if(!sendResponse(client, path)) {
			send404(client);
		}
	}

	delay(1);
	client.stop();
}

// Standard Arduino while(1) loop
void loop()
{
	// listen for incoming clients
	// TODO: make it asynchronous
	EthernetClient client = server.available();
	if (client) {
		receiveClient(client);
	}

	// Physical control of lights is achieved by standard switch, not push button,
	// so we must check whether state of the pin changed
	// TODO make it asynchronous/interrupt driven
	bool curMain = digitalRead(mainSwitch);
	bool curLabLeft = digitalRead(labSwitchLeft);
	bool curLabRight = digitalRead(labSwitchRight);

	if(curMain != lastMain) {
		delay(100);
		if(digitalRead(mainSwitch) != lastMain) {
			// If at least one of the lights is on, turn both off
			// else turn both on
			if(digitalRead(mainLightFront) || digitalRead(mainLightBack)) {
				digitalWrite(mainLightFront, LOW);
				digitalWrite(mainLightBack, LOW);
			} else {
				digitalWrite(mainLightFront, HIGH);
				digitalWrite(mainLightBack, HIGH);
			}
			lastMain = curMain;
		}
	}

	// Toggle lights when state of the switch changes
	if(curLabLeft != lastLabLeft) {
		delay(100);
		if(digitalRead(labSwitchLeft) != lastLabLeft) {
			digitalWrite(labLightLeft, !digitalRead(labLightLeft));
			lastLabLeft = curLabLeft;
		}
	}

	if(curLabRight != lastLabRight) {
		delay(100);
		if(digitalRead(labSwitchRight) != lastLabRight) {
			digitalWrite(labLightRight, !digitalRead(labLightRight));
			lastLabRight = curLabRight;
		}
	}

	if(countdownTimer && (unsigned long)(millis() - countdownStart) >= countdownMillis) {
		turnEverythingOff();
		countdownTimer = false;
	}
}
