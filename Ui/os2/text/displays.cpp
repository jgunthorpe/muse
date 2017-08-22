/* ########################################################################

   Displays - Various displays

   ########################################################################
*/
#ifndef CPPCOMPILE
#ifndef DTS
#include <IDLSequence.xh>
#pragma pack(4)
#include <OutputBs.xh>
#include <FormatBs.xh>
#include <DebugUtils.xh>
#include <DAC.xh>
#include <EfxFiltr.hx>
#pragma pack()
#else
#include <IDLSequence.hh>
#include <OutputBs.hh>
#include <FormatBs.hh>
#include <DebugUtils.hh>
#include <DAC.hh>
#include <EfxFiltr.hh>
#define SOMDelete(x) delete x;
#endif
#else
#include <IDLSeq.hc>
#include <OutputBs.hc>
#include <FormatBs.hc>
#include <DebugUtl.hc>
#include <DAC.hc>
#include <EfxFiltr.hc>
#endif

#include <MinMax.h>
#include <Flags.h>

#define INCL_DOS
#define INCL_KBD
#define INCL_VIO
#include <os2.h>
#include <time.h>

#include "Displays.h"

#include <math.h>
#define abs(x)((x) > 0?(x):-1*(x))
#define round(x)((x) >= 0 ? floor((x)+0.5):ceil((x)-0.5))

char BaseDisplay::Run = 1;
char DACScreen::Run = 1;
int WizStatScreen::CurLine = 0;

/* ########################################################################

   Class - BaseDisplay (Base display)
   Member - Constructor (Inits)

   Just uses the base to load the screen.

   ########################################################################
*/
BaseDisplay::BaseDisplay(Screen *Scr) : Pane(Scr,1)
{
   DBU_FuncTrace("BaseDisplay","Constructor",TRACE_SIMPLE);
   // Get the location table
   void *Ptr;
   if (DosGetResource(0,RT_RCDATA,ID,&Ptr) != 0)
      return;
   memcpy(&Locs,Ptr,sizeof(Locs));
   DosFreeResource(Ptr);
DBU_FuncLineTrace;
   Scr->SetBaseWindow(Locs.Start,Locs.Lines);
   DrawString(Scr->OutputClass->GetTypeName(),Locs.OutputType);
   TitleLen = 0;
   StartTime = 0;

DBU_FuncLineTrace;
   TID Tid;
   if (DosCreateThread(&Tid,&Timer,(unsigned long)this,2,70000) != 0)
      somPrintf("Thread spawn Error\n");
   DosSetPriority(PRTYS_THREAD,PRTYC_IDLETIME,31,Tid);
DBU_FuncLineTrace;
}

/* ########################################################################

   Class - BaseDisplay (Base display)
   Member - SetFileName (Draws the information about the File)

   ########################################################################
*/
void BaseDisplay::SetFileName(DirItem &File,const char *Class)
{
   DBU_FuncTrace("BaseDisplay","SetFileName",TRACE_SIMPLE);

   // Parse out the path
   char *String = File.Name;
   for (const char *C = String + strlen(String); C != String && *C != '/' && *C != '\\';C--);
   if (C != String) C++;

   // Draw the file name
   TitleLen = strlen(C);
   DrawString(C,Locs.FileName);

   // Draw the path
   int Pos = Locs.Location.X;
   DrawString("",Locs.Location);
   if (C != String && *C != ':')
   {
      DrawString(String,Locs.Location.X,Locs.Location.Y,min(Locs.Location.Max,C - String - 1));
      Pos += min(Locs.Location.Max,C - String - 1);
   }
   else
   {
      DrawString(String,Locs.Location.X,Locs.Location.Y,min(Locs.Location.Max,C - String));
      Pos += min(Locs.Location.Max,C - String);
   }
   Pos++;

   // Get the file size
   char S[100];
   if (File.Size > 1024*1024*10)
      sprintf(S,"(%uM)",File.Size/(1024*1024));
   else
      if (File.Size > 1024*10)
         sprintf(S,"(%uK)",File.Size/1024);
      else
         sprintf(S,"(%u Bytes)",File.Size);
   DrawString(S,Pos,Locs.Location.Y,Locs.Location.Max - Pos);

   // Draw the class
   if (Class == 0)
      DrawString("",Locs.Type);
   else
      DrawString(Class,Locs.Type);

   // Draw the song title
   if (File.Title == 0)
      return;

   DrawPoint P = Locs.FileName;
   P.X += TitleLen + 1;
   P.Max -= TitleLen + 1;
   if (strlen(File.Title) < P.Max - 1)
   {
      DrawString("'",P);
      P.X++;
      P.Max--;
      DrawString(File.Title,P);
      P.X += strlen(File.Title);
      P.Max -= max((int)strlen(File.Title),0);
      DrawString("'",P);
   }
}

