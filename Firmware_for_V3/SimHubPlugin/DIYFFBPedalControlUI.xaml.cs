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
        private readonly double[] Pedal_output_reading = new double[3];
        private readonly double[] Pedal_travel_reading = new double[3];
        private static readonly TimeSpan HomeOutputWindow = TimeSpan.FromSeconds(10);
        private static readonly TimeSpan HomeOutputSampleInterval = TimeSpan.FromMilliseconds(80);
        private readonly Queue<KeyValuePair<DateTime, double>>[] homePedalHistory =
            { new Queue<KeyValuePair<DateTime, double>>(),
              new Queue<KeyValuePair<DateTime, double>>(),
              new Queue<KeyValuePair<DateTime, double>>() };
        private readonly DateTime[] lastHomeOutputSampleUtc = new DateTime[3];
        private DispatcherTimer homeRefreshTimer;
        private int homeRefreshTicks;
        private readonly int[] homeCurveSignature = { int.MinValue, int.MinValue, int.MinValue };
        private bool updatingGameProfileUI;
        private bool[] Serial_connect_status = new bool[3] { false,false,false};
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
            SystemProfile_Tab.Settings = plugin.Settings;
            SystemProfile_Tab.calculation = plugin._calculations;
            RefreshGameProfileUI();

            homeRefreshTimer = new DispatcherTimer(DispatcherPriority.Render)
                { Interval = TimeSpan.FromMilliseconds(40) };
            homeRefreshTimer.Tick += (sender, args) =>
                UpdateHomeDashboard(++homeRefreshTicks % 12 == 0);
            homeRefreshTimer.Start();
            Unloaded += (sender, args) => homeRefreshTimer.Stop();
            Loaded += (sender, args) => homeRefreshTimer.Start();


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
            if (plugin.Settings.AutoProfileByGame)
            {
                plugin.Page_update_flag = false;
                ApplyAutomaticProfile(plugin._calculations.profile_index);
            }
            System.Threading.Thread.Sleep(50);

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
            UpdateHomeDashboard();
        }

        private void HomeSlotApply_Click(object sender, RoutedEventArgs e)
        {
            if (Plugin == null) return;
            var button = sender as System.Windows.Controls.Button;
            uint slot;
            if (button == null || !UInt32.TryParse(button.Tag.ToString(), out slot)) return;

            if (!HasLinkedPedal((int)slot))
            {
                HomeApplyFeedback.Text = String.Format("Slot {0} has no enabled pedal files. Set it up in System > Profiles.",
                    (char)('A' + slot));
                return;
            }

            for (int pedal = 0; pedal < 3; pedal++)
            {
                if (Plugin.Settings.file_enable_check[slot, pedal] == 1 &&
                    !File.Exists(Plugin.Settings.Pedal_file_string[slot, pedal]))
                {
                    HomeApplyFeedback.Text = String.Format("Slot {0}: a pedal config file is missing. Check System > Profiles.",
                        (char)('A' + slot));
                    return;
                }
            }
            try
            {
                SystemProfile_Tab.Settings = Plugin.Settings;
                SystemProfile_Tab.calculation = Plugin._calculations;
                SystemProfile_Tab.ApplySlot(slot);
                Plugin._calculations.profile_index = slot;
                Profile_change(slot);
                int sent = ApplyHomeSlotToConnectedPedals(slot);
                HomeApplyFeedback.Text = sent > 0
                    ? String.Format("{0} applied · sent to {1} connected pedal(s)",
                        Plugin.Settings.Profile_name[slot], sent)
                    : String.Format("{0} loaded · no connected pedal to send to",
                        Plugin.Settings.Profile_name[slot]);
                UpdateHomeDashboard();
            }
            catch (Exception ex)
            {
                HomeApplyFeedback.Text = "Profile could not be applied: " + ex.Message;
            }
        }

        private int ApplyHomeSlotToConnectedPedals(uint slot)
        {
            int sent = 0;
            for (int pedal = 0; pedal < 3; pedal++)
            {
                if (Plugin.Settings.file_enable_check[slot, pedal] != 1) continue;
                bool connected = Plugin.Settings.Pedal_ESPNow_Sync_flag[pedal]
                    ? Plugin.ESPsync_serialPort != null && Plugin.ESPsync_serialPort.IsOpen &&
                        Plugin._calculations.PedalAvailability[pedal]
                    : Plugin._serialPort[pedal] != null && Plugin._serialPort[pedal].IsOpen;
                if (!connected) continue;

                var config = dap_config_st[pedal];
                config.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                config.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
                config.payloadPedalConfig_.pedal_type = (byte)pedal;
                Plugin.SendConfigWithoutSaveToEEPROM(config, (byte)pedal);
                sent++;
            }
            return sent;
        }

        private void HomeOpenPedals_Click(object sender, RoutedEventArgs e)
        {
            Function_Tab_seleciton.SelectedItem = Tab_Pedals;
        }

        private void HomeOpenSystem_Click(object sender, RoutedEventArgs e)
        {
            Function_Tab_seleciton.SelectedItem = Tab_System;
        }

        private void UpdateHomePedal(int pedal, TextBlock status, TextBlock value,
            System.Windows.Controls.ProgressBar bar, Polyline graph, Polyline configGraph,
            Line positionLine, Ellipse positionMarker,
            TextBlock settings, WrapPanel effects, bool refreshSummary)
        {
            bool connected = Plugin._calculations.PedalAvailability[pedal] ||
                             Plugin._calculations.PedalSerialAvailability[pedal];
            double percent = connected ? Math.Max(0, Math.Min(100,
                Pedal_output_reading[pedal] * 100.0 / 32767.0)) : 0;
            string statusText = connected ? "Connected · live output" : "Disconnected";
            if (status.Text != statusText) status.Text = statusText;
            string valueText = connected ? String.Format("{0:0}%", percent) : "—";
            if (value.Text != valueText) value.Text = valueText;
            if (Math.Abs(bar.Value - percent) > 0.1) bar.Value = percent;

            DateTime now = DateTime.UtcNow;
            Queue<KeyValuePair<DateTime, double>> history = homePedalHistory[pedal];
            if (history.Count == 0 || now - lastHomeOutputSampleUtc[pedal] >= HomeOutputSampleInterval)
            {
                history.Enqueue(new KeyValuePair<DateTime, double>(now, percent));
                lastHomeOutputSampleUtc[pedal] = now;
            }
            while (history.Count > 0 && now - history.Peek().Key > HomeOutputWindow)
                history.Dequeue();
            var points = new PointCollection();
            foreach (KeyValuePair<DateTime, double> sample in history)
            {
                double x = 700 - (now - sample.Key).TotalSeconds * 70;
                points.Add(new Point(x, 100 - sample.Value));
            }
            graph.Points = points;

            UpdateHomePositionMarker(connected, Pedal_travel_reading[pedal],
                configGraph, positionLine, positionMarker);

            if (!refreshSummary) return;
            var config = dap_config_st[pedal].payloadPedalConfig_;
            string settingsText = String.Format("Max {0:0.#} kg · Travel {1}–{2}%",
                config.maxForce, config.pedalStartPosition, config.pedalEndPosition);
            if (settings.Text != settingsText) settings.Text = settingsText;
            UpdateHomeEffectTags(pedal, effects, graph.Stroke);
            UpdateHomeConfigGraph(pedal, config, configGraph);
            UpdateHomePositionMarker(connected, Pedal_travel_reading[pedal],
                configGraph, positionLine, positionMarker);
        }

        private void UpdateHomeEffectTags(int pedal, WrapPanel panel, Brush accent)
        {
            var names = new List<string>();
            var settings = Plugin.Settings;
            if (settings.ABS_enable_flag[pedal] == 1) names.Add("ABS");
            if (settings.RPM_enable_flag[pedal] == 1) names.Add("RPM");
            if (dap_config_st[pedal].payloadPedalConfig_.BP_trigger == 1) names.Add("BITE POINT");
            if (settings.G_force_enable_flag[pedal] == 1) names.Add("G-FORCE");
            if (settings.WS_enable_flag[pedal] == 1) names.Add("WHEEL SLIP");
            if (settings.Road_impact_enable_flag[pedal] == 1) names.Add("ROAD IMPACT");
            if (settings.CV1_enable_flag[pedal]) names.Add("CUSTOM 1");
            if (settings.CV2_enable_flag[pedal]) names.Add("CUSTOM 2");

            string signature = String.Join("|", names);
            if (Equals(panel.Tag, signature)) return;
            panel.Tag = signature;
            panel.Children.Clear();
            foreach (string name in names)
            {
                panel.Children.Add(new Border
                {
                    Background = new SolidColorBrush(Color.FromRgb(45, 54, 63)),
                    BorderBrush = accent,
                    BorderThickness = new Thickness(1),
                    CornerRadius = new CornerRadius(10),
                    Padding = new Thickness(7, 2, 7, 2),
                    Margin = new Thickness(0, 0, 5, 5),
                    Child = new TextBlock { Text = name, Foreground = accent, FontSize = 9 }
                });
            }
        }

        private void UpdateHomePositionMarker(bool connected, double rawTravel,
            Polyline configGraph, Line positionLine, Ellipse positionMarker)
        {
            PointCollection curve = configGraph.Points;
            if (!connected || curve == null || curve.Count < 2)
            {
                positionLine.Visibility = Visibility.Collapsed;
                positionMarker.Visibility = Visibility.Collapsed;
                return;
            }

            double x = Math.Max(0, Math.Min(202, rawTravel * 202.0 / 65535.0));
            double y = curve[curve.Count - 1].Y;
            for (int point = 1; point < curve.Count; point++)
            {
                if (x <= curve[point].X)
                {
                    double span = curve[point].X - curve[point - 1].X;
                    double fraction = span > 0 ? (x - curve[point - 1].X) / span : 0;
                    y = curve[point - 1].Y + fraction * (curve[point].Y - curve[point - 1].Y);
                    break;
                }
            }
            positionLine.X1 = x;
            positionLine.X2 = x;
            Canvas.SetLeft(positionMarker, Math.Max(0, Math.Min(192, x - 5)));
            Canvas.SetTop(positionMarker, Math.Max(0, Math.Min(68, y - 5)));
            positionLine.Visibility = Visibility.Visible;
            positionMarker.Visibility = Visibility.Visible;
        }

        private void UpdateHomeConfigGraph(int pedal, payloadPedalConfig config, Polyline graph)
        {
            byte[] travel = { config.relativeTravel00, config.relativeTravel01, config.relativeTravel02,
                config.relativeTravel03, config.relativeTravel04, config.relativeTravel05,
                config.relativeTravel06, config.relativeTravel07, config.relativeTravel08,
                config.relativeTravel09, config.relativeTravel10 };
            byte[] force = { config.relativeForce00, config.relativeForce01, config.relativeForce02,
                config.relativeForce03, config.relativeForce04, config.relativeForce05,
                config.relativeForce06, config.relativeForce07, config.relativeForce08,
                config.relativeForce09, config.relativeForce10 };
            int count = Math.Min(11, (int)config.quantityOfControl);
            int signature = count;
            for (int point = 0; point < count; point++)
                signature = unchecked(signature * 31 + travel[point] * 101 + force[point]);
            if (homeCurveSignature[pedal] == signature) return;
            homeCurveSignature[pedal] = signature;
            var x = new List<double>();
            var y = new List<double>();
            for (int point = 0; point < count; point++)
            {
                if (x.Count > 0 && travel[point] <= x[x.Count - 1]) continue;
                x.Add(travel[point]);
                y.Add(force[point]);
            }
            if (x.Count < 2)
            {
                graph.Points = new PointCollection();
                return;
            }
            var curve = Cubic.Interpolate1D(x.ToArray(), y.ToArray(), 51);
            var points = new PointCollection();
            for (int point = 0; point < curve.xs.Length; point++)
            {
                double px = Math.Max(0, Math.Min(100, curve.xs[point])) * 2.02;
                double py = 78 - Math.Max(0, Math.Min(100, curve.ys[point])) * 0.78;
                points.Add(new Point(px, py));
            }
            graph.Points = points;
        }

        private void UpdateHomeDashboard(bool refreshSummary = true)
        {
            if (Plugin == null || HomeGasStatus == null) return;
            if (refreshSummary)
            {
                uint slot = Plugin._calculations.profile_index;
                if (slot < 6)
                {
                    string profileText = String.Format("Slot {0} · {1}",
                        (char)('A' + slot), Plugin.Settings.Profile_name[slot]);
                    if (HomeCurrentProfile.Text != profileText) HomeCurrentProfile.Text = profileText;
                }
                UpdateHomeSlotButton(HomeSlotA, 0);
                UpdateHomeSlotButton(HomeSlotB, 1);
                UpdateHomeSlotButton(HomeSlotC, 2);
                int connected = 0;
                for (int pedal = 0; pedal < 3; pedal++)
                {
                    if (Plugin._calculations.PedalAvailability[pedal] ||
                        Plugin._calculations.PedalSerialAvailability[pedal]) connected++;
                }
                string connectionText = String.Format("{0} of 3 pedals connected", connected);
                if (HomeConnectionSummary.Text != connectionText)
                    HomeConnectionSummary.Text = connectionText;
                string gameText = String.IsNullOrWhiteSpace(Plugin.Current_Game)
                    ? "Current game: none" : "Current game: " + Plugin.Current_Game;
                if (GameProfileCurrentGame.Text != gameText)
                    GameProfileCurrentGame.Text = gameText;
            }
            if (Function_Tab_seleciton.SelectedItem != Tab_Home) return;

            UpdateHomePedal(2, HomeGasStatus, HomeGasValue, HomeGasBar, HomeGasGraph,
                HomeGasConfigGraph, HomeGasPositionLine, HomeGasPositionMarker,
                HomeGasSettings, HomeGasEffects, refreshSummary);
            UpdateHomePedal(1, HomeBrakeStatus, HomeBrakeValue, HomeBrakeBar, HomeBrakeGraph,
                HomeBrakeConfigGraph, HomeBrakePositionLine, HomeBrakePositionMarker,
                HomeBrakeSettings, HomeBrakeEffects, refreshSummary);
            UpdateHomePedal(0, HomeClutchStatus, HomeClutchValue, HomeClutchBar, HomeClutchGraph,
                HomeClutchConfigGraph, HomeClutchPositionLine, HomeClutchPositionMarker,
                HomeClutchSettings, HomeClutchEffects, refreshSummary);
        }

        private void UpdateHomeSlotButton(System.Windows.Controls.Button button, int slot)
        {
            string name = Plugin.Settings.Profile_name[slot];
            string label = String.IsNullOrWhiteSpace(name)
                ? String.Format("Slot {0} Apply", (char)('A' + slot))
                : name.Trim() + " Apply";
            if (!Equals(button.Content, label)) button.Content = label;
            string tooltip = String.Format("Apply Slot {0}: {1}", (char)('A' + slot), name);
            if (!Equals(button.ToolTip, tooltip)) button.ToolTip = tooltip;
        }

        private bool HasLinkedPedal(int slot)
        {
            for (int pedal = 0; pedal < 3; pedal++)
            {
                if (Plugin.Settings.file_enable_check[slot, pedal] == 1) return true;
            }
            return false;
        }

        private sealed class GameProfileMappingItem
        {
            public string GameCode { get; set; }
            public int Slot { get; set; }
            public string SlotName { get; set; }
            public override string ToString()
            {
                return String.Format("{0}  →  {1}: {2}", GameCode, (char)('A' + Slot), SlotName);
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
                GameProfileGame.Items.Clear();
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
                if (!String.IsNullOrWhiteSpace(Plugin.Current_Game))
                    gameCodes.Add(Plugin.Current_Game);
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
                GameProfileStatus.Text = String.Format("{0} game codes available", gameCodes.Count);
            }
            catch (Exception ex)
            {
                GameProfileStatus.Text = "Could not read game list: " + ex.Message;
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
                GameProfileStatus.Text = "Choose a game and a slot first.";
                return;
            }
            var mappings = new Dictionary<string, int>(Plugin.Settings.GameProfileSlots,
                StringComparer.OrdinalIgnoreCase);
            mappings[code] = slot;
            Plugin.Settings.GameProfileSlots = mappings;
            RefreshGameProfileUI();
            GameProfileGame.Text = code;
            GameProfileStatus.Text = String.Format("{0} → Slot {1} saved", code, (char)('A' + slot));
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
            GameProfileStatus.Text = mapping.GameCode + " mapping removed";
            Plugin.RequestAutomaticProfileRefresh();
        }

        private void GameProfileRefresh_Click(object sender, RoutedEventArgs e)
        {
            RefreshGameProfileUI();
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
            if (!HasLinkedPedal((int)slot))
            {
                GameProfileStatus.Text = String.Format("Slot {0} has no enabled pedal files", (char)('A' + slot));
                HomeApplyFeedback.Text = GameProfileStatus.Text;
                SimHub.Logging.Current.Error("DIY pedal auto profile: " + GameProfileStatus.Text);
                return;
            }
            for (int pedal = 0; pedal < 3; pedal++)
            {
                if (Plugin.Settings.file_enable_check[slot, pedal] == 1 &&
                    !File.Exists(Plugin.Settings.Pedal_file_string[slot, pedal]))
                {
                    GameProfileStatus.Text = String.Format("Slot {0} has a missing config file", (char)('A' + slot));
                    HomeApplyFeedback.Text = GameProfileStatus.Text;
                    SimHub.Logging.Current.Error("DIY pedal auto profile: " + GameProfileStatus.Text);
                    return;
                }
            }
            try
            {
                SystemProfile_Tab.Settings = Plugin.Settings;
                SystemProfile_Tab.calculation = Plugin._calculations;
                SystemProfile_Tab.ApplySlot(slot);
                Plugin._calculations.profile_index = slot;
                Profile_change(slot);
                int sent = ApplyHomeSlotToConnectedPedals(slot);
                GameProfileStatus.Text = String.Format("Slot {0} applied · {1} pedal(s) updated",
                    (char)('A' + slot), sent);
                HomeApplyFeedback.Text = "Auto profile: " + GameProfileStatus.Text;
                SimHub.Logging.Current.Info("DIY pedal auto profile: " + GameProfileStatus.Text);
                UpdateHomeDashboard();
            }
            catch (Exception ex)
            {
                GameProfileStatus.Text = "Auto switch failed: " + ex.Message;
                HomeApplyFeedback.Text = GameProfileStatus.Text;
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
