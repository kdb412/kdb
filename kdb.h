/**
 * Irfan G. (c) 2025.
 */

#pragma once

#include <string>
#include <fstream>

namespace kdb {
  using std::string;
  using std::ofstream;
  using std::ifstream;

  class db {
  private:
    ofstream ofs;
    static constexpr int MAX_BLOCK_ALLOC = 0x64;

    struct _db_header {
      char dbid[1];
      char magic[4];
      char version[4];
      char oname[100];
      char kek[512];
      int64_t idate;
    };

    struct _db_block {
      char blkid[1];
      char dbid[2];
      char flid[30];
      char flname[100];
      char unused[4096 - 133];
    } kdb_blk;


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

  public:
    db(const db &) = delete;

    db &operator=(const db &) = delete;

    db(db &&) = delete;

    db &operator=(db &&) = delete;

    db(string path = "file.db") : db_path(path), active(false), odbf(), idbf() {
      active = init();
    }

    bool is_open() { return active; }

    // todo: read() / write() / delete() / find()
  };
}
