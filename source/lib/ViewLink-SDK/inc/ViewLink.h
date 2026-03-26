/**
 * @mainpage ViewLink SDK
 * @details ViewLink SDK is developed for those who want to integrate Viewpro Gimbal to their own
 *			sofware system or to control Gimbal via some special equipments. With this SDK, developers
 *			can easily complete their secondary development without regard to the complicated control protocols. \n
 *          This SDK is suitable for use with F, T, TL, TIR, TIRM, TM, Z-fusionS, Z6KA7, XIR, ZIR, QIR19, Z36N, Z-fusion, Q40TIR \n
 *          We provide multiple binary libraries for different OS and uniform interfaces. ViewLink SDK is currently
 *			supported on the following OS:\n
 *
 *			windows(32bit, 64bit)\n
 *
 *			linux(x86_32, x86_64, arm_32, arm_64) \n
 *
 *			android(32bit, 64bit)\n
 *  @author DreamSky
 *  @version 3.5.3
 *  @date    2025-03-27
 *  @copyright Copyright (c) 2020 Shenzhen Viewpro Technology Co., Ltd
 */

/**
 * @file ViewLink.h
 * @brief ViewLink SDK header file
 * @details This header file defines ViewLink SDK interfaces and the necessary data structures
 * @details You need to create a VLKLog folder in the root directory of the program, please refer to the demo
 * @author DreamSky
 * @version 3.5.3
 * @date    2025-03-27
 * @copyright Copyright (c) 2020 Shenzhen Viewpro Technology Co., Ltd
 */

#ifndef __VIEW_LINK_H__
#define __VIEW_LINK_H__

#if (defined _WIN32) || (defined _WIN64) // windows
	#ifdef VLK_EXPORTS
		#define VLK_API extern "C" __declspec(dllexport)
	#else
		#define VLK_API extern "C" __declspec(dllimport)
	#endif
	#define VLK_CALL __stdcall
#else
    #define VLK_API
	#define VLK_CALL
#endif

/** @name error code
 *  @brief usually used as return value of interface function to indicate the operation success or not
 * @{
 */
#define VLK_ERROR_NO_ERROR 0
#define VLK_ERROR_INVALID_PARAM -1
/** @} error code */

/** @name ViewLink SDK constant
 * @{
 */
/** the max value of yaw */
#define VLK_YAW_MAX 180.0
/** the min value of yaw */
#define VLK_YAW_MIN -180.0
/** the max value of pitch */
#define VLK_PITCH_MAX 90.0
/** the min value of pitch */
#define VLK_PITCH_MIN -90.0

/** the yaw max moving speed (20 degrees per second)*/
#define VLK_MAX_YAW_SPEED 2000.0
/** the pitch max moving speed (20 degrees per second)*/
#define VLK_MAX_PITCH_SPEED 2000.0

/** the min zoom (or focus) speed*/
#define VLK_MIN_ZOOM_SPEED 1
/** the max zoom (or focus) speed*/
#define VLK_MAX_ZOOM_SPEED 8
/** @} ViewLink SDK constant */

/** @name ViewLink SDK data structures
 * @{
 */

/**
 * device connection type
 */
typedef enum _VLK_CONN_TYPE
{
    VLK_CONN_TYPE_SERIAL_PORT = 0x00, ///< serial port
    VLK_CONN_TYPE_TCP = 0x01, ///< tcp
    VLK_CONN_TYPE_UDP = 0x02, ///< udp
	VLK_CONN_TYPE_BUTT
}VLK_CONN_TYPE;

/**
 * TCP connect information
 */
typedef struct _VLK_DEV_IPADDR VLK_DEV_IPADDR;
struct _VLK_DEV_IPADDR
{
    char szIPV4[16]; ///< device ipv4 address, e.g: 192.168.2.119
    int iPort; ///< tcp port, e.g: 2000
};

/**
 * serial port connect information
 */
typedef struct _VLK_DEV_SERIAL_PORT VLK_DEV_SERIAL_PORT;
struct _VLK_DEV_SERIAL_PORT
{
    char szSerialPortName[16]; ///< serial port name, e.g: /dev/ttyS0 on linux or COM1 on windows
    int iBaudRate; ///< baudrate, e.g: 115200
};

/**
 * device connect parameter
 */
typedef struct _VLK_CONN_PARAM VLK_CONN_PARAM;
struct _VLK_CONN_PARAM
{
	VLK_CONN_TYPE emType;
	union {
		VLK_DEV_IPADDR IPAddr;
		VLK_DEV_SERIAL_PORT SerialPort;
	} ConnParam;
};

/**
 * device connection status
 */
