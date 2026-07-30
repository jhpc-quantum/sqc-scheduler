#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

namespace sqc_auth {

///
/// Error instance for User and UserDB classes.
///
class UserError: public std::runtime_error {
  using std::runtime_error::runtime_error;
}; // class UserError

///
/// A user entry.
///
/// It currently has 'user ID' field only.
///
class User {
public:
  User() = delete;

  ///
  /// Create a user with ID.
  ///
  User(const std::string& id);

  ///
  /// Copy constructor.
  ///
  /// @param   other  the instance to copy from.
  ///
  User(const User& other) = default;

  ///
  /// Copy assignment operator.
  ///
  /// @param   other  the instance to copy from.
  /// @return  *this.
  ///
  User& operator=(const User& other) = default;

  ///
  /// Destructor.
  ///
  ~User() = default;

  ///
  /// Return user ID.
  ///
  /// @return  the user ID of the instance.
  ///
  inline const std::string& id() const noexcept { return id_; }

private:
  std::string id_;   ///< User ID.
}; // struct User

///
/// User database.
///
class UserDB {
public:
  ///
  /// Create an empty user database.
  ///
  UserDB();

  ///
  /// Copy constructor.
  ///
  /// @param   other  the instance to copy from.
  ///
  UserDB(const UserDB& other) = default;

  ///
  /// Copy assignment operator.
  ///
  /// @param   other  the instance to copy from.
  /// @return  *this.
  ///
  UserDB& operator=(const UserDB& other) = default;

  ///
  /// Destructor.
  ///
  ~UserDB() = default;

  ///
  /// Check if the specified user ID exists in the database.
  ///
  /// @param   id  User ID.
  /// @return  true if the user exists.
  ///
  bool has_user(const std::string& id) const;

  ///
  /// Create an instance by reading user data from the file.
  ///
  /// @param   file   Path to the file to read.
  /// @return  the created database.
  ///
  /// An exception is thrown if failed to read the file.
  ///
  static UserDB from_file(const std::string& file);

  ///
  /// Create an instance by reading user data from the file.
  ///
  /// @param   dir  Path to the directory where the user data file resides.
  /// @return  the created database.
  ///
  /// An exception is thrown if failed to read the file.
  ///
  static UserDB from_conf_dir(const std::string& dir);

  static constexpr char db_file[] = "users.db";   ///< database file.

private:
  ///
  /// Add a user with the specified ID.
  ///
  /// @param   id  User ID.
  ///
  /// An exception is thrown if the user already exists.
  ///
  void add_user(const std::string& id);

  std::unordered_map<std::string, User> db_;   ///< Mapping table from user ID to entry.
}; // UserDB

} // namespace sqc_auth