void BaseDisplay::LoopStyle(int Style)
{
   char *S[3] = {" ","\x1A","\x1D"};
   if (Locs.Type.X > 0)
      DrawString(S[Style],Locs.Type.X + Locs.Type.Max - 1,Locs.Type.Y,1);
}

/* ########################################################################

   Class - BaseDisplay (Base display)
   Member - SetModule (Draws the information about the module on the screen)

   ########################################################################
*/
void BaseDisplay::SetModule(museFormatBase *Module)
{
   DBU_FuncTrace("BaseDisplay","SetModule",TRACE_SIMPLE);
   char *C;

   // Draw the file type
   DrawString(C = Module->GetTypeName(),Locs.Type);
   SOMFree(C);

   // Draw the song title
   DrawPoint P = Locs.FileName;
   P.X += TitleLen + 1;
   P.Max -= TitleLen + 1;
   if (strlen(C = Module->GetTitle()) < P.Max - 1)
   {
      DrawString("'",P);
      P.X++;
      P.Max--;
      DrawString(C,P);
      P.X += strlen(C);
      P.Max -= max((int)strlen(C),0);
      DrawString("'",P);
   }
   StartTime = clock();
   SOMFree(C);
}

/* ########################################################################

   Class - BaseDisplay (Base display)
   Member - Timer (Runs the timer)

   Used to record the time from start

   ########################################################################
*/
void APIENTRY BaseDisplay::Timer(unsigned long Data)
{
   BaseDisplay *cls = (BaseDisplay *)Data;

   cls->StartTime = clock();
   while (BaseDisplay::Run == 1)
   {
      DosSleep(1000);
      if (BaseDisplay::Run == 0)
         return;
      clock_t Now = clock();
      unsigned long Time = (Now - cls->StartTime)/CLOCKS_PER_SEC;

      char S[30];
      if (cls->Locs.HMS == 1)
         sprintf(S,"%02uh%02um%02u",(unsigned int)((Time % (24*60*60))/60/60),
                                 (unsigned int)((Time % (60*60))/60),
                                 (unsigned int)(Time % 60));
      else
         sprintf(S,"%02u:%02u:%02u",(unsigned int)((Time % (24*60*60))/60/60),
                                 (unsigned int)((Time % (60*60))/60),
                                 (unsigned int)(Time % 60));
      cls->DrawString(S,cls->Locs.Time);
   }

}

/* ########################################################################

   Class - BaseDisplay (Base display)
   Member - SetParms (Draws the 3 changable params)

   Draw them on the screen.

   ########################################################################
*/
void BaseDisplay::SetParms(museEffectFilter *Out)
{
   char S[100];
   int NumDots = round(float((Out->_get_Volume()*Locs.Volume.Max)/100));

   // Generate the scale
   for (int I = 0; I != NumDots; I++)
      S[I] = Resource[Locs.Volume.X + Locs.Volume.Y*80].Character;
   S[I] = 0;

   DrawString(S,Locs.Volume);
}

/* ########################################################################

   Class - AnvilScreen (Base display)
   Member - Constructor (Inits)

   Just uses the base to load the screen.

   ########################################################################
*/
AnvilScreen::AnvilScreen(Screen *Scr) : Pane(Scr,4)
{
   Key = 'a';
}

/* ########################################################################

   Class - StatScreen (Base display)
   Member - Constructor (Inits)

   Just uses the base to load the screen.

   ########################################################################
*/
StatScreen::StatScreen(Screen *Scr) : Pane(Scr,2)
{
   DBU_FuncTrace("StatScreen","Constructor",TRACE_SIMPLE);
   Key = 's';

   // Get the location table
   void *Ptr;
   if (DosGetResource(0,RT_RCDATA,ID,&Ptr) != 0)
      return;
   memcpy(&Locs,Ptr,sizeof(Locs));
   DosFreeResource(Ptr);
   OrgLines = Lines;
   Lines -= 2;

   Data = (ScreenCell *)SOMCalloc(25*80*2,sizeof(ScreenCell));
   memcpy(Data,Resource,Size*sizeof(ScreenCell));
   for (int I = Locs.ComRow;I != 49; I++)
      memcpy(Data + I*80,Resource + Locs.ComRow*80,80*2);
}

