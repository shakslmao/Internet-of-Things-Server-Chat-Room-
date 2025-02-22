
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>

#include <atomic>
#include <iostream>

// IOT socket api
#include <iot/socket.hpp>
// #include <chat.hpp>
#include "chat_new.hpp"
#include <gui.hpp>
#include <colors.hpp>
#include <util.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

// CHAT_CLIENT

namespace
{
    std::atomic<bool> sent_leave{false};
};

//---------------------------------------------------------------------------------------

/**
 * @brief Convert a string command from the UI into a chat command.
 *  NOTE: It is only a subset of all command types.
 *
 * @param cmd command to convert
 * @return command type ID representing the passed in command
 */
chat::chat_type to_type(std::string cmd)
{
    switch (string_to_int(cmd.c_str()))
    {
    // case string_to_int("join"): return chat::JOIN;
    // case string_to_int("bc"): return chat::BROADCAST;
    // case string_to_int("dm"): return chat::DIRECTMESSAGE;
    case string_to_int("list"):
        return chat::LIST;
    case string_to_int("leave"):
        return chat::LEAVE;
    case string_to_int("exit"):
        return chat::EXIT;
    case string_to_int("creategroup"): // to create a group
        return chat::CREATE_GROUP;
    case string_to_int("addtogroup"): // to add a user to a group
        return chat::ADD_TO_GROUP;
    default:
        return chat::UNKNOWN;
    }

    return chat::UNKNOWN; // unknowntype
}

//----------------------------------------------------------------------------------------