typedef enum _VLK_CONN_STATUS
{
    VLK_CONN_STATUS_CONNECTED = 0x00, ///< deprecated
    VLK_CONN_STATUS_DISCONNECTED = 0x01, ///< deprecated
    VLK_CONN_STATUS_TCP_CONNECTED = 0x02, ///< TCP connected
    VLK_CONN_STATUS_TCP_DISCONNECTED = 0x03, ///< TCP disconnected
    VLK_CONN_STATUS_SERIAL_PORT_CONNECTED = 0x04, ///< serial port connected
    VLK_CONN_STATUS_SERIAL_PORT_DISCONNECTED = 0x05, ///< serial port disconnectd
	VLK_CONN_STATUS_BUTT
}VLK_CONN_STATUS;

/**
 * track template size
 */
typedef enum _VLK_TRACK_TEMPLATE_SIZE
{
	VLK_TRACK_TEMPLATE_SIZE_AUTO = 0,
	VLK_TRACK_TEMPLATE_SIZE_32 = 32,
	VLK_TRACK_TEMPLATE_SIZE_64 = 64,
	VLK_TRACK_TEMPLATE_SIZE_128 = 128,
	VLK_TRACK_TEMPLATE_SIZE_BUTT
}VLK_TRACK_TEMPLATE_SIZE;

/**
 * sensor type  
 */
typedef enum _VLK_SENSOR
{
    VLK_SENSOR_VISIBLE1 = 0, ///< visible light only
    VLK_SENSOR_IR = 1, ///< IR only
    VLK_SENSOR_VISIBLE_IR = 2, ///< visible light + IR PIP
    VLK_SENSOR_IR_VISIBLE = 3, ///< IR + visible light PIP
    VLK_SENSOR_VISIBLE2 = 4, ///< prime lens visible light
	VLK_SENSOR_BUTT
}VLK_SENSOR;

/**
 * track mode parameter
 */
typedef struct _VLK_TRACK_MODE_PARAM VLK_TRACK_MODE_PARAM;
struct _VLK_TRACK_MODE_PARAM
{
	VLK_TRACK_TEMPLATE_SIZE emTrackTempSize;
	VLK_SENSOR emTrackSensor;
};

/**
 * Record mode
 */
typedef enum _VLK_RECORD_MODE
{
    VLK_RECORD_MODE_NONE = 0, ///< none mode, neither photo nor record mode
    VLK_RECORD_MODE_PHOTO = 1, ///< photo mode, take photos only
    VLK_RECORD_MODE_RECORD = 2, ///< record mode, video record only
	VLK_RECORD_MODE_BUTT
}VLK_RECORD_MODE;

/**
 * Focus mode
 */
typedef enum _VLK_FOCUS_MODE
{
    VLK_FOCUS_MODE_AUTO = 0, ///< automatic focus
    VLK_FOCUS_MODE_MANU = 1, ///< manual focus
	VLK_FOCUS_MODE_BUTT
}VLK_FOCUS_MODE;

/**
 * Laser zoom mode
 */
typedef enum _VLK_LASER_ZOOM_MODE
{
    VLK_LASER_ZOOM_MODE_FOLLOW_EO = 0, ///< follow visible light zoom
    VLK_LASER_ZOOM_MODE_MANU = 1, ///< manual control
	VLK_LASER_ZOOM_MODE_BUTT
}VLK_LASER_ZOOM_MODE;

/**
 * OSD mask
 */
typedef enum _VLK_OSD_MASK
{
    VLK_OSD_MASK_ENABLE_OSD = 0x1, ///< enable OSD
    VLK_OSD_MASK_CROSS = 0x2, ///< enable cross frame in center of image
    VLK_OSD_MASK_PITCH_YAW = 0x4, ///< show pitch and yaw
    VLK_OSD_MASK_XYSHIFT = 0x8, ///< enable xy shift
    VLK_OSD_MASK_GPS = 0x10, ///< enable GPS
    VLK_OSD_MASK_TIME = 0x20, ///< enable time
    VLK_OSD_MASK_VL_MAG = 0x40, ///< enable VL-MAG
    VLK_OSD_MASK_BIG_FONT = 0x80 ///< enable big font
}VLK_OSD_MASK;

/**
 * OSD input mask
 */
typedef enum _VLK_OSD_INPUT_MASK
{
    VLK_OSD_INPUT_MASK_PERMANENT_SAVE = 0x1, ///< save OSD configuration, still work after reboot
    VLK_OSD_INPUT_MASK_TIME = 0x2, ///< enalbe input time
    VLK_OSD_INPUT_MASK_GPS = 0x4, ///< enable input GPS
    VLK_OSD_INPUT_MASK_MGRS = 0x8,///< enable input MGRS
    VLK_OSD_INPUT_MASK_PITCH_YAW = 0x10, ///< enable input pitch and yaw
    VLK_OSD_INPUT_MASK_VL_MAG = 0x20, ///< enalbe input VL-MAG
    VLK_OSD_INPUT_MASK_ZOOM_TIMES_OR_FOV = 0x40, ///< display zoom times or FOV
    VLK_OSD_INPUT_MASK_CHAR_BLACK_BORDER = 0x80 ///< enable character black frame
}VLK_OSD_INPUT_MASK;

