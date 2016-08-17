/*
 * Arduino-HDK Shield Code: VERSION 2.0.0 - BASSEL ALESH 2016
 * 
 * The Arduino-HDK Shield was a prototype that later became a ready-for-production PCB that can be mounted on the WNC M18Q2 model for an LTE connection using AT Commands, UART, TDP sockets, and the AT&T M2X API. 
 * It was the end-of-internship project by me (Bassel Alesh)that utilized the different tests and mini-projects that were tackled over the summer and combined everything that was learned into a comprehensive, user-friendly project.
 * The shield is a 2-layer PCB that containts the wiring for an Arduino Pro Mini and an Adafruit 4-bit Bi-Directional Level Shifter to the Hardware Development Kit (HDK) from WNC & their Qualcomm-based M18Q2 chipset. 
 * 2.0.0 code includes: Set Time option, Posting every 1-1:30 minutes (depending on door closing and opening), Uses PIN0 and PIN1 for Hardware Serial UART communication, Reads from pins 5, 6, and 7 for sensor info and M2X
 * Pin 7 uses Wire and oneWire (based on I2C protocol) to post temperature data using a Dallas temperature sensor. Pins 5 and 6 use digitalRead to read IR sensors. 
 * 
*/



//Libraries
#include <AltSoftSerial.h> //communicate through pin 8 & 9
#include <Time.h> //time library
#include <OneWire.h>
#include <DallasTemperature.h>


//Sensor Information
#define ONE_WIRE_BUS 7 //ds18b20 temperature sensor attached to pin 7
OneWire oneWire(ONE_WIRE_BUS); //oneWire instance to communicate with oneWire devices
DallasTemperature sensors(&oneWire); //pass oneWire reference to Dallas Temperature
#define left_door 5 //left door IR sensor
#define right_door 6 //right door IR sensor
#define ResetPin 12 //reset pin to overcome the module locking issue - needs PIN 12 to be connected to RESET pin on HDK using a level shifter (5V -> 1.8V)



//INFORMATION THAT THE USER CAN ADJUST FOR DESIRED PREFERENCES

//Timing Information
#define speed2 100 //delay time between AT@SOCKWRITE messages - do not change this
//change the #define labels below to your preference
#define timezone "-04:00"
#define my_day 8
#define my_month 8
#define my_year 2016 //date of choice (with timezone)
#define post_time 3000 //sleep time before posting again
#define my_min 23
#define my_hour 9 
#define my_sec 0 //time of choice

//M2X Information
String deviceID = "5768a2a07576f36fe"; //half of the deviceID - to accommodate for the Arduino's memory capability
String deviceID2 = "969eee3be685a39"; //other half of device ID
String stream1 = "temp"; //stream name - already created online on your M2X account
String stream2 = "dr"; //second stream name for M2X account - checks if door is open
String m2x_key = "2e54c4af7a4443da"; //half of M2X API Key
String m2x_key2 = "392c65aa8fe22d5f"; //other half of M2X API Key

int stream_number; //initialize stream_number variable for picking the right stream & collecting the right data
int reset_counter; //initialize reset counter for reboot in case of module lock-ups

/*


  that should be it for the setup - everything else is automatic by the sketch
  only the values above should be altered in order to operate the sketch
  direct any questions to Bassel Alesh - basselalesh@gmail.com


*/

AltSoftSerial mySerial; // the serial between UNO and WNC devices

void setup() { // run once

  Serial.begin(115200); // the serial baud rate between the UNO and PC
  while (!Serial) {
    ; // wait for serial port to connect
  }
  delay(10000); //give the user time to turn on the WNC module (HOLDS THE YELLOW BUTTON FOR 5-7 seconds)
  pinMode(13, OUTPUT); 
  digitalWrite(13, LOW); //LED to low before any connection checks
  connection_check(); //check for connection - blink GREEN for 3 seconds if it works; stuck in a loop otherwise
  setTime(my_hour, my_min, my_sec, my_day, my_month, my_year); //
  reset_counter = 0; //intialize reset counter

}

void loop() { // run over and over
  //get timestamp
  String timestamp = make_timestamp(); //start the timestamp - starts at 00:00:00 automatically with the specified date & timezone

  //get data values
  int indicator = 1; //assign indicator for data collection
  String data_val1 = get_data(&indicator); //get data value from get_data function

  indicator = 2;
  String data_val2 = get_data(&indicator); //get data value from get_data function

  //post to first stream
  stream_number = 1; //pick the stream #
  String socketID = setup_socket();
  delay(1000); //wait a bit before starting the socket write function (AT@SOCKWRITE)
  sock_write(&socketID, &stream_number, &timestamp, &data_val1); //send the socket - recorded value is posted
  delay(post_time); //wait before starting the next sequence
  reset_counter++; //increment posting counter

  //post to second stream 
  stream_number = 2; //pick the stream #
  socketID = setup_socket();
  delay(1000); //wait a bit before starting the socket write function (AT@SOCKWRITE)
  sock_write(&socketID, &stream_number, &timestamp, &data_val2); //send the socket - recorded value is posted
  delay(post_time); //wait before starting the next sequence
  reset_counter++; //increment posting counter

//allow 100 counts before rebooting - maximum data loss: 100/n where n is the number of streams being accessed
  if (reset_counter >= 100) {
    reset_counter = 0; //reset counter
    reboot(); //enter reboot function
    delay(4000); //give some time to reboot the module
    connection_check(); //check for connection - exits if successful; stuck in loop if not
  }
}


