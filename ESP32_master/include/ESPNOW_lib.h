#pragma once
#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "esp_now.h"
#include "ESPNowW.h"
#include "Main.h"
#include <list>
#include <iterator>


//#define ESPNow_debug
#define ESPNOW_LOG_MAGIC_KEY_U8 0x99
#define ESPNOW_LOG_MAGIC_KEY_2_U8 0x97
#define ESPNOW_ASSIGNMENT_MAGIC_KEY_U8 0x99
#define MAX_CAPACITY_OF_SCAN_PEDAL_U8 3
#define TIMEOUT_OF_UNASSIGNED_SCAN_U32 5000
uint8_t g_espMaster_au8[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t g_pedalMac_aau8[3][6] = {0};
uint8_t g_broadcastMac_au8[]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t g_espHost_au8[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t g_espMac_au8[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t* g_recvMac_pu8;
uint16_t g_espNowSend_u16=0;
uint16_t g_espNowReceive_u16=0;
int g_rssiDisplay_i32;
int32_t g_rssi_ai32[3]={0,0,0};
//bool MAC_get=false;
bool g_espNowStatus_b =false;
bool g_espNowInitialStatus_b=false;
bool g_espNowUpdate_b= false;
bool g_espNowNoDevice_b=false;
bool g_updateBasicState_ab[3]={false,false,false};
bool g_updateExtendState_ab[3]={false,false,false};
bool g_sendAssignment_ab[3] = {false, false, false};
bool g_pedalOtaAction_b=false;
bool g_pedalWirelessSyncEnabled_ab[3]={true,true,true};
uint16_t g_joystickValue_au16[]={0,0,0};
uint16_t g_joystickThrottleValueFromPedal_u16=0;
uint16_t g_joystickValueOriginal_au16[]={0,0,0};
unsigned long g_pedalLastUpdate_au32[3]={1,1,1};
bool g_espNowRequestConfig_ab[3]={false,false,false};
bool g_espNowError_ab[3]={false,false,false};
uint16_t g_pedalThrottleValue_u16=0;
uint16_t g_pedalBrakeValue_u16=0;
uint16_t g_pedalClutchValue_u16=0;
uint16_t g_pedalBrakeRudderValue_u16=0;
uint16_t g_pedalThrottleRudderValue_u16=0;
uint8_t g_pedalStatus_u8=0;
bool g_espNowPairingStatus_b = false;
bool g_updatePairingToEeprom_b = false;
#define WIFI_CH_EEPROM_MAGIC 0xA6
#ifndef EEPROM_offset
#define EEPROM_offset 15
#endif
#define WIFI_CH_EEPROM_OFFSET 60
struct WifiChannelConfig_t {
  uint8_t magic_u8;
  uint8_t channel_u8;
  uint8_t checksum_u8;
};


extern uint8_t g_currentWifiChannel_u8;

inline DapMacAddresses_t loadMacAddressesFromEeprom() {
  DapMacAddresses_t macCfg;
  EEPROM.get(DAP_MAC_ADDRESSES_EEPROM_OFFSET_U32, macCfg);
  if (macCfg.payloadHeader_st.payloadType_u8 == DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8 &&
      macCfg.payloadHeader_st.version_u8 == DAP_VERSION_MAC_ADDRESSES_U8) {
    uint16_t crc = checksumCalculator((uint8_t*)(&(macCfg.payloadHeader_st)),
                                      sizeof(macCfg.payloadHeader_st) + sizeof(macCfg.payloadMacAddresses_st));
    if (crc == macCfg.payloadFooter_st.checkSum_u16) {
      WiFi.macAddress(macCfg.payloadMacAddresses_st.ownMacAddress_au8);
      macCfg.payloadMacAddresses_st.ownNodeType_u8 = 3; // Bridge
      return macCfg;
    }
  }
  memset(&macCfg, 0, sizeof(macCfg));
  macCfg.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  macCfg.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  macCfg.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8;
  macCfg.payloadHeader_st.version_u8 = DAP_VERSION_MAC_ADDRESSES_U8;
  macCfg.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  macCfg.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
  macCfg.payloadMacAddresses_st.wifiChannel_u8 = 11;
  WiFi.macAddress(macCfg.payloadMacAddresses_st.ownMacAddress_au8);
  macCfg.payloadMacAddresses_st.ownNodeType_u8 = 3; // Bridge
  return macCfg;
}

inline void storeMacAddressesToEeprom(DapMacAddresses_t &macCfg) {
  macCfg.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  macCfg.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  macCfg.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8;
  macCfg.payloadHeader_st.version_u8 = DAP_VERSION_MAC_ADDRESSES_U8;
  macCfg.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  macCfg.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
  macCfg.payloadFooter_st.checkSum_u16 = checksumCalculator((uint8_t*)(&(macCfg.payloadHeader_st)),
                                                            sizeof(macCfg.payloadHeader_st) + sizeof(macCfg.payloadMacAddresses_st));
  EEPROM.put(DAP_MAC_ADDRESSES_EEPROM_OFFSET_U32, macCfg);
  EEPROM.commit();
}

inline void applyMacAddressesConfig(const DapMacAddresses_t &macCfg) {
  uint8_t ch = macCfg.payloadMacAddresses_st.wifiChannel_u8;
  if (ch >= 1 && ch <= 13) {
    g_currentWifiChannel_u8 = ch;
  }
  esp_wifi_set_channel(g_currentWifiChannel_u8, WIFI_SECOND_CHAN_NONE);

  for (int i = 0; i < 3; i++) {
    memcpy(g_pedalMac_aau8[i], macCfg.payloadMacAddresses_st.macAddress_aau8[i], 6);
    bool hasMac = false;
    for (int b = 0; b < 6; b++) {
      if (g_pedalMac_aau8[i][b] != 0) { hasMac = true; break; }
    }
    if (hasMac) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, g_pedalMac_aau8[i], 6);
      peerInfo.channel = g_currentWifiChannel_u8;
      peerInfo.ifidx = WIFI_IF_STA;
      peerInfo.encrypt = false;
      if (!esp_now_is_peer_exist(g_pedalMac_aau8[i])) {
        esp_now_add_peer(&peerInfo);
      } else {
        esp_now_mod_peer(&peerInfo);
      }
    }
  }
}

inline uint8_t loadWifiChannelFromEeprom() {
  WifiChannelConfig_t cfg;
  EEPROM.get(WIFI_CH_EEPROM_OFFSET, cfg);
  if (cfg.magic_u8 == WIFI_CH_EEPROM_MAGIC &&
      (uint8_t)(cfg.magic_u8 ^ cfg.channel_u8) == cfg.checksum_u8 &&
      cfg.channel_u8 >= 1 && cfg.channel_u8 <= 14) {
    return cfg.channel_u8;
  }
  return 11;
}

inline void saveWifiChannelToEeprom(uint8_t ch) {
  if (ch < 1 || ch > 14) return;
  WifiChannelConfig_t cfg;
  cfg.magic_u8 = WIFI_CH_EEPROM_MAGIC;
  cfg.channel_u8 = ch;
  cfg.checksum_u8 = (uint8_t)(WIFI_CH_EEPROM_MAGIC ^ ch);
  EEPROM.put(WIFI_CH_EEPROM_OFFSET, cfg);
  EEPROM.commit();
}

extern uint8_t g_currentWifiChannel_u8;
bool g_espNowPairingAction_b = false;
bool g_softwarePairingAction_b = false;
bool g_newUnassignedPedalDetected_ab[3]={false,false,false};
QueueHandle_t g_messageQueueHandle_pv;

extern DAP_servo_config_st_t dap_servo_config_response_st[3];
extern bool send_servo_config_to_host[3];

int16_t uint16ToInt16Convertor(uint16_t unsignedValue)
{
  const uint16_t OFFSET = 0x8000;
  int16_t tmp = int16_t(unsignedValue-OFFSET);
  return tmp;
}

bool macCheck(uint8_t* Mac_A, uint8_t* Mac_B)
{
  return memcmp(Mac_A, Mac_B, 6) == 0;
}


typedef struct EspPairingReg_t

{
  uint8_t pairStatus_au8[4];
  uint8_t pairMac_aau8[4][6];
} EspPairingReg_t;

typedef struct EspNowMessage_t
{
  char text_ac[240];
} EspNowMessage_t;

EspPairingReg_t g_espPairingReg_st;

void espNowPairingCallback(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  if(data_len==sizeof(DapEspPairing_t))
  {
    memcpy(&dap_esppairing_st, data , sizeof(DapEspPairing_t));
    //pedal reg
    if(dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==0||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==1||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==2)
    {
      uint8_t devId = dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8;
      memcpy(g_pedalMac_aau8[devId], mac_addr, 6);
      if(!esp_now_is_peer_exist(mac_addr))
      {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, mac_addr, 6);
        peerInfo.channel = 0;
        peerInfo.ifidx = WIFI_IF_STA;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
      }
      memcpy(&g_espPairingReg_st.pairMac_aau8[devId], mac_addr , 6);
      g_espPairingReg_st.pairStatus_au8[devId]=1;
      g_updatePairingToEeprom_b = true;
    }
    //bridge and analog device
    if(dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==99||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==98)
    {
      memcpy(&g_espPairingReg_st.pairMac_aau8[3], mac_addr , 6);
      g_espPairingReg_st.pairStatus_au8[3]=1;
      g_updatePairingToEeprom_b = true;
    }
  }
}

void onRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
  static uint32_t lastRxDiagTime = 0;
  if (millis() - lastRxDiagTime > 3000) {
    lastRxDiagTime = millis();
    ActiveSerial->printf("[ESPNOW RX] Packet len=%d from %02X:%02X:%02X:%02X:%02X:%02X (ch=%d)\n",
                         data_len,
                         esp_now_info->src_addr[0], esp_now_info->src_addr[1], esp_now_info->src_addr[2],
                         esp_now_info->src_addr[3], esp_now_info->src_addr[4], esp_now_info->src_addr[5],
                         g_currentWifiChannel_u8);
  }
  if(g_espNowPairingAction_b)
  {
    espNowPairingCallback(esp_now_info->src_addr, data, data_len);
  }

  uint8_t actual_pedal_tag = 255; // Standardwert (ungültig)

  // 1. MAC-Prüfung gegen aktuell im RAM gespeicherte MACs
  if(macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[0])){
    actual_pedal_tag = 0;
    if(esp_now_info->rx_ctrl != NULL){
      g_rssi_ai32[0] = esp_now_info->rx_ctrl->rssi;
      g_rssiDisplay_i32 = g_rssi_ai32[0];
    }
  } else if(macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[1])){
    actual_pedal_tag = 1;
    if(esp_now_info->rx_ctrl != NULL){
      g_rssi_ai32[1] = esp_now_info->rx_ctrl->rssi;
      g_rssiDisplay_i32 = g_rssi_ai32[1];
    }
  } else if(macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[2])){
    actual_pedal_tag = 2;
    if(esp_now_info->rx_ctrl != NULL){
      g_rssi_ai32[2] = esp_now_info->rx_ctrl->rssi;
      g_rssiDisplay_i32 = g_rssi_ai32[2];
    }
  }

  // 2. Auto-Discovery: Falls Absender-MAC der Bridge noch unbekannt ist (actual_pedal_tag == 255)
  if(actual_pedal_tag == 255)
  {
    // Auto-Discovery im Pairing-Modus
    if(g_espNowPairingAction_b && data_len == sizeof(DapEspPairing_t))
    {
      if(dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8 < 3)
      {
        actual_pedal_tag = dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8;
        memcpy(g_pedalMac_aau8[actual_pedal_tag], esp_now_info->src_addr, 6);
        if(!esp_now_is_peer_exist(esp_now_info->src_addr))
        {
          esp_now_peer_info_t peerInfo = {};
          memcpy(peerInfo.peer_addr, esp_now_info->src_addr, 6);
          peerInfo.channel = 0;
          peerInfo.ifidx = WIFI_IF_STA;
          peerInfo.encrypt = false;
          esp_now_add_peer(&peerInfo);
        }
        if(esp_now_info->rx_ctrl != NULL){
          g_rssi_ai32[actual_pedal_tag] = esp_now_info->rx_ctrl->rssi;
          g_rssiDisplay_i32 = g_rssi_ai32[actual_pedal_tag];
        }
      }
    }
    // Auto-Discovery via normales DapStateBasic_t Telemetrie-Paket
    else if(data_len == sizeof(DapStateBasic_t))
    {
      const DapStateBasic_t *st_cand = (const DapStateBasic_t *)data;
      if(st_cand->payloadHeader_st.version_u8 == DAP_VERSION_CONFIG_U8 &&
         st_cand->payloadHeader_st.payloadType_u8 == DAP_PAYLOAD_TYPE_STATE_BASIC_U8)
      {
        uint16_t crcChecker = checksumCalculator((uint8_t*)(&(st_cand->payloadHeader_st)),
            sizeof(st_cand->payloadHeader_st) + sizeof(st_cand->payloadPedalStateBasic_st));
        if(crcChecker == st_cand->payloadFooter_st.checkSum_u16)
        {
          if (st_cand->payloadHeader_st.pedalTag_u8 < 3)
          {
            uint8_t discTag = st_cand->payloadHeader_st.pedalTag_u8;
            // Dynamisch im RAM merken
            memcpy(g_pedalMac_aau8[discTag], esp_now_info->src_addr, 6);
            if(!esp_now_is_peer_exist(esp_now_info->src_addr))
            {
              esp_now_peer_info_t peerInfo = {};
              memcpy(peerInfo.peer_addr, esp_now_info->src_addr, 6);
              peerInfo.channel = 0;
              peerInfo.ifidx = WIFI_IF_STA;
              peerInfo.encrypt = false;
              esp_now_add_peer(&peerInfo);
            }
            actual_pedal_tag = discTag;
            if(esp_now_info->rx_ctrl != NULL){
              g_rssi_ai32[discTag] = esp_now_info->rx_ctrl->rssi;
              g_rssiDisplay_i32 = g_rssi_ai32[discTag];
            }
          }
        }
      }
    }
  }

  // 3. Nur Pakete mit gültigem / gelerntem actual_pedal_tag (< 3) verarbeiten
  if(actual_pedal_tag < 3)
  {

    //ActiveSerial->printf("Message received from pedal: %d, overwritten to: %d\n", actual_pedal_tag, actual_pedal_tag); 

    if(data[0]==DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8 && data[1]==ESPNOW_LOG_MAGIC_KEY_U8 && data[2]==ESPNOW_LOG_MAGIC_KEY_2_U8)
    {

      PayloadHidMessage_t receivedMsg;
      //getESPNOWLog_b = true;
      int copyLen = data[3];
      if (copyLen >= sizeof(receivedMsg.text_ac)) copyLen = sizeof(receivedMsg.text_ac) - 1;
      if (copyLen > 0)
      {

        memset(receivedMsg.text_ac, 0, sizeof(receivedMsg.text_ac));
        receivedMsg.payloadType_u8=DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8;
        receivedMsg.magicKey1_u8 = ESPNOW_LOG_MAGIC_KEY_U8;
        receivedMsg.magicKey2_u8=ESPNOW_LOG_MAGIC_KEY_2_U8;
        receivedMsg.length_u8= copyLen;
        memcpy(receivedMsg.text_ac, &data[4], copyLen);
        receivedMsg.text_ac[copyLen] = '\0';
        xQueueSend(g_messageQueueHandle_pv, &receivedMsg, 0);
      }
    }
    if(data_len==sizeof(DapStateBasic_t))
    {
      
      //g_joystickValue_au16[dap_state_basic_st.payloadHeader_st.pedalTag_u8]=dap_state_basic_st.payloadPedalStateBasic_st.joystickOutput_u16;
      DapStateBasic_t dap_state_basic_st_lcl;
      memcpy(&dap_state_basic_st_lcl, data, sizeof(DapStateBasic_t));
      bool structChecker=true;
      if(dap_state_basic_st_lcl.payloadHeader_st.version_u8!=DAP_VERSION_CONFIG_U8) structChecker=false;
      if(dap_state_basic_st_lcl.payloadHeader_st.payloadType_u8!=DAP_PAYLOAD_TYPE_STATE_BASIC_U8) structChecker=false;
      uint16_t crcChecker = checksumCalculator((uint8_t*)(&(dap_state_basic_st_lcl.payloadHeader_st)), sizeof(dap_state_basic_st_lcl.payloadHeader_st) + sizeof(dap_state_basic_st_lcl.payloadPedalStateBasic_st));
      if(crcChecker!=dap_state_basic_st_lcl.payloadFooter_st.checkSum_u16) structChecker=false;
      
      //fill the joystick value
      if(structChecker){
        // Nutze den aus der MAC-Adresse abgeleiteten Tag
        uint8_t pedalTag = actual_pedal_tag;

        if(pedalTag < 3){
          if (!g_pedalWirelessSyncEnabled_ab[pedalTag]) {
            return;
          }
          // 1. Überschreibe den Tag NUR in der lokalen Kopie
          dap_state_basic_st_lcl.payloadHeader_st.pedalTag_u8 = pedalTag;

          // 2. Kopiere die modifizierte lokale Kopie ins globale Array
          memcpy(&dap_state_basic_st[pedalTag], &dap_state_basic_st_lcl, sizeof(DapStateBasic_t));

          g_updateBasicState_ab[pedalTag]=true;
          g_pedalLastUpdate_au32[pedalTag]=millis();
          if(dap_state_basic_st_lcl.payloadPedalStateBasic_st.errorCode_u8!=0) g_espNowError_ab[pedalTag]=true;
          float joystickData_u32= dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16/32767.0f*10000.0f;
          uint16_t joystickNormalizedToInt16 = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16; 
          switch (pedalTag)
          {
            case PEDAL_ID_CLUTCH:
              g_pedalClutchValue_u16=joystickNormalizedToInt16;
              g_joystickValue_au16[0]=joystickData_u32;
              g_joystickValueOriginal_au16[0] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
              break;
            case PEDAL_ID_BRAKE:
              g_pedalBrakeValue_u16=joystickNormalizedToInt16;
              g_joystickValue_au16[1]=joystickData_u32;
              g_joystickValueOriginal_au16[1] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
              break;
            case PEDAL_ID_THROTTLE:
              g_pedalThrottleValue_u16=joystickNormalizedToInt16;
              g_joystickValue_au16[2]=joystickData_u32;
              g_joystickValueOriginal_au16[2] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
              g_pedalStatus_u8=dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.pedalStatus_u8;//control pedal status only by Throttle
              g_joystickThrottleValueFromPedal_u16=dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
            break;
            default:
            break;
          }
        }
      }
    }

    if(data_len==sizeof(DapStateExtended_t))
    {
      DapStateExtended_t dap_state_extend_st_lcl;
      memcpy(&dap_state_extend_st_lcl, data, sizeof(DapStateExtended_t));
      bool structChecker=true;      
      uint8_t pedalTag=dap_state_extend_st_lcl.payloadHeader_st.pedalTag_u8;
      if(dap_state_extend_st_lcl.payloadHeader_st.version_u8!=DAP_VERSION_CONFIG_U8) structChecker=false;
      if(dap_state_extend_st_lcl.payloadHeader_st.payloadType_u8!=DAP_PAYLOAD_TYPE_STATE_EXTENDED_U8) structChecker=false;
      uint16_t crcChecker = checksumCalculator((uint8_t*)(&(dap_state_extend_st_lcl.payloadHeader_st)), sizeof(dap_state_extend_st_lcl.payloadHeader_st) + sizeof(dap_state_extend_st_lcl.payloadPedalStateExtended_st));
      if(crcChecker!=dap_state_extend_st_lcl.payloadFooter_st.checkSum_u16) structChecker=false;
      if(structChecker){
        uint8_t pedalTag = actual_pedal_tag; // Auch hier den verifizierten Tag erzwingen
        if(pedalTag < 3){
          if (!g_pedalWirelessSyncEnabled_ab[pedalTag]) {
            return;
          }
          memcpy(&dap_state_extended_st[pedalTag], data, sizeof(DapStateExtended_t));
          dap_state_extend_st_lcl.payloadHeader_st.pedalTag_u8 = pedalTag;
          dap_state_extended_st[pedalTag].payloadHeader_st.pedalTag_u8 = pedalTag;

          g_updateExtendState_ab[pedalTag]=true;
        }
      }

    }

    if(data_len==sizeof(DapConfig_t))
    {
      memcpy(&dap_config_st_Temp, data, sizeof(DapConfig_t));
      
      uint8_t pedalTag = actual_pedal_tag; // Verifizierten Tag aus der MAC-Adresse nutzen
      
      if(pedalTag < 3){
        if (!g_pedalWirelessSyncEnabled_ab[pedalTag]) {
          return;
        }
        // Überschreibe den Tag im Header und den Typ in der Konfiguration zwingend
        dap_config_st_Temp.payloadHeader_st.pedalTag_u8 = pedalTag;
        dap_config_st_Temp.payloadPedalConfig_st.pedalType_u8 = pedalTag;
        
        g_espNowRequestConfig_ab[pedalTag]=true;
        
        if(pedalTag==0){
          memcpy(&dap_config_st_Clu, &dap_config_st_Temp, sizeof(DapConfig_t));
          memcpy(&dap_config_st[0], &dap_config_st_Temp, sizeof(DapConfig_t));
        }
        else if(pedalTag==1){
          memcpy(&dap_config_st_Brk, &dap_config_st_Temp, sizeof(DapConfig_t));
          memcpy(&dap_config_st[1], &dap_config_st_Temp, sizeof(DapConfig_t));
        }
        else if(pedalTag==2){
          memcpy(&dap_config_st_Gas, &dap_config_st_Temp, sizeof(DapConfig_t));
          memcpy(&dap_config_st[2], &dap_config_st_Temp, sizeof(DapConfig_t));
        }
      }
    }

    if(data_len==sizeof(DAP_servo_config_st_t))
    {
      DAP_servo_config_st_t received_servo_config;
      memcpy(&received_servo_config, data, sizeof(DAP_servo_config_st_t));
      bool structChecker=true;
      if(received_servo_config.payloadHeader_st.version_u8!=DAP_VERSION_CONFIG_U8) structChecker=false;
      if(received_servo_config.payloadHeader_st.payloadType_u8!=DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8) structChecker=false;
      uint16_t crcChecker = checksumCalculator((uint8_t*)(&(received_servo_config.payloadHeader_st)), sizeof(received_servo_config.payloadHeader_st) + sizeof(received_servo_config.payloadServoConfig_st));
      if(crcChecker!=received_servo_config.payloadFooter_st.checkSum_u16) structChecker=false;
      if(structChecker){
        uint8_t pedalTag = actual_pedal_tag; // Verifizierten Tag erzwingen
        if(pedalTag < 3){
          if (!g_pedalWirelessSyncEnabled_ab[pedalTag]) {
            return;
          }
          received_servo_config.payloadHeader_st.pedalTag_u8 = pedalTag;
          memcpy(&dap_servo_config_response_st[pedalTag], &received_servo_config, sizeof(DAP_servo_config_st_t));
          send_servo_config_to_host[pedalTag] = true;
        }
      }
    }
  }
  


}
void onSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{

}

