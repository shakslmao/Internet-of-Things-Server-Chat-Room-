# Worksheet 2 Part 2 README


## Worksheet Overview
- This worksheet focuses on the development of a simple chat application, leveraging the Internet of Things concepts. It includes building a server-side application while utilising a provided client side application for communication. The worksheet is designed to enhance understanding of socket programming, thread management, and the use of data structures.

## Task 1 Server Side
- **Objective**: Implement parts of the server to handle `JOIN`, `DIRECTMESSAGE` and `EXIT` commands.
- **Key Components**: 
    - Understanding of server loops and message handling
    - Usasge of maps for managing online users.
    - Implementation of specific handler functions for processing different types of chat messages.


## void_handle_join():
- **Function Signature**: 
```cppp
void handle_join(
online_users &online_users, std::string username,
    std::string, struct sockaddr_in &client_address,
    uwe::socket &sock, bool &exit_loop) {}
```
- **online_users &online_users**: A reference to the map that tracks the online status of clients, the key is is the username, and the value is a pointer to a `sockaddr_in` a structure representing the clients address.
`typedef std::map<std::string, sockaddr_in *> online_users`; 
- **std::string username**: The username of the client attempting to join the chat.
- **struct sockaddr_in &client_address**: A reference to a strucutre containing the clients network address.
- **uwe::socket &sock**: A reference to the socket object used for network communications.
- **bool &exit_loop**: A reference to a boolean variable that controls the servers main loop, allowing the server to exit gracefully.

**Debug Message**
```cpp
DEBUG("Received join\n");
```
- Logs a message indicating a join request has been recieved.

**Checks for Existing User**
```cpp
if (online_users.find(username) != online_users.end())
{
    handle_error(ERR_USER_ALREADY_ONLINE, client_address, sock, exit_loop);
    return;
}
// chat.hpp: #define ERR_USER_ALREADY_ONLINE 0
```
- Checks if the username already exists in the online_users map. If found, it sends an error response to the client and returns early. online_users.find(username): This line attempts to find the username in the map collection named that holds the usernames of all currently online users.
- `if (online_users.find(username) != online_users.end())`: This condition checks if the iterator returned by `online_users.find(username)` is not equal to the end iterator of the `online_users` container. If its not equal, it means the username was found in the container, indicating the user is already marked as online.
- `handle_error(ERR_USER_ALREADY_ONLINE, client_address, sock, exit_loop)`: If the user is already online, this line is executed to handle the error situation. The `handle_error` function is called with several arguments:
`ERR_USER_ALREADY_ONLINE`: A preprocessor macro defined with the value 0, used as an error code indicating the specific error (user already online).

**Validate Username**
```cpp
    if (username.empty() || username.length() > MAX_USERNAME_LENGTH)
    {
        DEBUG("Invalid username encountered: %s\n", username.c_str());
        handle_error(ERR_UNKNOWN_USERNAME, client_address, sock, exit_loop);
        return; 
    }
    // chat.hpp: #define MAX_USERNAME_LENGTH 64
    // chat.hpp: #define ERR_UNKNOWN_USERNAME 1
```
- Checks if the username is either empty or exceeds the maximum allowed length. If either condition is met, it indicates an invalid username, triggering an error response to the client and an early return from the function.
- `if (username.empty() || username.length() > MAX_USERNAME_LENGTH):` This line evaluates two conditions:
    - `username.empty(): `Checks if the username string is empty, which is an invalid state for a username.
    - `username.length() > MAX_USERNAME_LENGTH:` Compares the length of the username to a predefined maximum length (MAX_USERNAME_LENGTH, defined as 64). This condition checks if the username length exceeds the maximum allowed limit.
- If either of these conditions are true, it implies that the username is invalid for joining the server.
- `DEBUG`: This debug statement logs the invalid username.
- `handle_error(ERR_UNKNOWN_USERNAME, client_address, sock, exit_loop)`: Upon encountering an invalid username, this line executes to handle the error situation. The `handle_error` function is invoked with these arguments:
    - `ERR_UNKNOWN_USERNAME`: An error code indicating the specific error condition (unknown or invalid username). A preprocessor macro defined with the value 1, used to define that the given username is not known.
