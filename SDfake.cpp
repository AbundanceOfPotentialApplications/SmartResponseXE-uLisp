/* 
 *  rev 14 - converted sectors to unsigned int 8, instead of byte-char confusion
 *  rev 13 - debugged on platform
 *  rev 11 - debugged in sim
 * */
 
#define POS_MAGIC_BYTE 0
#define POS_ERASE_COUNTER 1
#define POS_FIRST_SECTOR 7
#define POS_ESCAPE 8
#define POS_NEXT_SECTOR 9
#define POS_SECTOR_SIZE 10
#define POS_DATA_START  12
#define POS_FIRST_DATA_START 26
#define POS_LAST 4095

 

 /*
#define FLASH_CMD_WRITE 
#define FLASH_CMD_UNLOCK 
*/
//#define FLASH_CMD_READ 

#define FLASH_CMD_WREN 6
#define FLASH_CMD_CHECK 5
//#define FLASH_CMD_ERASE 0x20

#define FLASH_MASK_READY 1
// fixme, check the ready condition

#include "SDfake.h"
 
/*
 * sector header
 * pos  size
 * 0    1b  magic byte
 * 1    3b  erase counter (- 16.5M)
 * 4    3b  reserved, for validity/last write indicator 
 *            extended erase counter
 *            bad sector map or sth.
 * 7    1b  first sector
 * 8    1b  escape character
 * 9    1b  next sector
 * 10   2b  sector data size
 * 
 * if first sector:
 * 12   12b name
 * 24   2b  extra: type + reserved
 * 26   X   data
 */ 
 /*
  * File constructors: 
  * () - false, empty file, placeholder too
  * (name,sector) - create new file, for writing (return false if no space)
  * (sector, mode) - open existing file, for reading, or for write-append (includes seeking to the end)
  * (name) - open existing file(by name) or return false file.
  */
  
  
/*
 * Only the main directory is handled, names: ".","/","\"
 *  sector holds next sector to read.
 *  
 * note: not-quite-compliant applications,
 *   reading the directory as data (ie. with read instead of openNext)
 *   returns the filenames and sizes as text
 * 
 */
 
 #ifdef LocalTest
    uint8_t flashData[0x20000];
    //4000 = 12bits
    //32 =5  bits
    //=17 bits
          
#else       
//#include "SRXE_SPI.h"
#include <Arduino.h> 
#endif

#ifdef debugEOF
void dbgp(uint8_t c);
void dbgh(uint8_t c);
void dbgi(uint16_t c);
void dbgnl();
#endif

namespace SRXE_SDLib {
 
     uint8_t SRXE_SDClass::sector_seize(uint8_t first_sector,uint8_t escape)
     // includes: finding an empty sector, erasing, bookkeeping the counters and flags
     // setting escape character, and first sector
     // returns -1 on failure.
     {
       uint8_t sector=last_seize;
       do
       {
         if (empty_sector(sector))
         {
           // fixme todo book keeping?  or during begin?
           //?if (!erased(sector)) erase_sector(sector);
           if (first_sector & 0x80) first_sector = sector;
           SD.i_write(sector,POS_FIRST_SECTOR,first_sector);
           SD.i_write(sector,POS_ESCAPE,escape);

#ifdef LocalTest
        fprintf(stderr," seized sector %ihh\n",sector);
#endif                    
           last_seize=sector;
           return sector;
         }         
         sector=(sector+1) & 31;
       } while (sector!=last_seize);
       return -1;            
     }
          
     void SRXE_SDClass::sector_finish(uint8_t sector, uint8_t next_sector, uint16_t data_size)
     // stores the
     {
       SD.i_write(sector,POS_NEXT_SECTOR,next_sector);  
       SD.i_write(sector,POS_SECTOR_SIZE,data_size>>8);
       SD.i_write(sector,POS_SECTOR_SIZE+1,data_size&0xff);
      // SD.i_commit();
     }
     
