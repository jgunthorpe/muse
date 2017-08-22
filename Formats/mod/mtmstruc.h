#pragma pack(1)

// Header for each instrument
struct MTMInstrument
{
   char FName[22];                // Instrument file name
   unsigned long SampleLen;       // Size of the sample 
   unsigned long LoopBeg;         // Sample loop point
   unsigned long LoopLen;         // Sample loop length
   unsigned char FineTune;        // Finetune value
   unsigned char Volume;          // Default volume (0->64)
   char Attrib;                   // 0 = 8bit, 1 = 16bit
};

// Main file header
struct MTMHeader
{
   char ID[3];                    // 'MTM'
   unsigned char Version;         // Tracker version
   char Name[20];                 // Song Name
   unsigned short NoTracks;       // Number of tracks saved
   unsigned char LastPattern;     // Last pattern saved
   unsigned char LastOrder;       // Last order to play
   unsigned short CommentLength;  // Length of comment field
   unsigned char NoOfSamples;     // No of samples
   unsigned char Attrib;
   unsigned char BeatsPerTrack;   // Initial beats/track
   unsigned char NoOfTracks;      // Number of tracks to play
   unsigned char PanPos[32];      // Initial Pan Positions
};

#pragma pack()