- `return`: This statement causes an early return from the function if an invalid username is encountered, preventing any further action or processing for this particular client request.

**Register a new User**
```cpp
sockaddr_in *client_addr = new sockaddr_in(client_address);
online_users[username] = client_addr;
```
- `sockaddr_in *client_addr = new sockaddr_in(client_address);`: This line creates a new instance of `sockaddr_in`, which is a structure used to store the internet address. The `new` keyword dynamically allocates memory for this structure and initialises it with the value of `client_address`, which contains the address information of the new users connection. The result is a pointer to this dynamically allocated address structure, stored in `client_addr`.
- `online_users[username] = client_addr`: This line adds the new user to a collection that tracks which users are currently online and their associated network address information. The key for this collection is the user `username`, and the value is the pointer to the `sockaddr_in` structure. This effectively registers the user as online and allows the system to keep track of the users network address.

**Send Join Acknnowledgment (JACK)**
```cpp
    auto jack_message = chat::jack_msg();
    ssize_t sent_bytes = sock.sendto(reinterpret_cast<const char *>(&jack_message), 
                sizeof(jack_message), 0,(sockaddr *)&client_address, sizeof(client_address));

    if (sent_bytes != sizeof(jack_message))
    {
        DEBUG("Failed to send JACK message to new user: %s\n", username.c_str());
        delete client_addr;           
        online_users.erase(username);
    }
```
- **Creating the JACK Message**:
    - `auto jack_message = chat::jack_msg()`: This line creates an instance of a join acknnowledgment message, known as `jack_message`. This message is created by calling the `jack_msg()` method on the `chat` namespace.

- **Sending the JACK Message**:
    - ` ssize_t send_bytes`, This line attempts to send the `jack_message` to the new users network address. The `sendto` function is a socket operation that sends data to a specified network address.
        - `reinterpret_cast<const char *>(&jack_message)`: This casts the address of the jack_message to a pointer to `const char` as `sendto` expects a raw byte buffer.
        - `sizeof(jack_message)`:  The size of the jack_message, indicating how many bytes to send.
        - `0`: Flags parameter, set to 0 indicating no special behaviour.
        - `(sockaddr *)&client_address`: A cast of the clients address to `sockaddr*`, specifying the destination address.
        - `sizeof(client_address)`: The size of the clients address structure.
        - The `sendto` function returns the number of bytes sent. This value is stored in `sent_bytes`.

**Error Handling**
- The `if` condition checks if the number of bytes sent does not equal the size of the `jack_message`. This indicates that the message was not sent successfully.
- `DEBUG`: If an error occurs, a debug message is logged indicating the failure to send the JACK message to the new user, with the username specified for clarity.
`delete client_addr;`: If sending the message fails, this line deallocates the memory allocated for the users address information, cleaning up to prevent memory leaks.
- `online_users.erase(username);`: Additionally, the username is removed from the `online_users` collection, reversing the registration since the join acknowledgment failed. This ensures the systems state remains consistent and does not consider the user as successfully joined.

**Broadcast the New User Join**:
```cpp
        chat::chat_message broadcast_msg = chat::broadcast_msg("Server", username + " has joined the chat.");
        for (const auto &[user, addr] : online_users)
        {
            if (user != username) 
            {
                sent_bytes = sock.sendto(reinterpret_cast<const char *>(&broadcast_msg), sizeof(broadcast_msg),
                                 0, (sockaddr *)addr, sizeof(struct sockaddr_in));
                if (sent_bytes != sizeof(broadcast_msg))
                {
                    DEBUG("Failed to send broadcast message to user %s\n", user.c_str());
                }
            }
        }
```
- **Creating the Broadcast Message**
    - `chat::chat_message broadcast_msg = chat::broadcast_msg()`: A message is created using `broadcast_msg` function, this function takes two arguments, a sender "Sever" and the messasge content. The result is stored in  a `broadcast_msg`.

- **Iterating Over Online Users**:
    - The `for` loop iterates over each entry in the `online_users` collection, which maps usernames to their network address. This holds the current state of users who are considered online.

