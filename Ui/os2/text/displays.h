/* ########################################################################

   Displays - Various displays

   ########################################################################
*/

#ifndef DISPLAYS_H
#define DISPLAYS_H

#include <string.h>
#include "Screen.h"

class BaseDisplay : public Pane
{
#pragma pack(1)
   struct
   {
      DrawPoint OutputType;
      DrawPoint FileName;
      DrawPoint Mode;
      DrawPoint Type;
      DrawPoint Time;
      DrawPoint Volume;
      int Start, Lines;
      int HMS;
      DrawPoint Location;
   } Locs;
#pragma pack()

   unsigned long TitleLen;
   unsigned long StartTime;

   static void APIENTRY Timer(unsigned long Data);
   static char Run;
   public:

   void DisplayStatus(const char *String) {DrawString(String,Locs.Mode);};
   virtual void SetFileName(DirItem &File,const char *Class);
   virtual void SetModule(museFormatBase *Module);
   virtual void LoopStyle(int Style);
   virtual void SetParms(museEffectFilter *Out);

   BaseDisplay(Screen *Scrn);
   ~BaseDisplay() {Run = 0;};
};

class AnvilScreen : public Pane
{
   public:
   AnvilScreen(Screen *Scrn);
};

class StatScreen : public Pane
{
#pragma pack(1)
   struct
   {
      DrawPoint FileName;
      DrawPoint FileType;
      DrawPoint FileType2;
      DrawPoint Title;
      DrawPoint NumPats;
      DrawPoint NumOrds;
      DrawPoint RowsPerPat;
      DrawPoint NumChans;
      int ComRow;
   } Locs;
#pragma pack()
   int OrgLines;
   public:

   virtual void SetModule(museFormatBase *Module);
   virtual void SetFileName(DirItem &File,const char *Class);
   StatScreen(Screen *Scrn);
};

class WizStatScreen : public Pane
{
#pragma pack(1)
   struct
   {
      DrawPoint FileName;
      DrawPoint FileType;
      DrawPoint FileType2;
      DrawPoint Title;
      DrawPoint NumOrds;
      DrawPoint NumChans;
      DrawPoint ComTop;
      DrawPoint LoopStyle;
      DrawPoint Pitch;
      DrawPoint Speed;
      unsigned long ComRows;
      unsigned long ComBlank;
      DrawPoint Depth;
      DrawPoint Balance;
      DrawPoint Center;
   } Locs;
#pragma pack()
   static void APIENTRY Scroll(unsigned long Data);
   ScreenCell *Comment;
   int ComLines;
   static int CurLine;
   public:

   virtual void SetModule(museFormatBase *Module);
   virtual void SetFileName(DirItem &File,const char *Class);
   virtual void LoopStyle(int Style);
   virtual void SetParms(museEffectFilter *Out);
   virtual void SetPan(museEffectFilter *Out);
   WizStatScreen(Screen *Scrn);
   ~WizStatScreen() {ComLines = -1;CurLine = -1;}
};

class InstScreen : public Pane
{
#pragma pack(1)
   struct
   {
      DrawPoint Name;
      DrawPoint Size;
      DrawPoint Start;
      DrawPoint End;
      DrawPoint Bits;
      DrawPoint Vol;
      DrawPoint Centre;
      unsigned long Bottom;
   } Locs;
#pragma pack()
   public:

   virtual void SetModule(museFormatBase *Module);
   InstScreen(Screen *Scrn);
};

class HelpScreen : public Pane
{
#pragma pack(1)
   struct
   {
      int XStart;
      int XLen;
      int Bottom;
   } Locs;
#pragma pack()

   public:
   HelpScreen(Screen *Scrn);
};

class DACScreen : public Pane
{
#pragma pack(1)
   struct
   {
      DrawPoint Scale;
      int FilterRow;
      int FilterCol;
      int FilterCount;
      int ScaleRow;
      int ScaleCol;
      int ScaleCount;
      DrawPoint IdealScale;
      DrawPoint Lowest;
      DrawPoint AmpFactor;
   } Locs;
#pragma pack()

   static void APIENTRY Timer(unsigned long Data);
   static char Run;
   public:
   virtual void KeyPress(KBDKEYINFO &Key);
   DACScreen(Screen *Scrn);
   ~DACScreen() {Run = 0;};
};

#endif
