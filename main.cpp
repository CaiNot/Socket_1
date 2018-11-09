#include <iostream>
#include <winsock2.h>
#include "WinsockEnv.h"
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <map>
#include <queue>


using namespace std;

static mutex my_mutex;

class HttpServer {
    int af;
    int type;
    int protocol;
    SOCKET server_socketId;
    struct sockaddr_in server_sockaddr;
public:
    HttpServer() {

        /**
         * IPV4协议   字节流套接字  TCP传输协议
         */
        this->af = AF_INET;
        this->type = SOCK_STREAM;
        this->protocol = IPPROTO_TCP;
    }

    int start() {
        return WinsockEnv::Startup() == 0;
    }

    int setSocket(int af = AF_INET, int type = SOCK_STREAM, int protocol = IPPROTO_TCP) {
        this->af = af;
        this->type = type;
        this->protocol = protocol;

        this->server_socketId = socket(this->af, this->type, this->protocol);
        if (this->server_socketId == -1) {
            cout << "Socket Error" << endl;
            return -1;
        }
        return 0;
    }

    /**
    * 协议 IPV4  端口 80   宏定义绑定本地IP
    */
    int bind_hs(u_long addr = INADDR_ANY, u_short port = 80) {

        server_sockaddr.sin_family = this->af;
        server_sockaddr.sin_port = htons(port);
        server_sockaddr.sin_addr.s_addr = htonl(addr);

        if (bind(server_socketId, (LPSOCKADDR) &server_sockaddr, sizeof(server_sockaddr)) == -1) {
            cout << "Bind Error" << endl;
            return -1;
        }
        return 0;
    }

    int listen_hs(int blacklog = 20) {
        if (listen(server_socketId, blacklog) == -1) {
            cout << "listen Error" << endl;
            return -1;
        }
        return 0;
    }

    SOCKET getServerID() {
        return server_socketId;
    }
};

struct Data {
    char *rcvData;
    string path;
    string requestOder;
    int len;
    string msg;

    Data(int len = 1000) {
        this->len = len;
        rcvData = new char[len];
        path = "";
        requestOder = "";
    }

    string getRequest() {
        path = "";
        for (int i = 5; i < len; i++) {
            if (rcvData[i] != ' ') {
                path = path + rcvData[i];
            } else {
                break;
            }
        }
        for (int i = 0; i < 5; i++) {
            requestOder = requestOder + rcvData[i];
        }
        return path;
    }

    int sendData(SOCKET &client_sock) {
        ifstream infile("D:/ServerData/" + this->path);
//        cout << this->path << endl;
        string http;

        int fileSize;
        if (infile.is_open()) {
            infile.seekg(0, ios::end);
            fileSize = (int) infile.tellg();
            infile.seekg(0, ios::beg);

            http = "HTTP/1.1 200 OK\r\n";
            http += "Server: cainot's server\r\n";
            http += "Connection: close\r\n";
            http += "Content-Type: text/html; charset=utf-8\r\n";
            http += "Content-Length: " + to_string(fileSize) + "\r\n\r\n";

            msg = "200 OK";
        } else {
            infile.open("D:/ServerData/notFound.html");
            if (!infile.is_open()) {
                fileSize = 0;
            } else {
                infile.seekg(0, ios::end);
                fileSize = (int) infile.tellg();
                infile.seekg(0, ios::beg);
            }

            http = "HTTP/1.1 404 Not Found\r\n";
            http += "Server: cainot's server\r\n";
            http += "Connection: close\r\n";
            http += "Content-Type: text/html; charset=utf-8\r\n";
            http += "Content-Length: " + to_string(fileSize) + "\r\n\r\n";

            msg = "404 Not Found";
            if (!fileSize) {
                send(client_sock, http.c_str(), http.size(), 0);
                msg = "Local File Open Error";
                return 1;
            }
        }
        send(client_sock, http.c_str(), http.size(), 0);

        int length = 10240;
        char *buffer = new char[length];

        while (!infile.eof()) {
            infile.read(buffer, length);
            http = string(buffer);
            send(client_sock, http.c_str(), http.size(), 0);
        }

        this_thread::sleep_for(chrono::seconds(5));
        return 0;
    }

    virtual ~Data() {
        if (rcvData)
            delete[] rcvData;
    }
};


map<SOCKET, thread *> threads;
queue<thread *> doneThreads;

int respond(SOCKET client_sock, struct sockaddr_in client_sockaddr) {
    Data data;
    int recvLen = recv(client_sock, data.rcvData, 1000, 0);


    if (recvLen == SOCKET_ERROR) {
        cout << "receive Error" << endl;
        return -1;
    }
    data.getRequest();

    data.sendData(client_sock);
    closesocket(client_sock);

    lock_guard<mutex> lockGuard(my_mutex);
    cout << "address: " << inet_ntoa(client_sockaddr.sin_addr) << "\tport: " << client_sockaddr.sin_port
         << "\trequest: " + data.requestOder + data.path << "\tresult: " << data.msg << endl;
    thread *t = ::threads[client_sock];
    doneThreads.push(t);
    ::threads.erase(client_sock);
    return 0;
}

void deleteThread(int *isEnd) {
    thread *t;
    while (true) {
        while (!doneThreads.empty()) {
            t = doneThreads.front();
            if (t->joinable())
                t->join();
            else {
                cout << "Thread can not join" << endl;
                return;
            }
            delete t;
            doneThreads.pop();
        }
    }

}


int main() {
    HttpServer hp;
    hp.start();
    hp.setSocket();
    hp.bind_hs();
    hp.listen_hs();
    int *isEnd = 0;

    thread thread2(deleteThread, isEnd);


    while (1) {
        struct sockaddr_in client_sockaddr;
        int client_sockaddr_size = sizeof(client_sockaddr);
        SOCKET client_sock = accept(hp.getServerID(), (LPSOCKADDR) &client_sockaddr, &client_sockaddr_size);
        if (client_sock == -1) {
            cout << "accept Error" << endl;
            return -1;

        }
        thread *t = new thread(respond, client_sock, client_sockaddr);
        threads[client_sock] = t;
    }
    cout << "aaa normal" << endl;

    closesocket(hp.getServerID());

    cout << "OK";

    return 0;
}