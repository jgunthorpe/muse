#include <Muse.h>
#include <HandLst.hc>

#include <S3MForm.hc>
#include <MODForm.hc>
#include <XMForm.hc>
#include <ITForm.hc>
#include <DART.hc>
#include <SimpFilter.hc>
#include <DSC.hc>
#include <M32Flat.hc>
#include <Echo.hc>
#include <iostream.h>
#include <stdio.h>

#define INCL_DOS
#define INCL_VIO
#define INCL_KBD
#define INCL_DOSERRORS
#include <os2.h>

musePlayerControl Play;
museOutputBase *Output;
SequenceOFilterClass OFilters;
SequenceMixerClass Mixers;
museDACMixer *DAC;
unsigned long SPercent;
void APIENTRY Keyboard(unsigned long Data)
{
   while (1)
   {
      KBDKEYINFO info;
      memset(&info,0,sizeof(info));
      if (KbdCharIn(&info,IO_WAIT,0) != 0)
         return;

      if (info.chChar >= 'A' && info.chChar <= 'Z')
      {
         int I = info.chChar - 'A';
         if (I >= OFilters.size())
            continue;
         museOutputFilterBase *Old = DAC->UseOFilter((museOutputFilterBase *)OFilters[I]->somNew());
         cout << I << "; Filter : " << OFilters[I]->GetTypeName() << endl;
         delete Old;
         continue;
      }

      if (info.chChar >= '1' && info.chChar <= '9')
      {
         int I = info.chChar - '1';
         if (I >= Mixers.size())
            continue;
         museMixerBase *Old = DAC->UseMixer((museMixerBase *)Mixers[I]->somNew());
         cout << I << "; Mixer : " << Mixers[I]->GetTypeName() << endl;
         delete Old;
         continue;
      }

      switch (info.chChar)
      {
         // Esc
         case 27:
            Output->ForceCompError();
            break;

         case '+':
            SPercent += 10;
            SPercent = DAC->SetScale(SPercent);
            printf("SC : %u\n",SPercent);
            break;

         case '-':
            SPercent -= 10;
            SPercent = DAC->SetScale(SPercent);
            printf("SC : %u\n",SPercent);
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
   museXMFormat::__ClassObject->Link();
   museITFormat::__ClassObject->Link();

   museDACDART::__ClassObject->Link();

   museScaleFilter::__ClassObject->Link();
   museNoisyClipFilter::__ClassObject->Link();
   museLightFilter::__ClassObject->Link();
   museLight2Filter::__ClassObject->Link();
   museHeavyDeepFilter::__ClassObject->Link();
   museHeavyDeep2Filter::__ClassObject->Link();
   museHeavyDeep3Filter::__ClassObject->Link();
   museDSCFilter::__ClassObject->Link();

   museMixerOrg::__ClassObject->Link();
   museMixerInt::__ClassObject->Link();
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
   ShowVersionInf(Formats,Outputs);

   // Query the # of output types
   Handlers.GetMixers(&Mixers);
   cout << Mixers.size() << " Mixers supported." << endl;

   // Query the # of output types
   SequenceFilterClass Filters;
   Handlers.GetFilters(&Filters);
   cout << Filters.size() << " Filters supported." << endl;

   // Query the # of output types
   Handlers.GetOFilters(&OFilters);
   cout << OFilters.size() << " Output Filters supported." << endl;

   // Detect the output device
   museOutputClass *OutputClass = Handlers.GetBestOutput(museDACMixer::__ClassObject);
//   museOutputClass *OutputClass = Handlers.GetBestOutput(0);
   if (OutputClass == 0)
   {
      cout << "Unable to detect a device" << endl;
      return;
   }

   // Open the output device
   Output = (museOutputBase *)(OutputClass->somNew());
   if (Output == 0)
   {
      cout << "Unable to construct output class" << endl;
      return;
   }
   DAC = (museDACMixer *)Output;
   DAC->UseMixer((museMixerBase *)Mixers[1]->somNew());
   DAC->UseOFilter((museOutputFilterBase *)OFilters[0]->somNew());

//   museEchoFilter *EchoFilt = new museEchoFilter;
//   DAC->AddFilter(EchoFilt);
//   DAC->AddFilter(new museDSCFilter);

/*   int NoOfEchos;
   InputEcho *Echos;

   NoOfEchos = 4;
   Echos = new InputEcho[4];
   Echos[0].Delay = 2.0/16.0;
   Echos[0].ScaleL = 3.0/8.0;
   Echos[0].ScaleR = 3.0/8.0;
   Echos[0].Flags = ECHO_Flipped;
   Echos[1].Delay = 2.0/8.0;
   Echos[1].ScaleL = 3.3/8.0;
   Echos[1].ScaleR = 3.3/8.0;
   Echos[1].Flags = ECHO_Surround;
   Echos[2].Delay = 4.0/8.0;
   Echos[2].ScaleL = 2.0/8.0;
   Echos[2].ScaleR = 2.0/8.0;
   Echos[2].Flags = ECHO_Flipped | ECHO_Surround;
   Echos[3].Delay = 6.0/8.0;
   Echos[3].ScaleL = 3.5/8.0;
   Echos[3].ScaleR = 3.5/8.0;
   Echos[3].Flags = ECHO_Normal | ECHO_Feedback;
   EchoFilt->SetupEcho ( NoOfEchos, Echos );
   delete [] Echos;*/

   cout << "Filter :" << OFilters[0]->somGetName() << endl;

   if (OutputClass->somDescendedFrom(museDACMixer::__ClassObject) == TRUE)
   {
//      cout << "Setting to 22k" << endl;
//      ((museDACMixer *)Output)->SetMixParams(22050,16,1);
   }

   char *Error;
   if (Output->InitPlay(&Error) != 0)
   {
      cout << "Couldn't Initialize device. Error String is '" << Error << "'" << endl;
      return;
   }
   SPercent = DAC->SetScale(0);

   TID Tid;
   DosCreateThread(&Tid,&Keyboard,0,2,20000);
   DosSetPriority(PRTYS_THREAD,PRTYC_REGULAR,5,Tid);

   BYTE C[2] = {0,0};
   C[1] = 0x0F;
//   VioScrollUp(0,0,25,80,25,C,0);

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
      Module->Play(Output,&Play);
   }
   Output->StopWhenDone();
}
