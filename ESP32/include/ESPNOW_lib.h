#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "ESPNowW.h"
//#define ESPNow_debug
//uint8_t esp_master[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x31};
//uint8_t esp_master[] = {0xdc, 0xda, 0x0c, 0x22, 0x8f, 0xd8}; // S3
uint8_t esp_master[] = {0x48, 0x27, 0xe2, 0x59, 0x48, 0xc0}; // S2 mini
uint8_t Clu_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x32};
uint8_t Gas_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33};
uint8_t Brk_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x34};
uint8_t* Recv_mac;
uint16_t ESPNow_send=0;
uint16_t ESPNow_recieve=0;
//bool MAC_get=false;
bool ESPNOW_status =false;
bool ESPNow_initial_status=false;
bool ESPNow_update= false;
//https://github.com/nickgammon/I2C_Anything/tree/master
struct ESPNow_Send_Struct
{ 
  uint16_t pedal_position;
  float pedal_position_ratio;
};
ESPNow_Send_Struct _ESPNow_Recv;
ESPNow_Send_Struct _ESPNow_Send;

void onRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) 
{
  /*
  if(ESPNOW_status)
  {
    memcpy(&ESPNow_recieve, data, sizeof(ESPNow_recieve));
    ESPNow_update=true;
  }
  */
  if(ESPNOW_status)
  {
    memcpy(&_ESPNow_Recv, data, sizeof(_ESPNow_Recv));
    ESPNow_update=true;
  }

}
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{

}



typedef struct struct_message {
    uint64_t cycleCnt_u64;
    int64_t timeSinceBoot_i64;
    int32_t controllerValue_i32;
} struct_message;
struct_message myData;

void sendMessageToMaster(int32_t controllerValue)
{

  myData.cycleCnt_u64++;
  myData.timeSinceBoot_i64 = esp_timer_get_time() / 1000;
  myData.controllerValue_i32 = controllerValue;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(esp_master, (uint8_t *) &myData, sizeof(myData));
   
  /*if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }*/
}



// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //Serial.print("\r\nLast Packet Send Status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void ESPNow_initialize()
{

    /*WiFi.mode(WIFI_MODE_STA);
    Serial.println("Initializing Rudder, please wait"); 
    Serial.print("Current MAC Address:  ");  
    Serial.println(WiFi.macAddress());
    if(dap_config_st.payLoadPedalConfig_.pedal_type==0)
    {
      esp_wifi_set_mac(WIFI_IF_STA, &Clu_mac[0]);
    }
    if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
    {
      esp_wifi_set_mac(WIFI_IF_STA, &Brk_mac[0]);
    }
    if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
    {
      esp_wifi_set_mac(WIFI_IF_STA, &Gas_mac[0]);
    }
    delay(300);
    Serial.print("Modified MAC Address:  ");  
    Serial.println(WiFi.macAddress());
    ESPNow.init();
    Serial.println("wait 10s for ESPNOW initialized");
    delay(10000);

    if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
    {
      Recv_mac=Gas_mac;      
    }

    if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
    {
      Recv_mac=Brk_mac;
    }
    if(ESPNow.add_peer(Recv_mac)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add peer");
    }
    else
    {
      ESPNOW_status=false;
      Serial.println("Fail to add peer");
    }

    // add master esp
    if(ESPNow.add_peer(esp_master)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add peer master");
    }
    else
    {
      ESPNOW_status=false;
      Serial.println("Fail to add peer master");
    }



    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(OnSent);
    ESPNow_initial_status=true;
    Serial.println("Rudder Initialized");

*/

  if (ESPNOW_status == false)
  {
    
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }

    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    //esp_now_register_send_cb(OnDataSent);


    // add master esp
    /*if(ESPNow.add_peer(esp_master)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add peer master");
    }
    else
    {
      ESPNOW_status=false;
      Serial.println("Fail to add peer master");
    }*/


    // Register peer
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, esp_master, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    // Add peer        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    }


    ESPNOW_status = true;
  }
  
}
