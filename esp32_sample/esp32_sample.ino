/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <WiFi.h>
#include <lwip/sockets.h>
#include <errno.h>

#undef read
#undef write
#undef close

const char* ssid     = "SSID";
const char* password = "PASS";

WiFiServer server(80);

void setup()
{
    Serial.begin(115200);
    delay(10);

    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.printf("Connecting to %s\n", ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();

    // setup SD
    Serial.println("Setup SD Card");
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

}

int TIMEOUT_FOR_NOT_AVAILABLE = 30;

void loop(){
 WiFiClient client = server.available();   // listen for incoming clients
 int notAvailableCount = 0;

  if (client) {
    Serial.println("new client");
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
    String currentLine = "";                // make a String to hold incoming data from the client
    bool hasBody = false;
    String boundary = "";                   // multipart form data boundary
    int contentLength = 0;                  // http header "Content-Length" value
    while (client.connected()) {            // loop while the client's connected
      int bytes = client.available();
      if (bytes) {                          // if there's bytes to read from the client,
        notAvailableCount = 0;
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {
            if (hasBody && (boundary.length() > 0)) {
              Serial.println(">>Start boundary");
              saveFile(client, SD, contentLength, boundary);
            }
          } else {

            if (hasBody && boundary.length() == 0) {
              // get boundary
              boundary = getBoundary(currentLine);
              Serial.println(">>boundary is : " + boundary);
            }
            if (currentLine.indexOf("Content-Length") > -1 && contentLength == 0) {
              contentLength = getContentLength(currentLine);
              Serial.print(">>Content-Length is : ");
              Serial.println(contentLength);
            }
            currentLine = "";               // clear Line
          }
        } else if (c != '\r') {  //   if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        if (currentLine.indexOf("Content-Type: multipart/form-data; boundary=") > -1) {
          hasBody = true;
        } else if (currentLine == "Expect: 100-continue") {
          // if you got "Expect: 100-continue" header, send response of "100 Continue"
          client.println("HTTP/1.1 100 Continue");
          client.println();
          client.println();
          Serial.println("");
          Serial.println(">>response 100 Continue.");
          currentLine = "";
        }

      } else {
        notAvailableCount += 1;
        if (notAvailableCount >= TIMEOUT_FOR_NOT_AVAILABLE) {
          Serial.println("NOT AVAILABLE");
          break;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
    SD.end();
    Serial.println("SD Card unmounted.");
  }
}

String getBoundary(String header) {
  int index = header.indexOf("boundary=");
  return header.substring(index + 9);
}

int getContentLength(String header) {
  int index = header.indexOf("Content-Length: ");
  String ret = header.substring(index + 16);
  ret.trim();
  return ret.toInt();
}

/*
 * ファイル保存.
 * 受信するファイルは1つであるという前提
 */
void saveFile(WiFiClient client, fs::FS &fs, int contentLength, String boundary) {
  Serial.println(">>saveFile");
  String currentLine = "";
  String filename = "";
  int nowByteSize = 0;
  while (true) {
    char c = client.read();
    ++nowByteSize;
    if (c == '\n') {
      Serial.println(currentLine);
      // filename
      if (currentLine.indexOf("Content-Disposition") > -1 && currentLine.indexOf("filename=") > -1 ) {
        int index = currentLine.indexOf("filename=");
        filename = currentLine.substring(index + 9);
        filename.replace("\"", "");
        Serial.println(">>filename is: " + filename);
      }
      if (currentLine.length() == 0) {
        // parse file body
        Serial.println(">>parse file body");
        Serial.printf(">>nowByteSize: %d\n", nowByteSize);
        int bodySize = contentLength - nowByteSize - boundary.length() - 6; // calcurate file body size
        Serial.printf(">>file size: %d\n", bodySize);
        uint8_t buf[bodySize];
        client.read(&buf[0], bodySize);
        String path = "/" + filename;
        File file = fs.open(path.c_str(), FILE_WRITE);
        if(!file){
            Serial.println("Failed to open file for writing");
            return;
        }
        file.write(buf, bodySize);
        file.close();
        break;
      }
      currentLine = "";
    } else if (c != '\r') {
      currentLine += c;
    }
  }

}

