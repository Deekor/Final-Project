void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
  {
    std::string mess = "";
    if (!error)
    {
		std::cout << "Received " << data_ << std::endl;
		
		//The Following if statements are for message handling.
		//CREATE MESSAGE
		if (data_[1] == 'R')
		{
			mess = "Create has been called";
			sendMessage(mess, mess.size());
			//boost::asio::async_write(socket_, boost::asio::buffer(data_, bytes_transferred), boost::bind(&session::sendCallback, this, boost::asio::placeholders::error));
		}
		//JOIN MESSAGE
		else if (data_[1] == 'O')
		{
			mess = "Join has been called";
			sendMessage(mess, mess.size());
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
			mess = "Leave has been called";
			sendMessage(mess, mess.size());
		}
		else{}
		
      //boost::asio::async_write(socket_, boost::asio::buffer(data_, bytes_transferred), boost::bind(&session::handle_write, this, boost::asio::placeholders::error));
    }
    else
    {
      delete this;
    }
  }