- **Sending the Message to Each User**:
    - Within the loop, theres a check to ensure that the new user (the one who just joined) does not receive their own join notification: `if (user != username).`
    - `reinterpret_cast<const char *>(&broadcast_msg)`: Converts the address of `broadcast_msg` to a const char*, as required by the `sendto` function.
    - `sizeof(broadcast_msg)`: The size of the message to be sent.
    - `0`: Flags parameter, here set to `0` for default behaviour.
    - `(sockaddr *)addr`: The recipients address cast to a `sockaddr*`. This is the address to which the message will be sent.
    - `sizeof(struct sockaddr_in)`: The size of the recipients address structure.

**Error Handling**
- After attempting to sedn the message, the code checks if the number of bytes sent matches the expected size, if the message size does not match, this indactes an error in sending the message.

**Sending a Private Welcome Message to New User**
```cpp
 auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);

        std::stringstream time_stream;
        time_stream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

        std::string welcome_msg = "Welcome to the chat, " + username + "! It is now " + time_stream.str();
        chat::chat_message priv_welcome_msg = chat::dm_msg("Server", welcome_msg);
        sent_bytes = sock.sendto(reinterpret_cast<const char *>(&priv_welcome_msg), sizeof(priv_welcome_msg), 0, (sockaddr *)&client_address, sizeof(client_address));
        if (sent_bytes != sizeof(priv_welcome_msg))
        {
            DEBUG("Failed to send private welcome message to new user\n");
        }

        handle_list(online_users, "__ALL", "", client_address, sock, exit_loop);
```

**Getting Current Time**
- `auto now = std::chrono::system_clock::now()`: Captures the current time point using the system clock.
- `auto now_c = std::chrono::system_clock::to_time_t(now)`: Converts the time point to a `time_t` type, which represents the time in seconds since the Unix epoch.

**Formatting the Time**
-  `std::stringstream` named `time_stream` is used to format the current time into a readable string.
-  `time_stream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S")`: Inserts the formatted current time into `time_stream`. The format specified by %Y-%m-%d %H:%M:%S corresponds to "Year-Month-Day Hour:Minute:Second".

**Creating and Sending the Welcome Message**:
- Welcome message constructed by concatenating a greeting with the username and the formatted current time.
- `chat::chat_message priv_welcome_msg = chat::dm_msg("Server", welcome_msg)`: Creates a direct message (dm_msg) from the "Server" to the new user, containing the welcome message.
- The message is sent using `sock.sendto`, similar to the previously described broadcast process. This function sends the `priv_welcome_msg` to the `client_address`, which is the network address of the new user.
![alt text](/images/screenshotjoin.png)


## void_handle_directmessage():
**Function Signature**:
```cpp
void handle_directmessage(
    online_users &online_users, std::string username, std::string message,
    struct sockaddr_in &client_address, uwe::socket &sock, bool &exit_loop);
```
- `online_users &online_users`: A reference to a map tracking online clients. Keys are usernames, and values are pointers to `sockaddr_in` structures with clients network addresses.
- `std::string username`: The senders username.
- `std::string message`: The raw message content, expected to include a recipient username, followed by a colon, and then the actual message.
- `struct sockaddr_in &client_address`: A reference to the senders network address structure.
- `uwe::socket &sock`: A reference to the socket object used for network communication.
- `bool &exit_loop`: A reference to a boolean variable controlling the servers main event loop, not directly used in this function.


**Extract the Recipients Username and Message**
```cpp
std::size_t colon_pos = message.find(':');
    if (colon_pos == std::string::npos)
    {
        DEBUG("Invalid DM format, missing colon. Received: %s\n", message.c_str());
        return; // Invalid format, could log or handle error here
    }

    std::string recipient_username = message.substr(0, colon_pos);
    std::string actual_message = message.substr(colon_pos + 1);
    DEBUG("Parsed DM: Recipient: %s, Message: %s\n", recipient_username.c_str(), actual_message.c_str());
```

- The message format expected is "recipient_username:message_content". The code needs to identify the recipient and the actual message to proceed correctly.

**Finding the Colon Position**
- `std::size_t colon_pos = message.find(':')`: This line searches for the position of the first colon `:` in the `message` string. The `find` method returns the position of the first occurrence of the colon, if this is not found, it returns `std::string::npos`, which indicates no matches were found.

