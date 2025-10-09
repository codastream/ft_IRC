#include "Client.hpp"
#include "Config.hpp"
#include "LogManager.hpp"
#include "ReplyHandler.hpp"
#include "Server.hpp"
#include "reply_codes.hpp"
#include "utils.hpp"

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>

/************************************************************
 *		🥚 CONSTRUCTORS & DESTRUCTOR						*
 ************************************************************/

ReplyHandler::ReplyHandler(Server* server) : _server(server) {}

/*************************************************************
 *		🛠️ FUNCTIONS											*
 *************************************************************/

/*
<message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
<prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
<command>  ::= <letter> { <letter> } | <number> <number> <number>
<SPACE>    ::= ' ' { ' ' }
<params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]

<middle>   ::= <Any *non-empty* sequence of octets not including SPACE
               or NUL or CR or LF, the first of which may not be ':'>
<trailing> ::= <Any, possibly *empty*, sequence of octets not including
                 NUL or CR or LF>

<crlf>     ::= CR LF

  1)    <SPACE> is consists only of SPACE character(s) (0x20).
        Specially notice that TABULATION, and all other control
        characters are considered NON-WHITE-SPACE.

  2)    After extracting the parameter list, all parameters are equal,
        whether matched by <middle> or <trailing>. <Trailing> is just
        a syntactic trick to allow SPACE within parameter.

  3)    The fact that CR and LF cannot appear in parameter strings is
        just artifact of the message framing. This might change later.

  4)    The NUL character is not special in message framing, and
        basically could end up inside a parameter, but as it would
        cause extra complexities in normal C string handling. Therefore
        NUL is not allowed within messages.

  5)    The last parameter may be an empty string.

  6)    Use of the extended prefix (['!' <user> ] ['@' <host> ]) must
        not be used in server to server communications and is only
        intended for server to client messages in order to provide
        clients with more useful information about who a message is
        from without the need for additional queries.
*/

/**
 * @brief [TODO:return the id sequence <nick|parameter>!<user>@<host>]
 *
 * @param client [TODO:Client]
 * @param nickname [TODO:if not given, id will be contruct with client.get_nickname()]
 * @return [TODO: string <nick>!<user>@<host>]
 */
std::string ReplyHandler::get_id_of(Client& client, const std::string& nickname)
{
    std::string identity(":");

    if (nickname.empty()) {
        identity += client.get_nickname();
    } else {
        identity += nickname;
    }
    identity += "!" + client.get_user_name() + "@" + ircConfig.get_name();

    return (identity);
}

