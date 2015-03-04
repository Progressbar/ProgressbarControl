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
			"Front <a href=\"lights/mainFrontOn\">on</a>|<a href=\"lights/mainFrontOff\">off</a><br>"
			"Back <a href=\"lights/mainBackOn\">on</a>|<a href=\"lights/mainBackOff\">off</a><br>"
			"Left <a href=\"lights/labLeftOn\">on</a>|<a href=\"lights/labLeftOff\">off</a><br>"
			"Right <a href=\"lights/labRightOn\">on</a>|<a href=\"lights/labRightOff\">off</a><br>"
			"<h2>Sockets</h2>"
			"A <a href=\"sockets/AOn\">on</a>|<a href=\"sockets/AOff\">off</a><br>"
			"B <a href=\"sockets/BOn\">on</a>|<a href=\"sockets/BOff\">off</a><br>"
			"C <a href=\"sockets/COn\">on</a>|<a href=\"sockets/COff\">off</a><br>"
			"D <a href=\"sockets/DOn\">on</a>|<a href=\"sockets/DOff\">off</a><br>"
			"E <a href=\"sockets/EOn\">on</a>|<a href=\"sockets/EOff\">off</a><br>"
			"<h2>The red button</h2>"
			"<a href=\"everythingOff\">Shutdown the Universe</a><br>"
			"</body>"
		"</html>"
	};

	sendPMString(client, (uint_farptr_t)index);
}

// Lights
// TODO: replace static resources with something more inteligent
void mainFrontOn(EthernetClient &client) {
	redirect(client);
	digitalWrite(mainLightFront, HIGH);
}

void mainFrontOff(EthernetClient &client) {
	redirect(client);
	digitalWrite(mainLightFront, LOW);
}

void mainBackOn(EthernetClient &client) {
	redirect(client);
	digitalWrite(mainLightBack, HIGH);
}

void mainBackOff(EthernetClient &client) {
	redirect(client);
	digitalWrite(mainLightBack, LOW);
}

void labLeftOn(EthernetClient &client) {
	redirect(client);
	digitalWrite(labLightLeft, HIGH);
}

void labLeftOff(EthernetClient &client) {
	redirect(client);
	digitalWrite(labLightLeft, LOW);
}

void labRightOn(EthernetClient &client) {
	redirect(client);
	digitalWrite(labLightRight, HIGH);
}

void labRightOff(EthernetClient &client) {
	redirect(client);
	digitalWrite(labLightRight, LOW);
}

// Sockets
void socketAOn(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'A', 1);
}

void socketBOn(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'B', 1);
}

void socketCOn(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'C', 1);
}

void socketDOn(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'D', 1);
}

void socketEOn(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'E', 1);
}

void socketAOff(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'A', 0);
}

void socketBOff(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'B', 0);
}

void socketCOff(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'C', 0);
}

void socketDOff(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'D', 0);
}

void socketEOff(EthernetClient &client) {
	redirect(client);
	solight.control(31, 'E', 0);
}

// Kill switch
// TODO: add timer support
void everythingOff(EthernetClient &client) {
	redirect(client);
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

#define LI(NAME) StaticResource("/lights/" #NAME, &NAME)
#define SI(NAME) StaticResource("/sockets/" #NAME, &socket ## NAME)

// TODO: some sort of automatic table generation?
StaticResource resTable[] = {
	StaticResource("/", &sendIndex),
	LI(mainFrontOn),
	LI(mainFrontOff),
	LI(mainBackOn),
	LI(mainBackOff),
	LI(labLeftOn),
	LI(labLeftOff),
	LI(labRightOn),
	LI(labRightOff),
	SI(AOn),
	SI(AOff),
	SI(BOn),
	SI(BOff),
	SI(COn),
	SI(COff),
	SI(DOn),
	SI(DOff),
	SI(EOn),
	SI(EOff),
	StaticResource("/everythingOff", &everythingOff)
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
}
