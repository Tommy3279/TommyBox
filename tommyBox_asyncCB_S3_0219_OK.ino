//INCLUDE ========================================================================

//HARDWARE =======================================================================
//RS485 Tx 17, RX 18
//LCD I2C SDA 8; SCL 9

// ESP32_S3_DevKit_1 connections
// ESP32_S3 SCK pin GPIO12  to MFRC522 card  SCK
// ESP32_S3 MISO pin GPIO13  to MFRC522 card  MISO
// ESP32_S3 MOSI pin GPIO11  to MFRC522 card  MOSI
// ESP32_S3 SS  pin GPIO 10   to MFRC522 card SS (marked SDA on PCB)
// connect ESP32_S3 GND and 3.3V to MFRC522 GND and 3V3
// RST 21

//5.2.18 simplify

#include <LCDI2C_Multilingual.h>
#include <Keypad.h>
#include "C:\Users\ADMIN\Documents\Arduino\libraries\Custom2\src\Custom2Master.h"
#include "C:\Users\ADMIN\Documents\Arduino\libraries\Custom2\src\Custom2Master.cpp"
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
//CONNECT =======================================================================
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#include <Time.h>

//DATABASE ======================================================================
#include <FirebaseClient.h>
#include <ArduinoJson.h>
#include <FirebaseJson.h>

//OTHER =========================================================================
#include <UUID.h>
UUID uuid;

//KHỞI TẠO HARDWARE =============================================================
// Khởi tạo màn hình LCD
LCDI2C_Vietnamese lcd(0x27, 16, 2);  // I2C address: 0x27; Display size: 16x2

// Khởi tạo bàn phím
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {1, 2, 15, 14};
byte colPins[COLS] = {4, 5, 6, 7};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Define RS485 pins
const int RS485_TX = 20; 
const int RS485_RX = 19;

// instantiate Custom1 object
Custom2Master node;

// RC522
#define RST_PIN         21
#define SS_PIN          10

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

//KHỞI TẠO WF ================================================================



//DATABASE ================================================================
#define API_KEY "AIzaSyBfD6Opp7FoO0ZkIVObf5tneDzkZLBjELw"
#define DATABASE_URL "https://tommybox1-8d4ea-default-rtdb.firebaseio.com/"
#define USER_EMAIL "hoangngoctuanvt@gmail.com" 
#define USER_PASSWORD "b0001@2025"


//OTHER =========================================================================
//time
time_t now;
tm tm;
/* Configuration of NTP */
#define MY_TZ 25200 // GMT offset for +07:00 in seconds
#define MY_DST 3600  // Daylight Saving Time offset in seconds (if applicable)
#define MY_NTP_SERVER "vn.pool.ntp.org" 
//DATABASE =====================================================================
void asyncCB(AsyncResult &aResult);
void printResult(AsyncResult &aResult);
DefaultNetwork network;
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;

#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client, getNetwork(network));
RealtimeDatabase Database;

// Biến toàn cục
//HARDWARE =====================================================================

char tu[4] = {'A','B','C','D'};   //Bắt đầu từ A=0
char phoneNumber[11] = "";  // Biến chứa Số điện thoại nhập từ bàn phím

enum State {
  OK_STATE,
  FULL_STATE,
  FAULT_STATE
};
State tuTrangThai[sizeof(tu)];

bool openAddress[4];  //Arr chua cpm co the mo

String content= "";     //Biến chứa mã thẻ RFID
const String boxId = "b0001";
const String updateStr = "picked";
String dlvOrder = "";
String dlvStatus = "";
String URFIDpath = "";
String querypath = "";
String uPNumber = "";
String userRFID = "";
 
const bool state485 = true;
bool requestFlag = false;  
bool openFlag = false;
bool dlvFlag = false;
bool readFlag = false;
bool count = false; //Biến phụ in ra LCD khi cảnh báo tủ đang mở
bool isEEPROMInitialized = false; // Flag to track if EEPROM has been initialized
bool backlightOn = true; // Start with backlight on
bool uRFIDverifiedFlag = false; // Flag to verified if scanned RFID matched.
bool ok2Open = false;

const uint8_t slave = 1;
uint8_t result485;
uint8_t receivedData[16];
uint8_t responseLength = 11;

uint8_t writeAddress = 0;   //Số compartment, bắt đầu từ 1
uint8_t readAddress = 0;
uint8_t orderQty ;    //Số lượng đơn của cùng 1 user trong 1 box

