#pragma once

#include <stdint.h>

#include <string>
#include <cstring>
#include <arpa/inet.h>

#define MAX_USERNAME_LENGTH 64
#define MAX_MESSAGE_LENGTH 1024
#define MAX_GROUPNAME_LENGTH 64 // define max group name length

// Server always run on this port
#define SERVER_PORT 8867

namespace chat
{

    /**
     * @brief Chat protocol command types
     * @var chat_type::JOIN
     * Client join server message
     * @var chat_type::JACK
     * Client ACK in reply to JOIN
     * @var chat_type::BROADCAST
     * Client sends message to all online users
     * @var chat_type::DIRECTMESSAGE
     * Client sends message to particlar user
     * @var chat_type::LIST
     * Client request list of current online users
     * Server sends list of current online users (might be multiple of these terminated with user END)
     * @var chat_type::LEAVE
     * Client requests to leave
     * Server sents to all online users that particular user has left
     * @var chat_type::LACK
     * Server sends in response to LEAVE
     * @var chat_type::EXIT
     * Client sends message to terminate server and all online clients
     * Server sends to all online users informing them to terminate
     * @var chat_type::ERROR
     * Server sends to client if an error has occured
     *
     */
    enum chat_type
    {
        JOIN = 0,
        JACK,
        BROADCAST,
        DIRECTMESSAGE,
        CREATE_GROUP,      // Implemented
        ADD_TO_GROUP,      // Implemented
        GROUP_MESSAGE,     // Implemented
        REMOVE_FROM_GROUP, // CBA.
        LIST,
        LEAVE,
        LACK,
        EXIT,
        ERROR,
        UNKNOWN,
    };

    /** @brief check if type is indeed a valid chat_type.
     * @param type the command type to check
     * @return true if a valid type, otherwise false
     */
    inline bool is_valid_type(chat_type type)
    {
        // return type >= JOIN && type <= ERROR;
        return type >= JOIN && type <= ERROR;
    }

    /**
     * @struct chat_message
     * @brief Representation of chat protocol message
     * @var chat_message::type_
     *  Member 'type_' contains the chat command
     * @var chat_message::username_
     *  Member 'username_' the messages associated username
     * @var chat_message::message_
     *  Member 'message_' the message body
     */
    struct chat_message
    {
        uint8_t type_;
        int8_t username_[MAX_USERNAME_LENGTH];
        int8_t groupname_[MAX_GROUPNAME_LENGTH];
        int8_t message_[MAX_MESSAGE_LENGTH];
    };

    /**
     * @brief Creates a chat_message structure for the purpose of creating a new group, initialising it with the provided group name and username.
     *        The function ensures that the group name and username fit within predefined maximum lengths, and explicitly null-terminates these strings
     *        within the chat_message structure. This is important to prevent buffer overflow and ensure string safety. The message field of the chat_message
     *        is left empty, as it's not needed for group creation messages. This function is typically used to prepare a message that will be sent
     *        to inform the system or server about the intention to create a new chat group.
     *
     * @param group_name The name of the group to be created. It is truncated if it exceeds MAX_GROUPNAME_LENGTH - 1 characters to leave space for a null terminator.
     * @param username The username of the user creating the group. It is truncated if it exceeds MAX_USERNAME_LENGTH - 1 characters to leave space for a null terminator.
     * @return A chat_message structure populated with the type set to CREATE_GROUP, and the group name and username fields filled with the provided values, appropriately truncated and null-terminated. The message field is left empty.
     */
    inline chat_message create_group(std::string group_name, std::string username)
    {
        chat_message msg;
        msg.type_ = CREATE_GROUP;

        // copied string does not exceed the buffer size, leaving space for null terminator
        size_t group_name_length = std::min(group_name.length(), static_cast<size_t>(MAX_GROUPNAME_LENGTH - 1));
        memcpy(&msg.groupname_[0], group_name.c_str(), group_name.length());
        msg.groupname_[group_name.length()] = '\0'; // NULL terminate

        size_t username_length = std::min(username.length(), static_cast<size_t>(MAX_USERNAME_LENGTH - 1));
        memcpy(&msg.username_[0], username.c_str(), username_length);
        msg.username_[username_length] = '\0'; // NULL terminate
        msg.message_[0] = '\0';
        return msg;
    }

