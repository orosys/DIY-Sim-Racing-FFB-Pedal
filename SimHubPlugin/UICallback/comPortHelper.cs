using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Management;


namespace User.PluginSdkDemo
{
    public class VidPidResult
    {
        public bool Found { get; set; }
        public string Vid { get; set; }
        public string Pid { get; set; }
        public string DeviceName { get; set; }
        public string ComPortName { get; set; }
    }
    public static class ComPortHelper
    {

        public static VidPidResult GetVidPidFromComPort(string targetPort)
        {
            var result = new VidPidResult { Found = false };
            result.Pid = "na";
            result.Vid = "na";
            result.DeviceName = "na";
            result.ComPortName = targetPort;
            var searcher = new ManagementObjectSearcher("SELECT * FROM Win32_PnPEntity WHERE Name LIKE '%(COM%'");

            foreach (ManagementObject device in searcher.Get())
            {
                string name = device["Name"] != null ? device["Name"].ToString() : "";
                string deviceID = device["DeviceID"] != null ? device["DeviceID"].ToString() : "";

                // check if the port in the list
                if (!name.Contains("(" + targetPort.ToUpper() + ")"))
                    continue;

                // Match VID/PID
                Match vidMatch = Regex.Match(deviceID, "VID_([0-9A-Fa-f]{4})");
                Match pidMatch = Regex.Match(deviceID, "PID_([0-9A-Fa-f]{4})");

                if (vidMatch.Success && pidMatch.Success)
                {
                    result.Found = true;
                    result.Vid = vidMatch.Groups[1].Value.ToUpper();
                    result.Pid = pidMatch.Groups[1].Value.ToUpper();
                    result.DeviceName = name;
                    //result.ComPortName = targetPort;
                    return result;
                }
            }

            return result;
        }
    }
}
