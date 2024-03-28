#include <map>
// IOT socket api
#include <iot/socket.hpp>

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <chat.hpp>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#define USER_ALL "__ALL"
#define USER_END "END"

// ./chat_client "192.168.1.10" 1000 s2-akram
// ./chat_client "127.0.0.1" 1000 s2-akram

// ./chat_client "127.0.0.1" 1020 user1
// ./chat_client "192.168.1.10" 1020 user1
// /opt/iot/bin/packets_clear

// CHAT_SERVER

/**
 * @brief map of current online clients
 */
typedef std::map<std::string, sockaddr_in *>
    online_users;

void handle_list(
    online_users &online_users, std::string username, std::string,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop);

/**
 * @brief Send a given message to all clients
 *
 * @param msg to send
 * @param username used if  not to send to that particular user
 * @param online_users current online users
 * @param sock socket for communicting with client
 * @param send_to_username determines also to send to username
 */
void send_all(
    chat::chat_message &msg, std::string username, online_users &online_users,
    uwe::socket &sock, bool send_to_username = true)
{
    for (const auto user : online_users)
    {
        if ((send_to_username && user.first.compare(username) == 0) || user.first.compare(username) != 0)
        {
            int len = sock.sendto(
                reinterpret_cast<const char *>(&msg), sizeof(chat::chat_message), 0,
                (sockaddr *)user.second, sizeof(struct sockaddr_in));
        }
    }
}

/**
 * @brief handle sending an error and incoming error messages
 *
 * Note: there should not be any incoming errors messages!
 *
 * @param err code for error
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */
void handle_error(uint16_t err, struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    auto msg = chat::error_msg(err);
    int len = sock.sendto(
        reinterpret_cast<const char *>(&msg), sizeof(chat::chat_message), 0,
        (sockaddr *)&client_address, sizeof(struct sockaddr_in));
}

/**
 * @brief handle broadcast message
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */
void handle_broadcast(
    online_users &online_users, std::string username, std::string msg,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received broadcast\n");

    // send message to all users, except the one we received it from
    for (const auto user : online_users)
    {
        DEBUG("username %s\n", user.first.c_str());
        if (strcmp(inet_ntoa(client_address.sin_addr), inet_ntoa(user.second->sin_addr)) == 0 &&
            client_address.sin_port != user.second->sin_port)
        {
            // send BC
            auto m = chat::broadcast_msg(username, msg);
            int len = sock.sendto(
                reinterpret_cast<const char *>(&m), sizeof(chat::chat_message), 0,
                (sockaddr *)user.second, sizeof(struct sockaddr_in));
        }
        else
        {
            DEBUG("Not sending message to self: %s\n", msg.c_str());
        }
    }
}

/**
 * @brief handle join messageß
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */

///////////////////////////////////// WORKSHEET IMPLEMENTATION  //////////////////////////////////////////////////////
void handle_join(
    online_users &online_users, std::string username, std::string,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received join\n");

    // first check user not already online
    if (auto search = online_users.find(username); search != online_users.end())
    {
        handle_error(ERR_USER_ALREADY_ONLINE, client_address, sock, exit_loop);
    }
    else
    {
        // Allocate new sockaddr_in structure and copy the client address into it
        sockaddr_in *new_user_address = new sockaddr_in;
        memcpy(new_user_address, &client_address, sizeof(client_address));

        // Check if the username is valid
        if (username.empty() || username.length() > MAX_USERNAME_LENGTH)
        {
            // Since no specific error code for invalid username, using ERR_UNKNOWN_USERNAME
            DEBUG("Invalid username encountered: %s\n", username.c_str());
            handle_error(ERR_UNKNOWN_USERNAME, client_address, sock, exit_loop);
            return;
        }
        // Add the new user to the map
        online_users[username] = new_user_address;

        // Send back a JACK message to the client that has joined
        auto jack_message = chat::jack_msg();
        ssize_t sent_bytes = sock.sendto(reinterpret_cast<const char *>(&jack_message), sizeof(jack_message), 0, (sockaddr *)&client_address, sizeof(client_address));

        // Check if the JACK message was sent successfully
        if (sent_bytes != sizeof(jack_message))
        {
            DEBUG("Failed to send JACK message to new user\n");
            delete new_user_address;      // Free the memory allocated for the new user's address
            online_users.erase(username); // Remove the new user from the map

            return;
        }

        // Send a broadcast message to all other clients about the new join
        chat::chat_message broadcast_msg = chat::broadcast_msg("Server", username + " has joined the chat.");
        for (const auto &[user, addr] : online_users)
        {
            if (user != username) // Avoid sending the message to the user who just joined
            {
                sent_bytes = sock.sendto(reinterpret_cast<const char *>(&broadcast_msg), sizeof(broadcast_msg), 0, (sockaddr *)addr, sizeof(struct sockaddr_in));
                if (sent_bytes != sizeof(broadcast_msg))
                {
                    DEBUG("Failed to send broadcast message to user %s\n", user.c_str());
                }
            }
        }

        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);

        std::stringstream time_stream;
        time_stream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

        // Send a private welcome message to the new user
        std::string welcome_msg = "Welcome to the chat, " + username + "! It is now " + time_stream.str();
        chat::chat_message priv_welcome_msg = chat::dm_msg("Server", welcome_msg);
        sent_bytes = sock.sendto(reinterpret_cast<const char *>(&priv_welcome_msg), sizeof(priv_welcome_msg), 0, (sockaddr *)&client_address, sizeof(client_address));
        if (sent_bytes != sizeof(priv_welcome_msg))
        {
            DEBUG("Failed to send private welcome message to new user\n");
        }

        // Optionally, send a list of all currently online users to the new user
        handle_list(online_users, "__ALL", "", client_address, sock, exit_loop);
    }
}