    /**
     * @brief Constructs a chat_message object configured to request the addition of a user to a specified group. The function populates the chat_message
     *        with the necessary group name and username, ensuring both are null-terminated and do not exceed their respective buffer sizes. This setup is essential
     *        for safely transmitting the request over a network or handling it within the system. The message field of the chat_message is left empty,
     *        as it is not required for this operation. The function is primarily used to generate a properly formatted message that signifies a user's addition to a group.
     *
     * @param group_name The name of the group to which the user is to be added. It is truncated to fit within the buffer allocated for the group name in the chat_message,
     *                   ensuring there is space for a null terminator to maintain string safety.
     * @param username The username of the user being added to the group. It is truncated to fit within the buffer allocated for the username in the chat_message,
     *                 ensuring there is space for a null terminator to maintain string safety.
     * @return A chat_message object filled with the group name and username, both truncated if necessary and null-terminated. The type of the message is set to ADD_TO_GROUP,
     *         indicating the action to be performed. The message part of the chat_message is explicitly null-terminated but otherwise left empty as it is not needed for this operation.
     */

    inline chat_message add_to_group(std::string group_name, std::string username)
    {
        chat_message msg = {};
        // Ensure we do not exceed buffer size - 1 to leave space for null terminator
        size_t group_name_len = std::min(group_name.length(), sizeof(msg.groupname_) - 1);
        size_t username_len = std::min(username.length(), sizeof(msg.username_) - 1);

        memcpy(msg.groupname_, group_name.c_str(), group_name_len);
        msg.groupname_[group_name_len] = '\0'; // Explicitly null-terminate
        memcpy(msg.username_, username.c_str(), username_len);
        msg.username_[username_len] = '\0'; // Explicitly null-terminate

        msg.type_ = ADD_TO_GROUP; // Set the message type
        msg.message_[0] = '\0';   // Explicitly null-terminate the message part

        return msg;
    }

    /**
     * @brief Constructs a chat_message object designed for sending a message within a group chat. This function prepares the message by setting the type to GROUP_MESSAGE
     *        and copying the provided group name, username, and message into the chat_message structure, ensuring each string is properly null-terminated and does not exceed
     *        its allocated buffer size. This is crucial for maintaining data integrity and preventing buffer overflows when the message is processed or transmitted.
     *        The function is used to facilitate communication within groups by formatting messages according to a predefined protocol.
     *
     * @param group_name The name of the group to which the message is to be sent. The function ensures this name does not exceed MAX_GROUPNAME_LENGTH - 1 characters to leave space for a null terminator, preventing buffer overflow.
     * @param username The username of the user sending the message. It is truncated to ensure it does not exceed MAX_USERNAME_LENGTH - 1 characters, leaving space for a null terminator, thus maintaining string safety.
     * @param message The content of the message to be sent to the group. It is truncated to ensure it does not exceed MAX_MESSAGE_LENGTH - 1 characters, leaving space for a null terminator, to maintain the integrity of the message.
     * @return A chat_message object populated with the group name, username, and message content, all appropriately truncated and null-terminated. The message type is set to GROUP_MESSAGE, indicating its purpose for group communication.
     */
    inline chat_message group_message(std::string group_name, std::string username, std::string message)
    {
        chat_message msg;
        msg.type_ = GROUP_MESSAGE;

        size_t group_name_len = std::min(group_name.length(), static_cast<size_t>(MAX_GROUPNAME_LENGTH - 1));
        std::memcpy(msg.groupname_, group_name.c_str(), group_name_len);
        msg.groupname_[group_name_len] = '\0'; // NULL terminate

        size_t username_len = std::min(username.length(), static_cast<size_t>(MAX_USERNAME_LENGTH - 1));
        std::memcpy(msg.username_, username.c_str(), username_len);
        msg.username_[username_len] = '\0'; // NULL terminate

        size_t message_len = std::min(message.length(), static_cast<size_t>(MAX_MESSAGE_LENGTH - 1));
        std::memcpy(msg.message_, message.c_str(), message_len);
        msg.message_[message_len] = '\0'; // NULL terminate

        return msg;
    }

