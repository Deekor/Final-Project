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
#include <vector>
#include <fstream>
#include <map>

using boost::asio::ip::tcp;

class spreadsheet;
class session;

class server
  {
  public:

    std::vector<spreadsheet> spreadsheets;
  
    server(boost::asio::io_service& io_service, short port);

    bool createSpreadsheet(session* session, std::string name, std::string password);
    bool joinSpreadsheet(session* session, std::string name, std::string password);
    bool leaveSpreadsheet(session* session, std::string spreadsheetname);
  
  private:
    void start_accept();
    void handle_accept(session* new_session, const boost::system::error_code& error);

    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
  };




//-------------------------//
//-------- SESSION --------//
//-------------------------//
class session
{
public:
  session(boost::asio::io_service& io_service, server * server)
    : socket_(io_service)
  {
    ser = server;
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
    // std::string message = "HEY";
    // size_t size = 3;
    // sendMessage(message, size);
  }

private:

  server * ser;

  //Callback method for everytime we read some info. Right now it just sends whatever it reads back. An echo
  //Probably needs a new name
  void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
  {
    std::string mess = "";
    if (!error)
    {
        //std::cout << "Reading " << data_ << std::endl;
        //boost::asio::async_write(socket_, boost::asio::buffer(data_, bytes_transferred), boost::bind(&session::sendCallback, this, boost::asio::placeholders::error));
        std::cout << "Received " << data_ << std::endl;

      //The Following if statements are for message handling.
      
      //-------------------------//
      //-------- CREATE ---------//
      //-------------------------//
      if (data_[1] == 'R')
      {
        std::string name = "";
        std::string pswd = "";
        int startpassfor;

        for(int i =13; i <=bytes_transferred; i++)
        {
          if(data_[i] == '\\' && data_[i+1] == 'n')
            break;
          name += data_[i];
          startpassfor = i;
        }

        for(int i = startpassfor + 12; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\\' && data_[i+1] == 'n')
            break;
          pswd += data_[i];
        }


        if(!ser->createSpreadsheet(this, name, pswd)) //if it failed to create
        {
          mess = "CREATE FAIL\nName:" + name +"\nA spreadsheet with that name already exists\n";
          sendMessage(mess, mess.size());
        }
        else
        {
          spreadsheetname = name;
          mess = "CREATE OK\nName:" + name +"\nPassword:" + pswd +"\n";
          sendMessage(mess, mess.size());
        }
      }
      //JOIN MESSAGE
      else if (data_[1] == 'O')
      {
        std::string name = "";
        std::string pswd = "";
        int startpassfor;

        for(int i =11; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\\' && data_[i+1] == 'n')
            break;
          name += data_[i];
          startpassfor = i;
        }

        for(int i = startpassfor + 12; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\\' && data_[i+1] == 'n')
            break;
          pswd += data_[i];
        }


        if(!ser->joinSpreadsheet(this, name, pswd)) //if it failed to create
        {
          mess = "JOIN FAIL\nName:" + name +"\nA spreadsheet with that name doesn't exist.\n";
          sendMessage(mess, mess.size());
        }
        else
        {
          spreadsheetname = name;
          mess = "JOIN OK\nName:" + name +"\nVersion:" + pswd +"\n";
          sendMessage(mess, mess.size());
        }
      }
      //CHANGE MESSAGE
      else if (data_[1] == 'H')
      {
        mess = "Change has been called";
        sendMessage(mess, mess.size());
      }
      //UNDO MESSAGE
      else if (data_[1] == 'N')
      {
        mess = "Undo has been called";
        sendMessage(mess, mess.size());
      }
      //SAVE MESSAGE
      else if (data_[1] == 'A')
      {
        mess = "Save has been called";
        sendMessage(mess, mess.size());
      }
      //LEAVE MESSAGE
      else if (data_[3] == 'V')
      {
        ser->leaveSpreadsheet(this, spreadsheetname);
        mess = "Leave has been called";
        socket_.close();
        sendMessage(mess, mess.size());
      }
      else
      {
        mess = "ERROR\n";
        sendMessage(mess, mess.size());
      }
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

  //Used to send a message to the spreadsheet
  void sendMessage(std::string message, size_t bytes_to_transfer)
  {
    boost::asio::async_write(socket_, boost::asio::buffer(message, bytes_to_transfer), boost::bind(&session::sendCallback, this, boost::asio::placeholders::error));
  }

  //The old send callback method. Still here jsut for refrence, will be deleted when we dont need it anymore.
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

  //this method is called each time we send a message.
  void sendCallback(const boost::system::error_code& error)
  {
    std::cout << "In our write call" << std::endl;

  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  std::string spreadsheetname;
};

//-------------------------//
//------ SPREADSHEET ------//
//-------------------------//
class spreadsheet
{
public:
  std::string name;
  std::string password;
  std::vector<session*> sessions;
  std::map<std::string, std::string> cells;
  int version;

  
  //constructor
  spreadsheet(std::string name, std::string password)
  {
      std::ifstream ifs;
      std::string filename = std::string("ss/") + name + ".txt";
      ifs.open (&filename[0], std::ifstream::in);
      this->name = name;
      this->password = password;
      version = 0;

      //Check if the spreadsheet already exists
      if ( (ifs.rdstate() & std::ifstream::failbit ) != 0 )
      {
        //Spreadsheet doesnt exist, create one
        std::ofstream ofs; 
        ofs.open (&filename[0], std::ofstream::out | std::ofstream::app);
        ofs << "PASS " << password << std::endl;
        ofs << "I wish I had Jeff's PPI";
        ofs.close();
      }
      else
      {
        //if the file does exist, Open it.


      }
      
      ifs.close();
  }
  bool linkSession(session* s)
  {
      sessions.push_back(s);
      return true;
  }

private:

};

//-------------------------//
//-------- SERVER ---------//
//-------------------------//
  server::server(boost::asio::io_service& io_service, short port)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
  {
    start_accept();
  }

  //create a new spreadsheet if it doesn't already exist. Link the session to the spreadsheet.
  bool server::createSpreadsheet(session* session, std::string name, std::string password)
  {
    bool found = false;
    for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == name) //spread sheet already exists
        {
          found = true;

        }
    }
    if(!found) //create a spreadsheet
    {
      spreadsheet * spr = new spreadsheet(name, password);
      spr->linkSession(session);
      spreadsheets.push_back(*spr);
    }
    else
    {
      return false;
    }
    return true;
  }

  //Join a session to spreadsheet.
  bool server::joinSpreadsheet(session* session, std::string name, std::string password)
  {
    bool found = false;
    for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == name) //spread sheet already exists
        {
          found = true;
          spreadsheets.at(i).linkSession(session);
        }
    }
    if(!found) //create a spreadsheet
    {
      //send fail
      return false;
    }
    return true;
  }
 
  bool server::leaveSpreadsheet(session* session, std::string spreadsheetname)
  {
    for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == spreadsheetname) //spread sheet already exists
        {
          spreadsheets.erase(spreadsheets.begin()+i);
          return true;
        }
    }
    return false;
  }

  void server::start_accept()
  {
    //Create a new session with the socket that connects
    session* new_session = new session(io_service_, this);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  //Method called when a new socket connects
  //a callback method
  void server::handle_accept(session* new_session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      //Client has connected
      std::cout << "Client Connection Established" << std::endl;
      new_session->start(); //start the session
    }
    else
    {
      delete new_session;
    }

    start_accept();
  }




//-------------------------//
//-------- MAIN -----------//
//-------------------------//
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
