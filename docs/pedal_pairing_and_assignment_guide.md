# Wireless Pedal Pairing & Assignment Guide

This guide explains how to pair, assign, reassign, and manage your DIY Sim Racing Active Pedals using the SimHub Plugin and the ESP-NOW wireless USB Bridge.

---

## 1. Overview & Architecture

The wireless communication between your pedals and the PC uses Espressif's high-speed **ESP-NOW** protocol operating at 2.4 GHz. 

Key features of the system:
- **Factory Hardware MAC Addresses**: Each pedal and Bridge uses its unique, factory-burned eFuse MAC address. There are zero hardcoded fake MAC addresses. Multiple simulator rigs in the same room operate completely isolated with zero cross-talk or interference.
- **Automatic Discovery**: New or unassigned pedals are detected automatically over the air. SimHub notifies you with a popup dialog the moment a pedal is found.
- **Peer-to-Peer Rudder Sync**: In flight simulation modes (Airplane/Helicopter Rudder), the Throttle and Brake pedals communicate directly peer-to-peer with zero host latency.
- **Smart Wi-Fi Channel Hunting (1–13)**: Pedals automatically scan all 2.4 GHz Wi-Fi channels (1 to 13) to acquire the Bridge if the channel was changed while a pedal was powered off.

---

## 2. Initial Setup: Pairing New Pedals

When a pedal is powered on for the first time (or after its assignment has been cleared), it operates in **Unassigned Mode** and broadcasts discovery beacons.

![SimHub Unassigned Pedal Detected Notification](media/images/simhub_pedal_detected_toast.png)  
*Figure 1: Toast notification in SimHub when an unassigned pedal is detected.*

### Step-by-Step Pairing Workflow:

1. **Power on your Bridge**: Plug the ESP32-S3 Bridge module into your PC via USB. Ensure the SimHub DIY FFB Pedal plugin is active.
2. **Power on the Pedal**: Connect power to the pedal controller board.
3. **Automatic Popup**: Within 1–3 seconds, SimHub automatically displays the **Pedal Assignment** window:

![SimHub Pedal Assignment Window](media/images/simhub_pedal_assignment_window.png)  
*Figure 2: The Assignment Configuration Window with detected pedals and role selection.*

4. **Identify the Physical Pedal**:
   - Select the detected pedal from the dropdown list (e.g., `#1 - 48:27:E2:59:48:C0`).
   - Click the **"Beep"** button. The corresponding pedal will emit a short acoustic tone from its onboard buzzer.
   - Alternatively, use **"Vibration On / Off"** to test the motor haptic vibration.
5. **Assign the Role**:
   - In SimHub, select the tab corresponding to the role you want to configure (**Throttle**, **Brake**, or **Clutch**).
   - Click the **"Assign"** button.
6. **Confirmation**:
   - The pedal sounds a confirmation beep.
   - The pedal stores the Bridge's hardware MAC address and its assigned role in onboard EEPROM.
   - The Bridge saves the pedal's hardware MAC address to its internal pairing table.
   - The pedal status updates to **Connected / Ready** in SimHub.

> [!TIP]
> If you are setting up a 3-pedal set (Clutch, Brake, Throttle), repeat this process for each pedal. When an assignment is completed, the window automatically updates for the next unassigned pedal.

---

## 3. Reassigning or Swapping Pedal Roles

If you want to change a pedal's role (for example, swapping your Brake and Throttle pedals, or moving a pedal from Clutch to Throttle), follow either of the methods below:

### Method A: Wireless Reassignment via SimHub (Recommended)

1. Open SimHub and navigate to the **DIY FFB Pedal** plugin.
2. Select the pedal tab you wish to unassign (e.g., **Brake**).
3. Open the **System Setting / Wireless** section and click **"Assignment"** (or the assignment icon).
4. Since the pedal is currently connected, the dialog displays:
   > *"[Role] is connected, click Clear to remove Assignment"*
5. Click **"Clear Assignment"**.

![SimHub Clear Assignment Dialog](media/images/simhub_clear_assignment.png)  
*Figure 3: Clearing an existing pedal assignment.*

6. The pedal sounds an acoustic tone, erases its role from EEPROM, and restarts into **Unassigned Mode**.
7. Within a few seconds, SimHub detects the unassigned pedal and re-opens the **Pedal Assignment** dialog.
8. Choose your desired new role and click **"Assign"**.

---

### Method B: Direct USB Connection (Fallback)

If wireless communication is unavailable:
1. Connect a USB-C cable directly from your PC to the pedal controller board.
2. Open SimHub. The pedal connects directly as a wired USB device.
3. In the pedal configuration tab, click **"Clear Assignment"** (or send the reset command).
4. Disconnect the USB cable. The pedal will power-cycle into Unassigned Mode, ready to pair wirelessly with the Bridge.

