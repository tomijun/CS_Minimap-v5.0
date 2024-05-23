/*

  ShadowFlare MPQ API Library. (c) ShadowFlare Software 2002

  All functions below are actual functions that are part of this
  library and do not need any additional dll files.  It does not
  even require Storm to be able to decompress or compress files.

  This library emulates the interface of Lmpqapi and Storm MPQ
  functions, so it may be used as a replacement for them in
  MPQ extractors/archivers without even needing to recompile
  the program that uses Lmpqapi or Storm.  It has a few features
  not included in Lmpqapi and Storm, such as extra flags for some
  functions, setting the locale ID of existing files, and adding
  files without having to write them somewhere else first.  Also,
  MPQ handles used by functions prefixed with "SFile" and "Mpq"
  can be used interchangably; all functions use the same type
  of MPQ handles.  You cannot, however, use handles from this
  library with storm or lmpqapi or vice-versa.  Doing so will
  most likely result in a crash.

  Revision History:
  06/12/2002 1.07 (ShadowFlare)
  - No longer requires Storm.dll to compress or decompress
    Warcraft III files
  - Added SFileListFiles for getting names and information
    about all of the files in an archive
  - Fixed a bug with renaming and deleting files
  - Fixed a bug with adding wave compressed files with
    low compression setting
  - Added a check in MpqOpenArchiveForUpdate for proper
    dwMaximumFilesInArchive values (should be a number that
    is a power of 2).  If it is not a proper value, it will
    be rounded up to the next higher power of 2

  05/09/2002 1.06 (ShadowFlare)
  - Compresses files without Storm.dll!
  - If Warcraft III is installed, this library will be able to
    find Storm.dll on its own. (Storm.dll is needed to
    decompress Warcraft III files)
  - Fixed a bug where an embedded archive and the file that
    contains it would be corrupted if the archive was modified
  - Able to open all .w3m maps now

  29/06/2002 1.05 (ShadowFlare)
  - Supports decompressing files from Warcraft III MPQ archives
    if using Storm.dll from Warcraft III
  - Added MpqAddFileToArchiveEx and MpqAddFileFromBufferEx for
    using extra compression types

  29/05/2002 1.04 (ShadowFlare)
  - Files can be compressed now!
  - Fixed a bug in SFileReadFile when reading data not aligned
    to the block size
  - Optimized some of SFileReadFile's code.  It can read files
    faster now
  - SFile functions may now be used to access files not in mpq
    archives as you can with the real storm functions
  - MpqCompactArchive will no longer corrupt files with the
    MODCRYPTKEY flag as long as the file is either compressed,
    listed in "(listfile)", is "(listfile)", or is located in
    the same place in the compacted archive; so it is safe
    enough to use it on almost any archive
  - Added MpqAddWaveFromBuffer
  - Better handling of archives with no files
  - Fixed compression with COMPRESS2 flag

  15/05/2002 1.03 (ShadowFlare)
  - Supports adding files with the compression attribute (does
    not actually compress files).  Now archives created with
    this dll can have files added to them through lmpqapi
    without causing staredit to crash
  - SFileGetBasePath and SFileSetBasePath work more like their
    Storm equivalents now
  - Implemented MpqCompactArchive, but it is not finished yet.
    In its current state, I would recommend against using it
    on archives that contain files with the MODCRYPTKEY flag,
    since it will corrupt any files with that flag
  - Added SFMpqGetVersionString2 which may be used in Visual
    Basic to get the version string

  07/05/2002 1.02 (ShadowFlare)
  - SFileReadFile no longer passes the lpOverlapped parameter it
    receives to ReadFile.  This is what was causing the function
    to fail when used in Visual Basic
  - Added support for more Storm MPQ functions
  - GetLastError may now be used to get information about why a
    function failed

  01/05/2002 1.01 (ShadowFlare)
  - Added ordinals for Storm MPQ functions
  - Fixed MPQ searching functionality of SFileOpenFileEx
  - Added a check for whether a valid handle is given when
    SFileCloseArchive is called
  - Fixed functionality of SFileSetArchivePriority when multiple
    files are open
  - File renaming works for all filenames now
  - SFileReadFile no longer reallocates the buffer for each block
    that is decompressed.  This should make SFileReadFile at least
    a little faster

  30/04/2002 1.00 (ShadowFlare)
  - First version.
  - Compression not yet supported
  - Does not use SetLastError yet, so GetLastError will not return any
    errors that have to do with this library
  - MpqCompactArchive not implemented

  This library is freeware, you can do anything you want with it but with
  one exception.  If you use it in your program, you must specify this fact
  in Help|About box or in similar way.  You can obtain version string using
  SFMpqGetVersionString call.

  THIS LIBRARY IS DISTRIBUTED "AS IS".  NO WARRANTY OF ANY KIND IS EXPRESSED
  OR IMPLIED. YOU USE AT YOUR OWN RISK. THE AUTHOR WILL NOT BE LIABLE FOR 
  DATA LOSS, DAMAGES, LOSS OF PROFITS OR ANY OTHER KIND OF LOSS WHILE USING
  OR MISUSING THIS SOFTWARE.

  Any comments or suggestions are accepted at blakflare@hotmail.com (ShadowFlare)
*/

