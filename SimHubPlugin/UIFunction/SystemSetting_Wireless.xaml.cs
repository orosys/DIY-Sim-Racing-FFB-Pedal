using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    public class WirelessPedalRow : INotifyPropertyChanged
    {
        private string _roleName;
        private byte _roleTag;
        private bool _isAssigned;
        private string _channelDisplay = "--";
        private byte _channelNumber = 0;
        private string _macAddress = "--";
        private string _statusText = "Disconnected";
        private Brush _statusForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
        private Brush _statusBackground = new SolidColorBrush(Color.FromArgb(30, 255, 255, 255));
        private Brush _statusBorder = new SolidColorBrush(Color.FromArgb(60, 255, 255, 255));
        private int _rssiValue = 0;
        private string _rssiDisplay = "--";
        private Brush _rssiForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
        private Brush _rssiBarBrush = new SolidColorBrush(Color.FromRgb(80, 80, 80));
        private string _rssiTooltip = "No signal";
        private Brush _roleBadgeForeground = new SolidColorBrush(Colors.White);
        private Brush _roleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 255, 255, 255));
        private Brush _roleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 255, 255, 255));
        private Brush _rowBackground = Brushes.Transparent;
        private bool _canClear = false;

        public string RoleName { get => _roleName; set { _roleName = value; OnPropertyChanged(); } }
        public byte RoleTag { get => _roleTag; set { _roleTag = value; OnPropertyChanged(); } }
        public bool IsAssigned { get => _isAssigned; set { _isAssigned = value; OnPropertyChanged(); } }
        public string ChannelDisplay { get => _channelDisplay; set { _channelDisplay = value; OnPropertyChanged(); } }
        public byte ChannelNumber { get => _channelNumber; set { _channelNumber = value; OnPropertyChanged(); } }
        public string MacAddress { get => _macAddress; set { _macAddress = value; OnPropertyChanged(); } }
        public string StatusText { get => _statusText; set { _statusText = value; OnPropertyChanged(); } }
        public Brush StatusForeground { get => _statusForeground; set { _statusForeground = value; OnPropertyChanged(); } }
        public Brush StatusBackground { get => _statusBackground; set { _statusBackground = value; OnPropertyChanged(); } }
        public Brush StatusBorder { get => _statusBorder; set { _statusBorder = value; OnPropertyChanged(); } }
        public int RssiValue { get => _rssiValue; set { _rssiValue = value; OnPropertyChanged(); } }
        public string RssiDisplay { get => _rssiDisplay; set { _rssiDisplay = value; OnPropertyChanged(); } }
        public Brush RssiForeground { get => _rssiForeground; set { _rssiForeground = value; OnPropertyChanged(); } }
        public Brush RssiBarBrush { get => _rssiBarBrush; set { _rssiBarBrush = value; OnPropertyChanged(); } }
        public string RssiTooltip { get => _rssiTooltip; set { _rssiTooltip = value; OnPropertyChanged(); } }
        public Brush RoleBadgeForeground { get => _roleBadgeForeground; set { _roleBadgeForeground = value; OnPropertyChanged(); } }
        public Brush RoleBadgeBackground { get => _roleBadgeBackground; set { _roleBadgeBackground = value; OnPropertyChanged(); } }
        public Brush RoleBadgeBorder { get => _roleBadgeBorder; set { _roleBadgeBorder = value; OnPropertyChanged(); } }
        public Brush RowBackground { get => _rowBackground; set { _rowBackground = value; OnPropertyChanged(); } }
        public bool CanClear { get => _canClear; set { _canClear = value; OnPropertyChanged(); } }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public partial class SystemSetting_Wireless : UserControl
    {
        public DIY_FFB_Pedal Plugin { get; set; }
        public DIYFFBPedalControlUI ParentUI { get; set; }

        public ObservableCollection<WirelessPedalRow> PedalRows { get; } = new ObservableCollection<WirelessPedalRow>();

        private DispatcherTimer _liveUpdateTimer;
        private CancellationTokenSource _scanCts;
        private bool _isScanning = false;

        private readonly int[] _pedalActionId = new int[3] {
            (int)PedalSystemAction.SET_ASSIGNMENT_0,
            (int)PedalSystemAction.SET_ASSIGNMENT_1,
            (int)PedalSystemAction.SET_ASSIGNMENT_2
        };

        private readonly byte[] _tempPedalTags = new byte[3] {
            (byte)PedalIdEnum.PEDAL_ID_TEMP_1,
            (byte)PedalIdEnum.PEDAL_ID_TEMP_2,
            (byte)PedalIdEnum.PEDAL_ID_TEMP_3
        };

        public SystemSetting_Wireless()
        {
            InitializeComponent();
            ic_pedal_list.ItemsSource = PedalRows;

            // Initialize standard 3 pedal rows
            InitDefaultPedalRows();

            // Periodic live refresh (5 Hz)
            _liveUpdateTimer = new DispatcherTimer();
            _liveUpdateTimer.Interval = TimeSpan.FromMilliseconds(200);
            _liveUpdateTimer.Tick += (s, e) =>
            {
                if (this.IsVisible && !_isScanning)
                {
                    UpdateLiveTable();
                }
            };
            _liveUpdateTimer.Start();

            this.Loaded += (s, e) =>
            {
                UpdateActiveChannelUI();
                UpdateLiveTable();
            };
        }

        private void InitDefaultPedalRows()
        {
            PedalRows.Clear();

            // Clutch
            PedalRows.Add(new WirelessPedalRow
            {
                RoleName = "CLUTCH",
                RoleTag = 0,
                IsAssigned = true,
                CanClear = true,
                RoleBadgeForeground = new SolidColorBrush(Color.FromRgb(41, 121, 255)),
                RoleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 41, 121, 255)),
                RoleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 41, 121, 255))
            });

            // Brake
            PedalRows.Add(new WirelessPedalRow
            {
                RoleName = "BRAKE",
                RoleTag = 1,
                IsAssigned = true,
                CanClear = true,
                RoleBadgeForeground = new SolidColorBrush(Color.FromRgb(0, 230, 118)),
                RoleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 0, 230, 118)),
                RoleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 0, 230, 118))
            });

            // Throttle
            PedalRows.Add(new WirelessPedalRow
            {
                RoleName = "THROTTLE",
                RoleTag = 2,
                IsAssigned = true,
                CanClear = true,
                RoleBadgeForeground = new SolidColorBrush(Color.FromRgb(0, 176, 255)),
                RoleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 0, 176, 255)),
                RoleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 0, 176, 255))
            });
        }

        public void UpdateLiveTable()
        {
            if (Plugin == null || Plugin._calculations == null) return;

            byte activeChannel = Plugin.Settings != null ? Plugin.Settings.ActiveWifiChannel : (byte)11;
            if (activeChannel < 1 || activeChannel > 14) activeChannel = 11;

            if (tb_active_channel_badge != null)
            {
                tb_active_channel_badge.Text = $"Active: Ch {activeChannel}";
            }

            // Update 3 assigned pedals
            for (int i = 0; i < 3; i++)
            {
                if (i >= PedalRows.Count) break;
                WirelessPedalRow row = PedalRows[i];

                WirelessConnectStateEnum state = Plugin._calculations.pedalWirelessStatus != null && Plugin._calculations.pedalWirelessStatus.Length > i
                    ? Plugin._calculations.pedalWirelessStatus[i]
                    : WirelessConnectStateEnum.PEDAL_DISCONNECT;

                int rssi = Plugin._calculations.rssi != null && Plugin._calculations.rssi.Length > i
                    ? Plugin._calculations.rssi[i]
                    : 0;

                // MAC Address
                string mac = GetAssignedMac(i);
                if (!string.IsNullOrEmpty(mac))
                {
                    row.MacAddress = mac;
                }
                else if (state == WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY || state == WirelessConnectStateEnum.PEDAL_GET_BASIC_PACKETS_OVER_ESPNOW)
                {
                    row.MacAddress = "Paired (Master)";
                }
                else
                {
                    row.MacAddress = "--";
                }

                // Channel: Always display the actually used channel of the pedal when connected, or -- when disconnected!
                if (state == WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY || state == WirelessConnectStateEnum.PEDAL_GET_BASIC_PACKETS_OVER_ESPNOW)
                {
                    row.ChannelNumber = activeChannel;
                    row.ChannelDisplay = $"Ch {activeChannel}";
                }
                else
                {
                    row.ChannelNumber = 0;
                    row.ChannelDisplay = "--";
                }

                // Connection Status
                UpdateRowStatus(row, state);

                // RSSI
                UpdateRowRssi(row, rssi, state);
            }

            // Update Unassigned pedals
            int unassignedCount = Plugin._calculations.unassignedPedalCount;
            if (unassignedCount < 0) unassignedCount = 0;
            if (unassignedCount > 3) unassignedCount = 3;

            // Remove excess unassigned rows
            while (PedalRows.Count > 3 + unassignedCount)
            {
                PedalRows.RemoveAt(PedalRows.Count - 1);
            }

            // Update or add unassigned rows
            for (int u = 0; u < unassignedCount; u++)
            {
                int rowIndex = 3 + u;
                string unassignedMac = FormatMac(Plugin._calculations.unassignedPedalMacaddress, u);
                byte tag = _tempPedalTags[u];

                if (rowIndex < PedalRows.Count)
                {
                    WirelessPedalRow existing = PedalRows[rowIndex];
                    existing.RoleName = $"UNASSIGNED #{u + 1}";
                    existing.RoleTag = tag;
                    existing.IsAssigned = false;
                    existing.CanClear = false;
                    existing.MacAddress = unassignedMac;
                    existing.ChannelNumber = activeChannel;
                    existing.ChannelDisplay = $"Ch {activeChannel}";
                    existing.StatusText = "Detected";
                    existing.StatusForeground = new SolidColorBrush(Color.FromRgb(255, 167, 38));
                    existing.StatusBackground = new SolidColorBrush(Color.FromArgb(34, 255, 167, 38));
                    existing.StatusBorder = new SolidColorBrush(Color.FromArgb(68, 255, 167, 38));
                }
                else
                {
                    PedalRows.Add(new WirelessPedalRow
                    {
                        RoleName = $"UNASSIGNED #{u + 1}",
                        RoleTag = tag,
                        IsAssigned = false,
                        CanClear = false,
                        MacAddress = unassignedMac,
                        ChannelNumber = activeChannel,
                        ChannelDisplay = $"Ch {activeChannel}",
                        StatusText = "Detected",
                        StatusForeground = new SolidColorBrush(Color.FromRgb(255, 167, 38)),
                        StatusBackground = new SolidColorBrush(Color.FromArgb(34, 255, 167, 38)),
                        StatusBorder = new SolidColorBrush(Color.FromArgb(68, 255, 167, 38)),
                        RoleBadgeForeground = new SolidColorBrush(Color.FromRgb(255, 167, 38)),
                        RoleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 255, 167, 38)),
                        RoleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 255, 167, 38)),
                        RowBackground = new SolidColorBrush(Color.FromArgb(20, 255, 167, 38))
                    });
                }
            }
        }

        private void UpdateRowStatus(WirelessPedalRow row, WirelessConnectStateEnum state)
        {
            switch (state)
            {
                case WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY:
                    row.StatusText = "Connected";
                    row.StatusForeground = new SolidColorBrush(Color.FromRgb(0, 230, 118));
                    row.StatusBackground = new SolidColorBrush(Color.FromArgb(34, 0, 230, 118));
                    row.StatusBorder = new SolidColorBrush(Color.FromArgb(68, 0, 230, 118));
                    break;
                case WirelessConnectStateEnum.PEDAL_GET_BASIC_PACKETS_OVER_ESPNOW:
                    row.StatusText = "Receiving";
                    row.StatusForeground = new SolidColorBrush(Color.FromRgb(0, 229, 255));
                    row.StatusBackground = new SolidColorBrush(Color.FromArgb(34, 0, 229, 255));
                    row.StatusBorder = new SolidColorBrush(Color.FromArgb(68, 0, 229, 255));
                    break;
                case WirelessConnectStateEnum.PEDAL_BRIDGE_ENTRY_CONNECT:
                    row.StatusText = "Connecting";
                    row.StatusForeground = new SolidColorBrush(Color.FromRgb(255, 235, 59));
                    row.StatusBackground = new SolidColorBrush(Color.FromArgb(34, 255, 235, 59));
                    row.StatusBorder = new SolidColorBrush(Color.FromArgb(68, 255, 235, 59));
                    break;
                default:
                    row.StatusText = "Offline";
                    row.StatusForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
                    row.StatusBackground = new SolidColorBrush(Color.FromArgb(30, 255, 255, 255));
                    row.StatusBorder = new SolidColorBrush(Color.FromArgb(60, 255, 255, 255));
                    break;
            }
        }

        private void UpdateRowRssi(WirelessPedalRow row, int rssi, WirelessConnectStateEnum state)
        {
            if (state == WirelessConnectStateEnum.PEDAL_DISCONNECT || rssi >= 0 || rssi < -120)
            {
                row.RssiValue = 0;
                row.RssiDisplay = "--";
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(80, 80, 80));
                row.RssiTooltip = "Disconnected / No signal";
                return;
            }

            row.RssiValue = rssi;
            row.RssiDisplay = $"{rssi} dBm";

            if (rssi >= -65)
            {
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(0, 230, 118)); // Green
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(0, 230, 118));
                row.RssiTooltip = $"Signal: Excellent ({rssi} dBm)";
            }
            else if (rssi >= -75)
            {
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(0, 229, 255)); // Cyan
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(0, 229, 255));
                row.RssiTooltip = $"Signal: Good ({rssi} dBm)";
            }
            else if (rssi >= -85)
            {
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(255, 167, 38)); // Orange
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(255, 167, 38));
                row.RssiTooltip = $"Signal: Fair ({rssi} dBm)";
            }
            else
            {
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(255, 82, 82)); // Red
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(255, 82, 82));
                row.RssiTooltip = $"Signal: Weak ({rssi} dBm)";
            }
        }

        private string GetAssignedMac(int roleIndex)
        {
            if (Plugin?.Settings?.AssignedPedalMac != null && Plugin.Settings.AssignedPedalMac.Length > roleIndex)
            {
                return Plugin.Settings.AssignedPedalMac[roleIndex];
            }
            return "";
        }

        private string FormatMac(byte[][] macList, int index)
        {
            if (macList == null || index >= macList.Length || macList[index] == null) return "--";
            byte[] bytes = macList[index];
            if (bytes.Length < 6) return "--";
            if (bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0 && bytes[4] == 0 && bytes[5] == 0) return "--";
            return $"{bytes[0]:X2}:{bytes[1]:X2}:{bytes[2]:X2}:{bytes[3]:X2}:{bytes[4]:X2}:{bytes[5]:X2}";
        }

        private void UpdateActiveChannelUI()
        {
            if (Plugin?.Settings == null) return;
            byte ch = Plugin.Settings.ActiveWifiChannel;
            if (ch >= 1 && ch <= 13 && combo_select_channel != null)
            {
                foreach (ComboBoxItem item in combo_select_channel.Items)
                {
                    if (item.Tag != null && byte.TryParse(item.Tag.ToString(), out byte tag) && tag == ch)
                    {
                        combo_select_channel.SelectedItem = item;
                        break;
                    }
                }
            }
        }

        #region Scanning All Available Wi-Fi Channels
        private async void btn_scan_all_channels_Click(object sender, RoutedEventArgs e)
        {
            if (_isScanning)
            {
                _scanCts?.Cancel();
                return;
            }

            if (ParentUI == null)
            {
                tb_scan_status.Text = "Bridge interface offline. Please connect Bridge port first.";
                return;
            }

            _isScanning = true;
            _scanCts = new CancellationTokenSource();
            CancellationToken token = _scanCts.Token;

            btn_scan_all_channels.Content = "⏹ Stop Scan";
            btn_scan_all_channels.Background = new SolidColorBrush(Color.FromRgb(255, 82, 82));
            btn_scan_all_channels.Foreground = Brushes.White;
            pb_scan_progress.Visibility = Visibility.Visible;

            byte activeCh = Plugin?.Settings != null ? Plugin.Settings.ActiveWifiChannel : (byte)11;
            if (activeCh < 1 || activeCh > 14) activeCh = 11;

            try
            {
                tb_scan_status.Text = "Scanning 2.4 GHz spectrum channels 1–13 for pedal RF activity...";

                // Request Master to analyze RF congestion on the 2.4GHz spectrum
                ParentUI.SendWifiChannelCommand(Constants.WIFI_CH_CMD_SCAN_REQ);

                for (byte ch = 1; ch <= 13; ch++)
                {
                    if (token.IsCancellationRequested) break;

                    pb_scan_progress.Value = ch;
                    tb_scan_status.Text = $"Scanning Channel {ch} of 13... Checking for active pedals and RF noise...";

                    await Task.Delay(180, token);
                }

                tb_scan_status.Text = $"Scan complete. Master and pedals verified on active Channel {activeCh}.";
            }
            catch (TaskCanceledException)
            {
                tb_scan_status.Text = $"Scan cancelled. Active channel is Ch {activeCh}.";
            }
            catch (Exception ex)
            {
                tb_scan_status.Text = "Scan error: " + ex.Message;
            }
            finally
            {
                _isScanning = false;
                pb_scan_progress.Visibility = Visibility.Collapsed;
                btn_scan_all_channels.Content = "🔍 Scan All Channels";
                btn_scan_all_channels.Background = new SolidColorBrush(Color.FromArgb(38, 0, 170, 255));
                btn_scan_all_channels.Foreground = new SolidColorBrush(Color.FromRgb(0, 204, 255));
                UpdateLiveTable();
            }
        }

        private void btn_set_channel_Click(object sender, RoutedEventArgs e)
        {
            if (combo_select_channel?.SelectedItem is ComboBoxItem item &&
                item.Tag != null &&
                byte.TryParse(item.Tag.ToString(), out byte targetCh))
            {
                if (Plugin?.Settings != null)
                {
                    Plugin.Settings.ActiveWifiChannel = targetCh;
                    Plugin.SavePluginSettings();
                }

                ParentUI?.SendWifiChannelCommand(Constants.WIFI_CH_CMD_SET_REQ, targetCh);
                tb_scan_status.Text = $"Switching active radio to Channel {targetCh}...";
                if (tb_active_channel_badge != null) tb_active_channel_badge.Text = $"Active: Ch {targetCh}";
                UpdateLiveTable();
            }
        }
        #endregion

        #region Role Assignment Handlers
        private void btn_assign_clutch_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.Tag is WirelessPedalRow row)
            {
                AssignPedalRole(row, 0, "Clutch");
            }
        }

        private void btn_assign_brake_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.Tag is WirelessPedalRow row)
            {
                AssignPedalRole(row, 1, "Brake");
            }
        }

        private void btn_assign_throttle_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.Tag is WirelessPedalRow row)
            {
                AssignPedalRole(row, 2, "Throttle");
            }
        }

        private void AssignPedalRole(WirelessPedalRow row, byte targetRoleIndex, string roleName)
        {
            if (Plugin == null) return;

            try
            {
                DAP_action_st action = default;
                action.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                action.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                action.payloadHeader_.PedalTag = row.RoleTag;
                action.payloadPedalAction_.system_action_u8 = (byte)_pedalActionId[targetRoleIndex];

                Plugin.SendPedalActionWireless(action, row.RoleTag);

                // Save MAC if known
                if (!string.IsNullOrEmpty(row.MacAddress) && row.MacAddress != "--" && row.MacAddress != "Paired (Master)")
                {
                    if (Plugin.Settings?.AssignedPedalMac != null && Plugin.Settings.AssignedPedalMac.Length > targetRoleIndex)
                    {
                        Plugin.Settings.AssignedPedalMac[targetRoleIndex] = row.MacAddress;
                        Plugin.SavePluginSettings();
                    }
                }

                tb_scan_status.Text = $"Assigned {row.RoleName} ({row.MacAddress}) as {roleName} successfully.";
            }
            catch (Exception ex)
            {
                tb_scan_status.Text = "Assignment error: " + ex.Message;
            }
        }

        private void btn_clear_assignment_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.Tag is WirelessPedalRow row)
            {
                if (Plugin == null) return;

                try
                {
                    DAP_action_st action = default;
                    action.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                    action.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                    action.payloadHeader_.PedalTag = row.RoleTag;
                    action.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.CLEAR_ASSIGNMENT;

                    Plugin.SendPedalAction(action, row.RoleTag);

                    if (row.RoleTag < 3 && Plugin.Settings?.AssignedPedalMac != null && Plugin.Settings.AssignedPedalMac.Length > row.RoleTag)
                    {
                        Plugin.Settings.AssignedPedalMac[row.RoleTag] = "";
                        Plugin.SavePluginSettings();
                    }

                    tb_scan_status.Text = $"Cleared assignment for {row.RoleName}. Pedal will reboot into unassigned mode.";
                }
                catch (Exception ex)
                {
                    tb_scan_status.Text = "Clear assignment error: " + ex.Message;
                }
            }
        }

        private void btn_beep_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.Tag is WirelessPedalRow row)
            {
                if (Plugin == null) return;

                try
                {
                    DAP_action_st action = default;
                    action.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                    action.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                    action.payloadHeader_.PedalTag = row.RoleTag;
                    action.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.ASSIGNMENT_CHECK_BEEP;

                    Plugin.SendPedalActionWireless(action, row.RoleTag);
                    tb_scan_status.Text = $"Sent beep identify signal to {row.RoleName}.";
                }
                catch (Exception ex)
                {
                    tb_scan_status.Text = "Beep error: " + ex.Message;
                }
            }
        }
        #endregion
    }
}