**Validating the Message Format**
- The `if` statetemnt checks if the `colon_pos` equals `std::string::npos`, which would mean no colon was found in the message, indicating an invalid dm format.
- If the formot is invalid, a debug messasge is logged using the `DEBUG` macro to indicate the issue, inclduding the recieved message.

**Extracting the Recipients Username and Message**
- if the colon is found, which indicates a potentially valid format, the code proceeds to extract the recipients username and the message content.
- `std::string recipient_username = message.substr(0, colon_pos);`: This line extracts the substring from the start of the message up to (but not including) the colon position.
- `std::string actual_message = message.substr(colon_pos + 1);`: This line extracts the substring starting just after the colon to the end of the message string. 

**Validate the Sender**
```cpp
   auto sender_it = online_users.find(username);
    if (sender_it == online_users.end() || sender_it->second->sin_addr.s_addr != client_address.sin_addr.s_addr)
    {
        DEBUG("Sender %s not found\n", username.c_str());
        return;
    }
```
**Finding the Sender in `online_users`**:
- `auto sender_it = online_users.find(username);`: This line attempts to find the username in the `online_users` map, which keeps track of users currently online. The find method returns an iterator to the element if the key (username) is found, otherwise, it returns an iterator to the end of the map (`online_users.end()`).

**Verifying the Senders Address**
- The `if` statement checks two conditions to verify the sender:
    - `sender_it == online_users.end()`: This condiiton checks if the iterator points to the end of the map, which would indicate that the `username` was not found amongst online users. If this was true, it means the sender is not recognised as currently online.
    - `sender_it->second->sin_addr.s_addr != client_address.sin_addr.s_addr`: This condition checks if the senders IP address recorded in the map does not match the iP address from the `client_address` of the current message. The `sender_it->second` part derefernces the iterator to access the value, and then `sin_addr.s_addr` is compared between the stored address and the `client_address`.

**Handling a Non-Verified Sender**
- `DEBUG`: If the sender cannot be verified (not found in the map, or IP mismatch) a debug message is logged indicating that the sender was not found.

**Send the DM**
```cpp
auto recipient_it = online_users.find(recipient_username);
    if (recipient_it != online_users.end())
    {
        DEBUG("Sending DM to %s\n", recipient_username.c_str());

        chat::chat_message dm_msg = chat::dm_msg(username, actual_message);
        ssize_t sent_bytes = sock.sendto(reinterpret_cast<const char *>(&dm_msg), sizeof(dm_msg), 
                        0, (sockaddr *)recipient_it->second, sizeof(struct sockaddr_in));

        if (sent_bytes != sizeof(dm_msg))
        {
            DEBUG("Failed to send DM to %s\n", recipient_username.c_str());
        }
    }
    else
    {
        DEBUG("Recipient %s not found\n", recipient_username.c_str());
        chat::chat_message err_msg = chat::error_msg(ERR_UNKNOWN_USERNAME);
        sock.sendto(reinterpret_cast<const char *>(&err_msg), sizeof(err_msg), 0, 
                            (sockaddr *)&client_address, sizeof(client_address));
    }
```
- **Finding the Recipient in `online_users`**
    - `auto recipient_it = online_users.find(recipient_username);`: Searches for the `recipient_username` in the `online_users` map.
    - The result of the search is an iterator, recipient_it, which points to the found entry or to the end of the map if the username is not found.

- **Handling if the Recipient is Found**
    - If the iterator does not equal `online_users.end()`, it indicates that the recipient is found in the map.
    - `DEBUG`: Logs a debug message indicating an attempt to send a DM to the recipient.
    - The DM is created by calling `chat::dm_msg(username, actual_message)`, whihc takes the senders username and the message content to create a `chat_message` object.
    - The message is sent using `sock.sendto` which sends the message to the network address associated with `recipient_it->second`. The `sendto` function parameters are designed to convert the message to a raw byte format and specify the recipienss address and message size.

- **Handling if the Recipient is Not Found**:
    - If the recipient is not found (`recipient_it == online_users.end()`), it logs a debug message indicating the recipient is not found.
    - An error message is then prepared using `chat::error_msg(ERR_UNKNOWN_USERNAME)`.
