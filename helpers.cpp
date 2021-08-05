#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE (sizeof(HEADER_TERMINATOR) - 1)
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE (sizeof(CONTENT_LENGTH) - 1)

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void compute_message(char *message, const char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
}

int open_connection(const char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

void close_connection(int sockfd)
{
    close(sockfd);
}

void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);

    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}

char *receive_from_server(int sockfd)
{
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0){
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
        
        header_end = buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0) {
            header_end += HEADER_TERMINATOR_SIZE;
            
            int content_length_start = buffer_find_insensitive(&buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);
            
            if (content_length_start < 0) {
                continue;           
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t) header_end;
    
    while (buffer.size < total) {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

char *basic_extract_json_response(char *str)
{
    return strstr(str, "{\"");
}

/**
 * Parseaza raspunsul primit in urma unei cereri de inregistrare utilizator.
 * In cazul in care inregistrarea s-a efectuat cu succes, se afiseaza un
 * mesaj corespunzator. In caz contrar, se afiseaza mesajul de eroare
 * primit de la server.
 * @response = mesajul primit de la server.
*/
void parseRegisterResponse(char* response) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    while(getline(tok, token, '\n')) {              // imparte mesajul in tokens
        tokens.push_back(token);
    }
    if (tokens[0].find("201") != string::npos) {    // codul 201 = succes
        cout << "Registration success!" << endl;
        cout << "\n";
    } else {                                        // eroare
        if (tokens[tokens.size() - 1].find(ERROR_MSG) != string::npos) {
            string s = tokens[tokens.size() - 1].erase(0, strlen(ERROR_MSG) + 1);
            s.erase(prev(s.end()));
            s.erase(prev(s.end()));
            cout << s << endl;
            cout << "\n";
        } else {                                    // timeout
            cout << tokens[tokens.size() - 1] << endl;
            cout << "\n";
        }
    }
}

/**
 * Parseaza raspunsul primit in urma unei cereri de autentificare.
 * In cazul in care autentificarea s-a efectuat cu succes, se afiseaza un
 * mesaj corespunzator de intampinare. 
 * In caz contrar, se afiseaza mesajul de eroare primit de la server.
 * Returneaza un boolean true daca autentificarea s-a efectuat cu succes
 * pentru a se sti daca se poate extrage sau nu cookieul de sesiune.
 * @response = mesajul primit de la server;
 * @user = datele utilizatorului care tocmai s-a logat.
 * @return autentificare succes/fail.
*/
bool parseLoginResponse(char* response, json user) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    bool ok = false;
    while(getline(tok, token, '\n')) {
        tokens.push_back(token);                    // imparte mesajul in tokens
    }
    if (tokens[0].find("200") != string::npos) {    // codul 200 = succes
        cout << "Login success! Welcome " << user["username"] << "!" << endl;
        cout << "\n";
        ok = true;
    } else {                                         // eroare
        if (tokens[tokens.size() - 1].find(ERROR_MSG) != string::npos) {
            string s = tokens[tokens.size() - 1].erase(0, strlen(ERROR_MSG) + 1);
            s.erase(prev(s.end()));
            s.erase(prev(s.end()));
            cout << s << endl;
            cout << "\n";
        } else {                                     // timeout
            cout << tokens[tokens.size() - 1] << endl;
            cout << "\n";
        }
    }
    return ok;
}

/**
 * Parseaza raspunsul primit in urma unei cereri de intrare in biblioteca.
 * In cazul in care intrarea s-a efectuat cu succes, se afiseaza un
 * mesaj corespunzator de intampinare. 
 * In caz contrar, se afiseaza mesajul de eroare primit de la server.
 * Returneaza un boolean true daca intrarea s-a efectuat cu succes
 * pentru a se sti daca se poate extrage sau nu tokenul jwt.
 * @response = mesajul primit de la server;
 * @return intrare succes/fail.
*/
bool parseEnterLibraryResponse(char* response) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    bool ok = false;
    while(getline(tok, token, '\n')) {              // imparte mesajul in tokens
        tokens.push_back(token);
    }
    if (tokens[0].find("200") != string::npos) {    // codul 200 = succes
        cout << "Enter success! You have acces to library!" << endl;
        cout << "\n";
        ok = true;
    } else {                                        // eroare
        if (tokens[tokens.size() - 1].find(ERROR_MSG) != string::npos) {
            string s = tokens[tokens.size() - 1].erase(0, strlen(ERROR_MSG) + 1);
            s.erase(prev(s.end()));
            s.erase(prev(s.end()));
            cout << s << endl;
            cout << "\n";
        } else {                                    // timeout
            cout << tokens[tokens.size() - 1] << endl;
            cout << "\n";
        }
    }
    return ok;
}

