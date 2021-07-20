#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <boost/asio.hpp>
#include "boost/bind.hpp"
#include "AlcPacket.h"
#include "EncodingSymbol.h"
#include "File.h"
#include "FileDeliveryTable.h"

const short multicast_port = 40085;

std::unique_ptr<LibFlute::FileDeliveryTable> fdt;
std::map<uint64_t, std::unique_ptr<LibFlute::File>> files;

class receiver
{
public:
  receiver(boost::asio::io_service& io_service,
      const boost::asio::ip::address& listen_address,
      const boost::asio::ip::address& multicast_address)
    : socket_(io_service)
  {
    // Create the socket so that multiple may be bound to the same address.
    boost::asio::ip::udp::endpoint listen_endpoint(
        listen_address, multicast_port);
    socket_.open(listen_endpoint.protocol());
    socket_.set_option(boost::asio::ip::multicast::enable_loopback(true));
    socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    socket_.bind(listen_endpoint);

    // Join the multicast group.
    socket_.set_option(
        boost::asio::ip::multicast::join_group(multicast_address));

    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&receiver::handle_receive_from, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_receive_from(const boost::system::error_code& error,
      size_t bytes_recvd)
  {
    if (!error)
    {
    //  std::cout.write(data_, bytes_recvd);
    //  std::cout << std::endl;
    //
      auto alc = LibFlute::AlcPacket(data_, bytes_recvd);
      if (alc.toi() == 0 && (!fdt || fdt->instance_id() != alc.fdt_instance_id())) {
        if (files.find(alc.toi()) == files.end()) {
          files.emplace(alc.toi(), std::make_unique<LibFlute::File>(alc.toi(), alc.fec_oti()));
        }
      }

      if (files.find(alc.toi()) != files.end() && !files[alc.toi()]->complete()) {
        auto encoding_symbols = LibFlute::EncodingSymbol::initialize_from_payload(
            data_ + alc.header_length(), 
            bytes_recvd - alc.header_length(),
            files[alc.toi()]->fec_oti());

        for (const auto& symbol : encoding_symbols) {
          files[alc.toi()]->put_symbol(symbol);
        }

        auto file = files[alc.toi()].get();
        if (files[alc.toi()]->complete()) {
          std::cout << "File complete: " << std::to_string(alc.toi()) << std::endl;
          if (alc.toi() == 0) { // parse complete FDT
            fdt = std::make_unique<LibFlute::FileDeliveryTable>(
                alc.fdt_instance_id(), files[alc.toi()]->buffer(), files[alc.toi()]->length());

            files.erase(alc.toi());
            for (auto file_entry : fdt->file_entries()) {
              // automatically receive all files in the FDT
              if (files.find(file_entry.toi) == files.end()) {
                files.emplace(file_entry.toi, std::make_unique<LibFlute::File>(file_entry.toi, file_entry.fec_oti));
              }
            }
          }
        }
      }

      socket_.async_receive_from(
          boost::asio::buffer(data_, max_length), sender_endpoint_,
          boost::bind(&receiver::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
  }

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  enum { max_length = 2048 };
  char data_[max_length];
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: receiver <listen_address> <multicast_address>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    receiver 0.0.0.0 239.255.0.1\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    receiver 0::0 ff31::8000:1234\n";
      return 1;
    }

    boost::asio::io_service io_service;
    receiver r(io_service,
        boost::asio::ip::address::from_string(argv[1]),
        boost::asio::ip::address::from_string(argv[2]));
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