---

## 4. Replacing or Swapping the USB Bridge Module

If you replace your ESP32-S3 Bridge module with a new board (or flash a different Bridge device with a new hardware MAC address), the pedals will automatically detect the change:

1. **Loss Detection**: When the original Bridge is disconnected, the pedals notice the absence of heartbeats. After **5 seconds**, the pedals automatically activate **Auto-Discovery Broadcast Mode**.
2. **New Bridge Detection**: As soon as the new Bridge is plugged into your PC, it catches the discovery packets and reports the pedals to SimHub.
3. **Pairing**: The SimHub assignment popup appears for each pedal. Simply click **"Assign"** to pair the pedal with your new Bridge.
4. All previously tuned pedal curves, force settings, and travel limits remain intact on each pedal.

---

## 5. Wi-Fi Channel Configuration & Automatic Hunting (Channels 1–13)

By default, the wireless system operates on **Wi-Fi Channel 11**. In congested 2.4 GHz environments, switching to another channel (such as Channel 1, 6, or any channel from 1 to 13) can improve signal stability and eliminate packet jitter.

![SimHub Wi-Fi Channel Optimizer](media/images/wifi_channel_optimizer_ui.png)  
*Figure 4: Wi-Fi Channel Optimizer and selection in the SimHub plugin.*

### Changing the Wi-Fi Channel:
1. Navigate to **System Settings -> Wireless / Wi-Fi Channel**.
2. Select your desired target channel (Channels 1 through 13).
3. Click **"Set Channel"**.
4. The Bridge broadcasts the channel migration command across all paired pedals.
5. Each pedal switches its radio to the new channel and schedules a deferred EEPROM write (saved safely when the motor is idle to prevent motion stutter).

### Automatic Channel Hunting:
What happens if a pedal was powered off when the channel was changed?
- When the pedal is powered back on, it initially boots on its old channel.
- After 3.5 seconds of silence from the Bridge, the pedal automatically activates **Channel Hunting**.
- It systematically scans through all channels: **1, 6, 11, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13**.
- At 400 ms beacon intervals from the Bridge, the pedal detects the Bridge on the new channel within ~3 seconds, locks on, and updates its EEPROM.

---

## 6. Rudder Modes & Peer-to-Peer Communication

In flight simulation mode, the DIY FFB Pedals support direct synchronization:
- **Airplane Rudder**: Throttle and Brake act as coupled left/right rudder pedals (one pushes forward as the other moves back).
- **Helicopter Rudder**: Center-sprung or friction anti-torque pedals.
- **Airplane with Toe Brakes**: Differential braking on upper pedal angles.

![SimHub Rudder Mode Configuration](media/images/plugin_rudder_mode_airplane_0.png)  
*Figure 5: Rudder Mode configuration in SimHub.*

### How Peer-to-Peer Pairing Works:
1. When any Rudder mode is activated in SimHub, the Bridge automatically syncs the complete hardware MAC table of all rig pedals to each pedal.
2. The Throttle and Brake pedals register each other as direct ESP-NOW peers.
3. Position and force data are exchanged directly between the pedals via unicast at up to **280 Hz with < 3 ms latency**, bypassing PC USB polling entirely.
4. Because real hardware MACs are used, adjacent flight sim rigs in the same room will never cross-connect.

---

## 7. Troubleshooting & Status Indicators

### Checking Signal Quality (RSSI):
In the SimHub plugin status bar, verify the real-time RSSI signal strength:
- **-30 to -55 dBm**: Excellent signal.
- **-56 to -66 dBm**: Good, stable connection.
- **< -75 dBm**: Weak signal. Consider repositioning the Bridge or pedals to ensure direct line-of-sight (see the [ESP-NOW Placement Guide](espnow_pcb_and_bridge_placement_guide.md)).

### LED Status Codes:
| LED Color / Blink | Meaning | Action |
| :--- | :--- | :--- |
| **Blinking Yellow/White** | Unassigned mode / Waiting for pairing | Check SimHub popup and assign role. |
| **Solid Blue / Green** | Connected and active | Ready to race. |
| **Blinking Red** | Lost connection to Bridge | Check Bridge power and USB cable. |
| **Double Purple Blink** | Calibration / Homing in progress | Keep feet off pedals until homed. |

### Common Issues & Solutions:
- **Pedal does not show up in SimHub popup**: Ensure power supply is connected and board LED is illuminated. If previously paired to an old bridge, wait 5 seconds after powering on for auto-discovery broadcast to trigger.
- **Popup does not appear automatically**: In SimHub, navigate to **Settings -> Wireless** and click **"Scan Unassigned Pedals"** manually.
- **Channel mismatch**: Ensure no external 2.4 GHz antennas are obstructed by heavy aluminum 8020 extrusion beams.
