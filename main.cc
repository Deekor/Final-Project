//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class session
{
public:
  session(boost::asio::io_service& io_service)
    : socket_(io_service)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

private:
  void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
  {
    if (!error)
    {
      std::cout << "Reading " << data_ << std::endl;
      boost::asio::async_write(socket_, boost::asio::buffer(data_, bytes_transferred), boost::bind(&session::ourWriteCall, this, boost::asio::placeholders::error));
    }
    else
    {
      delete this;
    }
  }

  //Used to send a message to the spreadsheet
  void sendMessage(const boost::system::error_code& error, std::string message, size_t bytes_to_transfer)
  {
    boost::asio::async_write(socket_, boost::asio::buffer(message, bytes_to_transfer), boost::bind(&session::ourWriteCall, this, boost::asio::placeholders::error));
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      std::cout << "Writing " << data_ << std::endl;
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      delete this;
    }
  }

  void ourWriteCall(const boost::system::error_code& error)
  {
    std::cout << "In our write call" << std::endl;
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(boost::asio::io_service& io_service, short port)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    session* new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(session* new_session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      std::cout << "Client Connection Established" << std::endl;
      new_session->start();
    }
    else
    {
      delete new_session;
    }

    start_accept();
  }

  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  try
  {
    
    //We are in!
    std::cout << "Server Established" << std::endl;

    boost::asio::io_service io_service;

    using namespace std; // For atoi.
    server s(io_service, atoi("1984")); //create a server on port 1984

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}