// The callback that does the magic
void promiscuousRxCb(void *buf, wifi_promiscuous_pkt_type_t type)
 {
  // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  //const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  //const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t* payload = ppkt->payload;
  if (ppkt->rx_ctrl.sig_len > 24)
  {
    const uint8_t *addr_DESTINATION = payload + 4;   
    const uint8_t *addr_SOURCE = payload + 10;  // å‚³é€ ç«¯ MAC
    uint8_t addr_package[6];
    memcpy(addr_package, addr_SOURCE, 6);
    if (macCheck(addr_package, g_pedalMac_aau8[0]))
    {
      if (g_pedalWirelessSyncEnabled_ab[0]) {
        g_rssi_ai32[0]=ppkt->rx_ctrl.rssi;
        g_rssiDisplay_i32=g_rssi_ai32[0];
      }
    }
    if (macCheck(addr_package, g_pedalMac_aau8[1]))
    {
      if (g_pedalWirelessSyncEnabled_ab[1]) {
        g_rssi_ai32[1]=ppkt->rx_ctrl.rssi;
        g_rssiDisplay_i32=g_rssi_ai32[1];
      }
    }
    if (macCheck(addr_package, g_pedalMac_aau8[2]))
    {
      if (g_pedalWirelessSyncEnabled_ab[2]) {
        g_rssi_ai32[2]=ppkt->rx_ctrl.rssi;
        g_rssiDisplay_i32=g_rssi_ai32[2];
      }
    }
  }
  
  //int g_rssi_ai32 = ppkt->rx_ctrl.g_rssi_ai32;
  //g_rssiDisplay_i32 = g_rssi_ai32;
  
}