unsigned long previousBlinkMillis = 0;
unsigned long previousMillis = 0;
unsigned long openMillis = 0;
unsigned long blink = 0;
const unsigned long blinkInterval = 500;
const unsigned long backLightInterval = 10000; // 10 giây
const unsigned long timeout = 2000; // Thời gian chờ 2 giây (2000 milliseconds)
const unsigned long openInterval = 10000; // Thời gian chờ 30 giây
const uint8_t EEPROM_ADDRESS = 0;
const int EEPROM_INIT_FLAG_ADDRESS = sizeof(tu);
// Khai bao man hinh
enum Screen {
  HOME_SCREEN,		//0
  INPUT_SCREEN,		//1
  CHECK_SCREEN		//2
};

Screen currentScreen = HOME_SCREEN;
//Transfer
void preTransmission(){}
void postTransmission(){}

String dlvStatusPath = "";
String senderId = "keypad";   // Tạm thời mặc định
uint8_t cpmId = 0;    //Biến lưu tủ để tạo lệnh mở


// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t connectTimeoutMs = 5000;
//SETUP ========================================================================

void setup() {
  lcd.init();
  Serial.begin(9600);
  node.receive();
  previousMillis = millis();
  Serial2.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  EEPROM.begin(sizeof(tu)+1);
  Wire.begin();
  readEEPROMtoTuTrangThai();  // khi khởi động lên - Đọc trạng thái cuối cùng
  Serial.print("EEPROM iniitialized: ");
  Serial.println(isEEPROMInitialized);  
//GIAO-NHAN Check if EEPROM has been initialized
  if (!isEEPROMInitialized) {
    // Save status to EEPROM
    for (int i = 0; i < sizeof(tu); i++) {
      tuTrangThai[i] = OK_STATE;
      EEPROM.write(EEPROM_ADDRESS + i, OK_STATE);
    }
    EEPROM.write(EEPROM_INIT_FLAG_ADDRESS, 1);
    EEPROM.commit(); 
    isEEPROMInitialized = true;
    Serial.println("EEPROM Initialized");
  }
//HW - INIT RFD reader, RS485
  SPI.begin(12, 13, 11, 10);      // Initiate  SPI bus - SPI.begin(SCK, MISO, MOSI, SS);
  mfrc522.PCD_Init();   // Initiate MFRC522 

  node.begin(slave, Serial2);  // Modbus slave ID 1
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

//HW - KẾT NỐI WF và NTP ======================================================================
  // Don't save WiFi configuration in flash - optional
  WiFi.persistent(false);
  Serial.begin(9600);
//HW - TIME AND WF SETUP	-------------------------------------------------------------------
	WiFi.mode(WIFI_STA); // Set WiFi to station mode
	// Register multi WiFi networks
	wifiMulti.addAP("Hoang Tuan", "09021989");
  wifiMulti.addAP("TheNest202", "202987654321");
	wifiMulti.addAP("TheNest206", "206987654321");
	wifiMulti.addAP("TheNest203", "203987654321");

  // Maintain WiFi connection
    if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
      Serial.print("WiFi connected: ");
      Serial.print(WiFi.SSID());
      Serial.print(" ");
      Serial.println(WiFi.localIP());
      Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    } else {
        Serial.println("WiFi not connected!");
    }

    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    ssl_client.setInsecure();

    Serial.println("Initializing the app...");
    initializeApp(aClient, app, getAuth(user_auth), asyncCB, "authTask");
//Database init
    // Binding the FirebaseApp for authentication handler.
    // To unbind, use Database.resetApp();
    app.getApp<RealtimeDatabase>(Database);

    // Set your database URL (requires only for Realtime Database)
    Database.url(DATABASE_URL);
    
    configTime(MY_TZ, MY_DST, MY_NTP_SERVER); 

    Serial.println();
//Lcd


