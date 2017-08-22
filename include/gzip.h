// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   GZip - This is a simple archiver class for Muse, it handles gzip'd
          files.

   gzip files are described as file.gz/file. There is no concept of a
   gzip path.
   
   ##################################################################### */
									/*}}}*/
#ifndef GZIP_H
#define GZIP_H

class museGZipClass;

/* We dont instantiate this, the metaclass does all the work, archive
   extraction is simple and stateless. */
class museGZipArchiver : public museArchiverBase
{
   public:
   static museGZipClass *Meta;
   inline virtual MetaClass *GetMetaClass();
};

class museGZipClass : public museArchiverClass
{
   public:

   virtual const char *GetTypeName() {return "GNU GZip";};
   
   // These do the actual work.
   virtual bool IsArchivePath(const char *Path);
   virtual bool IsArchiveFile(const char *Path);
   virtual void ProcessArchive(const char *Path,museSongList &List);
   virtual bool ExtractArchive(museSongItem &Item,octet *&Block,
				  unsigned long &Size);
   
   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museArchiverBase::Meta)
         return true;
      return museArchiverClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museGZipArchiver";};

   virtual void Link() {};
   museGZipClass();
};

MetaClass *museGZipArchiver::GetMetaClass() {return Meta;};

#endif