![alt text](/images/screenshotdm.png)


## void handle_exit():
```cpp
    chat::chat_message exit_message = chat::exit_msg();
    for (const auto &[user, addr] : online_users)
    {
        ssize_t send_bytes = sock.sendto(reinterpret_cast<const char *>(&exit_message), sizeof(exit_message),
                        0,(sockaddr *)addr, sizeof(struct sockaddr_in));
        if (send_bytes != sizeof(exit_message))
        {
            DEBUG("Failed to send exit message to user %s\n", user.c_str());
        }
    }
```
**Creating the Exit Message**:
    - `chat::chat_message exit_message = chat::exit_msg()`: This line invokes the `exit_msg` funciton, which is part of the chat namespace, the function creates a chat_message objected configured as an exit message.
```cpp
  inline chat_message exit_msg()
    {
        return chat_message{EXIT, '\0', '\0'};
    }
```

**Iterating Over Online Users**:
    - The `for` loop iterates through each entry in the `online_users` collection. Each entry consists of a users username  and their network address information (addr). 

**Sending the Exit Messasge to Each User**
- Inside the loop, the `sendto` function is used to send the exit message to the address associated with each user:
    - `reinterpret_cast<const char *>(&exit_message)`: This casts the address of the `exit_message` object to a const char*, which is necessary because `sendto` requires a byte buffer as its input.
    - `sizeof(exit_message)`: Specifies the size of the exit message to be sent.
    - `0`: The flags parameter, set to 0 for no special behavior.
    - `(sockaddr *)addr`: Casts the users address to a `sockaddr*`, which `sendto` requires to specify the destination address.
    - `sizeof(struct sockaddr_in):` Provides the size of the address structure, ensuring the correct amount of address data is used by sendto.

**Memory Cleanup for Online Users**
```cpp
 for (const auto &user : online_users)
    {
        delete user.second;
    }
    online_users.clear();
    exit_loop = true;
```
- The for loop iterates over each entry in the `online_users` map. Each entry contains a pair where the first element is the users identifier and the second element is a pointer to dynamically allocated memory.
- `delete user.second;`: For each user, this line deallocates the memory pointed to by the second element of the pair. This is necessary to prevent memory leaks, ensuring that all dynamically allocated resources are properly released when they're no longer needed.
- `online_users.clear()`: After deallocating the memory for each user, this line clears the `online_users` container, removing all its elements. This step ensures that the container is left in a clean state, with no dangling pointers.


## Task 2 Client Side
- **Objective**: Implement and enhance the clients ability to interact with a chat server, handle user inputs, display messages and manage the state of the chat session effectively.
- **Key Components**: 
    - Network and Communication, establishing and managing UDP socket connection with the server.
    - Send and recieve messages using the socket, including handling different types of chat commands.
    - Implement functions to create and send various types of messages to the server, such as join, leave, direct message, create group, add to group, etc.
    - Implement a receiver thread that listens for incoming messages from the server without blocking the main thread or UI.

**Message Reception**
```cpp
ssize_t recv_len = sock->recvfrom(reinterpret_cast<char *>(&msg), sizeof(chat::chat_message), 0, nullptr, nullptr);
```
- `sock->recvfrom`: Calls the recvfrom method on the socket object to receive data from the server. This method blocks the thread until a message arrives if theres no data currently available.
- `reinterpret_cast<char *>(&msg):` Casts the address of msg to a `char*` pointer, as recvfrom expects a buffer of bytes to store the incoming data.
- `sizeof(chat::chat_message)`: Specifies the size of the `chat::chat_message `structure, indicating how many bytes should be read and stored into msg
- The function returns the length of the received message in bytes and stores this value in `recv_len`. If `recv_len` is greater than 0, it indicates that data has been successfully received.

**Sending Message to Main UI thread**
```cpp
if (recv_len > 0)
{
    tx.send(msg);
}
```
- After successfully receiving a message, the received` chat::chat_message` object `msg` is sent to the main UI thread using the `tx` channel.
- `tx.send(msg)`: Sends the `msg` object through the channel to what is listening on the other end of this channel.