// Handle the NTP connecton
    Serial.print("Waiting for NTP time sync...");
    int retries = 0;
    const int maxRetries = 60;
    while (retries < maxRetries) {
      delay(1000);
      Serial.print(".");
      retries++;
      if (time(&now)>=45000) { 
      break; // Exit the loop if time is successfully set
      }
    }
    if (retries >= maxRetries) {
      Serial.println("Failed to synchronize time after several retries.");
      // Handle failure (e.g., restart ESP8266)
      esp_restart();
    } else {
      Serial.println("NTP time synchronized successfully");
  }

  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  TỦ GIAO NHẬN  ");
  lcd.setCursor(0, 1);
  lcd.print("   THÔNG MINH   ");
}
//FINISH SETUP.
void loop() {
// GIAO - NHAN Kiểm tra response modbus ----------------------------------------------------------------
  if (requestFlag) {   //Nếu có lệnh mở thì mới check response
    responseCheck(); //Gọi CT check response
  }

//HW - Kiểm tra phím bấm --------------------------------------------------------------------------
  char key = keypad.getKey();
  if (key) {
    lcd.backlight();
    previousMillis = millis();
    switch (currentScreen) {
      case HOME_SCREEN:
        homeScreen1();
        homeScreen(key);
        break;
      case INPUT_SCREEN:
        inputScreen(key);
        break;
      case CHECK_SCREEN:
        currentScreen = HOME_SCREEN;
        lcd.clear();
        break;
    }
} 
// Kiểm tra flag tủ đang mở-------------------------------------------------------------------
  if (openFlag) {   //Kiểm tra thời gian tủ mở, nếu quá thời gian thì cảnh báo
    if (millis() - openMillis >= openInterval) { //Nếu 30s không đóng cửa thì cảnh báo nhấp nháy backlight
      // Blink LCD backlight at 2Hz (every 500ms)
      count = true; //Biến phụ để in ra LCD khi cảnh báo tủ đang mở
      if (millis() - previousBlinkMillis >= blinkInterval) {
        backlightOn = !backlightOn; // Toggle backlight state
        if (backlightOn) {
          lcd.backlight();
        } else {
          lcd.noBacklight();
        }
        previousBlinkMillis = millis(); // Update the time of the last blink
      }
    }
      if (count == true){
        lcd.setCursor(0, 0);
        lcd.print("Hãy đóng tủ     ");
        lcd.setCursor(0, 1);
        lcd.print("sau khi xong    ");
        count = false;    //Reset biến phụ
        previousMillis = millis();
      }
    }
//Nếu 30s không bấm nút thì tắt đèn nền  -----------------------------------------------------
  if (millis()-previousMillis >= backLightInterval) {
    memset(phoneNumber, '\0', sizeof(phoneNumber)); 
    lcd.clear();
    lcd.noBacklight(); // Tắt đèn nền
    //memset(phoneNumber, '\0', sizeof(phoneNumber)); 
    currentScreen = HOME_SCREEN;
    previousMillis = millis();
  }
// 1# NHẬN Check & verfy RFID ---------------------------------------------------------------------------------
  RFIDtagScan(content);
  if (readFlag) {		//If last cycle already read card RFID
    readFlag = false;	
    //getURFID-Get user RFID from DB to compare
	Serial.println(content);
    DatabaseOptions options;

    options.filter.orderBy("dlvStatus").equalTo("placed").limitToFirst(5);
    String URFIDpath = "/dlvOrders/" + boxId;
    Database.get(aClient, URFIDpath, options, asyncCB, "uRFID");
  } 
//Database -----------------------------
  app.loop();
  Database.loop();
//If last cycle confirmed ok2Open==========================================================================
  if (ok2Open) {
    Serial.println("OK2Open");
    ok2Open = false;
      //Reset các biến lên quan đến nhận
      content = "";

      //Cho phép cycle sau đọc response
      responseLength = 11;
      requestFlag = true;	//Cho phép đọc response
      dlvFlag = true;		//Cho phép vào function xử lý giao nhận.
      result485 = node.writeSingleCoil(writeAddress, state485);    //Gửi lệnh mở
      lcd.setCursor(0, 0);
      lcd.print("   Đang mở tủ:  ");
      lcd.print(tu[writeAddress]);
      previousMillis = millis();
      if (orderQty - 1 > 0) {
        lcd.print("Bạn còn ");
        lcd.print(orderQty - 1);
        lcd.print("đơn hàng");	    
      }
      orderQty = 0;
  }
}
//END MAIN LOOP ----------------------------------------------------------------------------------

