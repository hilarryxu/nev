#pragma once

#include <stdint.h>

#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/callbacks.h"
#include "nev/tcp_connection.h"

using apache::thrift::TProcessor;
using apache::thrift::protocol::TProtocol;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::transport::TNullTransport;
using apache::thrift::transport::TTransport;
using apache::thrift::transport::TTransportException;

namespace nev {

class ThriftServer;

class NEV_EXPORT ThriftConnection
    : NonCopyable,
      public std::enable_shared_from_this<ThriftConnection> {
 public:
  ThriftConnection(ThriftServer* server, const TcpConnectionSharedPtr& conn);

 private:
  enum StateE { kExpectFrameSize, kExpectFrame };

  void onMessage(const TcpConnectionSharedPtr& conn,
                 Buffer* buf,
                 base::TimeTicks receive_time);

  void process();

  void close();

  ThriftServer* server_;
  TcpConnectionSharedPtr conn_;

  std::shared_ptr<TNullTransport> null_transport_;
  std::shared_ptr<TMemoryBuffer> input_transport_;
  std::shared_ptr<TMemoryBuffer> output_transport_;
  std::shared_ptr<TProtocol> input_protocol_;
  std::shared_ptr<TProtocol> output_protocol_;
  std::shared_ptr<TProcessor> processor_;

  StateE state_;
  uint32_t frame_size_;
};

using ThriftConnectionSharedPtr = std::shared_ptr<ThriftConnection>;

}  // namespace nev