    /**
     * @brief Create a JOIN message
     * @param username to be stored in the message
     * @return the chat message
     */
    inline chat_message
    join_msg(std::string username)
    {
        chat_message msg;
        msg.type_ = JOIN;
        memcpy(&msg.username_[0], username.c_str(), username.length());
        msg.username_[username.length()] = '\0';
        msg.message_[0] = '\0';
        return msg;
    }

    /**
     * @brief Create a JACK message

     * @return the chat message
    */
    inline chat_message jack_msg()
    {
        return chat_message{JACK, '\0', '\0'};
    }

    /**
     * @brief Create a BROADCAST message
     * @param username to be stored in the message
     * @param message to be stored in the message
     * @return the chat message
     */
    inline chat_message broadcast_msg(std::string username, std::string message)
    {
        chat_message msg{BROADCAST, '\0', '\0'};
        memcpy(&msg.username_[0], username.c_str(), username.length());
        msg.username_[username.length()] = '\0';
        memcpy(&msg.message_[0], message.c_str(), message.length());
        msg.message_[message.length()] = '\0';
        return msg;
    }

    /**
     * @brief Create a DIRECTMESSAGE message
     * @param username to be stored in the message
     * @param message to be stored in the message
     * @return the chat message
     */
    inline chat_message dm_msg(std::string username, std::string message)
    {
        chat_message msg{DIRECTMESSAGE, '\0', '\0'};
        memcpy(&msg.username_[0], username.c_str(), username.length());
        msg.username_[username.length()] = '\0';
        memcpy(&msg.message_[0], message.c_str(), message.length());
        msg.message_[message.length()] = '\0';
        return msg;
    }

    /**
     * @brief Create a LIST message
     * @param username to be stored in the message
     * @param message to be stored in the message
     * @return the chat message
     */
    inline chat_message list_msg(std::string username = "", std::string message = "")
    {
        chat_message msg{LIST, '\0', '\0'};
        memcpy(&msg.username_[0], username.c_str(), username.length());
        msg.username_[username.length()] = '\0';
        memcpy(&msg.message_[0], message.c_str(), message.length());
        msg.message_[message.length()] = '\0';
        return msg;
    }

    /**
     * @brief Create a LEAVE message
     * @return the chat message
     */
    inline chat_message leave_msg()
    {
        return chat_message{LEAVE, '\0', '\0'};
    }

    /**
     * @brief Create a LACK message
     * @return the chat message
     */
    inline chat_message lack_msg()
    {
        return chat_message{LACK, '\0', '\0'};
    }

    /**
     * @brief Create a EXIT message
     * @return the chat message
     */
    inline chat_message exit_msg()
    {
        return chat_message{EXIT, '\0', '\0'};
    }

    /**
     * @brief Create a ERROR message
     * @param err code
     * @return the chat message
     */
    inline chat_message error_msg(uint16_t err)
    {
        chat_message msg{ERROR, '\0'};
        *((int *)(&msg.message_[0])) = htons(err);
        return msg;
    }

    /**
     * @brief Print a chat message to stdout
     * @param message to be printed
     */
    void print_message(chat_message message);

#define ERR_USER_ALREADY_ONLINE 0
#define ERR_UNKNOWN_USERNAME 1
#define ERR_UNEXPECTED_MSG 2
#define ERR_GROUP_ALREADY_EXISTS 3
#define ERR_USER_ALREADY_IN_GROUP 4
#define ERR_GROUP_NOT_FOUND 5
#define ERR_USER_NOT_IN_GROUP 6

}; // namespace chat