// 1#GIAO ===============================================================================================
void homeScreen1() {
//Display trạng thái các ngăn tủ
	char Mc[2] = {2,11};
	char Mr[2] = {0,1};
  lcd.clear();
  // Hiển thị các tủ
	for (int i = 0; i < 4; i++) {
		lcd.setCursor(Mc[i / 2]-2, Mr[i % 2]); 
		lcd.print(tu[i]); 
	}
  //Load lại trạng thái tủ
  readEEPROMtoTuTrangThai();
	for (int c = 0; c < 2; c++) {
		for (int r = 0; r < 2; r++) {
      lcd.setCursor(Mc[c],Mr[r]);
			switch (tuTrangThai[c*2+r]) {
				case OK_STATE:
					lcd.print("OK");
					break;
				case FULL_STATE:
					lcd.print("ĐẦY");
					break;
				case FAULT_STATE:
					lcd.print("LỖI");
					break;
				default:
					lcd.print("Uknwn");
					break;
			}
		}
	}
}

void homeScreen(char key) {
  // Xử lý các sự kiện trên màn hình Home
  if (key >= 'A' && key <= 'D') {
	  lcd.backlight();
    if (tuTrangThai[key - 'A'] == 0) { //OK_STATE
      writeAddress = key - 'A' + 1; //Xác định tủ tương ứng với nút bấm ABCD
      currentScreen = INPUT_SCREEN;
      inputScreen(key);
    } else if (tuTrangThai[key - 'A'] == 1) { //FULL_STATE
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Tủ đã đầy");
		lcd.setCursor(0, 1);
		lcd.print("Hãy chọn tủ khác");
		delay(2000);
		currentScreen = HOME_SCREEN;
		homeScreen1();
	  } else if (tuTrangThai[key - 'A'] == 2) { //FAULT_STATE
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Tủ khôg làm việc");
		lcd.setCursor(0, 1);
		lcd.print("Hãy chọn tủ khác");
		delay(2000);
		currentScreen = HOME_SCREEN;
		homeScreen1();
	  } else if (key == '*') {
      back();
    }
  }
}

// 1#GIAO Xử lý các sự kiện trên màn hình Input =========================================================
void inputScreen(char key) {
	//lcd.backlight(); // Bật đèn nền
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Nhập SĐT ng nhận");
  if (key >= '0' && key <= '9') {
    // Thêm số vào chuỗi số điện thoại
    sprintf(phoneNumber, "%s%c", phoneNumber, key);
    if (strlen(phoneNumber) <= 10) {  // Giới hạn độ dài
	    lcd.setCursor(0, 1);
      lcd.print(phoneNumber);
      if (strlen(phoneNumber) == 10) {
          lcd.setCursor(0, 0);
          lcd.print("OK - Bấm # để mở");
        }
    } else {
      lcd.setCursor(0, 0);
      lcd.print("SĐT quá nhiều số");
      lcd.setCursor(0, 1);
      lcd.print("  Hãy nhập lại !");      
      memset(phoneNumber, '\0', sizeof(phoneNumber)); 
      delay(2000);
      currentScreen = INPUT_SCREEN;
    }
  } else if (key == '*') {
    // Xóa ký tự cuối cùng
    if (strlen(phoneNumber) > 0) {
      phoneNumber[strlen(phoneNumber) - 1] = '\0';
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(phoneNumber);
    } else {
      back();
      }
  } else if (key == '#') {  // Xử lý số điện thoại đã nhập =====================================
    getUPN();
    // Query số điện thoại từ DB
    //So sánh với số nhập từ bàn phím
    //Nếu OK thì gửi lệnh mở và TRUYỀN RFID (content) ra ngoài để sử dụng
    //Nếu SĐT sai thì báo nhập lại
  }
  lcd.setCursor(0, 1);
	lcd.print(phoneNumber);
}

