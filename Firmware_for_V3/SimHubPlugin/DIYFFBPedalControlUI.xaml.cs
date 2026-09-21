//using SimHub.Plugins.OutputPlugins.Dash.GLCDTemplating;
using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;

using System.Windows.Media.TextFormatting;
using System.Text.Json;
using FMOD;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.IO;
using System.Text;
using System.Web;
using MahApps.Metro.Controls;
using System.Runtime.CompilerServices;
using System.CodeDom.Compiler;
using System.Windows.Forms;
using static System.Net.Mime.MediaTypeNames;
using System.Runtime.InteropServices.ComTypes;
using Microsoft.Win32;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;
using System.Windows.Input;
using System.Windows.Shapes;
using MouseEventArgs = System.Windows.Input.MouseEventArgs;
using SimHub.Plugins.OutputPlugins.GraphicalDash.PSE;
using SimHub.Plugins.Styles;
using System.Windows.Media;
using System.Runtime.Remoting.Messaging;
using SimHub.Plugins.OutputPlugins.GraphicalDash.Behaviors.DoubleText.Imp;
using System.Reflection;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using Newtonsoft.Json;
using System.Threading;
using System.Text.RegularExpressions;
using SimHub.Plugins;
using log4net.Plugin;
//using System.Drawing;

using vJoyInterfaceWrap;
//using vJoy.Wrapper;
using System.Runtime;
using SimHub.Plugins.DataPlugins.ShakeItV3.Settings;
using System.Windows.Media.Effects;
using System.Diagnostics;
using System.Collections;
using System.Linq;
using Windows.UI.Notifications;
//using System.Diagnostics;
using System.Windows.Navigation;
using System.CodeDom;
using System.Media;
using System.Windows.Threading;
using System.Net.Http;
using System.Threading.Tasks;
using static User.PluginSdkDemo.DIY_FFB_Pedal;
using User.PluginSdkDemo.UIFunction;
using Windows.UI.ViewManagement;



// Win 11 install, see https://github.com/jshafer817/vJoy/releases
//using vJoy.Wrapper;



