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
- **Function Params**: 
```cppp
void handle_join(
online_users &online_users, std::string username,
    std::string, struct sockaddr_in &client_address,
    uwe::socket &sock, bool &exit_loop) {}
```
- **online_users &online_users**: A reference to the map that tracks the online status of clients, the key is is the username, and the value is a pointer to a `sockaddr_in` a structure representing the clients address.
typedef std::map<std::string, sockaddr_in *> online_users; 