// 2# GIAO NHAN =========================================================================================
//CHECK MODBUS RESPONSE và XỬ LÝ ========================================================================
void responseCheck() { // Chờ response & tách function
  memset(receivedData, 0, sizeof(receivedData)); //Reset receivedData để nhận dữ liệu phản hồi
  while (Serial2.available() < responseLength) {
    delay(10); 
  }

    for (int i = 0; i < responseLength; i++) {
      receivedData[i] = Serial2.read();
    }

    Serial.println(receivedData[6]);
    switch (receivedData[6]){   //Kiểm tra function byte
      case 130:                 //F82 Write Single coil
        Serial.println("82");
        responseLength = 10;
        //Kiểm tra send thành công + slaveID match + address match.
        if (result485 == 0 && receivedData[5] == slave && receivedData[8] == writeAddress){
          Serial.print("Tủ ");
          Serial.print(receivedData[8]);
          switch (receivedData[9]){
            case 0x00:
              Serial.println(" Open");
//2# NHAN =================================================================================================			  
              if (dlvFlag){         // Khi nhận hàng	
                lcd.clear();
                lcd.backlight();
                lcd.setCursor(0, 0);
                lcd.print("  HÃY LẤY HÀNG  ");
                lcd.setCursor(0, 1);
                lcd.print("  TỪ TỦ   ");
                lcd.print(tu[writeAddress-1]);
//2# GIAO =================================================================================================	
              } else {            // Khi giao hàng
                lcd.clear();
                lcd.backlight();
                lcd.setCursor(0, 0);
                lcd.print("Hãy giao hàng");
                lcd.setCursor(0, 1);
                lcd.print("vào tủ ");
                lcd.print(tu[writeAddress-1]);
              }
			          openFlag = true;		//Biến báo tủ đang mở
                openMillis = millis();    //biến kiểm tra cửa mở
                previousMillis = millis();	//Biến đếm thời gian mở tủ để cảnh báo
            break;
            case 0x01:
              Serial.println(" Closed");
              //Gửi lệnh mở thành công nhưng tiếp điểm chưa mở
              //Retry
              //Set cờ, nếu retry không thành công thì báo lỗi
              //
            break;
            case 0xFF:
              Serial.println(" Fault");
              //Tiếp đểm báo lỗi
              // Set trạng thái tủ thành FAULT
              tuTrangThai[writeAddress] == FAULT_STATE;
              
            break;
            }
          } else {
            Serial.print("write Single coil Fail!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Lỗi mở tủ ");
            lcd.print(receivedData[8]);		//In địa chỉ
            lcd.setCursor(0, 1);
            lcd.print(result485,HEX);
            }
		    printDev();
        memset(receivedData, 0, sizeof(receivedData)); //Reset receivedData để nhận dữ liệu phản hồi
        break;
      case 131:               //F83 Read Single coil
        Serial.println("83");
        responseLength = 10;
        break;  
      case 132:               //F84 Read discreteInput  57 4B 4C 59 08 01 84 84  => 57 4B 4C 59 0E 01 84 00 04 00 00 00 00 86 
        Serial.println("84");
        responseLength = 10;
        //Kiểm tra lại 
        if (receivedData[5] == slave && receivedData[8] == sizeof(tu)){
            homeScreen1();
          }
        responseLength = 10;
        memset(receivedData, 0, sizeof(receivedData)); // HOAN THANH ORDER, RESET
        break;

      case 133:               //F85 Toggle input - door close/ open   57 4B 4C 59 0A 01 85 01 01 86
        responseLength = 11;
        //Kiểm tra lại 
        if (receivedData[5] == slave && receivedData[7] == writeAddress){
          switch (receivedData[8]) { // Kiểm tra byte 8
            case 0:	//Cửa đang đóng mở ra
              // NÂNG CẤP SAU
              Serial.println("cảnh báo cửa mở đột ngột");
            break;
            case 1:	//Cửa đang mở đóng lại
//3# GIAO NHAN ==============================================================================================
              openFlag = false;	//Reset openFlag => tắt cảnh báo đóng tủ nếu có
//3# NHAN ===================================================================================================      
			  if (dlvFlag){					// Khi NHẬN hàng xong và đóng tủ xong.
				        dlvFlag = false;
                Serial.println("Delivery success!");
                tuTrangThai[writeAddress-1] = OK_STATE;
                lcd.clear();
                lcd.backlight();                
                lcd.setCursor(0, 0);
                lcd.print("   NHẬN HÀNG    ");
                lcd.setCursor(0, 1);
                lcd.print("  THÀNH CÔNG!   ");
                previousMillis = millis();
                //GỌI CT UPDATE dlv trên Database = "Picked" ================================================
                String updateStr;
                // Create a scope for dlvStatusUpdate
                object_t json;
                JsonWriter writer;
                writer.create(json, "dlvStatus", "picked");
        
                Serial.println(json); 

                Database.update(aClient, dlvStatusPath, json, asyncCB, "updateDlvStatus");
//3# GIAO ===================================================================================================
              }else {   //Trường hợp GIAO hàng xong, đóng tủ lại.
                //tạo dlvOrder mới ===========================================================
                dlvOrderGen();
                Serial.println("tạo dlvOrder mới");
                //Set dlvOrder lên DB=========================================================
                setNewOrder();
                tuTrangThai[writeAddress-1] = FULL_STATE;
                lcd.clear();
                lcd.backlight();                
                lcd.setCursor(0, 0);
                lcd.print("  ĐÃ GIAO HÀNG  ");
                lcd.setCursor(0, 1);
                lcd.print("   VÀO TỦ  ");
                lcd.print(tu[writeAddress-1]);
              }
//3# GIAO NHAN ===============================================================================================
              writeEEPROM();
              homeScreen1();
              printDev();
              memset(receivedData, 0, sizeof(receivedData)); // HOAN THANH ORDER, RESET 
              requestFlag = false;
              previousMillis = millis();
              Serial.print("Previous millis = "); //debug
              Serial.println(previousMillis);   //DEBUG
              writeAddress = 0;  //Reset writeAddress     
            break;
		      }
		    } else if (receivedData[7] != writeAddress) {           //Nếu đóng nhầm tủ khác so với tủ đang thao tác
            printDev();
            requestFlag = true;
            responseLength = 10;                             //Tiếp tục chờ response 85
            previousMillis = millis();
          }
        break;
    }
	  //Finish request - response.
	  //IN GLOBAL RA KIEM TRA LẠI
  //}
}
//TẠM THỜI, DEBUG ==========================================================================
void printDev(){
    Serial.print("Transaction Data: ");
    for (int i = 0; i < receivedData[4]; i++) {   //Print frameLength
      Serial.print(receivedData[i], HEX); // Print each byte in hexadecimal format
      Serial.print(" ");
    }
    Serial.println(" ");
  }