/* ########################################################################

   Class - StatScreen (Base display)
   Member - SetModule (Draws the information about the module on the screen)

   ########################################################################
*/
void StatScreen::SetModule(museFormatBase *Module)
{
   DBU_FuncTrace("StatScreen","SetModule",TRACE_SIMPLE);
//   char *CTypeName = ((museFormatClass*)Module->somGetClass())->GetTypeName();
   char *CTypeName = Module->GetTypeName();
   char *TypeName = Module->GetTypeName();

   // Check if they are the same string
   if (strcmp(TypeName,CTypeName) == 0)
   {
      DrawString(TypeName,Locs.FileType);
      DrawString("",Locs.FileType2);
   }
   else
   {
      DrawString(CTypeName,Locs.FileType);
      DrawString(TypeName,Locs.FileType2);
   }
   SOMFree(TypeName);

   char *C;
   DrawString(C = Module->GetTitle(),Locs.Title);
   SOMFree(C);
   DrawInt(Module->GetNumPatterns(),Locs.NumPats);
   DrawInt(Module->GetNumOrders(),Locs.NumOrds);
   DrawInt(Module->GetRowsAPattern(),Locs.RowsPerPat);
   int Chans = Module->GetNumChannels();
   if (Chans == 0)
      Chans = Scr->Output->_get_MaxChannels();
   DrawInt(Chans,Locs.NumChans);

   // Get the comment info
   Lines = OrgLines - 2;
   string Comment;
   Module->GetSongComment(&Comment);
   if (Comment != 0)
   {
      char *C = Comment;
      unsigned int Lines = 1;
      char *Start = C;
      for (;*C != 0; C++)
      {
         if (*C == '\n')
         {
            *C = 0;
           this->Lines = OrgLines + Lines - 1;
            DrawString(Start,0,Locs.ComRow + Lines - 1,80);
            *C = '\n';
            Lines++;
            Start = C + 1;
         }
      }
      SOMFree(Comment);
   }
   Scr->ReDisplay(this);
}

/* ########################################################################

   Class - StatScreen (Base display)
   Member - SetModule (Draws the information about the module on the screen)

   ########################################################################
*/
void StatScreen::SetFileName(DirItem &File,const char *)
{
   DrawString(File.Name,Locs.FileName);
}

/* ########################################################################

   Class - InstScreen (Base display)
   Member - Constructor (Inits)

   Just uses the base to load the screen.

   ########################################################################
*/
InstScreen::InstScreen(Screen *Scr) : Pane(Scr,3)
{
   DBU_FuncTrace("InstScreen","Constructor",TRACE_SIMPLE);
   Key = 'i';

   // Get the location table
   void *Ptr;
   if (DosGetResource(0,RT_RCDATA,ID,&Ptr) != 0)
      return;
   memcpy(&Locs,Ptr,sizeof(Locs));
   DosFreeResource(Ptr);
}

