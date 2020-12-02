#include "nev/channel.h"

#include "nev/event_loop.h"
#include "nev/libev_prelude.h"

namespace nev {

const int Channel::kNoneEvent = EV_NONE;
const int Channel::kReadEvent = EV_READ;
const int Channel::kWriteEvent = EV_WRITE;

class Channel::Impl {
 public:
  Impl() {}

  ~Impl() {}

  struct ev_io* io_watcher() {
    return &io_watcher_;
  }

 private:
  struct ev_io io_watcher_;
};

Channel::Channel(EventLoop* loop, SocketDescriptor fd_arg)
    : impl_(std::make_unique<Impl>()),
      loop_(loop),
      fd_(fd_arg),
      events_(0),
      revents_(0) {}

Channel::~Channel() {}

struct ev_io* Channel::io_watcher() {
  return impl_->io_watcher();
}

void Channel::update() {
  loop_->updateChannel(this);
}

void Channel::handleEvent() {
  if (revents_ & EV_READ) {
    if (read_cb_)
      read_cb_();
  }

  if (revents_ & EV_WRITE) {
    if (write_cb_)
      write_cb_();
  }
}

}  // namespace nev
