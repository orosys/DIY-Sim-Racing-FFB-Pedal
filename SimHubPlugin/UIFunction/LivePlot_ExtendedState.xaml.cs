using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    public partial class LivePlot_ExtendedState : UserControl
    {
        public class SignalDef
        {
            public string Id { get; set; }
            public string Name { get; set; }
            public string Subsystem { get; set; }
            public string Unit { get; set; }
            public Color DefaultColor { get; set; }
            public Func<payloadPedalState_Extended, double> Getter { get; set; }
            public string Format { get; set; } = "F2";
        }

        private class TelemetryPoint
        {
            public DateTime Timestamp;
            public payloadPedalState_Extended State;
        }

        // All supported signals from payloadPedalState_Extended
        private static readonly List<SignalDef> AllSignals = new List<SignalDef>
        {
            // Servo Registers
            new SignalDef { Id = "servo_pos_target", Name = "Servo Target Pos", Subsystem = "Servo Registers", Unit = "cts", DefaultColor = Color.FromRgb(0xFF, 0xD7, 0x40), Getter = s => s.servoPositionTarget_i32, Format = "F0" },
            new SignalDef { Id = "servo_pos_fb", Name = "Servo Feedback Pos", Subsystem = "Servo Registers", Unit = "cts", DefaultColor = Color.FromRgb(0xFF, 0x52, 0x52), Getter = s => s.servoPositionFeedback_i32, Format = "F0" },
            new SignalDef { Id = "servo_pos_err", Name = "Servo Pos Error", Subsystem = "Servo Registers", Unit = "cts", DefaultColor = Color.FromRgb(0xFF, 0x40, 0x81), Getter = s => s.servoPositionError_i16, Format = "F0" },
            new SignalDef { Id = "servo_voltage", Name = "Servo Voltage", Subsystem = "Servo Registers", Unit = "V", DefaultColor = Color.FromRgb(0xFF, 0xFF, 0x00), Getter = s => s.servoVoltage0p1V_i16 / 10.0, Format = "F1" },
            new SignalDef { Id = "servo_current", Name = "Servo Current", Subsystem = "Servo Registers", Unit = "%", DefaultColor = Color.FromRgb(0x69, 0xF0, 0xAE), Getter = s => s.servoCurrentPercent_i16, Format = "F0" },
            new SignalDef { Id = "servo_cycle", Name = "Servo Cycle Count", Subsystem = "Servo Registers", Unit = "cts", DefaultColor = Color.FromRgb(0xB0, 0xBE, 0xC5), Getter = s => s.servoStateCycleCount_u32, Format = "F0" },

            // ESP32 & Physical Values
            new SignalDef { Id = "force_filtered", Name = "Filtered Force", Subsystem = "ESP32 & Forces", Unit = "kg", DefaultColor = Color.FromRgb(0x00, 0xE5, 0xFF), Getter = s => s.pedalForceFiltered_fl32, Format = "F2" },
            new SignalDef { Id = "force_raw", Name = "Raw Force", Subsystem = "ESP32 & Forces", Unit = "kg", DefaultColor = Color.FromRgb(0x40, 0xC4, 0xFF), Getter = s => s.pedalForceRaw_fl32, Format = "F2" },
            new SignalDef { Id = "force_vel_est", Name = "Force Velocity Est", Subsystem = "ESP32 & Forces", Unit = "kg/s", DefaultColor = Color.FromRgb(0x76, 0xFF, 0x03), Getter = s => s.forceVelEst_fl32, Format = "F2" },
            new SignalDef { Id = "esp_target_pos", Name = "ESP Target Pos", Subsystem = "ESP32 & Forces", Unit = "cts", DefaultColor = Color.FromRgb(0xFF, 0xAB, 0x00), Getter = s => s.targetPosition_i32, Format = "F0" },
            new SignalDef { Id = "speed_hz", Name = "Current Speed", Subsystem = "ESP32 & Forces", Unit = "Hz", DefaultColor = Color.FromRgb(0xFF, 0x6D, 0x00), Getter = s => s.currentSpeedInHz_i32, Format = "F0" },
            new SignalDef { Id = "brake_resistor", Name = "Brake Resistor", Subsystem = "ESP32 & Forces", Unit = "", DefaultColor = Color.FromRgb(0xEA, 0x80, 0xFC), Getter = s => s.brakeResistorState_b, Format = "F0" },
            new SignalDef { Id = "osc_monitor", Name = "Oscillation Monitor", Subsystem = "ESP32 & Forces", Unit = "", DefaultColor = Color.FromRgb(0xD5, 0x00, 0xF9), Getter = s => s.oscillationMonitorValue_u8, Format = "F0" },
            new SignalDef { Id = "esp_cycle", Name = "ESP Cycle Count", Subsystem = "ESP32 & Forces", Unit = "cts", DefaultColor = Color.FromRgb(0xCF, 0xD8, 0xDC), Getter = s => s.cycleCount_u32, Format = "F0" },

            // Admittance Model
            new SignalDef { Id = "adm_expected_force", Name = "Admittance Exp Force", Subsystem = "Admittance Model", Unit = "N", DefaultColor = Color.FromRgb(0xE0, 0x40, 0xFB), Getter = s => s.admittance_expectedForce_N, Format = "F2" },
            new SignalDef { Id = "adm_is_oscillating", Name = "Is Oscillating", Subsystem = "Admittance Model", Unit = "", DefaultColor = Color.FromRgb(0xFF, 0x17, 0x44), Getter = s => s.admittance_isOscillating, Format = "F0" },
            new SignalDef { Id = "adm_psi", Name = "Admittance Psi", Subsystem = "Admittance Model", Unit = "N", DefaultColor = Color.FromRgb(0xAA, 0x00, 0xFF), Getter = s => s.admittance_admittancePsi_N, Format = "F2" },
            new SignalDef { Id = "adm_mass", Name = "Virtual Mass", Subsystem = "Admittance Model", Unit = "kg", DefaultColor = Color.FromRgb(0x00, 0xB0, 0xFF), Getter = s => s.admittance_virtualMass_kg, Format = "F2" },
            new SignalDef { Id = "adm_damping", Name = "Virtual Damping", Subsystem = "Admittance Model", Unit = "Ns/m", DefaultColor = Color.FromRgb(0x1D, 0xE9, 0xB6), Getter = s => s.admittance_virtualDamping_Ns_m, Format = "F2" },
            new SignalDef { Id = "adm_pos", Name = "Virtual Position", Subsystem = "Admittance Model", Unit = "m", DefaultColor = Color.FromRgb(0xAE, 0xEA, 0x00), Getter = s => s.admittance_virtualPosition_m, Format = "F4" },
            new SignalDef { Id = "adm_vel", Name = "Virtual Velocity", Subsystem = "Admittance Model", Unit = "m/s", DefaultColor = Color.FromRgb(0x00, 0xE6, 0x76), Getter = s => s.admittance_virtualVelocity_mps, Format = "F3" },
            new SignalDef { Id = "adm_acc", Name = "Virtual Accel", Subsystem = "Admittance Model", Unit = "m/s\u00B2", DefaultColor = Color.FromRgb(0x7C, 0x4D, 0xFF), Getter = s => s.admittance_virtualAcceleration_mps2, Format = "F2" }
        };

        // Active signals list
        private readonly HashSet<string> _activeSignalIds = new HashSet<string>();
        private readonly Dictionary<string, Polyline> _signalPolylines = new Dictionary<string, Polyline>();

        // Telemetry buffer
        private readonly object _dataLock = new object();
        private readonly List<TelemetryPoint> _points = new List<TelemetryPoint>();
        private const int MAX_BUFFER_POINTS = 3000;
        private DateTime _lastSampleTime = DateTime.MinValue;

        // Render timer
        private DispatcherTimer _renderTimer;
        private double _windowSeconds = 5.0;
        private bool _isPaused = false;
        private bool _autoScale = true;
        private int _selectedPedal = 1; // Default to Brake (1)

        // Packet rate calculation
        private int _packetCounter = 0;
        private DateTime _lastRateCheck = DateTime.UtcNow;
        private double _packetRate = 0.0;
        private DateTime _lastPacketTime = DateTime.MinValue;

        // Cached visible points for hover lookup
        private List<TelemetryPoint> _cachedVisiblePoints = new List<TelemetryPoint>();
        private readonly Dictionary<string, (double min, double max)> _cachedSignalRanges = new Dictionary<string, (double, double)>();
        private DateTime _lastRenderNow = DateTime.UtcNow;

        // References to SimHub plugin & parent UI
        public DIY_FFB_Pedal Plugin { get; set; }
        public DIYFFBPedalControlUI ParentUI { get; set; }

        public LivePlot_ExtendedState()
        {
            InitializeComponent();
            InitControls();
            InitContextMenu();
            InitRenderTimer();

            // Default active signals: Filtered Force & Feedback Position
            _activeSignalIds.Add("force_filtered");
            _activeSignalIds.Add("servo_pos_fb");
            RebuildPolylinesAndLegends();
        }

        public void SetReferences(DIY_FFB_Pedal plugin, DIYFFBPedalControlUI parentUI)
        {
            Plugin = plugin;
            ParentUI = parentUI;

            if (Plugin != null && Plugin._calculations != null)
            {
                Plugin._calculations.OnExtendedStateReceived -= OnPacketReceived;
                Plugin._calculations.OnExtendedStateReceived += OnPacketReceived;
            }

            UpdatePedalButtonStyles();
        }

        public void OnTabSelected(bool isSelected)
        {
            if (isSelected)
            {
                EnsureStreaming(_selectedPedal, true);
                if (_renderTimer != null && !_renderTimer.IsEnabled) _renderTimer.Start();
            }
            else
            {
                EnsureStreaming(_selectedPedal, false);
                if (_renderTimer != null && _renderTimer.IsEnabled) _renderTimer.Stop();
            }
        }

        private void EnsureStreaming(int pedalIndex, bool enable)
        {
            if (ParentUI == null || ParentUI.Plugin == null) return;
            try
            {
                var config = ParentUI.dap_config_st[pedalIndex];
                byte oldFlags = config.payloadPedalConfig_.debug_flags_0;
                if (enable)
                {
                    config.payloadPedalConfig_.debug_flags_0 |= 64;
                }
                else
                {
                    // Only disable if trace file dump is NOT currently running
                    if (!ParentUI.Plugin._calculations.dumpPedalToResponseFile[pedalIndex])
                    {
                        config.payloadPedalConfig_.debug_flags_0 = (byte)(config.payloadPedalConfig_.debug_flags_0 & ~64);
                    }
                }
                if (config.payloadPedalConfig_.debug_flags_0 != oldFlags)
                {
                    ParentUI.dap_config_st[pedalIndex] = config;
                    ParentUI.Plugin.SendConfigWithoutSaveToEEPROM(config, (byte)pedalIndex);
                }
            }
            catch { }
        }

        private void InitControls()
        {
            // Time window combo options
            cb_time_window.Items.Add("3s");
            cb_time_window.Items.Add("5s");
            cb_time_window.Items.Add("10s");
            cb_time_window.Items.Add("20s");
            cb_time_window.Items.Add("30s");
            cb_time_window.SelectedIndex = 1; // 5s default

            UpdatePedalButtonStyles();
        }

        private void InitContextMenu()
        {
            plot_context_menu.Items.Clear();

            var groups = AllSignals.GroupBy(s => s.Subsystem);
            foreach (var group in groups)
            {
                var subMenu = new MenuItem { Header = group.Key };
                foreach (var sig in group)
                {
                    var item = new MenuItem
                    {
                        Header = sig.Name + (!string.IsNullOrEmpty(sig.Unit) ? $" ({sig.Unit})" : ""),
                        Tag = sig.Id,
                        IsCheckable = true,
                        IsChecked = _activeSignalIds.Contains(sig.Id)
                    };
                    item.Click += MenuItem_Click;
                    subMenu.Items.Add(item);
                }
                plot_context_menu.Items.Add(subMenu);
            }

            plot_context_menu.Items.Add(new Separator());

            var itemReset = new MenuItem { Header = "Reset to Defaults (Force + Pos)" };
            itemReset.Click += (s, e) => ResetToDefaultSignals();
            plot_context_menu.Items.Add(itemReset);

            var itemClear = new MenuItem { Header = "Clear All Signals" };
            itemClear.Click += (s, e) => ClearAllSignals();
            plot_context_menu.Items.Add(itemClear);
        }

        private void Plot_context_menu_Opened(object sender, RoutedEventArgs e)
        {
            foreach (var item in plot_context_menu.Items)
            {
                if (item is MenuItem subMenu && subMenu.HasItems)
                {
                    foreach (var subItem in subMenu.Items)
                    {
                        if (subItem is MenuItem checkItem && checkItem.Tag is string sigId)
                        {
                            checkItem.IsChecked = _activeSignalIds.Contains(sigId);
                        }
                    }
                }
            }
        }

        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuItem item && item.Tag is string sigId)
            {
                ToggleSignal(sigId);
            }
        }

        private void BtnSignalsMenu_Click(object sender, RoutedEventArgs e)
        {
            plot_context_menu.PlacementTarget = btn_signals_menu;
            plot_context_menu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
            plot_context_menu.IsOpen = true;
        }

        private void ToggleSignal(string sigId)
        {
            if (_activeSignalIds.Contains(sigId))
            {
                _activeSignalIds.Remove(sigId);
            }
            else
            {
                _activeSignalIds.Add(sigId);
            }
            RebuildPolylinesAndLegends();
        }

        private void ClearAllSignals()
        {
            _activeSignalIds.Clear();
            RebuildPolylinesAndLegends();
        }

        private void ResetToDefaultSignals()
        {
            _activeSignalIds.Clear();
            _activeSignalIds.Add("force_filtered");
            _activeSignalIds.Add("servo_pos_fb");
            RebuildPolylinesAndLegends();
        }

        private void BtnClearAllSignals_Click(object sender, RoutedEventArgs e) => ClearAllSignals();
        private void BtnResetDefaultSignals_Click(object sender, RoutedEventArgs e) => ResetToDefaultSignals();

        private void RebuildPolylinesAndLegends()
        {
            canvas_plot.Children.Clear();
            _signalPolylines.Clear();
            panel_legends.Children.Clear();

            tb_active_count.Text = $"({_activeSignalIds.Count} active)";
            tb_no_signals_hint.Visibility = _activeSignalIds.Count == 0 ? Visibility.Visible : Visibility.Collapsed;

            foreach (var sigId in _activeSignalIds)
            {
                var sig = AllSignals.FirstOrDefault(s => s.Id == sigId);
                if (sig == null) continue;

                // Create Polyline
                var poly = new Polyline
                {
                    Stroke = new SolidColorBrush(sig.DefaultColor),
                    StrokeThickness = 2.0,
                    StrokeLineJoin = PenLineJoin.Round,
                    SnapsToDevicePixels = true,
                    IsHitTestVisible = false
                };
                _signalPolylines[sigId] = poly;
                canvas_plot.Children.Add(poly);

                // Create Legend Badge
                var legendBorder = new Border
                {
                    Background = new SolidColorBrush(Color.FromRgb(0x28, 0x28, 0x28)),
                    BorderBrush = new SolidColorBrush(Color.FromArgb(0x80, sig.DefaultColor.R, sig.DefaultColor.G, sig.DefaultColor.B)),
                    BorderThickness = new Thickness(1),
                    CornerRadius = new CornerRadius(3),
                    Padding = new Thickness(6, 3, 6, 3),
                    Margin = new Thickness(0, 0, 6, 4),
                    Tag = sigId
                };

                var sp = new StackPanel { Orientation = Orientation.Horizontal, VerticalAlignment = VerticalAlignment.Center };

                var swatch = new Border
                {
                    Width = 8,
                    Height = 8,
                    CornerRadius = new CornerRadius(4),
                    Background = new SolidColorBrush(sig.DefaultColor),
                    Margin = new Thickness(0, 0, 5, 0),
                    VerticalAlignment = VerticalAlignment.Center
                };
                sp.Children.Add(swatch);

                var tbName = new TextBlock
                {
                    Text = sig.Name + ": ",
                    FontWeight = FontWeights.SemiBold,
                    Foreground = Brushes.White,
                    FontSize = 10,
                    VerticalAlignment = VerticalAlignment.Center
                };
                sp.Children.Add(tbName);

                var tbVal = new TextBlock
                {
                    Name = "val_" + sigId,
                    Text = "-- " + sig.Unit,
                    Foreground = new SolidColorBrush(sig.DefaultColor),
                    FontSize = 10,
                    FontWeight = FontWeights.Bold,
                    VerticalAlignment = VerticalAlignment.Center,
                    Margin = new Thickness(0, 0, 6, 0)
                };
                sp.Children.Add(tbVal);

                var tbRange = new TextBlock
                {
                    Name = "range_" + sigId,
                    Text = "[-- / --]",
                    Foreground = new SolidColorBrush(Color.FromRgb(0x9E, 0x9E, 0x9E)),
                    FontSize = 9,
                    VerticalAlignment = VerticalAlignment.Center,
                    Margin = new Thickness(0, 0, 6, 0)
                };
                sp.Children.Add(tbRange);

                var btnRemove = new Button
                {
                    Content = "×",
                    FontSize = 12,
                    FontWeight = FontWeights.Bold,
                    Foreground = new SolidColorBrush(Color.FromRgb(0xAA, 0xAA, 0xAA)),
                    Background = Brushes.Transparent,
                    BorderThickness = new Thickness(0),
                    Padding = new Thickness(2, 0, 2, 0),
                    Cursor = Cursors.Hand,
                    VerticalAlignment = VerticalAlignment.Center,
                    Tag = sigId
                };
                btnRemove.Click += (s, e) =>
                {
                    if (s is Button b && b.Tag is string id)
                    {
                        ToggleSignal(id);
                    }
                };
                sp.Children.Add(btnRemove);

                legendBorder.Child = sp;
                panel_legends.Children.Add(legendBorder);
            }
        }

        private void InitRenderTimer()
        {
            _renderTimer = new DispatcherTimer(DispatcherPriority.Render);
            _renderTimer.Interval = TimeSpan.FromMilliseconds(33); // 30 Hz refresh
            _renderTimer.Tick += RenderTimer_Tick;
            _renderTimer.Start();
        }

        public void OnPacketReceived(int pedalIdx, DAP_state_extended_st packet)
        {
            if (pedalIdx != _selectedPedal) return;
            if (_isPaused) return;

            DateTime now = DateTime.UtcNow;
            _lastPacketTime = now;
            _packetCounter++;

            // Throttle queue insertion to 100 Hz max (every 10ms) to avoid queue explosion while retaining full responsiveness
            if ((now - _lastSampleTime).TotalMilliseconds >= 9.0 || _points.Count == 0)
            {
                _lastSampleTime = now;
                lock (_dataLock)
                {
                    _points.Add(new TelemetryPoint
                    {
                        Timestamp = now,
                        State = packet.payloadPedalExtendedState_
                    });

                    if (_points.Count > MAX_BUFFER_POINTS)
                    {
                        _points.RemoveRange(0, _points.Count - MAX_BUFFER_POINTS);
                    }
                }
            }
        }

        private void RenderTimer_Tick(object sender, EventArgs e)
        {
            DateTime now = DateTime.UtcNow;
            _lastRenderNow = now;

            // Update Stream Status & Rate
            TimeSpan rateDiff = now - _lastRateCheck;
            if (rateDiff.TotalSeconds >= 1.0)
            {
                _packetRate = _packetCounter / rateDiff.TotalSeconds;
                _packetCounter = 0;
                _lastRateCheck = now;
            }

            if ((now - _lastPacketTime).TotalSeconds < 1.5 && _packetRate > 0)
            {
                ellipse_stream_indicator.Fill = new SolidColorBrush(Color.FromRgb(0x00, 0xE6, 0x76));
                tb_stream_status.Text = $"● {_packetRate:F0} Hz";
                tb_stream_status.Foreground = new SolidColorBrush(Color.FromRgb(0x00, 0xE6, 0x76));
            }
            else
            {
                ellipse_stream_indicator.Fill = new SolidColorBrush(Color.FromRgb(0x75, 0x75, 0x75));
                tb_stream_status.Text = "○ Idle";
                tb_stream_status.Foreground = new SolidColorBrush(Color.FromRgb(0x9E, 0x9E, 0x9E));
            }

            if (_isPaused) return;

            // Prune points outside window
            DateTime cutoff = now.AddSeconds(-_windowSeconds);
            List<TelemetryPoint> visiblePoints;
            lock (_dataLock)
            {
                while (_points.Count > 1 && _points[1].Timestamp < cutoff)
                {
                    _points.RemoveAt(0);
                }
                visiblePoints = new List<TelemetryPoint>(_points);
            }

            _cachedVisiblePoints = visiblePoints;

            double width = canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 714;
            double height = canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578;

            DrawGridlines(width, height);

            if (visiblePoints.Count == 0 || _activeSignalIds.Count == 0) return;

            // Render each active polyline with high performance (pixel-decimated points)
            foreach (var sigId in _activeSignalIds)
            {
                if (!_signalPolylines.TryGetValue(sigId, out var poly)) continue;
                var sig = AllSignals.FirstOrDefault(s => s.Id == sigId);
                if (sig == null) continue;

                double minVal = double.MaxValue;
                double maxVal = double.MinValue;
                double lastVal = 0.0;

                for (int i = 0; i < visiblePoints.Count; i++)
                {
                    double v = sig.Getter(visiblePoints[i].State);
                    if (v < minVal) minVal = v;
                    if (v > maxVal) maxVal = v;
                    lastVal = v;
                }

                if (minVal == double.MaxValue) { minVal = 0; maxVal = 1; }
                if (Math.Abs(maxVal - minVal) < 1e-6)
                {
                    maxVal += 1.0;
                    minVal -= 1.0;
                }

                _cachedSignalRanges[sigId] = (minVal, maxVal);

                // Pixel-decimated point generation to guarantee 0 render lag
                PointCollection points = new PointCollection();
                double lastX = -999.0;

                for (int i = 0; i < visiblePoints.Count; i++)
                {
                    var pt = visiblePoints[i];
                    double age = (now - pt.Timestamp).TotalSeconds;
                    if (age < 0) age = 0;
                    double x = width * (1.0 - (age / _windowSeconds));
                    x = Math.Max(0.0, Math.Min(width, x));

                    // Only add point if it is at least 1.5px apart or is the last point
                    if (i == visiblePoints.Count - 1 || Math.Abs(x - lastX) >= 1.5)
                    {
                        lastX = x;
                        double v = sig.Getter(pt.State);
                        double normY = _autoScale ? ((v - minVal) / (maxVal - minVal)) : Math.Max(0.0, Math.Min(1.0, v / 100.0));
                        double y = (height - 12.0) - (normY * (height - 24.0));
                        points.Add(new Point(x, y));
                    }
                }

                poly.Points = points;

                // Update Legend Values
                UpdateLegendDisplay(sigId, lastVal, minVal, maxVal, sig);
            }
        }

        private void UpdateLegendDisplay(string sigId, double current, double min, double max, SignalDef sig)
        {
            foreach (var child in panel_legends.Children)
            {
                if (child is Border b && (string)b.Tag == sigId && b.Child is StackPanel sp)
                {
                    foreach (var elem in sp.Children)
                    {
                        if (elem is TextBlock tb)
                        {
                            if (tb.Name == "val_" + sigId)
                            {
                                tb.Text = $"{current.ToString(sig.Format)} {sig.Unit}";
                            }
                            else if (tb.Name == "range_" + sigId)
                            {
                                tb.Text = $"[{min.ToString(sig.Format)} / {max.ToString(sig.Format)}]";
                            }
                        }
                    }
                    break;
                }
            }
        }

        private void DrawGridlines(double width, double height)
        {
            canvas_grid.Children.Clear();

            // Horizontal gridlines (25%, 50%, 75%)
            for (int i = 1; i <= 3; i++)
            {
                double y = (height / 4.0) * i;
                var line = new Line
                {
                    X1 = 0,
                    Y1 = y,
                    X2 = width,
                    Y2 = y,
                    Stroke = new SolidColorBrush(Color.FromRgb(0x28, 0x28, 0x28)),
                    StrokeThickness = 1,
                    StrokeDashArray = new DoubleCollection { 4, 4 }
                };
                canvas_grid.Children.Add(line);
            }

            // Vertical time gridlines
            int divisions = 5;
            for (int i = 1; i < divisions; i++)
            {
                double x = (width / divisions) * i;
                var line = new Line
                {
                    X1 = x,
                    Y1 = 0,
                    X2 = x,
                    Y2 = height,
                    Stroke = new SolidColorBrush(Color.FromRgb(0x22, 0x22, 0x22)),
                    StrokeThickness = 1
                };
                canvas_grid.Children.Add(line);

                double sec = _windowSeconds * (1.0 - ((double)i / divisions));
                var tb = new TextBlock
                {
                    Text = $"-{sec:F0}s",
                    Foreground = new SolidColorBrush(Color.FromRgb(0x55, 0x55, 0x55)),
                    FontSize = 9
                };
                Canvas.SetLeft(tb, x + 3);
                Canvas.SetBottom(tb, 4);
                canvas_grid.Children.Add(tb);
            }
        }

        // ==================== MOUSE HOVER DATA TIP & CROSSHAIR ====================

        private void Canvas_plot_MouseMove(object sender, MouseEventArgs e)
        {
            if (_cachedVisiblePoints == null || _cachedVisiblePoints.Count == 0 || _activeSignalIds.Count == 0)
            {
                HideDataTip();
                return;
            }

            Point pos = e.GetPosition(canvas_plot);
            double width = canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 714;
            double height = canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578;

            if (pos.X < 0 || pos.X > width || pos.Y < 0 || pos.Y > height)
            {
                HideDataTip();
                return;
            }

            // Calculate target age from mouse X
            double targetAgeSec = (1.0 - pos.X / width) * _windowSeconds;
            DateTime targetTime = _lastRenderNow.AddSeconds(-targetAgeSec);

            // Find closest sample point
            TelemetryPoint closest = null;
            double minDiff = double.MaxValue;
            for (int i = 0; i < _cachedVisiblePoints.Count; i++)
            {
                double diff = Math.Abs((_cachedVisiblePoints[i].Timestamp - targetTime).TotalSeconds);
                if (diff < minDiff)
                {
                    minDiff = diff;
                    closest = _cachedVisiblePoints[i];
                }
            }

            if (closest == null)
            {
                HideDataTip();
                return;
            }

            // Show Crosshair Line
            line_crosshair.X1 = pos.X;
            line_crosshair.X2 = pos.X;
            line_crosshair.Y1 = 0;
            line_crosshair.Y2 = height;
            line_crosshair.Visibility = Visibility.Visible;

            // Populate Data Tip Card
            double actualAge = (_lastRenderNow - closest.Timestamp).TotalSeconds;
            tb_datatip_time.Text = $"Time: -{actualAge:F2}s";

            sp_datatip_items.Children.Clear();
            foreach (var sigId in _activeSignalIds)
            {
                var sig = AllSignals.FirstOrDefault(s => s.Id == sigId);
                if (sig == null) continue;

                double val = sig.Getter(closest.State);

                var row = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 1, 0, 1) };
                var dot = new Border
                {
                    Width = 7,
                    Height = 7,
                    CornerRadius = new CornerRadius(3.5),
                    Background = new SolidColorBrush(sig.DefaultColor),
                    Margin = new Thickness(0, 0, 5, 0),
                    VerticalAlignment = VerticalAlignment.Center
                };
                var tbItem = new TextBlock
                {
                    Text = $"{sig.Name}: ",
                    FontSize = 10,
                    Foreground = new SolidColorBrush(Color.FromRgb(0xCC, 0xCC, 0xCC))
                };
                var tbVal = new TextBlock
                {
                    Text = $"{val.ToString(sig.Format)} {sig.Unit}",
                    FontSize = 10,
                    FontWeight = FontWeights.Bold,
                    Foreground = new SolidColorBrush(sig.DefaultColor)
                };

                row.Children.Add(dot);
                row.Children.Add(tbItem);
                row.Children.Add(tbVal);
                sp_datatip_items.Children.Add(row);
            }

            // Position Data Tip Card smoothly avoiding edges
            card_datatip.Visibility = Visibility.Visible;
            double tipX = pos.X + 14;
            if (tipX + 160 > width)
            {
                tipX = pos.X - 170;
            }
            double tipY = Math.Max(10, Math.Min(pos.Y - 20, height - 140));

            card_datatip.Margin = new Thickness(tipX, tipY, 0, 0);
        }

        private void Canvas_plot_MouseLeave(object sender, MouseEventArgs e)
        {
            HideDataTip();
        }

        private void HideDataTip()
        {
            if (line_crosshair != null) line_crosshair.Visibility = Visibility.Collapsed;
            if (card_datatip != null) card_datatip.Visibility = Visibility.Collapsed;
        }

        // ==================== PEDAL CONTROLS ====================

        private void BtnPedal_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && int.TryParse(btn.Tag?.ToString(), out int pIdx))
            {
                if (_selectedPedal != pIdx)
                {
                    EnsureStreaming(_selectedPedal, false);
                    _selectedPedal = pIdx;
                    EnsureStreaming(_selectedPedal, true);

                    lock (_dataLock)
                    {
                        _points.Clear();
                    }
                    UpdatePedalButtonStyles();
                }
            }
        }

        private void UpdatePedalButtonStyles()
        {
            SetPedalButtonStyle(btn_pedal_clutch, _selectedPedal == 0, Color.FromRgb(0xE5, 0x39, 0x35));
            SetPedalButtonStyle(btn_pedal_brake, _selectedPedal == 1, Color.FromRgb(0x43, 0xA0, 0x47));
            SetPedalButtonStyle(btn_pedal_throttle, _selectedPedal == 2, Color.FromRgb(0x1E, 0x88, 0xE5));
        }

        private void SetPedalButtonStyle(Button btn, bool isSelected, Color activeColor)
        {
            if (btn == null) return;
            if (isSelected)
            {
                btn.Background = new SolidColorBrush(Color.FromArgb(0x40, activeColor.R, activeColor.G, activeColor.B));
                btn.BorderBrush = new SolidColorBrush(activeColor);
                btn.Foreground = Brushes.White;
                btn.FontWeight = FontWeights.Bold;
            }
            else
            {
                btn.Background = new SolidColorBrush(Color.FromRgb(0x2A, 0x2A, 0x2A));
                btn.BorderBrush = new SolidColorBrush(Color.FromRgb(0x44, 0x44, 0x44));
                btn.Foreground = new SolidColorBrush(Color.FromRgb(0xBB, 0xBB, 0xBB));
                btn.FontWeight = FontWeights.Normal;
            }
        }

        private void BtnLivePause_Click(object sender, RoutedEventArgs e)
        {
            _isPaused = !_isPaused;
            if (_isPaused)
            {
                btn_live_pause.Content = "▶ Live";
                btn_live_pause.Foreground = new SolidColorBrush(Color.FromRgb(0xFF, 0xD7, 0x00));
            }
            else
            {
                btn_live_pause.Content = "⏸ Pause";
                btn_live_pause.Foreground = Brushes.White;
            }
        }

        private void BtnClear_Click(object sender, RoutedEventArgs e)
        {
            lock (_dataLock)
            {
                _points.Clear();
            }
            foreach (var poly in _signalPolylines.Values)
            {
                poly.Points.Clear();
            }
            HideDataTip();
        }

        private void CbTimeWindow_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cb_time_window.SelectedItem is string item)
            {
                string s = item.Replace("s", "");
                if (double.TryParse(s, out double sec))
                {
                    _windowSeconds = sec;
                }
            }
        }

        private void ChkAutoScale_Changed(object sender, RoutedEventArgs e)
        {
            _autoScale = chk_auto_scale.IsChecked == true;
        }

        private void Canvas_plot_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            canvas_grid.Width = canvas_plot.ActualWidth;
            canvas_grid.Height = canvas_plot.ActualHeight;
        }
    }
}