#ifndef SHADOWFLARE_MPQ_API_INCLUDED
#define SHADOWFLARE_MPQ_API_INCLUDED

#include <windows.h>

#ifdef SFMPQAPI_EXPORTS
#define SFMPQAPI __declspec(dllexport)
#else
#define SFMPQAPI __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	WORD Major;
	WORD Minor;
	WORD Revision;
	WORD Subrevision;
} SFMPQVERSION;

// These no longer need to be called.  They only provided for
// compatibility with older versions of this code
extern void LoadSFMpq();
extern void FreeSFMpq();

// MpqInitialize does nothing.  It is only provided for
// compatibility with MPQ archivers that use lmpqapi.
typedef BOOL (WINAPI* funcMpqInitialize)();
extern funcMpqInitialize MpqInitialize;

typedef LPCSTR (WINAPI* funcMpqGetVersionString)();
typedef float  (WINAPI* funcMpqGetVersion)();
extern funcMpqGetVersionString MpqGetVersionString;
extern funcMpqGetVersion MpqGetVersion;

// SFMpqGetVersionString2's return value is the required length of the buffer plus
// the terminating null, so use SFMpqGetVersionString2(0, 0); to get the length.
typedef LPCSTR (WINAPI* funcSFMpqGetVersionString)();
typedef DWORD  (WINAPI* funcSFMpqGetVersionString2)(LPCSTR lpBuffer, DWORD dwBufferLength);
typedef SFMPQVERSION (WINAPI* funcSFMpqGetVersion)();
extern funcSFMpqGetVersionString SFMpqGetVersionString;
extern funcSFMpqGetVersionString2 SFMpqGetVersionString2;
extern funcSFMpqGetVersion SFMpqGetVersion;

// Returns 0 if the dll version is equal to the version your program was compiled
// with, 1 if the dll is newer, -1 if the dll is older.
extern long SFMpqCompareVersion();

// General error codes
#define MPQ_ERROR_MPQ_INVALID      0x85200065
#define MPQ_ERROR_FILE_NOT_FOUND   0x85200066
#define MPQ_ERROR_DISK_FULL        0x85200068 //Physical write file to MPQ failed. Not sure of exact meaning
#define MPQ_ERROR_HASH_TABLE_FULL  0x85200069
#define MPQ_ERROR_ALREADY_EXISTS   0x8520006A
#define MPQ_ERROR_BAD_OPEN_MODE    0x8520006C //When MOAU_READ_ONLY is used without MOAU_OPEN_EXISTING

#define MPQ_ERROR_COMPACT_ERROR    0x85300001

// MpqOpenArchiveForUpdate flags
#define MOAU_CREATE_NEW        0x00
#define MOAU_CREATE_ALWAYS     0x08 //Was wrongly named MOAU_CREATE_NEW
#define MOAU_OPEN_EXISTING     0x04
#define MOAU_OPEN_ALWAYS       0x20
#define MOAU_READ_ONLY         0x10 //Must be used with MOAU_OPEN_EXISTING
#define MOAU_MAINTAIN_LISTFILE 0x01

// MpqAddFileToArchive flags
#define MAFA_EXISTS           0x80000000 //Will be added if not present
#define MAFA_UNKNOWN40000000  0x40000000
#define MAFA_MODCRYPTKEY      0x00020000
#define MAFA_ENCRYPT          0x00010000
#define MAFA_COMPRESS         0x00000200
#define MAFA_COMPRESS2        0x00000100
#define MAFA_REPLACE_EXISTING 0x00000001