std::string ReplyHandler::select_response(Client& client, ReplyCode code, const std::string& parameters, Client* sender)
{
    std::string nick = client.get_nickname();
    std::string response(":" + ircConfig.get_name() + " ");
    std::string responseWithCode        = response + utils::code_to_str(code) + " ";
    std::string responseWithCodeAndNick = responseWithCode + nick + " ";
    LOG_D_CMD("resp w code and nick", responseWithCodeAndNick);
    if (!sender)
        sender = &client;
    switch (code) {
    case RPL_WELCOME:
        return (response + utils::code_to_str(code) + nick + RPL_WELCOME_MSG);
    case RPL_PING:
        return response + parameters; // PONG :token
    case RPL_NICK:
        return (get_id_of(client, parameters) + " NICK " + nick);
    case RPL_JOIN:
        return (get_id_of(*sender, "") + " JOIN :" + parameters);
    case RPL_NOTICE:
        return (response + " NOTICE " + nick + " :" + parameters);
    case RPL_PRIVMSG:
        return (get_id_of(*sender) + " PRIVMSG " + parameters);
    case RPL_KICK:
        return (response + sender->get_full_userhost() + " KICK " + parameters);
    case RPL_QUIT:
        return sender->get_nickname() + " has quit " + ircConfig.get_name() + parameters;
    case RPL_INVITING:
        return (response + utils::code_to_str(code) + nick + " " + parameters);
    case RPL_INVITING_TARGET:
        return (get_id_of(*sender, "") + parameters);
    case RPL_WHOREPLY:
        return (response + utils::code_to_str(code) + parameters);
    case RPL_ENDOFWHO:
        return (response + utils::code_to_str(code) + nick + " " + parameters + RPL_ENDOFWHO_MSG);
    case RPL_MODE:
        return (response + " MODE " + parameters);
    case RPL_TOPIC:
        return (response + utils::code_to_str(code) + nick + " " + parameters);
    case RPL_NAMREPLY:
        return (response + utils::code_to_str(code) + nick + " = " + parameters);
    case RPL_ENDOFNAMES:
        return (response + utils::code_to_str(code) + nick + " " + parameters + RPL_ENDOFNAMES_MSG);
    case RPL_NOTOPIC:
        return (response + utils::code_to_str(code) + nick + " " + parameters + RPL_NOTOPIC_MSG);
    case RPL_CHANNELMODEIS:
        return (responseWithCodeAndNick + parameters + " " + ircCodes.trailing(code));
    case RPL_BOT:
        return (parameters);
    case ERR_CHANOPRIVSNEEDED:
        return (responseWithCodeAndNick + parameters + ERR_CHANOPRIVSNEEDED_MSG);
    case ERR_UNKNOWNCOMMAND:
        return (std::string("421") + ERR_UNKNOWNCOMMAND_MSG + parameters);
    case ERR_NEEDMOREPARAMS:
        return (responseWithCodeAndNick + parameters + ERR_NEEDMOREPARAMS_MSG);
    case ERR_USERNOTINCHANNEL:
        return (responseWithCodeAndNick + parameters + ERR_USERNOTINCHANNEL_MSG);
    case ERR_USERONCHANNEL:
        return (responseWithCodeAndNick + parameters + ERR_USERONCHANNEL_MSG);
    case ERR_NOSUCHCHANNEL:
        return (responseWithCodeAndNick + parameters + ERR_NOSUCHCHANNEL_MSG);
    case ERR_NONICKNAMEGIVEN:
        return (std::string("431") + ERR_NONICKNAMEGIVEN_MSG);
    case ERR_TOOMANYTARGETS:
        return (responseWithCodeAndNick + parameters + " :too many recipients (you dont have that much friends)");
    case ERR_NOSUCHNICK:
        return (responseWithCodeAndNick + parameters + " :No such nickname (imaginary friend issue)");
    case ERR_NOTEXTTOSEND:
        return (responseWithCodeAndNick + ERR_NOTEXTTOSEND_MSG);
    case ERR_ERRONEUSNICKNAME:
        return (std::string("432") + ERR_ERRONEUSNICKNAME_MSG);
    case ERR_NICKNAMEINUSE:
        return (responseWithCodeAndNick + ERR_NICKNAMEINUSE_MSG);
    case ERR_PASSWDMISMATCH:
        return (std::string("464") + ERR_PASSWDMISMATCH_MSG);
    case ERR_NOTREGISTERED:
        return (TO_STRING(ERR_NOTREGISTERED) + "* " + parameters + " :You have not registered");
    case ERR_ALREADYREGISTRED:
    case ERR_BADCHANMASK:
    case ERR_BADCHANNELKEY:
    case ERR_BANNEDFROMCHAN:
    case ERR_CHANNELISFULL:
    case ERR_INVITEONLYCHAN:
    case ERR_KEYSET:
    case ERR_NOORIGIN:
    case ERR_NOTONCHANNEL:
    case ERR_UNKNOWNMODE:
    case ERR_WRONG_FORMAT:
        return (responseWithCodeAndNick + parameters + ircCodes.trailing(code));    
    default:
        return ("");
    }
}

int ReplyHandler::process_response(Client& client, ReplyCode code, const std::string& parameters, Client* sender)
{
    std::string response = select_response(client, code, parameters, sender);

    if (!response.empty()) {
        LOG_CMD.sending(__FILE_NAME__, __FUNCTION__, response, &client);
        _send_reply(client, response);
    }
    return (code);
}

/**
 * @brief send RFC_2812 formmated message to the client
 *
 * @param client who waiting response from the server
 * @param code of response
 * @param parameters message corresponding of the code
 * @return
 */
int ReplyHandler::process_code_response(Client& client, ReplyCode code, const std::string& parameters)
{
    std::string response(":" + ircConfig.get_name());
    response = response + utils::code_to_str(code) + " " + parameters;

    if (!response.empty()) {
        LOG_CMD.info("ReplyHandler::process_response --> response to " + client.get_nickname() + ":\n" + response);
        _send_reply(client, response);
    }
    return (code);
}

void ReplyHandler::_send_reply(Client& client, const std::string& msg)
{
    client.append_to_send_buffer(msg + "\r\n");
    _server->add_events_of(client, POLLOUT);
}

/*************************************************************
 *		👁️‍ GETTERS and SETTERS				 				*
 *************************************************************/

ReplyHandler& ReplyHandler::get_instance(Server* s)
{
    static ReplyHandler instance(s);
    return instance;
}
