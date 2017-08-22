// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   SongList - Class to encapsulate a song list
   
   The songlist class is pretty simple, it has a function that will add
   a given path to the song list. If a directory is given then dir\* is 
   assumed. Archive support is handled transparently through the archive
   metaclass tree.
   
   Archiver support is pretty comprehensive, a wildcard path into an 
   archive can be given and expanded. This introduces the notion of
   an archive path (zip path) ie
     \songs.zip\s3m\k_macro.s3m

   would be a zip path. When added to a song list it will be determined that
   \songs.zip is an archive and the archive processor will separate and
   scan the two components.
   
   ##################################################################### */
									/*}}}*/
#ifndef SONGLIST_H
#define SONGLIST_H

// This is a single item in the song list
#define SITEM_Dir     (1 << 0)
#define SITEM_Archive (1 << 1)
class museSongItem
{
   public:
   
   char *FileName;
   unsigned long Flags;
   unsigned long Size;
   unsigned long CompressedSize;

   void Free();
   museSongItem();
};

#define SequenceSongItem Sequence<museSongItem>

class museSongList;
class museArchiverBase;
class museArchiverClass;

class museSongList
{
   SequenceSongItem List;
   
   void WildCards(const char *Path,SequenceSongItem &List);
   
   public:
   
   // Directory separator to be used.
   static char DirSep;
   
   bool MatchWild(const char *String,const char *Pattern);
   bool AddItem(museSongItem &Item);
   unsigned long Add(const char *Path);
   bool Load(museSongItem &Item,octet *&Block,unsigned long &Size);
   
   // Some STL Stuff to make it easy to manipulate the song list.
   typedef SequenceSongItem::iterator iterator;
   inline iterator begin() {return List.begin();};
   inline iterator end() {return List.end();};
};

/* We dont instantiate this, the metaclass does all the work, archive
   extraction is simple and stateless. */
class museArchiverBase : public MetaObject
{
   public:
   static museArchiverClass *Meta;
   inline virtual MetaClass *GetMetaClass();
};

class museArchiverClass : public MetaClass
{
   public:

   virtual const char *GetTypeName() {return 0;};
   
   // These do the actual work.
   virtual bool IsArchivePath(const char *) {return false;};
   virtual bool IsArchiveFile(const char *) {return false;};
   virtual void ProcessArchive(const char *,museSongList &) {};
   virtual bool ExtractArchive(museSongItem &,octet *&,unsigned long &) 
   {return false;};
  
   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museArchiverBase::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museArchiverBase";};

   virtual void Link() {};
};

MetaClass *museArchiverBase::GetMetaClass() {return Meta;};

#endif