// MpqAddFileToArchiveEx compression flags
#define MAFA_COMPRESS_STANDARD 0x08 //Standard PKWare DCL compression
#define MAFA_COMPRESS_DEFLATE  0x02 //ZLib's deflate compression
#define MAFA_COMPRESS_WAVE     0x81 //Standard wave compression
#define MAFA_COMPRESS_WAVE2    0x41 //Unused wave compression

// Flags for individual compression types used for wave compression
#define MAFA_COMPRESS_WAVECOMP1 0x80 //Main compressor for standard wave compression
#define	MAFA_COMPRESS_WAVECOMP2 0x40 //Main compressor for unused wave compression
#define MAFA_COMPRESS_WAVECOMP3 0x01 //Secondary compressor for wave compression

// ZLib deflate compression level constants (used with MpqAddFileToArchiveEx and MpqAddFileFromBufferEx)
#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)

// MpqAddWaveToArchive quality flags
#define MAWA_QUALITY_HIGH    1
#define MAWA_QUALITY_MEDIUM  0
#define MAWA_QUALITY_LOW     2

// SFileGetFileInfo flags
#define SFILE_INFO_BLOCK_SIZE      0x01 //Block size in MPQ
#define SFILE_INFO_HASH_TABLE_SIZE 0x02 //Hash table size in MPQ
#define SFILE_INFO_NUM_FILES       0x03 //Number of files in MPQ
#define SFILE_INFO_TYPE            0x04 //Is MPQHANDLE a file or an MPQ?
#define SFILE_INFO_SIZE            0x05 //Size of MPQ or uncompressed file
#define SFILE_INFO_COMPRESSED_SIZE 0x06 //Size of compressed file
#define SFILE_INFO_FLAGS           0x07 //File flags (compressed, etc.), file attributes if a file not in an archive
#define SFILE_INFO_PARENT          0x08 //Handle of MPQ that file is in
#define SFILE_INFO_POSITION        0x09 //Position of file pointer in files
#define SFILE_INFO_LOCALEID        0x0A //Locale ID of file in MPQ
#define SFILE_INFO_PRIORITY        0x0B //Priority of open MPQ
#define SFILE_INFO_HASH_INDEX      0x0C //Hash index of file in MPQ

// SFileListFiles flags
#define SFILE_LIST_MEMORY_LIST  0x01 // Specifies that lpFilelists is a file list from memory, rather than being a list of file lists
#define SFILE_LIST_ONLY_KNOWN   0x02 // Only list files that the function finds a name for
#define SFILE_LIST_ONLY_UNKNOWN 0x04 // Only list files that the function does not find a name for

#define SFILE_TYPE_MPQ  0x01
#define SFILE_TYPE_FILE 0x02

#define SFILE_OPEN_HARD_DISK_FILE 0x0000 //Open archive without regard to the drive type it resides on
#define SFILE_OPEN_CD_ROM_FILE    0x0001 //Open the archive only if it is on a CD-ROM
#define SFILE_OPEN_ALLOW_WRITE    0x8000 //Open file with write access

#define SFILE_SEARCH_CURRENT_ONLY 0x00 //Used with SFileOpenFileEx; only the archive with the handle specified will be searched for the file
#define SFILE_SEARCH_ALL_OPEN     0x01 //SFileOpenFileEx will look through all open archives for the file

typedef HANDLE MPQHANDLE;

struct FILELISTENTRY {
	DWORD dwFileExists; // Nonzero if this entry is used
	LCID lcLocale; // Locale ID of file
	DWORD dwCompressedSize; // Compressed size of file
	DWORD dwFullSize; // Uncompressed size of file
	DWORD dwFlags; // Flags for file
	char szFileName[260];
};

struct MPQARCHIVE;
struct MPQFILE;
struct MPQHEADER;
struct BLOCKTABLEENTRY;
struct HASHTABLEENTRY;

struct MPQHEADER {
	DWORD dwMPQID; //"MPQ\x1A" for mpq's, "BN3\x1A" for bncache.dat
	DWORD dwHeaderSize; // Size of this header
	DWORD dwMPQSize; //The size of the mpq archive
	WORD wUnused0C; // Seems to always be 0
	WORD wBlockSize; // Size of blocks in files equals 512 << wBlockSize
	DWORD dwHashTableOffset; // Offset to hash table
	DWORD dwBlockTableOffset; // Offset to block table
	DWORD dwHashTableSize; // Number of entries in hash table
	DWORD dwBlockTableSize; // Number of entries in block table
};

