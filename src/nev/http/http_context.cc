#include "nev/socket_descriptor.h"
#include "nev/http/http_context.h"

#include <stdio.h>
#include <string.h>  // memset

#include "base/logging.h"

#include "nev/buffer.h"

namespace nev {

namespace {

void parseHttpVersion(void* data, const char* at, size_t len) {
  HttpContext* context = static_cast<HttpContext*>(data);

  std::string version(at, at + len);
  if (version == "HTTP/1.0") {
    context->request().setVersion(HttpRequest::kHttp10);
  } else if (version == "HTTP/1.1") {
    context->request().setVersion(HttpRequest::kHttp11);
  }
}

void parseRequestMethod(void* data, const char* at, size_t len) {
  HttpContext* context = static_cast<HttpContext*>(data);

  context->request().setMethod(at, at + len);
}

void parseRequestPath(void* data, const char* at, size_t len) {
  HttpContext* context = static_cast<HttpContext*>(data);

  context->request().setPath(at, at + len);
}

void parseQueryString(void* data, const char* at, size_t len) {
  HttpContext* context = static_cast<HttpContext*>(data);

  context->request().setQuery(at, at + len);
}

void parseHttpField(void* data,
                    const char* field,
                    size_t flen,
                    const char* value,
                    size_t vlen) {
  HttpContext* context = static_cast<HttpContext*>(data);

  if (flen > 0) {
    context->request().addHeader(field, flen, value, vlen);
  }
}

}  // namespace

HttpContext::HttpContext() : nparsed_(0) {
  memset(&parser_, 0, sizeof(parser_));
  http_parser_init(&parser_);

  parser_.data = this;

  parser_.http_version = parseHttpVersion;
  parser_.request_method = parseRequestMethod;
  parser_.request_path = parseRequestPath;
  parser_.query_string = parseQueryString;
  parser_.http_field = parseHttpField;
}

// return false if any error
bool HttpContext::parseRequest(Buffer* buf) {
  bool ok = true;

  size_t delta = http_parser_execute(&parser_, buf->peek(),
                                     buf->readableBytes(), nparsed_);
  nparsed_ += delta;
  buf->retrieve(delta);

  if (http_parser_has_error(&parser_))
    return false;

  return ok;
}

}  // namespace nev
