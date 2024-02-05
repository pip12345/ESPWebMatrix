#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include <math.h>
#include <Preferences.h> // EEPROM library
#include <MapFloat.h> // Float mapping for percentage
#include "webpage.h" // Web page that gets displayed
#include "webmatrix_functions.h"
#include "case_defines.h"

#define DATA_PIN 32 // Matrix datapin
#define VOLTAGE_PIN 33 // Pin to sense voltage on

#define VOLTAGE_DIVIDER_RATIO 11 // 36.3 V -> 3.3 V max, ratio: 11, 100 KΩ + 10 KΩ voltage divider
#define ADC_RANGE 4095 // bits of ADC range (for esp32: 4095)
#define MAX_INPUT_VOLTAGE 3.3f // voltage of uC where ADC range maxes out (3.3v for esp32)
#define BATTERY_VOLTAGE_MINIMUM 9.0f // battery is 9.5v at 0% charged
#define BATTERY_VOLTAGE_MAXIMUM 13.4f // battery is 13.3v at 100% charged
#define BATTERY_PERCENTAGE_WARNING 10  // give battery warnings at 10% charge left
#define BATTERY_PERCENTAGE_CRITICAL 2 // shut down matrix at 2% charge left
#define BATTERY_CALIBRATION_OFFSET 1.46f
#define BATTERY_POLL_TIME 5000 // ms between every battery status check
#define LOW_BAT_BRIGHTNESS 30 // 30 out of 255 brightness
#define TOGGLE_BATTERY_WARNING 1 // toggles whether the matrix will give low battery warnings
#define TOGGLE_BATTERY_CRITICAL 1 // toggles whether the matrix and esp will shut down after critical threshold is reached

#define LETTER_WIDTH 5 // Width in pixels of a single letter
#define MATRIX_WIDTH 60 // Width of matrix
//#define MATRIX_WIDTH 5 // Width of matrix prototype board
#define MATRIX_HEIGHT 7 // Height of matrix
#define INITIAL_BRIGHTNESS 30 // 255 full brightness
#define SCROLL_PAUSE 5 // Number of leds (width) of pause between each scroll message repeat
#define DEFAULT_R 100
#define DEFAULT_G 255
#define DEFAULT_B 150

// Matrix definition prototype board
//Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIX_WIDTH, MATRIX_HEIGHT, DATA_PIN, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE, NEO_GRB + NEO_KHZ800);

// Matrix definition
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIX_WIDTH, MATRIX_HEIGHT, DATA_PIN, NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG, NEO_GRB + NEO_KHZ800);

// rgb_colors[0] -> red, rgb_colors[1] -> green, rgb_colors[2] -> blue
const uint16_t rgb_colors[] = {matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };

// setup EEPROM for saving settings and password
Preferences eeprom;

/******************** Web Server ************************/
const char* ssid     = "LED Matrix Display";
const char* password = "123456789"; // default password

AsyncWebServer server(80); // Set web server port number to 80

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
/********************************************************/
/***************** Global Variables *********************/

// Setup
String user_password{}; // Default to default password
String ip_string{}; // Holds IP thats assigned to AP
bool button_toggle_led{};

// HTML GETS from string inputs
const char* INPUT_STRING_TEXT = "input_text";
const char* INPUT_STRING_TEXT2 = "input_text2";
const char* INPUT_STRING_SPEED = "input_speed";
const char* INPUT_STRING_BR_SLIDER = "scroll_value";
const char* INPUT_STRING_CLR_RED = "input_red";
const char* INPUT_STRING_CLR_GREEN = "input_green";
const char* INPUT_STRING_CLR_BLUE = "input_blue";
const char* INPUT_STRING_PASSWORD_OLD = "input_password_old";
const char* INPUT_STRING_PASSWORD_NEW = "input_password_new";
int sliderValue{INITIAL_BRIGHTNESS};

