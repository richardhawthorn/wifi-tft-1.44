#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>
#include <Math.h>

// Color definitions
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define __CS 16
#define __DC 5

//boxes
int h = 26; //height
int r = 3; //radius of corners

TFT_ILI9163C tft = TFT_ILI9163C(__CS, __DC);


//wifi details
const char* ssid     = "ssid here";
const char* password = "password here";

//heydata details
const char* host = "heydata.co.uk";
const char* token = "token here";

int counter = 0;
String result = "";
int result_int = 0;
float result_float = 0;


void tftStart() {

  tft.setRotation(2);
  tft.fillScreen();

}


void tftHeader() {

  tft.drawRoundRect(0, 0, 128, 13, 3, WHITE);

  tft.setCursor(3, 3);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.println("House Data");

}

String getHeyData(int source_id, bool single_value) {

  counter = 0;
  int limit = 0;

  if (single_value == true) {
    limit = 10;
  } else {
    limit = 14;
  }

  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return "";
  }

  // We now create a URI for the request
  String url = "/api/v1/source";
  url += "?source_id=";
  url += source_id;
  if (single_value == true) {
    url += "&view=string";
  }
  url += "&token=";
  url += token;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10000);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);

    counter++;

    //this is the line we want, return it
    if (counter == limit) {
      return line;
    }
  }

}

String processHeyDataValue(String result, String format) {

  result_int = result.toInt();
  result_float = result.toFloat();

  if (format == "float") {
    return (String) result_float;
  } else if (format == "string") {
    return result;
  } else if (format == "int") {
    return (String) result_int;
  } else {
    return " ";
  }

}

int getX(int col) {

  int x = 0;

  if (col == 1) {
    x = 65;
  }

  return x;

}

int getY(int row) {

  int y = 15 + (row * (h + 2));

  return y;

}

int getW(bool full_width) {

  if (full_width) {
    return 128;
  } else {
    return 63;
  }

}

void drawBox(int row, int col, bool full_width) {

  int y = getY(row);
  int x = getX(col);
  int w = getW(full_width);

  //draw black box to clear current area
  tft.fillRect(x, y, w, h, BLACK);

  //draw new empty box
  tft.drawRoundRect(x, y, w, h, r, WHITE);

}

void drawTitle(int x, int y, String title) {

  tft.setCursor(x + 3, y + 2);

  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print(title);

}

void drawValue(int x, int y, String value, bool temp, String unit) {

  tft.setCursor(x + 3, y + 10);

  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  tft.print(value);

  if (temp) {
    tft.setTextSize(1);
    tft.print("o");
    tft.setTextSize(2);
  }

  tft.print(unit);

}



void displayItem(int row, int col, bool full_width, boolean temp, int source_id, String unit, String title, String format) {

  int y = getY(row);
  int x = getX(col);

  String value = "";

  //get the data
  if (source_id > 0) {
    result = getHeyData(source_id, true);
    value = processHeyDataValue(result, format);
  }

  //draw the box
  drawBox(row, col, full_width);

  drawTitle(x, y, title);
  drawValue(x, y, value, temp, unit);

}

void processChartData(String results, int x, int y, int w, int val_max, int val_min){

  int last_start = 0;
  
  int count_start = 0;
  int count_end = 0;
  String value;
  int counter = 0;

  int height = 0;
  int col = 0;

  int chart_x;
  int chart_y1;
  int chart_y2;
  
  for (int i = 0; i < results.length(); i++){
    count_start = results.indexOf(",\"value\":\"", i) + 10;
    count_end = results.indexOf("\"", count_start);

    value = results.substring(count_start,count_end);
    //Serial.println(value);

    height = round(value.toFloat() * 10);
    col++;

    chart_x = x - 3 + w - col;
    chart_y1 = y + h - 2;
    chart_y2 = chart_y1 - height;

    //tft.drawFastVLine(x + 3 + col, y , height, WHITE);
    tft.drawLine(chart_x, chart_y1, chart_x, chart_y2, WHITE);
    //tft.drawPixel(x + 3 + col, y + h - height - 2, WHITE);

    counter++;
    i = count_end;

    if (last_start > count_start){
      break;
    }
    
    if (counter == 56){
      break;
    }

    last_start = count_start;
  }

  Serial.println("end of data");
  
}



void displayChart(int row, int col, bool full_width, int source_id, int val_max, int val_min) {

  int y = getY(row);
  int x = getX(col);
  int w = getW(full_width);

  //get the results
  String results = getHeyData(source_id, false);

  //draw the box
  drawBox(row, col, full_width);

  //output the results
  processChartData(results, x, y, w, val_max, val_min);

}


void setup() {

  tft.begin();

  tftStart();

  Serial.begin(115200);

  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print("---------------------");
  tft.println("      Hey Data");
  tft.print("---------------------");
  tft.println("Booting...");

  delay(10);

  tft.println("Connecting to");
  tft.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }

  tft.println("");
  tft.println("WiFi connected");
  tft.println("IP address: ");
  tft.println(WiFi.localIP());

  delay(5000);

  tftStart();
  tftHeader();

  displayItem(0, 0, false, false, 0, "", "", "");
  displayItem(0, 1, false, false, 0, "", "", "");

  displayItem(1, 0, false, false, 0, "", "", "");
  displayItem(1, 1, false, false, 0, "", "", "");

  displayItem(2, 0, false, false, 0, "", "", "");
  displayItem(2, 1, false, false, 0, "", "", "");

  displayItem(3, 0, false, false, 0, "", "", "");
  displayItem(3, 1, false, false, 0, "", "", "");

}




void loop() {

  displayItem(0, 0, true, true, 71, "C", "Bedroom Temp", "float");
  displayItem(1, 0, true, false, 72, "%", "Bedroom Humidity", "float");
  displayItem(2, 0, true, false, 14, "W", "House Power", "int");
  displayItem(3, 0, true, true, 39, "C", "Outside Temperature", "float");
  
  delay(60000);

}







