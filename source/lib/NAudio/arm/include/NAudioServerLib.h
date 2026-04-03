#ifndef __NAUDIOSERVERDLL_H_
#define __NAUDIOSERVERDLL_H_
#include <stdint.h>

//#define USE_TTS

//接口返回值
#define RET_SUCCESS								0  //操作成功
#define RET_START_CMDSERVER_FAIL				1  //启动命令服务失败
#define RET_START_DATASERVER_FAIL				2  //启动数据服务失败
#define RET_OPEN_AUDIOFILE_FAIL					3  //打开音频文件失败
#define RET_INIT_PAGING_FAIL					4  //初始化寻呼失败
#define RET_START_RECORD_FAIL					5  //启动录音失败
#define RET_DEV_NOT_EXIST						6  //操作时设备不存在
#define RET_DEV_EXIST							7  //设备已存在
#define RET_DEV_EXIST_IN_CALLINST				8  //对讲实例中已存在该设备
#define RET_BOTH_USERDEV_IN_CALLINST			9  //对讲实例中双方都是用户设备
#define RET_CALLINST_NOT_EXIST					10  //对讲实例不存在
#define RET_DEV_IS_CALLING						11  //设备正在通话
#define RET_PLAYINST_NOT_EXIST					12  //播放实例不存在
#define RET_SYSDEV_EXIST						13  //系统设备已存在
#define RET_SYSDEV_NOT_EXIST					14  //系统设备不存在
#define RET_CASTGROUP_EXIST						15  //系统广播分组已存在
#define RET_CASTGROUP_NOT_EXIST					16  //系统广播分组不存在
#define RET_GROUPDEV_EXIST						17  //设备已经存在在该组中
#define RET_GROUPDEV_NOT_EXIST					18  //设备未在在该组中
#define RET_NM_CASTGROUP_EXIST					19  //广播分组已在网络话筒中
#define RET_NM_CASTGROUP_NOT_EXIST				20  //网络话筒中不存在的广播分组
#define RET_NM_CASTDEV_EXIST					21  //广播设备已在网络话筒中
#define RET_NM_CASTDEV_NOT_EXIST				22  //网络话筒中不存在的广播设备
#define RET_NM_CALLDEV_EXIST					23  //对讲设备已在网络话筒中
#define RET_NM_CALLDEV_NOT_EXIST				24  //网络话筒中不存在的对讲设备
#define RET_DEV_NOT_SUPPORT_CALL				25  //设备不支持对讲
#define RET_DEV_NOT_SUPPORT_SAMPLER				26  //设备不支持采播
#define RET_DEV_RESP_TIMEOUT					27  //设备响应超时
#define RET_DEV_NOT_SUPPORT_FUNC				28  //设备不支持的功能
#define RET_DEV_GET_FILESIZE_FAIL				29  //获取文件大小失败
#define RET_DEV_DOWN_FILE_FAIL					30  //下载文件失败
#define RET_DEV_GET_DEVFILE_FAIL				31  //获取设备文件失败
#define RET_AUDIO_FILE_NOT_SUPPORTED    			32  //不支持的音频文件格式
#define RET_NOT_SUPPORT_FUNC    				33  //不支持的功能
#define RET_COMMON_FAIL							-1 //通用失败

//码率定义
#define BR_32KBPS								0
#define BR_64KBPS								1
#define BR_128KBPS								2

//设备状态定义
#define DEVS_OFFLINE							0  //离线状态
#define DEVS_IDLE								1  //空闲状态
#define DEVS_PLAYING							2  //播放状态
#define DEVS_INTERCOM							3  //对讲状态
#define DEVS_SAMPLER							4  //采集状态


//设备状态定义
#define DEVS_OFFLINE							0  //离线状态
#define DEVS_IDLE								1  //空闲状态
#define DEVS_PLAYING							2  //实时播放状态
#define DEVS_INTERCOM							3  //对讲状态
#define DEVS_SAMPLER							4  //采集状态
#define DEVS_ALARM								5  //报警状态
#define DEVS_LOCAL_PLAYING   					6  //本地播放状态
#define DEVS_TTS_PLAYING	   					7  //播放TTS状态

//播放状态
#define PS_STOP			0  //播放停止状态
#define PS_PLAYING		1  //播放中
#define PS_PAUSE		2  //播放暂停状态

//场景类型定义
#define SCENE_PLAY_AUDIOFILE		0 //播放音频文件
#define SCENE_PLAY_TTS				1 //文字转语音播放
#define SCENE_PLAY_PAGING			2 //话筒寻呼播放
#define SCENE_AUDIO_STREAM			3 //播放音频流

//播放实例
#define	PLAY_INST1					0 //播放实例1
#define	PLAY_INST2					1 //播放实例2
#define	PLAY_INST3					2 //播放实例3
#define	PLAY_INST4					3 //播放实例4
#define	PLAY_INST5					4 //播放实例5