/**
 * @brief handle jack message
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */
void handle_jack(
    online_users &online_users, std::string username, std::string,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received jack\n");
    handle_error(ERR_UNEXPECTED_MSG, client_address, sock, exit_loop);
}

/**
 * @brief handle direct message
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */

///////////////////////////////////// WORKSHEET IMPLEMENTATION //////////////////////////////////////////////////////

// remember you need to translate the client IO address with inet_ntoa,
// and also the user you are comparing

// check if user matches the incoming client address, using strcmp, and
// and if so send message with chat::dm_msg()

void handle_directmessage(
    online_users &online_users, std::string username, std::string message,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received directmessage to %s\n", username.c_str());
    DEBUG("Raw Message Recieved for DM: %s\n", message.c_str());

    // Extract the recipient username and actual message
    std::size_t colon_pos = message.find(':');
    if (colon_pos == std::string::npos)
    {
        DEBUG("Invalid DM format, missing colon. Received: %s\n", message.c_str());
        return; // Invalid format, could log or handle error here
    }

    std::string recipient_username = message.substr(0, colon_pos);
    std::string actual_message = message.substr(colon_pos + 1);
    DEBUG("Parsed DM: Recipient: %s, Message: %s\n", recipient_username.c_str(), actual_message.c_str());

    // Find the sender in the online_users map
    auto sender_it = online_users.find(username);
    if (sender_it == online_users.end() || sender_it->second->sin_addr.s_addr != client_address.sin_addr.s_addr)
    {
        DEBUG("Sender %s not found\n", username.c_str());
        return;
    }

    DEBUG("Parsed DM: To %s, Message %s\n", recipient_username.c_str(), actual_message.c_str());

    // Find the recipient in the online_users map
    auto recipient_it = online_users.find(recipient_username);
    if (recipient_it != online_users.end())
    {
        DEBUG("Sending DM to %s\n", recipient_username.c_str());

        // Create DM message
        chat::chat_message dm_msg = chat::dm_msg(username, actual_message);

        // Send DM to the recipient
        ssize_t sent_bytes = sock.sendto(reinterpret_cast<const char *>(&dm_msg), sizeof(dm_msg), 0, (sockaddr *)recipient_it->second, sizeof(struct sockaddr_in));
        // Check if the message was sent successfully
        if (sent_bytes != sizeof(dm_msg))
        {
            DEBUG("Failed to send DM to %s\n", recipient_username.c_str());
            // Optionally, handle the error, such as notifying the sender of the failure
        }
    }
    else
    {
        DEBUG("Recipient %s not found\n", recipient_username.c_str());
        // Here, you might want to send an error message back to the sender
        // indicating that the recipient username does not exist or is not online.
        chat::chat_message err_msg = chat::error_msg(ERR_UNKNOWN_USERNAME);
        sock.sendto(reinterpret_cast<const char *>(&err_msg), sizeof(err_msg), 0, (sockaddr *)&client_address, sizeof(client_address));
    }
}
/**
 * @brief handle list message
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */
void handle_list(
    online_users &online_users, std::string username, std::string,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received list\n");

    int username_size = MAX_USERNAME_LENGTH;
    int message_size = MAX_MESSAGE_LENGTH;

    char username_data[MAX_USERNAME_LENGTH] = {'\0'};
    char *username_ptr = &username_data[0];
    char message_data[MAX_MESSAGE_LENGTH] = {'\0'};
    char *message_ptr = &message_data[0];

    bool using_username = true;
    bool full = false;

    for (const auto user : online_users)
    {
        if (using_username)
        {
            if (username_size - (user.first.length() + 1) >= 0)
            {
                memcpy(username_ptr, user.first.c_str(), user.first.length());
                *(username_ptr + user.first.length()) = ':';
                username_ptr = username_ptr + user.first.length() + 1;
                username_size = username_size - (user.first.length() + 1);
                username_data[MAX_USERNAME_LENGTH - username_size] = '\0';
            }
            else
            {
                using_username = false;
            }
        }

        // otherwise we fill the message field
        if (!using_username)
        {
            if (message_size - (user.first.length() + 1) >= 0)
            {
                memcpy(message_ptr, user.first.c_str(), user.first.length());
                *(message_ptr + user.first.length()) = ':';
                message_ptr = message_ptr + user.first.length() + 1;
                message_size = message_size - (user.first.length() + 1);
            }
            else
            {
                // we are full and we need to send packet and start again
                chat::chat_message msg{chat::LIST, '\0', '\0'};
                username_data[MAX_USERNAME_LENGTH - username_size] = '\0';
                memcpy(msg.username_, &username_data[0], MAX_USERNAME_LENGTH - username_size);
                message_data[MAX_MESSAGE_LENGTH - message_size] = '\0';
                memcpy(msg.message_, &message_data[0], MAX_MESSAGE_LENGTH - message_size);

                //
                if (username.compare("__ALL") == 0)
                {
                    send_all(msg, "__ALL", online_users, sock);
                }
                else
                {
                    int len = sock.sendto(
                        reinterpret_cast<const char *>(&msg), sizeof(chat::chat_message), 0,
                        (sockaddr *)&client_address, sizeof(struct sockaddr_in));
                }

                username_size = MAX_USERNAME_LENGTH;
                message_size = MAX_MESSAGE_LENGTH;

                username_ptr = &username_data[0];
                message_ptr = &message_data[0];

                using_username = false;
            }
        }
    }

    if (using_username)
    {
        if (username_size >= 4)
        {
            // enough space to store end in username
            memcpy(&username_data[MAX_USERNAME_LENGTH - username_size], USER_END, strlen(USER_END));
            username_size = username_size - (strlen(USER_END) + 1);
        }
        else
        {
            username_size = username_size + 1; // this enables overwriting the last ':'
            using_username = false;
        }
    }

    if (!using_username)
    {
    }

    chat::chat_message msg{chat::LIST, '\0', '\0'};
    username_data[MAX_USERNAME_LENGTH - username_size] = '\0';
    DEBUG("username_data = %s\n", username_data);
    memcpy(msg.username_, &username_data[0], MAX_USERNAME_LENGTH - username_size);
    message_data[MAX_MESSAGE_LENGTH - message_size] = '\0';
    memcpy(msg.message_, &message_data[0], MAX_MESSAGE_LENGTH - message_size);

    if (username.compare("__ALL") == 0)
    {
        send_all(msg, "__ALL", online_users, sock);
    }
    else
    {
        int len = sock.sendto(
            reinterpret_cast<const char *>(&msg), sizeof(chat::chat_message), 0,
            (sockaddr *)&client_address, sizeof(struct sockaddr_in));
    }
}

