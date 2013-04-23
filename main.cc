

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
#include <stack>

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
    std::string sendChange(std::string name, int version, std::string cell, std::string content, std::string length,session* session);
    int makeChange(std::string name, int version, std::string cell, std::string content, std::string length);
    std::pair<std::string, std::string> undo(session* session, std::string spreadsheetname, std::string version);

  
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

  std::string spreadsheetname;
  bool ableToLeave;

  session(boost::asio::io_service& io_service, server * server)
    : socket_(io_service)
  {
    ser = server;
    ableToLeave = true;
    std::cout << "Session associated with server: "<< &server << std::endl;
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
    
    //leave or disconnect
    if((bytes_transferred < 1 || data_[3] == 'V') && ableToLeave)
    {
        ser->leaveSpreadsheet(this, spreadsheetname);
        socket_.close();
        ableToLeave = false;
    }
    std::string mess = "";
    if (!error)
    {
      std::cout << "message received is: " << data_ << std::endl;
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
          //std::cout << "mess is: " << mess << std::endl;
          sendMessage(mess, mess.size());
        }
        else if(version == -2) //found a spreadsheet, but passwords didnt match
        {
          mess = "JOIN FAIL\nName:" + name +"\nThe password for that spreadsheet was incorrect.\n";
          //std::cout << "mess is: " << mess << std::endl;
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
      
      int change = ser->makeChange(name, atoi(version.c_str()), cell, content, length);
      if(change == 1)
      {
        std::ostringstream mess;
        mess << "CHANGE OK\nName:" << name << "\nVersion:" << atoi(version.c_str())+1 << "\n";
        std::string mess2 = mess.str();
        sendMessage(mess2, mess2.size());
        ser->sendChange(name, atoi(version.c_str())+1, cell, content, length, this);
        //std::cout << "change ok"<< std::endl;
      }
      else if(change == 0)
      {
        std::ostringstream mess;
        mess << "CHANGE FAIL\nName:" << name << "\nThe change failed.\n";
        std::string mess2 = mess.str();
        sendMessage(mess2, mess2.size());
        //std::cout << "change fail "<< std::endl;
      }
      else if(change == -1)
        //version problem
      {
        std::ostringstream mess;
        mess << "CHANGE WAIT\nName:" << name << "\nVersion:" << atoi(version.c_str()) << "\n";
        std::string mess2 = mess.str();
        sendMessage(mess2, mess2.size());
      }

      }


      //UNDO MESSAGE
      else if (data_[1] == 'N')
      {
        std::string name = "";
        std::string version = "";
        int startpassfor = 0;

        for(int i = 10; i <= bytes_transferred; i++)
        {
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
        }
        std::cout << "name is  : "<<name << "   version is: " << version <<std::endl;

        std::pair<std::string, std::string> change = ser->undo(this, name, version);
        if(change.first == "fail")
        {
          std::ostringstream mess;
          mess << "UNDO FAIL\nName:" << name << "\nUndo failed.\n";
          std::string mess2 = mess.str();
          sendMessage(mess2, mess2.size());

        }
        else if(change.first == "wait")
        {
          std::ostringstream mess;
          mess << "UNDO WAIT\nName:" << name << "\nVersion:" << version<< "\n";
          std::string mess2 = mess.str();
          sendMessage(mess2, mess2.size());
        }
        else if(change.first == "end")
        {
          std::ostringstream mess;
          mess << "UNDO END\nName:" << name << "\nVersion:" << version<< "\n";
          std::string mess2 = mess.str();
          sendMessage(mess2, mess2.size());
        }
        else
        {
          std::ostringstream mess;
          mess << "UNDO OK\nName:" << name << "\nVersion:" << atoi(version.c_str()) + 1  << "\nCell:" << change.first << "\nLength:" << change.second.size() <<"\n" << change.second << "\n";
          std::string mess2 = mess.str();
          sendMessage(mess2, mess2.size());
        }
      }
      //SAVE MESSAGE
      else if (data_[1] == 'A')
      {

        std::string name = "";

        for(int i =10; i <= bytes_transferred; i++)
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
  std::stack< std::pair<std::string, std::string> > undoStack;
  int version;

  
  //constructor
  spreadsheet(std::string name, std::string password)
  {
      std::ifstream ifs;
      std::string filename = std::string("ss/") + name + ".xml";
      ifs.open (&filename[0], std::ifstream::in);
      this->name = name;
      this->password = password;
      version = 0;
  }
  bool linkSession(session* s)
  {
      s->spreadsheetname = name;
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
      std::cout << "Before adding " << name << "the slist size is " <<  spreadsheets.size() << std::endl;
      std::cout << "Adding " << name << " to the spreadsheet list" <<std::endl;
      spreadsheet * spr = new spreadsheet(name, password);
      spreadsheets.push_back(*spr);
      std::cout << "After adding " << name << "the slist size is " <<  spreadsheets.size() << std::endl;
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
    std::cout << "Leave Requested"<< std::endl;
    for(int i = 0; i < spreadsheets.size(); i++)
    {
       std::cout << "Size of the vecor is " << spreadsheets.at(i).sessions.size() << std::endl;
      if(spreadsheets.at(i).name == spreadsheetname) //spread sheet already exists
        {
          std::cout << "In erase i is " << i << std::endl;
          spreadsheets.at(i).sessions.erase(spreadsheets.at(i).sessions.begin()+i); 
        }
        std::cout << "Size of the vecor after erase is " << spreadsheets.at(i).sessions.size() << std::endl;
        if(spreadsheets.at(i).sessions.empty())//no one else connected, save it
        {
          std::cout << "Everyone is disconnected, saving " << spreadsheetname << std::endl;
          saveSheet(spreadsheetname);
        }
        return true;
    }
    return false;
  }

  bool server::saveSheet(std::string spreadsheetname)
  {
    boost::property_tree::ptree pt;
    
    std::cout << "Save request for " << spreadsheetname << std::endl;

    for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == spreadsheetname) //spread sheet already exists
        {
          std::map<std::string, std::string>::iterator it;

          //the sreadsheet to save is empty
          if(spreadsheets.at(i).cells.empty())
          {
              pt.put("spreadsheet.meta.password", spreadsheets.at(i).password);
          }

          for(it=spreadsheets.at(i).cells.begin(); it!=spreadsheets.at(i).cells.end(); ++it)
          {
            boost::property_tree::ptree pt1;
            pt1.put("name", it->first);
            pt1.put("contents", it->second);

            pt.add_child("spreadsheet.cell", pt1);
            pt.put("spreadsheet.meta.password", spreadsheets.at(i).password);
          }

          //write the file
          std::string filename = "ss/" + spreadsheetname + ".xml";
          write_xml(filename, pt);
          while(!spreadsheets.at(i).undoStack.empty())
            spreadsheets.at(i).undoStack.pop();
          
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

   std::string server::sendChange(std::string name, int version, std::string cell, std::string content, std::string length, session *s)
  {
    std::cout << "sendchange method "<< std::endl;
    std::string rtn = "FAIL";
  for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == name) //spread sheet already exists
        {
          
          
          for(int j = 0 ; j < spreadsheets.at(i).sessions.size(); j++)
          {
            if(spreadsheets.at(i).sessions.at(j)!= s)
            {
              //if()
              std::ostringstream s;
              s << "UPDATE\nName:" << name << "\nVersion:" << version << "\nCell:" << cell <<"\nLength:" <<length << "\n" << content << "\n"; //build the string to return (because of ints).
              std::string mess = s.str();
              
              spreadsheets.at(i).sessions.at(j)->sendMessage(mess, mess.size());
            }
        }
      rtn = "OK";
          
        }
    }
    
    return rtn;
  }
  
  //returns 0 = fail, 1 = ok, -1 = version error
  int server::makeChange(std::string name, int version, std::string cell, std::string content, std::string length)
  {
    //std::cout << "makechange method "  << cell<< std::endl;
  for(int i = 0; i < spreadsheets.size(); i++)
    {
      std::cout << "name is: " << spreadsheets.at(i).name << std::endl;
    if(spreadsheets.at(i).name == name) //spread sheet already exists
    {
      //std::cout << "name is: " << spreadsheets.at(i).name << std::endl;
      //std::cout << "version is: " << spreadsheets.at(i).version << std::endl;
      if(spreadsheets.at(i).version != version)
      {
        return -1;
      }
          
          
      std::map<std::string, std::string>::iterator it;
      for(it = spreadsheets.at(i).cells.begin(); it != spreadsheets.at(i).cells.end(); ++it)
      {
        if(it->first == cell)
        {
          if(content == "")
          {
            spreadsheets.at(i).undoStack.push(std::pair<std::string, std::string>(it->first, it->second));
            spreadsheets.at(i).cells.erase(cell);
            spreadsheets.at(i).version++;
            return 1;
          }
          spreadsheets.at(i).undoStack.push(std::pair<std::string, std::string>(it->first, it->second));
          it->second = content;
          spreadsheets.at(i).version++;
          return 1;
        }
      }
      //else insert it
      spreadsheets.at(i).undoStack.push(std::pair<std::string, std::string>(cell, ""));
      spreadsheets.at(i).cells.insert(std::pair<std::string, std::string>(cell, content));
      spreadsheets.at(i).version++;
      return 1;      
    }

    }
    
  return 0;
  }

  std::pair<std::string, std::string> server::undo(session* session, std::string spreadsheetname, std::string version)
  {
    //loop through the spreadsheets and find the one to work on
    for(int i = 0; i < spreadsheets.size(); i++)
    {
      if(spreadsheets.at(i).name == spreadsheetname) //the right ss, and an undo to do
      {
        if(spreadsheets.at(i).undoStack.size() == 0)
        {
          return std::pair<std::string, std::string>("end", "end");
        }
        if(spreadsheets.at(i).version != atoi(version.c_str()))
        {
          return std::pair<std::string, std::string>("wait", "wait");
        }
        else
        {
          std::pair<std::string, std::string> change = spreadsheets.at(i).undoStack.top();
          spreadsheets.at(i).undoStack.pop();

          //update it in the map
          std::map<std::string, std::string>::iterator it;
          for(it = spreadsheets.at(i).cells.begin(); it != spreadsheets.at(i).cells.end(); ++it)
          {
            if(it->first == change.first)
            {
              if(change.second == "")
              {
                spreadsheets.at(i).cells.erase(change.first);
                break;
              }
              else
              {
              it->second = change.second;
              }
            }
          }

          spreadsheets.at(i).version++;
          std::ostringstream length;
          length << change.second.size();
          sendChange(spreadsheetname, spreadsheets.at(i).version, change.first, change.second, length.str(), session);
          return change;
        }
      }
    }
    return std::pair<std::string, std::string>("fail", "fail");
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
