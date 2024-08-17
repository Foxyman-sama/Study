#include <boost/asio.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <print>
#include <thread>

#include "../chat_message.h"
#include "boost/asio/connect.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/system/detail/error_code.hpp"

using boost::asio::ip::tcp;

using chat_message_queue = std::deque<chat_message>;

class chat_client {
 public:
  chat_client(boost::asio::io_context& io, const tcp::resolver::results_type& endpoints) : io { io }, socket { io } {
    do_connect(endpoints);
  }

  void write(const chat_message& msg) {
    boost::asio::post(io, [this, msg]() {
      auto write_in_progress { !write_msgs.empty() };
      write_msgs.push_back(msg);
      if (!write_in_progress) {
        do_write();
      }
    });
  }

  void close() {
    boost::asio::post(io, [this]() { socket.close(); });
  }

 private:
  void do_connect(const tcp::resolver::results_type& endpoints) {
    boost::asio::async_connect(socket, endpoints, [this](boost::system::error_code ec, tcp::endpoint) {
      if (!ec && read_msg.decode_header()) {
        do_read_body();
      } else {
        socket.close();
      }
    });
  }

  void do_read_body() {
    boost::asio::async_read(socket, boost::asio::buffer(read_msg.get_body(), read_msg.get_body_length()),
                            [this](boost::system::error_code ec, std::size_t) {
                              if (!ec) {
                                std::cout.write(read_msg.get_body(), read_msg.get_body_length());
                                std::cout << "\n";
                                do_read_body();
                              } else {
                                socket.close();
                              }
                            });
  }

  void do_write() {
    boost::asio::async_write(socket,
                             boost::asio::buffer(write_msgs.front().get_data(), write_msgs.front().get_length()),
                             [this](boost::system::error_code ec, std::size_t) {
                               if (!ec) {
                                 write_msgs.pop_front();
                                 if (!write_msgs.empty()) {
                                   do_write();
                                 } else {
                                   socket.close();
                                 }
                               }
                             });
  }

  boost::asio::io_context& io;
  tcp::socket socket;
  chat_message read_msg;
  chat_message_queue write_msgs;
};

int main() {
  boost::asio::io_context io;

  tcp::resolver resolver { io };
  const auto endpoints { resolver.resolve("127.0.0.1", "9090") };
  chat_client client { io, endpoints };

  std::thread t { [&io]() { io.run(); } };

  char line[max_body_length + 1];
  while (std::cin.getline(line, max_body_length + 1)) {
    std::print("Your message: {0}", line);

    chat_message msg;
    msg.set_body_length(std::strlen(line));
    std::memcpy(msg.get_body(), line, msg.get_body_length());
    msg.encode_header();
    client.write(msg);
  }

  client.close();
  t.join();
}