/**
 * OSD configuration parameter
 */
typedef struct _VLK_OSD_PARAM VLK_OSD_PARAM;
struct _VLK_OSD_PARAM
{
    char cOSD; ///< mask of VLK_OSD_MASK, e.g: VLK_OSD_MASK_ENABLE_OSD | VLK_OSD_MASK_CROSS
    char cOSDInput; ///< mask of VLK_OSD_INPUT_MASK, e.g: VLK_OSD_INPUT_MASK_TIME | VLK_OSD_INPUT_MASK_GPS
};

/**
 * Image type
 */
typedef enum _VLK_IMAGE_TYPE
{
    VLK_IMAGE_TYPE_VISIBLE1, ///< visible light zoom lens
    VLK_IMAGE_TYPE_VISIBLE2, ///< visible light prime lens
    VLK_IMAGE_TYPE_IR1, ///< IR zoom lens
    VLK_IMAGE_TYPE_IR2, ///< IR prime lens
    VLK_IMAGE_TYPE_FUSION, ///< visible light + IR fusion
	VLK_IMAGE_TYPE_BUTT
}VLK_IMAGE_TYPE;

/**
 * IR color type
 */
typedef enum _VLK_IR_COLOR
{
	VLK_IR_COLOR_WHITEHOT,
	VLK_IR_COLOR_BLACKHOT,
	VLK_IR_COLOR_PSEUDOHOT,
	VLK_IR_COLOR_RUSTY,
	VLK_IR_COLOR_BUTT
}VLK_IR_COLOR;

/**
 * 2.4G wireless control channels map
 */
typedef struct _VLK_CHANNELS_MAP VLK_CHANNELS_MAP;
struct _VLK_CHANNELS_MAP
{
    char cYW; ///< high 4 bits save yaw left channel index, low 4 bits save yaw right channel index
    char cPT; ///< high 4 bits save pitch up channel index, low 4 bits save pitch down channel index
    char cMO; ///< high 4 bits save adjust speed channel index, low 4 bits save recenter channel index
    char cZM; ///< high 4 bits save zoom out channel index, low 4 bits save zoom in channel index
    char cFC; ///< high 4 bits save focus out channel index, low 4 bits save focus in channel index
    char cRP; ///< high 4 bits save take photo channel index, low 4 bits save Gimbal record channel index
    char cMU; ///< high 4 bits save start track channel index, low 4 bits save stop track channel index
};

/**
 * Device configuration
 */
typedef struct _VLK_DEV_CONFIG VLK_DEV_CONFIG;
struct _VLK_DEV_CONFIG
{
    char cTimeZone; ///< time zone, e.g: 8 (Beijing), 9 (Seoul)
    char cOSDCfg; ///< OSD configuration, equivalent to cOSD in VLK_OSD_PARAM
    char cMagneticVariation; ///< Magnetic Variation set https://skyvector.com/   http://www.magnetic-declination.com/
    char cOSDInput; ///< OSD input configuration, equivalent to cOSDInput in VLK_OSD_PARAM

    /**
     * @brief Gimbal serial port baudrate \n
     * 0: 2400 \n
     * 1: 4800 \n
     * 2: 9600 \n
     * 3: 19200 \n
     * 4: 38400 \n
     * 5: 57600 \n
     * 6: 115200 \n
     * 7: S.BUS mode
     */
    char cBaudRate;

    char cEODigitalZoom; ///< EO digital zoom, 1: on; 0: off
    short sTemperatureAlarmLine;///< temperature alarm, low 8 bits save lower limit, high 8 bits save upper limit
    char cTrack; ///< track status, 1: enabled, 0: disabled

    /**
     * @brief laser work mode \n
     * 0: stop LRF \n
     * 1: 1HZ get LRF data \n
     * 2: continuosly get LRF \n
     * 3: one time get LRF data
     */
    char cLaser;

    char cRecordDefinition; ///< record definition, 1: 4k 25fps; 2: 1080P 25fps
    char cOSDGPS; ///< OSD GPS, 0: OSD GPS is UAV; 1: OSD GPS is target

    /**
     * @brief s.bus/mavlink channels map set \n
     * 1: 1~7 \n
     * 2: 6~12 \n
     * 3: 8~14 \n
     * 4: custom channels \n
     */
    char cSBUSChnlMap;

    VLK_CHANNELS_MAP ChnlsMap; ///< custom channels map
    char cFocusHoldSet; ///< deprecated
    char cCameraType; ///< deprecated
    char cReserved1[5]; ///< reserved
    char cRestoreIP; ///< deprecated
    char cReserved2[5]; ///< reserved
    char cReserved3[43]; ///< reserved
    char cVersionNO[20]; ///< firmware version NO
    char cDeviceID[10]; ///< device model
    char cSerialNO[22]; ///< serial NO
};


/**
 * Device model
 */
