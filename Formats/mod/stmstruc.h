#pragma pack(1)

// Header for each instrument
struct STMInstrument
{
   char FName[12];                 // Instrument file name
   char Junk[4];
   unsigned short Length;          // Size of the sample
   unsigned short LoopBeg;         // Sample loop point
   unsigned short LoopLen;         // Sample loop length
   unsigned char Volume;           // Default volume (0->64)
   char Junk2;
   unsigned short FineTune;        // Middle C speed (hz)
   char Junk3[6];
};

// Main file header
struct STMHeader
{
   char Name[20];                  // Song Name
   char Sig[8];                    // Signiature
   char Junk[4];
   unsigned char DefaultTempo;     // Initial song tempo
   unsigned char NoOfPatterns;     // Number of patterns stored
   unsigned char GlobalVolume;     // Overall volume
   char Junk2[13];
   struct STMInstrument Samples[31]; //Samples
   unsigned char Patterns[128];      // Pattern info
};

struct STMRow
{
   unsigned char Note;
   unsigned char VolInst;
   unsigned char VolEffect;
   unsigned char EffectData;
};

struct STMPattern
{
   STMRow Row[64][4];
};

#pragma pack()
