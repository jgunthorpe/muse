#include <Muse.h>
#include <HandLst.hc>

#include <S3MForm.hc>
#include <MODForm.hc>
#include <MTMForm.hc>
#include <STMForm.hc>
#include <XMForm.hc>
#include <ITForm.hc>
#include <FARForm.hc>
#include <669Form.hc>
#include <ULTForm.hc>
#include <PTMForm.hc>
#include <DMFForm.hc>
#include <WAV.hc>
#include <RAW.hc>
#include <AU.hc>
#include <GUSMixer.hc>
#include <DART.hc>
#include <MMPM2.hc>
#include <EFXFiltr.hc>
#include <TimeBuf.hc>
#include <iostream.h>
#include <stdio.h>

#define INCL_DOS
#define INCL_VIO
#define INCL_KBD
#define INCL_DOSERRORS
#include <os2.h>

void APIENTRY TimeBuffer(unsigned long Data)
{
   museTimeBuffer &Buf = *((museTimeBuffer *)Data);
   museFrameInfo Frame;
   SequenceChannel Chan;
   Chan.construct();
   SequenceChannel Chan2;
   Chan2.construct();
   char S[200];
   while (1)
   {
      Buf.GetNextFrame(&Frame,&Chan);
      Chan2.reserve(Chan.size());
      Chan2.erasefrom(Chan2.begin());

      for (int CurChan = 0; CurChan < Chan.size(); CurChan++)
      {
         for (museChannel *I = Chan.begin(); I != Chan.end(); I++)
            if (I->ModuleChannel == CurChan)
               Chan2.push_back(*I);
      }

      unsigned long Time = Buf.LenToSeconds(Frame.Time);
      unsigned long Time2 = Buf.LenToSeconds(Frame.SongTime);
      sprintf(S," Ord %2u Row %2u Pat %2u GVol %2u Tempo %3u Spd %2u Chns %2u %2u:%2u:%2u %2u:%2u:%2u ",
         Frame.Order,Frame.Row,Frame.Pattern,Frame.GlobalVolume,Frame.Tempo,
         Frame.Speed,Frame.PlayingChannels,
         (Time/3600) % 60,(Time/60) % 60,Time % 60,
         (Time2/3600) % 60,(Time2/60) % 60,Time2 % 60);
      VioWrtCharStr(S,strlen(S),0,0,0);

      SequenceChannel::iterator I = Chan2.begin();
      unsigned long Line = 0;
      for (;I != Chan2.end(); I++, Line++)
      {
         if ((I->Flags & CHAN_Free) == 0)
         {
            sprintf(S," %2d Pan %6d Pitch %6u Sample %2u Vol %6d ",I->ModuleChannel,
               I->Pan,I->Pitch,I->Sample,I->MainVol);
            VioWrtCharStr(S,strlen(S),1+Line,0,0);
         }
         else
         {
            char A = ' ';
            VioWrtNChar(&A,80,1+Line,0,0);
         }
      }
   }
}

musePlayerControl Play;
void APIENTRY Keyboard(unsigned long Data)
{
   museEffectFilter &Filter = *((museEffectFilter *)Data);
   SequenceChanEfx CEfx;
   CEfx.construct();

   CEfx[0].Volume = 100;
   CEfx[0].Pitch = 100;
   CEfx[0].PanBalance = 0;
   CEfx[0].PanDepth = PanMax;
   CEfx[0].PanCenter = 0;
   CEfx[0].Disabled = FALSE;

   while (1)
   {
      KBDKEYINFO info;
      memset(&info,0,sizeof(info));
      if (KbdCharIn(&info,IO_WAIT,0) != 0)
         return;

      switch (info.chChar)
      {
         // Esc
         case 27:
            Filter.ForceCompError();
            break;

         case '+':
            CEfx[0].Volume = 100;
            Filter.SetChanEfx(&CEfx);
            cout << "V" << CEfx[0].Volume << endl;
            break;

         case '-':
            CEfx[0].Volume--;
            Filter.SetChanEfx(&CEfx);
            cout << "V" << CEfx[0].Volume << endl;
            break;
         case '!':
            cout << Filter.Sync(Filter.SecondsToLen(5)) << endl;
            break;
         case 'a':
            Play.Advance(0,1);
            break;
         case 'A':
            Play.Advance(0,5);
            break;
         case 'r':
            Play.Advance(8,0);
            break;
         case 'R':
            Play.Advance(16,0);
            break;
         case 'H':
            Play.Jump(0,0);
            break;
         case 'b':
            Play.Advance(-16,0);
            break;
      }
   }
}

