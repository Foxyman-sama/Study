#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <exception>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "boost/asio/buffer.hpp"
#include "boost/system/detail/error_code.hpp"

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::redirect_error;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;

class chat_participant {
 public:
  virtual ~chat_participant() {}

  virtual void deliver(const std::string& msg) = 0;
};

using chat_participant_ptr = std::shared_ptr<chat_participant>;

class chat_room {
 public:
  void join(chat_participant_ptr participant) {
    participants.insert(participant);

    for (auto msg : recent_msgs) {
      participant->deliver(msg);
    }
  }

  void leave(chat_participant_ptr participant) { participants.erase(participant); }

  void deliver(const std::string& msg) {
    recent_msgs.push_back(msg);

    while (recent_msgs.size() > max_recent_msgs) {
      recent_msgs.pop_front();
    }

    for (auto participant : participants) {
      participant->deliver(msg);
    }
  }

 private:
  enum { max_recent_msgs = 100 };

  std::set<chat_participant_ptr> participants;
  std::deque<std::string> recent_msgs;
};

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session> {
 public:
  chat_session(tcp::socket socket, chat_room& room)
      : socket { std::move(socket) }, timer { socket.get_executor() }, room { room } {
    timer.expires_at(std::chrono::steady_clock::time_point::max());
  }

  void start() {
    room.join(shared_from_this());

    co_spawn(socket.get_executor(), [self = shared_from_this()] { return self->reader(); }, detached);

    co_spawn(socket.get_executor(), [self = shared_from_this()] { return self->writer(); }, detached);
  }

  void deliver(const std::string& msg) {
    write_msgs.push_back(msg);
    timer.cancel_one();
  }

 private:
  awaitable<void> reader() {
    try {
      for (std::string read_msg;;) {
        auto n { co_await boost::asio::async_read_until(socket, boost::asio::dynamic_buffer(read_msg, 1024), "\n",
                                                        use_awaitable) };

        room.deliver(read_msg.substr(0, n));
        read_msg.erase(0, n);
      }
    } catch (const std::exception&) {
      stop();
    }
  }

  awaitable<void> writer() {
    try {
      while (socket.is_open()) {
        if (write_msgs.empty()) {
          boost::system::error_code ec;
          co_await timer.async_wait(redirect_error(use_awaitable, ec));
        } else {
          co_await boost::asio::async_write(socket, boost::asio::buffer(write_msgs.front()), use_awaitable);
          write_msgs.pop_front();
        }
      }
    } catch (const std::exception&) {
      stop();
    }
  }

  void stop() {
    room.leave(shared_from_this());
    socket.close();
    timer.cancel();
  }

  tcp::socket socket;
  boost::asio::steady_timer timer;
  chat_room& room;
  std::deque<std::string> write_msgs;
};

awaitable<void> listener(tcp::acceptor acceptor) {
  chat_room room;

  for (;;) {
    std::make_shared<chat_session>(co_await acceptor.async_accept(use_awaitable), room)->start();
  }
}

int main() {
  boost::asio::io_context io;

  co_spawn(io, listener(tcp::acceptor { io, { tcp::v4(), 9090 } }), detached);

  boost::asio::signal_set signals { io, SIGINT, SIGTERM };
  signals.async_wait([&](auto, auto) { io.stop(); });

  io.run();
}