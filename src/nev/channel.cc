#include "nev/channel.h"

#include "base/logging.h"

#include "nev/event_loop.h"
#include "nev/libev_prelude.h"

namespace nev {

namespace {

void ioWatcherCb(struct ev_loop* loop, struct ev_io* io, int revents) {
  Channel* channel = static_cast<Channel*>(io->data);
  channel->set_revents(revents);
  channel->handleEvent(base::TimeTicks::Now());
}

}  // namespace

const int Channel::kNoneEvent = EV_NONE;
const int Channel::kReadEvent = EV_READ;
const int Channel::kWriteEvent = EV_WRITE;

class Channel::Impl {
 public:
  Impl(Channel* ch) {
    ev_init(&io_watcher_, &ioWatcherCb);
    io_watcher_.data = ch;
  }
  ~Impl() {}

  struct ev_io io_watcher_;
};

Channel::Channel(EventLoop* loop, SocketDescriptor sockfd, int fd)
    : impl_(new Impl(this)),
      loop_(loop),
      sockfd_(sockfd),
      fd_(fd),
      events_(0),
      revents_(0),
      tied_(false),
      event_handling_(false) {}

Channel::~Channel() {
  LOG(DEBUG) << "Channel::dtor at " << this << " sockfd = " << sockfd_;
  DCHECK(!event_handling_);

  // FIXME(xcc): ensure ev_io_stop?
  DCHECK(ev_is_pending(io_watcher()) == 0);
  DCHECK(ev_is_active(io_watcher()) == 0);
}

void Channel::tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

struct ev_io* Channel::io_watcher() {
  return &(impl_->io_watcher_);
}

void Channel::update() {
  loop_->updateChannel(this);
}

void Channel::remove() {
  DCHECK(isNoneEvent());
  loop_->removeChannel(this);
}

void Channel::handleEvent(base::TimeTicks receive_time) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      handleEventWithGuard(receive_time);
    }
  } else {
    handleEventWithGuard(receive_time);
  }
}

void Channel::handleEventWithGuard(base::TimeTicks receive_time) {
  event_handling_ = true;

  // 处理读事件
  if (revents_ & EV_READ) {
    if (read_cb_)
      read_cb_(receive_time);
  }

  // 处理写事件
  if (revents_ & EV_WRITE) {
    if (write_cb_)
      write_cb_();
  }

  event_handling_ = false;
}

}  // namespace nev
