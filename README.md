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
- `if (online_users.find(username) != online_users.end())`: This condition checks if the iterator returned by `online_users.find(username)` is not equal to the end iterator of the `online_users` container. If it's not equal, it means the username was found in the container, indicating the user is already marked as online.