//NÂNG CẤP SAU =============================================================================
void doorJammed(){    //Gửi lệnh mở thành công nhưng tiếp điểm chưa mở

  }
//NÂNG CẤP SAU =============================================================================
void doorFault(){     //Tiếp đểm báo lỗi, Set trạng thái tủ thành FAULT

  }
//EEPROM  ==================================================================================
void readEEPROMtoTuTrangThai() {
  isEEPROMInitialized = (EEPROM.read(EEPROM_INIT_FLAG_ADDRESS) == 1);
  // Đọc dữ liệu từ EEPROM và lưu vào mảng tuTrangThai
  for (int i = 0; i < 4; i++) {
    tuTrangThai[i] = static_cast<State>(EEPROM.read(EEPROM_ADDRESS + i));
  }
}

void writeEEPROM() {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(EEPROM_ADDRESS + i, tuTrangThai[i]);
  }
  EEPROM.commit();
}
// BACK LCD ==============================================================================
void back(){
  strcpy(phoneNumber, "");
  lcd.clear();
  lcd.noBacklight(); // Tắt đèn nền
  currentScreen = HOME_SCREEN;
  }

//1.1 NHẬN - RFID SCAN ===============================================================================
void RFIDtagScan(String &content) {
  if (mfrc522.PICC_IsNewCardPresent()) {

    if (mfrc522.PICC_ReadCardSerial()) {

      //Show UID on serial monitor
      Serial.println();
      Serial.print(" UID tag :");
      content= "";
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
          Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
          Serial.print(mfrc522.uid.uidByte[i], HEX);
          content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
          content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      Serial.println();
      // Halt PICC
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      content.toUpperCase();  //RFID card READ ================================================
      
      readFlag = true;  //SetFlag"READ"
    }
  }
}


//FIREBASE ---------------------------------------------------------------------------------
//Log


