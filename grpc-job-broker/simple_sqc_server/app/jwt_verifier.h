#pragma once

#include <filesystem>
#include <string>
#include <jwt-cpp/jwt.h>

namespace sqc_auth {

using JwtTrait = jwt::traits::kazuho_picojson;

///
/// Error instance for Jwt and JwtVerifier classes.
///
class JwtError: public std::runtime_error {
  using std::runtime_error::runtime_error;
}; // class JwtError

///
/// A JSON web token.
///
class Jwt {
public:
  Jwt() = delete;

  ///
  /// Create an instance with the specified token (base64 encoded string).
  ///
  /// @param   token  the instance to copy from.
  ///
  Jwt(const std::string& token);

  ///
  /// Copy constructor.
  ///
  /// @param   other  the instance to copy from.
  ///
  Jwt(const Jwt& other) = default;

  ///
  /// Copy assignment operator.
  ///
  /// @param   other  the instance to copy from.
  /// @return  *this.
  ///
  Jwt& operator=(const Jwt& other) = default;

  ///
  /// Destructor.
  ///
  ~Jwt() = default;

  ///
  /// Return a decoded token.
  ///
  /// @return  decoded token.
  ///
  inline const jwt::decoded_jwt<JwtTrait>& decoded_token() const noexcept { return decoded_token_; }

  ///
  /// Return a header of the token.
  ///
  /// @return  a header
  ///
  inline const std::string& header() const noexcept { return decoded_token_.get_header(); }

  ///
  /// Return payload of the token.
  ///
  /// @return  payload
  ///
  inline const std::string& payload() const noexcept { return decoded_token_.get_payload(); }

  ///
  /// Return a sigunature of the token.
  ///
  /// @return  a signature
  ///
  inline const std::string& signature() const noexcept { return decoded_token_.get_signature(); }

  ///
  /// Return a subject of the token.
  ///
  /// @return  a subject or an empty string.
  ///
  /// It returns an empty string if the current has no subject.
  ///
  inline const std::string& sub() const noexcept { return sub_; }

  ///
  /// Create an instance by reading a token from the specified file.
  ///
  /// @param   file  path to a file of the token file.
  /// @return  the created JWT.
  ///
  static Jwt from_file(const std::string& file);

private:
  jwt::decoded_jwt<JwtTrait> decoded_token_;   ///< Decoded token.
  std::string sub_;   ///< Subject.
}; // class Jwt

///
/// Jwt verifier.
///
class JwtVerifier {
public:
  ///
  /// Create a verifier with an empty public key and an empty issuer.
  ///
  JwtVerifier();

  ///
  /// Create a verifier with the specified public key and the expected issuer.
  ///
  /// @param   public_key  ES256 public key in PEM format.
  /// @param   iss  Expected issuer.
  ///
  JwtVerifier(const std::string& public_key, const std::string& iss);

  ///
  /// Copy constructor.
  ///
  /// @param   other  the instance to copy from.
  ///
  JwtVerifier(const JwtVerifier& other) = default;

  ///
  /// Copy assignment operator.
  ///
  /// @param   other  the instance to copy from.
  /// @return  *this.
  ///
  JwtVerifier& operator=(const JwtVerifier& other) = default;

  ///
  /// Destructor.
  ///
  ~JwtVerifier() = default;

  ///
  /// Return the public key of the verifier.
  ///
  /// @return  the public key.
  ///
  inline const std::string& public_key() const noexcept { return public_key_; }

  ///
  /// Return the issuer of the verifier.
  ///
  /// @return  the issuer.
  ///
  inline const std::string& iss() const noexcept { return iss_; }

  ///
  /// Verify a token.
  ///
  /// @param   token  a JWT to verify.
  ///
  /// If failed, an exception is thrown.
  ///
  void verify(const Jwt& token) const;

  ///
  /// Create a verifier by reading public key and issuer files.
  ///
  /// @param   conf_dir  path to a directory of public key and issuer files.
  /// @return  the created verifier.
  ///
  /// It reads the public key file `jwt_pub.key` and the issuer file `jwt_iss.txt` in `conf_dir`.
  /// If failed, an exception is thrown.
  ///
  /// Note that it assumes the public key is written in PEM format and its algorithm is ES256.
  ///
  static JwtVerifier from_conf_dir(const std::string& conf_dir);

  static constexpr const char public_key_file[] = "jwt_pub.key";   ///< public key file.
  static constexpr const char iss_file[] = "jwt_iss.txt";   ///< issuer file.

private:
  std::string public_key_;   ///< public key.
  std::string iss_;   ///< Expected issuer.
}; // class JwtVerifier

} // namespace sqc_auth
