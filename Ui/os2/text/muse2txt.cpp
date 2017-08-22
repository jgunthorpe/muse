/* ########################################################################

   Muse2Txt - Text mode UI

   ########################################################################
*/
#ifndef CPPCOMPILE
#ifndef DTS
#include <IDLSequence.xh>
#include <somcdev.h>
#else
#include <IDLSequence.hh>
#endif
#else
#include <IDLSeq.hc>

#include <S3MForm.hc>
#include <MODForm.hc>
#include <MTMForm.hc>
#include <STMForm.hc>
#include <XMForm.hc>
#include <ITForm.hc>
#include <ULTForm.hc>
#include <FARForm.hc>
#include <669Form.hc>
#include <PTMForm.hc>

#include <WAV.hc>
#include <RAW.hc>
#include <AU.hc>
#include <GUSMixer.hc>
#include <DART.hc>
#include <MMPM2.hc>
#endif

#define INCL_KBD
#define INCL_VIO
#include <OS2.h>

#include "Screen.h"
#include <iostream.h>

void main(char argc,char *argv[])
{
   // Fix wacked watcom
   int Len = 0;
   int I;
   for (I = 1; I != argc; I++)
      Len += strlen(argv[I]) + 3;

   char *nargs = new char[Len];
   char *c = nargs;
   char **P = new char *[argc + 1];
   P[0] = argv[0];
   int nargc = 1;
   int Quote = 0;
   for (I = 1; I != argc; I++)
   {
      if (Quote == 0)
         P[nargc] = c;

      // Search for quotes.
      for (char *A = argv[I]; *A != 0; A++)
      {
         if (*A == '"')
         {
            if (Quote == 1)
               Quote = 0;
            else
               Quote = 1;
         }
         else
         {
            *c = *A;
            c++;
         }
      }

      // Terminate the string.
      if (Quote == 0)
      {
         *c = 0;
         c++;
         nargc++;
      }
      else
      {
         *c = ' ';
         c++;
      }
   }
   *c = 0;
   if (Quote == 1)
   {
      *c = 0;
      c++;
      nargc++;
   }

   // Banner
   cout << " Muse/2 Text Mode Interface V1.3                                          Ethos" << endl;

#ifdef CPPCOMPILE
   museS3MFormat::__ClassObject->Link();

   museMODFormat::__ClassObject->Link();
   museMTMFormat::__ClassObject->Link();
   museSTMFormat::__ClassObject->Link();
   museWOWFormat::__ClassObject->Link();
   museXMFormat::__ClassObject->Link();
   museITFormat::__ClassObject->Link();
   museULTFormat::__ClassObject->Link();
   museFARFormat::__ClassObject->Link();
   muse669Format::__ClassObject->Link();
   musePTMFormat::__ClassObject->Link();

   museFileWAV::__ClassObject->Link();
   museFileAU::__ClassObject->Link();
   museFileRAW::__ClassObject->Link();
   museGUSMixer::__ClassObject->Link();
   museDACDART::__ClassObject->Link();
   museDACMMPM2::__ClassObject->Link();
#endif

   // Main Object
   Screen TheScreen;
   if (TheScreen.HandleCommandLine(nargc,P) == 0)
   {
      if (TheScreen.Init() == 0)
         TheScreen.Play();
   }
   return;
}