/**
 * Parseaza un string ce reprezinta o lista de obiecte json intr-un vector
 * de stringuri individuale cu informatiile fiecarui obiect json in parte.
 * Delimitarea informatiilor in lista se face dupa secventa "},{".
 * @jsonsString = lista informatii carti sub forma de string;
 * @return books = vector de stringuri ce reprezinta informatiile cartilor.
*/
vector <string> parseJSONList(string jsonsString) {
    size_t pos = 0;                                // pozitia secv delimitatoare
    string token;                                  // token informatii carte
    vector <string> books;                         // informatii carti
    while ((pos = jsonsString.find("},{")) != string::npos) {
        token = jsonsString.substr(0, pos);        // extrage token
        if (token[0] != '{') {
            token.insert(0, 1, '{');               
        }
        if (token[token.size() - 1] != '}') {
            token.push_back('}');                   
        }
        books.push_back(token);                     
        jsonsString.erase(0, pos + 3);              // actualizeaza carti ramase
    }
    if (jsonsString != "") {                        // parseaza info ultima carte
        if (jsonsString[0] != '{') {
            jsonsString.insert(0, 1, '{');
        }
        if (token[token.size() - 1] != '}') {
            token.push_back('}');
        }
        books.push_back(jsonsString);
    }
    return books;
}

/**
 * Parseaza un string in obiecte json idividuale si afiseaza informatiile
 * din acestea.
 * @jsonsString = lista informatii carti sub forma de string;
 * @id = idul unei carti ce trebuie adaugat separat in jsons, deoarece acesta
 * nu este intors in mesajul de la server.
*/
void parseBooks(string jsonsString, string id) {
    vector<string> books = parseJSONList(jsonsString);
    if (books.size() == 0) {                         // verifica daca avem carti
        cout << "Unfortunately, you don't have any books." << endl;
        cout << "\n";
        return;
    }
    for (unsigned int i = 0; i < books.size(); i++) {
        auto j = json::parse(books[i]);             // parseaza informatii carte
        if (id != "") {                             // adauga id
            j["id"] = stoi(id);
        }
        cout << "Book " << i+1 << ":" << endl;
        cout << j.dump(4) << endl;
    }
    cout << "\n";
}

/**
 * Parseaza raspunsul primit in urma unei cereri de carti din biblioteca.
 * In cazul in care se cer informatii despre toate cartile, acestea sunt afisate.
 * In cazul in care se cere o carte explicit prin id, iar cartea a fost gasita, 
 * se afiseaza informatiile acesteia.
 * In cazul in care lista de carti este goala, se specifica acest lucru.
 * In caz contrar, se afiseaza mesajul de eroare primit de la server.
 * @response = mesajul primit de la server;
 * @id = idul cartii cerute.
*/
void parseGetBooksResponse(char* response, string id) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    while(getline(tok, token, '\n')) {              // imparte mesajul in tokens
        tokens.push_back(token);
    }
    if (tokens[0].find("200") != string::npos) {    // codul 200 = succes
        cout << "Get request success! There are your book(s):" << endl;
        string jsonsString = tokens[tokens.size() - 1];
        jsonsString.erase(prev(jsonsString.end()));
        jsonsString.erase(0, 1);
        parseBooks(jsonsString, id);
    } else {                                        // eroare
        if (tokens[tokens.size() - 1].find(ERROR_MSG) != string::npos) {
            string s = tokens[tokens.size() - 1].erase(0, strlen(ERROR_MSG) + 1);
            s.erase(prev(s.end()));
            s.erase(prev(s.end()));
            cout << s << endl;
            cout << "\n";
        } else {                                    // timeout
            cout << tokens[tokens.size() - 1] << endl;
            cout << "\n";
        }
    }
}

