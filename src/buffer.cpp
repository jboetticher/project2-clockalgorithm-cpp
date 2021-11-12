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

namespace badgerdb
{

  constexpr int HASHTABLE_SZ(int bufs) { return ((int)(bufs * 1.2) & -2) + 1; }

  //----------------------------------------
  // Constructor of the class BufMgr
  //----------------------------------------

  BufMgr::BufMgr(std::uint32_t bufs)
      : numBufs(bufs),
        hashTable(HASHTABLE_SZ(bufs)),
        bufDescTable(bufs),
        bufPool(bufs)
  {
    for (FrameId i = 0; i < bufs; i++)
    {
      bufDescTable[i].frameNo = i;
      bufDescTable[i].valid = false;
    }

    clockHand = bufs - 1;
  }

  /**
 * Advance clock to next frame in the buffer pool.
 */
  void BufMgr::advanceClock()
  {
    if (clockHand + 1 >= numBufs)
    {
      clockHand = 0;
    }
    else
    {
      clockHand++;
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
  void BufMgr::allocBuf(FrameId &frame)
  {
    std::cout << "        BufMgr: Starting allocBuf! \n";
    // store current frame so that we can check if it repeats itself
    FrameId curFrame = clockHand;

    // clock algorithm until it gets to the clock hand
    advanceClock();
    std::cout << "        BufMgr: Advanced the clock to " << clockHand << ".\n";

    bool allocated = false;
    while (!allocated && curFrame != clockHand)
    {
      // if the refbit is 1, reset it & advance the clock
      if (bufDescTable[clockHand].refbit)
      {
        bufDescTable[clockHand].refbit = false;
        advanceClock();
        std::cout << "        BufMgr: Refbit was 1, so it was reset and the clock was advanced to " << clockHand << ".\n";
      }
      // if it's being used, skip it.
      else if (bufDescTable[clockHand].pinCnt > 0)
      {
        advanceClock();
        std::cout << "        BufMgr: PinCnt > 0, clock advanced to " << clockHand << ".\n";
      }
      // else, frameid is set to that frame. set allocated to true.
      else
      {
        frame = bufDescTable[clockHand].frameNo;
        std::cout << "        BufMgr: FrameId set to " << frame << ".\n";

        // if frame is dirty, write to disk.
        if (bufDescTable[clockHand].dirty)
        {
          std::cout << "        BufMgr: Frame was dirty, starting to flush.\n";

          // NOTE:  algorithm says to flush, not sure if calling this is right.
          //        also not sure if I used pointers correctly.
          flushFile(bufDescTable[clockHand].file);
        }

        // TODO:    if the buffer frame allocated has a valid page in it, 
        //          remove the appropriate entry from the hash table.
        //hashTable.remove(bufDescTable[clockHand].file, bufDescTable[clockHand].pageNo);

        // clear + set frame + end
        // NOTE:    not sure if clearing is necessary
        std::cout << "        BufMgr: Starting to clear frame.\n";
        bufDescTable[clockHand].clear();
        bufDescTable[clockHand].Print();
        allocated = true;
      }
    }

    if (!allocated)
    {
      throw new BufferExceededException();
    }
  }

  // jeremy
  void BufMgr::readPage(File &file, const PageId pageNo, Page *&page)
  {
    // check if the page is already in the buffer pool via lookup method
    FrameId f;

    try
    {
      hashTable.lookup(file, pageNo, f);
      // page is in the buffer pool:

      // set the refbit to true
      bufDescTable[f].refbit = true;

      // increment pinCnt
      bufDescTable[f].pinCnt += 1;

      // return a pointer to the frame containing the page via page parameter
      *page = bufPool[f];
    }
    // page is not in the buffer pool:
    catch (HashNotFoundException e)
    {
      // call allocBuf
      allocBuf(f);

      // call the method file.readPage()
      Page pg = file.readPage(pageNo);

      // insert page into hashtable
      hashTable.insert(file, pageNo, f);

      // set() the frame to set it up properly
      bufDescTable[f].Set(file, pageNo);

      // return pointer to frame containing the page via page parameter
      *page = pg;
    }
  }

  void BufMgr::unPinPage(File &file, const PageId pageNo, const bool dirty)
  {
    FrameId fid;
    try
    {
      hashTable.lookup(file, pageNo, fid);
    }
    catch (HashNotFoundException e)
    {
      std::cerr << e.message();
      return;
    }
    if (bufDescTable[fid].pinCnt != 0)
    {
      bufDescTable[fid].pinCnt -= 1;
      if (dirty)
      {
        bufDescTable[fid].dirty = true;
      }
    }
    else
    {
      throw PageNotPinnedException(file.filename(), pageNo, fid);
    }
  }

  void BufMgr::allocPage(File &file, PageId &pageNo, Page* &page)
  {
    std::cout << "        BufMgr: Starting allocPage! \n";
    Page p = file.allocatePage();
    std::cout << "        BufMgr: file.allocatePage() \n";
    page = &(p);    // TODO : may not need & here
                                    // added a star here, removed an error? 

    std::cout << "        BufMgr: set Page* &page to newly allocated page \n";
    pageNo = (*page).page_number(); // TODO : check pointer syntax
    std::cout << "        BufMgr: pageNo set to " << pageNo << "\n";
    FrameId fid;
    allocBuf(fid);
    std::cout << "        BufMgr: allocBuf(fid) \n";
    hashTable.insert(file, pageNo, fid);
    bufDescTable[clockHand].Print();
    std::cout << "        BufMgr: inserted to hash table \n";
    bufDescTable[fid].Set(file, pageNo);
    bufDescTable[clockHand].Print();
    std::cout << "        BufMgr: set within bufDescTable \n";
    
    // TODO : add to bufPool? When did the page ever get added to bufPool?
  }

  void BufMgr::flushFile(File &file)
  {
    // TODO : need to check if pages are valid ?
    // I did this function backwards, by iterating through every
    // page in the file, then checking if that page was in the buffer
    // pool. Not sure what implications this has (i.e. running time)
    FileIterator fi = file.begin();
    FileIterator end = file.end();
    while (fi != end)
    {
      FrameId fid;
      PageId pageNo = (*fi).page_number();
      try
      {
        hashTable.lookup(file, pageNo, fid);
      }
      catch (HashNotFoundException e)
      {
        fi++;
        continue;
      }
      if (bufDescTable[fid].dirty)
      {
        file.writePage(pageNo, bufPool[fid]); // perform checks mentioned in method header ?
        bufDescTable[fid].dirty = false;
      }
      hashTable.remove(file, pageNo);
      bufDescTable[fid].clear();
    }
  }

  void BufMgr::disposePage(File &file, const PageId PageNo)
  {
    // TODO : need to check if pinned ?
    FrameId fid;
    bool frameAllocated = true;
    try
    {
      hashTable.lookup(file, PageNo, fid);
    }
    catch (HashNotFoundException e)
    {
      frameAllocated = false;
    }
    if (frameAllocated)
    {
      bufDescTable[fid].clear();
      hashTable.remove(file, PageNo);
    }
    file.deletePage(PageNo);
  }

  void BufMgr::printSelf(void)
  {
    int validFrames = 0;

    for (FrameId i = 0; i < numBufs; i++)
    {
      std::cout << "FrameNo:" << i << " ";
      bufDescTable[i].Print();

      if (bufDescTable[i].valid)
        validFrames++;
    }

    std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
  }

} // namespace badgerdb
