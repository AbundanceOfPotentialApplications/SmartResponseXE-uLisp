/* 
 *  rev 14 - converted sectors to unsigned int 8, instead of byte-char confusion
 *  rev 13 - debugged on platform
 *  rev 11 - debugged in sim
 * */

#ifndef __SD_H__
#define __SD_H__

#ifndef LocalTest

#include <Arduino.h>
#include <SPI.h>

#else
//#include <string>
//using namespace std;
//#define String string
//#include <iostream>
#endif



//#include "utility/SdFat.h"
//#include "utility/SdFatUtil.h"

//#define FILE_READ O_READ
//#define FILE_WRITE (O_READ | O_WRITE | O_CREAT | O_APPEND)
//#define FILE_CREATE (O_READ | O_WRITE | O_CREAT )

#define O_READ 1
#define O_WRITE 2 
#define O_RDWR 2
#define O_CREAT 4
#define O_APPEND 8
#define O_TRUNC 16

/*
#define FILE_READ 1
#define FILE_WRITE 6
#define FILE_APPEND FILE_WRITE
#define FILE_CREATE 4
*/

// fixme: make this enum? 
#define MODE_CLOSED 0
#define MODE_READ 1
#define MODE_WRITE 2
#define MODE_DIR_READ 3
#define MODE_APPEND 4

#define SD_CHIP_SELECT_PIN 17
//17      [PD3:TXD1:INT3] - U2 pin 1, CS_Flash

/*#define FILE_READ 0
#define FILE_APPEND 1
#define FILE_CREATE 2
#define FILE_DELETE 3
*/


namespace SRXE_SDLib {
class File;
class SRXE_SDClass {

    private:
 
      int8_t last_seize=-1;
      uint8_t cs_pin=SD_CHIP_SELECT_PIN;
      uint8_t cmd_pin;
      uint32_t validSectors;
      // uint32_t freeSectors;

      //int8_t current_sector; // and read-write-mode.
 //     int16_t current_offset;

      bool selected;      
      bool transaction; // to do...
      
#ifndef LocalTest           

      void i_select();   // pull down the CS line and set up transfers
      void i_unselect();   // end pull down the CS line
      void i_reselect();   // pulse the CS line to trigger command execution

      bool i_check(int16_t); // check if flash ready for write
      bool i_wren(); // enable write
     // void i_command(uint8_t cmd);
      void i_send(uint8_t data);
      void i_wait();
      uint8_t i_receive();
      
      void i_commit(); // check if flash ready for write
      
      void i_address(int8_t sector, int16_t offset);  // oor static?c
      
#endif      
#ifdef SDlowLevelTest
public:
#endif
      int8_t i_write(uint8_t sector, int16_t offset, uint8_t value);
      uint8_t i_read(uint8_t sector, int16_t offset);
      //void i_write(int8_t sector);
      bool i_erase(uint8_t sector);

#ifdef SDlowLevelTest
private:
#endif
      
    
      uint8_t sector_seize(uint8_t first_sector,uint8_t escape); 
      void sector_erase(uint8_t sector);
      void sector_finish(uint8_t sector, uint8_t next_sector,uint16_t _size);
      
      // fixme:
      bool empty_sector(uint8_t sector);
      bool valid_sector(uint8_t sector);
      int32_t file_size(uint8_t sector);
      
      
      int8_t find_file(const char *name);
      
      bool i_remove(const uint8_t sector);
     
      //void select();   // pull down the CS line and set up transfers
      // start_with
      // end_with or sth

      //int8_t seek(char *name);
      uint8_t randomescape(uint8_t);
      uint16_t escape_seed;
      
      friend class File;

    public:
   //   void unselect(); // finish the CS line pull down
      // This needs to be called to set up the connection to the SD card
      // before other methods are used.
      bool begin(uint8_t csPin = SD_CHIP_SELECT_PIN);
// what's this for though?
//      bool begin(uint32_t clock, uint8_t csPin)  { 
//        return begin(csPin);
//     }

      //call this when a card is removed. It will allow you to insert and initialise a new card.
      void end();

      // Open the specified file/directory with the supplied mode (e.g. read or
      // write, etc). Returns a File object for interacting with the file.
      // Note that currently only one file can be open at a time.
      File open(const char *filename, uint8_t mode = O_READ);
      

      // Methods to determine if the requested file path exists.
      bool exists(const char *filepath);

      // Create the requested directory heirarchy--if intermediate directories
      // do not exist they will be created.
      bool mkdir(const char *filepath) {return false;}

      // Delete the file.
      bool remove(const char *filepath);
      
      

      bool rmdir(const char *filepath) {return false;}

/*
 *    fixme, wrappers for the progmem string?
      bool exists(const String &filepath) {
        return exists(filepath.c_str());
      }
      
      bool mkdir(const String &filepath) {
        return mkdir(filepath.c_str());
      }
      
      File open(const String &filename, uint8_t mode = FILE_READ) {
        return open(filename.c_str(), mode);
      }
      bool remove(const String &filepath) {
        return remove(filepath.c_str());
      }      
      
      bool rmdir(const String &filepath) {
        return rmdir(filepath.c_str());
      }*/
      