/**
 * Parseaza raspunsul primit in urma unei incercari de adaugare in biblioteca.
 * In cazul in care utilizatorul nu este logat sau nu are acces la biblioteca
 * se afiseaza mesajul de eroare primit de la server.
 * @response = mesajul primit de la server;
 * @title = titlul cartii adaugate.
*/
void parseAddBookResponse(char* response, string title) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    while(getline(tok, token, '\n')) {
        tokens.push_back(token);                    // imparte mesajul in tokens
    }
    if (tokens[0].find("200") != string::npos) {    // codul 200 = succes
        cout << "Book \"" << title <<"\" has been published!" << endl;
        cout << "\n";
    } else {                                        // eroare
        if (tokens[tokens.size() - 1].find(ERROR_MSG) != string::npos) {
            string s = tokens[tokens.size() - 1].erase(0, strlen(ERROR_MSG) + 1);
            s.erase(prev(s.end()));
            s.erase(prev(s.end()));
            cout << s << endl;
            cout << "You don't have access to library!" << endl;
            cout << "\n";
        } else {                                  // timeout
            cout << tokens[tokens.size() - 1] << endl;
            cout << "\n";
        }
    }
}

/**
 * Parseaza raspunsul primit in urma unei incercari de stergere din biblioteca.
 * In cazul in care utilizatorul nu este logat sau nu are acces la biblioteca
 * se afiseaza mesajul de eroare primit de la server.
 * @response = mesajul primit de la server;
 * @id = idul cartii sterse.
*/
void parseDeleteBookResponse(char* response, string id) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    while(getline(tok, token, '\n')) {
        tokens.push_back(token);                    // imparte mesajul in tokens
    }
    if (tokens[0].find("200") != string::npos) {    // codul 200 = succes
        cout << "Book with id \"" << id <<"\" has been deleted!" << endl;
        cout << "\n";
    } else {                                        // eroare
        if (tokens[tokens.size() - 1].find(ERROR_MSG) != string::npos) {
            string s = tokens[tokens.size() - 1].erase(0, strlen(ERROR_MSG) + 1);
            s.erase(prev(s.end()));
            s.erase(prev(s.end()));
            cout << s << endl;
            cout << "\n";   
        } else {                                    // timeout
            cout << tokens[tokens.size() - 1] << endl;
            cout << "\n";
        }
    }
}

/**
 * Parseaza raspunsul primit in urma unei incercari de logout.
 * In cazul in care utilizatorul nu este logat se afiseaza mesajul de eroare 
 * primit de la server.
 * @response = mesajul primit de la server;
 * @user = informatiile userului care s-a delogat.
*/
void parseLogoutResponse(char* response, json user) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    while(getline(tok, token, '\n')) {
        tokens.push_back(token);                    // imparte mesajul in tokens
    }
    if (tokens[0].find("200") != string::npos) {    // codul 200 = succes
        cout << "Bye " << user["username"] << "!" << endl;
        cout << "\n";
    } else {                                        // eroare
        if (tokens[tokens.size() - 1].find(ERROR_MSG) != string::npos) {
            string s = tokens[tokens.size() - 1].erase(0, strlen(ERROR_MSG) + 1);
            s.erase(prev(s.end()));
            s.erase(prev(s.end()));
            cout << s << endl;
            cout << "\n";
        } else {                                    // timeout
            cout << tokens[tokens.size() - 1] << endl;
            cout << "\n";
        }
    }
}

/**
 * Parseaza cookieul de sesiune dintr-un mesaj de raspuns primit de la server.
 * @response = mesaj de raspuns primit de la server;
 * @return cookie de sesiune.
*/
string getSessionCookie(char* response) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    while(getline(tok, token, '\n')) {
        tokens.push_back(token);
    }
    for (unsigned int i = 0; i < tokens.size(); i++) {
        if (tokens[i].find(COOKIE_MSG) != string::npos) {
            string cookie = tokens[i].erase(0, strlen(COOKIE_MSG));
            vector <string> cookiesParts;
            string cookiePart;
            stringstream scook(cookie);
            while(getline(scook, cookiePart, ' ')) {
                cookiesParts.push_back(cookiePart);
                break;
            }
            return cookiesParts[0];
        }
    }
    return NULL;
}

