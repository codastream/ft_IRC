#ifndef PASS_HPP
#define PASS_HPP

#include "Channel.hpp"
#include "ICommand.hpp"
#include "reply_codes.hpp"

#include <string>

class Server;
class Client;

class Pass : public ICommand
{
  public:
    Pass(void);
    Pass(std::string& params);
    Pass(const Pass& other);
    Pass& operator=(const Pass& other);
    ~Pass();

    void             execute(Server& s, Client& c);
    static ReplyCode check_args(Server& s, Client& c, std::string& params);

  private:
    std::string _pass;
};

#endif