     void SRXE_SDClass::sector_erase(uint8_t sector)
     {
       uint32_t counter;
       uint8_t magic_byte;
       
       magic_byte=SD.i_read(sector,POS_MAGIC_BYTE);
       if (magic_byte == (sector | 0x80))
         {
           counter = SD.i_read(sector,POS_ERASE_COUNTER) << 16;
           counter |= SD.i_read(sector,POS_ERASE_COUNTER+1) << 8;
           counter |= SD.i_read(sector,POS_ERASE_COUNTER+2);
           if (counter<0x00ffffff) counter++;
         }
         
       if (magic_byte == (0xff))
         { //meh. could check if the whole sector is already erased too,
          // and just don't erase again here
           counter = 1;  
         }
        i_erase(sector);
        SD.i_write(sector,POS_MAGIC_BYTE, sector|0x80);
        SD.i_write(sector,POS_ERASE_COUNTER, counter >> 16);
        SD.i_write(sector,POS_ERASE_COUNTER+1, counter>>8);
        SD.i_write(sector,POS_ERASE_COUNTER+2, counter);
        //i_commit();
     }

#ifndef LocalTest     
     void SRXE_SDClass::i_select()
     {       
       digitalWrite(cs_pin,LOW);         
     }
     
     void SRXE_SDClass::i_unselect()
     {
       digitalWrite(cs_pin,HIGH);
     }

     void SRXE_SDClass::i_reselect()
     {
    //   int t;
       digitalWrite(cs_pin,HIGH);
   //    i_wait();
       digitalWrite(cs_pin,LOW);         
     }
     
     void SRXE_SDClass::i_send(uint8_t data)
     {     
      #ifdef hackermode
  i_wait(); this probably doesnt even work though.
  SPDR = data;                    // Start the transmission
  #else
  SPDR = data;                    // Start the transmission
  i_wait();
  #endif
     }

     uint8_t SRXE_SDClass::i_receive()
     {
      #ifdef hackermode
      i_wait(); 
  SPDR = 0;                    // Start the transmission
  i_wait();     // Wait for the end of the transmission
 #else
SPDR = 0;                    // Start the transmission
  i_wait();     // Wait for the end of the transmission
 #endif;
  return SPDR;                    // return the received byte
     //   return SPI_transfer(0);             
     }
     
     void SRXE_SDClass::i_commit()
     { // starts the actual write.
        i_unselect();
     }
     
     bool SRXE_SDClass::i_check(int16_t maxdelay)
     {
       uint8_t res;
        //i_select();  // ? reselect? fixme?
        do {
          i_select();
          i_send(FLASH_CMD_CHECK);
          res=i_receive();
          i_unselect();
         
          if ((res & FLASH_MASK_READY) == 0)
          {
            return true;
          }
          maxdelay-=10;
          delay(10);
        } while (maxdelay>0);

        return false;       
     }

    
     
     void SRXE_SDClass::i_address(int8_t sector,int16_t pos)
     {
       i_send(sector>>4);
       i_send((sector<<4) | (pos>>8));
       i_send(pos & 0xff);       
       i_wait();
     }

     void SRXE_SDClass::i_wait()
     {
       while (!(SPSR & (1<<SPIF)))     // Wait for the end of the transmission
  {
  };
    //  while (!(SPSR & (1<<SPIF))) ;    // Wait for the end of the transmission
     }
#endif    
     
    
     
     uint8_t SRXE_SDClass::randomescape(uint8_t old)
     {
       uint8_t newescape;
       escape_seed= escape_seed*13+old+111;
       escape_seed = escape_seed%4920;
      // newescape = ((escape_seed%126+old+1)%127) | 0x80;
       // fixme, double check the math.
       newescape = (escape_seed %126) | 0x80;
       if (newescape >= old) newescape++;
       //if ((0x80 | escape_seed%127) == old
       return newescape;
     }

int8_t static_s;
int16_t static_p;
     
     int8_t SRXE_SDClass::i_write(uint8_t sector,int16_t pos, uint8_t data) // hardcore mode
     { // fixme todo: add transaction mode for multiple writes in a row
       if (sector & 0x80) return 0xff;
       //if (sector>31) return 0xff;
       if (pos>=0x1000) return 0xff;
       if (pos<0) return 0xff; //?
             
#ifdef LocalTest
        flashData[(sector<<12) | (pos & 0xfff)] &= data;        
        if (flashData[(sector<<12) | (pos & 0xfff)] != data) fprintf(stderr,"write warning, some bits cleared already\n");        
        
#ifdef Debug
       if ((static_s!=sector) || (static_p+1 != pos))
       {
         fprintf(stderr,"\n%i:%i ",sector,pos);
       }
       static_s=sector;
       static_p=pos;
       if ((data>=32) && (data<=127))  { putc(data,stderr); }
         else { putc('%',stderr); }
#endif        
        
        
        
#else              
       if (!i_check(1000)) return 0;

       i_select();
       i_send(0x01); // cmd clear protection bits
       i_send(0x00); // clear
       i_reselect();
       i_send(0x06);
       i_reselect();
       i_send(0x02); // cmd page write
       i_address(sector,pos);
       i_send(data);
       i_unselect(); // if not transaction and not page change
       i_check(20);
#endif       
       return 1;
     }
          
     
     
