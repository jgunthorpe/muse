#pragma pack(1)
// Main file header
struct S3MHeader
{
   char Name[28];                  // Song Name
   unsigned char Jnk1;             // Empty
   unsigned char Type;             // Type of song
   unsigned short Jnk2;            // Empty
   unsigned short OrderNum;        // # of orders
   unsigned short InstNum;         // # of instruments
   unsigned short PatNum;          // # of patterns
   unsigned short Flags;           // Global song flags
   unsigned short Editor;          // Editor Key
   unsigned short FileFormatInfo;  // More flags
   char ID[4];                     // ID
   unsigned char GlobalVolume;     // Global volume 0->40
   unsigned char InitialSpeed;     // Initial Speed (Frames/sec)
   unsigned char InitialTempo;     // Initial Tempo (BPM)
   unsigned char MasterVolume;     // Master Volume (?)
   unsigned char UltraClick;       // Enable UltraClick removale
   unsigned char DefaultPan;       // Indicates if Default Pan positions are present
   unsigned long Jnk3;             // Empty
   unsigned long Jnk4;
   unsigned short Special;         // Special info
   unsigned char ChannelF[32];     // Channel flags
};

// Header for each instrument
struct S3MInsterment
{
   unsigned char Type;             // Instrument type
   char FName[12];                 // Instrumen file name
   unsigned char MemSeg1;          // Parapointer to sample (unused)
   unsigned short MemSeg2;
   unsigned long Length;           // Size of the sample
   unsigned long LoopBeg;          // Sample loop point
   unsigned long LoopEnd;          // End of sample
   unsigned char Volume;           // Default volume (0->64)
   unsigned char Jnk1;             // Empty
   unsigned char Pack;             // ??
   unsigned char Flags;            // Sample flags
   unsigned long C2Speed;          // Middle note Hz
   unsigned long Jnk2;             // Empty
   unsigned long Jnk3;
   unsigned long Jnk4;
   char Name[28];                  // Instrument name
   char ID[4];                     // ID code
};
#pragma pack()

// Bind the header and the sample together
struct S3MBoundInst
{
   S3MInsterment Inst;
   unsigned char *Sample;
};

// Bind the length to the pattern data
struct S3MBoundPattern
{
   unsigned short Length;
   unsigned char *Pattern;
};

struct CmdInfoS3M 
{
   long Period;
   unsigned char RowDelay;
   unsigned long DelayFlags;
   long PortTarget;
   unsigned long VibratoPoint;
   signed short *VibTable;
   unsigned long TremoloPoint;
   signed short *TrmoTable;
   unsigned char VibSize;
   unsigned char VibratoSpeed;
   unsigned char TrmoSize;
   unsigned char TremoloSpeed;
   signed long BasePeriod;
   signed short BaseVolume;
   signed short MainVol;
   unsigned char TremorFlipFlop;
   unsigned char TremorCounter;
   char RetrigCounter;
   char Info;
   float ArpeggioCounter;
   long GInfo;
   char HInfo;
   unsigned char Waste[3];
};