/**
 * Parseaza tokenul jwt dintr-un mesaj de raspuns primit de la server.
 * @response = mesaj de raspuns primit de la server;
 * @return token jwt.
*/
string getTokenJWT(char* response) {
    vector <string> tokens;
    string token;
    stringstream tok(response);
    while(getline(tok, token, '\n')) {
        tokens.push_back(token);
    }
    for (unsigned int i = 0; i < tokens.size(); i++) {
        if (tokens[i].find(JWT_MSG) != string::npos) {
            string jwt = tokens[i].erase(0, strlen(JWT_MSG));
            jwt.erase(prev(jwt.end()));
            jwt.erase(prev(jwt.end()));
            return jwt;
        }
    }
    return NULL;
}

/**
 * Formeaza mesaj de post request catre server pentru inregistrare utilizator.
 * @host = ip server;
 * @username = nume utilizator inregistrat;
 * @password = parola utilizator inregistrat;
 * @return post request.
*/
char* register_POST_req(const char* host, string username, string password) {
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    DIE(!message, "Message alocation failed!");
    char* line = (char*)calloc(LINELEN, sizeof(char));
    DIE(!line, "Line alocation failed!");

    json j;                                         
    j["username"] = username;
    j["password"] = password;

    memset(message, 0, BUFLEN);
    sprintf(line, "POST /api/v1/tema/auth/register");
    compute_message(message, line);                 // adauga tip + url

    sprintf(line, "Host: %s", host);
    compute_message(message, line);                 // adauga host

    sprintf(line, "Content-Type: %s", JSON_URL);
    compute_message(message, line);                 // adauga tip continut

    sprintf(line, "Content-Length: %ld", strlen(j.dump().c_str()));
    compute_message(message, line);                 // adauga lungime mesaj

    compute_message(message, "");                   // adauga linie noua
    compute_message(message, j.dump().c_str());     // adauga data

    free(line);
    return message;
}

/**
 * Inregistreaza un nou utilizator in server.
*/
void Register(int sockfd) {
    char *message;
    char *response;
    string username;
    string password;
    cout << "username=";
    getline(cin, username);
    cout << "password=";
    getline(cin, password);
    message = register_POST_req(SERVER, username, password);
    send_to_server(sockfd, message);            // trimite post request
    response = receive_from_server(sockfd);     // primeste raspuns
    parseRegisterResponse(response);            // afiseaza mesaj corespunzator
}

/**
 * Formeaza mesaj de post request catre server pentru autentificare utilizator.
 * @host = ip server;
 * @username = nume utilizator autentificat;
 * @password = parola utilizator autentificat;
 * @user = date utilizator autentificat;
 * @return post request.
*/
char* login_POST_req(const char* host, string username, string password, json& user) {
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    DIE(!message, "Message alocation failed!");
    char* line = (char*)calloc(LINELEN, sizeof(char));
    DIE(!line, "Line alocation failed!");

    json j;
    j["username"] = username;
    j["password"] = password;

    memset(message, 0, BUFLEN);
    sprintf(line, "POST /api/v1/tema/auth/login");  
    compute_message(message, line);                 // adauga tip + url

    sprintf(line, "Host: %s", host);
    compute_message(message, line);                 // adauga host

    sprintf(line, "Content-Type: %s", JSON_URL);    
    compute_message(message, line);                 // adauga tip continut

    sprintf(line, "Content-Length: %ld", strlen(j.dump().c_str()));
    compute_message(message, line);                 // adauga lungime mesaj

    compute_message(message, "");                   // adauga linie noua
    compute_message(message, j.dump().c_str());     // adauga data
    user = j;                                       // actualizeaza utilizator
    free(line);
    return message;
}