//音频流格式
#define ASF_PCM						0 //PCM格式
#define ASF_MP3						1 //mp3格式
#define ASF_IMA_ADPCM				2 //IMA-ADPCM格式

//定义
#define DISABLE						0  //禁止
#define ENABLE						1  //使能

//软件端编码格式
#define CALL_ENC_PCM				0   //原始数据
#define CALL_ENC_ADPCM				1   //ADPCM编码，默认编码格式

//呼叫状态
#define CALLS_IDLE					0	//空闲状态
#define CALLS_START					1	//启动对讲
#define CALLS_RING					2	//响铃状态
#define CALLS_CALLING				3	//呼叫状态
#define CALLS_HANGUP				4	//挂断状态

//呼叫回调类型
#define CALLCB_NONE						0
#define CALLCB_STATUS_CHANGE			1
#define CALLCB_CALLDATA_TO_UDEV			2   //对讲设备到用户设备

//网络话筒的音源
#define NM_AS_MIC						0 //前置MIC
#define NM_AS_LINEIN					1 //后置LINEIN

//网络话筒广播采样率定义
#define NM_SMPR_8KHZ					0
#define NM_SMPR_16KHZ					1
#define NM_SMPR_24KHZ					2
#define NM_SMPR_32KHZ					3

//音频帧头长度
#define AUDIO_FRAME_HEAD_LEN			36

//下载文件状态
#define DFS_IDLE							0
#define DFS_DOWNLOADING						1
#define DFS_DOWN_FAIL						2
#define DFS_WRITE_FAIL						3

//文件属性定义
#define FA_DIR								0
#define FA_FILE								1
#define FA_UNKNOWN							2

//异步函数返回类型定义
#define FUNC_GET_HWINFO							1
#define FUNC_DOWNLOAD_FILE						2
#define FUNC_DOWNLOAD_STATUS					3
#define FUNC_GET_DEV_FILES						4
#define FUNC_DELETE_DEV_FILE					5
#define FUNC_FORMAT_DEVICE						6
#define FUNC_GET_DEV_MEMINFO					7
#define FUNC_PLAY_DEV_FILE						8
#define FUNC_STOP_PLAY_DEV_FILE					9
#define FUNC_GET_PLAYING_FILE					10
#define FUNC_GET_DEV_FILEINFO					11
#define FUNC_UPLOAD_FILE						12
#define FUNC_GET_IO_STATUS						13
#define FUNC_SET_IO_VAL							14

//播放信息
typedef struct {
	unsigned int playstatus;  //播放状态
	int progress;  //播放进度0-100
	unsigned int totaltime;  //总播放时间(单位s)
	unsigned int samplerate;  //采样率
	unsigned int bitrate;  //比特率
}_play_info;

//监听参数
typedef struct
{
	unsigned char auto_mon;		//是否启动上线自动监听(1，自动监听)
	unsigned char save_audio_data;		//是否保存音频数据(1，保存)
	char save_dir[255];			//监听文件保存目录 
	char file_name[64];			//保存文件名
	unsigned int file_size;     //保存单个文件的大小
}_mon_para;

//自动触发参数，采集设备监听到有音源输入时自动启动采播
typedef struct
{
	unsigned char trig_enable;  //自动触发开关
	unsigned char priority;
	unsigned short trig_val; //触发值
}_auto_trig_para;

typedef unsigned long long* CALL_HANDLE;  //对讲实例

//对讲实例信息
typedef struct {
	unsigned int call_devno;  //主叫编号
	unsigned int called_devno;  //被叫编号
	unsigned int call_status;  //呼叫状态
}_call_inst_info;

//设备信息
typedef struct {
	unsigned int devno;  //设备编号
	char devname[16];	 //设备名称
	unsigned int devstatus;  //设备状态
}_device_info;

//组信息
typedef struct {
	unsigned int groupno;  //组编号
	char groupname[16];	 //分组名称
}_group_info;

//广播参数
typedef struct {
	unsigned char audio_src;  //音源,只对网络话筒有效(NM_AS_MIC \NM_AS_LINEIN)
	unsigned char priority;	//优先级 (1-255)
	unsigned short samplerate;	//采样率
	unsigned char input_gain;  //输入增益	
	unsigned char reserve[3];
}_cast_para;

