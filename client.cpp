#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"

using namespace std;
using json = nlohmann::json;

/**
 * Enum de coduri corespunzatoare comenzilor ce pot fi date de utilizator.
*/
enum string_code {
    REGISTER,
    LOGIN,
    ENTER_LIBRARY,
    GET_BOOKS,
    GET_BOOK,
    ADD_BOOK,
    DELETE_BOOK,
    LOGOUT,
    EXIT,
    UNREC
};

/**
 * Parseaza comanda data de utlizator si returneaza enum corespunzator.
 * @string = comanda de la stdin.
*/
string_code hashString(string const& string) {
    if (string == "register") 
        return REGISTER;
    if (string == "login") 
        return LOGIN;
    if (string == "enter_library")
        return ENTER_LIBRARY;
    if (string == "get_books")
        return GET_BOOKS;
    if (string == "get_book")
        return GET_BOOK;
    if (string == "add_book")
        return ADD_BOOK;
    if (string == "delete_book")
        return DELETE_BOOK;
    if (string == "logout")
        return LOGOUT;
    if (string == "exit")
        return EXIT;
    return UNREC; 
}

void parser() {
    bool quit = false;
    string sesionCookie;
    string jwt;
    json user;

    while (true) {
        string command;
        string id;
        int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
        DIE (sockfd < 0, "Socket open failed!");
        getline(cin, command);
        switch (hashString(command)) {
            case REGISTER:
                Register(sockfd);
                close_connection(sockfd);
                break;
            case LOGIN:
                if (sesionCookie != "") {
                    cout << "You are already logged in!" << endl << "\n";
                    close_connection(sockfd);
                    break;
                }
                LogIn(sockfd, sesionCookie, user);
                close_connection(sockfd);
                break;
            case ENTER_LIBRARY:
                EnterLibrary(sockfd, sesionCookie, jwt);
                close_connection(sockfd);
                break;
            case GET_BOOKS:
                GetBooks(sockfd, jwt);
                close_connection(sockfd);
                break;
            case GET_BOOK:
                GetBook(sockfd, jwt);
                close_connection(sockfd);
                break;
            case ADD_BOOK:
                AddBook(sockfd, jwt);
                close_connection(sockfd);
                break;
            case DELETE_BOOK:
                DeleteBook(sockfd, jwt);
                close_connection(sockfd);
                break;
            case LOGOUT:
                Logout(sockfd, sesionCookie, jwt, user);
                close_connection(sockfd);
                break;
            case EXIT:
                if (sesionCookie != "") {
                    Logout(sockfd, sesionCookie, jwt, user);
                }
                close_connection(sockfd);
                quit = true;
                break;
            default:
                close_connection(sockfd);
                cout << "Unrecognized command!" << endl;
                cout << "\n";
                break;
        }
        if (quit == true)
            break;
    }
}

int main() {
    parser();
    return 0;
}