     uint8_t SRXE_SDClass::i_read(uint8_t sector,int16_t pos)
     { // fixme todo: add transaction mode for multiple reads in a row
       uint8_t v;
       
       if (sector & 0x80) return 0xff;
       //if (sector>31) return 0xff;
       if (pos>=0x1000) return 0xff;
       if (pos<0) return 0xff; //?

#ifdef LocalTest
        v=flashData[(sector<<12) | (pos & 0xfff)];
#else              

       i_check(100);
       i_select();
       i_send(0x03); // read
       i_address(sector,pos);
       v=i_receive();
       i_unselect();
       
#endif
       return v;       
     }  
       
     bool SRXE_SDClass::i_erase(uint8_t sector)     
     { // erase sector content
       int cnt = 0;
       if (sector>31) return false;
#ifdef LocalTest
        for(cnt=0;cnt<0x1000;cnt++)
          flashData[(sector<<12) | (cnt)] = 0xff;        
#else  
       if (!i_check(1000)) return false; // fixme error?                          
       
       i_select();
       i_send(0x06); // wren
       i_reselect();
       i_send(0x20); // sector erase       
       i_address(sector,0);       
       i_unselect();
       i_check(1000);
    
#endif       
  return true;
     }
 
 
      File::File(void)      // 'empty' constructor
      {
#ifdef Debug
        fprintf(stderr,"File::File(void)\n");
#endif        
        
        my_name[0]=0;
        mode=MODE_CLOSED;
      }
        
      File::File(int8_t sector, int8_t _mode)  
      // open existing file for read or append
      {
#ifdef Debug
        fprintf(stderr,"File::File(sector,mode)\n");
#endif        
        if (_mode==MODE_DIR_READ)
        {
#ifdef Debug
        fprintf(stderr,"dir read\n");
#endif 
          mode=MODE_DIR_READ;
          current_sector=-1;
          current_pos=100;
          return;
        }
  

       
        for(int c=0;c<12;c++)
        {
          my_name[c]=SD.i_read(sector,POS_DATA_START+c);
        } 
        my_name[12]=0;  
        escape=127;
        candidate=254;
        available_flag=false;
        
        if (_mode == MODE_READ)
        {
          mode=MODE_READ;  
          first_sector= sector;
          current_sector= sector;
          current_pos=POS_FIRST_DATA_START;
          
         // candidate=254;          
        }
        else
        {
        // fixme, seek the end of file !!! fixme
          first_sector=sector;
          //current_sector=SD.i_read(first_sector,POS_NEXT_SECTOR);
          //current_pos=POS_FIRST_DATA_START;
          
          int8_t next_sector=first_sector;
          do {
            current_sector=next_sector;
            next_sector=SD.i_read(current_sector,POS_NEXT_SECTOR);
          } while ((next_sector & 0x80) == 0);
        
          if (current_sector==first_sector) { current_pos=POS_FIRST_DATA_START; }
            else  { current_pos=POS_DATA_START; }
          escape=SD.i_read(current_sector,POS_ESCAPE);
          candidate=SD.randomescape(escape);
          current_size=0;
          while(1)
          {
            if (SD.i_read(current_sector,current_pos) == 0xff)
            {
              if (SD.i_read(current_sector,current_pos+1) != 0xff)
              {
                escape=SD.i_read(current_sector,current_pos+1);
                current_pos++;
              }
              else
              {
                break;
              }
              
            }  
            current_pos++;
            // fixme, is position off by one?
            // check read and writes
          }
          
          
          mode=MODE_WRITE;          
        }
                
      }

