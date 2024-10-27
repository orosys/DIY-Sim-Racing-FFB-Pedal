#include <Arduino.h>
#include <ESP32OTAPull.h>
#include <esp_wifi.h>
#define VERSION "0.0.0"
#if PCB_VERSION==5
	#define JSON_URL   "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/Bridge/Fanatec_Bridge/Version.json"
#endif
#if PCB_VERSION==6
	#define JSON_URL   "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/Bridge/dev_kit/Version.json"
#endif
#if PCB_VERSION==7
	#define JSON_URL   "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/Bridge/Gilphilbert_dongle/Version.json"
#endif

bool OTA_enable_b =false;
bool OTA_status =false;
struct Basic_WIfi_info
{ 
    uint8_t payloadType;
    uint8_t device_ID;
    uint8_t wifi_action;
    uint8_t mode_select;
    uint8_t SSID_Length;
    uint8_t PASS_Length;
    uint8_t WIFI_SSID[30];
    uint8_t WIFI_PASS[30];
};

Basic_WIfi_info _basic_wifi_info;
char* SSID;
char* PASS;

void wifi_initialized(char* Wifi_SSID, char* Wifi_PASS)
{
    Serial.print("[L]SSID: ");
    Serial.print(Wifi_SSID);
    Serial.print(" PASS: ");
    Serial.println(Wifi_PASS);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    //esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
    WiFi.begin(Wifi_SSID, Wifi_PASS);

    // Display connection progress
    Serial.print("[L]Connecting to WiFi");
    
    // Wait until WiFi is connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
  }
  
  // Print confirmation message when WiFi is connected
  Serial.println("WiFi connected");
  Serial.print("[L]WiFi RSSI: ");
  Serial.println(WiFi.RSSI());

}
void OTAcallback(int offset, int totallength);

const char *errtext(int code)
{
	switch(code)
	{
		case ESP32OTAPull::UPDATE_AVAILABLE:
			return "An update is available but wasn't installed";
		case ESP32OTAPull::NO_UPDATE_PROFILE_FOUND:
			return "No profile matches";
		case ESP32OTAPull::NO_UPDATE_AVAILABLE:
			return "Profile matched, but update not applicable";
		case ESP32OTAPull::UPDATE_OK:
			return "An update was done, but no reboot";
		case ESP32OTAPull::HTTP_FAILED:
			return "HTTP GET failure";
		case ESP32OTAPull::WRITE_ERROR:
			return "Write error";
		case ESP32OTAPull::JSON_PROBLEM:
			return "Invalid JSON";
		case ESP32OTAPull::OTA_UPDATE_FAIL:
			return "Update fail (no OTA partition?)";
		default:
			if (code > 0)
				return "Unexpected HTTP response code";
			break;
	}
	return "Unknown error";
}

void DisplayInfo()
{
	char exampleImageURL[256];
	snprintf(exampleImageURL, sizeof(exampleImageURL), "https://example.com/Basic-OTA-Example-%s-%s.bin", ARDUINO_BOARD, VERSION);

	Serial.printf("Basic-OTA-Example v%s\n", VERSION);
	Serial.printf("You need to post a JSON (text) file similar to this:\n");
	Serial.printf("{\n");
	Serial.printf("  \"Configurations\": [\n");
	Serial.printf("    {\n");
	Serial.printf("      \"Board\": \"%s\",\n", ARDUINO_BOARD);
	Serial.printf("      \"Device\": \"%s\",\n", WiFi.macAddress().c_str());
	Serial.printf("      \"Version\": \"%s\",\n", VERSION);
	Serial.printf("      \"URL\": \"%s\"\n", exampleImageURL);
	Serial.printf("    }\n");
	Serial.printf("  ]\n");
	Serial.printf("}\n");
	Serial.printf("\n");
	Serial.printf("(Board, Device, Config, and Version are all *optional*.)\n");
	Serial.printf("\n");
	Serial.printf("Post the JSON at, e.g., %s\n", JSON_URL);
	Serial.printf("Post the compiled bin at, e.g., %s\n\n", exampleImageURL);
}

void OTAcallback(int offset, int totallength)
{
	Serial.print("[L]Updating");
    Serial.print(offset);
    Serial.print(" of ");
    Serial.print(totallength);
    Serial.print("(");
    Serial.print(100 * offset / totallength);
    Serial.println("%)");
    
}