// Loop
auto x_pos = matrix.width(); // cursor position for scroll programs
int pass_colors{0}; // used to pass colors in the random colors scroll
int prev_time{0};
int prev_time_bat{0}; // used for battery polling
int prev_time_show_batt{0};
float voltage{0}; // holds measured battery voltage
int percentage{100}; // holds battery percentage mapped from the battery voltage using the min and max battery voltage constants
bool change_brightness{true}; // default true so brightness gets set once at startup
bool text_toggle{true}; // Bool to keep track of which text is toggled, start with true (text 1)
int battery_show_count{0};

int matrix_case = CASE_SCROLL_COLOR_SINGLE; // use int for switch case so it can be saved in eeprom, see case_defines.h
int prev_matrix_case{};
String display_text{};
String display_text2{};
float wait_time{1};
int color_r{DEFAULT_R};
int color_r_prev{DEFAULT_R};
int color_g{DEFAULT_G};
int color_g_prev{DEFAULT_G};
int color_b{DEFAULT_B};
int color_b_prev{DEFAULT_B};
bool color_error{false};

/*******************************************************/
void setup() 
{
  Serial.begin(9600);
  eeprom.begin("webmatrix_mem", false); // namespace name

  // Read password from EEPROM
  user_password = eeprom.getString("wifi_password");
  if(user_password == "")
    user_password = password; // default pass if none in EEPROM
  //Serial.println("Password set: " + user_password);
  
  // Initialize matrix
  matrix.begin();
  matrix.setTextWrap(false);

  // Initialize the output variables as outputs
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(VOLTAGE_PIN, INPUT);
  digitalWrite(LED_BUILTIN, LOW);

  /*********** Web server setup ***********/

  Serial.print("\n----------------\nSetting AP (Access Point)…"); // Connect to Wi-Fi network with SSID and password
  WiFi.softAP(ssid, user_password.c_str());

  IPAddress IP = WiFi.softAPIP();
  Serial.print(" AP IP address: ");
  Serial.println(IP);
  ip_string = IP.toString();  
  display_text = ip_string; // Default input message to ip_string

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/page2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // GET: HTTP /get string input requests
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request){
    String input_message_text{};
    String input_message_text2{};
    String input_message_speed{};
    String input_message_r{};
    String input_message_g{};
    String input_message_b{};
    String input_message_password_old{};
    String input_message_password_new{};
    
    // GET input_text value on <ESP_IP>/get?input_text=<inputMessage>
    if (request->hasParam(INPUT_STRING_TEXT)) {
      input_message_text = request->getParam(INPUT_STRING_TEXT)->value();
      // Input error check
      bool error = check_valid_input_text(input_message_text);
      if(error) {
        request->send(200, "text/html", "Error, input must be ASCII<br><br><a href=\"/\">Return to Home Page</a>");
        Serial.println("Error in input_text");
      } else {
        display_text = input_message_text;
      }
      Serial.println("Received from client display text: " + input_message_text);
    } else {
      input_message_text = "No message sent";
    }

    // GET input_text2 
    if (request->hasParam(INPUT_STRING_TEXT2)) {
      input_message_text2 = request->getParam(INPUT_STRING_TEXT2)->value();
      // Input error check
      bool error = check_valid_input_text(input_message_text2);
      if(error) {
        request->send(200, "text/html", "Error, input must be ASCII<br><br><a href=\"/\">Return to Home Page</a>");
        Serial.println("Error in input_text2");
      } else {
        display_text2 = input_message_text2;
      }
      Serial.println("Received from client display text2: " + input_message_text2);
    } else {
      input_message_text2 = "No message sent";
    }
    
    // GET input_scroll value on <ESP_IP>/get?input_speed=<inputMessage>
    if (request->hasParam(INPUT_STRING_SPEED)) {
      input_message_speed = request->getParam(INPUT_STRING_SPEED)->value();
      // Input error check
      bool error = check_valid_speed(input_message_speed);
      if(error) {
        request->send(200, "text/html", "Error, input must be numeric and use \".\" for decimals<br><br><a href=\"/\">Return to Home Page</a>");
        Serial.println("Error in input_speed");
      } else {
        wait_time = input_message_speed.toFloat(); // Set scroll_speed to the converted int of the string message
      }
      Serial.println("Received from client scroll: " + input_message_speed);
    } else {
      input_message_speed = "No message sent";
    }

    // GET color RED values
    if (request->hasParam(INPUT_STRING_CLR_RED)) {
      input_message_r = request->getParam(INPUT_STRING_CLR_RED)->value();
      // Input error check
      color_r_prev = color_r;
      bool error = check_valid_rgb(input_message_r);
      if(error) {
        color_error = true; // global error check, avoid sending other colors
        request->send(200, "text/html", "Error, input must be a number between 0 and 255<br><br><a href=\"/\">Return to Home Page</a>");
        Serial.println("Error in input_red");
      } else {
        color_r = input_message_r.toInt();
        Serial.println("Received from client red: " + input_message_r);
        Serial.print("color_r is: ");
        Serial.println(color_r);
      }
    } else {
      input_message_r = "No message sent";
    }
    // GET color GREEN values
    if (request->hasParam(INPUT_STRING_CLR_GREEN)) {
      input_message_g = request->getParam(INPUT_STRING_CLR_GREEN)->value();
      // Input error check
      color_g_prev = color_g;
      bool error = check_valid_rgb(input_message_g);
      if(error) {
        color_error = true; // global error check, avoid sending other colors
        request->send(200, "text/html", "Error, input must be a number between 0 and 255<br><br><a href=\"/\">Return to Home Page</a>");
        Serial.println("Error in input_green");
      } else {
        color_g = input_message_g.toInt();
      }
      Serial.println("Received from client green: " + input_message_g);
      Serial.print("color_g is: ");
      Serial.println(color_g);
    } else {
      input_message_g = "No message sent";
    }
    // GET color BLUE values
    if (request->hasParam(INPUT_STRING_CLR_BLUE)) {
      input_message_b = request->getParam(INPUT_STRING_CLR_BLUE)->value();
      // Input error check
      color_b_prev = color_b;
      bool error = check_valid_rgb(input_message_b);
      if(error) {
        color_error = true; // global error check, avoid sending other colors
        request->send(200, "text/html", "Error, input must be a number between 0 and 255<br><br><a href=\"/\">Return to Home Page</a>");
        Serial.println("Error in input_blue");
      } else {
        color_b = input_message_b.toInt();
      }
      Serial.println("Received from client blue: " + input_message_b);
      Serial.print("color_b is: ");
      Serial.println(color_b);
    } else {
      input_message_b = "No message sent";
    }
    // GET wifi password
    if (request->hasParam(INPUT_STRING_PASSWORD_OLD)) {
      input_message_password_old = request->getParam(INPUT_STRING_PASSWORD_OLD)->value();
      Serial.println("Received from client old_password: " + input_message_password_old);
    } else {
      input_message_password_old = "No message sent";
    }
    if (request->hasParam(INPUT_STRING_PASSWORD_NEW)) {
      input_message_password_new = request->getParam(INPUT_STRING_PASSWORD_NEW)->value();
      Serial.println("Received from client password: " + input_message_password_new);
      Serial.println("Password length is: ");
      Serial.println(input_message_password_new.length());
      bool error = false;

      // Check if old and new password match
      if(input_message_password_old != user_password){
        request->send(200, "text/html", "Error, current password doesn't match.<br><br><a href=\"/\">Return to Home Page</a>");
      } else {
        // Check if wifi password is <= 32 characters and >= 8 and if it's ascii
        if((input_message_password_new.length() <= 32) && (input_message_password_new.length() >= 8)) {
          for(int i = 0; i < input_message_password_new.length(); i++) {
            if(!isAscii(input_message_password_new.c_str()[i])) {
              error = true;
            }
          }
        } else {
          error = true;
        }
        if(error){
          Serial.println("ERROR WITH PASSWORD");
          request->send(200, "text/html", "Error, password must be between 8 and 32 characters<br> and must only contain ASCII<br><br><a href=\"/\">Return to Home Page</a>");
        } else {
          eeprom.putString("wifi_password", input_message_password_new);
          Serial.println("Password written to EEPROM: " + input_message_password_new);
          request->send(200, "text/html", "Password set<br> Please restart device for changes to apply<br><br><a href=\"/\">Return to Home Page</a>");
        }
      }
    } else {
      input_message_password_new = "No message sent";
    }
    request->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL=/\"/>"); // Refresh page on receiving a message from client
  });
 
  // GET: HTTP /ButtonRequest requests
  server.on("/ButtonRequestDebug", HTTP_GET, [](AsyncWebServerRequest *request){
    // Toggle LED if ButtonRequestDebug javascript function is run by the button 
    if(!button_toggle_led){
      digitalWrite(LED_BUILTIN, HIGH);
      button_toggle_led = 1;
      matrix_case = CASE_DEBUG;
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      button_toggle_led = 0;
    }
    Serial.println("Debug button pressed!");
    // Still send OK to avoid client resending the button press in case javascript refreshes page too slowly
    request->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL=/\"/>"); 
  }); 

  server.on("/ButtonRequestScrollSingle", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Scroll single button pressed!");
    matrix_case = CASE_SCROLL_COLOR_SINGLE;
    // Still send OK to avoid client resending the button press in case javascript refreshes page too slowly
    request->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL=/\"/>");
  }); 

  server.on("/ButtonRequestScrollRGB", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Scroll RGB button pressed!");
    matrix_case = CASE_SCROLL_COLORS_RGB;
    request->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL=/\"/>");
  }); 

  server.on("/ButtonRequestFlash", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Flash button pressed!");
    text_toggle = true;
    matrix_case = CASE_FLASH;
    request->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL=/\"/>");
  }); 

  server.on("/ButtonRequestSaveSettings", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Save settings button pressed!");
    eeprom.putInt("matrix_case", matrix_case);
    eeprom.putString("display_text", display_text);
    eeprom.putString("display_text2", display_text2);
    eeprom.putFloat("wait_time", wait_time);
    eeprom.putInt("color_r", color_r);
    eeprom.putInt("color_g", color_g);
    eeprom.putInt("color_b", color_b);
    eeprom.putInt("slider_value", sliderValue);
    request->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL=/\"/>");
  }); 

  server.on("/ButtonRequestLoadSettings", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Load settings button pressed!");
    matrix_case = eeprom.getInt("matrix_case");
    display_text = eeprom.getString("display_text");
    display_text2 = eeprom.getString("display_text2");
    wait_time = eeprom.getFloat("wait_time");
    color_r = eeprom.getInt("color_r");
    color_g = eeprom.getInt("color_g");
    color_b = eeprom.getInt("color_b");
    sliderValue = eeprom.getInt("slider_value");
    request->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL=/\"/>");
  }); 

  server.on("/ButtonRequestBattery", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Show battery button pressed!");
    text_toggle = true;
    prev_matrix_case = matrix_case; // save current case before going into show voltage case
    matrix_case = CASE_SHOW_VOLTAGE;
    request->send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL=/\"/>");
  }); 

  // GET brightness slider value
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String input_message;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(INPUT_STRING_BR_SLIDER)) {
      input_message = request->getParam(INPUT_STRING_BR_SLIDER)->value();
      change_brightness = true;
      // Use an exponential curve for the slider value
      sliderValue = pow(0.1 * input_message.toInt(), 2) * 2.55f;
    } else {
      input_message = "No message sent";
    }
    Serial.println("Received from client display scroll: " + input_message);
    request->send(200, "text/plain", "OK");
  });

  server.onNotFound(notFound);
  server.begin();
}