    File::File(const char *_name)
    {
    // create and open a new file
      uint8_t  sector;
#ifdef Debug
        fprintf(stderr,"File::File(name)\n");
#endif              
      
      escape=127; 
      candidate=254;
      sector= SD.sector_seize(-1,escape);  
      first_sector = sector;
      if (sector & 0x80)
      {
        mode=MODE_CLOSED;
        return; 
      }      
      available_flag=false;
      mode=MODE_WRITE;
      current_sector=first_sector;
      current_pos=POS_FIRST_DATA_START;

      for(int c=0;c<12;c++)
      {
        SD.i_write(sector,POS_DATA_START+c,_name[c]);
        my_name[c]=_name[c];
      } 
      
      
      
    }
        
      

   //SD.i_read() for pos >=4096 or invalid sector returns 0xff
   // pos,sector points at the last byte actually read.
   
   
   
   int File::f_read()
   {
     uint8_t v;

    // dbg p('-');
     if (current_sector == 0xff)
     {
     // dbg p('X');
      mode=MODE_CLOSED;
      return EOF;
     }
     
     v=SD.i_read(current_sector,current_pos);  
    // dbg h(v);
     current_pos++;
     if (v == escape) {
     // dbg p('E');
      return 0xff;
     }
     if (0xff == v)
     {
      // dbg p('F');       
       v=escape; 
      // dbg h(v);
      // dbg p(' '); 
       escape=SD.i_read(current_sector,current_pos);
      // dbg p('E'); 
      // dbg h(escape);
       current_pos++;
       if (0xff == escape)
         {
          // dbg p('X'); 
           current_sector = SD.i_read(current_sector,POS_NEXT_SECTOR);
          // dbg h(current_sector); 
          // dbg p(' ');
           if (current_sector == 0xff) {
           // dbg p('*');
            mode=MODE_CLOSED;
            return EOF; 
           }
          // dbg p('E');
           escape = SD.i_read(current_sector,POS_ESCAPE);
          // dbg h(escape);
           current_pos = POS_DATA_START; 
          // dbg p('@');       
           return f_read();
         }
         
     }
    // dbg h(v);
    // dbg p('i');
     return v;
     
   }
   
   
   int8_t File::f_write(uint8_t v)
   {
     if (((v==escape) && (current_pos>= POS_LAST)) ||
          (current_pos> POS_LAST)) 
     {
       int8_t next_sector = SD.sector_seize(first_sector,escape); //
       if (next_sector & 0x80) return 0;       
       
       SD.sector_finish( current_sector, next_sector, current_size);
       current_size=0;
       
       current_sector = next_sector;
       current_pos = POS_DATA_START;
     }
     if (0xff==v) { v=escape;}
     else if (v==escape)
     {
       SD.i_write(current_sector,current_pos++,0xff);
       escape = candidate;
       v=escape;
       //fixme
       candidate = SD.randomescape(candidate);
     } else if (v==candidate)
     {
       candidate = SD.randomescape(candidate);
     }
     
     SD.i_write(current_sector,current_pos++,v);
     current_size++;
     return 1;
   }
   


    size_t File::write(uint8_t data)
      {
        if (mode != MODE_WRITE) {
#ifdef Debug
  putc('&',stderr);
#endif
          return 0;
        }
#ifdef Debug
  putc('+',stderr);
#endif

        return f_write(data);
      }
      

      
    int File::read()
      {
       // dbg p('r');
        if (mode == MODE_DIR_READ)
        {
        // a bit of a hack here.
#ifdef Debug
        fprintf(stderr,">(%i %i)",current_sector,current_pos);
#endif   
          if (current_pos>60)
          {
     
            do
            {
              #ifdef Debug
        fprintf(stderr,"@(%i)",current_sector);
#endif
current_sector++;
              if (current_sector > 32) {
                current_sector=32;
                return EOF;
              }
              
              if (SD.i_read(current_sector,POS_FIRST_SECTOR) == current_sector) break;
       
            } while (1);
            current_pos=0;            
          }

#ifdef Debug
        fprintf(stderr,"X");
#endif       
                    
          if (current_pos<20)
          {
            uint8_t c;
#ifdef Debug
        fprintf(stderr,"2(%i)",current_pos);
#endif
            c=SD.i_read(current_sector,POS_DATA_START+current_pos);
#ifdef Debug
        fprintf(stderr,"=%i ",c);
#endif
            if ((c==0) || (current_pos>=12))
            {
              current_pos+=20;                
            }
            else
             current_pos++;  
             return c;          
          }
          else if (current_pos<40)
          {
            #ifdef Debug
        fprintf(stderr,"4");
#endif
            current_pos++;
            if (current_pos==40) {
              int32_t s;
              s=SD.file_size(current_sector);
              my_name[0]=(s/100000)%10 + '0';
              my_name[1]=(s/10000)%10 + '0';
              my_name[2]=(s/1000)%10 + '0';
              my_name[3]=(s/100)%10 + '0';
              my_name[4]=(s/10)%10 + '0';
              my_name[5]=(s)%10 + '0';
              my_name[6]='b';
            }
            return ' ';  
          }
          else if (current_pos<46)
          {
            current_pos++;
            return my_name[current_pos-41];            
          }
          else
          {
            current_pos=61;
            return '\n';
          }
          return EOF;
       }
          
        
        
        if (mode != MODE_READ) return EOF;
#ifdef Debug
  putc('<',stderr);
#endif
        
        if (available_flag)
        {
         // dbg p('A');
          available_flag = false;
          return candidate;
        }
       // dbg p('f');
        return f_read();
      }
      
