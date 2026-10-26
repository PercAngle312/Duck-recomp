#pragma once
/* Audio ABI subset; compile tests with -fshort-enums like pinned VitaSDK. */
typedef enum { SCE_AUDIO_OUT_MODE_MONO=0, SCE_AUDIO_OUT_MODE_STEREO=1 } SceAudioOutMode;
typedef enum { SCE_AUDIO_OUT_PORT_TYPE_MAIN=0 } SceAudioOutPortType;
int sceAudioOutOpenPort(SceAudioOutPortType,int,int,SceAudioOutMode);
int sceAudioOutSetConfig(int,unsigned,int,SceAudioOutMode);
int sceAudioOutOutput(int,const void*);
int sceAudioOutReleasePort(int);