typedef struct _VLK_DEV_MODEL VLK_DEV_MODEL;
struct _VLK_DEV_MODEL
{
    char cModelCode; ///< Gimbal model code
    char szModelName[32]; ///< Gimbal model name
};

/**
 * Track X Y output
 */
typedef struct _VLK_DEV_TRACKXY VLK_DEV_TRACKXY;
struct _VLK_DEV_TRACKXY
{
    char iTrackX; ///< Gimbal X
    char iTrackY; ///< Gimbal Y
};


/**
 * Tracker status
 */
typedef enum _VLK_TRACKER_STATUS
{
	VLK_TRACKER_STATUS_STOPPED,
	VLK_TRACKER_STATUS_SEARCHING,
	VLK_TRACKER_STATUS_TRACKING,
	VLK_TRACKER_STATUS_LOST,
	VLK_TRACKER_STATUS_BUTT
}VLK_TRACKER_STATUS;

/**
 * AI status
 */
typedef enum _VLK_AI_STATUS
{
    VLK_AI_STATUS_DOING,
    VLK_AI_STATUS_STOP
}VLK_AI_STATUS;



/**
 * Device telemetry
 */
typedef struct _VLK_DEV_TELEMETRY VLK_DEV_TELEMETRY;
struct _VLK_DEV_TELEMETRY
{
    double dYaw; ///< Gimbal current yaw
    double dPitch; ///< Gimbal current pitch
    double dRoll; ///< Gimbal current roll
    VLK_SENSOR emSensorType; ///< sensor type
    VLK_TRACKER_STATUS emTrackerStatus; ///< tracker status
    double dTargetLat; ///< Target latitude
    double dTargetLng; ///< Target longitude
    double dTargetAlt; ///< Target altitude
	double dDroneLat; ///< Drone latitude
	double dDroneLng; ///< Drone longitude
	double dDroneAlt; ///< Drone altitude
	double dFov;///< Gimbal current horizontal FOV
	double dVertical;///< Gimbal current vertical FOV
	int iEODigitalZoom;///< Visible light electron doubling
	int iIRDigitalZoom;///< Infrared electron doubling 
    double dZoomMagTimes; ///< camera magnification times
    short sLaserDistance; ///< laser distance
    VLK_IR_COLOR emIRColor; ///< IR color
    VLK_RECORD_MODE emRecordMode; ///< record mode  
};


/**
 * Device status type \n
 * device status is passed by VLK_DevStatus_CB, \n
 * VLK_DEV_STATUS_TYPE will be passed to formal paramter iType, \n
 * and the device status data will be passed to szBuffer, \n
 * different iType matches different status data struct
 */
typedef enum _VLK_DEV_STATUS_TYPE
{
    VLK_DEV_STATUS_TYPE_MODEL, ///< matches VLK_DEV_MODEL
    VLK_DEV_STATUS_TYPE_CONFIG, ///< matches VLK_DEV_CONFIG
    VLK_DEV_STATUS_TYPE_TELEMETRY, ///< matches VLK_DEV_TELEMETRY
    VLK_DEV_STATUS_TYPE_AISTATE, ///< matches VLK_DEV_AISTATE
	VLK_DEV_STATUS_TYPE_BUTT,
    VLK_DEV_STATUS_TYPE_TRACKXY,
}VLK_DEV_STATUS_TYPE;
/** @} ViewLink SDK data structures */

/** @name ViewLink SDK callback functions
 * @{
 */
/** @brief connection status callback
 *  @param iConnStatus connection status, it must be one of VLK_CONN_STATUS
 *  @param szMessage extended string message
 *  @param iMsgLen extended string message length
 *  @return return 0 anyway
 *  @details VLK_ConnStatus_CB will be called by SDK once the connection status changed,
 *           for example, developers should check connect result in this callback function
 * @see VLK_Connect
 */
typedef int (*VLK_ConnStatus_CB)(int iConnStatus, const char* szMessage, int iMsgLen, void* pUserParam);

/** @brief device status callback
 *  @param iType device status type, it must be one of VLK_DEV_STATUS_TYPE
 *  @param szBuffer device status data, it is an address of a struct matchs iType, e.g: VLK_DEV_MODEL
 *  @param iBufLen device status data length, it must be equal to the size of struct matchs iType, e.g: iBufLen == sizeof(VLK_DEV_MODEL)
 *  @return return 0 anyway
 *  @details VLK_DevStatus_CB will be called by SDK once device new status is received, developers can easily get Gimbal latest status
 *  @see VLK_RegisterDevStatusCB
 */
typedef int (*VLK_DevStatus_CB)(int iType, const char* szBuffer, int iBufLen, void* pUserParam);
/** @} ViewLink SDK callback functions */


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get SDK version
 *  @return SDK version number, e.g: 3.5.0
 *  @details we recommend printing the SDK version number before you use SDK,
 *           this is the only interface can be called before VLK_Init()
 */
