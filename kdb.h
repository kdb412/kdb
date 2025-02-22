/**
 * Irfan G. (c) 2025.
 */

#pragma once

#include <bit>
#include <chrono>
#include <string>
#include <fstream>
#include <cassert>
#include <cstdint>
#include <cstring>

#pragma pack(1)

namespace kdb {
  using std::cout;
  using std::string;
  using std::ofstream;
  using std::ifstream;
  using std::bit_cast;
  using std::to_string;
  using std::chrono::system_clock;

  // crypto provider
  class Crypto {
    public:
    virtual ~Crypto() {}
    virtual void Enc(void *data, size_t sz) {}
    virtual void Dec(void *data, const size_t sz) {}
  };

  class Net {
    public:
    virtual ~Net() {}
    virtual bool Connect(){ return false; }
    virtual void Send(void *data, size_t &sz){}
    virtual void Recv(void *data, size_t &sz){}
  };

  struct kdb_tx {
    size_t tx_id;
    size_t tx_sz;
    char spec[128];
    void *data;
  };

  class Storage {
    public:
    enum class STMODE {
      READ = 0,
      WRITE = 1,
    };

    virtual ~Storage() {}
    virtual void GenerateTx(void *data, size_t &sz, kdb_tx &tx, STMODE s_mode = STMODE::READ){}
    virtual void ExecuteTx(kdb_tx &tx){}
  };

  class db {

    friend class kdbms;
    friend class storage;
  
  private:
    static constexpr int MAX_BLOCK_ALLOC = 0x64;
    static constexpr int BLOCK_SIZE = 0x1000;

    /* header info */
    static constexpr uint8_t  DBID = 0x11;
    static constexpr uint16_t DBVE = 0x0001;

    struct _db_header {
      char dbid[1];
      char version[2];
      char oname[100];
      char kek[512];
      uint64_t idate;
      char unused[BLOCK_SIZE -623];
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
        auto block = bit_cast<char *>( data );
        odbf.write( block, BLOCK_SIZE );
        return true;
      }
      catch ( ... ) {
        return false;
      }
    }

    bool read( void *data, unsigned long long &offset, uint32_t &bytes_r ) {
      try {
        idbf.seekg( offset );
        auto block = bit_cast<char *>( data );
        idbf.read( block, BLOCK_SIZE );
        bytes_r = idbf.gcount();
        return true;
      }
      catch ( ... ) {
        return false;
      }
    }

  public:
    db()                      = delete;
    db(db &&)                 = delete;
    db(const db &)            = delete;
    db &operator=(db &&)      = delete;
    db &operator=(const db &) = delete;

    db(string path = "file.db") : db_path(path), active(false), odbf(), idbf() {
      active = init();
    }

    bool is_open() const { return active; }

  };

  class kdbms {
  private:
    db        db_inst;
    bool      online;
    uint32_t  port;
    string    pass;
    char      kek[512];

  public:
      kdbms()                         = delete;
      kdbms(kdbms &&)                 = delete;
      kdbms(const kdbms &)            = delete;
      kdbms &operator=(kdbms &&)      = delete;
      kdbms &operator=(const kdbms &) = delete;

      kdbms(string path) :
      db_inst(path), online(false), port(0), pass(""), kek{0} {}

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
                _h->dbid[0]    = db::DBID;
                _h->version[0] = db::DBVE >> 8;
                _h->version[1] = db::DBVE ^ 0x007f;
                _h->idate = std::chrono::duration_cast<std::chrono::seconds>(system_clock::now().time_since_epoch()).count();

                if (db_inst.db_path.length() > sizeof(db::kdb_h.oname))
                  strncpy(_h->oname, &db_inst.db_path.c_str()[db_inst.db_path.length()-sizeof(db::kdb_h.oname)], sizeof(db::kdb_h.oname));
                else
                  strcpy(_h->oname, db_inst.db_path.c_str());

                // todo: generate kek / with security provider

                memcpy(_h->kek, kek, sizeof(db::kdb_h.kek));
                online = db_inst.write(buff, offset);
              }
              else if ( bytes_r == db::BLOCK_SIZE ) {
                // load header meta
                if ( _h->dbid[0] != db::DBID )
                  return false;
                if ( _h->version[0] != (db::DBVE >> 8 ) )
                  return false;
                if ( _h->version[1] != (db::DBVE ^ 0x007f) )
                  return false;

                // todo: kek validation with security provider

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
    static void Test() {

        kdbms t("file.db");
        assert( t.db_inst.is_open() == true );

        // _db_header / _db_data / _db_bdata
        assert( kdb::db::BLOCK_SIZE ==  sizeof(db::kdb_h) &&
           kdb::db::BLOCK_SIZE == sizeof(db::kdb_blk) &&  kdb::db::BLOCK_SIZE == sizeof(db::kdb_bblk));
        assert(t.Online() == true);

      }
#endif

  };
}
