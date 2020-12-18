#include "contrib/thrift/thrift_connection.h"

#include <thrift/transport/TTransportException.h>

#include "base/logging.h"

#include "nev/event_loop.h"
#include "nev/event_loop_thread_pool.h"

#include "contrib/thrift/thrift_server.h"

namespace nev {

ThriftConnection::ThriftConnection(ThriftServer* server,
                                   const TcpConnectionSharedPtr& conn)
    : server_(server), conn_(conn), state_(kExpectFrameSize), frame_size_(0) {
  conn_->setMessageCallback(
      std::bind(&ThriftConnection::onMessage, this, _1, _2, _3));

  null_transport_.reset(new TNullTransport());
  input_transport_.reset(new TMemoryBuffer(nullptr, 0));
  output_transport_.reset(new TMemoryBuffer());

  factory_input_transport_ =
      server_->getInputTransportFactory()->getTransport(input_transport_);
  factory_output_transport_ =
      server_->getOutputTransportFactory()->getTransport(output_transport_);

  input_protocol_ =
      server_->getInputProtocolFactory()->getProtocol(factory_input_transport_);
  output_protocol_ = server_->getOutputProtocolFactory()->getProtocol(
      factory_output_transport_);

  processor_ =
      server_->getProcessor(input_protocol_, output_protocol_, null_transport_);
}

void ThriftConnection::onMessage(const TcpConnectionSharedPtr& conn,
                                 Buffer* buf,
                                 base::TimeTicks receive_time) {
  bool more = true;

  while (more) {
    if (state_ == kExpectFrameSize) {
      if (buf->readableBytes() >= 4) {
        frame_size_ = static_cast<uint32_t>(buf->readInt32());
        state_ = kExpectFrame;
      } else {
        more = false;
      }
    } else if (state_ == kExpectFrame) {
      if (buf->readableBytes() >= frame_size_) {
        uint8_t* input_buffer =
            reinterpret_cast<uint8_t*>((const_cast<char*>(buf->peek())));

        // copy input_buffer to |input_transport_|
        input_transport_->resetBuffer(input_buffer, frame_size_,
                                      TMemoryBuffer::COPY);
        // preserve 4 bytes for |output_transport_|
        output_transport_->getWritePtr(4);
        output_transport_->wroteBytes(4);

        if (server_->hasWorkerThreadPool()) {
          server_->workerThreadPool()->getNextLoop()->queueInLoop(
              std::bind(&ThriftConnection::process, this));
        } else {
          process();
        }

        buf->retrieve(frame_size_);
        state_ = kExpectFrameSize;
      } else {
        more = false;
      }
    }
  }
}

void ThriftConnection::process() {
  try {
    processor_->process(input_protocol_, output_protocol_, nullptr);

    uint8_t* buf;
    uint32_t size;
    // size = 4 + frame_size
    output_transport_->getBuffer(&buf, &size);

    // write frame_size to buf front
    DCHECK(size >= 4);
    uint32_t frame_size = static_cast<uint32_t>(htonl(size - 4));
    memcpy(buf, &frame_size, 4);

    // send response to client
    conn_->send(buf, size);
  } catch (const TTransportException& ex) {
    LOG(ERROR) << "ThriftServer TTransportException: " << ex.what();
    close();
  } catch (const std::exception& ex) {
    LOG(ERROR) << "ThriftServer std::exception: " << ex.what();
    close();
  } catch (...) {
    LOG(ERROR) << "ThriftServer unknown exception";
    close();
  }
}

void ThriftConnection::close() {
  null_transport_->close();
  factory_input_transport_->close();
  factory_output_transport_->close();
}

}  // namespace nev
