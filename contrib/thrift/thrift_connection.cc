#include "contrib/thrift/thrift_connection.h"

#include <thrift/transport/TTransportException.h>

#include "base/logging.h"

#include "nev/event_loop.h"

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

  input_protocol_ =
      server_->inputProtocolFactory_->getProtocol(input_transport_);
  output_protocol_ =
      server_->inputProtocolFactory_->getProtocol(output_transport_);

  processor_ =
      server_->getProcessor(input_protocol_, output_protocol_, null_transport_);
}

void ThriftConnection::onMessage(const TcpConnectionSharedPtr& conn,
                                 Buffer* buf,
                                 base::TimeTicks receive_time) {
  bool more = true;

  while (more) {
    if (state_ == kExpectFrameSize) {
      if (buf->readableBytes() >= sizeof(int32_t)) {
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
        output_transport_->resetBuffer();
        output_transport_->getWritePtr(sizeof(int32_t));
        output_transport_->wroteBytes(sizeof(int32_t));

        process();

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
    DCHECK(size >= sizeof(int32_t));
    uint32_t frame_size = static_cast<uint32_t>(htonl(size - sizeof(int32_t)));
    memcpy(buf, &frame_size, sizeof(int32_t));

    // send response to client
    conn_->send(buf, size);
  } catch (const TTransportException& ex) {
    LOG(ERROR) << "ThriftServer TTransportException: " << ex.what();
  } catch (const std::exception& ex) {
    LOG(ERROR) << "ThriftServer std::exception: " << ex.what();
  } catch (...) {
    LOG(ERROR) << "ThriftServer unknown exception";
  }
}

}  // namespace nev