**Case `chat::EXIT`**
```cpp
case chat::EXIT:
{
    DEBUG("Received Exit from GUI\n");
    chat::chat_message exit_msg = chat::exit_msg();
    sock.sendto(reinterpret_cast<const char *>(&exit_msg), sizeof(chat::chat_message),
            0, (sockaddr *)&server_address, sizeof(server_address));
    exit_loop = true;
    break;
}
```
- **Functionality**: This case is triggered when the user decides to exit the chat app.
- **Operation**:
    - Logs the reception of an `EXIT` command from the GUI for debugging.
    - Constructs an `EXIT` message `using chat::exit_msg()` to notify the server of the clients intention to disconnect.
    - sends the `EXIT` message to the server.
    - Sets `exit_loop` to true, indicating the main event loop should terminate, closing the client application.

**Case `chat::LEAVE`**
```cpp
case chat::LEAVE:
{
    DEBUG("Received LEAVE from GUI\n");
    chat::chat_message leave_msg = chat::leave_msg();
    sock.sendto(reinterpret_cast<const char *>(&leave_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
    sent_leave = true;
    break;
}
```
- **Functionality**:Handles the scenario where the user wants to leave the chat but not necessarily close the application.
- **Operation**:
    - Logs the reception of a `LEAVE` command.
    - Constructs a `LEAVE` message using `chat::leave_msg().`
    - Sends the `LEAVE` message to the server, informing it that the user wishes to disconnect from the chat session but not exit the application.
    - Sets `sent_leave` to true, a flag indicating that a leave message has been sent.

**Case `chat::LIST`:
```cpp
case chat::LIST:
{
    DEBUG("Received LIST from GUI\n");
    chat::chat_message list_msg = chat::list_msg();
    sock.sendto(reinterpret_cast<const char *>(&list_msg), sizeof(chat::chat_message), 0, (sockaddr *)&server_address, sizeof(server_address));
    break;
}
```
- **Functionality**: Requests the current list of online users from the server.
- **Operation**:
    - Logs the action of requesting the online user list.
    - Constructs a `LIST` message using `chat::list_msg()`
    - Sends this message to the server, which should respond with a list of currently online users.

**Default Case**
```cpp
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
```
- **Functionality**: Makes the input to be a DM to another users
- **Operation**:
    - Logs the reception of a message intended for another user.

**Direct Message Handling**
```cpp
if (cmds.size() == 2 && type == chat::UNKNOWN)
{
    std::string recipient = cmds[0];
    std::string content = cmds[1];
    std::string dm_message = recipient + ":" + content;
    chat::chat_message dm_msg = chat::dm_msg(username, dm_message);
    sock.sendto(reinterpret_cast<const char *>(&dm_msg), sizeof(chat::chat_message), 
            0, (sockaddr *)&server_address, sizeof(server_address));
    DEBUG("DM sent to %s\n", recipient.c_str());
}
```
- **Command Parsing**: The condition `if (cmds.size() == 2 && type == chat::UNKNOWN)` checks if the parsed command consists of two parts and if the command type is `UNKNOWN`. In this context, `UNKNOWN`  means that the command doesnt match any predefined actions (such as JOIN, LEAVE), allowing for a direct message where the first part is the recipients username and the second part is the message content.

- **Recipient and Content Extraction**
    - `std::string recipient = cmds[0];` retrieves the recipients username from the first part of the parsed command.
    - `std::string content = cmds[1];` gets the actual message content intended for the recipient from the second part.

- **Message Construction**:
    - `std::string dm_message = recipient + ":" + content;` constructs the direct message string by appending the recipients username and the message content, separated by a colon.
    - `chat::chat_message dm_msg = chat::dm_msg(username, dm_message)`; constructs a `chat::chat_message` object for the direct message. The function `chat::dm_msg` is used to create this message object.

- **Sending the Direct Message**
    - The message is sent to the server using `sock.sendto`, which sends the constructed `dm_msg` object to the servers address.
    - The `sendto` function arguments ensure that the message is correctly formatted as a byte array and sent to the correct server address and port.


## Task 3 Group Messaging (This was a headache...)








