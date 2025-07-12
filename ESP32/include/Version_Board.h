const char *DAP_FIRMWARE_VERSION = "0.90.09";

#if PCB_VERSION==3
	#define CONTROL_BOARD "V3_ESP32"
#endif

#if PCB_VERSION==5
	#define CONTROL_BOARD "Speedcrafter"
#endif

#if PCB_VERSION==6
	#define CONTROL_BOARD "V4_ESP32S3"
#endif

#if PCB_VERSION==7
	#define CONTROL_BOARD "Gilphilbert_PCBAv1"
#endif

#if PCB_VERSION==8
	#ifdef ENABLE_ESP_NOW
		#define CONTROL_BOARD "Gilphilbert_PCBAv2"
	#else
		#define CONTROL_BOARD "Gilphilbert_PCBAv2_Without_Wireless"
	#endif
#endif
#if PCB_VERSION==9
	#define CONTROL_BOARD "Gilphilbert_PCBAv2"
#endif
#if PCB_VERSION==10
	#define CONTROL_BOARD "V4_ESP32S3"
#endif
#if PCB_VERSION==11
	#define CONTROL_BOARD "Switch-!t_ESP32S3"
#endif
#if PCB_VERSION==12
	#ifdef ENABLE_ESP_NOW
		#define CONTROL_BOARD "V5_ESP32S3"
	#else
		#define CONTROL_BOARD "V5_ESP32S3_Without_Wireless"
	#endif
#endif
#if PCB_VERSION==13
	#ifdef ENABLE_ESP_NOW
		#define CONTROL_BOARD "V6_ESP32S3"
	#else
		#define CONTROL_BOARD "V6_ESP32S3_Without_Wireless"
	#endif
#endif

void parse_version(const char *version, uint8_t *major, uint8_t *minor, uint8_t *patch) 
{
    int imajor, iminor, ipatch;
    sscanf(version, "%d.%d.%d", &imajor, &iminor, &ipatch);
    *major = (uint8_t)imajor;
    *minor = (uint8_t)iminor;
    *patch = (uint8_t)ipatch;
}