/*
 *    ----------------------------------
 *    WIFI V4 - ESP8266
 *    WRITTEN BY: CONNOR EDWARDS
 *    ----------------------------------
 *    PROVIDES A GUI TO GET THE ESP8266
 *    CONNECTED TO WIFI AND ALSO AN MQTT
 *    BROKER WITH PAYLOAD BUFFERING.
 *    ----------------------------------
 *    (c) Noval Studios, 2016
 *    
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

// Variables
#define connection_timeout 30
#define ap_switch 5
#define green_led 4

// Access Point Variables
const char WiFiAPPSK[] = "appassword";
ESP8266WebServer server(80);

// MQTT Settings
const char* mqtt_server = "pi.frooplexp.com";
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {

  // Setting direction of LED's
  pinMode(green_led, OUTPUT);

  digitalWrite(ap_switch, LOW);
  
  // Setting direction of button
  pinMode(ap_switch, INPUT);
  // Starting the Serial bus
  Serial.begin(115200);
  // Switching to WiFi Station
  WiFi.mode(WIFI_STA);

  // Slight delay
  delay(1000);

  // Setting up MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); 

  // Checking connection
  if (WiFi.status() != WL_CONNECTED) {
    // Not connected
    Serial.print("Not connected...");
  } else if (WiFi.status() == WL_CONNECTED) {
    // Connected!
    Serial.print("Connected to WiFi...");
  }

  // Webserver stuff for config
  server.on("/", render_home); // Home Page
  server.on("/setup", [](){ // Setup page

    // Getting the data from the form
    String network_index = server.arg("network");
    String network_name = WiFi.SSID(network_index.toInt());
    String network_password = server.arg("network_password");

    // Sending back response
    server.send(200, "text/plain", "Attempting to connect...");

    // Converting the SSID and Password to Char*
    int network_name_len = network_name.length() + 1;
    int network_password_len = network_password.length() + 1;

    // Creating the Character sets
    char ssid[network_name_len];
    char password[network_password_len];
    
    // Converting
    network_name.toCharArray(ssid, network_name_len);
    network_password.toCharArray(password, network_password_len);
    
    // Connecting to the network
    bool check_new_conn = connect_to_network(ssid, password);
    
    // Checking the connection
    if (check_new_conn == true) {
      Serial.println("Connection Successfull! My IP is:");
      Serial.println(WiFi.localIP());
    } else if (check_new_conn == false) {
      Serial.println("Failed to connect!");
    }
    
  });
  
  // Starting the HTTP Server
  server.begin();
  
}

void loop() {

  // Listening for AP Switch
  if (digitalRead(ap_switch) == HIGH) {
    turn_on_ap();
  }

  // Checking MQTT Connection
  if (!client.connected() && WiFi.status() == WL_CONNECTED) { // No Connection
    reconnect(); // Reconnect on Fail
  }

  client.loop();

  // Listening for requests
  server.handleClient();
            
}

// Used to reconnect to MQTT on fail
void reconnect() {
  while (!client.connected()) { // As long as were not connected
    Serial.println("Attempting MQTT Connection...");
    // Attempting connection
    if (client.connect("ESP8266Client")) { // This server was set in the void setup()
      // We're back online!
      Serial.println("Connected to MQTT!");
      // Subscribing to a TOPIC
      client.subscribe("noval_iot");
    } else {
      // Failed to connect
      Serial.println("Failed to connect to MQTT Broker, retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// MQTT Callback, called when we get a new message
void callback(char* topic, byte* payload, unsigned int length) {
  
  if (String((char)payload[0]) == "g") {
      digitalWrite(green_led, String((char)payload[1]).toInt());
  }
  
}

void render_home() {

  int num_of_seen_networks = scan_networks();
  
  // Preparing the page
  String setup_page = "<html>";
  setup_page += "<head>";
  setup_page += "<title>Setup your Node</title>";
  setup_page += "</head>";
  setup_page += "<body>";
  setup_page += "<h1>Set me up</h1>";
  setup_page += "<form method='POST' action='/setup'>";
  setup_page += "<p>";
  setup_page += "<select name='network'>";

  // Looping through networks
  if (num_of_seen_networks == 0)
    setup_page += "<option>no networks found</option>";
  else {
    for (int i = 0; i < num_of_seen_networks; ++i) {
      setup_page += "<option value=";
      setup_page += i;
      setup_page += ">";
      setup_page += WiFi.SSID(i);
      setup_page += "</option>";
    }
  }
  
  setup_page += "</select>";
  setup_page += "</p>";
  setup_page += "<p><input type='password' placeholder='Network Password' name='network_password'></p>";
  setup_page += "<p><input type='submit' value='Connect'></p>";
  setup_page += "</form>";
  setup_page += "</body>";
  setup_page += "</html>";

  // Sending the page
  server.send(200, "text/html", setup_page);
  
}

void turn_on_ap() {
  
  // Printing out to Serial
  Serial.println("STARTING ACCESS POINT...");

  WiFi.disconnect();
  
  // Changing WiFi mode
  WiFi.mode(WIFI_AP);
  
  // Giving it a MAC Address
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  
  // Creating a Unique name for the Device
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX); 
  macID.toUpperCase();
  String AP_NameString = "Noval Node - " + macID;
  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  // Starting the Access point
  WiFi.softAP(AP_NameChar, WiFiAPPSK);

  // Slight delay to prevent multiple button presses
  delay(3000); 

  // Alerting the console
  Serial.println("AP STARTED...");
  
}

int scan_networks() {
  // Scanning for networks
  int n = WiFi.scanNetworks();  // Getting the visible networks
  // Checking response and looping through each found network
  if (n == 0)
    Serial.println("no networks found");
  else {
    for (int i = 0; i < n; ++i)
    {
      Serial.println(WiFi.SSID(i)); // Printing the Network name
    }
  } 
  return n;
}

bool connect_to_network(char* ssid, char* password) {

  WiFi.disconnect();
  
  // Changing connection type
  WiFi.mode(WIFI_STA);
  
  // Starting the WiFi connection
  WiFi.begin(ssid, password);

  Serial.println("Connecing to ");
  Serial.println(ssid);
  Serial.println("With password: ");
  Serial.println(password);
  
  int counter = 0;
  bool connected_to_wifi = false;
  bool timeout = false;

  while (!connected_to_wifi && !timeout) {
    
    // Checking connection
    if (WiFi.status() != WL_CONNECTED) {
      counter++; 
    } else if (WiFi.status() == WL_CONNECTED) {
      connected_to_wifi = true;
    }
    
    // Checking timeout
    if (counter == connection_timeout) {
      timeout = true;
    }

    // Delaying 
    delay(500);
 
  }

  // Checking what happened
  if (timeout == true) {
    return false;
  } else if (connected_to_wifi == true) {
    return true;
  }
  
}

