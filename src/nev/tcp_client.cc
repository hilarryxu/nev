#include "nev/tcp_client.h"

#include "build/build_config.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"

#include "nev/ip_endpoint.h"
#include "nev/tcp_connection.h"
#include "nev/connector.h"
#include "nev/event_loop.h"

namespace nev {

namespace {

void removeConnectionInLoop(EventLoop* loop,
                            const TcpConnectionSharedPtr& conn) {
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

}  // namespace

TcpClient::TcpClient(EventLoop* loop, const IPEndPoint& server_addr)
    : loop_(loop),
      connector_(new Connector(loop, server_addr)),
      connect_(true),
      next_conn_id_(1) {
  LOG(DEBUG) << "TcpClient::ctor at " << this
             << " connector = " << connector_.get();
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, _1));
}

TcpClient::~TcpClient() {
  LOG(DEBUG) << "TcpClient::dtor at " << this
             << " connector = " << connector_.get();
  // 强引用一下 connection_，避免 TcpClient 销毁了就访问不了了。
  TcpConnectionSharedPtr conn;
  {
    base::AutoLock lock(lock_);
    conn = connection_;
  }

  if (conn) {
    // 之前 setCloseCallback 设置的回调需要访问 TcpClient
    // 这里重新设置替换掉之前的，新的不需要再访问 TcpClient 了
    // FIXME(xcc): not 100% safe, if we are in different thread
    CloseCallback cb = std::bind(&removeConnectionInLoop, loop_, _1);
    loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
  } else {
    // 没有建立过连接或者连接已销毁，停止连接器
    // FIXME(xcc): 不调用 connector_->stop() 让 connector_ 自己析构行不行？
    connector_->stop();
  }
}

void TcpClient::connect() {
  LOG(DEBUG) << "TcpClient::connect[" << this << "] - connecting to "
             << connector_->server_addr().toString();
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect() {
  connect_ = false;

  {
    base::AutoLock lock(lock_);
    if (connection_) {
      // 主动关闭连接
      connection_->shutdown();
    }
  }
}

void TcpClient::stop() {
  connect_ = false;
  // 停止连接器
  connector_->stop();
}

void TcpClient::newConnection(SocketDescriptor sockfd) {
  loop_->assertInLoopThread();

  std::string conn_name = base::StringPrintf(":#%d", next_conn_id_);
  ++next_conn_id_;

  // 初始化 TcpConnection 并关联 loop
  // FIXME(xcc): 从 sockfd 得到下面两个 IPEndPoint
  IPEndPoint peer_addr;
  IPEndPoint local_addr;
  TcpConnectionSharedPtr conn(
      new TcpConnection(loop_, conn_name, sockfd, local_addr, peer_addr));
  conn->setConnectionCallback(connection_cb_);
  conn->setMessageCallback(message_cb_);
  conn->setWriteCompleteCallback(write_complete_cb_);
  conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));

  // TcpClient 里也存一份
  {
    base::AutoLock lock(lock_);
    connection_ = conn;
  }

  // 当前就在连接对应的 loop 中所以可以直接调用
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionSharedPtr& conn) {
  loop_->assertInLoopThread();
  DCHECK(loop_ == conn->loop());

  // 移除 TcpClient 持有的 shared_ptr
  {
    base::AutoLock lock(lock_);
    DCHECK(connection_ == conn);
    connection_.reset();
  }

  // 投递到连接对应的 loop 中销毁
  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

}  // namespace nev