namespace User.PluginSdkDemo
{
    /// <summary>
    /// Logique d'interaction pour SettingsControlDemo.xaml
    /// </summary>
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {


        // payload revisiom
        //public uint pedalConfigPayload_version = 110;
        //public uint pedalConfigPayload_type = 100;
        //public uint pedalActionPayload_type = 110;

        public uint indexOfSelectedPedal_u = 1;
        public uint profile_select = 0;
        public DIY_FFB_Pedal Plugin { get; }
        public static DAP_config_st[] dap_config_st = new DAP_config_st[3];
        public static DAP_config_st dap_config_st_rudder;


        public DAP_bridge_state_st dap_bridge_state_st;
        public Basic_WIfi_info _basic_wifi_info;
        //private string stringValue;
        public bool[] waiting_for_pedal_config = new bool[3];
        public System.Windows.Forms.Timer[] pedal_serial_read_timer = new System.Windows.Forms.Timer[3];
        public System.Windows.Forms.Timer connect_timer;
        public System.Windows.Forms.Timer ESP_host_serial_timer;
        private SolidColorBrush defaultcolor;
        private SolidColorBrush lightcolor;
        private SolidColorBrush redcolor;
        private SolidColorBrush color_RSSI_1;
        private SolidColorBrush color_RSSI_2;
        private SolidColorBrush color_RSSI_3;
        private SolidColorBrush color_RSSI_4;
        private SolidColorBrush Red_Warning;
        private SolidColorBrush White_Default;
        private string info_text_connection;
        private string system_info_text_connection;
        private int current_pedal_travel_state= 0;
        //private int gridline_kinematic_count_original = 0;
        private double[] Pedal_position_reading=new double[3];
        private bool[] Serial_connect_status = new bool[3] { false,false,false};
        private bool updatingGameProfileUI;
        //public byte Bridge_RSSI = 0;
        public bool[] Pedal_wireless_connection_update_b = new bool[3] { false,false,false};
        public int Bridge_baudrate = 3000000;
        public bool[] Version_error_warning_b = new bool[3] { false, false, false };
        public bool[] Version_warning_first_show_b= new bool[3] { false, false, false };
        public bool Version_warning_first_show_b_bridge = false;
        public byte[] Pedal_version = new byte[3];
        private SerialMonitor_Window _serial_monitor_window;
        public bool Pedal_Log_warning_1st_show_b = true;
        private string[] Rudder_Pedal_idx_Name= new string[3] {"Clutch", "Brake","Throttle"};
        public byte Pedal_connect_status = 0;
        DateTime ConfigLiveSending_last = DateTime.Now;
        DateTime PedalTabChange_last = DateTime.Now;
        //public byte[,] PedalFirmwareVersion = new byte[3, 3] { { 0, 0, 0}, { 0, 0, 0 }, { 0, 0, 0 } };
        public bool PedalTabChange = false;


        public enum PedalAvailability        
        {
            NopedalConnect,
            SinglePedalClutch,
            SinglePedalBrake,
            SinglePedalThrottle,
            TwoPedalConnectClutchBrake,
            TwoPedalConnectClutchThrottle,
            TwoPedalConnectBrakeThrottle,
            ThreePedalConnect
        }
        
        
        
        unsafe public DIYFFBPedalControlUI()
        {
            
            DAP_config_set_default_rudder();
            for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
            {
                DAP_config_set_default(pedalIdx);
                
            }
            for (uint i = 0; i < 30; i++)
            {
                _basic_wifi_info.WIFI_PASS[i] = 0;
                _basic_wifi_info.WIFI_SSID[i] = 0;
            }
            InitializeComponent();
            
            //setting drawing color with Simhub theme workaround
            SolidColorBrush buttonBackground_ = btn_update.Background as SolidColorBrush;

            Color color = Color.FromArgb(150, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B);
            Color color_2 = Color.FromArgb(200, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B);
            Color color_3 = Color.FromArgb(255, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B);
            Color RED_color = Color.FromArgb(60, 139, 0, 0);
            redcolor = new SolidColorBrush(RED_color);
            SolidColorBrush Line_fill = new SolidColorBrush(color_2);
            
            //SolidColorBrush rect_fill = new SolidColorBrush(color);
            defaultcolor = new SolidColorBrush(color);
            
            lightcolor = new SolidColorBrush(color_3);
            
            color_RSSI_1 = new SolidColorBrush(Color.FromArgb(150, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B));
            color_RSSI_2 = new SolidColorBrush(Color.FromArgb(180, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B));
            color_RSSI_3 = new SolidColorBrush(Color.FromArgb(210, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B));
            color_RSSI_4 = new SolidColorBrush(Color.FromArgb(255, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B));
            Red_Warning = new SolidColorBrush(Color.FromArgb(255, 244, 67, 67));
            White_Default = new SolidColorBrush(Color.FromArgb(255, 255, 255, 255));
            CheckForUpdateAsync();
        }



        



        public DIYFFBPedalControlUI(DIY_FFB_Pedal plugin) : this()
        {
            this.Plugin = plugin;
            plugin.testValue = 1;
            plugin.wpfHandle = this;
            UpdateSerialPortList_click();
            
            indexOfSelectedPedal_u = plugin.Settings.table_selected;
            MyTab.SelectedIndex = (int)indexOfSelectedPedal_u;


            //auto connection with timmer
            if (connect_timer != null)
            {
                connect_timer.Dispose();
                connect_timer.Stop();
            }

            connect_timer = new System.Windows.Forms.Timer();
            connect_timer.Tick += new EventHandler(connection_timmer_tick);
            connect_timer.Interval = 5000; // in miliseconds try connect every 5s
            connect_timer.Start();
            System.Threading.Thread.Sleep(50);
            RefreshGameProfileUI();
            plugin.RequestAutomaticProfileRefresh();

        }



        

        public class SerialPortChoice
        {
            public SerialPortChoice(string display, string value)
            {
                Display = display;
                Value = value;
            }

            public string Value { get; set; }
            public string Display { get; set; }
        }

        



        

        Int64 writeCntr = 0;

        int[] timeCntr = { 0, 0, 0,0 };

        double[] timeCollector = { 0, 0, 0,0 };


        
        

        public void SerialPortSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string tmp = (string)SerialPortSelection.SelectedValue;
            //string tmp_2= Plugin.comportList[SerialPortSelection.SelectedIndex].ComPortName;
            //System.Windows.MessageBox.Show("connect to " + tmp_2);
            //Plugin._serialPort[indexOfSelectedPedal_u].PortName = tmp;


            //try 
            //{
            //    TextBox_debugOutput.Text = "Debug: " + Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u];
            //}
            //catch (Exception caughtEx)
            //{
            //    string errorMessage = caughtEx.Message;
            //    TextBox_debugOutput.Text = errorMessage;
            //}

            try
            {
                //if (Plugin.Settings.connect_status[indexOfSelectedPedal_u] == 0)
                if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen == false)
                {
                    Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u] = tmp;
                    Plugin._serialPort[indexOfSelectedPedal_u].PortName = tmp;
                }
                //TextBox_debugOutput.Text = "COM port selected: " + Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u];

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                TextBox2.Text = errorMessage;
            }
        }

        public void ESPNow_SerialPortSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string tmp = (string)SerialPortSelection_ESPNow.SelectedValue;
            try
            {
                //if (Plugin.Settings.connect_status[indexOfSelectedPedal_u] == 0)
                if (Plugin.ESPsync_serialPort.IsOpen == false)
                {
                    Plugin.Settings.ESPNow_port = tmp;
                    Plugin.ESPsync_serialPort.PortName = tmp;
                }
                //TextBox_debugOutput.Text = "COM port selected: " + Plugin.Settings.ESPNow_port;

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                TextBox2.Text = errorMessage;
            }



        }

        private void Checkbox_auto_remove_serial_line_bridge_Checked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            {
                Plugin.Settings.Serial_auto_clean_bridge = true;
            }
        }

        private void Checkbox_auto_remove_serial_line_bridge_Unchecked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            {
                Plugin.Settings.Serial_auto_clean_bridge = false;
            }
        }

        
        private void Tab_ConfigChanged(object sender, DAP_config_st e)
        {
            if (Plugin != null)
            {
                dap_config_st[indexOfSelectedPedal_u] = e;
                if (Plugin._calculations.IsUIRefreshNeeded)
                {
                    updateTheGuiFromConfig();
                    Plugin._calculations.IsUIRefreshNeeded = false;
                }
                PedalParameterLiveUpdate();
            }
            
        }

        private void Tab_SettingsChanged(object sender, DIYFFBPedalSettings e)
        {
            Plugin.Settings = e;
            updateTheGuiFromConfig();
        }

        private void Tab_CalculationChanged(object sender, CalculationVariables e)
        {
            Plugin._calculations = e;
            updateTheGuiFromConfig();
        }

        private void Rudder_ConfigChanged(object sender, DAP_config_st e)
        {
            if (Plugin != null)
            {
                dap_config_st_rudder = e;
                if (Plugin._calculations.IsUIRefreshNeeded)
                {
                    updateTheGuiFromConfig();
                    Plugin._calculations.IsUIRefreshNeeded = false;
                }
            }
        }

        private void SystemProfile_Tab_btn_send_profile_Click_event(object sender, EventArgs e)
        {
            Sendconfigtopedal_shortcut();
            
        }

        private void SystemProfile_Tab_btn_apply_profile_Click_event(object sender, EventArgs e)
        {
            Profile_change((uint)Plugin._calculations.profile_index);
        }

        private sealed class GameProfileMappingItem
        {
            public string GameCode { get; set; }
            public int Slot { get; set; }
            public string SlotName { get; set; }

            public override string ToString()
            {
                return String.Format("{0} -> {1}: {2}", GameCode, (char)('A' + Slot), SlotName);
            }
        }

        private void RefreshGameProfileUI()
        {
            if (Plugin == null || GameProfileGame == null) return;
            updatingGameProfileUI = true;
            try
            {
                if (Plugin.Settings.GameProfileSlots == null)
                    Plugin.Settings.GameProfileSlots = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);

                GameProfileAutoEnabled.IsChecked = Plugin.Settings.AutoProfileByGame;
                GameProfileDefaultSlot.Items.Clear();
                GameProfileSlot.Items.Clear();
                for (int slot = 0; slot < 6; slot++)
                {
                    string label = String.Format("{0}: {1}", (char)('A' + slot),
                        Plugin.Settings.Profile_name[slot]);
                    GameProfileDefaultSlot.Items.Add(label);
                    GameProfileSlot.Items.Add(label);
                }
                GameProfileDefaultSlot.SelectedIndex = Math.Max(0,
                    Math.Min(5, Plugin.Settings.DefaultProfileSlot));
                if (GameProfileSlot.SelectedIndex < 0) GameProfileSlot.SelectedIndex = 0;

                string selectedGame = GameProfileGame.Text;
                var gameCodes = new HashSet<string>(Plugin.Settings.GameProfileSlots.Keys,
                    StringComparer.OrdinalIgnoreCase);
                string gameDataPath = System.IO.Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "PluginsData");
                if (Directory.Exists(gameDataPath))
                {
                    foreach (string directory in Directory.GetDirectories(gameDataPath))
                    {
                        string code = System.IO.Path.GetFileName(directory);
                        if (code != "Common" && code != "_Backups" &&
                            code != "MotionRecords" && code != "MotionTrackProfiles")
                            gameCodes.Add(code);
                    }
                }
                if (!String.IsNullOrWhiteSpace(Plugin.Current_Game)) gameCodes.Add(Plugin.Current_Game);

                GameProfileGame.Items.Clear();
                foreach (string code in gameCodes.OrderBy(code => code, StringComparer.OrdinalIgnoreCase))
                    GameProfileGame.Items.Add(code);
                GameProfileGame.Text = selectedGame;

                GameProfileMappings.Items.Clear();
                foreach (var mapping in Plugin.Settings.GameProfileSlots.OrderBy(item => item.Key,
                    StringComparer.OrdinalIgnoreCase))
                {
                    if (mapping.Value < 0 || mapping.Value > 5) continue;
                    GameProfileMappings.Items.Add(new GameProfileMappingItem
                    {
                        GameCode = mapping.Key,
                        Slot = mapping.Value,
                        SlotName = Plugin.Settings.Profile_name[mapping.Value]
                    });
                }
                GameProfileStatus.Text = String.Format("{0} game mapping(s)", GameProfileMappings.Items.Count);
            }
            catch (Exception ex)
            {
                GameProfileStatus.Text = "Could not read game profiles: " + ex.Message;
            }
            finally
            {
                updatingGameProfileUI = false;
            }
        }

        private void GameProfileAutoEnabled_Changed(object sender, RoutedEventArgs e)
        {
            if (Plugin == null || updatingGameProfileUI) return;
            Plugin.Settings.AutoProfileByGame = GameProfileAutoEnabled.IsChecked == true;
            Plugin.RequestAutomaticProfileRefresh();
        }

        private void GameProfileDefaultSlot_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (Plugin == null || updatingGameProfileUI || GameProfileDefaultSlot.SelectedIndex < 0) return;
            Plugin.Settings.DefaultProfileSlot = GameProfileDefaultSlot.SelectedIndex;
            Plugin.RequestAutomaticProfileRefresh();
        }

        private void GameProfileSave_Click(object sender, RoutedEventArgs e)
        {
            if (Plugin == null) return;
            string code = GameProfileGame.Text.Trim();
            int slot = GameProfileSlot.SelectedIndex;
            if (code.Length == 0 || slot < 0 || slot > 5)
            {
                GameProfileStatus.Text = "Choose a game and slot first.";
                return;
            }
            var mappings = new Dictionary<string, int>(Plugin.Settings.GameProfileSlots,
                StringComparer.OrdinalIgnoreCase);
            mappings[code] = slot;
            Plugin.Settings.GameProfileSlots = mappings;
            RefreshGameProfileUI();
            GameProfileGame.Text = code;
            GameProfileStatus.Text = String.Format("{0} -> Slot {1} saved", code, (char)('A' + slot));
            Plugin.RequestAutomaticProfileRefresh();
        }

        private void GameProfileRemove_Click(object sender, RoutedEventArgs e)
        {
            if (Plugin == null) return;
            var mapping = GameProfileMappings.SelectedItem as GameProfileMappingItem;
            if (mapping == null) return;
            var mappings = new Dictionary<string, int>(Plugin.Settings.GameProfileSlots,
                StringComparer.OrdinalIgnoreCase);
            mappings.Remove(mapping.GameCode);
            Plugin.Settings.GameProfileSlots = mappings;
            RefreshGameProfileUI();
            Plugin.RequestAutomaticProfileRefresh();
        }

        private void GameProfileMappings_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (updatingGameProfileUI) return;
            var mapping = GameProfileMappings.SelectedItem as GameProfileMappingItem;
            if (mapping == null) return;
            GameProfileGame.Text = mapping.GameCode;
            GameProfileSlot.SelectedIndex = mapping.Slot;
        }

        public void ApplyAutomaticProfile(uint slot)
        {
            if (Plugin == null || !Plugin.Settings.AutoProfileByGame || slot > 5) return;
            bool hasLinkedPedal = false;
            for (int pedal = 0; pedal < 3; pedal++)
            {
                if (Plugin.Settings.file_enable_check[slot, pedal] != 1) continue;
                hasLinkedPedal = true;
                if (!File.Exists(Plugin.Settings.Pedal_file_string[slot, pedal]))
                {
                    GameProfileStatus.Text = String.Format("Slot {0} has a missing config file", (char)('A' + slot));
                    SimHub.Logging.Current.Error("DIY pedal auto profile: " + GameProfileStatus.Text);
                    return;
                }
            }
            if (!hasLinkedPedal)
            {
                GameProfileStatus.Text = String.Format("Slot {0} has no enabled pedal files", (char)('A' + slot));
                SimHub.Logging.Current.Error("DIY pedal auto profile: " + GameProfileStatus.Text);
                return;
            }

            try
            {
                SystemProfile_Tab.Settings = Plugin.Settings;
                SystemProfile_Tab.calculation = Plugin._calculations;
                SystemProfile_Tab.SelectProfile(slot);
                Plugin._calculations.profile_index = slot;
                Profile_change(slot);
                Sendconfigtopedal_shortcut();
                GameProfileCurrentGame.Text = "Current game: " +
                    (String.IsNullOrWhiteSpace(Plugin.Current_Game) ? "none" : Plugin.Current_Game);
                GameProfileStatus.Text = String.Format("Slot {0} applied", (char)('A' + slot));
                SimHub.Logging.Current.Info("DIY pedal auto profile: " + GameProfileStatus.Text);
            }
            catch (Exception ex)
            {
                GameProfileStatus.Text = "Auto switch failed: " + ex.Message;
                SimHub.Logging.Current.Error("DIY pedal auto profile: " + ex);
            }
        }

        private void SystemLicense_Tab_btn_test_Click_event(object sender, EventArgs e)
        {
            ToastNotification("Debug", "Print All parameter in Serial log");
            PrintUnknownStructParameters(dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_);
            //readRudderSettingToConfig();
            //PrintUnknownStructParameters(dap_config_st_rudder.payloadPedalConfig_);
            /*
            if (_serial_monitor_window != null)
            {
                _serial_monitor_window.TextBox_SerialMonitor.Text += "\nCom port count: " + Plugin.comportList.Count;
                foreach (var items in Plugin.comportList)
                {              
                    _serial_monitor_window.TextBox_SerialMonitor.Text += "\ndevice name:" + items.DeviceName + "\nVID:" + items.Vid + " PID:" + items.Pid;
                }
            }
            */

        }


    }
    
}