//SET DLV ORDER MỚI ------------------------------------------------------------------------
void setNewOrder() {
        //Set dlvOrder lên DB
        Serial.print("RFID content");
        Serial.println(content); //DEBUG
        String setPath = "/dlvOrders/" + boxId + "/" + dlvOrder;
        Database.set<String>(aClient, setPath + "/" + "boxId", boxId, asyncCB, "setString1Task");
        Database.set<int>(aClient, setPath + "/" + "compartmentId", uint8_t(writeAddress-1), asyncCB, "setInt2Task");
        Database.set<String>(aClient, setPath + "/" + "dlvStatus", "placed", asyncCB, "setString3Task");    //dlvOrder mới mặc định là "placed"
        Database.set<String>(aClient, setPath + "/" + "receiverId", content, asyncCB, "setString4Task");
        Database.set<String>(aClient, setPath + "/" + "senderId", senderId, asyncCB, "setString5Task");
        time_t now = time(&now); 
        struct tm tm;
        localtime_r(&now, &tm);
        // Format the timestamp as "yyyy-MM-dd HH:mm:ss"
        String timestamp =  String(tm.tm_year + 1900) + "-" + 
                            String(tm.tm_mon + 1, DEC) + "-" + 
                            String(tm.tm_mday, DEC) + " " + 
                            String(tm.tm_hour, DEC) + ":" + 
                            String(tm.tm_min, DEC) + ":" + 
                            String(tm.tm_sec, DEC);
        Database.set<String>(aClient, setPath + "/" + "orderTime", timestamp, asyncCB, "setString6Task");
        dlvOrder = "";
}

//Get phone number trên DB và so sánh với phonenumber nhập từ bàn phím, su đó xử lý nếu khớp/ k khớp---------------------
 void getUPN(){   
    Serial.println("Getting the user Phone number... ");
    String getPath = "/uLock/u" + String(phoneNumber);
    Serial.println(getPath);    //DEBUG
    Database.get(aClient, getPath, asyncCB, false, "getUPN");

}
// TẠO DLVORDER ------------------------------------------------------------------------------
void dlvOrderGen(){   //Tạo dlvOder
    String uuid;
    for (int i = 0; i < 8; i++) {
      uuid += String(random(0, 16), HEX); // Generate a random hexadecimal character
    }
    // Get the UUID as a character array 
    char uuidChar[9]; // Allocate space for 8 characters + null terminator
    uuid.toCharArray(uuidChar, 9); 
    // Convert the character array to a String
    String uuidString = String(uuidChar); 
    // Extract only the last 8 characters of the UUID
    uuidString.toUpperCase();
    //String uuid8Chars = uuidString.substring(uuidString.length() - 8); 
    Serial.print("UUID (sprintf): "); 
    Serial.println(uuidString);

    time(&now);                       // read the current time
    localtime_r(&now, &tm);           // update the structure tm with the current time
    int year = tm.tm_year -100;
    int month = tm.tm_mon + 1;
    // Format year with leading zero if needed
    String yearStr = (year < 10) ? "0" + String(year) : String(year); 
    String monStr = (month < 10) ? "0" + String(month) : String(month); 
    //generate dlvOrder = yyMMUUID8chars
    dlvOrder = yearStr + monStr + uuidString; 
    Serial.print("dlvOrder: ");
    Serial.println(dlvOrder);
}


