
///*
//  Rui Santos & Sara Santos - Random Nerd Tutorials
//  Complete project details at https://RandomNerdTutorials.com/get-change-esp32-esp8266-mac-address-arduino/
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.  
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//*/
//#include <WiFi.h>
//#include <esp_wifi.h>
//
//void readMacAddress(){
//  uint8_t baseMac[6];
//  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
//  if (ret == ESP_OK) {
//    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
//                  baseMac[0], baseMac[1], baseMac[2],
//                  baseMac[3], baseMac[4], baseMac[5]);
//  } else {
//    Serial.println("Failed to read MAC address");
//  }
//}
//
//void setup(){
//  Serial.begin(115200);
//
//  WiFi.mode(WIFI_STA);
//  WiFi.begin();
//
//  
//}
// 
//void loop(){
//	delay(1000);
//	Serial.print("[DEFAULT] ESP32 Board MAC Address: ");
//  	readMacAddress();
//}



/**********************************************************************************************/
/*                                                                                            */
/*                         controller  definitions                                            */
/*                                                                                            */
/**********************************************************************************************/
#define ACTIVATE_JOYSTICK_OUTPUT
#ifdef ACTIVATE_JOYSTICK_OUTPUT
#include "Controller.h"
#endif

/**********************************************************************************************/
/*                                                                                            */
/*                         ESPNOW definitions                                                 */
/*                                                                                            */
/**********************************************************************************************/
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Set your new MAC Address
//uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};
uint8_t esp_master[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x31};
uint8_t Clu_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x32};
uint8_t Gas_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33};
uint8_t Brk_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x34};


typedef struct struct_message {
    uint64_t cycleCnt_u64;
    int64_t timeSinceBoot_i64;
	int32_t controllerValue_i32;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Callback when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {

//void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
	memcpy(&myData, incomingData, sizeof(myData));
	
	#ifdef ACTIVATE_JOYSTICK_OUTPUT
	// normalize controller output
	int32_t joystickNormalizedToInt32 = NormalizeControllerOutputValue(myData.controllerValue_i32, 0, 10000, 100); 

	// send controller output
	if (IsControllerReady()) 
	{	
		// check whether sender was clutch, brake or throttle

		boolean clutchCheck_b = true;
		boolean brakeCheck_b = true;
		boolean throttleCheck_b = true;

		// Check if sender was brake, thottle or cluth
		//for (uint8_t byteIdx_u8 = 0; byteIdx_u8 < 6; byteIdx_u8++ )
		//{
		//	clutchCheck_b &= info->src_addr[byteIdx_u8] == Clu_mac[byteIdx_u8];
		//	brakeCheck_b &= info->src_addr[byteIdx_u8] == Brk_mac[byteIdx_u8];
		//	throttleCheck_b &= info->src_addr[byteIdx_u8] == Gas_mac[byteIdx_u8];
		//}


		if (clutchCheck_b)
		{
			SetControllerOutputValueAccelerator(joystickNormalizedToInt32);
		}

		if (brakeCheck_b)
		{
			SetControllerOutputValueBrake(joystickNormalizedToInt32);
		}

		if (throttleCheck_b)
		{
			SetControllerOutputValueThrottle(joystickNormalizedToInt32);
		}

		joystickSendState();
		
	}



	#else
	Serial.print("Bytes received: ");
	Serial.println(len);
	Serial.print("CycleCnt: ");
	Serial.println(myData.cycleCnt_u64);
	Serial.print("TimeSinceBoot in ms (shared): ");
	Serial.println(myData.timeSinceBoot_i64);
	Serial.print("controllerValue_i32: ");
	Serial.println(myData.controllerValue_i32);	
	Serial.println();
	#endif


	//Serial.print("Bytes received: ");
	//Serial.println(len);
	//Serial.print("CycleCnt: ");
	//Serial.println(myData.cycleCnt_u64);
	//Serial.print("TimeSinceBoot in ms (shared): ");
	//Serial.println(myData.timeSinceBoot_i64);
	//Serial.print("controllerValue_i32: ");
	//Serial.println(myData.controllerValue_i32);	
	//Serial.println();



}

/**********************************************************************************************/
/*                                                                                            */
/*                         setup function                                                     */
/*                                                                                            */
/**********************************************************************************************/
void setup()
{

	// Initialize Serial Monitor
  	//Serial.begin(115200);
#ifdef ACTIVATE_JOYSTICK_OUTPUT
	SetupController();
#endif
	// https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/
	// https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/

	
	//// Change ESP32 Mac Address
	//esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, &esp_master[0]);
	//if (err == ESP_OK) {
	//	Serial.println("Success changing Mac Address");
	//}
	

	// Set device as a Wi-Fi Station
  	WiFi.mode(WIFI_STA);

	// Init ESP-NOW
	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}

	
	// Register for a callback function that will be called when data is received
	esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
	
}




int32_t joystickNormalizedToInt32_local = 0;






/**********************************************************************************************/
/*                                                                                            */
/*                         Main function                                                      */
/*                                                                                            */
/**********************************************************************************************/
uint64_t cycleCntr_u64 = 0;
void loop() {
	delay(10);
	cycleCntr_u64++;
	Serial.println(cycleCntr_u64);
}