std::pair<std::thread, Channel<chat::chat_message>> make_receiver(uwe::socket *sock)
{
    auto [tx, rx] = make_channel<chat::chat_message>();

    std::thread receiver_thread{[](Channel<chat::chat_message> tx, uwe::socket *sock)
                                {
                                    try
                                    {
                                        for (;;)
                                        {
                                            chat::chat_message msg;
                                            //////////////////////////////////////// IMPLEMENTATION ////////////////////////////////////////////////////
                                            // you need to fill in
                                            // receive message from server
                                            // send it over channel (tx) to main UI thread
                                            ssize_t recv_len = sock->recvfrom(reinterpret_cast<char *>(&msg), sizeof(chat::chat_message), 0, nullptr, nullptr);
                                            if (recv_len > 0)
                                            {
                                                tx.send(msg);
                                            }
                                            // exit receiver thread
                                            if (msg.type_ == chat::EXIT || (msg.type_ == chat::LACK && sent_leave))
                                            {
                                                break;
                                            }
                                        }
                                    }
                                    catch (...)
                                    {
                                        DEBUG("caught exception\n");
                                    };
                                },
                                std::move(tx), sock};

    return {std::move(receiver_thread), std::move(rx)};
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("USAGE: %s <ipaddress> <port> <username>\n", argv[0]);
        exit(0);
    }

    std::string username{argv[3]};
    // Set client IP address
    uwe::set_ipaddr(argv[1]);

    // const char *server_name = "192.168.1.8";
    const char *server_name = "127.0.0.1";

    const int server_port = SERVER_PORT;

    sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;

    // creates binary representation of server name and stores it as sin_addr
    inet_pton(AF_INET, server_name, &server_address.sin_addr);

    // htons: port in network order format
    server_address.sin_port = htons(server_port);

    // open socket
    uwe::socket sock{AF_INET, SOCK_DGRAM, 0};

    // port for client
    const int client_port = std::atoi(argv[2]);

    // socket address used for the client
    struct sockaddr_in client_address;
    memset(&client_address, 0, sizeof(client_address));
    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(client_port);
    inet_pton(AF_INET, uwe::get_ipaddr().c_str(), &client_address.sin_addr);

    sock.bind((struct sockaddr *)&client_address, sizeof(client_address));

    chat::chat_message msg = chat::join_msg(username);

    // send data
    int len = sock.sendto(
        reinterpret_cast<const char *>(&msg), sizeof(chat::chat_message), 0,
        (sockaddr *)&server_address, sizeof(server_address));

    DEBUG("Join message (%s) sent, waiting for JACK\n", username.c_str());
    // wait for JACK
    sock.recvfrom(reinterpret_cast<char *>(&msg), sizeof(chat::chat_message), 0, nullptr, nullptr);

    if (msg.type_ == chat::JACK)
    {
        DEBUG("Received jack\n");

        // create GUI thread and communication channels
        auto [gui_thread, gui_tx, gui_rx] = chat::make_gui();
        auto [rec_thread, rec_rx] = make_receiver(&sock);

        // going to need recv thread for messages from server

        bool exit_loop = false;
        for (; !exit_loop;)
        {
            // check and see if any GUI messages to handle
            if (!gui_rx.empty() && !sent_leave)
            {
                auto result = gui_rx.recv();
                if (result)
                {
                    auto cmds = split(*result, ':');
                    if (cmds.size() > 1)
                    {
                        chat::chat_type type = to_type(cmds[0]);
                        //////////////////////////////////////// IMPLEMENTATION ////////////////////////////////////////////////////
                        switch (type)
                        {
                        case chat::CREATE_GROUP:
                        {
                            if (cmds.size() > 1)
                            {
                                std::string group_name = cmds[1];
                                chat::chat_message creategroup_msg = chat::create_group(group_name, username);
                                sock.sendto(reinterpret_cast<const char *>(&creategroup_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
                                DEBUG("Create group '%s' message sent\n", group_name.c_str());
                            }
                            else
                            {
                                DEBUG("Invalid creategroup command format\n");
                            }
                            break;
                        }

                        case chat::ADD_TO_GROUP:
                        {
                            if (cmds.size() > 2)
                            {
                                std::string group_name = cmds[1];
                                std::string user_to_add = cmds[2];

                                // Validate command input
                                if (!group_name.empty() && !user_to_add.empty())
                                {
                                    chat::chat_message addtogroup_msg = chat::add_to_group(group_name, user_to_add);
                                    ssize_t sent_bytes = sock.sendto(reinterpret_cast<const char *>(&addtogroup_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
                                    if (sent_bytes != sizeof(addtogroup_msg))
                                    {
                                        DEBUG("Error sending Add to Group message\n");
                                    }
                                    else
                                    {
                                        DEBUG("Add to Group message sent for user '%s' to group '%s'\n", user_to_add.c_str(), group_name.c_str());
                                    }
                                }
                                else
                                {
                                    DEBUG("Invalid Add to Group command format\n");
                                }
                            }
                            else
                            {
                                DEBUG("Invalid Add to Group command format\n");
                            }
                            break;
                        }
                        case chat::EXIT:
                        {
                            DEBUG("Received Exit from GUI\n");
                            // Send EXIT message to the server
                            chat::chat_message exit_msg = chat::exit_msg();
                            sock.sendto(reinterpret_cast<const char *>(&exit_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
                            exit_loop = true;
                            break;
                        }
                        case chat::LEAVE:
                        {
                            DEBUG("Received LEAVE from GUI\n");
                            // Send LEAVE message to the server
                            chat::chat_message leave_msg = chat::leave_msg();
                            sock.sendto(reinterpret_cast<const char *>(&leave_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
                            sent_leave = true;
                            break;
                        }
                        case chat::LIST:
                        {
                            DEBUG("Received LIST from GUI\n");
                            // you need to fill in
                            chat::chat_message list_msg = chat::list_msg();
                            sock.sendto(reinterpret_cast<const char *>(&list_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
                            break;
                        }
                        default:
                        {
                            // the default case is that the command is a username for DM
                            // <username> : message
                            if (cmds.size() == 2)
                            {
                                DEBUG("Received message from GUI\n");
                            }
                            break;
                        }
                        }
                        //////////////////////////////////////// IMPLEMENTATION ////////////////////////////////////////////////////
                        if (cmds.size() == 2 && type == chat::UNKNOWN)
                        {
                            std::string recipient = cmds[0];
                            std::string content = cmds[1];
                            std::string dm_message = recipient + ":" + content;
                            chat::chat_message dm_msg = chat::dm_msg(username, dm_message);
                            sock.sendto(reinterpret_cast<const char *>(&dm_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
                            DEBUG("DM sent to %s\n", recipient.c_str());
                        }
                        else if (cmds.size() >= 3 && cmds[0] == "groupmsg")
                        {
                            // Group message handling code
                            std::string group_name = cmds[1]; // Assume cmds[1] contains the group name
                            // Reconstruct the message content from the remaining parts of cmds
                            std::string message_content = cmds[2];
                            for (size_t i = 3; i < cmds.size(); ++i)
                            {
                                message_content += " " + cmds[i];
                            }
                            // Construct and send the group message
                            chat::chat_message group_msg = chat::group_message(group_name, username, message_content);
                            sock.sendto(reinterpret_cast<const char *>(&group_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
                            DEBUG("Group message sent to '%s'\n", group_name.c_str());
                        }
                        else
                        {
                            chat::chat_message bc_msg = chat::broadcast_msg(username, cmds[0]);
                            sock.sendto(reinterpret_cast<const char *>(&bc_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
                            DEBUG("Broadcast message sent\n");
                        }
                    }
                    else
                    {
                        // message to broadcast to everyone online
                        chat::chat_message msg = chat::broadcast_msg(username, *result);
                        // send data
                        int len = sock.sendto(
                            reinterpret_cast<const char *>(&msg), sizeof(chat::chat_message), 0,
                            (sockaddr *)&server_address, sizeof(server_address));
                    }
                }
            }
            // check to see if any messages received from the server
            if (!rec_rx.empty() && !exit_loop)
            {
                auto result = rec_rx.recv();
                if (result)
                {
                    switch ((*result).type_)
                    {
                    case chat::LEAVE:
                    {
                        chat::display_command cmd{chat::GUI_USER_REMOVE};
                        cmd.text_ = std::string{(char *)(*result).username_};
                        gui_tx.send(cmd);
                        break;
                    }
                    case chat::EXIT:
                    {
                        DEBUG("Received EXIT\n");
                        exit_loop = true;
                        break;
                    }
                    case chat::LACK:
                    {
                        DEBUG("Received LACK\n");
                        if (sent_leave)
                        {
                            exit_loop = true;
                            break;
                        }
                    }
                    case chat::BROADCAST:
                    {
                        std::string msg{(char *)(*result).username_};
                        msg.append(": ");
                        msg.append((char *)(*result).message_);
                        chat::display_command cmd{chat::GUI_CONSOLE, msg};
                        gui_tx.send(cmd);
                        break;
                    }
                    case chat::DIRECTMESSAGE:
                    {
                        std::string msg{"dm("};
                        msg.append((char *)(*result).username_);
                        msg.append("): ");
                        msg.append((char *)(*result).message_);
                        chat::display_command cmd{chat::GUI_CONSOLE, msg};
                        gui_tx.send(cmd);
                        break;
                    }

                    case chat::GROUP_MESSAGE:
                    {
                        std::string msg = "group(";
                        msg += std::string((char *)(*result).groupname_); // Append group name
                        msg += ") ";
                        msg += std::string((char *)(*result).username_); // Append username of the sender
                        msg += ": ";
                        msg += std::string((char *)(*result).message_); // Append the message content

                        chat::display_command cmd{chat::GUI_CONSOLE, msg};
                        gui_tx.send(cmd);
                        break;
                    }

                    case chat::LIST:
                    {
                        bool end = false;
                        auto users = split(std::string{(char *)(*result).username_}, ':');
                        for (auto u : users)
                        {
                            if (u.compare("END") == 0)
                            {
                                end = true;
                                break;
                            }
                            chat::display_command cmd{chat::GUI_USER_ADD, u};
                            gui_tx.send(cmd);
                        }

                        if (!end)
                        {
                            auto users = split(std::string{(char *)(*result).message_}, ':');
                            for (auto u : users)
                            {
                                if (u.compare("END") == 0)
                                {
                                    break;
                                }
                            }
                        }

                        break;
                    }
                    case chat::ERROR:
                    {
                        break;
                    }
                    default:
                    {
                    }
                    }
                }
            }
        }

        DEBUG("Exited loop\n");
        // send message to GUI to exit
        chat::display_command cmd{chat::GUI_EXIT};
        gui_tx.send(cmd);
        gui_thread.join();
        rec_thread.join();

        // so done...
        DEBUG("Time to rest\n");
    }
    else
    {
        DEBUG("Received invalid jack\n");
    }

    return 0;
}