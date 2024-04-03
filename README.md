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







