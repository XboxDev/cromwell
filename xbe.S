#define BASE_ADDRESS 0x10000
#define FILE_SIZE (1100*1024)
header_start:
  .ascii "XBEH"

// digital signature
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

  .long BASE_ADDRESS                    // base address
  .long headers_end - header_start      // size of headers
  .long FILE_SIZE+32*1024               // size of image
  .long header_end - header_start       // size of image header
  .long 0                               // time date stamp                 *unimportant*
  .long certificate                     // certificate address
  .long 1                               // number of section headers
  .long section_headers                 // section headers address 
  .long 0                               // initialization flags
  .long 0x11100 ^ 0xA8FC57AB            // entry point address XOR key **GAS ISSUE: "entry" won't work**
  .long 0x18000                         // thread local storage address
  .long 0                               // size of stack commit (PE copy)  *unimportant*
  .long 0                               // size of heap reserve (PE copy)  *unimportant*
  .long 0                               // size of heap commit (PE copy)   *unimportant*
  .long 0                               // original base address (PE copy) *unimportant*
  .long 0                               // original size of image (PE copy)*unimportant*
  .long 0                               // original checksum (PE copy)     *unimportant*
  .long 0                               // original time stamp (PE copy)   *unimportant*
  .long 0                               // debug path name address         *unimportant*
  .long 0                               // debug file name address         *unimportant*
  .long 0                               // debug unicode file name address *unimportant*
  .long 0x11000 ^ 0x5B6D40B6            // kernel image thunk address XOR key **GAS ISSUE: see above**
  .long 0                               // non-kernel import directory address
  .long 0                               // number of library versions      *unimportant*
  .long 0                               // library versions address        *unimportant*
  .long 0                               // kernel library version address  *unimportant*
  .long 0                               // XAPI library address            *unimportant*
  .long 0                               // logo bitmap address             *unimportant*
  .long 0                               // logo bitmap size                *unimportant*
header_end:

certificate:
  .long certificate_end - certificate   // size of certificate
  .long 0                               // time date stamp                 *unimportant*
  .long 0                               // title id                        *unimportant?*

  // title name (unicode string, 40 chars) *unimportant, but beautiful*
  .word 'L','i','n','u','x',0,0,0,0,0,0
  .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  .word 0,0,0,0,0,0,0,0,0,0,0,0,0

      

  // alternate title ids
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

  .long 0xC0000005
  .long 0x80000007   // game regions: all; don't delete eepromkey
  .long -1  // game ratings 
  .long 0   // disk number
  .long 0   // version
  
  // LAN key
  .long 0,0,0,0

  // signature key
  .long 0,0,0,0

  // 16 title alternate signature keys
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  .long 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
certificate_end:

section_headers:
//  .long 0x16            // flags: RO_HEAD_PAGE, EXECUTABLE, PRELOAD
  .long 0x07            // flags: WRITABLE, EXECUTABLE, PRELOAD
  .long section_1_start // virtual address
  .long FILE_SIZE       // virtual size
  .long 0x1000          // file pointer to raw data
  .long FILE_SIZE       // size of raw data
  .long section_name    // address of section name
  .long 0               // unknown
  .long rc1             // head shared page reference count address *not NULL important*
  .long rc2             // tail shared page reference count address *not NULL important*

  // unknown
  .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0,0,0

section_headers_end:

rc1:
  .word 0
rc2:
  .word 0
section_name:
  .byte '.','t','e','x','t',0

.align 4
headers_end:

