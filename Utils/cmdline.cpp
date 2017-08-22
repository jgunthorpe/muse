// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   CommandLine - A class to allow a set of independant classes to share
                 a command line.

   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <sequence.h>
#include <cmdline.h>
#include <string.h>
#include <stdio.h>
   									/*}}}*/

template class Sequence<IniOptions::Option>;
IniOptions *IniOptions::Ini = 0;

#define FREE_ARG 1

// IniOptions::IniOptions - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
IniOptions::IniOptions()
{
   // Get a fair number of options to start with
   Options.reserve(50);
}
									/*}}}*/
// IniOptions::~IniOptions - Destructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
IniOptions::~IniOptions()
{
}
									/*}}}*/
// IniOptions::FindOptLong - Locates an option by Long Command name 	/*{{{*/
// ---------------------------------------------------------------------
/* The array is searched by the long command line name */
IniOptions::Option *IniOptions::FindOptLong(const char *CmdOpt)
{
   // Find the matching option.
   Option *Opt;
   for (Opt = Options.begin(); Opt != Options.end(); Opt++)
      if (strcmp(CmdOpt,Opt->CmdOpt) == 0)
	 return Opt;

   return 0;
}
									/*}}}*/
// IniOptions::CommandLine - Command Line Parser			/*{{{*/
// ---------------------------------------------------------------------
/* This will parse the command line, fill in the option list and extract
   a complete list of files on the command line. */
int IniOptions::CommandLine(int argc,char **argv,SequenceString *Files)
{
   char Arg[300];
   
   for (int I = 1; I != argc; I++)
   {
      if (strlen(argv[I]) >= sizeof(Arg))
      {
	 printf("Argument %i is too long\n",I);
	 continue;
      }
      strcpy(Arg,argv[I]);
      
      // Option of some sort
      if (Arg[0] == '-')
      {
	 // Long option
	 if (Arg[1] == '-')
	 {
	    char *Value = 0;
	    
	    // Strip of the value when --arg=val is used
	    for (char *C = Arg + 2; *C != 0; C++) 
	       if (*C == '=')
	       {
		  *C = 0;
		  Value = argv[I] + (C - Arg + 1);
		  break;
	       }

	    // Find the matching option.
	    Option *Opt = FindOptLong(Arg+2);
	    if (Opt == 0)
	    {
	       printf("Note! Option %s is not recognized.\n",Arg+2);
	       continue;
	    }
	    
	    // Value is with the next argument
	    if (Value == 0 && Opt->Flag == 0)
	    {
	       if (I + 1 >= argc || argv[I+1][0] == '-')
	       {
		  printf("Option %s does not have a parameter.\n",Arg+2);
		  return -1;
	       }

	       Value = argv[I + 1];
	       I++;
	    }
	    
	    // Hmm flag with a value, woops.
	    if (Value != 0 && Opt->Flag == 1)
	       printf("Option %s is a flag, it should not have a value.\n",Arg+1);

	    // Use a pointer to this blank string to indicate a flag is flagged.
	    if (Opt->Flag == 1)
	       Value = "";
	    
	    // Store away the argument
	    if (Opt->Free & FREE_ARG != 0)
	       delete [] Opt->Arg;
	    
	    Opt->Free &= ~FREE_ARG;
	    Opt->Arg = Value;
	    continue;
	 }	 

	 // Short option
	 // Find the matching option.
	 Option *Opt;
	 for (Opt = Options.begin(); Opt != Options.end(); Opt++)
	    if (Opt->CmdShortOpt == Arg[1])
	       break;
	 
	 if (Opt == Options.end())
	 {
	    printf("Note! Option -%c is not recognized.\n",Arg[1]);
	    continue;
	 }
      
         char *Value = 0;

         // Value is right after the first letter
         if (Arg[2] != 0)
	    Value = argv[I] + 2;

         // Value must be in the next arg
         if (Opt->Flag == 0 && Value == 0)
	 {
	    if (I + 1 >= argc || argv[I+1][0] == '-')
	    {
	       printf("Option -%c does not have a parameter.\n",Arg[1]);
	       return -1;
	    }
	    
	    Value = argv[I + 1];
	    I++;
	 }
	 
	 // Hmm flag with a value, woops.
	 if (Value != 0 && Opt->Flag == 1)
	    printf("Option -%cs is a flag, it should not have a value.\n",Arg[1]);

	 // Use a pointer to this blank string to indicate a flag is flagged.
	 if (Opt->Flag == 1)
	    Value = "";
	 
	 // Store away the argument
	 if (Opt->Free & FREE_ARG != 0)
	    delete [] Opt->Arg;
	 
	 Opt->Free &= ~FREE_ARG;
	 Opt->Arg = Value;
	 continue;
      }      

      // If we are here then it is a file
      if (Files != 0)
	 Files->push_backv(argv[I]);
   }

   return 0;
}
									/*}}}*/
// IniOptions::AddOption - Adds a new option definition			/*{{{*/
// ---------------------------------------------------------------------
/* This is required to allow the class to parse command line options
   and ini files. */
int IniOptions::AddOption(const char *CmdOpt,const char *Section,
			  const char *Tag,int Flag,char ShortOpt,
			  const char *Arg)
{
   Option Opt;
   Opt.CmdShortOpt = ShortOpt;
   Opt.CmdOpt = CmdOpt;
   Opt.Section = Section;
   Opt.Tag = Tag;
   Opt.Arg = (char *)Arg;     // We can cast const away safely here
   Opt.Flag = Flag;
   Opt.Free = 0;

   Options.push_backv(Opt);
   return 0;
}
									/*}}}*/
   
