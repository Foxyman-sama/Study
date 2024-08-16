#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr auto header_length { std::size_t { 4 } };
constexpr auto max_body_length { std::size_t { 512 } };

class chat_message {
 public:
  const char *const get_data() const { return data; }

  char *get_data() { return data; }

  std::size_t get_length() const { return header_length + body_length; }

  const char *const get_body() const { return data + header_length; }

  char *get_body() { return data + body_length; }

  std::size_t get_body_length() const { return body_length; }

  void set_body_length(std::size_t new_length) {
    body_length = new_length > max_body_length ? max_body_length : new_length;
  }

  bool decode_header() {
    char header[header_length + 1] {};
    std::strncat(header, data, header_length);

    body_length = std::atoi(header);
    if (body_length > max_body_length) {
      body_length = 0;
      return false;
    }

    return true;
  }

  void encode_header() {
    char header[header_length + 1] {};
    std::sprintf(header, "%4d", static_cast<int>(body_length));
    std::memcpy(data, header, header_length);
  }

 private:
  char data[header_length + max_body_length] {};
  std::size_t body_length { 0 };
};

#endif