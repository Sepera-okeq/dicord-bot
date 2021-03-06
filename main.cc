#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>

#include <discordpp/bot.hh>
#include <discordpp/rest-beast.hh>
#include <discordpp/websocket-beast.hh>

namespace asio = boost::asio;
using json = nlohmann::json;
namespace dpp = discordpp;
using DppBot = dpp::WebsocketBeast<dpp::RestBeast<dpp::Bot> >;

std::string readTokenFile(const std::string &tokenFilePath);

int main() {
    std::cout << "Starting bot...\n\n";

    /*/
     * Read token from token file.
     * Tokens are required to communicate with Discord, and hardcoding tokens is a bad idea.
     * If your bot is open source, make sure it's ignore by git in your .gitignore file.
    /*/
    std::string token;
    if (boost::filesystem::exists("token.dat")) {
        token = readTokenFile("token.dat");
    } else {
        std::cerr
                << "CRITICAL: There is no valid way for Discord++ to obtain a token! Copy the example login.dat or token.dat to make one.\n";
        exit(1);
    }

    // Create Bot object
    DppBot bot;

    /*/
     * Create handler for the READY payload, this may be handled by the bot in the future.
     * The `self` object contains all information about the 'bot' user.
    /*/
    json self;
    bot.handlers.insert(
            {
                    "READY",
                    [&bot, &self](json data) {
                        self = data["user"];
                    }
            }
    );

    // Create handler for the MESSAGE_CREATE payload, this recieves all messages sent that the bot can see.
    bot.handlers.insert(
            {
                    "MESSAGE_CREATE",
                    [&bot, &self](json msg) {
                        // Scan through mentions in the message for self
                        bool mentioned = false;
                        for(const json &mention : msg["mentions"]){
                            mentioned = mentioned or mention["id"] == self["id"];
                        }
                        if(mentioned){
                            // Identify and remove mentions of self from the message
                            std::string mentioncode = "<@" + self["id"].get<std::string>() + ">";
                            std::string content = msg["content"];
                            while(content.find(mentioncode + ' ') != std::string::npos) {
                                content = content.substr(0, content.find(mentioncode + ' ')) + content.substr(content.find(mentioncode + ' ') + (mentioncode + ' ').size());
                            }
                            while(content.find(mentioncode) != std::string::npos) {
                                content = content.substr(0, content.find(mentioncode)) + content.substr(content.find(mentioncode) + mentioncode.size());
                            }

                            // Set status to Playing "with [author]"
                            bot.call(
                                    "POST",
                                    "/channels/" + msg["channel_id"].get<std::string>() + "/messages",
                                    {{"content", content}}
                            );

                            // Echo the created message
                            bot.send(3, {
                                    {"game", {
                                                     {"name", "with " + msg["author"]["username"].get<std::string>()},
                                                     {"type", 0}
                                             }},
                                    {"status", "online"},
                                    {"afk", false},
                                    {"since", "null"}
                            });
                        }
                    }
            }
    );

    // These handlers silence the GUILD_CREATE, PRESENCE_UPDATE, and TYPING_START payloads, as they're some that you see a lot.
    bot.handlers.insert({"GUILD_CREATE",   [](json){}}); // Ignoring
    bot.handlers.insert({"PRESENCE_UPDATE",[](json){}}); // Ignoring
    bot.handlers.insert({"TYPING_START",   [](json){}}); // Ignoring

    // Create Asio context, this handles async stuff.
    auto aioc = std::make_shared<asio::io_context>();

    // Set the bot up
    bot.initBot(6, token, aioc);

    // Run the bot!
    bot.run();

    return 0;
}

std::string readTokenFile(const std::string &tokenFilePath) {
    std::ifstream tokenFile;
    tokenFile.open(tokenFilePath);

    std::string token;

    if (tokenFile.is_open()) {
        std::getline(tokenFile, token);
    } else {
        std::cerr << "CRITICAL: There is no such file as " + tokenFilePath +
                     "! Copy the example login.dat to make one.\n";
        exit(1);
    }
    tokenFile.close();
    std::cout << "Retrieved token.\n\n";

    return token;
}