VLK_API	const char* VLK_CALL    GetSDKVersion();

/** @brief Initialize SDK
 *  @return VLK_ERROR_NO_ERROR on success, error code otherwise
 *  @details it should be called somewhere at the beginning of your application,
 *           and it should be called just once during the lifecirle of your application,
 *           all interfaces are unavailbe before initialized,
 *           you need to manually in application create a VLKLog folder
 */
VLK_API	int     VLK_CALL    VLK_Init();

/** @brief Uninitialize SDK
 *  @details it should be called somewhere at the end of your application,
 *           and it should be called just once during the lifecirle of your application,
 *           once you uninitialize sdk, all interfaces are nomore available
 */
VLK_API	void    VLK_CALL    VLK_UnInit();

/** @brief connect Gimbal with specific type, TCP and serial and UDP port are available
 *  @param pConnParam connect information, including connection type (TCP or serial port or UDP), ip address, port, serial port name, baudrate
 *  @param pUdpConnParam connect information, including connection type (UDP), UDP Local ip address, port, serial port name, baudrate
 *  @param pConnStatusCB  connnection status callback function, it will be called by SDK when device is connected or disconnected
 *  @param pUserParam user parameters, will be passed back when pConnStatusCB is called
 *  @return VLK_ERROR_NO_ERROR on success, error code otherwise 
 *  @details this function is asynchronous, the connect result will be passed back by callback function, do not use return value to judge connect result.
 *           SDK allows only one connection, it means if you create a new connection, the previous one will be disconnected automatically
 */
VLK_API	int     VLK_CALL    VLK_Connect(const VLK_CONN_PARAM* pConnParam, VLK_ConnStatus_CB pConnStatusCB, void* pUserParam);

/** @brief Check if Gimbal is connected
 *  @return
 *  @retval 1 if either TCP or serial port is connected
 *  @retval 0 if neither TCP nor serial port is connected
 */
VLK_API	int     VLK_CALL    VLK_IsConnected();


/** @brief Check if Gimbal is connected
* Turn on AI Identify people and vehicles
 */
VLK_API	void     VLK_CALL    VLK_StartAI();

/** @brief Check if Gimbal is connected
* Open AI state people and vehicles
 */
VLK_API	void     VLK_CALL    VLK_OpenAiState();

/** @brief Check if Gimbal is connected
* Close AI state people and vehicles
 */
VLK_API	void     VLK_CALL    VLK_CloseAiState();

/** @brief Check if Gimbal is connected
* stop Ai  
 */
VLK_API	void     VLK_CALL    VLK_StopAI();


/** @brief Check if Gimbal is connected The tracker outputs the amount of misses, only supports serial port
* Tracker output misses
 */
VLK_API	void     VLK_CALL    VLK_OpenTrackXY();


/** @brief Check if Gimbal is connected The tracker outputs the amount of misses, only supports serial port
* No off-target output
 */
VLK_API	void     VLK_CALL    VLK_CloseTrackXY();


/** @brief The gimbal points to the target position
* VLK_PointCameraHere
 */
VLK_API	void     VLK_CALL    VLK_PointCameraHere(double dLat, double dLng, double dAlt);


/** @brief Pointing to move
* VLK_PointToMove
 */
VLK_API	void     VLK_CALL    VLK_PointToMove(int x, int y);


/** @brief Check if Gimbal is TCP connected
 *  @return
 *  @retval 1 if TCP is connected
 *  @retval 0 if TCP is disconnected
 */
VLK_API	int     VLK_CALL    VLK_IsTCPConnected();

/** @brief Check if Gimbal is serial port connected
 *  @return
 *  @retval 1 if serial port is connected
 *  @retval 0 if serial port is disconnected
 */
VLK_API	int     VLK_CALL    VLK_IsSerialPortConnected();

/** @brief Check if Gimbal is UDP connected
 *  @return
 *  @retval 1 if UDP is connected
 *  @retval 0 if UDP is disconnected
 */
VLK_API	int     VLK_CALL    VLK_IsUDPConnected();

/** @brief Disconnect current connection no matter it is TCP, serial port or other
 */
VLK_API	void	VLK_CALL    VLK_Disconnect();

/** @brief Disconnect current TCP connection
 */
VLK_API	void	VLK_CALL    VLK_DisconnectTCP();


/** @brief Gyroscope calibration
 */
VLK_API	void	VLK_CALL    VLK_GyroCal();

/** @brief Disconnect current serial port connection
 */
VLK_API	void	VLK_CALL    VLK_DisconnectSerialPort();

/** @brief Set keep alive interval
 *  @param iMS keep alive interval in milliseconds range from 300 ms to 5000 ms
 *  @details keep alive interval determines the frequncy of querying Gimbal telemetry data,
 *           SDK default keep alive interval is 500 ms, no need to change it if not necessary
 */
VLK_API void	VLK_CALL    VLK_SetKeepAliveInterval(int iMS);