//connection check function - blinks GREEN for 3 seconds if connection successful; recurring loop otherwise 
void connection_check() {
  Serial.print("AT@INTERNET=1\r");
  delay(100); //wait
  if (Serial.available()) {
    digitalWrite(13, HIGH);
    delay(3000);
    digitalWrite(13, LOW);
    return; //exit recurring function
  }
  connection_check(); //recurse 
}

//function to set up the TDP socket for the API request
String setup_socket() 
{
  String resp1, resp2; //prepare empty strings for the responses
  delay(500); //give some start time before sending the first commands - also helpful when repeating the loop over and over again
  send_message_noln("AT@INTERNET=1\r", &resp1, 200); //misc command that usually does not get accepted on the HDK's end
  send_message_noln("AT@INTERNET=1\r", &resp1, 200); //set up the internet service
  send_message_noln("AT@SOCKDIAL=1\r", &resp1, 200); //set up TCP socket method
  send_message_noln("AT@SOCKCLOSE=1\r", &resp1, 200); //allows for socket reuse - recommended, not required
  send_message_noln("AT@SOCKCREAT=1\r", &resp1, 100); //creates a socket

  int bpost = resp1.indexOf(":");
  char socket_id = resp1[bpost + 1]; //parse the socket ID and use it in the next commands - this is usually unnecessary since the socket gets closed anyway

  send_message_noln("AT@DNSRESVDON=\"api-m2x.att.com\"\r", &resp2, 1000); //call up the AT&T M2X Server
  resp2.remove(0, 48); //parse until the first apostrophe
  int apost = resp2.indexOf("\""); //parse what's at the second apostrophe and after - leaving ONLY the IP Address number
  resp2.remove(apost);
  String IP_address = resp2; //parse the response to get IP address - also convert it into a string

  String socket_string = String(socket_id); //getting ready to use the socket ID in the AT@SOCKCONN command

  String sockconn = "AT@SOCKCONN=" + socket_string + ",\"" + IP_address + "\",80\r\r";
  send_message_noln(sockconn, &resp2, 200); //connect to the server with the given socket
  return socket_string;
}

//function to reboot the device
void reboot() {
  pinMode(ResetPin, OUTPUT); //blink
  digitalWrite(ResetPin, HIGH);   // preset RESET (HIGH)
  digitalWrite(ResetPin, LOW);   // assert RESET (LOW)
  delay(3100);
  pinMode(ResetPin, INPUT); //set it to be input, not going to control the reset pin, let the pull-high inside the HDK work.
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH); //blink
  delay(500); //
  digitalWrite(13, LOW); 
}

//the send_message function sends AT commands that are one line only
//the term 'noln' indicates no new line is posted after the command is send
void send_message_noln(const String cmd, String *resp, int ms)
{
  Serial.print(cmd); //send the command using UART
  delay(ms); //delay a certain amount of time
  (*resp) = ""; //clear the response buffer

  digitalWrite(13, LOW);
  while (Serial.available()) {
    char inByte = Serial.read(); //read incoming message from HDK
    (*resp) += inByte;
    digitalWrite(13, HIGH);
  }
  digitalWrite(13, LOW);
}


//standard ASCII to HEX conversion function - courtesy of Ophas from WNC
String binary_to_hex(const String bin) {
  String hex = "";
  int i = 0;
  for (i = 0; i < bin.length(); i++)
  {
    if (bin.charAt(i) == '\r') hex += "0d";
    else if (bin.charAt(i) == '\n') hex += "0a";
    else hex += String(bin.charAt(i), HEX);
  }
  return hex;
}

//this function makes the timestamp string using the ISO 8601 standard made mandatory by the M2X API
//look here for more info on ISO 8601: https://www.w3.org/TR/NOTE-datetime
String make_timestamp() {
  String start_day = String(day());
  String start_month = String(month());
  String start_year = String(year());

  int min_int = minute(), sec_int = second(), hr_int = hour();
  String minStr = String(min_int);
  if (minStr.length() == 1) minStr = String("0" + minStr);
  String secStr = String(sec_int);
  if (secStr.length() == 1) secStr = String("0" + secStr);
  String hrStr = String(hr_int);
  if (hrStr.length() == 1) hrStr = String("0" + hrStr);
  /*String start_day = String(day);
    String start_month = String(month);
    String start_year = String(year);*/
  if (start_day.length() == 1) start_day = "0" + start_day;
  if (start_month.length() == 1) start_month = "0" + start_month;
  String timestamp = start_year + "-" + start_month + "-" + start_day + "T" + hrStr + ":" + minStr + ":" + secStr;
  return timestamp;
}

