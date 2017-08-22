// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   CommandLine - A class to allow a set of independant classes to share
                 a command line.

   This class allows other entities to register a command line and INI
   file object. When the program is started the command line is compared
   to the global table, the local ini files are scanned and a complete
   description of all the program options is arrived at. After this point
   an option may be retrived from the class.
   
   Option addition may be performed in the metaclass constructor, or at
   any time after, though it makes little sense to add an option after
   the command line has been parsed ;>
   
   Each option is defined by 5 items,
      CmdShortOpt - Defines the single letter case sensitive command line
                    option associated with this. 0 means no command line
                    option is avail.
      CmdOpt - Defines the long command line option. This is in gnu style
                ie --longopt
      Section - Defines an INI file section
      Tag - Defines an INI file tag in the section.
      Flag - true if the option is a flag ie --value=Wank is not valid.
   
   This allows and option to optionally be specified on the command line 
   or in an ini file. This works best with a metaclass system or another
   way to perform startup object initialization. 
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef CMDLINE_H
#define CMDLINE_H

class IniOptions
{
   public:

   // This is a global instantiation of this class used for convience.
   static IniOptions *Ini;
   
   struct Option
   {
      char CmdShortOpt;        // Short Command line option
      const char *CmdOpt;      // Long Command line option
      const char *Section;     // INI file section
      const char *Tag;         // INI file tag.
      int Flag;                // 1 if this is a flag, 0 means it has an arg
      unsigned int Free;       // Bitmask of what is dynamically allocated.
      char *Arg;               // The options arg.
   };

   // List of all the options possible.
   Sequence<Option> Options;

   // Add a new option
   int AddOption(const char *CmdOpt,const char *Section,const char *Tag,
		 int Flag,char ShortOpt = 0,const char *Arg = 0);
   
   // Process the command line
   int CommandLine(int argc,char **argv,SequenceString *Files = 0);

   // Locates the option record.
   Option *FindOptLong(const char *CmdOpt);
   
   // Returns 1 if the flag specified by CmdOpt is set. -1 on error
   inline int IsFlag(const char *CmdOpt)
   {
      Option *Opt = FindOptLong(CmdOpt);
      if (Opt == 0)
	 return -1;
      if (Opt->Arg == 0)
	 return 0;
      return 1;
   };
   
   const char *GetArg(const char *CmdOpt,const char *Default = 0)
   {
      Option *Opt = FindOptLong(CmdOpt);
      if ((Opt == 0) || (Opt->Arg == 0))
	 return Default;
      return Opt->Arg;
   }

   IniOptions();
   ~IniOptions();
};

/* Simple inline to construct the global ini options class at first use.
   This should be used once per procedure, then the return value can be
   cached or IniOptions::Ini can be used (cache prefered) */
inline IniOptions *GetIniOptions()
{
   if (IniOptions::Ini == 0)
      IniOptions::Ini = new IniOptions;
   return IniOptions::Ini;
};

#endif