/** @brief Register device status callback
 *  @param pDevStatusCB receive device status callback function
 *  @param pUserParam user parameter, it will be passed back when pDevStatusCB is called
 *  @details pDevStatusCB will be called once SDK received new Gimbal,
 *           specifically, the telemetry data will keep updating once device is connected,
 *           ignore it if you don't need it
 */
VLK_API	void	VLK_CALL    VLK_RegisterDevStatusCB(VLK_DevStatus_CB pDevStatusCB, void* pUserParam);

/** @brief Control Gimbal yaw and pitch
 *  @param sHorizontalSpeed the speed of changing yaw (0.01 degrees/s), for example, 2000 means 20 degrees per second
 *  @param sVeritcalSpeed the speed of changing pitch (0.01 degrees/s)
 *  @details considering a small angle adjustment will result in a huge visual field changing in sky view, we limit
 *           speed in a proper range: \n
 *           -VLK_MAX_YAW_SPEED <= sHorizontalSpeed <= VLK_MAX_YAW_SPEED \n
 *           -VLK_MAX_PITCH_SPEED <= sVeritcalSpeed <= VLK_MAX_PITCH_SPEED \n
 *           some example: \n
 *           move up: VLK_Move(0, 1000); \n
 *           move left: VLK_Move(-1000, 0); \n
 *           move right: VLK_Move(1000, 0); \n
 *           move down: VLK_Move(0, -1000);
 */
VLK_API	void	VLK_CALL    VLK_Move(short sHorizontalSpeed, short sVeritcalSpeed);

/** @brief Stop moving
 *  @details once you call VLK_Move, Gimbal will keep moving until it reach the yaw and pitch limitation or VLK_Stop is called
 */
VLK_API	void	VLK_CALL    VLK_Stop();

/** @brief Zoom in
 *  @param sSpeed the speed of zoom in
 *  @details sSpeed must be a short number from VLK_MIN_ZOOM_SPEED to VLK_MAX_ZOOM_SPEED, the bigger the faster
 */
VLK_API	void	VLK_CALL    VLK_ZoomIn(short sSpeed);

/** @brief Zoom out
 *  @param sSpeed the speed of zoom out
 *  @details sSpeed must be a short number from VLK_MIN_ZOOM_SPEED to VLK_MAX_ZOOM_SPEED, the bigger the faster
 */
VLK_API	void	VLK_CALL    VLK_ZoomOut(short sSpeed);

/** @brief Stop zoom
 *  @details once you call VLK_ZoomIn or VLK_ZoomOut, Camera will keep zooming until it reach the limitation or VLK_StopZoom is called
 */
VLK_API	void	VLK_CALL    VLK_StopZoom();

/** @brief Enable track mode
 *  @param Param track mode parameters
 *  @see VLK_TrackTargetPositionEx
 *  @deprecated
 */
VLK_API	void	VLK_CALL    VLK_EnableTrackMode(const VLK_TRACK_MODE_PARAM* pParam);

/** @brief Track target by it's position
 *  @param iX target position, number of pixels from top-left corner in horizontal direction
 *  @param iY taget position, number of  pixels from top-left corner in vertical direction
 *  @param iVideoWidth video image width, e.g: 1920, 1280
 *  @param iVideoHeight video image height, e.g: 1080, 720
 *  @deprecated
 *  @see VLK_TrackTargetPositionEx
 */
VLK_API	void	VLK_CALL    VLK_TrackTargetPosition(int iX, int iY, int iVideoWidth, int iVideoHeight);

/** @brief Track target by it's position
 *  @param pParam track mode parameters,
 *  @param iX target position, number of pixels from top-left corner in horizontal direction
 *  @param iY taget position, number of  pixels from top-left corner in vertical direction
 *  @param iVideoWidth video image width, e.g: 1920, 1280
 *  @param iVideoHeight video image height, e.g: 1080, 720
 *  @details we usually use VLK_TRACK_TEMPLATE_SIZE_AUTO for pParam->emTrackTempSize and VLK_SENSOR_VISIBLE1 for pParam->emTrackSensor
 */
VLK_API	void	VLK_CALL    VLK_TrackTargetPositionEx(const VLK_TRACK_MODE_PARAM* pParam, int iX, int iY, int iVideoWidth, int iVideoHeight);

/** @brief Disable track mode
 */
VLK_API	void	VLK_CALL    VLK_DisableTrackMode();

/** @brief Focus in
 *  @param sSpeed the speed of focus in
 *  @details sSpeed must be a short number from VLK_MIN_ZOOM_SPEED to VLK_MAX_ZOOM_SPEED, the bigger the faster
 *           this function is available only when the Gimbal is in Manual focusing mode, call VLK_GetFocusMode to
 *           get current focus mode, call VLK_SetFocusMode to set focus mode
 */
VLK_API	void	VLK_CALL    VLK_FocusIn(short sSpeed);

