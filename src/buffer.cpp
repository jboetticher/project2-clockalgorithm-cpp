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

#include "bufHashTbl.h"
#include "file_iterator.h"

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

void BufMgr::advanceClock() {}

void BufMgr::allocBuf(FrameId& frame) {}

void BufMgr::readPage(File& file, const PageId pageNo, Page*& page) {}

void BufMgr::unPinPage(File& file, const PageId pageNo, const bool dirty) {
  FrameId fid;
  try {
    hashTable.lookup(file, pageNo, fid);
  } catch (HashNotFoundException e) {
    std::cerr << e.message();
    return;
  }
  if (bufDescTable[fid].pinCnt != 0) {
    bufDescTable[fid].pinCnt -= 1;
    if (dirty) {
      bufDescTable[fid].dirty = true;
    }
  } else {
    throw PageNotPinnedException(file.filename(), pageNo, fid);
  }
}

void BufMgr::allocPage(File& file, PageId& pageNo, Page*& page) {
  page = &file.allocatePage(); // TODO : may not need & here
  pageNo = (*page).page_number(); // TODO : check pointer syntax
  FrameId fid;
  allocBuf(fid);
  hashTable.insert(file, pageNo, fid);
  bufDescTable[fid].Set(file, pageNo);
  // TODO : do anything with bufPool ?
}

void BufMgr::flushFile(File& file) {
  // TODO : need to check if pages are valid ?
  // I did this function backwards, by iterating through every
  // page in the file, then checking if that page was in the buffer
  // pool. Not sure what implications this has (i.e. running time)
  FileIterator fi = file.begin();
  FileIterator end = file.end();
  while (fi != end) {
    FrameId fid;
    PageId pageNo = (*fi).page_number();
    try {
      hashTable.lookup(file, pageNo, fid);
    } catch (HashNotFoundException e) {
      fi++;
      continue;
    }
    if (bufDescTable[fid].dirty) {
      file.writePage(pageNo, bufPool[fid]); // perform checks mentioned in method header ?
      bufDescTable[fid].dirty = false;
    }
    hashTable.remove(file, pageNo);
    bufDescTable[fid].clear();

  }
}

void BufMgr::disposePage(File& file, const PageId PageNo) {
  // TODO : need to check if pinned ?
  FrameId fid;
  bool frameAllocated = true;
  try {
    hashTable.lookup(file, PageNo, fid);
  } catch (HashNotFoundException e) {
    frameAllocated = false;
  }
  if (frameAllocated) {
    bufDescTable[fid].clear();
    hashTable.remove(file, PageNo);
  }
  file.deletePage(PageNo);
}

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
