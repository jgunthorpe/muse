
/* ########################################################################

   DMFFormat - Muse/2 Class that handles X-Tracker files.

   ######################################################################## */

#include <Muse.h>

#ifndef CPPCOMPILE
#include <DMFForm.hh>
#else
#include <DMFForm.hc>
#endif

#include "DMFStruc.h"
#include "Unpack.h"

/* ########################################################################

   Class - museDMFFormat (Class to allow Muse/2 to handle DMF files)
   Member - Constructor (Init the class)
   IDL - somDefaultInit()

   ######################################################################## */

museDMFFormat::museDMFFormat()
{
   DBU_FuncTrace("museDMFFormat","museDMFFormat",TRACE_SIMPLE);

   // Init the structures

   Header = 0;
   Info = 0;
   Message = 0;
   Sequence = 0;
   PatternHeader = 0;
   Patterns = 0;
   InstrumentHeader = 0;
   Instruments = 0;
   Ranges = 0;
   SampleInfoHeader = 0;
   SampleInfos = 0;
   SampleInfos2 = 0;
   SampleHeader = 0;
   Samples = 0;
   TempPatternData = 0;
   TempSamples = 0;

   Private = false;
}

/* ########################################################################

   Class - museDMFFormat (Class to allow Muse/2 to handle DMF files)
   Member - Free (Delete all of the variables)
   IDL - void Free()

   This member deletes all of the allocated memory.

   It is private.

   ######################################################################## */

void museDMFFormat::Free()
{
   int x;
   DBU_FuncTrace("museDMFFormat","Free",TRACE_TESTED);

   if ( TempSamples && SampleInfoHeader )
   {
       for ( x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
       {
           delete [] TempSamples[x];
       }
       delete [] TempSamples;
   }

   if ( TempPatternData )
   {
       delete [] TempPatternData;
       TempPatternData = 0;
   }
   
   if ( Samples )
   {
       if ( Private )
       {
           for ( x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
           {
               delete [] Samples[x];
           }
       }
       delete [] Samples;
       Samples = 0;
   }
   if ( Private ) delete SampleHeader;
   SampleHeader = 0;
   
   if ( SampleInfos )
   {
       if ( Private )
       {
           for ( x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
           {
               delete SampleInfos[x];
           }
       }
       delete [] SampleInfos;
       SampleInfos = 0;
   }
   if ( SampleInfos2 )
   {
       if ( Private )
       {
           for ( x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
           {
               delete SampleInfos2[x];
           }
       }
       delete [] SampleInfos2;
       SampleInfos2 = 0;
   }
   if ( Private ) delete SampleInfoHeader;
   SampleInfoHeader = 0;

   if ( Ranges )
   {
       for ( x = 0; x < InstrumentHeader->NoOfInstruments; x++ )
       {
           delete [] Ranges[x];
       }
       delete [] Ranges;
       Ranges = 0;
   }

   if ( Instruments )
   {
       delete [] Instruments;
       Instruments = 0;
   }
   InstrumentHeader = 0;

   if ( Patterns )
   {
       if ( Private )
       {
           for ( x = 0; x < PatternHeader->NoOfPatterns; x++ )
           {
               delete [] Patterns[x];
           }
       }
       delete [] Patterns;
       Patterns = 0;
   }
   if ( Private ) delete PatternHeader;
   PatternHeader = 0;

   if ( Private )
   {
       delete Sequence;
       delete Message;
       delete Info;
       delete Header;
   }
   
   Sequence = 0;
   Message = 0;
   Info = 0;
   Header = 0;

   Private = false;

}

/* ########################################################################

   Class - museDMFFormat (Class to allow Muse/2 to handle DMF files)
   Member - LoadMemModule (Load a module into this class)
   IDL - long LoadMemModule(in octet *Region, in unsigned long Size)

   This member is responsable for assigning the correct pointers for an
   in-memory DMF.

   Returns 0 on success.

   ######################################################################## */

#pragma off(behaved)

long museDMFFormat::LoadMemModule ( unsigned char *Region, unsigned long Size )
{
    Free();
    DBU_FuncTrace("museDMFFormat","LoadModule",TRACE_TESTED);
    unsigned char *CurrentPos = Region, *StartPos;
    int x;

    Private = true;

    if ( CurrentPos + sizeof ( DMFHeader ) > Region + Size )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Truncated | 0 );
    }

    Header = new DMFHeader;
    memcpy ( Header, CurrentPos, sizeof (DMFHeader) );
    CurrentPos += sizeof ( DMFHeader );
    
    if ( memcmp ( Header->Sig, "DDMF", 4 ) )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Sigcheck | 0 );
    }

    if ( CurrentPos + sizeof (DMFInfo) > Region + Size )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Truncated | 1 );
    }

    Info = new DMFInfo;
    memcpy ( Info, CurrentPos, sizeof (DMFInfo) );

    if ( memcmp ( Info->Sig, "INFO", 4 ) ) goto TestMessage;
    CurrentPos += Info->HeaderSize + 8;
    