//Archive handles may be typecasted to this struct so you can access
//some of the archive's properties and the decrypted hash table and
//block table directly.
struct MPQARCHIVE {
	// Arranged according to priority with lowest priority first
	MPQARCHIVE * lpNextArc; // Pointer to the next ARCHIVEREC struct. Pointer to addresses of first and last archives if last archive
	MPQARCHIVE * lpPrevArc; // Pointer to the previous ARCHIVEREC struct. 0xEAFC5E23 if first archive
	char szFileName[260]; // Filename of the archive
	HANDLE hFile; // The archive's file handle
	DWORD dwFlags1; // Some flags, bit 1 (0 based) seems to be set when opening an archive from a CD
	DWORD dwPriority; // Priority of the archive set when calling SFileOpenArchive
	MPQFILE * lpLastReadFile; // Pointer to the last read file's FILEREC struct. Only used for incomplete reads of blocks
	DWORD dwUnk; // Seems to always be 0
	DWORD dwBlockSize; // Size of file blocks in bytes
	BYTE * lpLastReadBlock; // Pointer to the read buffer for archive. Only used for incomplete reads of blocks
	DWORD dwBufferSize; // Size of the read buffer for archive. Only used for incomplete reads of blocks
	DWORD dwMPQStart; // The starting offset of the archive
	MPQHEADER * lpMPQHeader; // Pointer to the archive header
	BLOCKTABLEENTRY * lpBlockTable; // Pointer to the start of the block table
	HASHTABLEENTRY * lpHashTable; // Pointer to the start of the hash table
	DWORD dwFileSize; // The size of the file in which the archive is contained
	DWORD dwOpenFiles; // Count of files open in archive + 1
	MPQHEADER MpqHeader;
	DWORD dwFlags; //The only flag that should be changed is MOAU_MAINTAIN_LISTFILE
	LPSTR lpFileName;
};

//Handles to files in the archive may be typecasted to this struct
//so you can access some of the file's properties directly.
struct MPQFILE {
	MPQFILE * lpNextFile; // Pointer to the next FILEREC struct. Pointer to addresses of first and last files if last file
	MPQFILE * lpPrevFile; // Pointer to the previous FILEREC struct. 0xEAFC5E13 if first file
	char szFileName[260]; // Filename of the file
	HANDLE hPlaceHolder; // Always 0xFFFFFFFF
	MPQARCHIVE * lpParentArc; // Pointer to the ARCHIVEREC struct of the archive in which the file is contained
	BLOCKTABLEENTRY * lpBlockEntry; // Pointer to the file's block table entry
	DWORD dwCryptKey; // Decryption key for the file
	DWORD dwFilePointer; // Position of file pointer in the file
	DWORD dwUnk1; // Seems to always be 0
	DWORD dwBlockCount; // Number of blocks in file
	DWORD * lpdwBlockOffsets; // Offsets to blocks in file. There are 1 more of these than the number of blocks
	DWORD dwReadStarted; // Set to 1 after first read
	DWORD dwUnk2; // Seems to always be 0
	BYTE * lpLastReadBlock; // Pointer to the read buffer for file. Only used for incomplete reads of blocks
	DWORD dwBytesRead; // Total bytes read from open file
	DWORD dwBufferSize; // Size of the read buffer for file. Only used for incomplete reads of blocks
	DWORD dwConstant; // Seems to always be 1
	HASHTABLEENTRY *lpHashEntry;
	LPSTR lpFileName;
};

struct BLOCKTABLEENTRY {
	DWORD dwFileOffset; // Offset to file
	DWORD dwCompressedSize; // Compressed size of file
	DWORD dwFullSize; // Uncompressed size of file
	DWORD dwFlags; // Flags for file
};

struct HASHTABLEENTRY {
	DWORD dwNameHashA; // First name hash of file
	DWORD dwNameHashB; // Second name hash of file
	LCID lcLocale; // Locale ID of file
	DWORD dwBlockTableIndex; // Index to the block table entry for the file
};

