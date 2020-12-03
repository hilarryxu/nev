#include "nev/channel.h"

#include "base/logging.h"

#include "nev/event_loop.h"
#include "nev/libev_prelude.h"

namespace nev {

namespace {

void ioWatcherCb(struct ev_loop* loop, struct ev_io* io, int revents) {
  Channel* channel = static_cast<Channel*>(io->data);
  channel->set_revents(revents);
  channel->handleEvent();
}

}  // namespace

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

Channel::Channel(EventLoop* loop, SocketDescriptor sockfd, int fd)
    : impl_(std::make_unique<Impl>()),
      loop_(loop),
      sockfd_(sockfd),
      fd_(fd),
      events_(0),
      revents_(0),
      event_handling_(false) {
  ev_init(impl_->io_watcher(), &ioWatcherCb);
  impl_->io_watcher()->data = this;
}

Channel::~Channel() {
  LOG(DEBUG) << "Channel::dtor at " << this << " sockfd = " << sockfd_
             << ", event_handling_ = " << event_handling_;
  // DCHECK(!event_handling_);
  // 现在这个断言会失败

  // FIXME(xcc): ev_io_stop
}

struct ev_io* Channel::io_watcher() {
  return impl_->io_watcher();
}

void Channel::update() {
  loop_->updateChannel(this);
}

void Channel::handleEvent() {
  LOG(DEBUG) << "BEGIN Channel::handleEvent at " << this
             << " revents = " << revents_;
  event_handling_ = true;

  // 处理读事件
  if (revents_ & EV_READ) {
    if (read_cb_)
      read_cb_();
  }

  // 处理写事件
  if (revents_ & EV_WRITE) {
    if (write_cb_)
      write_cb_();
  }

  LOG(DEBUG) << "Channel::handleEvent at " << this
             << " try set event_handling_ = false, event_handling_ = "
             << event_handling_;
  event_handling_ = false;
  LOG(DEBUG) << "Channel::handleEvent at " << this
             << " after set event_handling_ = false, event_handling_ = "
             << event_handling_;

  LOG(DEBUG) << "END Channel::handleEvent at " << this
             << " revents = " << revents_;
}

}  // namespace nev
