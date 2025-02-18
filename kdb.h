/**
 * Irfan G. (c) 2025.
 */

#pragma once

#include <bit>
#include <string>
#include <fstream>
#include <cassert>

#pragma pack(1)

namespace kdb {
  using std::cout;
  using std::string;
  using std::ofstream;
  using std::ifstream;
  using std::bit_cast;
  using std::to_string;

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
      char unused[BLOCK_SIZE - 629];
    } kdb_h;

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
    } kdb_bblk;


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
        auto block = bit_cast<db::_db_block *>( data );
        odbf.write( block->blkid, BLOCK_SIZE );
        return true;
      }
      catch ( ... ) {
        return false;
      }
    }

    bool read( void *data, unsigned long long &offset, uint32_t &bytes_r ) {
      try {
        idbf.seekg( offset );
        auto block = bit_cast<_db_block *>( data );
        idbf.read( block->blkid, BLOCK_SIZE );
        bytes_r = idbf.gcount();
        return true;
      }
      catch ( ... ) {
        return false;
      }
    }

  public:
    db() = delete;
    db(db &&) = delete;
    db(const db &) = delete;
    db &operator=(db &&) = delete;
    db &operator=(const db &) = delete;

    db(string path = "file.db") : db_path(path), active(false), odbf(), idbf() {
      active = init();
    }

    bool is_open() const { return active; }

  };

  class kdbms {
  private:
    db db_inst;
    bool online;
    uint32_t port;
    string pass;

  public:
      kdbms() = delete;
      kdbms(kdbms &&) = delete;
      kdbms(const kdbms &) = delete;
      kdbms &operator=(kdbms &&) = delete;
      kdbms &operator=(const kdbms &) = delete;

      kdbms(string path) :
      db_inst(path), online(false), port(0), pass("") {}

      bool Online(uint32_t port = 8000, string pass = "") {
        char buff[db::BLOCK_SIZE] = {0};
        auto _h = bit_cast<db::_db_header*>(&buff[0]);
        this->port = port;
        this->pass = pass;

        if (!online) {

          if (db_inst.is_open()) {
            auto offset = 0ULL;
            auto bytes_r = 0U;
            if (db_inst.read(buff, offset, bytes_r)) {

              if ( bytes_r == 0 ) {
                // do init

                // load header meta

                online = true;
              }
              else if ( bytes_r == db::BLOCK_SIZE ) {
                // load header meta

                // validate header

                online = true;
              }
            }
          }
        }

        return online;
      }

      static void StartService(kdbms &db) {

      }

#ifndef NDEBUG
    static bool Test() {

        kdbms t("file.db");
        assert( t.db_inst.is_open() == true );

        // _db_header / _db_data / _db_bdata
        assert( kdb::db::BLOCK_SIZE ==  sizeof(db::kdb_h) &&
           kdb::db::BLOCK_SIZE == sizeof(db::kdb_blk) &&  kdb::db::BLOCK_SIZE == sizeof(db::kdb_bblk));
        assert(t.Online() == true);

        return true;
      }
#endif

  };
}