/* ########################################################################

   Class - StatScreen (Base display)
   Member - SetModule (Draws the information about the module on the screen)

   ########################################################################
*/
void InstScreen::SetModule(museFormatBase *Module)
{
   DBU_FuncTrace("InstScreen","SetModule",TRACE_SIMPLE);
   // Get the instruments
   SequenceSample Samp;
   Module->GetSongSamples(Samp);

   unsigned long Length = min((long)(Samp[0].MaxNameLen),(long)Locs.Name.Max);
   unsigned long Length2 = min((long)(Length + Samp[0].MaxSNameLen + 1),(long)Locs.Name.Max) - Length - 1;

   if (Data != Resource)
      SOMFree(Data);
   Data = (ScreenCell *)SOMCalloc((Samp.size() + 3)*80,sizeof(ScreenCell));
   memcpy(Data,Resource,80*sizeof(ScreenCell));

   Lines = 0;
   int Size = Samp.size();
   for (int I = Samp.size(); I != 0;)
   {
      I--;
      if (Samp[I].Sample == 0 && (Samp[I].Name == 0 || Samp[I].Name[0] == 0))
         Size--;
      else
         break;
   }

   int Line = 1;
   for (I = 0; I != Size;)
   {
      memcpy(Data+Line*80,Resource+80,80*sizeof(ScreenCell));
      if ((Samp[I].Flags & (1 << 31)) == 0)
         DrawString(Samp[I].Name,Locs.Name.X,Line,Length);
      else
      {
         char N[100];
         N[26] = 0;
         for (int I2 = 0;I2 != Length;I2++)
         {
            N[I2] = Samp[I].Name[I2];
            if (N[I2] == 0)
               N[I2] = ' ';
         }
         DrawString(N,Locs.Name.X,Line,Length);
      }

      int OldLine = Line;
      int InstNo = Samp[I].InstNo;
      for (int SubInst = 0;Samp[I].InstNo == InstNo; SubInst++,Line++,I++)
      {
         // Skip 0 subsamples
         if (Samp[I].Sample == 0)
         {
            int Blank = 0;
            if (Samp[I].SampleName != 0)
            {
               for (int I3 = 0; I3 != Length2 && Samp[I].SampleName[I3] == ' ' && Samp[I].SampleName[I3] != 0;I3++);
               if (I3 != 0)
               {
                  if (I3 == Length)
                     Blank = 1;
                  if (Samp[I].SampleName[I3 - 1] == 0)
                     Blank = 1;
               }
            }

            if (Blank == 1)
            {
               SubInst--;
               Line--;
               continue;
            }
         }

         // Draw the background
         if (Line != OldLine)
            memcpy(Data+Line*80,Resource+80,80*sizeof(ScreenCell));

         if (SubInst == 1)
            DrawString("\xDA",Locs.Name.X + Length,Line-1,1);

         if (SubInst > 0)
            DrawString("\xB3",Locs.Name.X + Length,Line,1);

         DrawString(Samp[I].SampleName,Locs.Name.X + Length + 1,Line,Length2);

         if (Samp[I].Sample == 0)
            continue;

         DrawInt(Samp[I].SampleEnd,Locs.Size.X,Line,Locs.Size.Max);

         // Only draw loop info if it's looped.
         if ((Samp[I].Flags & (1 << 0)) != 0)
         {
            DrawInt(Samp[I].LoopBegin,Locs.Start.X,Line,Locs.Start.Max);
            DrawInt(Samp[I].LoopEnd,Locs.End.X,Line,Locs.End.Max);
         }

         // Draw the bit rate
         if ((Samp[I].Flags & (1 << 1)) != 0)
            DrawString("16",Locs.Bits.X,Line,Locs.Bits.Max);
         else
            DrawString(" 8",Locs.Bits.X,Line,Locs.Bits.Max);

         char C[10];
         sprintf(C,"%X",(unsigned long)Samp[I].Volume);
         DrawString(C,Locs.Vol.X,Line,Locs.Vol.Max);
         DrawInt(Samp[I].Center,Locs.Centre.X,Line,Locs.Centre.Max);
      }

      // Entire thing was empty
      if (OldLine == Line)
         Line++;

      if (SubInst > 1)
         DrawString("\xC0",Locs.Name.X + Length,Line-1,1);
   }
   memcpy(Data+(Line+1)*80,Resource+80*2,80*sizeof(ScreenCell));
   Lines = Line;
   Bottom = Locs.Bottom;
   Scr->ReDisplay(this);
}