/**
 * Autentifica un utilizator si pastreaza cookie primit de la server.
*/
void LogIn(int sockfd, string& cookie, json& user) {
    char *message;
    char *response;
    string username;
    string password;
    cout << "username=";
    getline(cin, username);
    cout << "password=";
    getline(cin, password);
    message = login_POST_req(SERVER, username, password, user);
    send_to_server(sockfd, message);            // trimite post request
    response = receive_from_server(sockfd);     // primeste raspuns
    if (parseLoginResponse(response, user)) {   // afiseaza mesaj corespunzator
        cookie = getSessionCookie(response);    // actualizeaza cookie
    }
}

/**
 * Formeaza mesaj de get request catre server pentru primire acces biblioteca.
 * @host = ip server;
 * @cookie = coockie de sesiune;
 * @return get request.
*/
char* enter_GET_req(const char* host, string cookie) {
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    DIE(!message, "Message alocation failed!");
    char* line = (char*)calloc(LINELEN, sizeof(char));
    DIE(!line, "Line alocation failed!");
    char *cookie_buffer = (char*)calloc(LINELEN, sizeof(char));
    DIE(!cookie_buffer, "Cookie alocation failed!");

    memset(message, 0, BUFLEN);
    sprintf(line, "GET /api/v1/tema/library/access HTTP/1.1");
    compute_message(message, line);         // adauga tip + url + versiune

    sprintf(line, "Host: %s", host);
    compute_message(message, line);         // adauga host

    if (cookie.c_str()) {                   // adauga cookie
        strcpy(cookie_buffer, "Cookie: ");
        strcat(cookie_buffer, cookie.c_str());
        compute_message(message, cookie_buffer);
    }

    compute_message(message, "");           // adauga linie noua
    free(line);
    free(cookie_buffer);
    return message;
}

/**
 * Ofera acces in biblioteca utilizatorului curent panstrand tokenul jwt pentru
 * a demonstra ca utlizatorul are acces pentru alte operatii.
*/
void EnterLibrary(int sockfd, string cookie, string& jwt) {
    char *message;
    char *response;
    message = enter_GET_req(SERVER, cookie);
    send_to_server(sockfd, message);            // trimite get request
    response = receive_from_server(sockfd);     // primeste raspuns
    if (parseEnterLibraryResponse(response)) {  // afiseaza mesaj corespunzator
        jwt = getTokenJWT(response);            // actualizare token
    }
}

/**
 * Formeaza mesaj de get request catre server pentru primire informatii carti.
 * @host = ip server;
 * @jwt = token jwt de acces;
 * @queryParams = parametrii queryului (in cazul acesta se asteapta o lista
 * de obiecte json asa ca se va trimite ca paramtru "application/json");
 * @return get request.
*/
char* getBooks_GET_req(const char* host, string jwt, const char* queryParams) {
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    DIE(!message, "Message alocation failed!");
    char* line = (char*)calloc(LINELEN, sizeof(char));
    DIE(!line, "Line alocation failed!");
    char *token_buffer = (char*)calloc(LINELEN, sizeof(char));
    DIE(!token_buffer, "Cookie alocation failed!");

    memset(message, 0, BUFLEN);
    if (queryParams != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", "/api/v1/tema/library/books", queryParams);
    } else {
        sprintf(line, "GET %s HTTP/1.1", "/api/v1/tema/library/books");
    }
    compute_message(message, line);         // adauga tip + versiune + parametri

    sprintf(line, "Host: %s", host);        // adauga host
    compute_message(message, line);

    if (jwt.c_str()) {                      // adauga token jwt
        strcpy(token_buffer, "Authorization: Bearer ");
        strcat(token_buffer, jwt.c_str());
        compute_message(message, token_buffer);
    }

    compute_message(message, "");           // adauga linie noua
    free(line);
    free(token_buffer);
    return message;
}

/**
 * Efectueaza cererea de vizualizare carti disponibile.
*/
void GetBooks(int sockfd, string jwt) {
    char *message;
    char *response;
    message = getBooks_GET_req(SERVER, jwt, JSON_URL);
    send_to_server(sockfd, message);                // trimite get request
    response = receive_from_server(sockfd);         // primeste raspuns
    parseGetBooksResponse(response, "");            // afiseaza mesaj corespunzator
}