void DumpModuleInfo(museFormatBase &M)
{
   char *Free[2];

   cout << "  | '" << (Free[0] = M.GetTitle()) << "' A " << (Free[1] = M.GetTypeName()) << endl;
   SOMFree(Free[0]); SOMFree(Free[1]);

   cout << "  | Has " << M.GetNumChannels() << " channels and " << M.GetNumOrders() << " Orders" << endl;
}

// Taken from Commandline.cpp
void ShowVersionInf(SequenceFormatClass &FileFormats,SequenceOutputClass &Outputs)
{
   long MajorV;
   long MinorV;
   #ifdef DTS
   char *Name = museDACMixer::__ClassObject->somGetName();
   museDACMixer::__ClassObject->somGetVersionNumbers(&MajorV,&MinorV);
   #else
   char *Name = _museDACMixerClass->somGetName();
   _museDACMixerClass->somGetVersionNumbers(&MajorV,&MinorV);
   #endif

   cout << "   " << "Muse/2 Digital Mixer" << " (" <<
        Name << ") is Version " << MajorV <<
        '.' << MinorV << endl;

   // Display file formats methoods
   cout << "  File Formats:" << endl;

   for (SequenceFormatClass::iterator I = FileFormats.begin(); I != FileFormats.end(); I++)
   {
      long MajorV;
      long MinorV;
      (*I)->somGetVersionNumbers(&MajorV,&MinorV);
      cout << "   " << (*I)->GetTypeName() << " (" <<
           (*I)->somGetName() << ") is Version " << MajorV <<
           '.' << MinorV << endl;
   }

   // Display output methoods
   cout << "  Output Methods:" << endl;

   for (SequenceOutputClass::iterator I2 = Outputs.begin(); I2 != Outputs.end(); I2++)
   {
      long MajorV;
      long MinorV;
      (*I2)->somGetVersionNumbers(&MajorV,&MinorV);
      cout << "   " << (*I2)->GetTypeName() << " (" <<
           (*I2)->somGetName() << ") is Version " << MajorV << '.'
           << MinorV;

      int I3 = (*I2)->IsSupported();
      if (I3 == 0xFF)
      {
         cout << endl;
         continue;
      }

      if (I3 == 0)
         cout << " (Not Detected)" << endl;
      else
         cout << " (Detected, Level=" << I3 << ')' << endl;
   }
}
void main(char argc,char *argv[])
{
   /* Force the compiler to link the various formats, normally it will
      not include init segments in a library without specific reference
      to a symbol in the segment.

      After these calls all the objects will link themselfs into
      museHandlerListClass which can be accessed from an instantiation
      of museHandlerList.

      This is really ment for a player, for specific things like demos/games
      the classes can be referenced by name ie museS3MFormat without this
      hassle, although it is recommended to call link for all Output types
      so the autodetect code can be used.
    */
#ifdef CPPCOMPILE
   museS3MFormat::__ClassObject->Link();
   museMODFormat::__ClassObject->Link();
   museMTMFormat::__ClassObject->Link();
   museSTMFormat::__ClassObject->Link();
   museWOWFormat::__ClassObject->Link();
   museXMFormat::__ClassObject->Link();
   museITFormat::__ClassObject->Link();
   muse669Format::__ClassObject->Link();
   museFARFormat::__ClassObject->Link();
   museULTFormat::__ClassObject->Link();
   musePTMFormat::__ClassObject->Link();
   museDMFFormat::__ClassObject->Link();
   museFileWAV::__ClassObject->Link();
   museFileAU::__ClassObject->Link();
   museFileRAW::__ClassObject->Link();
   museGUSMixer::__ClassObject->Link();
   museDACDART::__ClassObject->Link();
   museDACMMPM2::__ClassObject->Link();
#endif

   // Create the handlers list class
   museHandlerList Handlers;

   // Query the # of formats
   SequenceFormatClass Formats;
   Handlers.GetFormats(&Formats);
   cout << Formats.size() << " File Formats supported." << endl;

   // Query the # of output types
   SequenceOutputClass Outputs;
   Handlers.GetOutputs(&Outputs);
   cout << Outputs.size() << " Output Devices supported." << endl;

   // Dump the versioning information
//   ShowVersionInf(Formats,Outputs);

   // Detect the output device
//   museOutputClass *OutputClass = Handlers.GetBestOutput(museDACMixer::__ClassObject);
   museOutputClass *OutputClass = Handlers.GetBestOutput(0);
   if (OutputClass == 0)
   {
      cout << "Unable to detect a device" << endl;
      return;
   }

   // Open the output device
   museOutputBase *Output = (museOutputBase *)(OutputClass->somNew());
   if (Output == 0)
   {
      cout << "Unable to construct output class" << endl;
      return;
   }

   // Special stuf for song to file output
   #ifndef DTS
      if (OutputClass->somDescendedFrom(_museFileOutput) == TRUE)
   #else
      if (OutputClass->somDescendedFrom(museFileOutput::__ClassObject) == TRUE)
   #endif
   {
      if (argc > 1)
      {
         char *N = new char [300];
         char *I = argv[1];
         for (I += strlen(I); I != argv[1] && *I != '\\' && *I != '/' && *I != ':'; I--);
         if (I != argv[1])
            I++;
         sprintf(N,"%s%s",I,((museFileOutput *)Output)->_get_Extension());
         ((museFileOutput *)Output)->_set_FileName(N);
      }
   }

   // Get the file name and show it.
   boolean AllowLoop = TRUE;
   #ifndef DTS
      if (OutputClass->somDescendedFrom(_museFileOutput) == TRUE)
   #else
      if (OutputClass->somDescendedFrom(museFileOutput::__ClassObject) == TRUE)
   #endif
   {
      cout << "Writing song to file " << ((museFileOutput *)Output)->_get_FileName() << endl;
   }
   AllowLoop = FALSE;

   // Init the output device
   #ifndef DTS
      if (OutputClass->somDescendedFrom(_museDACMixer) == TRUE)
   #else
      if (OutputClass->somDescendedFrom(museDACMixer::__ClassObject) == TRUE)
   #endif
   {
      ((museDACMixer *)Output)->SetMixParams(22050,16,1);
   }
   char *Error;
   if (Output->InitPlay(&Error) != 0)
   {
      cout << "Couldn't Initialize device. Error String is '" << Error << "'" << endl;
      return;
   }

   museEffectFilter Filter;
   musePrintFilter Filter2;
   Filter2.NextLink = &Filter;
   Filter.NextLink = Output;
   museTimeBuffer TimeBuf;
   TimeBuf.NextLink = &Filter;

   TID Tid;
   DosCreateThread(&Tid,&Keyboard,(unsigned long)(&Filter),2,20000);
   DosSetPriority(PRTYS_THREAD,PRTYC_REGULAR,5,Tid);
   DosCreateThread(&Tid,&TimeBuffer,(unsigned long)(&TimeBuf),2,20000);
   DosSetPriority(PRTYS_THREAD,PRTYC_IDLETIME,31,Tid);

   BYTE C[2] = {0,0};
   C[1] = 0x0F;
   VioScrollUp(0,0,25,80,25,C,0);

   for (int I = 1; I != argc; I++)
   {
      char *FileName = argv[I];
      cout << "  Playing " << FileName;

      // Locate the meta class for the file based on file name
      museFormatClass *FileClass = Handlers.GetClassForFile(FileName);
      if (FileClass == 0)
      {
         cout << endl << " Don't recognize this file type" << endl;
         continue;
      }

      cout << ". A " << FileClass->GetTypeName() << endl;
// Song Scanner test
      museSongInfo Info;
      if (FileClass->ScanFile(FileName,&Info) == 0)
      {
         cout << Info.Title << endl;
         cout << Info.TypeName << endl;
         cout << Info.ClassName << endl;
         cout << (int)Info.Channels << ',' << Info.Patterns << ',' << Info.Orders << endl;
      }

      // Instantiate the actual class,
      museFormatBase *Module = (museFormatBase *)FileClass->somNew();
      if (Module == 0)
      {
         cout << " Fatal Error, unable to instantiate module!" << endl;
         continue;
      }

      // Load the file (Modules can also be loaded from RAM, LoadMemModule)
      if (Module->LoadModule(FileName) != 0)
      {
         cout << " Loader Error" << endl;
         continue;
      }

      // Display information about the module (test the loader)
      DumpModuleInfo(*Module);

      // Play it
//      Module->_set_AllowLoop(AllowLoop);

      // Change to Module->Play(&Filter) to remove the pattern dump
      Module->Play(&TimeBuf,&Play);
   }
   Output->StopWhenDone();
}
