/* ########################################################################

   Screen - Class that represents the screen

   ########################################################################
*/

#ifndef SCREEN_H
#define SCREEN_H

#ifndef CPPCOMPILE
#ifndef DTS
#include <HandLst.xh>
#else
#include <HandLst.hh>
#endif
#else
#include <HandLst.hc>
#include <FormatBs.hc>
#include <OutputBs.hc>
#endif

#include <stdio.h>
#include "SongList.h"

class museEffectFilter;

#pragma pack(1)
typedef struct
{
   char Character;
   unsigned char Colour;
} ScreenCell;

typedef struct
{
   int X;
   int Y;
   int Max;
} DrawPoint;
#pragma pack()

class Screen;
class Pane
{
   protected:
   ScreenCell *Resource;
   ScreenCell *Data;
   unsigned long Size;
   unsigned long ID;
   Screen *Scr;

   public:
   unsigned short Lines;
   int Key;
   unsigned long Protect;
   char Bottom;

   ScreenCell *GetBuffer() {return Data;};
   unsigned long GetSize() {return Lines*80;};
   void Dup();

   void DrawString(const char *String,int X,int Y,int Length);
   void DrawString(const char *String,DrawPoint P)
   {
      DrawString(String,P.X,P.Y,P.Max);
   };
   void DrawInt(unsigned long I,int X,int Y,int Max)
   {
      char S[80];
      sprintf(S,"%u",I);
      DrawString(S,X,Y,Max);
   }
   void DrawInt(unsigned long I,DrawPoint P)
   {
      DrawInt(I,P.X,P.Y,P.Max);
   };
   void DrawBuffer(ScreenCell *B,int XLen, int YLen,int X,int Y);

   virtual void SetModule(museFormatBase *Module) {};
   virtual void SetFileName(DirItem &File,const char *Class) {};
   virtual void KeyPress(KBDKEYINFO &Key) {};
   virtual void LoopStyle(int Style) {};
   virtual void SetParms(museEffectFilter *Out) {};
   virtual void SetPan(museEffectFilter *Out) {};
   Pane(Screen *Scr,unsigned long ID);
   virtual ~Pane();
};

class BaseDisplay;
class Screen
{
   SongList *Songs;
   museHandlerList *Handlers;
   boolean HiPrio;
   boolean AllowLoop;
   boolean Server;
   boolean RealTime;
   float DefVolume;
   int LoopStyle;
   char *OutputOptions;
   char *DeviceDesc;
   unsigned long PlayTid;
   int SongListLoop;

   ScreenCell *OldScreen;
   unsigned short OldX;
   unsigned short OldY;
   unsigned short Rows;
   unsigned short Cols;

   BaseDisplay *BaseDisplay;
   Pane *CurrentDisplay;
   unsigned short BaseStartLine;
   unsigned short BaseLines;
   unsigned short ScrollLoc;
   char *FileName;

   DirItem Song;
   int NextSong;
   int CurSong;
   museFormatBase *Module;

   IDLSequence<Pane *> Displays;

   char SpawnType;
   static void APIENTRY SpawnThreads(unsigned long Data);

   char Exit;
   char Detached;
   unsigned long KeyProtect;
   char KeyDelay;
   void Keyboard();

   HEV BeginPlay;
   HEV DonePlay;
   char Changed;
   char Playing;
   void SongChange();
   void PlayThread();

   unsigned long PipeHandle;
   void PipeThread();

   unsigned long ServerWait;

   VIOCURSORINFO Cursor;
   USHORT OAttrib;

   char *StdError;
   void StdErrThread();

   public:
   static FILE *DebugFile;
   int UIBase;

   museOutputClass *OutputClass;
   museOutputBase *Output;
   museEffectFilter *EfxOut;
   SequenceOutputClass Outputs;
   SequenceFormatClass FileFormats;

   void DrawString(int X,int Y,int Length,Pane *Display);
   void Display(Pane *Display);
   void ReDisplay(Pane *W = 0) {if (W == 0 || W == CurrentDisplay) Display(CurrentDisplay);};

   void SetBaseWindow(unsigned short Start,unsigned short Lines)
   {
      BaseStartLine = Start;
      BaseLines = Lines;
   };
   int HandleCommandLine(char argc,char *argv[]);
   long AddDllsInDir(string Dir, string Pattern);
   int Help();
   int Init();
   void Play();

   Screen();
   ~Screen();
};

#endif