/**
 * Formeaza mesaj de get request catre server pentru primire informatii 
 * pentru o carte specifica.
 * @host = ip server;
 * @jwt = token jwt de acces;
 * @queryParams = parametrii queryului (in cazul acesta se asteapta o lista
 * de obiecte json asa ca se va trimite ca paramtru "application/json");
 * @bookId = idul cartii dorite;
 * @return get request.
*/
char* getBook_GET_req(const char* host, string jwt, const char* queryParams, string bookID) {
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    DIE(!message, "Message alocation failed!");
    char* line = (char*)calloc(LINELEN, sizeof(char));
    DIE(!line, "Line alocation failed!");
    char *token_buffer = (char*)calloc(LINELEN, sizeof(char));
    DIE(!token_buffer, "Token alocation failed!");

    memset(message, 0, BUFLEN);
    if (queryParams != NULL) {
        sprintf(line, "GET %s%s?%s HTTP/1.1", "/api/v1/tema/library/books/", bookID.c_str(), queryParams);
    } else {
        sprintf(line, "GET %s%s HTTP/1.1", "/api/v1/tema/library/books/", bookID.c_str());
    }
    compute_message(message, line);         // adauga tip + versiune + parametri

    sprintf(line, "Host: %s", host);
    compute_message(message, line);         // adauga host

    if (jwt.c_str()) {                      // adauga token jwt
        strcpy(token_buffer, "Authorization: Bearer ");
        strcat(token_buffer, jwt.c_str());
        compute_message(message, token_buffer);
    }

    compute_message(message, "");           // adauga linie noua
    free(line);
    free(token_buffer);
    return message;
}

/**
 * Efectueaza cerere vizualizare informatii pentru o carte specifica.
*/
void GetBook(int sockfd, string jwt) {
    char *message;
    char *response;
    string id;
    cout << "id=";
    getline(cin, id);
    message = getBook_GET_req(SERVER, jwt, JSON_URL, id);
    send_to_server(sockfd, message);            // trimite get request
    response = receive_from_server(sockfd);     // primeste raspuns
    parseGetBooksResponse(response, id);        // afiseaza mesaj corespunzator
}

/**
 * Formeaza mesaj de post request catre server pentru a adauga o noua carte.
 * @host = ip server;
 * @jwt = token jwt de acces;
 * @return post request.
*/
char* addBook_POST_req(const char* host, string jwt, string title, string author,
                        string genre, string publisher, string page_count) {
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    DIE(!message, "Message alocation failed!");
    char* line = (char*)calloc(LINELEN, sizeof(char));
    DIE(!line, "Line alocation failed!");
    char *token_buffer = (char*)calloc(LINELEN, sizeof(char));
    DIE(!token_buffer, "Token alocation failed!");

    json j;
    j["title"] = title;
    j["author"] = author;
    j["genre"] = genre;
    j["publisher"] = publisher;
    j["page_count"] = page_count;

    memset(message, 0, BUFLEN);
    sprintf(line, "POST /api/v1/tema/library/books");
    compute_message(message, line);                 // adauga tip + url

    sprintf(line, "Host: %s", host);
    compute_message(message, line);                 // adauga host

    if (jwt.c_str()) {                              // formeaza token jwt
        strcpy(token_buffer, "Authorization: Bearer ");
        strcat(token_buffer, jwt.c_str());
    }

    sprintf(line, "Content-Type: %s", JSON_URL);
    compute_message(message, line);                 // adauga tip continut

    sprintf(line, "Content-Length: %ld", strlen(j.dump().c_str()));
    compute_message(message, line);                 // adauga lungime continut

    compute_message(message, token_buffer);         // adauga token jwt

    compute_message(message, "");                   // adauga linie noua
    compute_message(message, j.dump().c_str());     // adauga date
    
    free(line);
    free(token_buffer);
    return message;
}