    int File::peek()
      {
        if (mode != MODE_READ) return -1;
        if (available_flag) { return candidate; }
        int v;
        v=f_read();
        if (v>0) {
          candidate = v;
          available_flag = true;
        }
        return v;
      }
      
      int File::available()
      { 
        return (peek()>=0); 
      }
      
      
      void File::flush()
      {
        return; // fixme not implemented yet...
            //, and actually... not needed at all with this implementation
        };
      //int File::read(void *buf, uint16_t nbyte);
      
      
      
      bool File::seek(uint32_t pos)
      {
        //fixme not implemented yet
        return false;
      }
      
      uint32_t File::position()
      {
        //fixme not implemented yet
        return -1;
      }
  
      int32_t SRXE_SDClass::file_size(uint8_t sector)
      {
        uint32_t size=0;        
        int8_t next_sector;
        int16_t tpos;
        //int8_t tescape;
              
        tpos = POS_FIRST_DATA_START;
        
        while (0<=(next_sector=SD.i_read(sector,POS_NEXT_SECTOR)))
        {
          size+= (SD.i_read(sector,POS_SECTOR_SIZE) << 8) +  SD.i_read(sector,POS_SECTOR_SIZE+1);          
          sector=next_sector;
          tpos = POS_DATA_START; 
        } 
        //tescape = i_read(sector,POS_ESCAPE);        
        
        while (1)
        {
          if (SD.i_read(sector,tpos)!=0xff)
           {
             size++;
             tpos++;
           }
           else
           {
             tpos++;
             if (SD.i_read(sector,tpos)==0xff) return size;
             tpos++;
             size++;
           }          
        }    
        
      }
      
      uint32_t File::size()
      {
        if (mode == MODE_CLOSED) return 0; // ...
        return SD.file_size(first_sector);
      
      }
      
      int File::availableForWrite()
      {
        if (mode==MODE_WRITE) return 1;
        return 0;  
        
      }
      void File::close()
      {
        flush();
        //name[0]=0;
        mode=MODE_CLOSED;  
        
      }
      
      /*operator File::bool()
      {
        return (mode != MODE_CLOSED);          
      }*/
      
      char * File::name()
      {
        return my_name;
      }

      bool File::isDirectory(void)
      {
        if (mode == MODE_DIR_READ) return true;
        return false;          
      };
      
      
      File File::openNextFile(uint8_t mode)
      {    
        // to do   
        /*if (mode == MODE_DIR_READ)
        {
        for(++current_sector;current_sector<MAX_SECTOR;current_sector++)
          {
          if (sector_valid(current_sector)) // yes this is a valid sector
            if (SD.i_read(current_sector,POS_FIRST_SECTOR) == current_sector) // yes, file starts here
              {
                File next(current_sector);
                return next;                
              }
          
          }
        }*/
        return File();
      }
      
      void File::rewindDirectory(void)
      {
        if (mode == MODE_DIR_READ) { current_sector = -1; }
      }
      

  bool SRXE_SDClass::valid_sector(uint8_t sector)
  {
    if (sector>31) return false;
    if ((validSectors>>sector) & 1 == 1) return true;
    return false;
    //return (i_read(sector,POS_MAGIC_BYTE)==sector);
  }

  bool SRXE_SDClass::empty_sector(uint8_t sector)
  {
    if (valid_sector(sector))
    {
      return (i_read(sector,POS_FIRST_SECTOR)==0xff);
    }
    return false;
  }
  