/** @brief Focus out
 *  @param sSpeed the speed of focus out
 *  @details sSpeed must be a short number from VLK_MIN_ZOOM_SPEED to VLK_MAX_ZOOM_SPEED, the bigger the faster
 *           this function is available only when the Gimbal is in Manual focusing mode, call VLK_GetFocusMode to
 *           get current focus mode, call VLK_SetFocusMode to set focus mode
 */
VLK_API	void	VLK_CALL    VLK_FocusOut(short sSpeed);

/** @brief Stop focus
 *  @details  once you call VLK_FocusIn or VLK_FocusOut, Camera will keep focusing until it reach the limitation or VLK_StopFocus is called
 */
VLK_API	void	VLK_CALL    VLK_StopFocus();

/** @brief Move to home position
 */
VLK_API	void	VLK_CALL    VLK_Home();

/** @brief Switch motor
 *  @param iOn turn on/turn off \n
 *         1 turn on \n
 *         0 turn off
 */
VLK_API	void	VLK_CALL    VLK_SwitchMotor(int iOn);

/** @brief Check motor status
 *  @return current motor status
 *  @retval 1 on
 *  @retval 0 off
 */
VLK_API	int     VLK_CALL    VLK_IsMotorOn();

/** @brief Enable Gimbal follow drone
 *  @param iEnable enable or disable \n
 *         1 enable \n
 *         0 disable
 */
VLK_API	void	VLK_CALL    VLK_EnableFollowMode(int iEnable);

/** @brief Check follow mode
 *  @return current follow status
 *  @retval 1 enabled
 *  @retval 0 disabled
 */
VLK_API	int     VLK_CALL    VLK_IsFollowMode();

/** @brief Turn to specific yaw and pitch
 *  @param dYaw specific yaw, must be a double value from VLK_YAW_MIN to VLK_YAW_MAX
 *  @param dPitch specific pitch, must be a double value from VLK_PITCH_MIN to VLK_PITCH_MAX
 */
VLK_API	void	VLK_CALL    VLK_TurnTo(double dYaw, double dPitch);

/** @brief Enable track
 */
VLK_API	void	VLK_CALL    VLK_StartTrack();


/** @brief Disable track
 */
VLK_API	void	VLK_CALL    VLK_StopTrack();

/** @brief Check if track is enabled
 *  @return current track status
 *  @retval 1 enabled
 *  @retval 0 disabled
 */
VLK_API	int     VLK_CALL    VLK_IsTracking();

/** @brief Set track template size
 *  @param emSize track template size enumeration
 *  @see VLK_TRACK_TEMPLATE_SIZE
 */
VLK_API	void	VLK_CALL    VLK_SetTrackTemplateSize(VLK_TRACK_TEMPLATE_SIZE emSize);

/** @brief Set image color
 *  @param emImgType image type enumeration
 *  @param iEnablePIP enable picture in picture \n
 *         1 enable \n
 *         0 disable
 * @param emIRColor IR color enumeration
 *  @see VLK_IMAGE_TYPE
 */
VLK_API	void	VLK_CALL    VLK_SetImageColor(VLK_IMAGE_TYPE emImgType, int iEnablePIP, VLK_IR_COLOR emIRColor);

/** @brief Ask Gimbal take a photograph
 *  @details make sure there is SD card in the Gimbal
 */
VLK_API	void	VLK_CALL    VLK_Photograph();

/** @brief Gimbal start or stop recording
 *  @param iOn \n
 *          1 start recording \n
 *          0 stop recording
 *  @details make sure there is SD card in the Gimbal
 */
VLK_API	void	VLK_CALL    VLK_SwitchRecord(int iOn);

/** @brief Check if Gimbal is recording
 *  @return flag of recording status
 *  @retval 1 recording
 *  @retval 0 not recording
 *  @details make sure there is SD card in the Gimbal
 */
VLK_API	int     VLK_CALL    VLK_IsRecording();

/** @brief Enable defog
 *  @param iOn \n
 *          1 defog is enabled \n
 *          0 defog is disabled
 */
VLK_API	void	VLK_CALL    VLK_SwitchDefog(int iOn);

/** @brief Check if defog is enabled
 *  @return flag of defog status
 *  @retval 1 defog is enabled
 *  @retval 0 defog is disabled
 */
VLK_API	int     VLK_CALL    VLK_IsDefogOn();

/** @brief Set record mode
 *  @param emMode record mode enumeration
 *  @details some models could not take photo while it is recording,
 *           they must be switched to a certain
 * @see VLK_RECORD_MODE
 */
VLK_API	void	VLK_CALL    VLK_SetRecordMode(VLK_RECORD_MODE emMode);

/** @brief Get current record mode
 *  @return record mode enumeration
 *  @see VLK_RECORD_MODE
 */
VLK_API	int     VLK_CALL    VLK_GetRecordMode();

