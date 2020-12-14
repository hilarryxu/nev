#include <stdio.h>

#include "nev/socket_descriptor.h"
#include "base/logging.h"
#include "base/threading/platform_thread.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "nev/ip_endpoint.h"
#include "nev/tcp_server.h"

using namespace nev;

void onConnection(const TcpConnectionSharedPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): tid=%d new connection [%s] from %s\n",
           base::PlatformThread::CurrentId(), conn->name().c_str(),
           conn->peer_addr().toString().c_str());
  } else {
    printf("onConnection(): tid=%d connection [%s] is down\n",
           base::PlatformThread::CurrentId(), conn->name().c_str());
  }
}

void onMessage(const TcpConnectionSharedPtr& conn,
               Buffer* buf,
               base::TimeTicks receive_time) {
  LOG(DEBUG) << "onMessage(): tid=" << base::PlatformThread::CurrentId()
             << " received " << buf->readableBytes()
             << " bytes from connection [" << conn->name() << "] at "
             << receive_time;

  conn->send(buf->retrieveAllAsString());
}

int main(int argc, char* argv[]) {
  EnsureNevInit();

  IPEndPoint listen_addr(IPAddress::IPv4Localhost(), 9981);
  EventLoop loop;

  TcpServer server(&loop, listen_addr, "test7");
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  if (argc > 1) {
    server.setThreadNum(atoi(argv[1]));
  }
  server.start();

  loop.loop();
}
