#include <cstddef>
#include <filesystem>
#include <fstream>
#include "user_db.h"

namespace sqc_auth {

//
// class User.
//
User::User(const std::string& id)
  : id_(id) {}

//
// class UserDB.
//
UserDB::UserDB()
  : db_() {}

bool UserDB::has_user(const std::string& id) const {
  auto iter = db_.find(id);
  return (iter != db_.end());
}

void UserDB::add_user(const std::string& id) {
  if (id.size() == 0) {
    throw UserError(std::string("try adding a user with an empty ID"));
  }
  if (db_.find(id) != db_.end()) {
    throw UserError(std::string("try adding an existing user ").append(id));
  }
  auto user = User(id);
  db_.insert({id, user});
}

UserDB UserDB::from_file(const std::string& file) {
  UserDB db;

  try {
    std::ifstream stream(file);
    if (!stream) {
      throw UserError(std::string("failed to open the user db file ")
                      .append(file));
    }

    std::string line;
    while (std::getline(stream, line)) {
      std::size_t id_end_pos = line.find_first_of(":\r\n");
      if (id_end_pos == std::string::npos) {
        id_end_pos = line.size();
      }

      auto id = line.substr(0, id_end_pos);
      db.add_user(id);
    }
  } catch (const std::exception& e) {
    throw;
  }

  return db;
}

UserDB UserDB::from_conf_dir(const std::string& dir) {
  auto file = std::filesystem::path(dir) / std::filesystem::path(db_file);
  return from_file(file.string());
}

} // namespace sqc_auth
