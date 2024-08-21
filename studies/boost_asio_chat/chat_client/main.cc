#include <unistd.h>

#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "../chat_message.h"
#include "boost/asio/connect.hpp"
#include "boost/asio/error.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/posix/stream_descriptor.hpp"
#include "boost/asio/read_until.hpp"
#include "boost/asio/streambuf.hpp"
#include "boost/asio/write.hpp"
#include "boost/system/detail/error_code.hpp"

using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;

class posix_chat_client {
 public:
  posix_chat_client(boost::asio::io_context& io, const tcp::resolver::results_type& endpoints)
      : socket { io },
        input { io, dup(STDIN_FILENO) },
        output { io, dup(STDOUT_FILENO) },
        input_buf { max_body_length } {
    do_connect(endpoints);
  }

 private:
  void do_connect(const tcp::resolver::results_type& endpoints) {
    boost::asio::async_connect(socket, endpoints, [this](boost::system::error_code ec, tcp::endpoint) {
      if (!ec) {
        do_read_header();
        do_read_input();
      }
    });
  }

  void do_read_header() {
    boost::asio::async_read(socket, boost::asio::buffer(read_msg.get_data(), header_length),
                            [this](boost::system::error_code ec, std::size_t) {
                              if (!ec && read_msg.decode_header()) {
                                do_read_body();
                              } else {
                                close();
                              }
                            });
  }

  void do_read_body() {
    boost::asio::async_read(socket, boost::asio::buffer(read_msg.get_body(), read_msg.get_body_length()),
                            [this](boost::system::error_code ec, std::size_t) {
                              if (!ec) {
                                do_write_output();
                              } else {
                                close();
                              }
                            });
  }

  void do_write_output() {
    static char eol[] { '\n' };
    std::array<boost::asio::const_buffer, 2> buffers {
      { boost::asio::buffer(read_msg.get_body(), read_msg.get_body_length()), boost::asio::buffer(eol) }
    };
    boost::asio::async_write(output, buffers, [this](boost::system::error_code ec, std::size_t) {
      if (!ec) {
        do_read_header();
      } else {
        close();
      }
    });
  }

  void do_read_input() {
    boost::asio::async_read_until(input, input_buf, '\n', [this](boost::system::error_code ec, std::size_t length) {
      if (!ec) {
        write_msg.set_body_length(length - 1);
        input_buf.sgetn(write_msg.get_body(), length - 1);
        input_buf.consume(1);
        write_msg.encode_header();
        do_write_message();
      } else if (ec == boost::asio::error::not_found) {
        write_msg.set_body_length(input_buf.size());
        input_buf.sgetn(write_msg.get_body(), input_buf.size());
        write_msg.encode_header();
        do_write_message();
      } else {
        close();
      }
    });
  }

  void do_write_message() {
    boost::asio::async_write(socket, boost::asio::buffer(write_msg.get_data(), write_msg.get_length()),
                             [this](boost::system::error_code ec, std::size_t) {
                               if (!ec) {
                                 do_read_input();
                               } else {
                                 close();
                               }
                             });
  }

  void close() {
    socket.close();
    input.close();
    output.close();
  }

  tcp::socket socket;
  posix::stream_descriptor input;
  posix::stream_descriptor output;
  chat_message read_msg;
  chat_message write_msg;
  boost::asio::streambuf input_buf;
};

int main() {
  boost::asio::io_context io;
  tcp::resolver resolver { io };
  auto endpoints { resolver.resolve("127.0.0.1", "9090") };

  posix_chat_client client { io, endpoints };

  io.run();
}