/* ########################################################################

   Class - HelpScreen (Base display)
   Member - Constructor (Inits)

   Just uses the base to load the screen.

   ########################################################################
*/
HelpScreen::HelpScreen(Screen *Scr) : Pane(Scr,5)
{
   DBU_FuncTrace("HelpScreen","Constructor",TRACE_SIMPLE);
   Key = 'h';

   // Get the location table
   void *Ptr;
   if (DosGetResource(0,RT_RCDATA,ID,&Ptr) != 0)
      return;
   memcpy(&Locs,Ptr,sizeof(Locs));
   DosFreeResource(Ptr);

   Data = (ScreenCell *)SOMCalloc(300*80,sizeof(ScreenCell));
   memcpy(Data,Resource,Size*sizeof(ScreenCell));
   for (int I = Lines - Locs.Bottom;I != 199; I++)
      memcpy(Data + I*80,Resource + (Lines - 1 - Locs.Bottom)*80,80*2);

   PVOID Res;
   APIRET Rc;
   unsigned long Size;

   // Read resource
   Rc = DosGetResource(0,1,99,&Res);
   DosQueryResourceSize(0,1,99,&Size);

   if (Rc != 0)
      return;

   // Display the text
   int Start = 0;
   int Left = Size;
   for (int Lines = 0;;)
   {
      int Amount = 0;
      for (;Left != 0 && ((char *)Res)[Amount+Start] != '\r' && ((char *)Res)[Amount+Start] != '\n';Amount++,Left--);

      DrawString(((char *)Res) + Start,Locs.XStart,Lines + this->Lines - 1,min(Amount,Locs.XLen));

      Amount++;
      Left--;
      if (((char *)Res)[Amount+Start] == '\n')
      {
         Amount++;
         Left--;
      }

      Lines++;
      Start += Amount;

      if (Left == 0)
         break;
   }
   DosFreeResource(Res);
   Lines++;

   // Display output system help
   SequenceString Seq;
   Seq.construct();
   Seq.reserve(100);
   SequenceOutputClass &Outputs = Scr->Outputs;
   for (I = 0; I != Outputs.size(); I++)
   {
      Seq.erasefrom(Seq.begin());
      Outputs[I]->GetOptionHelp(&Seq);

      if (Seq.size() != 0)
      {
         DrawString("Options for ",Locs.XStart,Lines + this->Lines - 1,min(12,Locs.XLen));
         DrawString(Outputs[I]->GetTypeName(),Locs.XStart+12,Lines + this->Lines - 1,Locs.XLen - 12);
         Lines++;
         DrawString(" -o",Locs.XStart,Lines + this->Lines - 1,min(5,Locs.XLen));

         for (char **Cur = Seq.begin(); Cur != Seq.end(); Cur++)
         {
            char *C = *Cur;
            while (*C != 0)
            {
               char *Start = C;
               for (; *C != 0 && *C != '\n'; C++);
               if (Start == *Cur)
                  DrawString(Start,Locs.XStart + 4,Lines + this->Lines - 1,min(C - Start,Locs.XLen));
               else
                  DrawString(Start,Locs.XStart,Lines + this->Lines - 1,min(C - Start,Locs.XLen));
               Lines++;
               if (*C != 0)
                  C++;
            }
         }
      }
      Lines++;
   }
   Seq.free();

   memcpy(Data+(this->Lines + Lines + 1)*80,Resource+(this->Lines - 1)*80,80*sizeof(ScreenCell));
   this->Lines += Lines;
   Bottom = Locs.Bottom;
}

/* ########################################################################

   Class - WizStatScreen (Base display)
   Member - Constructor (Inits)

   Just uses the base to load the screen.

   ########################################################################
*/
WizStatScreen::WizStatScreen(Screen *Scr) : Pane(Scr,2)
{
   DBU_FuncTrace("WizStatScreen","Constructor",TRACE_SIMPLE);
   Key = 's';

   // Get the location table
   void *Ptr;
   if (DosGetResource(0,RT_RCDATA,ID,&Ptr) != 0)
      return;
   memcpy(&Locs,Ptr,sizeof(Locs));
   DosFreeResource(Ptr);
   ComLines = 0;
   CurLine = 0;

   Comment = (ScreenCell *)SOMCalloc(50*Locs.ComTop.Max,sizeof(ScreenCell));

   TID Tid;
   if (DosCreateThread(&Tid,&Scroll,(unsigned long)this,2,70000) != 0)
      somPrintf("Thread spawn Error\n");
   DosSetPriority(PRTYS_THREAD,PRTYC_IDLETIME,31,Tid);
   Dup();
}

