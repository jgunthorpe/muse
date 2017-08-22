#pragma pack(1)

// Header for each instrument
struct MODInstrument
{
   char FName[22];                 // Instrumen file name
   unsigned short SampleWords;     // Size of the sample / 2
   char FineTune;                  // Finetune value (nibble)
   unsigned char Volume;           // Default volume (0->64)
   unsigned short LoopBeg;         // Sample loop point
   unsigned short LoopLen;         // Sample loop length
};

// Main file header
struct MODHeader
{
   char Name[20];                  // Song Name
   MODInstrument Samples[31];      // Samples
   unsigned char OrderNum;         // # of orders
   unsigned char RestartPos;       // Restart position, very rare
   unsigned char Patterns[128];    // Pattern info
   char ID[4];                     // ID
};

#pragma pack()

// Bind the length to the pattern data
struct MODBoundPattern
{
   unsigned short Length;
   unsigned char *Pattern;
};

