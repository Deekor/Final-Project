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
#include <sstream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using boost::asio::ip::tcp;

class spreadsheet;
class session;

class server
  {
  public:

    std::vector<spreadsheet> spreadsheets;
  
    server(boost::asio::io_service& io_service, short port);

    bool createSpreadsheet(session* session, std::string name, std::string password);
    int joinSpreadsheet(session* session, std::string name, std::string password);
    bool leaveSpreadsheet(session* session, std::string spreadsheetname);
    bool saveSheet(std::string name);
    std::string getXML(std::string name);
     std::string sendChange(std::string name, int version, std::string cell, std::string content, std::string length);
    bool makeChange(std::string name, int version, std::string cell, std::string content, std::string length);

  
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
  memset(data_,'\0',max_length);
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    // std::string message = "HEY";
    // size_t size = 3;
    // sendMessage(message, size);
  }

    //Used to send a message to the spreadsheet
  void sendMessage(std::string message, size_t bytes_to_transfer)
  {
    boost::asio::async_write(socket_, boost::asio::buffer(message, bytes_to_transfer), boost::bind(&session::sendCallback, this, boost::asio::placeholders::error));
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
        

      //The Following if statements are for message handling.
      
      //-------------------------//
      //-------- CREATE ---------//
      //-------------------------//
      if (data_[1] == 'R')
      {
        std::string name = "";
        std::string pswd = "";
        int startpassfor;
        
        
        //std::cout << "Received: " << data_ << std::endl;
        
        for(int i = 0; i <=bytes_transferred; i++)
        {
			std::cout << data_[i] << " ";
        }
        

        for(int i = 12; i <=bytes_transferred; i++)
        {
			//std::cout << data_[i] << " ";
          if(data_[i] == '\n')
            break;
          name += data_[i];
          startpassfor = i;
        }

        for(int i = startpassfor + 11; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\n')
            break;
          pswd += data_[i];
        }
		//std::cout << "name is: " <<name << std::endl;
		//std::cout << "pass is: " <<pswd << std::endl;


        if(!ser->createSpreadsheet(this, name, pswd)) //if it failed to create
        {
          mess = "CREATE FAIL\nName:" + name +"\nA spreadsheet with that name already exists\n";
          		//std::cout << "mess is: " << mess << std::endl;

          sendMessage(mess, mess.size());
        }
        else
        {
          spreadsheetname = name;
          mess = "CREATE OK\nName:" + name +"\nPassword:" + pswd +"\n";
          //std::cout << "mess is: " << mess << std::endl;
          sendMessage(mess, mess.size());
        }
      }
      //JOIN MESSAGE
      else if (data_[1] == 'O')
      {
        std::string name = "";
        std::string pswd = "";
        std::string xml = "";
        int length = 0;

        int startpassfor;

        for(int i =10; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\n')
            break;
          name += data_[i];
          startpassfor = i;
        }

        for(int i = startpassfor + 11; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\n')
            break;
          pswd += data_[i];
        }

        int version = ser->joinSpreadsheet(this, name, pswd);

        if(version == -1) //if it failed to create
        {
          mess = "JOIN FAIL\nName:" + name +"\nA spreadsheet with that name doesn't exist.\n";
          std::cout << "mess is: " << mess << std::endl;
          sendMessage(mess, mess.size());
        }
        else if(version == -2) //found a spreadsheet, but passwords didnt match
        {
          mess = "JOIN FAIL\nName:" + name +"\nThe password for that spreadsheet was incorrect.\n";
          std::cout << "mess is: " << mess << std::endl;
          sendMessage(mess, mess.size());
        }
        else
        {
          xml = ser->getXML(name);
          length = xml.size();

          std::ostringstream s;
          s << "JOIN OK\nName:" << name << "\nVersion:" << version << "\nLength:" << length << "\n" << xml << "\n"; //build the string to return (becuase of ints).
          mess = s.str();
          sendMessage(mess, mess.size());
        }
      }
      //CHANGE MESSAGE
      else if (data_[1] == 'H')
      {
        std::string name = "";
        std::string cell = "";
        std::string content = "";
        std::string length = "";
        std::string version = "";
        int startpassfor = 0;
        
    for(int i = 12; i <= bytes_transferred; i++)
        {
      //std::cout << data_[i] << " ";
          if(data_[i] == '\n')
            break;
          name += data_[i];
          startpassfor = i;
        }

        for(int i = startpassfor + 10; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\n')
            break;
          version += data_[i];
          startpassfor = i;
        }
        
        for(int i = startpassfor + 7; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\n')
            break;
          cell += data_[i];
          startpassfor = i;
        }
        
        for(int i = startpassfor + 9; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\n')
            break;
          length += data_[i];
          startpassfor = i;
        }
        
        for(int i = startpassfor + 2; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\n')
            break;
          content += data_[i];
        }
      std::cout << "message received is: " << data_ << std::endl;
      
      if(ser->makeChange(name, atoi(version.c_str()), cell, content, length))
      {
      mess = "CHANGE OK\nName:" + name + "\nVersion:" + version + "\n";
      sendMessage(mess, mess.size());
      ser->sendChange(name, atoi(version.c_str())+1, cell, content, length);
      std::cout << "change ok"<< std::endl;
      }
      else
      {
      mess = "CHANGE FAIL";
      sendMessage(mess, mess.size());
      std::cout << "change fail "<< std::endl;
      }
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

        std::string name = "";

        for(int i =11; i <= bytes_transferred; i++)
        {
          if(data_[i] == '\n')
            break;
          name += data_[i];
        }

        if(ser->saveSheet(name))
          mess = "SAVE OK\nName:" + name + "\n";
        else
          mess = "SAVE FAIL\nName:" + name + "\nThe save failed because..." + "\n";
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
      
      memset(data_,'\0',max_length);
      
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

      //Check if the spreadsheet already exists, if not create one
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
      if(spreadsheets.at(i).name == name) //spread sheet already exists in memory
        {
          found = true;

        }
    }
    if(!found) //create a spreadsheet
    {
      spreadsheet * spr = new spreadsheet(name, password);
      spreadsheets.push_back(*spr);
    }
    else
    {
      return false;
    }
    return true;
  }

  //Join a session to spreadsheet.
  int server::joinSpreadsheet(session* session, std::string name, std::string password)
  {
    int found = -1;

    //check all spreadsheets in memory
    for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == name) //spread sheet already exists
        {
          //check the password
          if(spreadsheets.at(i).password == password)
          {
            //passwords match
            found = spreadsheets.at(i).version;
            spreadsheets.at(i).linkSession(session);
          }
          else
          {
            //passwords don't match.
            return -2;
          }
        }
    }
    //No spreadsheet in memory, check files
    if(found < 0)
    {
      std::ifstream ifs;
      std::string filename = std::string("ss/") + name + ".xml";
      ifs.open (&filename[0], std::ifstream::in);
      //Check if the spreadsheet file already exists
      if ( (ifs.rdstate() & std::ifstream::failbit ) != 0 )
      {
        //doesnt exist return false
        ifs.close();
        return -1;
      }
      else //found the file open it and join the session
      {
        //create a spreadsheet and link it
        spreadsheet * spr = new spreadsheet(name, password);


        //read the xml
        boost::property_tree::ptree pt;
        std::string filename = "ss/"+ name + ".xml";
        read_xml(filename, pt);
        std::cout << "About to check the password" << std::endl;
        //need to check the password before we link the session
        BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, pt.get_child("spreadsheet")) 
        {
          if(v.first == "meta")
          {
            if(password != v.second.get<std::string>("password"))
            {
              //passwords dont match
              return -2;
            }
          }
        }
        std::cout << "Passed the password check" << std::endl;

        //link the session
        spr->linkSession(session);

        std::cout << "Populating the tree" << std::endl;
        //for each the pt and add to the spreadsheet map
        BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, pt.get_child("spreadsheet")) 
        {
          if(v.first == "cell")
          {
            std::string name2 = v.second.get<std::string>("name");
            std::string contents = v.second.get<std::string>("contents");
            spr->cells.insert(std::pair<std::string, std::string>(name2, contents));
          }
        }

        //place spreadsheet into memory
        spreadsheets.push_back(*spr);
        return spr->version;
      }

    }
    return found;
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

  bool server::saveSheet(std::string spreadsheetname)
  {
    boost::property_tree::ptree pt;
    

    for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == spreadsheetname) //spread sheet already exists
        {
          std::map<std::string, std::string>::iterator it;
          for(it=spreadsheets.at(i).cells.begin(); it!=spreadsheets.at(i).cells.end(); ++it)
          {
            boost::property_tree::ptree pt1;
            pt1.put("name", it->first);
            pt1.put("contents", it->second);

            pt.add_child("spreadsheet.cell", pt1);
            pt.put("spreadsheet.meta.password", spreadsheets.at(i).password);
          }

          //write the file
          std::string filename = "" + spreadsheetname + ".xml";
          write_xml(filename, pt);
          
        }
    }


  }

  std::string server::getXML(std::string name)
  {
    for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == name) //spread sheet already exists
        {
          std::map<std::string, std::string>::iterator it;
          std::ostringstream s;
          s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?><spreadsheet>";
          for(it=spreadsheets.at(i).cells.begin(); it!=spreadsheets.at(i).cells.end(); ++it)
          {
            s << "<cell>";
            s << "<name>" << it->first << "</name>";
            s << "<contents>" << it->second << "</contents>";
            s << "</cell>";
          }
          s << "</spreadsheet>";

          return s.str();
          
        }
    }
  }

   std::string server::sendChange(std::string name, int version, std::string cell, std::string content, std::string length)
  {
    std::cout << "sendchange method "<< std::endl;
    std::string rtn = "FAIL";
  for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == name) //spread sheet already exists
        {
          spreadsheet temp = spreadsheets.at(i);
          
          for(int i = 0 ; i < temp.sessions.size(); i++)
          {
      std::ostringstream s;
      s << "UPDATE\nName:" << name << "\nVersion:" << version << "\nCell:" << cell <<"\nLength:" <<length << "\n" << content << "\n"; //build the string to return (because of ints).
      std::string mess = s.str();
        
        temp.sessions.at(i)->sendMessage(mess, mess.size());
      }
      rtn = "OK";
          
        }
    }
    
    return rtn;
  }
  
  bool server::makeChange(std::string name, int version, std::string cell, std::string content, std::string length)
  {
    std::cout << "makechange method "  << cell<< std::endl;
  for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == name && spreadsheets.at(i).version == version ) //spread sheet already exists
        {
      std::cout << "name is: " << spreadsheets.at(i).name << std::endl;
      std::cout << "version is: " << spreadsheets.at(i).version << std::endl;
          
          
          std::map<std::string, std::string>::iterator it;
          for(it = spreadsheets.at(i).cells.begin(); it != spreadsheets.at(i).cells.end(); ++it)
          {
        std::cout << "cell names: " << it->first << std::endl;
        if(it->first == cell)
        {
          std::cout << "found cell name: " << it->first << std::endl;
        it->second = content;
        spreadsheets.at(i).version++;
        return true;
        }
          }
          
          spreadsheets.at(i).cells.insert(std::pair<std::string, std::string>(cell, content));
          spreadsheets.at(i).version++;
      return true;
          
          
        }
        return false;
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