/* ########################################################################

   Class - WizStatScreen (Base display)
   Member - SetModule (Draws the information about the module on the screen)

   ########################################################################
*/
void WizStatScreen::SetModule(museFormatBase *Module)
{
   DBU_FuncTrace("WizStatScreen","SetModule",TRACE_SIMPLE);
//   char *CTypeName = ((museFormatClass*)Module->somGetClass())->GetTypeName();
   char *CTypeName = Module->GetTypeName();
   char *TypeName = Module->GetTypeName();

   // Check if they are the same string
   if (strcmp(TypeName,CTypeName) == 0)
   {
      DrawString(TypeName,Locs.FileType);
      DrawString("",Locs.FileType2);
   }
   else
   {
      DrawString(CTypeName,Locs.FileType);
      DrawString(TypeName,Locs.FileType2);
   }
   SOMFree(TypeName);

   char *C;
   DrawString(C = Module->GetTitle(),Locs.Title);
   SOMFree(C);
   DrawInt(Module->GetNumOrders(),Locs.NumOrds);
   int Chans = Module->GetNumChannels();
   if (Chans == 0)
      Chans = Scr->Output->_get_MaxChannels();
   DrawInt(Chans,Locs.NumChans);

   // Get the comment info
   string Comment;
   Module->GetSongComment(&Comment);

   // Blank the comment thingy
   ComLines = 0;
   CurLine = 0;
   for (int I = 0; I != 49; I++)
      memcpy(this->Comment+I*Locs.ComTop.Max,
             Resource+Locs.ComTop.X+Locs.ComBlank*80,
             Locs.ComTop.Max*sizeof(ScreenCell));
   if (Comment != 0)
   {
      char *C = Comment;
      unsigned int Lines = 1;
      char *Start = C;
      for (;*C != 0; C++)
      {
         if (*C == '\n')
         {
            *C = 0;
            ScreenCell *S = this->Comment + (Lines - 1)*Locs.ComTop.Max;
            ScreenCell *E = this->Comment + Lines*Locs.ComTop.Max;
            for (;Start != C && S != E; Start++,S++)
               S->Character = *Start;
            *C = '\n';
            Lines++;
            Start = C + 1;
         }
      }
      SOMFree(Comment);
      ComLines = Lines;
      CurLine = 0;
      DrawBuffer(this->Comment,Locs.ComTop.Max,Locs.ComRows,Locs.ComTop.X,Locs.ComTop.Y);
   }
   else
   {
      for (int I = 0; I != Locs.ComRows; I++)
         memcpy(Data + 80*(Locs.ComTop.Y+I) + Locs.ComTop.X,
                Resource+80*(Locs.ComTop.Y+I) + Locs.ComTop.X,
                sizeof(ScreenCell)*Locs.ComTop.Max);
      ComLines = 0;
      CurLine = 0;
   }
   Scr->ReDisplay(this);
}

/* ########################################################################

   Class - WizStatScreen (Base display)
   Member - SetModule (Draws the information about the module on the screen)

   ########################################################################
*/
void WizStatScreen::SetFileName(DirItem &File,const char *)
{
   DrawString(File.Name,Locs.FileName);
}

/* ########################################################################

   Class - WizStatScreen (Base display)
   Member - LoopStyle (Draws the loop Style Text)

   ########################################################################
*/
void WizStatScreen::LoopStyle(int Style)
{
   char *S[3] = {"Normal","None","Repeat"};
   if (Style > 2)
      return;
   DrawString(S[Style],Locs.LoopStyle);
}

/* ########################################################################

   Class - WizStatScreen (Base display)
   Member - SetParms (Draws the Pitch and Speed)

   ########################################################################
*/
void WizStatScreen::SetParms(museEffectFilter *Out)
{
   char S[100];
   sprintf(S,"%u%%",(unsigned long)Out->_get_Pitch());
   DrawString(S,Locs.Pitch);

   sprintf(S,"%u%%",(unsigned long)Out->_get_Speed());
   DrawString(S,Locs.Speed);
}

/* ########################################################################

   Class - WizStatScreen (Base display)
   Member - SetPan (Updates the pan info display)

   Changes the pan info display.

   ########################################################################
*/
void WizStatScreen::SetPan(museEffectFilter *Out)
{
   long Depth = Out->_get_PanDepth();
   long Center = Out->_get_PanCenter();
   long Balance = Out->_get_PanBalance();
   somPrintf("%d,%d,%d\n",Depth,Center,Balance);
   DrawPoint P = Locs.Depth;
   memcpy(Data+P.X+P.Y*80,Resource+P.X+P.Y*80,(P.Max + 1)*sizeof(ScreenCell));

   ScreenCell C = Resource[P.X + P.Y*80 - 1];
   C.Character = 'R';

   int X1 = round((float)(Depth*(P.Max/2))/PanSpan);
   int X2 = round((float)(-1*Depth*(P.Max/2))/PanSpan);
   somPrintf("WANK %d,%d\n",X1,X2);

   // Draw the two items, - depth cause inversion..
   if (X1 == X2)
   {
      C.Character = 'M';
      Data[P.X + X1 + P.Max/2 + P.Y*80] = C;
   }
   else
   {
      C.Character = 'R';
      Data[P.X + X1 + P.Max/2 + P.Y*80] = C;
      C.Character = 'L';
      Data[P.X + X2 + P.Max/2 + P.Y*80] = C;
   }

   // Draw center
   P = Locs.Center;
   memcpy(Data+P.X+P.Y*80,Resource+P.X+P.Y*80,(P.Max+1)*sizeof(ScreenCell));

   X1 = round((float)(Center*(P.Max/2))/PanMax);
   C.Character = 'I';
   Data[P.X + X1 + P.Max/2 + P.Y*80] = C;

   // Draw balance
   P = Locs.Balance;
   memcpy(Data+P.X+P.Y*80,Resource+P.X+P.Y*80,(P.Max+1)*sizeof(ScreenCell));

   X1 = round((float)(Balance*(P.Max/2))/PanMax);
   C.Character = 'I';
   Data[P.X + X1 + P.Max/2 + P.Y*80] = C;

   Scr->ReDisplay(this);
}