  bool SRXE_SDClass::begin(uint8_t csPin) {  
#ifdef LocalTest
    FILE *flashFile;
    flashFile = fopen("filesystem.dat","r");
    if (flashFile)
    {
      int32_t pos;
      int c;
      for(pos=0;pos<32*0x1000;pos++)
      {
        c=getc(flashFile);
        if (c!= EOF) { flashData[pos] = c; }
           else { 
             fprintf(stderr,"Waring!\n Filesystem file ended too soon!\n");
             break; }
      }
      fclose(flashFile);         
    }
    else
    {      
      fprintf(stderr,"Creating blank file system.\n");
      for(int pos=0;pos<32*0x1000;pos++) flashData[pos] = 0xff;      
    }              
#endif       
    
    int sector;
    uint32_t minwrites=0xffffffff;
    uint32_t writes;
  
    if (last_seize>=0) //fixme?
    {
      end();
    }  
  
    cs_pin=csPin;
    validSectors = 0;
    last_seize = 0;   
    
    int8_t mb;
    for (sector=0;sector<32;sector++)
    {
      mb=SD.i_read(sector,POS_MAGIC_BYTE);
      if ((mb == (sector | 0x80)) || (mb=0xff)) {
        validSectors|= (1 << sector);
#ifdef Debug
        fprintf(stderr,"$");
#endif        
        }
        else if (mb==0xb4)
        {
         sector+=2;
        }
// fixme: handling the basic files?        
/*        writes=SD.i_read(sector,POS_ERASE_COUNTER)<<16
          |SD.i_read(sector,POS_ERASE_COUNTER+1)<<8
          SD.i_read(sector,POS_ERASE_COUNTER+2)
        if (writes<minwrites) {
           if (SD.i_read(sector,POS_FIRST_SECTOR)!=0xff)
           minwrites = writes;
           last_seize = 
          
          29-4=25
          5   20  983
          8   17
          9   16
          
          
     7, 10, 11     
        }*/
      
    }      
      
    /*if (validSector(sector)){
        validSectors|= (1 << sector);   
      if (sectorEmpty(sector) && (sectorWrites(sector)<=minwrites))
        {
         minwrites = sectorWrites(sector);
         initFlag_nextSeize = sector; 
      }
     }  */
    
    return true;
   // initFlag_nextSeize |= 128;
}


  //call this when a card is removed. It will allow you to insert and initialise a new card.
  void SRXE_SDClass::end() {
#ifdef LocalTest
    FILE *flashFile;
    fprintf(stderr,"Saving\n");
    flashFile = fopen("filesystem.dat","w");
    if (flashFile)
    {
      int32_t pos;
      int c;
      for(pos=0;pos<32*0x1000;pos++)
      {
        putc(flashData[pos],flashFile);
      }
         
      fclose(flashFile);         
    }
    else
    {      
      fprintf(stderr,"Error saving the filesystem.\n");      
    }              
#endif       




    last_seize = -1;
//    initFlag_nextSeize = -1;
// pin(cs_pin, high)
  } 

  File SRXE_SDClass::open(const char *filepath, uint8_t mode) {
    /*

       Open the supplied file path for reading or writing.

       Defaults to read only.

       If `write` is true, default action (when `append` is true) is to
       append data to the end of the file.

       If `append` is false then the file will be truncated first.

       If the file does not exist and it is opened for writing the file
       will be created.

       An attempt to open a file for reading that does not exist is an
       error.

    */
    /*
    if ((mode & 30) == 0) { mode = MODE_READ; }
      else
      if (mode & O_APPEND) { mode= MODE_APPEND; }
      else { mode=MODE_WRITE;}
      */  
      


    
    if ( (filepath[1]==0) && ((filepath[0]=='.') || (filepath[0]=='/')))    
        { // root directory
 #ifdef Debug
        fprintf(stderr,"root\n");
 #endif
          if (mode & 30) return File();
          return File(-1,MODE_DIR_READ);            
        }

    int8_t first_sector = find_file(filepath);
 
 #ifdef Debug
        fprintf(stderr,"open  %i\n",first_sector);
        if (first_sector& 0x80) fprintf(stderr,"negative\n");
#endif        



 
    if ((mode & 30) == 0)
    { 
#ifdef Debug
        fprintf(stderr,"mode read\n");
#endif        
       if (first_sector & 0x80) return File();
       return File(first_sector, MODE_READ);  
    }
    
    
    if ((first_sector& 0x80) == 0) 
    {
#ifdef Debug
      fprintf(stderr,"file already found %i\n",first_sector);
#endif      
      if (mode & O_APPEND)
      {
#ifdef Debug
      fprintf(stderr,"appending \n");
#endif      
        return File(first_sector, MODE_WRITE);
      }
#ifdef Debug
      fprintf(stderr,"removing \n",first_sector);
#endif      

      if ((mode & O_TRUNC) == 0) return File();
      
      i_remove(first_sector);
      first_sector = -1;         
      mode |= O_CREAT; /* ?? */
      
    }

    if (mode & O_CREAT)  return File(filepath);
    return File();
  }


