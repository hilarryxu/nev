#pragma once

#include <stdio.h>

#include <string>
#include <map>

#include "base/time/time.h"

#include "nev/nev_export.h"

namespace nev {

class NEV_EXPORT HttpRequest {
 public:
  enum Method { kInvalid, kGet, kPost, kHead, kPut, kDelete };
  enum Version { kUnknown, kHttp10, kHttp11 };

  HttpRequest() : method_(kInvalid), version_(kUnknown) {}

  void setVersion(Version v) { version_ = v; }

  Version version() const { return version_; }

  bool setMethod(const char* start, const char* end) {
    std::string m(start, end);
    if (m == "GET") {
      method_ = kGet;
    } else if (m == "POST") {
      method_ = kPost;
    } else if (m == "HEAD") {
      method_ = kHead;
    } else if (m == "PUT") {
      method_ = kPut;
    } else if (m == "DELETE") {
      method_ = kDelete;
    } else {
      method_ = kInvalid;
    }
    return method_ != kInvalid;
  }

  Method method() const { return method_; }

  const char* methodString() const {
    const char* result = "UNKNOWN";
    switch (method_) {
      case kGet:
        result = "GET";
        break;
      case kPost:
        result = "POST";
        break;
      case kHead:
        result = "HEAD";
        break;
      case kPut:
        result = "PUT";
        break;
      case kDelete:
        result = "DELETE";
        break;
      default:
        break;
    }
    return result;
  }

  void setPath(const char* start, const char* end) { path_.assign(start, end); }

  const std::string& path() const { return path_; }

  void setQuery(const char* start, const char* end) {
    query_.assign(start, end);
  }

  const std::string& query() const { return query_; }

  void setReceiveTime(base::TimeTicks t) { receive_time_ = t; }

  base::TimeTicks receiveTime() const { return receive_time_; }

  void addHeader(const char* start, const char* colon, const char* end) {
    std::string field(start, colon);
    ++colon;
    while (colon < end && isspace(*colon)) {
      ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && isspace(value[value.size() - 1])) {
      value.resize(value.size() - 1);
    }
    headers_[field] = value;
  }

  void addHeader(const char* field,
                 size_t flen,
                 const char* value,
                 size_t vlen) {
    std::string s_field(field, field + flen);
    std::string s_value(value, value + vlen);
    headers_[s_field] = s_value;
  }

  std::string getHeader(const std::string& field) const {
    std::string result;
    std::map<std::string, std::string>::const_iterator it =
        headers_.find(field);
    if (it != headers_.end()) {
      result = it->second;
    }
    return result;
  }

  const std::map<std::string, std::string>& headers() const { return headers_; }

  void swap(HttpRequest& that) {
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    // receive_time_.swap(that.receive_time_);
    headers_.swap(that.headers_);
  }

 private:
  Method method_;
  Version version_;
  std::string path_;
  std::string query_;
  base::TimeTicks receive_time_;
  std::map<std::string, std::string> headers_;
};

}  // namespace nev