//this function gets the data that is to be posted onto M2X
String get_data(int *stream_number)
{
  if (*stream_number == 1) { //if the data needed is temperature data
    sensors.requestTemperatures(); // Send the command to get temperatures
    return String(1.8 * sensors.getTempCByIndex(0) + 32.0); //read from the pin 7 the temperature in Fahrenheit
  }
  if (*stream_number == 2) { //if the temperature needed is the # of door openings per interval (~50 seconds or door-check loops)
    int door_timer = 0;     //reset door timer
    for (int i = 0; i <50 ; i++) { //
      if (digitalRead(left_door) || digitalRead(right_door)) { 
        while (digitalRead(left_door) || digitalRead(right_door)) {
          ;
        }
        door_timer++;
      }
      delay(1000);
    }
    return String(door_timer);
  }
}

/*the AT@SOCKRWITE command
   this is sent in the following format: AT@SOCKWRITE=socket_ID, byte size (default at 500 for this sketch), "HEX message"
   byte size = 500 mainly because only one data point is sent at a time
   the socket_ID is passed through the function
   this is split up in a way to overcome the buffer limit on the Arduino and to handle the set baud rate
  The message itself follows the following format:
  POST /v2/devices/5768a2a07576f36fe969eee3be685a39/updates HTTP/1.1
  Host: api-m2x.att.com
  Accept: *//* (only one slash here - forgive the error to avoid messing with the comment structure)
  User-Agent: WNC IoT Dev Kit
  X-M2X-KEY: 2e54c4af7a4443da392c65aa8fe22d5f
  Content-Type: application/json
  Content-Length: 120
  { "values": { "stream": [{"timestamp": "2016-06-21T14:29:10Z", "value": data_val}] } }

  The message should be clear as the sock_write function is sent
  Follow the document to send HTTP requests using the HDK to understand the formatting of different carriage returns and newlines (the 0d0a sequences, or \r\n in this case)
  The content-length is set to 120 to accommodate different data value sizes
  Everything else is either specified by the headers at the beginning of this sketch (by the user or by default)
*/

void sock_write(String *socket_string, int *stream_number, String *timestamp, String *data_val) {
  String resp3; //to store responses

  send_message_noln("AT@SOCKWRITE=" + *socket_string + ",500,\"", &resp3, speed2); //begin the sockwrite command with the socket ID and byte size
  send_message_noln(binary_to_hex("POST /v2/devices/"), &resp3, speed2); //begin the actual hexadecimal message
  send_message_noln(binary_to_hex(deviceID), &resp3, speed2);
  send_message_noln(binary_to_hex(deviceID2), &resp3, speed2);
  send_message_noln(binary_to_hex("/updates HTTP/1.1"), &resp3, speed2);
  send_message_noln(binary_to_hex("\r\nHost: api"), &resp3, speed2);
  send_message_noln(binary_to_hex("-m2x.att.com\r\n"), &resp3, speed2);
  send_message_noln(binary_to_hex("Accept: */*\r\n"), &resp3, speed2);

  String skeleton2 = "User-Agent: WNC IoT";
  String str2 = binary_to_hex(skeleton2);
  send_message_noln(str2, &resp3, speed2);
  send_message_noln(binary_to_hex(" Dev Kit\r\n"), &resp3, speed2);

  String skeleton10 = "X-M2X-KEY: ";
  String str10 = binary_to_hex(skeleton10);
  send_message_noln(str10, &resp3, speed2);

  send_message_noln(binary_to_hex(m2x_key), &resp3, 500);
  send_message_noln(binary_to_hex(m2x_key2), &resp3, 500);
  send_message_noln(binary_to_hex("\r\n"), &resp3, speed2);
  String skeleton3 = "Content-Type: ";
  String str3 = binary_to_hex(skeleton3);
  send_message_noln(str3, &resp3, speed2);
  send_message_noln(binary_to_hex("application/json"), &resp3, speed2);
  send_message_noln(binary_to_hex("\r\n"), &resp3, speed2);

  send_message_noln(binary_to_hex("Content-Length:"), &resp3, speed2);
  send_message_noln(binary_to_hex(" 120\r\n\r\n"), &resp3, speed2);

  String skeleton5 = "{ \"values\": { \"";
  String str5 = binary_to_hex(skeleton5);
  send_message_noln(str5, &resp3, speed2);

  String stream;
  if (*stream_number == 1) stream = stream1;
  if (*stream_number == 2) stream = stream2;

  String str11 = binary_to_hex(stream + "\": [{");
  send_message_noln(str11, &resp3, speed2);
  String str6 = binary_to_hex("\"timestamp\": \"");
  send_message_noln(str6, &resp3, speed2);
  send_message_noln(binary_to_hex(*timestamp), &resp3, 300);
  send_message_noln(binary_to_hex(timezone), &resp3, 300);
  send_message_noln(binary_to_hex("\", \"value\": "), &resp3, speed2);
  send_message_noln(binary_to_hex(*data_val + "}] } }"), &resp3, speed2);
  send_message_noln(binary_to_hex("\r\n\r\n"), &resp3, speed2);
  send_message_noln("\"\r", &resp3, 2000); //one last carriage return to send the actual command
}