/** @brief Set focus mode ( manual focus or automatic focus)
 *  @param focus mode enumeration
 *  @see VLK_FOCUS_MODE
 */
VLK_API	void	VLK_CALL    VLK_SetFocusMode(VLK_FOCUS_MODE emMode);

/** @brief Get current focus mode ( manual focus or automatic focus)
 *  @return focus mode enumeration
 *  @see VLK_FOCUS_MODE
 */
VLK_API	int     VLK_CALL    VLK_GetFocusMode();

/** @brief Zoom to a specific magnification
 *  @param fMag specific magnification
 *  @details the camera will reach the limitation if the specific magnification over it's capability
 */
VLK_API	void	VLK_CALL    VLK_ZoomTo(float fMag);

/** @brief IR digital zoom in
 *  @param sSpeed deprecated, pass 0
 *  @details zoom in x1 then stop automatically
 */
VLK_API	void	VLK_CALL    VLK_IRDigitalZoomIn(short sSpeed);

/** @brief IR digital zoom out
 *  @param sSpeed deprecated, pass 0
 *  @details zoom out x1 then stop automatically
 */
VLK_API	void	VLK_CALL    VLK_IRDigitalZoomOut(short sSpeed);

/** @brief Switch EO digital zoom status
 *  @param iOn \n
 *         1 enable \n
 *         0 disable
 */
VLK_API	void	VLK_CALL    VLK_SwitchEODigitalZoom(int iOn);

/** @brief Enter S-BUS mode
 *  @deprecated
 */
VLK_API	void	VLK_CALL    VLK_EnterSBUSMode();

/** @brief Exit S-BUS mode
 *  @deprecated
 */
VLK_API	void	VLK_CALL    VLK_ExitSBUSMode();

/** @brief Query device configuration
 *  @details this function is asynchronous, configuration data will be passed back throw VLK_DevStatus_CB
 *  @see VLK_DevStatus_CB
 */
VLK_API	void	VLK_CALL    VLK_QueryDevConfiguration();

/** @brief Set OSD
 *  @param pParam OSG configuration
 *  @details we don't provide VLK_GetOSD because OSD is included in device configuration
 *  @see VLK_OSD_PARAM
 */
VLK_API	void	VLK_CALL    VLK_SetOSD(const VLK_OSD_PARAM* pParam);

/** @brief AI Gimbal Set OSD
 *  @param pParam OSG configuration
 *  @details we don't provide VLK_GetOSD because OSD is included in device configuration
 *  @see VLK_OSD_PARAM
 */
VLK_API	void	VLK_CALL    VLK_SetOSDLeft(const VLK_OSD_PARAM* pParam);

/** @brief  AI Gimbal Set OSD 
 *  @param pParam OSG configuration
 *  @details we don't provide VLK_GetOSD because OSD is included in device configuration
 *  @see VLK_OSD_PARAM
 */
VLK_API	void	VLK_CALL    VLK_SetOSDRight(const VLK_OSD_PARAM* pParam);

/** @brief Set wireless control channels map
 *  @param pChnlsMap wireless control channels map configuration
 *  @details we don't provide VLK_GetWirelessCtrlChnlMap because channels map is included in device configuration
 *  @see VLK_CHANNELS_MAP
 */
VLK_API	void	VLK_CALL    VLK_SetWirelessCtrlChnlMap(const VLK_CHANNELS_MAP* pChnlsMap);

/** @brief Send extent command
 *  @param szCmd command data pointer
 *  @param iLen command data length
 *  @details for some seldom-used Gimbal control command which we didn't provide specific interface,
 *           you should call this function to send command data directly   
 */
VLK_API	void	VLK_CALL    VLK_SendExtentCmd(const char* szCmd, int iLen);


/** @name Laser Control functions
 * @{
 */

/** @brief Switch laser status
 *  @param iOn \n
 *         1 turn on \n
 *         0 turn off
 *  @details make sure your device has laser capability
 */
VLK_API	void	VLK_CALL    VLK_SwitchLaser(int iOn);

/** @brief Laser single ranging
 */
VLK_API	void	VLK_CALL    VLK_LaserSingle();

/** @brief Laser zoom in
 *  @param sSpeed deprecated, pass 0
 */
VLK_API	void	VLK_CALL    VLK_LaserZoomIn(short sSpeed);

/** @brief Laser zoom out
 *  @param sSpeed deprecated, pass 0
 */
VLK_API	void	VLK_CALL    VLK_LaserZoomOut(short sSpeed);

/** @brief Laser stop zoom
 */
VLK_API	void	VLK_CALL    VLK_LaserStopZoom();

/** @brief Set laser zoom mode
 *  @param emMode laser zoom mode enumeration
 */
VLK_API	void	VLK_CALL    VLK_SetLaserZoomMode(VLK_LASER_ZOOM_MODE emMode);
/** @} Laser Control functions */

#ifdef __cplusplus
}
#endif

#endif //__VIEW_LINK_H__
