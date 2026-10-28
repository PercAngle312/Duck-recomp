#pragma once
/* Audio ABI subset; compile tests with -fshort-enums like pinned VitaSDK. */
typedef enum { SCE_AUDIO_OUT_MODE_MONO=0, SCE_AUDIO_OUT_MODE_STEREO=1 } SceAudioOutMode;
typedef enum { SCE_AUDIO_OUT_PORT_TYPE_MAIN=0 } SceAudioOutPortType;
typedef enum {SCE_AUDIO_VOLUME_FLAG_L_CH=1,SCE_AUDIO_VOLUME_FLAG_R_CH=2} SceAudioOutChannelFlag;
enum {SCE_AUDIO_OUT_MAX_VOL=32768};
int sceAudioOutOpenPort(SceAudioOutPortType,int,int,SceAudioOutMode);
int sceAudioOutSetVolume(int,SceAudioOutChannelFlag,int*);
int sceAudioOutSetConfig(int,unsigned,int,SceAudioOutMode);
int sceAudioOutOutput(int,const void*);
int sceAudioOutReleasePort(int);
