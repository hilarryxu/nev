#include <stdio.h>

#include <string>

#include "nev/nev_init.h"
#include "nev/ip_endpoint.h"
#include "nev/event_loop.h"
#include "nev/http/http_server.h"
#include "nev/http/http_request.h"
#include "nev/http/http_response.h"

using namespace nev;

void onHttpCallback(const HttpRequest&, HttpResponse* resp) {
  resp->setStatusCode(HttpResponse::k200Ok);
  resp->setStatusMessage("OK");
  resp->setBody("nev_httpd");
  resp->setCloseConnection(true);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: nev_httpd <port>\n");
    return 1;
  } else {
    EnsureNevInit();

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    IPEndPoint listen_addr(IPAddress::IPv4Localhost(), port);

    EventLoop loop;

    HttpServer server(&loop, listen_addr, "nev_httpd");
    server.setHttpCallback(onHttpCallback);
    server.start();

    loop.loop();

    return 0;
  }
}