      operator bool()
      {
         return (last_seize>=0);
      }

  };

#ifdef LocalTest           
  class File {
#else        
  class File : public Stream {
#endif      
#ifdef SDlowLevelTest
public:
#else
    private:
#endif
      char my_name[13]; // our name
//      SdFile *_file;  // underlying file pointer
      uint8_t first_sector;  // first sector of the file
      uint8_t mode;   // mode- read, append, read-directory
      uint8_t escape; // escape char
      uint8_t candidate; // next escape char(for write) or peeked next char for read
      bool available_flag;
      uint8_t current_sector; // current sector
      int16_t current_pos; // reading/writing position in the current sector
      int16_t current_size; // current size of sector, in data bytes(excluding padding)

      //*fakeSDClass my_fs;
      int f_read();
      int8_t f_write(uint8_t);

    public:
//      File(SdFile f, const char *name);     // wraps an underlying SdFile
      File(void);      // 'empty' constructor - ie. false/failed
      File(int8_t sector, int8_t mode); // open existing file
      File(const char *name); // create and open new file.      

#ifdef LocalTest
      size_t write(uint8_t);
      int availableForWrite();
      int read();      
      int peek();
      int available();
      void flush();
#else
      virtual size_t write(uint8_t);
      using Print::write;   // these are assorted write functions, they use write(uint8_t) internally    
      virtual int availableForWrite();
      virtual int read();      
      virtual int peek();
      virtual int available();
      virtual void flush();
#endif
      
      
      int read(void *buf, uint16_t nbyte); // fixme, add bulk mode.
      bool seek(uint32_t pos);
      uint32_t position();
      uint32_t size();
      void close();
      operator bool()
      {
        return (mode != MODE_CLOSED);          
      }
      
      char * name();   // not necessary for lisp?

      bool isDirectory(void);   // not necessary for lisp?
      File openNextFile(uint8_t mode = O_READ);// not necessary for lisp?
      void rewindDirectory(void);// not necessary for lisp?

 
      
    private:
      friend class SRXE_SDClass;
      
  };


  extern SRXE_SDClass SD;

};

// We enclose File and SD classes in namespace SDLib to avoid conflicts
// with others legacy libraries that redefines File class.

// This ensure compatibility with sketches that uses only SD library
using namespace SRXE_SDLib;

// This allows sketches to use SDLib::File with other libraries (in the
// sketch you must use SDFile instead of File to disambiguate)
typedef SRXE_SDLib::File    SDFile;
typedef SRXE_SDLib::SRXE_SDClass SDFileSystemClass;
#define SDFileSystem   SRXE_SDLib::SD

#endif


/*
 * 
 * minimal surface interface - for use by lisp only...
 * 
 *   file.write(data & 0xFF); //file.write(data>>8 & 0xFF);
  uint8_t b0 = file.read(); uint8_t b1 = file.read();
  file = SD.open("/ULISP.IMG", O_RDWR | O_CREAT | O_TRUNC);
 file.close();
  if (!file) {}
    SD.begin(SDCARD_SS_PIN);
      file = SD.open(MakeFilename(arg, buffer), O_RDWR | O_CREAT | O_TRUNC);
      
          file = SD.open(MakeFilename(arg, buffer));
    int mode = 0;
  if (params != NULL && first(params) != NULL) mode = checkinteger(WITHSDCARD, first(params));
  int oflag = O_READ;
  if (mode == 1) oflag = O_RDWR | O_CREAT | O_APPEND; else if (mode == 2) oflag = O_RDWR | O_CREAT | O_TRUNC;
  if (mode >= 1) {
    char buffer[BUFFERSIZE];
    SDpfile = SD.open(MakeFilename(filename, buffer), oflag);
    if (!SDpfile) error2(WITHSDCARD, PSTR("problem writing to SD card or invalid filename"));        


*
* file.write(data & 0xFF);
  file.read();
  file = SD.open("/ULISP.IMG", O_RDWR | O_CREAT | O_TRUNC);
  file.close();
  if (!file) {}
  SD.begin(SDCARD_SS_PIN);
  * 
  * 
  * 
  file = SD.open(MakeFilename(arg, buffer), O_RDWR | O_CREAT | O_TRUNC);
  
  int oflag = O_READ;
  if (mode == 1) oflag = O_RDWR | O_CREAT | O_APPEND; else if (mode == 2) oflag = O_RDWR | O_CREAT | O_TRUNC;
  if (mode >= 1) {
    char buffer[BUFFERSIZE];
    SDpfile = SD.open(MakeFilename(filename, buffer), oflag);
    if (!SDpfile) error2(WITHSDCARD, PSTR("problem writing to SD card or invalid filename"));        

 * 
 */