/* ########################################################################

   Class - BaseDisplay (Base display)
   Member - Timer (Runs the timer)

   Used to record the time from start

   ########################################################################
*/
void APIENTRY WizStatScreen::Scroll(unsigned long Data)
{
   WizStatScreen *cls = (WizStatScreen *)Data;

   while (CurLine != -1)
   {
      if (cls->ComLines == -1 || CurLine == -1)
         return;
      if (cls->ComLines == 0 || cls->ComLines < cls->Locs.ComRows)
      {
         DosSleep(2000);
         continue;
      }
      DosSleep(2000);

      if (cls->ComLines == 0 || cls->ComLines < cls->Locs.ComRows)
         continue;

      if (CurLine >= cls->ComLines)
         CurLine = 0;

      cls->DrawBuffer(cls->Comment + CurLine*cls->Locs.ComTop.Max,
                 cls->Locs.ComTop.Max,min((int)cls->Locs.ComRows,cls->ComLines - CurLine),
                 cls->Locs.ComTop.X,cls->Locs.ComTop.Y);
      if (cls->ComLines - CurLine < cls->Locs.ComRows)
         cls->DrawBuffer(cls->Comment,
                    cls->Locs.ComTop.Max,cls->Locs.ComRows - (cls->ComLines - CurLine),
                    cls->Locs.ComTop.X,cls->Locs.ComTop.Y + (cls->ComLines - CurLine));

      CurLine++;
   }
}

/* ########################################################################

   Class - DACScreen (Digital Mixer control)
   Member - Constructor (Inits)

   Just uses the base to load the screen.

   ########################################################################
*/
DACScreen::DACScreen(Screen *Scr) : Pane(Scr,6)
{
   DBU_FuncTrace("DACScreen","Constructor",TRACE_SIMPLE);
   Key = 'd';

   // Get the location table
   void *Ptr;
   if (DosGetResource(0,RT_RCDATA,ID,&Ptr) != 0)
      return;
   memcpy(&Locs,Ptr,sizeof(Locs));
   DosFreeResource(Ptr);

   long P = ((museDACMixer *)Scr->Output)->_get_ScalePercent();
   char S[100];
   sprintf(S,"%u%%",(unsigned long)P);
   DrawString(S,Locs.Scale);

   P = ((museDACMixer *)Scr->Output)->_get_DAmpType();
   DrawString("\xFA",Locs.ScaleCol,Locs.ScaleRow,1);
   P = P % Locs.ScaleCount;
   DrawString("\x04",Locs.ScaleCol,Locs.ScaleRow+P,1);

   P = ((museDACMixer *)Scr->Output)->_get_CurFilter();
   DrawString("\xFA",Locs.FilterCol,Locs.FilterRow,1);
   P = P % Locs.FilterCount;
   DrawString("\x04",Locs.FilterCol,Locs.FilterRow+P,1);

   P = ((museDACMixer *)Scr->Output)->_get_DAmpFactor();
   sprintf(S,"%u",(unsigned long)(100-P));
   DrawString(S,Locs.AmpFactor);

   TID Tid;
   Run = 1;
   if (DosCreateThread(&Tid,&Timer,(unsigned long)this,2,70000) != 0)
      somPrintf("Thread spawn Error\n");
   DosSetPriority(PRTYS_THREAD,PRTYC_IDLETIME,31,Tid);
}

