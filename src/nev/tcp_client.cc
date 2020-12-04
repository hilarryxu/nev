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
  // ǿ����һ�� connection_������ TcpClient �����˾ͷ��ʲ����ˡ�
  TcpConnectionSharedPtr conn;
  {
    base::AutoLock lock(lock_);
    conn = connection_;
  }

  if (conn) {
    // ֮ǰ setCloseCallback ���õĻص���Ҫ���� TcpClient
    // �������������滻��֮ǰ�ģ��µĲ���Ҫ�ٷ��� TcpClient ��
    // FIXME(xcc): not 100% safe, if we are in different thread
    CloseCallback cb = std::bind(&removeConnectionInLoop, loop_, _1);
    loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
  } else {
    // û�н��������ӻ������������٣�ֹͣ������
    // FIXME(xcc): ������ connector_->stop() �� connector_ �Լ������в��У�
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
      // �����ر�����
      connection_->shutdown();
    }
  }
}

void TcpClient::stop() {
  connect_ = false;
  // ֹͣ������
  connector_->stop();
}

void TcpClient::newConnection(SocketDescriptor sockfd) {
  loop_->assertInLoopThread();

  std::string conn_name = base::StringPrintf(":#%d", next_conn_id_);
  ++next_conn_id_;

  // ��ʼ�� TcpConnection ������ loop
  // FIXME(xcc): �� sockfd �õ��������� IPEndPoint
  IPEndPoint peer_addr;
  IPEndPoint local_addr;
  TcpConnectionSharedPtr conn(
      new TcpConnection(loop_, conn_name, sockfd, local_addr, peer_addr));
  conn->setConnectionCallback(connection_cb_);
  conn->setMessageCallback(message_cb_);
  conn->setWriteCompleteCallback(write_complete_cb_);
  conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));

  // TcpClient ��Ҳ��һ��
  {
    base::AutoLock lock(lock_);
    connection_ = conn;
  }

  // ��ǰ�������Ӷ�Ӧ�� loop �����Կ���ֱ�ӵ���
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionSharedPtr& conn) {
  loop_->assertInLoopThread();
  DCHECK(loop_ == conn->loop());

  // �Ƴ� TcpClient ���е� shared_ptr
  {
    base::AutoLock lock(lock_);
    DCHECK(connection_ == conn);
    connection_.reset();
  }

  // Ͷ�ݵ����Ӷ�Ӧ�� loop ������
  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

}  // namespace nev