//Archive handles may be typecasted to this struct so you can access
//some of the archive's properties and the decrypted hash table and
//block table directly.
/*typedef struct {
	MPQHEADER MpqHeader;
	HASHTABLEENTRY *lpHashTable;
	BLOCKTABLEENTRY *lpBlockTable;
	DWORD dwStart; //Offset to the archive's header
	HANDLE hFile; //The archive's file handle
	LPSTR lpFileName;
	DWORD dwPriority; //When searching for a file in open archives, ones with higher priority are checked first
	DWORD dwFlags; //The only flag that should be changed is MOAU_MAINTAIN_LISTFILE
} MPQARCHIVE;

//Handles to files in the archive may be typecasted to this struct
//so you can access some of the file's properties directly.
typedef struct {
	HASHTABLEENTRY *lpHashEntry;
	MPQHANDLE hMPQ;
	LPSTR lpFileName;
	DWORD FilePointer;
	DWORD *dwBlockStart;
	DWORD dwCryptKey;
} MPQFILE;*/

// Defines for backward compatability with old lmpqapi function names
#define MpqAddFileToArcive MpqAddFileToArchive
#define MpqOpenArchive     SFileOpenArchive
#define MpqOpenFileEx      SFileOpenFileEx
#define MpqGetFileSize     SFileGetFileSize
#define MpqReadFile        SFileReadFile
#define MpqCloseFile       SFileCloseFile
#define MpqCloseArchive    SFileCloseArchive

// Storm functions implemented by this library
typedef BOOL      (WINAPI* funcSFileOpenArchive)(LPCSTR lpFileName, DWORD dwPriority, DWORD dwFlags, MPQHANDLE *hMPQ);
typedef BOOL      (WINAPI* funcSFileCloseArchive)(MPQHANDLE hMPQ);
typedef BOOL      (WINAPI* funcSFileGetArchiveName)(MPQHANDLE hMPQ, LPCSTR lpBuffer, DWORD dwBufferLength);
typedef BOOL      (WINAPI* funcSFileOpenFile)(LPCSTR lpFileName, MPQHANDLE *hFile);
typedef BOOL      (WINAPI* funcSFileOpenFileEx)(MPQHANDLE hMPQ, LPCSTR lpFileName, DWORD dwSearchScope, MPQHANDLE *hFile);
typedef BOOL      (WINAPI* funcSFileCloseFile)(MPQHANDLE hFile);
typedef DWORD     (WINAPI* funcSFileGetFileSize)(MPQHANDLE hFile, LPDWORD lpFileSizeHigh);
typedef BOOL      (WINAPI* funcSFileGetFileArchive)(MPQHANDLE hFile, MPQHANDLE *hMPQ);
typedef BOOL      (WINAPI* funcSFileGetFileName)(MPQHANDLE hFile, LPCSTR lpBuffer, DWORD dwBufferLength);
typedef DWORD     (WINAPI* funcSFileSetFilePointer)(MPQHANDLE hFile, long lDistanceToMove, PLONG lplDistanceToMoveHigh, DWORD dwMoveMethod);
typedef BOOL      (WINAPI* funcSFileReadFile)(MPQHANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped);
typedef LCID      (WINAPI* funcSFileSetLocale)(LCID nNewLocale);
typedef BOOL      (WINAPI* funcSFileGetBasePath)(LPCSTR lpBuffer, DWORD dwBufferLength);
typedef BOOL      (WINAPI* funcSFileSetBasePath)(LPCSTR lpNewBasePath);
extern funcSFileOpenArchive SFileOpenArchive;
extern funcSFileCloseArchive SFileCloseArchive;
extern funcSFileGetArchiveName SFileGetArchiveName;
extern funcSFileOpenFile SFileOpenFile;
extern funcSFileOpenFileEx SFileOpenFileEx;
extern funcSFileCloseFile SFileCloseFile;
extern funcSFileGetFileSize SFileGetFileSize;
extern funcSFileGetFileArchive SFileGetFileArchive;
extern funcSFileGetFileName SFileGetFileName;
extern funcSFileSetFilePointer SFileSetFilePointer;
extern funcSFileReadFile SFileReadFile;
extern funcSFileSetLocale SFileSetLocale;
extern funcSFileGetBasePath SFileGetBasePath;
extern funcSFileSetBasePath SFileSetBasePath;