  /*
    File SDClass::open(byte  *filepath, uint8_t mode) {
    //

       Open the supplied file path for reading or writing.

       The file content can be accessed via the `file` property of
       the `SDClass` object--this property is currently
       a standard `SdFile` object from `sdfatlib`.

       Defaults to read only.

       If `write` is true, default action (when `append` is true) is to
       append data to the end of the file.

       If `append` is false then the file will be truncated first.

       If the file does not exist and it is opened for writing the file
       will be created.

       An attempt to open a file for reading that does not exist is an
       error.

     //

    // TODO: Allow for read&write? (Possibly not, as it requires seek.)

    fileOpenMode = mode;
    walkPath(filepath, root, callback_openPath, this);

    return File();

    }
  */


  //bool SDClass::close() {
  //  /*
  //
  //    Closes the file opened by the `open` method.
  //
  //   */
  //  file.close();
  //}


  bool SRXE_SDClass::exists(const char *filepath) {
    if (find_file(filepath)>0) return true;
    return false;
  }

  /* fixme not implemented yet
   * dummy wrappers already declared in header 
   bool SRXE_SDClass::mkdir(const byte  *filepath) {
    return false; // fixme not implemented yet
  }

  bool SRXE_SDClass::rmdir(const byte  *filepath) {
    return false; // fixme not implemented yet
  }
  * 
  * 
   // allows you to recurse into a directory
  File File::openNextFile(uint8_t mode) {
    // fixme not implemented yet
    return File();
  }

  void File::rewindDirectory(void) {
    return;// fixme  not implemented yet
  }
  * */

  bool SRXE_SDClass::remove(const char *filepath) {
    int8_t sector;
#ifdef Debug
    fprintf(stderr,"removing file%d\n");
#endif    
    sector=find_file(filepath);
    if (sector& 0x80) return false;
    return i_remove(sector);
  }

  bool SRXE_SDClass::i_remove(const uint8_t first_sector) {
    uint8_t  sector=first_sector;    
    if (first_sector& 0x80) {
    fprintf(stderr,"removing from sector -1??? %d\n",first_sector);
    return false;
  }
#ifdef Debug
    fprintf(stderr,"removing from sector %d\n",first_sector);
#endif    
    do
    {
      sector++;
      sector&= 31;
      // fixme add check for sector valid 
      if (i_read(sector,POS_FIRST_SECTOR) == first_sector)
        sector_erase(sector);      
    } while (sector!=first_sector);
    // note the first sector is erased last.
    // to prevent more serious corruption.
    // ideally the wake-up scan will finish off the delete action
    
    return true;
  }

/*bool check_name(const int8_t sector,const byte  *name)
  {
    int8_t n;
    for (n=0;n<12;n++)
    {
      if (SD.i_read(sector,POS_DATA_START+n) != name[n]) return false;
      if (name[n]==0) return true;
    }
    return true;
  }*/

  int8_t SRXE_SDClass::find_file(const char *name)
  {
    // fixme ;_;
    int8_t sector;    
    for(sector=0;sector<32;sector++)
    {
     if (SD.valid_sector(sector))
       if (SD.i_read(sector,POS_FIRST_SECTOR)==sector)
       {
         int8_t n=0;
         do {
           if (SD.i_read(sector,POS_DATA_START+n) != name[n]) {n=13;}
           else { if (name[n]==0) return sector; }
           n++;
          } while (n<12);
       }

    }
    
    return -1;
  }

 

  SRXE_SDClass SD;

} 