void espNowInitialize()
{

    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(true, true);
    WiFi.setSleep(false);
    ActiveSerial->println("[L]Initializing Wifi."); 
    delay(1000);
    WiFi.macAddress(g_espMac_au8); 
    memcpy(g_espHost_au8, g_espMac_au8, 6);
    ActiveSerial->printf("[L]Bridge Factory Hardware Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_espMac_au8[0], g_espMac_au8[1], g_espMac_au8[2], g_espMac_au8[3], g_espMac_au8[4], g_espMac_au8[5]);

    ActiveSerial->println("[L]Initializing ESP-NOW");
    ESPNow.init();
    delay(3000);
    #ifdef Using_Board_ESP32
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_L);
    #endif
    
    #ifdef Using_Board_ESP32S3
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_L);
      #ifdef LOW_TX_POWER
      esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
      ActiveSerial->println("[L]Setting Wifi strength to 8.5dbm ");
      #endif
    #endif

    // Read MAC configuration from EEPROM (Offset 0)
    DapMacAddresses_t macCfg = loadMacAddressesFromEeprom();
    applyMacAddressesConfig(macCfg);
    for (int i = 0; i < 3; i++) {
      ActiveSerial->printf("[L]Configured Pedal #%d Mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
                           i, g_pedalMac_aau8[i][0], g_pedalMac_aau8[i][1], g_pedalMac_aau8[i][2],
                           g_pedalMac_aau8[i][3], g_pedalMac_aau8[i][4], g_pedalMac_aau8[i][5]);
    }

    bool addPeerCHecker = true;
    for (int i = 0; i < 3; i++)
    {
      if (g_espPairingReg_st.pairStatus_au8[i] == 1)
      {
        if (ESPNow.add_peer(g_pedalMac_aau8[i]) != ESP_OK) addPeerCHecker = false;
      }
    }
    // Broadcast disabled: pure unicast architecture
    if(addPeerCHecker) ActiveSerial->println("[L]Peers added successfully.");
    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(onSent);
    ActiveSerial->printf("[L]ESPNow Channel: %d\n", g_currentWifiChannel_u8);
    //g_rssi_ai32 calculate
    // esp_wifi_set_promiscuous(true);
    // esp_wifi_set_promiscuous_rx_cb(&promiscuousRxCb);
    g_espNowInitialStatus_b=true;
    g_espNowStatus_b=true;
    ActiveSerial->println("[L]ESPNow Initialized");
  
}
void printStructHex(DapBridgeState_t* s)
 {
    const uint8_t* p = (const uint8_t*)s;
    for (size_t i = 0; i < sizeof(DapBridgeState_t); i++) 
    {
      ActiveSerial->print("0x");  
      if (p[i] < 16) ActiveSerial->print('0');
      ActiveSerial->print(p[i], HEX);
      ActiveSerial->print("-");
    }
    ActiveSerial->println("");
}



