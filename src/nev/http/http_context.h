#pragma once

#include "nev/nev_export.h"
#include "nev/http/http_request.h"
#include "nev/http/http11_parser.h"

namespace nev {

class Buffer;

class NEV_EXPORT HttpContext {
 public:
  HttpContext();

  bool parseRequest(Buffer* buf);

  bool gotAll() { return http_parser_is_finished(&parser_) == 1; }

  void reset() {
    HttpRequest dummy;
    request_.swap(dummy);
    nparsed_ = 0;
    http_parser_init(&parser_);
  }

  const HttpRequest& request() const { return request_; }

  HttpRequest& request() { return request_; }

 private:
  HttpRequest request_;
  struct http_parser parser_;
  size_t nparsed_;
};

}  // namespace nev