TestMessage:
    
    if ( CurrentPos + sizeof (DMFMessage) > Region + Size )
    {
        Free();
        return ( LOADPART_Message | LOADFAIL_Truncated );
    }
    Message = new DMFMessage;
    memcpy ( Message, CurrentPos, sizeof (DMFMessage) );
    if ( memcmp ( Message->Sig, "CMSG", 4 ) )
    {
        Free();
        return ( LOADPART_Message | LOADFAIL_Sigcheck );
    }
    CurrentPos += Message->HeaderSize + 8;
    
    if ( CurrentPos + sizeof ( DMFSequence ) > Region + Size )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Truncated | 2 );
    }
    Sequence = (DMFSequence *) new unsigned char[((DMFSequence *) CurrentPos)->HeaderSize+8];
    memcpy ( Sequence, CurrentPos, ((DMFSequence *) CurrentPos)->HeaderSize+8 );
    if ( memcmp ( Sequence->Sig, "SEQU", 4 ) )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Sigcheck | 2 );
    }
    CurrentPos += Sequence->HeaderSize + 8;
    
    StartPos = CurrentPos;
    if ( CurrentPos + sizeof ( DMFPatternHeader ) > Region + Size )
    {
        Free();
        return ( LOADPART_Patterns | LOADFAIL_Truncated );
    }
    PatternHeader = new DMFPatternHeader;
    memcpy ( PatternHeader, CurrentPos, sizeof (DMFPatternHeader) );
    CurrentPos += sizeof ( DMFPatternHeader );
    if ( memcmp ( PatternHeader->Sig, "PATT", 4 ) )
    {
        Free();
        return ( LOADPART_Patterns | LOADFAIL_Sigcheck );
    }
    
    unsigned char *DataPointer, *EndPointer;
    int w, y, z;
    char Counters[33];
    
    Patterns = new DMFPattern *[PatternHeader->NoOfPatterns];
    memset ( Patterns, 0, PatternHeader->NoOfPatterns*sizeof(DMFPattern *) );
    for ( x = 0; x < PatternHeader->NoOfPatterns; x++ )
    {
        DMFPattern *Pattern;
        
        Pattern = (DMFPattern *) CurrentPos;
        CurrentPos += sizeof ( DMFPattern );
        if ( CurrentPos > Region + Size )
        {
            Free();
            return ( LOADPART_Patterns | LOADFAIL_Truncated | x );
        }
        CurrentPos += Pattern->HeaderSize - sizeof ( DMFPattern ) + 8;
        if ( CurrentPos > Region + Size )
        {
            Free();
            return ( LOADPART_Patterns | LOADFAIL_Truncated | x );
        }
        
        DataPointer = &Pattern->PatternData;
        EndPointer = &Pattern->PatternData + Pattern->HeaderSize - sizeof ( DMFPattern ) + 1 + 8;
        
        memset ( Counters, 0, 33 );
        
        y = 0;
        while ( y < Pattern->Rows )
        {
            if ( !Counters[0] )
            {
                if ( DataPointer >= EndPointer )
                {
                    Free();
                    return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
                }
                
                w = *DataPointer++;
                if ( w & PATTERN_Counter )
                {
                    if ( DataPointer >= EndPointer )
                    {
                        Free();
                        return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
                    }
                    Counters[0] = *DataPointer++;
                }
                if ( w & ~PATTERN_Counter ) DataPointer++;
            } else
                Counters[0]--;
            
            for ( z = 0; z < Pattern->Channels; z++ )
            {
                if ( !Counters[z+1] )
                {
                    if ( DataPointer >= EndPointer )
                    {
                        Free();
                        return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
                    }
                    w = *DataPointer++;
                    if ( w & PATTERN_Counter )
                    {
                        if ( DataPointer >= EndPointer )
                        {
                            Free();
                            return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
                        }
                        Counters[z+1] = *DataPointer++;
                    }
                    if ( w & PATTERN_Instrument ) DataPointer += 1;
                    if ( w & PATTERN_Note ) DataPointer += 1;
                    if ( w & PATTERN_Volume ) DataPointer += 1;
                    if ( w & PATTERN_InstrumentEffect ) DataPointer += 2;
                    if ( w & PATTERN_NoteEffect ) DataPointer += 2;
                    if ( w & PATTERN_VolumeEffect ) DataPointer += 2;
                } else
                    Counters[z+1]--;
            }
            y++;
        }
        
        if ( DataPointer > EndPointer )
        {
            Free();
            return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
        }

        Patterns[x] = (DMFPattern *) new unsigned char[EndPointer-((unsigned char *)Pattern)];
        memcpy ( Patterns[x], Pattern, EndPointer-((unsigned char *)Pattern) );
    }
    CurrentPos = StartPos + PatternHeader->HeaderSize + 8;
    
    StartPos = CurrentPos;
    if ( CurrentPos + sizeof ( DMFSampleInfoHeader ) > Region + Size )
    {
        Free();
        return ( LOADPART_Instruments | LOADFAIL_Truncated );
    }
    SampleInfoHeader = new DMFSampleInfoHeader;
    memcpy ( SampleInfoHeader, CurrentPos, sizeof(DMFSampleInfoHeader) );
    CurrentPos += sizeof ( DMFSampleInfoHeader );
    if ( memcmp ( SampleInfoHeader->Sig, "SMPI", 4 ) )
    {
        Free();
        return ( LOADPART_Instruments | LOADFAIL_Sigcheck );
    }
    
    SampleInfos = new DMFSampleInfo *[SampleInfoHeader->NoOfSamples];
    SampleInfos2 = new DMFSampleInfo2 *[SampleInfoHeader->NoOfSamples];
    for ( x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
    {
        if ( CurrentPos + sizeof ( DMFSampleInfo ) > Region + Size )
        {
            Free();
            return ( LOADPART_Instruments | LOADFAIL_Truncated | x );
        }
        SampleInfos[x] = (DMFSampleInfo *) new unsigned char[*CurrentPos + 1];
        memcpy ( SampleInfos[x], CurrentPos, *CurrentPos + 1 );
        CurrentPos += *CurrentPos + 1;
        
        if ( CurrentPos + sizeof ( DMFSampleInfo2 ) > Region + Size )
        {
            Free();
            return ( LOADPART_Instruments | LOADFAIL_Truncated | x );
        }
        SampleInfos2[x] = new DMFSampleInfo2;
        memcpy ( SampleInfos2[x], CurrentPos, sizeof(DMFSampleInfo2) );
        CurrentPos += sizeof ( DMFSampleInfo2 );
        if ( Header->Version >= 8 ) CurrentPos += 8;
    }
    CurrentPos = StartPos + SampleInfoHeader->HeaderSize + 8;
    
    StartPos = CurrentPos;
    if ( CurrentPos + sizeof(DMFSampleHeader) > Region + Size )
    {
        Free();
        return ( LOADPART_Samples | LOADFAIL_Truncated );
    }
    SampleHeader = new DMFSampleHeader;
    memcpy ( SampleHeader, CurrentPos, sizeof(DMFSampleHeader) );
    CurrentPos += sizeof ( DMFSampleHeader );
    if ( memcmp ( SampleHeader->Sig, "SMPD", 4 ) )
    {
        Free();
        return ( LOADPART_Samples | LOADFAIL_Sigcheck );
    }
    
    Samples = new DMFSample *[SampleInfoHeader->NoOfSamples];
    for ( x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
    {
        DMFSample *Sample;
        
        if ( CurrentPos + sizeof( DMFSample ) > Region + Size )
        {
            Free();
            return ( LOADPART_Samples | LOADFAIL_Truncated | x );
        }

        Sample = (DMFSample *) CurrentPos;
        if ( CurrentPos + Sample->SampleSize + 4 > Region + Size )
        {
            Free();
            return ( LOADPART_Samples | LOADFAIL_Truncated | x );
        }

        Samples[x] = (DMFSample *) new unsigned char[Sample->SampleSize + 4];
        memcpy ( Samples[x], CurrentPos, Sample->SampleSize + 4 );
        CurrentPos += Sample->SampleSize + 4;
    }
    CurrentPos = StartPos + SampleHeader->HeaderSize + 8;
    
    return LOADOK;
}

long museDMFFormat::LoadMemModuleKeep ( unsigned char *Region, unsigned long Size )
{
    Free();
    DBU_FuncTrace("museDMFFormat","LoadModule",TRACE_TESTED);
    unsigned char *CurrentPos = Region, *StartPos;
    int x;
    
    Header = (DMFHeader *) CurrentPos;
    CurrentPos += sizeof ( DMFHeader );
    if ( CurrentPos > Region + Size )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Truncated | 0 );
    }
    
    if ( memcmp ( Header->Sig, "DDMF", 4 ) )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Sigcheck | 0 );
    }
    
    Info = (DMFInfo *) CurrentPos;
    if ( CurrentPos + sizeof (DMFInfo) > Region + Size )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Truncated | 1 );
    }
    if ( memcmp ( Info->Sig, "INFO", 4 ) ) goto TestMessage;
    CurrentPos += Info->HeaderSize + 8;
    