/**
 * Adauga o noua carte in bilbioteca.
*/
void AddBook(int sockfd, string jwt) {
    char *message;
    char *response;
    string title;
    string author;
    string genre;
    string publisher;
    string page_count;
    cout << "title=";
    getline(cin, title);
    cout << "author=";
    getline(cin, author);
    cout << "genre=";
    getline(cin, genre);
    cout << "publisher=";
    getline(cin, publisher);
    cout << "page_count=";
    getline(cin, page_count);
    message = addBook_POST_req(SERVER, jwt, title, author, genre, publisher, page_count);
    send_to_server(sockfd, message);            // trimite post request
    response = receive_from_server(sockfd);     // primeste raspuns
    parseAddBookResponse(response, title);      // afiseaza mesaj corespunzator
}

/**
 * Formeaza mesaj de delete request catre server pentru a sterge o carte.
 * @host = ip server;
 * @jwt = token jwt de acces;
 * @queryParams = parametrii queryului (in cazul acesta NULL);
 * @bookId = idul cartii dorite a fi sterse;
 * @return delete request.
*/
char* DELETE_req(const char* host, string jwt, const char* queryParams, string bookID) {
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    DIE(!message, "Message alocation failed!");
    char* line = (char*)calloc(LINELEN, sizeof(char));
    DIE(!line, "Line alocation failed!");
    char *token_buffer = (char*)calloc(LINELEN, sizeof(char));
    DIE(!token_buffer, "Token alocation failed!");

    memset(message, 0, BUFLEN);
    if (queryParams != NULL) {
        sprintf(line, "DELETE %s%s?%s HTTP/1.1", "/api/v1/tema/library/books/", bookID.c_str(), queryParams);
    } else {
        sprintf(line, "DELETE %s%s HTTP/1.1", "/api/v1/tema/library/books/", bookID.c_str());
    }
    compute_message(message, line);     // adauga tip + versiune + parametri

    sprintf(line, "Host: %s", host);
    compute_message(message, line);     // adauga host

    if (jwt.c_str()) {
        strcpy(token_buffer, "Authorization: Bearer ");
        strcat(token_buffer, jwt.c_str());
        compute_message(message, token_buffer); // adauga token jwt
    }

    compute_message(message, "");               // adauga linie noua
    free(line);
    free(token_buffer);
    return message;
}

/**
 * Sterge o carte din server.
*/
void DeleteBook(int sockfd, string jwt) {
    char *message;
    char *response;
    string id;
    cout << "id=";
    getline(cin, id);
    message = DELETE_req(SERVER, jwt, NULL, id);
    send_to_server(sockfd, message);             // trimite get request
    response = receive_from_server(sockfd);      // primeste raspuns
    parseDeleteBookResponse(response, id);       // afiseaza mesaj corespunzator
}

/**
 * Formeaza mesaj de get request catre server pentru delogare utilizator.
 * @host = ip server;
 * @cookie = cookie de sesiune;
 * @return post request.
*/
char* logout_GET_req(const char* host, string cookie) {
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    DIE(!message, "Message alocation failed!");
    char* line = (char*)calloc(LINELEN, sizeof(char));
    DIE(!line, "Line alocation failed!");
    char *cookie_buffer = (char*)calloc(LINELEN, sizeof(char));
    DIE(!cookie_buffer, "Cookie alocation failed!");

    memset(message, 0, BUFLEN);
    sprintf(line, "GET /api/v1/tema/auth/logout");
    compute_message(message, line);                 // adauga tip + url

    sprintf(line, "Host: %s", host);
    compute_message(message, line);                 // adauga host

    if (cookie.c_str()) {                           // adauga cookie
        strcpy(cookie_buffer, "Cookie: ");
        strcat(cookie_buffer, cookie.c_str());
        compute_message(message, cookie_buffer);
    }

    compute_message(message, "");                   // adauga linie noua
    free(line);
    free(cookie_buffer);
    return message;
}

/**
 * Delogheaza utilizatorul curent.
*/
void Logout(int sockfd, string& cookie, string& jwt, json user) {
    char *message;
    char *response;
    message = logout_GET_req(SERVER, cookie);
    send_to_server(sockfd, message);            // trimite get request
    response = receive_from_server(sockfd);     // primeste raspuns
    cookie = "";                                // sterge cookie
    jwt = "";                                   // sterge token jwt
    parseLogoutResponse(response, user);        // afiseaza mesaj corespunzator
}