#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <U8g2lib.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "iQOO Z6 Pro";
const char* password = "12345678";

// ESP8266 Web server
ESP8266WebServer server(80);

// Initialize OLED display (I2C pins: SDA = D2, SCL = D1)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Sensor pins

const int heartRatePin = D5; // Heart-rate sensor connected to D5 (Digital input)
const int lm35TouchPin = D6; // LM35 sensor touch pin connected to D6

// Variables to store sensor readings
float temperature = 0.0;
int heartRate = 0;
int heartRateValues[10];  // Array to store 10 heart rate values for pulse graph

// Timer for updating data (1 second intervals)
unsigned long previousMillis = 0;
const long interval = 1000;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Initialize OLED
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);  // Set font for OLED display
  
  // Connect to WiFi
  connectToWiFi();

  // Start Web Server
  server.on("/", handleRoot);
  server.on("/data", handleData);  // JSON endpoint for sensor data
  server.begin();
  
  // Setup the pin for LM35 touch detection
  pinMode(lm35TouchPin, INPUT);
  
  Serial.println("Server started");

  // Initialize heart rate values with random values
  generateHeartRateValues();
}

void loop() {
  // Handle client requests
  server.handleClient();
  
  // Get and update sensor data every second
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Update temperature: Show 0°C if not touched, 37°C to 50°C if touched
    temperature = getTemperature();
    
    // Check if the heart rate sensor is touched
    if (isSensorTouched()) {
      heartRate = getHeartRate();  // Only read heart rate if the sensor is touched
    } else {
      heartRate = 0;  // No pulse detected if not touched
    }

    // Update OLED display
    updateDisplay();
    
    // Print values to Serial Monitor for debugging
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" C, Heart Rate: ");
    Serial.println(heartRate);
    
    // Update heart rate values for pulse graph
    shiftHeartRateValues(heartRate);
  }
}

// Function to connect to WiFi
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Display IP address on OLED
  displayMessage("IP: " + WiFi.localIP().toString());
}

// Function to display a message on the OLED
void displayMessage(String message) {
  u8g2.clearBuffer();
  u8g2.setCursor(0, 10);
  u8g2.print(message);
  u8g2.sendBuffer();
}

// Function to update OLED with sensor data
void updateDisplay() {
  u8g2.clearBuffer();  // Clear the internal memory
  
  // Display temperature
  u8g2.setCursor(0, 10);
  u8g2.print("Temperature: ");
  u8g2.print(temperature);
  u8g2.print(" C");

  // Display heart rate
  u8g2.setCursor(0, 30);
  u8g2.print("HR: ");
  u8g2.print(heartRate);
  
  // Draw pulse graph (heart rate values over time)
  drawPulseGraph();
  
  u8g2.sendBuffer();  // Transfer the internal memory to the display
}

// Function to simulate temperature from LM35 sensor in the range of 37-50°C
// If the sensor is not touched, display 0°C
float getTemperature() {
  // Check if the LM35 sensor is touched
  if (isLM35Touched()) {
    // Simulate a temperature between 37°C and 50°C when touched
    float simulatedTemp = random(370, 501) / 10.0; // Random value between 37.0 and 50.0
    return simulatedTemp;
  } else {
    return 0; // Show 0°C when not touched
  }
}

// Function to simulate heart rate sensor reading
int getHeartRate() {
  // Simulate heart rate detection with a random value between 60 and 100
  int simulatedHeartRate = random(60, 100);

  // Return valid heart rate
  return simulatedHeartRate;
}

// Function to check if the LM35 sensor is touched
bool isLM35Touched() {
  int sensorValue = digitalRead(lm35TouchPin);
  
  // Assume HIGH means the sensor is touched, and LOW means no touch
  return (sensorValue == HIGH);
}

// Function to check if the heart rate sensor is touched
bool isSensorTouched() {
  int sensorValue = digitalRead(heartRatePin);
  
  // Assume HIGH means sensor touched, and LOW means no touch
  if (sensorValue == HIGH) {
    return true;  // Sensor touched
  } else {
    return false;  // Sensor not touched
  }
}

// Function to generate 10 random heart rate values initially
void generateHeartRateValues() {
  for (int i = 0; i < 10; i++) {
    heartRateValues[i] = random(60, 100);  // Initial heart rate values
  }
}

// Function to shift heart rate values and add the latest value
void shiftHeartRateValues(int newValue) {
  for (int i = 0; i < 9; i++) {
    heartRateValues[i] = heartRateValues[i + 1];  // Shift values left
  }
  heartRateValues[9] = newValue;  // Add the new heart rate value
}

// Function to draw the heart rate pulse graph on OLED
void drawPulseGraph() {
  // Draw the pulse graph by connecting points in the heartRateValues array
  for (int i = 0; i < 9; i++) {
    int x1 = i * 12 + 10; // X-coordinate (spaced by 12 pixels, starting at x=10)
    int y1 = 64 - map(heartRateValues[i], 0, 100, 10, 50);  // Y-coordinate (scaled and inverted for display)
    
    int x2 = (i + 1) * 12 + 10; // X-coordinate for the next point
    int y2 = 64 - map(heartRateValues[i + 1], 0, 100, 10, 50);  // Next Y-coordinate
    
    u8g2.drawLine(x1, y1, x2, y2);  // Draw line between two points
  }
}

// Web server root handler
void handleRoot() {
  String html = "<html><head><title>ESP8266 Monitor</title>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script></head><body>";
  html += "<h1>ESP8266 Monitoring</h1>";
  
  // Adding canvas elements for the two graphs
  html += "<canvas id='tempChart' width='400' height='200'></canvas>";
  html += "<canvas id='heartRateChart' width='400' height='200'></canvas>";
  
  // JavaScript to fetch data from "/data" and update the charts
  html += "<script>";
  
  // Setup initial data arrays and chart instances
  html += "var tempData = [];\nvar heartRateData = [];\n";
  html += "var tempLabels = [];\nvar hrLabels = [];\n";
  
  // Create temperature chart
  html += "var tempCtx = document.getElementById('tempChart').getContext('2d');";
  html += "var tempChart = new Chart(tempCtx, { type: 'line', data: { labels: tempLabels, datasets: [{ label: 'Temperature (C)', data: tempData, fill: false, borderColor: 'rgba(75, 192, 192, 1)', tension: 0.1 }]}});";
  
  // Create heart rate chart
  html += "var hrCtx = document.getElementById('heartRateChart').getContext('2d');";
  html += "var heartRateChart = new Chart(hrCtx, { type: 'line', data: { labels: hrLabels, datasets: [{ label: 'Heart Rate (bpm)', data: heartRateData, fill: false, borderColor: 'rgba(255, 99, 132, 1)', tension: 0.1 }]}});";
  
  // Function to update data from "/data" endpoint
  html += "setInterval(function() { fetch('/data').then(response => response.json()).then(data => {";
  html += "if (tempData.length >= 20) { tempData.shift(); tempLabels.shift(); }";
  html += "if (heartRateData.length >= 20) { heartRateData.shift(); hrLabels.shift(); }";
  html += "tempData.push(data.temperature);";
  html += "heartRateData.push(data.heartRate);";
  html += "var now = new Date(); var time = now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds();";
  html += "tempLabels.push(time); hrLabels.push(time); tempChart.update(); heartRateChart.update();";
  html += "}); }, 1000);";  // Update every second
  
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Web server data handler (returns sensor data as JSON)
void handleData() {
  String json = "{";
  json += "\"temperature\": " + String(temperature, 1) + ",";
  json += "\"heartRate\": " + String(heartRate);
  json += "}";
  
  server.send(200, "application/json", json);
}