void DACScreen::KeyPress(KBDKEYINFO &Key)
{
   switch (Key.chChar)
   {
      case '/':
      {
         long P = ((museDACMixer *)Scr->Output)->_get_ScalePercent();

         if (P > 5)
            P -= 5;
         ((museDACMixer *)Scr->Output)->_set_ScalePercent(P);

         char S[100];
         sprintf(S,"%u%%",(unsigned long)P);
         DrawString(S,Locs.Scale);
         break;
      }

      case '*':
      {
         long P = ((museDACMixer *)Scr->Output)->_get_ScalePercent();

         if (P <= 6000)
            P += 5;
         ((museDACMixer *)Scr->Output)->_set_ScalePercent(P);

         char S[100];
         sprintf(S,"%u%%",(unsigned long)P);
         DrawString(S,Locs.Scale);
         break;
      }

      case '`':
      {
         long P = ((museDACMixer *)Scr->Output)->_get_DAmpFactor();

         if (P < 100)
            P++;
         ((museDACMixer *)Scr->Output)->_set_DAmpFactor(P);

         char S[100];
         sprintf(S,"%u",(unsigned long)(100-P));
         DrawString(S,Locs.AmpFactor);
         break;
      }

      case '~':
      {
         long P = ((museDACMixer *)Scr->Output)->_get_DAmpFactor();

         if (P > 0)
            P--;
         ((museDACMixer *)Scr->Output)->_set_DAmpFactor(P);

         char S[100];
         sprintf(S,"%u",(unsigned long)(100-P));
         DrawString(S,Locs.AmpFactor);
         break;
      }

      // Match to ideal
      case 'm':
         ((museDACMixer *)Scr->Output)->_set_ScalePercent(((museDACMixer *)Scr->Output)->_get_ComputedSP());
         break;

      // Match to lowest (and switch to downwards)
      case 'M':
      {
         ((museDACMixer *)Scr->Output)->_set_ScalePercent(((museDACMixer *)Scr->Output)->_get_DAmpLowest());

         long P = ((museDACMixer *)Scr->Output)->_get_DAmpType();
         DrawString("\xFA",Locs.ScaleCol,Locs.ScaleRow+P,1);
         P = 3;
         ((museDACMixer *)Scr->Output)->_set_DAmpType(P);
         DrawString("\x04",Locs.ScaleCol,Locs.ScaleRow+P,1);
         break;
      }

   }
   somPrintf("Key : %u Shift : %u\n",(int)Key.chScan,(int)Key.fsState);
   switch (Key.chScan)
   {
      // F9
      case 92:
      case 67:
      {
         long P = ((museDACMixer *)Scr->Output)->_get_CurFilter();

         DrawString("\xFA",Locs.FilterCol,Locs.FilterRow+P,1);
         if ((Key.fsState & 3) != 0)
            P = (P - 1) % Locs.FilterCount;
         else
            P = (P + 1) % Locs.FilterCount;
         if (P < 0)
            P = Locs.FilterCount - 1;
         ((museDACMixer *)Scr->Output)->SetFilter(P);
         DrawString("\x04",Locs.FilterCol,Locs.FilterRow+P,1);
         break;
      }

      // F10
      case 93:
      case 68:
      {
         long P = ((museDACMixer *)Scr->Output)->_get_DAmpType();

         DrawString("\xFA",Locs.ScaleCol,Locs.ScaleRow+P,1);
         if ((Key.fsState & 3) != 0)
            P = (P - 1) % Locs.ScaleCount;
         else
            P = (P + 1) % Locs.ScaleCount;
         if (P < 0)
            P = Locs.ScaleCount - 1;
         ((museDACMixer *)Scr->Output)->_set_DAmpType(P);
         DrawString("\x04",Locs.ScaleCol,Locs.ScaleRow+P,1);
         break;
      }

   }
}

/* ########################################################################

   Class - BaseDisplay (Base display)
   Member - Timer (Runs the timer)

   Used to record the time from start

   ########################################################################
*/
void APIENTRY DACScreen::Timer(unsigned long Data)
{
   DACScreen *cls = (DACScreen *)Data;

   long LastP = 0;
   while (DACScreen::Run == 1)
   {
      DosSleep(1000);
      if (DACScreen::Run == 0)
         return;

      long P = ((museDACMixer *)cls->Scr->Output)->_get_ComputedSP();
      if (P == LastP) continue;
      LastP = P;

      char S[100];
      sprintf(S,"%u%%",(unsigned long)P);
      cls->DrawString(S,cls->Locs.IdealScale);

      sprintf(S,"%u%%",((museDACMixer *)cls->Scr->Output)->_get_DAmpLowest());
      cls->DrawString(S,cls->Locs.Lowest);

      sprintf(S,"%u%%",((museDACMixer *)cls->Scr->Output)->_get_ScalePercent());
      cls->DrawString(S,cls->Locs.Scale);
   }
}

