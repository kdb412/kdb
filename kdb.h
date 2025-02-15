/**
 * Irfan G. (c) 2025.
 */

#pragma once

#include <bit>
#include <string>
#include <fstream>

namespace kdb {
  using std::string;
  using std::ofstream;
  using std::ifstream;
  using std::bit_cast;

  class db {

  friend class kdbms;
  
  private:
    ofstream ofs;
    static constexpr int MAX_BLOCK_ALLOC = 0x64;
    static constexpr int BLOCK_SIZE = 0x1000;

    struct _db_header {
      char dbid[1];
      char magic[4];
      char version[4];
      char oname[100];
      char kek[512];
      int64_t idate;
    };

    // blkid : A/C/D
    struct _db_block {
      char blkid[1];
      char dbid[2];
      char flid[30];
      char flname[100];
      char flsize[8];
      char unused[BLOCK_SIZE -141];
    } kdb_blk;

    struct _db_bblock {
      char blkid[1];
      char blkno[8];
      char flid[30];
      char spec[11];
      char data[BLOCK_SIZE -50];
    } kdb_bblock;


    const string db_path;
    bool active;
    ofstream odbf;
    ifstream idbf;

    bool init() {
      odbf.open(db_path, std::ios::binary | std::ios::app);
      idbf.open(db_path, std::ios::binary | std::ios::in);

      if (odbf.is_open() && idbf.is_open())
        return true;
      else {
        odbf.close();
        idbf.close();
      }

      return false;
    }

    bool write( void *data, unsigned long long &offset ) {

      try {
        odbf.seekp( offset );
        auto block = bit_cast<_db_block *>( data );
        odbf.write( block->blkid, BLOCK_SIZE );
        return true;
      }
      catch ( ... ) {
        return false;
      }
    }

    bool read( void *data, unsigned long long &offset ) {
      try {
        idbf.seekg( offset );
        auto block = bit_cast<_db_block *>( data );
        idbf.read( block->blkid, BLOCK_SIZE );
        return true;
      }
      catch ( ... ) {
        return false;
      }
    }

  public:
    db(db &&) = delete;
    db(const db &) = delete;
    db &operator=(db &&) = delete;
    db &operator=(const db &) = delete;
    db() = delete;

    db(string path = "file.db") : db_path(path), active(false), odbf(), idbf() {
      active = init();
    }

    bool is_open() const { return active; }

  };

  class kdbms {
  private:
    const string kdb_path;
    db db_inst;

    public:
      kdbms() = delete;
      kdbms(kdbms &&) = delete;
      kdbms(const kdbms &) = delete;
      kdbms &operator=(kdbms &&) = delete;
      kdbms &operator=(const kdbms &) = delete;

      kdbms(string path) : kdb_path(path), db_inst(kdb_path) {}
  };
}
