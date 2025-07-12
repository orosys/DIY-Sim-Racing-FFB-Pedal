using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    static class Constants
    {
        // payload revisiom
        public const uint pedalConfigPayload_version = 150;


        // pyload types
        public const uint pedalConfigPayload_type = 100;
        public const uint pedalActionPayload_type = 110;
        public const uint pedalStateBasicPayload_type = 120;
        public const uint pedalStateExtendedPayload_type = 130;
        public const uint bridgeStatePayloadType = 210;
        public const uint Basic_Wifi_info_type = 220;
        public const string pluginVersion = "0.90.09";
        public const string version_control_url = "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/version_control.json";
    }

    public enum enumServoStatus
    {
        Off,
        On,
        Idle
    }
    public enum bridgeAction
    {
        BRIDGE_ACTION_NONE,
        BRIDGE_ACTION_ENABLE_PAIRING,
        BRIDGE_ACTION_RESTART,
        BRIDGE_ACTION_DOWNLOAD_MODE,
        BRIDGE_ACTION_DEBUG,
        BRIDGE_ACTION_JOYSTICK_FLASHING_MODE,
        BRIDGE_ACTION_JOYSTICK_DEBUG
    };

    public enum PedalSystemAction
    {
        NONE,
        RESET_PEDAL_POSITION,//not in use
        PEDAL_RESTART,
        ENABLE_OTA,//not in use
        ENABLE_PAIRING,//not in use
        ESP_BOOT_INTO_DOWNLOAD_MODE,
        PRINT_PEDAL_INFO
    };
    
}
