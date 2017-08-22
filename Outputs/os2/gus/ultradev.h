#ifdef __cplusplus
extern "C" {
#endif

APIRET UltraSetNVoices(int);
APIRET UltraEnableOutput(void);
APIRET UltraDisableOutput(void);
APIRET UltraBlockTimerHandler1();
APIRET UltraBlockTimerHandler2();
APIRET UltraUnblockAll(void);
APIRET UltraStartTimer(int, int);
APIRET UltraStopTimer(int);
APIRET UltraSetBalance(int, int);
APIRET UltraSetFrequency(int, int);
APIRET UltraSetAll(int, char, int, int, unsigned char, unsigned char);
APIRET UltraPokeData(long, int);
APIRET UltraPeekData(int *);
APIRET UltraMemAlloc(unsigned long, unsigned long *);
APIRET UltiMODMemAlloc(unsigned long, unsigned long *);
APIRET UltraMemFree(int size, int location);
APIRET UltiMODMemFree(void);
APIRET UltraMemInit(void);
APIRET UltraSizeDram(int *);
APIRET UltraStartVoice(int, unsigned int, unsigned int, unsigned int, unsigned char);
APIRET UltraStopVoice(int);
APIRET UltraVectorLinearVolume(int, unsigned int, unsigned char, unsigned char);
APIRET UltraDownload(void *, unsigned char, unsigned long, unsigned int, int);
APIRET UltraDownload2(void *, unsigned char, unsigned long, unsigned int, int);
APIRET UltraGetAccess(void);
APIRET UltraReleaseAccess(void);
APIRET UltraGetDriverVersion(int *);
APIRET UltraSetLoopMode(int, int);
APIRET UltraVoiceStopped(int *);

// Type of samples needed for UltraDownload (control byte)
#define DMA_16          0x40	//16 bits sample
#define DMA_8           0x00	//8 bits sample

#define DMA_CVT_2       0x80    //unsigned data
#define DMA_NO_CVT      0x00    //signed data


//voice register bits needed for UltraStartVoice (mode byte)
#define VC_DATA_TYPE            0x04            /* 0=8 bit,1=16 bit */
#define VC_LOOP_ENABLE          0x08            /* 1=enable */
#define VC_BI_LOOP		0x10		/* 1=bi directional looping */

#ifdef __cplusplus
};
#endif
