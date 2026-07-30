#include <filesystem>
#include <fstream>
#include "jwt_verifier.h"

namespace sqc_auth {

//
// class Jwt.
//
Jwt::Jwt(const std::string& token)
  : decoded_token_(jwt::decode(token)),
    sub_() {
  try {
    sub_ = decoded_token_.get_subject();
  } catch (const std::runtime_error& e) {
  }
}

Jwt Jwt::from_file(const std::string& file) {
  std::ifstream stream(file);
  if (!stream) {
    throw JwtError(std::string("failed to open the file ").append(file));
  }
  std::string token((std::istreambuf_iterator<char>(stream)),
                    std::istreambuf_iterator<char>());
  stream.close();

  return Jwt(token);
}

//
// class JwtVerifier.
//
JwtVerifier::JwtVerifier()
  : public_key_(),
    iss_() {
}

JwtVerifier::JwtVerifier(const std::string& public_key, const std::string& iss)
  : public_key_(public_key),
    iss_(iss) {
}

void JwtVerifier::verify(const Jwt& token) const {
  auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::es256(public_key_));
  verifier.with_issuer(iss_);
  verifier.verify(token.decoded_token());
}

JwtVerifier JwtVerifier::from_conf_dir(const std::string& conf_dir) {
  auto fs_pubkey_path = std::filesystem::path(conf_dir) / std::filesystem::path(public_key_file);
  std::ifstream pubkey_stream(fs_pubkey_path);
  if (!pubkey_stream) {
    throw JwtError(std::string("failed to open the file ").append(fs_pubkey_path));
  }
  std::string pubkey((std::istreambuf_iterator<char>(pubkey_stream)),
                     std::istreambuf_iterator<char>());
  pubkey_stream.close();

  auto fs_iss_path = std::filesystem::path(conf_dir) / std::filesystem::path(iss_file);
  std::ifstream iss_stream(fs_iss_path);
  if (!iss_stream) {
    throw JwtError(std::string("failed to open the file ").append(fs_iss_path));
  }
  std::string iss((std::istreambuf_iterator<char>(iss_stream)),
                  std::istreambuf_iterator<char>());
  iss_stream.close();
  std::size_t iss_end_pos = iss.find_last_of(" \t\r\n");
  if (iss_end_pos != std::string::npos) {
    iss = iss.substr(0, iss_end_pos);
  }

  return JwtVerifier(pubkey, iss);
}

} // namespace sqc_auth
