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
#include <thread>
#include <atomic>
#include <fcntl.h>
#include <iostream>

// net
#include <unistd.h>
#ifndef WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#pragma pack(1)

namespace kdb {
  using std::cout;
  using std::atomic;
  using std::string;
  using std::thread;
  using std::ofstream;
  using std::ifstream;
  using std::bit_cast;
  using std::to_string;
  using std::chrono::system_clock;

  // crypto provider - SAMPLE ONLY
  class Crypto {
  private:
    const string pass;

  public:
    Crypto(string pass) : pass(pass) {}
    virtual ~Crypto() {}
    virtual void Enc(void *data, const size_t sz) {
      auto p = bit_cast<char*>(data);
      size_t enc_bCnt = 0;
      while(enc_bCnt < sz) {
        p[enc_bCnt] ^= 21;
        enc_bCnt++;
      }
    }
    virtual void Dec(void *data, const size_t sz) {
      auto p = bit_cast<char*>(data);
      size_t enc_bCnt = 0;
      while(enc_bCnt < sz) {
        p[enc_bCnt] ^= 21;
        enc_bCnt++;
      }
    }
    virtual void GenRandBits(void *data, const size_t sz) {
      auto p = bit_cast<char*>(data);
      size_t bytesGen = 0;

      while (bytesGen < sz) {
        p[bytesGen] ^= (rand() % 0x100);
        bytesGen++;
      }
    }

#ifndef NDEBUG
    static void Test() {
      string data = "hello world!";
      size_t sz = data.length();
      Crypto crypt("password");

      crypt.Enc(data.data(), sz);
      assert(data.compare("hello world!") != 0);
      sz = data.length();
      crypt.Dec(data.data(), sz);
      assert(data.compare("hello world!") == 0);

    }
#endif
  };

  class Net {
    protected:
    enum class Mode {
      CONNECT = 0,
      LISTEN,
    };
    const int sockfd;
    const int port;
    const Mode mode;
    struct sockaddr_in sockaddr;
    bool active;

  public:
    Net(int sockfd, Mode mode = Mode::CONNECT) : sockfd(sockfd), port(0), mode(mode), sockaddr({0}), active(true) {}
    Net(string h, int port, Mode mode = Mode::LISTEN) :
      sockfd(socket(AF_INET, SOCK_STREAM, 0)), port(port), mode(mode), sockaddr({0}), active(false) {
      switch (mode) {
        case Mode::LISTEN:
          if (sockfd > 0) {
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_port = htons(port);
            sockaddr.sin_addr.s_addr = inet_addr(h.c_str());
            int len = sizeof(sockaddr);
#ifndef WIN32
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&sockaddr, len);
#endif
            if ( 0 == bind(sockfd, reinterpret_cast<struct sockaddr *>(&sockaddr), len) )
              if ( 0 == listen(sockfd, 100) ) {
                active = true;
              }
          }
          break;
        case Mode::CONNECT:
          if (sockfd > 0) {
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_port = htons(port);
            sockaddr.sin_addr.s_addr = inet_addr(h.c_str());
            int len = sizeof(sockaddr);
            if ( 0 == connect(sockfd, reinterpret_cast<struct sockaddr *>(&sockaddr), len) )
              active = true;
          }
          break;
      }
    }

    virtual ~Net() {
      close(sockfd);
      active = false;
    }

    virtual void Send(void *data, unsigned &sz) {
      auto w_sz = ::send(sockfd, bit_cast<char*>(data), sz, 0);
      if (w_sz > 0)
        sz = w_sz;
      else
        sz = 0;
    }