/**
 * @brief handle leave message
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */
void handle_leave(
    online_users &online_users, std::string username, std::string,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received leave\n");

    username = "";
    // find username
    for (const auto user : online_users)
    {
        if (strcmp(inet_ntoa(client_address.sin_addr), inet_ntoa(user.second->sin_addr)) == 0 &&
            client_address.sin_port == user.second->sin_port)
        {
            username = user.first;
        }
    }
    DEBUG("%s is leaving the sever\n", username.c_str());

    if (username.length() == 0)
    {
        // this should never happen
        handle_error(ERR_UNKNOWN_USERNAME, client_address, sock, exit_loop);
    }
    else if (auto search = online_users.find(username); search != online_users.end())
    {
        // first free memory for sockaddr
        struct sockaddr_in *addr = search->second;
        delete addr;

        // now delete from username map
        online_users.erase(search);

        // finally send back LACK
        auto msg = chat::lack_msg();
        int len = sock.sendto(
            reinterpret_cast<const char *>(&msg), sizeof(chat::chat_message), 0,
            (sockaddr *)&client_address, sizeof(struct sockaddr_in));

        // handle_broadcast(online_users, username, "has left the server", client_address, sock, exit_loop);
        msg = chat::chat_message{chat::LEAVE, '\0', '\0'};
        memcpy(msg.username_, username.c_str(), username.length() + 1);
        send_all(msg, username, online_users, sock, false);
    }
    else
    {
        handle_error(ERR_UNKNOWN_USERNAME, client_address, sock, exit_loop);
    }
}

