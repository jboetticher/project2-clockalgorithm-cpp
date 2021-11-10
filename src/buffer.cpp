/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University
 * of Wisconsin-Madison.
 */

#include "buffer.h"

#include <iostream>
#include <memory>

#include "exceptions/bad_buffer_exception.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/hash_not_found_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"

namespace badgerdb {

constexpr int HASHTABLE_SZ(int bufs) { return ((int)(bufs * 1.2) & -2) + 1; }

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

BufMgr::BufMgr(std::uint32_t bufs)
    : numBufs(bufs),
      hashTable(HASHTABLE_SZ(bufs)),
      bufDescTable(bufs),
      bufPool(bufs) {
  for (FrameId i = 0; i < bufs; i++) {
    bufDescTable[i].frameNo = i;
    bufDescTable[i].valid = false;
  }

  clockHand = bufs - 1;
}

/**
 * Advance clock to next frame in the buffer pool.
 */
void BufMgr::advanceClock() {
  if(clockHand + 1 >= numBufs) {
    clockHand = 0;
  }
  else {
    clockhand++;
  }
}

/**
 * Allocates a free frame using the clock algorithm; if necessary, writing 
 * a dirty page back to disk. Throws BufferExceededException if all buffer 
 * frames are pinned. This private method will get called by the readPage()
 * and allocPage() methods described below. Make sure that if the buffer
 * frame allocated has a valid page in it, you remove the appropriate entry
 * from the hash table.
 */
void BufMgr::allocBuf(FrameId& frame) {
  // store current frame so that we can check if it repeats itself
  FrameId curFrame = clockHand;

  // clock algorithm until it gets to the clock hand
  advanceClock();
  bool allocated = false;
  while(!allocated && curFrame != clockHand) {
    // if the refbit is 1, advance the clock
    if(bufDescTable[clockHand].refbit) {
      advanceClock();
    }
    // else, frameid is set to that frame. set allocated to true.
    else {
      frame = bufDescTable[clockHand].frameNo;

      // TODO:  if frame is dirty, write to disk. 
      //        Otherwise, clear frame + new page is read into location

      allocated = true;
    }
  }

  if(!allocated) {
    throw new BufferExceededException();
  }
}

// jeremy
void BufMgr::readPage(File& file, const PageId pageNo, Page*& page) {

}

void BufMgr::unPinPage(File& file, const PageId pageNo, const bool dirty) {}

void BufMgr::allocPage(File& file, PageId& pageNo, Page*& page) {}

void BufMgr::flushFile(File& file) {}

void BufMgr::disposePage(File& file, const PageId PageNo) {}

void BufMgr::printSelf(void) {
  int validFrames = 0;

  for (FrameId i = 0; i < numBufs; i++) {
    std::cout << "FrameNo:" << i << " ";
    bufDescTable[i].Print();

    if (bufDescTable[i].valid) validFrames++;
  }

  std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
}

}  // namespace badgerdb