//音频流格式
typedef struct {
	uint32_t audio_head;	 //音频包头标记
	uint8_t scene_type;  //场景,0:广播,1:对讲,2
	uint8_t instance;  //实例
	uint8_t priority;  //帧优先级
	uint8_t audiofmt;	 //音频格式	
	uint32_t sampleRate;  //采样率
	uint32_t bitrate;  //比特率		
	uint8_t reserve[3];  //保留
	uint8_t addr_type;  //地址类型,0:编号,1:IP地址
	uint32_t audio_src;  //音源（设备编号/IP）
	uint32_t audio_dst;  //目标（设备编号/IP)，仅对讲时用
	uint32_t frame_no;  //帧序号	
	uint16_t frame_len;  //数据帧长度
	uint16_t reserve1;  //保留
	uint8_t data[1200];  //帧数据
}_audio_frame;

//硬件信息
typedef struct
{
	char IMEI[16];
	char iccid[21];
}_dev_hwinfo;

//下载状态
typedef struct
{
	unsigned char status;
	unsigned char percent;
}_down_status;

typedef struct{	
	unsigned char dir;   //0:目录,1:文件
	unsigned int file_size;  //文件大小
	char szFileName[256];  //文件名/路径
}_dev_fileinfo;

typedef struct
{
	unsigned short packet_no; //包编号
	unsigned short status; //0:正常读取,1:全部文件读取完成,2:读取错误	
	int count; //文件数
	_dev_fileinfo files_info[256];
}_dev_files;

//设备存储信息
typedef struct
{
	int total_size; //总大小KB
	int free_size; //剩余大小KB
}_dev_meminfo;

//设备文件数据
typedef struct
{
	int devno;
	int packet_no; //包编号
	int status; //状态
	int data_len; //数据长度
	uint8_t *data;  //文件数据
}_dev_filedata;

/*收到广播音频流回调
devno:设备编号
frame:回调的音频帧
*/
typedef void(* _pfn_stream_callback)(unsigned int devno, unsigned char* data,int len);

/*设备状态改变通知
devno:设备编号
online:0,设备下线;1设备上线
*/
typedef void(*_pfn_device_status_change)(unsigned int devno,unsigned short status);
/*播放状态改变通知
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
status:播放状态
*/
typedef void(* _pfn_play_status_change)(unsigned char scene, unsigned char inst, unsigned int status);

/*
监听解码回调,回调数据用于某个设备音频流实时播放
samplerate:采样率
data:解码的数据
len:解码后的数据长度
*/
typedef void(* _pfn_monitor_decode)(unsigned int samplerate,unsigned char* data,unsigned int len);

/*
监听收到数据回调，
devno:设备编号
data:ADPCM-IMA格式的数据
len:数据长度
*/
typedef void(* _pfn_monitor_callback)(unsigned int devno,unsigned char* data, unsigned int len);

/*
对讲收到数据回调，
devno:设备编号
cbtype:回调类型
data:回调数据
len:数据长度
*/
typedef void(* _pfn_call_callback)(CALL_HANDLE call_inst, unsigned int cbtype, unsigned char* data, unsigned int len);

/*收到IO端口变化回调(只有输入端口值有变化时才回调此函数)
devno:设备编号
prevIOVal:IO端口变化前的端口值
currIOVal:当前IO端口值
*/
typedef void(* _pfn_io_callback)(unsigned int devno, unsigned short prevIOVal, unsigned short currIOVal);

/*收到数据串口数据回调
devno:设备编号
data:收到的串口数据
len:收到的串口数据长度
*/
typedef void(* _pfn_serial_callback)(unsigned int devno,unsigned char *data,int len);

/*设置函数异步回调
devno:设备编号
cbtype:异步回调类型
data:异步回调参数
*/
typedef void(* _pfn_async_callback)(unsigned int devno,int cbtype, void* data);

#ifdef __cplusplus
extern "C"
{
#endif
/*****************************系统设置**********************************/
/*设置服务端回调函数
dsc:设备状态改变通知函数
psc:播放状态改变通知函数
*/
int na_set_callback(_pfn_device_status_change dsc, _pfn_play_status_change psc);

/*设置函数异步回调用
asc:异步回调通知函数
*/
int na_set_async_callback(_pfn_async_callback asc);

/*设置广播音频流回调函数
sscb:音频流回调函数
*/
int na_set_stream_callback(_pfn_stream_callback sscb);
	
/*启动音频服务
servip:服务器IP
devport:设备服务端口,设备服务端口有两个,第一个为命令端口(默认8890),第二个为数据端口,此处指令命令端口，数据端口为命令端口+1
*/
int na_server_start(const char* servip, int devport);
/*
停止音频服务
程序退出时调用
*/
int na_server_stop();

/*****************************设备管理**********************************/
/*设置设备心跳超时
  timeout:心跳超时时间，默认心跳超时时间为6秒
*/
int na_set_heartbeat_timeout(unsigned int timeout);
/*获取在线设备
devlist:在线设备编号列表,每个设备在系统中有唯一编号
devnbr:设备个数(输入输出参数),传入的*devnbr为buffer devlist的大小,执行完后*devnbr为实际设备的个数
*/
int na_get_online_device(unsigned int* devlist,int *devnbr);
/*获取设备名称
devno:设备编号
devnbr:设备名称,最大32byte
*/
int na_get_device_name(unsigned int devno, char* devname);

/*获取设备类型
devno:设备编号
type_name:设备类型,最大16byte
*/
int na_get_type_name(unsigned int devno, char* type_name);

/*获取设备功能
devno:设备编号
devfunc:设备功能
*/
int na_get_device_func(unsigned int devno, unsigned int *devfunc);

/*获取设备固件版本
devno:设备编号
version:设备固件版本,最大16byte
*/
int na_get_device_version(unsigned int devno, char* version);

/*获取设备状态
devno:设备编号
status:设备状态,DEVS_OFFLINE、DEVS_IDLE....
*/
int na_get_device_status(unsigned int devno, unsigned short* status);

/*复位设备
devno:设备编号
*/
int na_reset_device(unsigned int devno);

/************************************音频播放接口************************************/
/*获取设备音量
devno:设备编号
volume:设备音量,0-100....
*/
int na_get_device_volume(unsigned int devno, unsigned char* volume);

/*设置设备音量
devno:设备IP
volume:设备音量,0-100....
*/
int na_set_device_volume(unsigned int devno, unsigned char volume);
/*添加播放设备到指定场景的播放实例
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
devip:添加的设备IP
*/
int na_add_play_device(unsigned char scene, unsigned char inst, unsigned int devno);
/*从指定场景的播放实例移除播放设备
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
devip:移除的设备IP
*/
int na_remove_play_device(unsigned char scene, unsigned char inst, unsigned int devno);

/*清空指定场景播放实例的播放设备
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
*/
int na_clear_play_device(unsigned char scene, unsigned char inst);

/*
获取播放设备
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
pdevlist:获取到的设备编号列表
devcnt:获取到的设备数目
*/
int na_get_play_device(unsigned char scene, unsigned char inst,unsigned int* pdevlist, int* devcnt);

/*设置播放音量(音源)
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
volume:音量(0-100)
*/
int na_set_play_volume(unsigned char scene, unsigned char inst, int volume);

/*获取播放音量(音源)
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
volume:音量(0-100)
*/
int na_get_play_volume(unsigned char scene, unsigned char inst, int* volume);

/*设置广播系统的最大码率
max_bitrate:
*/
int na_set_max_bitrate(int max_bitrate);

/*设置TTS播放语速
inst:播放实例，PLAY_INST1、PLAY_INST2...
speed:语速(-10~+10,0为正常语速)
*/
int na_set_tts_speed(unsigned char inst, int speed);

/*获取TTS播放语速
inst:播放实例，PLAY_INST1、PLAY_INST2...
speed:语速(-10~+10,0为正常语速)
*/
int na_get_tts_speed(unsigned char inst, int *speed);

/*设置TTS语音增益
inst:播放实例，PLAY_INST1、PLAY_INST2...
gain:0-100
*/
int na_set_tts_gain(int gain);

/*设置TTS生成文件的目录
dir:生成的TTS文件存放的目录
*/
int na_set_tts_dir(char* dir);

/*
set xunfei tts params
login_params:
session_params:
*/
int na_set_tts_params(const char* login_params,const char* session_params);


/*设置推流音频参数
inst:推流播放实例，PLAY_INST1、PLAY_INST2...
audiofmt:音频流格式,ASF_PCM、ASF_MP3
channelnbr:通道数(仅对PCM格式有效)
samplerate:采样率(仅对PCM格式有效)
*/
int na_set_audio_stream_para(unsigned char inst,int audiofmt,int channelnbr,int samplerate);

/*解码一帧IMA-ADPCM音频数据
data_in:一帧IMA-ADPCM音频数据，包括帧头信息
data_out:IMA-ADPCM解码后的原始音频数据，长度为2020bytes
*/
int na_decode_imaadpcm(unsigned char* data_in, unsigned char* data_out);

/*发送音频数据
inst:推流播放实例，PLAY_INST1、PLAY_INST2...
data:发送的音频数据
len:音频数据长度
*/
int na_send_play_data(unsigned char inst, unsigned char* data,int len);

/*启动音频播放,同时传入要播放的设备
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
audiosrc:
	音频文件播放场景:audiosrc为音频文件路径,支持mp3、wma、wav 格式音频
	文字转语音播放场景:audiosrc为需要播放的文字
	寻呼场景:忽略此参数
priority:音源的优先级(0-255),值越大优先级越高
pdevlist:播放的设备编号列表
devcnt:设备数目
*/
int na_start_play(unsigned char scene, unsigned char inst,const char* audiosrc,\
	int priority, unsigned int* pdevlist,int devcnt);

/*启动同步音频播放,同时传入要播放的设备,播放完成后函数返回,实时寻呼场景除外(立即返回)
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
audiosrc:
	音频文件播放场景:audiosrc为音频文件路径,支持mp3、wma、wav 格式音频
	文字转语音播放场景:audiosrc为需要播放的文字
	寻呼场景:忽略此参数
priority:音源的优先级(0-255),值越大优先级越高
pdevlist:播放的设备编号列表
devcnt:设备数目
*/
int na_start_play_sync(unsigned char scene, unsigned char inst,const char* audiosrc, \
	int priority, unsigned int* pdevlist, int devcnt);

/*启动音频播放,播放设备通过na_add_play_device和na_remove_play_device函数添加和删除
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
audiosrc:
	音频文件播放场景:audiosrc为音频文件路径,支持mp3、wma、wav 格式音频
	文字转语音播放场景:audiosrc为需要播放的文字
	寻呼场景:忽略此参数
priority:音源的优先级(0-255),值越大优先级越高
*/

int na_start_play_1(unsigned char scene, unsigned char inst,const char* audiosrc, int priority);

/*启动同步音频播放,播放设备通过na_add_play_device和na_remove_play_device函数添加和删除,播放完成后函数返回,实时寻呼场景除外(立即返回)
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
audiosrc:
	音频文件播放场景:audiosrc为音频文件路径,支持mp3、wma、wav 格式音频
	文字转语音播放场景:audiosrc为需要播放的文字
	寻呼场景:忽略此参数
priority:音源的优先级(0-255),值越大优先级越高
*/
int na_start_play_sync_1(unsigned char scene, unsigned char inst,const char* audiosrc, int priority);

/*暂停播放音频
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
*/
int na_pause_play(unsigned char scene, unsigned char inst);
/*暂停之后恢复播放音频
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
*/
int na_restore_play(unsigned char scene, unsigned char inst);
/*停止播放音频
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
*/
int na_stop_play(unsigned char scene, unsigned char inst);
/*获取播放信息
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
playinfo:获取到的播放信息
*/
int na_get_playinfo(unsigned char scene, unsigned char inst,_play_info* playinfo);
/*获取播放状态
scene:播放场景,SCENE_PLAY_AUDIOFILE、SCENE_PLAY_TTS、SCENE_PLAY_PAGING
inst:播放实例，PLAY_INST1、PLAY_INST2...
status:获取到的播放状态
*/
int na_get_play_status(unsigned char scene, unsigned char inst,unsigned int* status);

/*获取音频流
audiostreamlist:所有音频流
stream_cnt:音频流数目
*/
int na_get_audio_stream(unsigned int* audiostreamlist, int* stream_cnt);

/**************************************监听接口*******************************************/
/*设置监听回调函数，
* 
mon_cb:监听数据回调，所有监听到的音频数据(ADPCM-IMA格式)通过此接口回调
decode_cb:解码回调，用于将监听到的音频数据实时播放出来，不需要此功能传递NULL
*/
int na_set_monitor_callback(_pfn_monitor_callback mon_cb,_pfn_monitor_decode decode_cb);

/*设置设备自动触发采集参数，只对采集设备有效。
采集设备只有在上电开始才获取此参数，要做自动触发需要先启动服务器，采集设备再上电，设备才能进入自动触发采集状态。
devno:设备编号
para:自动触发参数
*/
int na_set_auto_trig_para(unsigned int devno, _auto_trig_para* para);

/*设置监听参数
devno:设备编号
para:监听参数
*/
int na_set_monitor_para(unsigned int devno, _mon_para* para);

/*设备启动监听，启动后设备将发出监听数据,监听数据通过_pfn_monitor_decode回调
devno:设备编号
*/
int na_start_dev_monitor(unsigned int devno);

/*设备停止监听，停止后设备将停止发送监听数据
devno:设备编号
*/
int na_stop_dev_monitor(unsigned int devno);

/*设备启动监听，启动后设备将发出监听数据,监听数据通过_pfn_stream_callback回调
devno:设备编号
*/
int na_start_dev_monitor_1(unsigned int devno);

/*设备停止监听，停止后设备将停止发送监听数据
devno:设备编号
*/
int na_stop_dev_monitor_1(unsigned int devno);

/*启动监听解码,将对选择的设备的音频数据解码,解码后的数据通过_pfn_monitor_decode回调,可以调用API进行播放
devno:设备编号
*/
int na_start_monitor_decode(unsigned int devno);

/*停止监听解码,不再解码音频数据
*/
int na_stop_monitor_decode();

/**************************************对讲接口*******************************************/
/*设置软件端对讲编码格式
  encode_fmt:编码格式，CALL_ENC_PCM/CALL_ENC_ADPCM,默认ADPCM，带宽为PCM的1/4
*/
int na_set_call_encode_fmt(int encode_fmt);

/*
	设置对讲录音  
	enable:对讲录音打开或关闭
	recdir:录音文件存放目录,recdir为NULL时存放在默认目录
*/
int na_set_call_record(int enable,const char* recdir);

/*设置对讲回调函数，
talk_cb:对讲回调函数
*/
int na_set_call_callback(_pfn_call_callback call_cb);

/*添加用户对讲设备（虚拟对讲设备）
devno:设备编号
devname:设备名称
*/
int na_add_user_call_device(unsigned int devno,const char* devname);

/*移除用户对讲设备（虚拟对讲设备）
talk_dev:对讲设备
*/
int na_remove_user_call_device(unsigned int devno);

/*获取对讲终端，
devlist:所有对讲设备
devnbr:对讲设备数
*/
int na_get_call_device(unsigned int* devlist, int* devnbr);

/*获取用户对讲设备（虚拟对讲设备）
devlist:所有对讲设备
devnbr:对讲设备数
*/
int na_get_user_call_device(unsigned int* devlist, int* devnbr);

/*获取对讲实例中的用户设备
call_inst:对讲的实例
devno:用户设备编号
*/
int na_get_call_inst_user_device(CALL_HANDLE call_inst, unsigned int *devno);

/*获取对讲实例信息
call_inst:对讲的实例
p_inst_info:实例信息
*/
int na_get_call_inst_info(CALL_HANDLE call_inst, _call_inst_info* p_inst_info);

/*检测对讲实例是否有效
call_inst:对讲的实例
inst_valid:0,无效 1，有效
*/
int na_is_call_inst_valid(CALL_HANDLE call_inst, unsigned char* inst_valid);

/*通过主叫编号获取对讲实例，
call_devno:主叫设备编号
call_inst:获取到的实例
*/
int na_get_call_inst_by_call_devno(unsigned int call_devno, CALL_HANDLE* call_inst);

/*通过被叫编号获取对讲实例，
call_devno:被叫设备编号
call_inst:获取到的实例
*/
int na_get_call_inst_by_called_devno(unsigned int called_devno, CALL_HANDLE* call_inst);

/*主叫方启动对讲，
call_inst:对讲实例
call_devno:主叫设备(用户设备)编号
called_devno:被叫设备编号
*/
int na_start_call(CALL_HANDLE* call_inst, unsigned int call_devno, unsigned int called_devno);

/*停止对讲，
call_inst:对讲实例
*/
int na_stop_call(CALL_HANDLE call_inst);

/*作为被叫方接听对讲，
call_inst:对讲实例
*/
int na_answer_call(CALL_HANDLE call_inst);

/*推流音频数据到对讲终端
call_inst:对讲实例
data:PCM音频数据,8kHZ采样率，单声道，16bit采样，小端模式
sample_nbr:采样点数
*/
int na_send_audio_data_to_calldev(CALL_HANDLE call_inst,unsigned short* data,int sample_nbr);

/*开设备端门禁
call_inst:对讲实例
*/
int na_call_unlock(CALL_HANDLE call_inst);

/**************************************网络话筒接口*******************************************/

/*添加设备到系统
* devno:设备编号
* devname:设备名称
*/
int na_add_sysdev(unsigned int devno, char* devname);

/*从系统中移除设备
* devno:设备编号
*/
int na_remove_sysdev(unsigned int devno);

/*获取系统设备
* devinfolist:所有设备信息
* devcnt:设备数目
*/
int na_get_sysdev(_device_info* devinfolist, int* devcnt);

/*清空系统设备
*/
int na_clear_sysdev();

/*添加广播分组到系统
* groupno:分组编号
* group_name:分组名称
*/
int na_add_castgroup(unsigned int groupno,char* group_name);

/*从系统中移除广播分组
* groupno:移除的广播分组编号
*/
int na_remove_castgroup(unsigned int groupno);

/*获取广播分组
* groupinfolist:所有广播分组信息
* groupcnt:广播分组数目
*/
int na_get_castgroup(_group_info* groupinfolist, int* groupcnt);

/*清空广播分组
*/
int na_clear_castgroup();

/*添加广播设备到广播分组中
* groupno:分组编号
* devno:系统设备编号
*/
int na_add_dev_to_group(unsigned int groupno, unsigned int devno);

/*从广播分组中移除设备
* groupno:分组编号
* devno:系统设备编号
*/
int na_remove_dev_from_group(unsigned int groupno, unsigned int devno);

/*获取组设备
* groupno:分组编号
* devnolist:分组下设备编号列表
* devcnt:设备数目
*/
int na_get_groupdev(unsigned int groupno, unsigned int* devnolist,int *devcnt);

/*清空组设备
* groupno:分组编号
*/
int na_clear_groupdev(unsigned int groupno);

/*为网络话筒添加广播分组
* nm_devno:网络话筒编号
* groupno:分组编号
*/
int na_nm_add_castgroup(unsigned int nm_devno, unsigned int groupno);

/*网络话筒移除广播分组
* nm_devno:网络话筒编号
* groupno:移除的设备分组编号
*/
int na_nm_remove_castgroup(unsigned int nm_devno, unsigned int groupno);

/*网络话筒获取广播分组
* nm_devno:网络话筒编号
* groupnolist:获取的广播分组编号列表
* groupcnt:分组数
*/
int na_nm_get_castgroup(unsigned int nm_devno, unsigned int* groupnolist,int* groupcnt);

/*网络话筒清空广播分组
* nm_devno:网络话筒编号
*/
int na_nm_clear_castgroup(unsigned int nm_devno);

/*为网络话筒添加广播设备
* nm_devno:网络话筒编号
* devinfo:设备信息
*/
int na_nm_add_castdev(unsigned int nm_devno, unsigned int devno);

/*网络话筒移除广播设备
* nm_devno:网络话筒编号
* devno:移除的设备编号
*/
int na_nm_remove_castdev(unsigned int nm_devno, unsigned int devno);

/*网络话筒获取广播
* nm_devno:网络话筒编号
* devnolist:获取的广播设备编号列表
* devcnt:设备数
*/
int na_nm_get_castdev(unsigned int nm_devno, unsigned int* devnolist, int* devcnt);

/*网络话筒清空广播设备
* nm_devno:网络话筒编号
*/
int na_nm_clear_castdev(unsigned int nm_devno);

/*为网络话筒添加对讲设备
* nm_devno:网络话筒编号
* devinfo:对讲设备信息
*/
int na_nm_add_calldev(unsigned int nm_devno, unsigned int devno);

/*网络话筒移除对讲设备
* nm_devno:网络话筒编号
* devno:移除的设备编号
*/
int na_nm_remove_calldev(unsigned int nm_devno, unsigned int devno);

/*网络话筒获取对讲设备
* nm_devno:网络话筒编号
* devnolist:获取的对讲设备编号列表
* devcnt:设备数
*/
int na_nm_get_calldev(unsigned int nm_devno, unsigned int* devnolist, int* devcnt);

/*网络话筒清空对讲设备
* nm_devno:网络话筒编号
*/
int na_nm_clear_calldev(unsigned int nm_devno);

/*网络话筒启动广播
nm_devno:话筒编号
cast_para:广播参数
*/
int na_nm_start_cast(unsigned int nm_devno, _cast_para cast_para);

/*网络话筒停止广播
nm_devno:话筒编号
*/
int na_nm_stop_cast(unsigned int nm_devno);

/*网络话筒启动对讲
call_devinfo:主叫设备信息
called_devinfo:被叫设备信息
*/
int na_nm_start_call(_device_info call_devinfo, _device_info called_devinfo);

/*网络话筒停止对讲
devno:要执行话筒编号
*/
int na_nm_stop_call(unsigned int devno);

/*网络话筒接听对讲
devno:话筒编号
*/
int na_nm_answer_call(unsigned int devno);

/**************************************端口控制接口*******************************************/
/*设置IO端口回调函数，
io_cb:IO端口回调函数
*/
int na_set_io_callback(_pfn_io_callback io_cb);
/*获取设备端口状态，会阻塞，在线程中调用
devip:设备编号
iomode:端口模式,一个端口占2bit,0:悬空输入；1:上拉输入;2:下拉输入;3:推挽输出
ioval:端口值
*/
int na_get_iostatus(unsigned int devno, unsigned int* iomode, unsigned int* ioval);

/*获取设备端口状态(异步方式)
devip:设备编号
iomode:端口模式,一个端口占2bit,0:悬空输入；1:上拉输入;2:下拉输入;3:推挽输出
ioval:端口值
*/
int na_get_iostatus_async(unsigned int devno);

/*设置设备端口状态，会阻塞，在线程中调用
devno:设备编号
ioval:端口值，低16bit表示要设置的端口值，高16bit指定哪个端口需要设置
*/
int na_set_ioval(unsigned int devno, unsigned int ioval);

/*设置设备端口状态(异步方式)
devno:设备编号
ioval:端口值，低16bit表示要设置的端口值，高16bit指定哪个端口需要设置
*/
int na_set_ioval_async(unsigned int devno, unsigned int ioval);

/**************************************串口控制接口*******************************************/
/*设置数据串口回调函数，
dserial_cb:数据串口回调函数
*/
int na_set_dserial_callback(_pfn_serial_callback dserial_cb);

/*写数据串口
devno:设备编号
data:写入的串口数据
len:写入的数据长度
*/
int na_write_dserial(unsigned int devno, unsigned char* data,int len);

/************************************4G终端控制接口*****************************************/
/*获取4G终端的信号
devno:设备编号
csq:信号强度
*/
int na_get_csq(unsigned int devno, unsigned char* csq);

/*获取4G终端的硬件信息
devno:设备编号
imei:设备的IMEI号
iccid:SIM卡的卡ID
*/
int na_get_hwinfo(unsigned int devno,char* imei,char* iccid);

/*异步方式获取4G终端的硬件信息
devno:设备编号
*/
int na_get_hwinfo_async(unsigned int devno);

/*下载文件到4G终端
devno:设备编号
url:音频文件的URL,不支持重定向URL
path:音频文件下载到4G终端的路径
*/
int na_download_file(unsigned int devno,const char* url,const char* path);

/*异步方式下载文件到4G终端
devno:设备编号
url:音频文件的URL,不支持重定向URL
path:音频文件下载到4G终端的路径
*/
int na_download_file_async(unsigned int devno,const char* url,const char* path);

/*获取4G终端下载文件状态
devno:设备编号
status:下载状态
percent:下载百分比
*/
int na_get_download_status(unsigned int devno,unsigned char* status, unsigned char* percent);

/*异步方式获取4G终端下载文件状态
devno:设备编号
*/
int na_get_download_status_async(unsigned int devno);

/*异步模式上载文件到服务器
devno:设备编号
path:文件的路径
packetno:包编号
*/
int na_upload_file_async(unsigned int devno,const char* path,int packetno);

/*获取设备指定目录文件
devno:设备编号
currdir:当前目录
file_info:获取到的文件信息列表
file_count:文件个数
*/
int na_get_dev_files(unsigned int devno,const char* currdir, _dev_fileinfo* file_info,int *file_count);

/*异步方式获取设备指定目录文件
devno:设备编号
currdir:当前目录
packet_no:包编号
*/
int na_get_dev_files_async(unsigned int devno,const char* currdir,int packet_no);

/*获取设备文件信息
devno:设备编号
fileinfo:设备文件信息
*/
int na_get_dev_fileinfo(unsigned int devno, _dev_fileinfo* fileinfo);

/*异步方式获取设备文件信息
devno:设备编号
fileinfo:设备文件信息
*/
int na_get_dev_fileinfo_async(unsigned int devno,const char *path);

/*删除设备文件,执行删除文件后需等1秒才能读取设备文件，期间不能有读写文件操作
devno:设备编号
path:删除的文件或目录
*/
int na_del_dev_file(unsigned int devno,const char* path);


/*异步方式删除设备文件,执行删除文件后需等1秒才能读取设备文件，期间不能有读写文件操作
devno:设备编号
path:删除的文件或目录
*/
int na_del_dev_file_async(unsigned int devno,const char* path);


/*格式化设备存储,将删除所有设备文件,执行格式化需要6秒才能完成,期间不能有读写文件操作
devno:设备编号
*/
int na_format_device(unsigned int devno);

/*异步方式格式化设备存储,将删除所有设备文件,执行格式化需要6秒才能完成,期间不能有读写文件操作
devno:设备编号
*/
int na_format_device_async(unsigned int devno);

/*获取设备存储信息
devno:设备编号
total_size:总的存储空间大小，单位KB
free_size:剩余的存储空间大小，单位KB
*/
int na_get_dev_meminfo(unsigned int devno,int *total_size, int* free_size);

/*异步方式获取设备存储信息
devno:设备编号
*/
int na_get_dev_meminfo_async(unsigned int devno);

/*播放设备的音频文件
devno:设备编号
path:设备音频文件路径
priority:优先级，0-255
*/
int na_dev_play(unsigned int devno,const char *path,int priority);

/*异步方式播放设备的音频文件
devno:设备编号
path:设备音频文件路径
priority:优先级，0-255
*/
int na_dev_play_async(unsigned int devno,const char* path, int priority);

/*播放状态为本地播放时调用此函数获取正在播放的本地文件
devno:设备编号
path:设备音频文件路径
*/
int na_get_playing_file(unsigned int devno, char* path);

/*播放状态为本地播放时调用此函数获取正在播放的本地文件(异步方式)
devno:设备编号
path:设备音频文件路径
*/
int na_get_playing_file_async(unsigned int devno);

/*设备停止播放音频文件
devno:设备编号
*/
int na_dev_stop_play(unsigned int devno);

/*设备停止播放音频文件(异步方式)
devno:设备编号
*/
int na_dev_stop_play_async(unsigned int devno);

/************************************测试接口*****************************************/
/*
crash测试
*/
int na_crash_test();

#ifdef __cplusplus
}
#endif

#endif

