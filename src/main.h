/**
 * @mainpage BadgerDB Documentation
 *
 * @section toc_sec Table of contents
 *
 * <ol>
 *   <li> @ref file_layout_sec
 *   <li> @ref building_sec
 *   <ol>
 *     <li> @ref prereq_sec
 *     <li> @ref commands_sec
 *     <li> @ref modify_run_main_sec
 *     <li> @ref documentation_sec
 *   </ol>
 *   <li> @ref api_sec
 *   <ol>
 *     <li> @ref storage_sec
 *     <ol>
 *       <li> @ref file_management_sec
 *       <li> @ref file_data_sec
 *       <li> @ref page_sec
 *     </ol>
 *   </ol>
 * </ol>
 *
 * @section file_layout_sec File layout
 *
 * The files in this package are organized under the following hierarchy:
 * <pre>
 * docs/                  generated documentation
 * src/                   code for BadgerDB
 * </pre>
 *
 * You will likely be most interested in <code>src</code>
 *
 * @section building_sec Building and modifying the system
 *
 * @subsection prereq_sec Prerequisites
 *
 * To build and run the system, you need the following packages:
 * <ul>
 *   <li>A modern C++ compiler (GCC >= 4.6, any recent version of clang)
 *   <li>Doxygen 1.6 or higher (for generating documentation only)
 * </ul>
 *
 * The build system is configured to work on CSL RedHat 5 and 6 machines out of
 * the box.
 *
 * @subsection commands_sec Executing a build
 *
 * All command examples are meant to be run at the command prompt from the
 * <code>badgerdb</code> directory.  When executing a command, omit the
 * <code>$</code> prompt (so &ldquo;<code>$ make</code>&rdquo; means you just
 * type &ldquo;<code>make</code>&rdquo; and press enter).
 *
 * To build the executable:
 * @code
 *   $ make
 * @endcode
 *
 * @subsection modify_run_main_sec Modifying and running main
 *
 * To run the executable, first build the code, then run:
 * @code
 *   $ ./src/badgerdb_main
 * @endcode
 *
 * If you want to edit what <code>badgerdb_main</code> does, edit
 * <code>src/main.cpp</code>.
 *
 * @subsection documentation_sec Rebuilding the documentation
 *
 * Documentation is generated by using Doxygen.  If you have updated the
 * documentation and need to regenerate the output files, run:
 * @code
 *  $ make doc
 * @endcode
 * Resulting documentation will be placed in the <code>docs/</code>
 * directory; open <code>index.html</code> with your web browser to view it.
 *
 * @section api_sec BadgerDB API
 *
 * @subsection storage_sec File storage
 *
 * Interaction with the underlying filesystem is handled by two classes: File
 * and Page.  Files store zero or more fixed-length pages; each page holds zero
 * or more variable-length records.
 *
 * Record data is represented using std::strings of arbitrary characters.
 *
 * @subsubsection file_management_sec Creating, opening, and deleting files
 *
 * Files must first be created before they can be used:
 * @code
 *  // Create and open a new file with the name "filename.db".
 *  badgerdb::File new_file = badgerdb::File::create("filename.db");
 * @endcode
 *
 * If you want to open an existing file, use File::open like so:
 * @code
 *  // Open an existing file with the name "filename.db".
 *  badgerdb::File existing_file = badgerdb::File::open("filename.db");
 * @endcode
 *
 * Multiple File objects share the same stream to the underlying file.  The
 * stream will be automatically closed when the last File object is out of
 * scope; no explicit close command is necessary.
 *
 * You can delete a file with File::remove:
 * @code
 *  // Delete a file with the name "filename.db".
 *  badgerdb::File::remove("filename.db");
 * @endcode
 *
 * @subsubsection file_data_sec Reading and writing data in a file
 *
 * Data is added to a File by first allocating a Page, populating it with data,
 * and then writing the Page back to the File.
 *
 * For example:
 * @code
 *   #include "file.h"
 *
 *   ...
 *
 *   // Write a record with the value "hello, world!" to the file.
 *   badgerdb::File db_file = badgerdb::File::open("filename.db");
 *   badgerdb::Page new_page = db_file.allocatePage();
 *   new_page.insertRecord("hello, world!");
 *   db_file.writePage(new_page);
 * @endcode
 *
 * Pages are read back from a File using their page numbers:
 * @code
 *   #include "file.h"
 *   #include "page.h"
 *
 *   ...
 *
 *   // Allocate a page and then read it back.
 *   badgerdb::Page new_page = db_file.allocatePage();
 *   db_file.writePage(new_page);
 *   const badgerdb::PageId& page_number = new_page.page_number();
 *   badgerdb::Page same_page = db_file.readPage(page_number);
 * @endcode
 *
 * You can also iterate through all pages in the File:
 * @code
 *   #include "file_iterator.h"
 *
 *   ...
 *
 *   for (badgerdb::FileIterator iter = db_file.begin();
 *        iter != db_file.end();
 *        ++iter) {
 *     std::cout << "Read page: " << iter->page_number() << std::endl;
 *   }
 * @endcode
 *
 * @subsubsection page_sec Reading and writing data in a page
 *
 * Pages hold variable-length records containing arbitrary data.
 *
 * To insert data on a page:
 * @code
 *   #include "page.h"
 *
 *   ...
 *
 *   badgerdb::Page new_page;
 *   new_page.insertRecord("hello, world!");
 * @endcode
 *
 * Data is read by using RecordIds, which are provided when data is inserted:
 * @code
 *   #include "page.h"
 *
 *   ...
 *
 *   badgerdb::Page new_page;
 *   const badgerdb::RecordId& rid = new_page.insertRecord("hello, world!");
 *   new_page.getRecord(rid); // returns "hello, world!"
 * @endcode
 *
 * As Pages use std::string to represent data, it's very natural to insert
 * strings; however, any data can be stored:
 * @code
 *   #include "page.h"
 *
 *   ...
 *
 *   struct Point {
 *     int x;
 *     int y;
 *   };
 *   Point new_point = {10, -5};
 *   badgerdb::Page new_page;
 *   std::string new_data(reinterpret_cast<char*>(&new_point),
 *                        sizeof(new_point));
 *   const badgerdb::RecordId& rid = new_page.insertRecord(new_data);
 *   Point read_point =
 *       *reinterpret_cast<const Point*>(new_page.getRecord(rid).data());
 * @endcode
 * Note that serializing structures like this is not industrial strength; it's
 * better to use something like Google's protocol buffers or Boost
 * serialization.
 *
 * You can also iterate through all records in the Page:
 * @code
 *   #include "page_iterator.h"
 *
 *   ...
 *
 *   for (badgerdb::PageIterator iter = new_page.begin();
 *        iter != new_page.end();
 *        ++iter) {
 *     std::cout << "Record data: " << *iter << std::endl;
 *   }
 * @endcode
 *
 */
