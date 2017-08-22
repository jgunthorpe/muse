#pragma pack(1)	//needed for communication with the driver!

typedef struct
{
    short int voice;
    unsigned short int end_idx;		/* end location in ultra DRAM */
    unsigned char rate;			/* 0 to 63 */
    unsigned char mode;			/* mode to run the volume ramp in ... */
} VolumeStruct;

typedef struct
{
    short int voice;
    long speed_khz;
} FreqStruct;

typedef struct
{
    short int voice;
    unsigned char data;
} BalanceStruct;

typedef struct
{
    short int voice;		  /* voice to start */
    unsigned long begin; /* start location in ultra DRAM */
    unsigned long start; /* start loop location in ultra DRAM */
    unsigned long end;	  /* end location in ultra DRAM */
    unsigned char mode;  /* mode to run the voice (loop etc) */
} VoiceStruct;

typedef struct
{
    short int timer;
    unsigned char time;
} TimerStruct;

typedef struct
{
    unsigned long size;
    unsigned long location;
} AllocStruct;

typedef struct
{
    unsigned long size;
    unsigned long location;
} FreeStruct;

typedef struct
{
    long address;
    unsigned char data;
} PokeStruct;

typedef struct
{
    unsigned char control;
    unsigned long dram_loc;
} XferStruct;

typedef struct
{
    long freq;
    short int voice;
    unsigned char balance;
    unsigned short int volume;
    unsigned char rate;
    unsigned char mode;
} AllStruct;


#pragma pack()	//restore original packing