void loop() 
{
  // Reset colors to old colors in case of an input error
  if(color_error) {
    color_r = color_r_prev;
    Serial.print("debug color_r: "); Serial.println(color_r);
    color_g = color_g_prev;
    Serial.print("debug color_g: "); Serial.println(color_g);
    color_b = color_b_prev;
    Serial.print("debug color_b: "); Serial.println(color_b);
    color_error = false; // reset global color error
  }

  auto curr_time = millis();
  
  // Change brightness if bool is set
  if(change_brightness){
    matrix.setBrightness(sliderValue);
    matrix.show();
    change_brightness = false; // reset bool
    Serial.print("Brightness changed: ");
    Serial.println(sliderValue);
  }

  // Check battery status
  if(curr_time - prev_time_bat >= BATTERY_POLL_TIME){
    voltage = MAX_INPUT_VOLTAGE * (analogRead(VOLTAGE_PIN)/(float)ADC_RANGE) * VOLTAGE_DIVIDER_RATIO + BATTERY_CALIBRATION_OFFSET;
    percentage = mapFloat(voltage, BATTERY_VOLTAGE_MINIMUM, BATTERY_VOLTAGE_MAXIMUM, 0.0, 100.0);    
    if(TOGGLE_BATTERY_WARNING) {
      if(percentage <= BATTERY_PERCENTAGE_WARNING) {
        if(matrix.getBrightness() != LOW_BAT_BRIGHTNESS){
          matrix.setBrightness(LOW_BAT_BRIGHTNESS); // set brightness only if its not set already
          Serial.println("LOW BAT: Reduced brightness!");
        }
        matrix_case = CASE_LOW_BATTERY;
      }
      if(TOGGLE_BATTERY_CRITICAL && (percentage <= BATTERY_PERCENTAGE_CRITICAL) ) {
        matrix_case = CASE_DEEP_SLEEP;
      }
    }
    prev_time_bat = curr_time; // Reset timer
  }
 
  switch(matrix_case) {
    case CASE_SCROLL_COLORS_RGB:
    {
      float scroll_speed = (1000/LETTER_WIDTH) * (1/wait_time); // convert wait time to letters per second scroll speed

      if(curr_time - prev_time >= scroll_speed){
        int message_length = display_text.length() * 6 - 1;
        matrix.fillScreen(0); // Backlight behind letters
        matrix.setCursor(x_pos, 0);
        matrix.print(display_text);
        // scroll text
        if(--x_pos < -message_length) {
          x_pos = matrix.width(); // reset cursor
          if(++pass_colors >= 3) pass_colors = 0;
            matrix.setTextColor(rgb_colors[pass_colors]);
          if(++pass_colors >= 3) pass_colors = 0;
            matrix.setTextColor(rgb_colors[pass_colors]);
        }
        matrix.show();

        prev_time = curr_time; // Reset timer
      }
      break;
    }
    case CASE_SCROLL_COLOR_SINGLE:
    {
      float scroll_speed = (1000/LETTER_WIDTH) * (1/wait_time); // convert wait time to letters per second scroll speed
      
      if(curr_time - prev_time >= scroll_speed){
        int message_length = display_text.length() * 6 - 1;
        matrix.fillScreen(0); // Backlight behind letters
        matrix.setCursor(x_pos, 0);
        matrix.setTextColor(matrix.Color(color_r, color_g, color_b)); // Default color blue
        matrix.print(display_text);
        if(--x_pos < -message_length) {
          x_pos = matrix.width();
        }
        matrix.show();

        prev_time = curr_time; // Reset timer
      }
      break;
    }
    case CASE_FLASH:
    {
      float flash_speed = (1000) * wait_time; // convert wait time to letters per second scroll speed
      int message_length1 = display_text.length() * 6;
      int message_length2 = display_text2.length() * 6;
      int middle1 = (MATRIX_WIDTH/2) - (message_length1/2);
      int middle2 = (MATRIX_WIDTH/2) - (message_length2/2);

      if(curr_time - prev_time >= flash_speed){
        matrix.fillScreen(0); // Backlight behind letters
        matrix.setTextColor(matrix.Color(color_r, color_g, color_b));
        if(text_toggle){
          matrix.setCursor(middle1, 0);
          matrix.print(display_text);
          matrix.show();
          text_toggle = !text_toggle;
        } else {
          matrix.setCursor(middle2, 0);
          matrix.print(display_text2);
          matrix.show();
          text_toggle = !text_toggle;
        }
        prev_time = curr_time; // Reset timer
      }
        
      break;
    }
    case CASE_DEBUG:
    {
      float flash_speed = (1000) * wait_time;

      if(curr_time - prev_time >= flash_speed){
        matrix.fillScreen(matrix.Color(255,255,255));
        matrix.show();
        prev_time = curr_time; // Reset timer
      }
      break;
    }
    case CASE_SHOW_VOLTAGE:
    {
      String voltage_string = String(voltage,2); 
      String percentage_string = String(percentage);
      
      int message_length_voltage = voltage_string.length() * 6;
      int message_length_percentage = percentage_string.length() * 6;
      int middle_voltage = (MATRIX_WIDTH/2) - (message_length_voltage/2);
      int middle_percentage = (MATRIX_WIDTH/2) - (message_length_percentage/2);

      int wait_for_msec = 1000;

      if(curr_time - prev_time >= wait_for_msec){
        battery_show_count++;
        Serial.println("Measured voltage: " + voltage_string);
        Serial.println("Measured voltage percentage: " + percentage_string);
        matrix.fillScreen(0); // Backlight behind letters
        matrix.setTextColor(matrix.Color(DEFAULT_R, DEFAULT_G, DEFAULT_B));
        if(text_toggle){
          matrix.setCursor(middle_voltage, 0);
          matrix.print(voltage_string + "V");
          matrix.show();
          text_toggle = !text_toggle;
        } else {
          matrix.setCursor(middle_percentage, 0);
          matrix.print(percentage_string + "%");
          matrix.show();
          text_toggle = !text_toggle;
        }
        if(battery_show_count > 6) {
          battery_show_count = 0;
          matrix_case = prev_matrix_case; // return to previous case
        }
        prev_time = curr_time; // Reset timer
      }

      break;
    }
    case CASE_LOW_BATTERY:
    {
      String message1 = "LOW BAT";
      String message2 = String(percentage);
      String voltage_string = String(voltage,2);
      String message3 = message2 + "% " + voltage_string + "V";

      int message_length1 = message1.length() * 6;
      int message_length2 = message3.length() * 6;
      int middle1 = (MATRIX_WIDTH/2) - (message_length1/2);
      int middle2 = (MATRIX_WIDTH/2) - (message_length2/2);

      int wait_for_msec = 2000;

      if(curr_time - prev_time >= wait_for_msec){
        matrix.fillScreen(0); // Backlight behind letters
        matrix.setTextColor(matrix.Color(255, 0, 0));
        if(text_toggle){
          matrix.setCursor(middle1, 0);
          matrix.print(message1);
          Serial.println(message1);
          matrix.show();
          text_toggle = !text_toggle;
        } else {
          matrix.setCursor(middle2, 0);
          matrix.print(message3);
          Serial.println(message2 + "%");
          matrix.show();
          text_toggle = !text_toggle;
        }
        if(percentage > BATTERY_PERCENTAGE_WARNING) {
          matrix_case = CASE_SCROLL_COLOR_SINGLE; // edge case, so no need to remember previous case
        }
        prev_time = curr_time; // Reset timer
      }
      break;
    }
    case CASE_DEEP_SLEEP:
    {
      Serial.println("!!!!BATTERY EMPTY SHUTTING DOWN ESP32 TO PREVENT OVERDISCHARGE!!!!");
      matrix.setTextColor(matrix.Color(0, 0, 0));
      matrix.setBrightness(0);
      matrix.show();
      delay(3000);
      ESP.deepSleep(4294967295); // sleep for 71.58 minutes (max int)    
      break;
    }
    default:
      break;  
  }
}