    virtual void Recv(void *data, unsigned &sz, unsigned *c) {
      if ( mode == Mode::LISTEN)
      {
        if ( nullptr != c && (*c) == 0 ) {
          (*c) = accept(sockfd, nullptr, nullptr);
#ifndef WIN32
          fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
#else
          u_long mode = 1;  // 1 to enable non-blocking socket
          ioctlsocket(sockfd, FIONBIO, &mode);
#endif
        }
        else if ( nullptr != c && (*c) > 0 ){
          auto r_sz = ::recv(*c, bit_cast<char*>(data), sz, 0);
          if (r_sz > 0)
            sz = r_sz;
          else
            sz = 0;
        }
      }
      else {
        auto r_sz = recv(sockfd, bit_cast<char*>(data), sz, 0);
        if (r_sz > 0)
          sz = r_sz;
        else
          sz = 0;
      }
    }

#ifndef NDEBUG
    static void Test() {
      using namespace std::chrono_literals;
      atomic<bool> server_run = true;

      thread server([&server_run]
      {
        char data[30] = "hello world!";
        unsigned data_sz = strlen(data);
        unsigned data_sent = 0;
        Net host("0.0.0.0", 9100, Mode::LISTEN);

        unsigned client_sock = 0;
        host.Recv(data, data_sz, &client_sock);
        while (client_sock == 0)
          host.Recv(data, data_sz, &client_sock);
        Net clnt(client_sock);

        while (server_run) {
          while ( data_sent != strlen(data)) {
            clnt.Send(&data[data_sent], data_sz);
            if (data_sz >0) {
              data_sent += data_sz;
              data_sz = strlen(data) - data_sent;
            }
            else
              data_sz = strlen(data) - data_sent;
          }
          break;
        }

        while (server_run)
          std::this_thread::sleep_for(std::chrono::milliseconds(100));

      });

      thread client( [&server_run] {
        Net client("127.0.0.1", 9100, Mode::CONNECT);
        char rdata[30] = {0};
        unsigned bytes_r = 0;
        unsigned r_sz = sizeof(rdata);
        assert(client.active == true);

        while (server_run && bytes_r < strlen("hello world!")) {
          client.Recv(&rdata[bytes_r], r_sz, 0);
          if (r_sz > 0) {
            bytes_r += r_sz;
            r_sz = strlen(rdata) - bytes_r;
          }
        }
        server_run = false;
        assert(bytes_r == strlen("hello world!"));
        assert(strncmp(rdata, "hello world!", strlen("hello world!")) == 0);
      });

      server.join();
      client.join();
    };
#endif

  };


  struct kdb_tx {
    size_t tx_id;
    size_t tx_sz;
    char spec[128];
    void *data;
  };

  class storage;
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

  class Storage {
  private:
    db &dbinst;
  public:
    enum class STMODE {
      READ = 0,
      WRITE = 1,
    };
    Storage()   = delete;
    Storage(Storage &&) = delete;
    Storage(Storage const &) = delete;
    Storage &operator=(Storage &&) = delete;
    Storage &operator=(Storage const &) = delete;

    Storage(db &dbinst) : dbinst(dbinst) {}

    virtual ~Storage() {}
    virtual void HandleReq(int c_sock){}
  };

  class kdbms {
  private:
    static atomic<bool>  _active;
    db            db_inst;
    bool          online;
    uint32_t      port;
    char          kek[512];
    Crypto        *_cipher;
    Net           *_net;
    Storage       *_storage;

  public:
      kdbms()                         = delete;
      kdbms(kdbms &&)                 = delete;
      kdbms(const kdbms &)            = delete;
      kdbms &operator=(kdbms &&)      = delete;
      kdbms &operator=(const kdbms &) = delete;

      virtual ~kdbms() { if (nullptr != _storage) delete _storage; _storage = nullptr;  }

      kdbms(string path) :
      db_inst(path), online(false), port(0), kek{0}, _cipher(nullptr), _net(nullptr), _storage(nullptr) {}

      template<class S>
      void setProviders(Crypto *_cipher, Net *_net) {
        this->_cipher  = _cipher;
        this->_net     = _net;
        this->_storage = new S(db_inst);
      }

      bool Online() {
        char buff[db::BLOCK_SIZE] = {0};
        auto _h = bit_cast<db::_db_header*>(&buff[0]);

        if (!online && nullptr != _cipher) {

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

                _cipher->GenRandBits(_h->kek, sizeof(_h->kek));
                _cipher->Enc(_h->kek, sizeof(_h->kek));

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

                _cipher->Dec(_h->kek, sizeof(_h->kek));
                // todo: kek validation with security provider

                online = true;
              }
            }
          }
        }

        return online;
      }

      template<class D>
      static void StartService(D &kdb, uint32_t port = 8000) {
        unsigned client = 0;
        char buff[200] = {0};
        unsigned sz = sizeof(buff);

        while (kdb._active) {
          client = 0;
          kdb._net->Recv(buff, sz, client);
          if (client > 0)
            kdb._storage->HandleReq(client);
        }
      }

#ifndef NDEBUG
    static void Test() {

        kdbms   t("file.db");
        Crypto  c("password");
        Net     n("0.0.0.0", 8000);
        t.setProviders<Storage>(&c, &n);
        assert( t.db_inst.is_open() == true );

        // _db_header / _db_data / _db_bdata
        assert( kdb::db::BLOCK_SIZE ==  sizeof(db::kdb_h) &&
           kdb::db::BLOCK_SIZE == sizeof(db::kdb_blk) &&  kdb::db::BLOCK_SIZE == sizeof(db::kdb_bblk));
        assert(t.Online() == true);

      }
#endif

  };
}