TestMessage:
    Message = (DMFMessage *) CurrentPos;
    CurrentPos += sizeof ( DMFMessage );
    if ( CurrentPos > Region + Size )
    {
        Free();
        return ( LOADPART_Message | LOADFAIL_Truncated );
    }
    if ( memcmp ( Message->Sig, "CMSG", 4 ) )
    {
        Free();
        return ( LOADPART_Message | LOADFAIL_Sigcheck );
    }
    
    CurrentPos += Message->HeaderSize - sizeof ( DMFMessage ) + 8;
    
    Sequence = (DMFSequence *) CurrentPos;
    CurrentPos += sizeof ( DMFSequence );
    if ( CurrentPos > Region + Size )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Truncated | 2 );
    }
    
    if ( memcmp ( Sequence->Sig, "SEQU", 4 ) )
    {
        Free();
        return ( LOADPART_Header | LOADFAIL_Sigcheck | 2 );
    }
    CurrentPos += Sequence->HeaderSize - sizeof ( DMFSequence ) + 8;
    
    StartPos = CurrentPos;
    PatternHeader = (DMFPatternHeader *) CurrentPos;
    CurrentPos += sizeof ( DMFPatternHeader );
    if ( CurrentPos > Region + Size )
    {
        Free();
        return ( LOADPART_Patterns | LOADFAIL_Truncated );
    }
    if ( memcmp ( PatternHeader->Sig, "PATT", 4 ) )
    {
        Free();
        return ( LOADPART_Patterns | LOADFAIL_Sigcheck );
    }
    
    unsigned char *DataPointer, *EndPointer;
    int w, y, z;
    char Counters[33];
    
    Patterns = new DMFPattern *[PatternHeader->NoOfPatterns];
    for ( x = 0; x < PatternHeader->NoOfPatterns; x++ )
    {
        Patterns[x] = (DMFPattern *) CurrentPos;
        CurrentPos += sizeof ( DMFPattern );
        if ( CurrentPos > Region + Size )
        {
            Free();
            return ( LOADPART_Patterns | LOADFAIL_Truncated | x );
        }
        CurrentPos += Patterns[x]->HeaderSize - sizeof ( DMFPattern ) + 8;
        if ( CurrentPos > Region + Size )
        {
            Free();
            return ( LOADPART_Patterns | LOADFAIL_Truncated | x );
        }
        
        DataPointer = &Patterns[x]->PatternData;
        EndPointer = &Patterns[x]->PatternData + Patterns[x]->HeaderSize - sizeof ( DMFPattern ) + 1 + 8;
        
        memset ( Counters, 0, 33 );
        
        y = 0;
        while ( y < Patterns[x]->Rows )
        {
            if ( !Counters[0] )
            {
                if ( DataPointer >= EndPointer )
                {
                    Free();
                    return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
                }
                
                w = *DataPointer++;
                if ( w & PATTERN_Counter )
                {
                    if ( DataPointer >= EndPointer )
                    {
                        Free();
                        return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
                    }
                    Counters[0] = *DataPointer++;
                }
                if ( w & ~PATTERN_Counter ) DataPointer++;
            } else
                Counters[0]--;
            
            for ( z = 0; z < Patterns[x]->Channels; z++ )
            {
                if ( !Counters[z+1] )
                {
                    if ( DataPointer >= EndPointer )
                    {
                        Free();
                        return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
                    }
                    w = *DataPointer++;
                    if ( w & PATTERN_Counter )
                    {
                        if ( DataPointer >= EndPointer )
                        {
                            Free();
                            return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
                        }
                        Counters[z+1] = *DataPointer++;
                    }
                    if ( w & PATTERN_Instrument ) DataPointer += 1;
                    if ( w & PATTERN_Note ) DataPointer += 1;
                    if ( w & PATTERN_Volume ) DataPointer += 1;
                    if ( w & PATTERN_InstrumentEffect ) DataPointer += 2;
                    if ( w & PATTERN_NoteEffect ) DataPointer += 2;
                    if ( w & PATTERN_VolumeEffect ) DataPointer += 2;
                } else
                    Counters[z+1]--;
            }
            y++;
        }
        
        if ( DataPointer > EndPointer )
        {
            Free();
            return ( LOADPART_Patterns | LOADFAIL_Corrupt | x );
        }
        
    }
    CurrentPos = StartPos + PatternHeader->HeaderSize + 8;
    
    StartPos = CurrentPos;
    SampleInfoHeader = (DMFSampleInfoHeader *) CurrentPos;
    CurrentPos += sizeof ( DMFSampleInfoHeader );
    if ( CurrentPos > Region + Size )
    {
        Free();
        return ( LOADPART_Instruments | LOADFAIL_Truncated );
    }
    if ( memcmp ( SampleInfoHeader->Sig, "SMPI", 4 ) )
    {
        Free();
        return ( LOADPART_Instruments | LOADFAIL_Sigcheck );
    }
    
    SampleInfos = new DMFSampleInfo *[SampleInfoHeader->NoOfSamples];
    SampleInfos2 = new DMFSampleInfo2 *[SampleInfoHeader->NoOfSamples];
    for ( x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
    {
        SampleInfos[x] = (DMFSampleInfo *) CurrentPos;
        if ( CurrentPos + 4 > Region + Size )
        {
            Free();
            return ( LOADPART_Instruments | LOADFAIL_Truncated | x );
        }
        CurrentPos += *CurrentPos + 1;
        SampleInfos2[x] = (DMFSampleInfo2 *) CurrentPos;
        CurrentPos += sizeof ( DMFSampleInfo2 );
        if ( Header->Version >= 8 ) CurrentPos += 8;
        if ( CurrentPos > Region + Size )
        {
            Free();
            return ( LOADPART_Instruments | LOADFAIL_Truncated | x );
        }
    }
    CurrentPos = StartPos + SampleInfoHeader->HeaderSize + 8;
    
    StartPos = CurrentPos;
    SampleHeader = (DMFSampleHeader *) CurrentPos;
    CurrentPos += sizeof ( DMFSampleHeader );
    if ( CurrentPos > Region + Size )
    {
        Free();
        return ( LOADPART_Samples | LOADFAIL_Truncated );
    }
    if ( memcmp ( SampleHeader->Sig, "SMPD", 4 ) )
    {
        Free();
        return ( LOADPART_Samples | LOADFAIL_Sigcheck );
    }
    
    Samples = new DMFSample *[SampleInfoHeader->NoOfSamples];
    for ( x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
    {
        Samples[x] = (DMFSample *) CurrentPos;
        if ( CurrentPos + sizeof( DMFSample ) > Region + Size )
        {
            Free();
            return ( LOADPART_Samples | LOADFAIL_Truncated | x );
        }
        CurrentPos += Samples[x]->SampleSize + 4;
        if ( CurrentPos > Region + Size )
        {
            Free();
            return ( LOADPART_Samples | LOADFAIL_Truncated | x );
        }
    }
    CurrentPos = StartPos + SampleHeader->HeaderSize + 8;
    
    return LOADOK;
}

#pragma on(behaved)

/* ########################################################################

   Class - museDMFFormat (Class to allow Muse/2 to handle DMF files)
   Members - Query members

   These members return variouse bits of info about the DMF file.

   ######################################################################## */

unsigned short museDMFFormat::GetNumPatterns()
{
   DBU_FuncTrace("museDMFFormat","GetNumPatterns",TRACE_TESTED);

   if (PatternHeader == 0)
      return 0;
   return PatternHeader->NoOfPatterns;
}

unsigned short museDMFFormat::GetNumOrders()
{
   DBU_FuncTrace("museDMFFormat","GetNumOrders",TRACE_TESTED);
   if (Sequence == 0 || (Sequence->HeaderSize-6)/2 <= 0)
      return 0;
   return (Sequence->HeaderSize-6)/2;
}

unsigned short museDMFFormat::GetRowsAPattern()
{
   DBU_FuncTrace("museDMFFormat","GetRowsAPattern",TRACE_TESTED);
   return ( 64);
}

string museDMFFormat::GetTitle()
{
   DBU_FuncTrace("museDMFFormat","GetTitle",TRACE_TESTED);
   if (Header == 0)
      return 0;

   char *Temp = (char *)SOMMalloc(31);

   // Ensure correct termination
   for (unsigned int I = 0; I != 30; I++)
      Temp[I] = Header->SongName[I];
   Temp[I] = 0;

   return Temp;
}

unsigned short museDMFFormat::GetNumChannels()
{
   DBU_FuncTrace("museDMFFormat","GetNumChannels",TRACE_TESTED);
   if (PatternHeader == 0)
      return 0;

   // Ennumerate the # of channels
   return PatternHeader->NoOfChannels;
}

// Make sure to add the new structure members
void museDMFFormat::GetSongSamples(SequenceSample &Samples_)
{
   DBU_FuncTrace("museDMFFormat","GetsongSamples",TRACE_TESTED);
   if (Header == 0)
      return;

   Samples_.construct();
   Samples_.reserve(SampleInfoHeader->NoOfSamples);

   for ( int x = 0; x < SampleInfoHeader->NoOfSamples; x++ )
   {
      if ( SampleInfos2[x]->Length == 0 ) Samples_[x].Sample = 0;
      else
      {
          Samples_[x].SampleEnd = SampleInfos2[x]->Length-8;

          if ( ( SampleInfos2[x]->Flags & SMPL_PackType ) == SMPL_PackType0 ) {
              register unsigned long PackedLen;
              register unsigned long SampleLen;
              unsigned char *Sample;

              PackedLen = Samples[x]->SampleSize - 4;
              SampleLen = SampleInfos2[x]->Length - 8;

              Sample = new unsigned char [SampleLen];

              if ( TempSamples && TempSamples[x] )
              {
                  Samples_[x].Sample = TempSamples[x];
              } else {
//                  printf ( ">>%d", x );
                  fflush ( stdout );
                  if ( Unpack8 ( &Samples[x]->SampleData, Sample, PackedLen, SampleLen ) )
                  {
//                      printf ( " BAD\n" );
                      delete [] Sample;
                      Samples_[x].Sample = (unsigned char *) &Samples[x]->SampleData;
                      if ( Samples_[x].SampleEnd > Samples[x]->SampleSize - 4 ) Samples_[x].SampleEnd = Samples[x]->SampleSize - 4;
                  }
                  else
                  {
//                      printf ( " OK\n" );
                      if ( !TempSamples )
                      {
                          TempSamples = new unsigned char *[SampleInfoHeader->NoOfSamples];
                          memset ( TempSamples, 0, SampleInfoHeader->NoOfSamples*sizeof(unsigned char *) );
                      }
                      
                      TempSamples[x] = Sample;

                      Samples_[x].Sample = Sample;
                  }
              }
          } else {
//              if ( ( SampleInfos2[x]->Flags & SMPL_PackType ) ) printf (">>%d<<\n", ( SampleInfos2[x]->Flags & SMPL_PackType ) );
              Samples_[x].Sample = (unsigned char *) &Samples[x]->SampleData;
              if ( Samples_[x].SampleEnd > Samples[x]->SampleSize - 4 ) Samples_[x].SampleEnd = Samples[x]->SampleSize - 4;
          }
      }
       
      if ( SampleInfos2[x]->Flags & SMPL_Loop ) {
          Samples_[x].LoopBegin = SampleInfos2[x]->LoopStart;
          Samples_[x].LoopEnd = SampleInfos2[x]->LoopEnd;
          if ( Samples_[x].LoopEnd > Samples_[x].SampleEnd )
              Samples_[x].LoopEnd = Samples_[x].SampleEnd;
          Samples_[x].Flags = INST_Loop;
      } else {
          Samples_[x].LoopBegin = 0;
          Samples_[x].LoopEnd = SampleInfos2[x]->Length;
          Samples_[x].Flags = 0;
      }
      Samples_[x].Name = &SampleInfos[x]->Name;
      Samples_[x].InstNo = x;
      Samples_[x].SampleName = &SampleInfos[x]->Name;
      if ( SampleInfos2[x]->Flags & SMPL_16Bit ) Samples_[x].Flags |= INST_16Bit;
      else Samples_[x].Flags |= INST_Signed;
      
      Samples_[x].Center = SampleInfos2[x]->Frequency;
      Samples_[x].Volume = SampleInfos2[x]->Volume;
      }
}

string museDMFFormat::GetTypeName()
{
   DBU_FuncTrace("museDMFFormat","GetTypeName",TRACE_TESTED);
   char *C = "X-Tracker/Delusion Module";
   char *Str = (char *)SOMMalloc(strlen(C) + 1);
   strcpy(Str,C);
   return Str;
}

/* ########################################################################

   Class - museDMFFormatClass (Class of museDMFFormat)
   Member - GetTypeName (Returns the name of this class)
   IDL - string GetTypeName

   Returns a pointer to a string containing 'Extended Module'.
   This string is valid while the object exists.

   ######################################################################## */

string museDMFFormatClass::GetTypeName()
{
   return "X-Tracker/Delusion Module";
}

/* ########################################################################

   Class - museDMFFormatClass (Class of museDMFFormat)
   Member - GetClassForFile (Check for *.DMF and then returns this)
   IDL - museFormatClass GetClassForFile(in string FName)

   Returns a pointer to the metaclass which can handle that file.

   ######################################################################## */

museFormatClass *museDMFFormatClass::GetClassForFile(string FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (stricmp(End,"DMF") != 0)
      return 0;
   return this;
}

/* ########################################################################

   Class - museDMFFormatClass (Class of museDMFFormat)
   Header Scanning code

   This code is used to get preliminary song information. It scans a minimum
   number of bytes in the file.

   ######################################################################## */

unsigned long museDMFFormatClass::GetScanSize()
{
   return sizeof(DMFHeader);
}

unsigned long museDMFFormatClass::Scan(octet *Region, museSongInfo *Info)
{
   museFormatClass::Scan(Region,Info);
   return 1;
}

#ifndef CPPCOMPILE
extern "C"
{
   void QueryFileHandlers(Sequence<museFormatClass *> *);
}

void QueryFileHandlers(Sequence<museFormatClass *> *List)
{
   List->construct();
   (*List)[0] = (museFormatClass *)museDMFFormat::__ClassObject;
}
#else
#include <HandLst.hc>
museDMFFormatClass *museDMFFormat::__ClassObject = new museDMFFormatClass;
museDMFFormatClass::museDMFFormatClass()
{
   museHandlerList::__ClassObject->AddFormat(this);
   MajorVersion = 1;
   MinorVersion = 0;
}
#endif
