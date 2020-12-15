#pragma once

#include <string>
#include <map>

#include "nev/nev_export.h"

namespace nev {

class Buffer;

class NEV_EXPORT HttpResponse {
 public:
  enum HttpStatusCode {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  explicit HttpResponse(bool flag_close)
      : status_code_(kUnknown), close_connection_(flag_close) {}

  void setStatusCode(HttpStatusCode code) { status_code_ = code; }

  void setStatusMessage(const std::string& message) {
    status_message_ = message;
  }

  void setCloseConnection(bool on) { close_connection_ = on; }

  bool closeConnection() const { return close_connection_; }

  void setContentType(const std::string& content_type) {
    addHeader("Content-Type", content_type);
  }

  void addHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
  }

  void setBody(const std::string& body) { body_ = body; }

  void appendToBuffer(Buffer* output) const;

 private:
  std::map<std::string, std::string> headers_;
  HttpStatusCode status_code_;
  std::string status_message_;
  bool close_connection_;
  std::string body_;
};

}  // namespace nev
