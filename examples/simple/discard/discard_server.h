#include "nev/socket_descriptor.h"

#include "nev/tcp_server.h"

class DiscardServer {
 public:
  DiscardServer(nev::EventLoop* loop, const nev::IPEndPoint& listen_addr);

  void start();

 private:
  void onConnection(const nev::TcpConnectionSharedPtr& conn);

  void onMessage(const nev::TcpConnectionSharedPtr& conn,
                 nev::Buffer* buf,
                 base::TimeTicks receive_time);

  nev::TcpServer server_;
};
