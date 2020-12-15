#include "nev/http/http_response.h"

#include <stdio.h>

#include "nev/buffer.h"

namespace nev {

void HttpResponse::appendToBuffer(Buffer* output) const {
  char buf[32];
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", status_code_);
  output->append(buf);
  output->append(status_message_);
  output->append("\r\n");

  if (close_connection_) {
    output->append("Connection: close\r\n");
  } else {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("Connection: Keep-Alive\r\n");
  }

  for (const auto& header : headers_) {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }

  output->append("\r\n");
  output->append(body_);
}

}  // namespace nev
