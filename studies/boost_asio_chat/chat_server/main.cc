#include <boost/asio.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>

#include "../chat_message.h"
#include "boost/asio/impl/write.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/system/detail/error_code.hpp"

using boost::asio::ip::tcp;
using chat_message_queue = std::deque<chat_message>;

class chat_participant {
 public:
  virtual ~chat_participant() {}

  virtual void deliver(const chat_message &msg) = 0;
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

  void deliver(const chat_message &msg) {
    recent_msgs.push_back(msg);

    while (recent_msgs.size() > max_recent_msgs) {
      recent_msgs.pop_front();
    }

    for (auto participiant : participants) {
      participiant->deliver(msg);
    }
  }

 private:
  enum { max_recent_msgs = 100 };

  std::set<chat_participant_ptr> participants;
  chat_message_queue recent_msgs;
};

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session> {
 public:
  chat_session(tcp::socket socket, chat_room &room) : socket { std::move(socket) }, room { room } {}

  void start() { room.join(shared_from_this()); }

  void deliver(const chat_message &msg) {
    auto wirte_in_progress { !write_msgs.empty() };
    write_msgs.push_back(msg);
    if (!wirte_in_progress) {
      do_write();
    }
  }

 private:
  void do_read_header() {
    auto self { shared_from_this() };
    boost::asio::async_read(socket, boost::asio::buffer(read_msg.get_data(), header_length),
                            [this, self](boost::system::error_code ec, std::size_t) {
                              if (!ec && read_msg.decode_header()) {
                                do_read_body();
                              } else {
                                room.leave(shared_from_this());
                              }
                            });
  }

  void do_read_body() {
    auto self { shared_from_this() };
    boost::asio::async_read(socket, boost::asio::buffer(read_msg.get_body(), read_msg.get_body_length()),
                            [this, self](boost::system::error_code ec, std::size_t) {
                              if (!ec) {
                                room.deliver(read_msg);
                                do_read_header();
                              } else {
                                room.leave(shared_from_this());
                              }
                            });
  }

  void do_write() {
    auto self { shared_from_this() };
    boost::asio::async_write(socket,
                             boost::asio::buffer(write_msgs.front().get_data(), write_msgs.front().get_length()),
                             [this, self](boost::system::error_code ec, std::size_t) {
                               if (!ec) {
                                 write_msgs.pop_front();
                                 if (!write_msgs.empty()) {
                                   do_write();
                                 }
                               } else {
                                 room.leave(shared_from_this());
                               }
                             });
  }

  tcp::socket socket;
  chat_room &room;
  chat_message read_msg;
  chat_message_queue write_msgs;
};

class chat_server {
 public:
  chat_server(boost::asio::io_context &io, const tcp::endpoint &endpoint) : acceptor(io, endpoint) {}

 private:
  void do_accept() {
    acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::make_shared<chat_session>(std::move(socket), room)->start();
      }

      do_accept();
    });
  }

  tcp::acceptor acceptor;
  chat_room room;
};

int main() {
  std::cout << "SERVER STARTED :>>\n";

  boost::asio::io_context io;

  const tcp::endpoint endpoint { tcp::v4(), 9090 };
  chat_server server { io, endpoint };

  io.run();
}