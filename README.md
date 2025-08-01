## Client Application Implementation

This project is a C++ implementation of a web client that interacts with a RESTful API using HTTP methods such as `GET`, `POST`, and `DELETE`. It simulates a command-line client capable of registering users, logging in, managing sessions with cookies and JWT tokens, and performing CRUD operations on a book database provided by the server.

The client features:
- Persistent interactive CLI for user input and command parsing
- HTTP request construction and socket communication with the server
- Session management via cookies (for login/logout)
- Authorization with JWT tokens for protected endpoints
- JSON request/response handling using the [`nlohmann/json`](https://github.com/nlohmann/json) library
- Clear separation of concerns via modular helper functions

To implement the client program that interacts with the server, I used part of the skeleton from Lab 10. I also followed the conceptual structure provided in the lab for solving the assignment.

Unlike the lab, where an interactive console was not required and the connection to the server was established only once, here the connection is closed after each command is executed and reopened upon receiving a new command.

In the `client.cpp` file, the `parser()` function is implemented, which keeps the interactive console open, allowing the user to input commands for the client program.

I used a custom `enum string_code` type to encode all possible user commands. Combined with the `hashString` function, each command is mapped to its corresponding enum value, allowing me to use a `switch-case` structure for better code clarity.

The implementation of each command follows a common structure (with minor adjustments as needed):

1. A function is created to construct the GET/POST/DELETE request message to be sent to the server.
2. A function is called from within the parser that sends the message to the server, receives the response (success or error), extracts any important data from the response (e.g., cookies, tokens), and then calls another function.
3. This third function parses the response and displays a suitable message to `stdout`. It checks the first line of the server’s response for a success code and displays the appropriate success message. If not successful, it checks the last line for an error message in the format `error:...` and displays the error accordingly.

Thanks to the very user-friendly functions provided by the `nlohmann` library, I used it to parse server responses containing book data into JSON objects for cleaner display. I also used JSON objects to encapsulate information sent to the server, such as the username and password for `register` and `login`.

All helper functions used to implement the commands described in the spec are located in the `helpers.cpp` file.

### Implemented Commands

1. **register**:  
   A POST request message is created as specified, with the username and password included as a JSON object. The message is sent to the server, and a success, error, or timeout message is displayed.

2. **login**:  
   A POST request similar to the one in `register` is created and sent to the server. The user credentials are stored in a JSON object.  
   On success, the session cookie returned by the server is saved for future authenticated actions.  
   If a user is already logged in, the command returns an error. Logging out is required before another user can log in.

3. **enter_library**:  
   A GET request is created using the session cookie received during login.  
   On success, the JWT token is saved for future access to library resources.

4. **get_books**:  
   A GET request is created using the JWT token obtained from `enter_library`.  
   On success, the list of books received from the server is parsed into JSON objects (one per book) and displayed.

5. **get_book**:  
   A GET request is created using the access token and the ID of the book whose information is requested.  
   On success, the response is parsed into a JSON object. The book ID is manually added since the server does not return it. The data is displayed.  
   Note: Keys in the JSON object are displayed in alphabetical order due to how the `nlohmann` library renders JSON.

6. **add_book**:  
   A POST request is created with the access token and a JSON object containing the book's information.  
   On success, a confirmation message is shown to inform the user that the book has been added.

7. **delete_book**:  
   A DELETE request is created using the access token.  
   The title of the deleted book is stored to create a proper success message once the server confirms the deletion.

8. **logout**:  
   A GET request is created using the session cookie to verify the user is logged in.  
   On logout, both the session cookie and JWT token are deleted so that any operations requiring authentication become inaccessible until another user logs in again.

9. **exit**:  
   The program checks whether a user is currently logged in by checking for the session cookie.  
   - If a user is logged in: they are logged out and the program exits.  
   - If no user is logged in: the program exits immediately.