//CALL BACK -------------------------------------------------------------------------------
void asyncCB(AsyncResult &aResult) {
  printResult(aResult);
	if (aResult.uid() == "getUPN") {
	//VERIFY PHONE NUMBER ------------------------------------------------------------------
    // Parse the JSON payload
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, aResult.c_str());
    if (!error) {
      Serial.println("DEBUG PN - getUPN PARSED");
      	if (aResult.payload().length() <=6 ) {
          Serial.println("Không có người dùng này");
          memset(phoneNumber, '\0', sizeof(phoneNumber)); //Reset biến phoneNumber (bàn phím)
			    lcd.clear();
          lcd.backlight();
			    lcd.setCursor(0, 0);
			    lcd.print("    KHÔNG CÓ    ");
          lcd.setCursor(0, 1);
          lcd.print(" NGƯỜI DÙNG NÀY ");
          
          delay(2000); // Chờ 2 giây
			    currentScreen = HOME_SCREEN;
          previousMillis = millis();
        } else {
          //Extract the "uPNumber" value
			    uPNumber = doc["uPNumber"].as<String>();

			    //Truyền user RFID ra ngoài      
          content = doc["uKey"].as<String>();

          doc.clear();		//FREE MEMORY

			    // Compare with the "phoneNumber"
          if (strcmp(uPNumber.c_str(), phoneNumber) == 0) { //nếu khớp=========================
			      lcd.clear();
			      lcd.setCursor(0, 0);
			      lcd.print("SĐT OK!");
			      memset(phoneNumber, '\0', sizeof(phoneNumber)); //Reset biến phoneNumber (bàn phím)
			      result485 = node.writeSingleCoil(writeAddress, state485);    //Gửi lệnh mở
			      responseLength = 11;
			      requestFlag = true;     //Loop tiếp theo gọi ct response check
          }
			} 

    } else {
		  Serial.print("deserializeJson() failed: ");
		  Serial.println(error.c_str());
		}

//1# NHẬN - CB RFID =============================================================================================
  } else if (aResult.uid() == "uRFID") {
		URFIDpath = "";
		// Parse the JSON payload
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, aResult.c_str());  
        if (!error) {
      	  if (aResult.payload().length() <=6 ) {        //Nếu trả về Null
            Serial.println("KHÔNG CÓ ĐƠN HÀNG!");
            lcd.clear();
            lcd.backlight();
            lcd.setCursor(0, 0);
            lcd.print("  BẠN KHÔNG CÓ  ");
            lcd.setCursor(0, 1);
            lcd.print("  ĐƠN HÀNG NÀO  ");
            delay(2000); // Chờ 2 giây
            currentScreen = HOME_SCREEN;

        } else {										//Nếu không lỗi, không null
          String documentKeys[sizeof(tu)]; // Array to store the found dlvOrders// Adjust the size as needed
          String RFIds[sizeof(tu)];
          uint8_t compartments[sizeof(tu)];
          uint8_t numKeys = 0;  //Biến chứa index của document 
            for (const auto& item : doc.as<JsonObject>()) {
			  
			      //Get dlvOrder item 
              String documentKey = item.key().c_str();
              documentKeys[numKeys] = documentKey;
              documentKey = "";

              //Get RFID of order
              String receiverId = const_cast<JsonPair&>(item).value()["receiverId"].as<String>(); 
              RFIds[numKeys] = receiverId;
              receiverId = "";

              //Get compartmentID of order
              uint8_t compartmentId = 99;

              compartmentId = const_cast<JsonPair&>(item).value()["compartmentId"].as<uint8_t>();
              compartments[numKeys] = compartmentId;
              compartmentId = 99;

              numKeys++;
              }
          doc.clear();		//FREE MEMORY
          //Lấy dữ liệu từ json Obj
          for (int j = 0; j< numKeys; j++) {
            Serial.println(RFIds[j]);
            Serial.println(content);
            if (strcmp(RFIds[j].c_str(), content.c_str()) == 0) {
              //PATH to update the dlvStatus
              dlvStatusPath = "/dlvOrders/" + boxId + "/" + documentKeys[j].c_str();
              //set flag "ok2Open"
              ok2Open = true;
              //Cycle sau ra lệnh mở
              writeAddress = compartments[j]+1; //Địa chỉ tủ sẽ mở=A/0; B/1; c/2; D/3========================================================

            }
            else {
              Serial.println("KHÔNG CÓ ĐƠN HÀNG!");
              lcd.clear();
              lcd.backlight();
              lcd.setCursor(0, 0);
              lcd.print("  BẠN KHÔNG CÓ  ");
              lcd.setCursor(0, 1);
              lcd.print("  ĐƠN HÀNG NÀO  ");
              delay(2000); // Chờ 2 giây
              currentScreen = HOME_SCREEN;
              content = "";
              previousMillis = millis();
              }
          }
          //DEBUG print
          // Print the found dlvOrder
              Serial.print("Found: ");
              Serial.print(numKeys);
              Serial.println(" Order(s): ");
          for (int i = 0; i < numKeys; i++) {
              Serial.print("{ ");
              Serial.print("Order ");
              Serial.print(documentKeys[i]);
              Serial.print(": RFID: ");
              Serial.print(RFIds[i]);
              Serial.print(", Compartment: ");
              Serial.print(compartments[i]);
              Serial.println(" }");
            }

          orderQty = numKeys; //TRUYỀN BIẾN RA NGOÀI ĐỂ xem có bao nhiêu đơn
          content = ""; //Reset
        }
	  } else {
		  doc.clear();		//FREE MEMORY
          Serial.print(F("Order deserializeJson() failed: "));
          Serial.println(error.c_str());
        }
        
// 3# NHAN ======================================================================================== 
	} else if (aResult.uid() == "updateDlvStatus") {
			dlvStatusPath = "";
			Serial.println("Final Step - dlvOrder status updated");
		}
}

//IN KQ CHECK -----------------------------------------------------------------------------
void printResult(AsyncResult &aResult)
{
    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
      Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());

    }
}

