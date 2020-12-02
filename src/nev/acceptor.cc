#include "nev/acceptor.h"

#include "base/logging.h"

#include "nev/event_loop.h"
#include "nev/ip_endpoint.h"
#include "nev/sockets_ops.h"

namespace nev {

Acceptor::Acceptor(EventLoop* loop, const IPEndPoint& listen_addr)
    : loop_(loop),
      accept_socket_(sockets::CreateNonblockingOrDie()),
      accept_channel_(loop, accept_socket_.fd()),
      listenning_(false) {
  accept_socket_.setReuseAddr(true);
  accept_socket_.bindAddress(listen_addr);
  // accept_socket_ 可读时调用 handleRead
  accept_channel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {}

void Acceptor::listen() {
  loop_->assertInLoopThread();

  listenning_ = true;
  accept_socket_.listen();
  accept_channel_.enableReading();
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();

  IPEndPoint peer_addr;
  SocketDescriptor connfd = accept_socket_.accept(&peer_addr);
  if (connfd != kInvalidSocket) {
    if (new_connection_cb_) {
      new_connection_cb_(connfd, peer_addr);
    } else {
      sockets::Close(connfd);
    }
  } else {
    LOG(ERROR) << "in Acceptor::handleRead";
  }
}

}  // namespace nev