// Extra storm-related functions
typedef DWORD     (WINAPI* funcSFileGetFileInfo)(MPQHANDLE hFile, DWORD dwInfoType);
typedef BOOL      (WINAPI* funcSFileSetArchivePriority)(MPQHANDLE hMPQ, DWORD dwPriority);
typedef DWORD     (WINAPI* funcSFileFindMpqHeader)(HANDLE hFile);
typedef BOOL      (WINAPI* funcSFileListFiles)(MPQHANDLE hMPQ, LPCSTR lpFileLists, FILELISTENTRY *lpListBuffer, DWORD dwFlags);
extern funcSFileGetFileInfo SFileGetFileInfo;
extern funcSFileSetArchivePriority SFileSetArchivePriority;
extern funcSFileFindMpqHeader SFileFindMpqHeader;
extern funcSFileListFiles SFileListFiles;

// Archive editing functions implemented by this library
typedef MPQHANDLE (WINAPI* funcMpqOpenArchiveForUpdate)(LPCSTR lpFileName, DWORD dwFlags, DWORD dwMaximumFilesInArchive);
typedef DWORD     (WINAPI* funcMpqCloseUpdatedArchive)(MPQHANDLE hMPQ, DWORD dwUnknown2);
typedef BOOL      (WINAPI* funcMpqAddFileToArchive)(MPQHANDLE hMPQ, LPCSTR lpSourceFileName, LPCSTR lpDestFileName, DWORD dwFlags);
typedef BOOL      (WINAPI* funcMpqAddWaveToArchive)(MPQHANDLE hMPQ, LPCSTR lpSourceFileName, LPCSTR lpDestFileName, DWORD dwFlags, DWORD dwQuality);
typedef BOOL      (WINAPI* funcMpqRenameFile)(MPQHANDLE hMPQ, LPCSTR lpcOldFileName, LPCSTR lpcNewFileName);
typedef BOOL      (WINAPI* funcMpqDeleteFile)(MPQHANDLE hMPQ, LPCSTR lpFileName);
typedef BOOL      (WINAPI* funcMpqCompactArchive)(MPQHANDLE hMPQ);
extern funcMpqOpenArchiveForUpdate MpqOpenArchiveForUpdate;
extern funcMpqCloseUpdatedArchive MpqCloseUpdatedArchive;
extern funcMpqAddFileToArchive MpqAddFileToArchive;
extern funcMpqAddWaveToArchive MpqAddWaveToArchive;
extern funcMpqRenameFile MpqRenameFile;
extern funcMpqDeleteFile MpqDeleteFile;
extern funcMpqCompactArchive MpqCompactArchive;

// Extra archive editing functions
typedef BOOL      (WINAPI* funcMpqAddFileToArchiveEx)(MPQHANDLE hMPQ, LPCSTR lpSourceFileName, LPCSTR lpDestFileName, DWORD dwFlags, DWORD dwCompressionType, DWORD dwCompressLevel);
typedef BOOL      (WINAPI* funcMpqAddFileFromBufferEx)(MPQHANDLE hMPQ, LPVOID lpBuffer, DWORD dwLength, LPCSTR lpFileName, DWORD dwFlags, DWORD dwCompressionType, DWORD dwCompressLevel);
typedef BOOL      (WINAPI* funcMpqAddFileFromBuffer)(MPQHANDLE hMPQ, LPVOID lpBuffer, DWORD dwLength, LPCSTR lpFileName, DWORD dwFlags);
typedef BOOL      (WINAPI* funcMpqAddWaveFromBuffer)(MPQHANDLE hMPQ, LPVOID lpBuffer, DWORD dwLength, LPCSTR lpFileName, DWORD dwFlags, DWORD dwQuality);
typedef BOOL      (WINAPI* funcMpqSetFileLocale)(MPQHANDLE hMPQ, LPCSTR lpFileName, LCID nOldLocale, LCID nNewLocale);
extern funcMpqAddFileToArchiveEx MpqAddFileToArchiveEx;
extern funcMpqAddFileFromBufferEx MpqAddFileFromBufferEx;
extern funcMpqAddFileFromBuffer MpqAddFileFromBuffer;
extern funcMpqAddWaveFromBuffer MpqAddWaveFromBuffer;
extern funcMpqSetFileLocale MpqSetFileLocale;

// These functions do nothing.  They are only provided for
// compatibility with MPQ extractors that use storm.
typedef BOOL      (WINAPI* funcSFileDestroy)();
typedef void      (WINAPI* funcStormDestroy)();
extern funcSFileDestroy SFileDestroy;
extern funcStormDestroy StormDestroy;

#ifdef __cplusplus
};  // extern "C" 
#endif

#endif