/**
 * @brief handle lack message
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */
void handle_lack(
    online_users &online_users, std::string username, std::string,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received lack\n");
    handle_error(ERR_UNEXPECTED_MSG, client_address, sock, exit_loop);
}

/**
 * @brief handle exit message
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */

///////////////////////////////////// WORKSHEET IMPLEMENTATION //////////////////////////////////////////////////////
void handle_exit(
    online_users &online_users, std::string username, std::string,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received exit\n");

    // Create exit message
    chat::chat_message exit_message = chat::exit_msg();

    // Send exit message to each user
    for (const auto &[user, addr] : online_users)
    {
        ssize_t send_bytes = sock.sendto(reinterpret_cast<const char *>(&exit_message), sizeof(exit_message), 0, (sockaddr *)addr, sizeof(struct sockaddr_in));
        if (send_bytes != sizeof(exit_message))
        {
            DEBUG("Failed to send exit message to user %s\n", user.c_str());
        }
    }

    // Clear up memory for each user
    for (const auto &user : online_users)
    {
        delete user.second;
    }
    online_users.clear();

    // Leave this code as it is required for exiting
    exit_loop = true;
}

/**
 * @brief
 *
 * @param online_users map of usernames to their corresponding IP:PORT address
 * @param username part of chat protocol packet
 * @param msg part of chat protocol packet
 * @param client_address address of client to send message to
 * @param sock socket for communicting with client
 * @parm exit_loop set to true if event loop is to terminate
 */
void handle_error(
    online_users &online_users, std::string username, std::string,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop)
{
    DEBUG("Received error\n");
}

/**
 * @brief function table, mapping command type to handler.
 */
void (*handle_messages[9])(online_users &, std::string, std::string, struct sockaddr_in &, uwe::socket &, bool &exit_loop) = {
    handle_join, handle_jack, handle_broadcast, handle_directmessage,
    handle_list, handle_leave, handle_lack, handle_exit, handle_error};

/**
 * @brief server for chat protocol
 */
void server()
{
    // keep track of online users
    online_users online_users;

    // port to start the server on

    // socket address used for the server
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;

    // htons: host to network short: transforms a value in host byte
    // ordering format to a short value in network byte ordering format
    server_address.sin_port = htons(SERVER_PORT);

    // htons: host to network long: same as htons but to long
    // server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    // creates binary representation of server name and stores it as sin_addr
    inet_pton(AF_INET, uwe::get_ipaddr().c_str(), &server_address.sin_addr);

    // create a UDP socket
    uwe::socket sock{AF_INET, SOCK_DGRAM, 0};

    sock.bind((struct sockaddr *)&server_address, sizeof(server_address));

    // socket address used to store client address
    struct sockaddr_in client_address;
    size_t client_address_len = 0;

    char buffer[sizeof(chat::chat_message)];
    DEBUG("Entering server loop\n");
    bool exit_loop = false;
    for (; !exit_loop;)
    {
        int len = sock.recvfrom(
            buffer, sizeof(buffer), 0, (struct sockaddr *)&client_address, &client_address_len);

        // DEBUG("Received message:\n");
        if (len == sizeof(chat::chat_message))
        {
            // handle incoming packet
            chat::chat_message *message = reinterpret_cast<chat::chat_message *>(buffer);
            auto type = static_cast<chat::chat_type>(message->type_);
            std::string username{(const char *)&message->username_[0]};
            std::string msg{(const char *)&message->message_[0]};

            if (is_valid_type(type))
            {
                DEBUG("handling msg type %d\n", type);
                // valid type, so dispatch message handler
                handle_messages[type](online_users, username, msg, client_address, sock, exit_loop);
            }
        }
        else
        {
            DEBUG("Unexpected packet length\n");
        }
    }
}

/**
 * @brief entry point for chat server application
 */
int main(void)
{
    // Set server IP address
    // uwe::set_ipaddr("192.168.1.8");
    uwe::set_ipaddr("127.0.0.1");

    server();

    return 0;
}