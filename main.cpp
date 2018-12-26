#include <iostream>
#include <winsock2.h>
#include "WinsockEnv.h"
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <map>
#include <string.h>
#include <queue>
#include <sstream>


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

    /**
     * 设置Socket
     * @param af IP协议
     * @param type 字节流套接字
     * @param protocol 传输协议
     * @return 成功返回0 失败返回 -1
     */
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
        server_sockaddr.sin_addr.s_addr = addr;

        if (bind(server_socketId, (LPSOCKADDR) &server_sockaddr, sizeof(server_sockaddr)) == -1) {
            cout << "Bind Error" << endl;
            return -1;
        }
        return 0;
    }
    /**
     * 设置最大监听的个数
     * @param blacklog 允许监听的个数
     * @return 成功返回0 失败返回-1
     */
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
    static string dir;
    string requestOder;
    int len;
    string msg;
    string fileType;

    Data(int len = 1000) {
        this->len = len;
        rcvData = new char[len];
        path = "";
        requestOder = "";
        fileType = "";
    }
    /**
     * 提取请求中的信息
     * @return 请求的文件名
     */
    string getRequest() {
        path = "";
        requestOder = "";
        fileType = "unKnown";

        for (int i = 0; i < 5; i++) {
            requestOder = requestOder + rcvData[i];
        }

        // 请求的命令不符合预期
        if (requestOder != "GET /") {
            path = "None";
            fileType = "UnKnow";
            return path;
        }
        bool isType = 0;

        // 获得请求的文件类型和文件名字
        for (int i = 5; i < len; i++) {

            if (rcvData[i] != ' ' && rcvData[i] != '?') {
                if (isType) {
                    fileType += rcvData[i];
                }

                if (rcvData[i] == '.') {
                    fileType = "";
                    isType = 1;
                }

                path = path + rcvData[i];
            } else {
                break;
            }
        }
        return path;
    }

    /**
     * 判断文件类型并返回对应的值
     * @return
     * 0: 为止
     * 1: 文本
     * 2: 图片
     * 3: js
     * 4: application
     */
    int whichType() {
        if (fileType == "html" || fileType == "plain" || fileType == "css") {
            return 1;
        } else if (fileType == "png" || fileType == "jpeg" || fileType == "jpg" ||
                   fileType == "gif") {
            return 2;
        } else if (fileType == "js") {
            return 3;
        } else if (fileType == "ttf") {
            fileType = "x-font-ttf";
            return 4;
        } else if (fileType == "otf") {
            fileType = "x-font-opentype";
            return 4;
        } else if (fileType == "woff" || fileType == "woff2") {
            fileType = "font-" + fileType;
            return 4;
        } else if (fileType == "eot") {
            fileType = "vnd.ms-fontobject";
            return 4;
        } else if (fileType == "sfnt") {
            fileType = "font-sfnt";
            return 4;
        } else if (fileType == "svg") {
            fileType += "svg+xml";
            return 2;
        }
        return 0;
    }

    /**
     * 对请求的文件进行检查、传输并记录相关信息
     * @param client_sock 与创建的连接相关的套接字
     * @return 文件定位成功返回0 文件定位失败返回1
     */
    int sendData(SOCKET &client_sock) {

        // 二进制打开文件以传输图片等媒体信息
        ifstream infile(dir + this->path, ios::binary);
        string http;

        long long fileSize;
        int type = 0;
        if (infile.is_open()) {
            infile.seekg(0, ios::end);
            fileSize = (long long) infile.tellg();
            infile.seekg(0, ios::beg);

            // 成功的回应头
            http = "HTTP/1.1 200 OK\r\n";
            http += "Server: cainot's server\r\n";
            http += "Connection: keep-alive\r\n";

            // 获取文件类型
            type = this->whichType();

            // 根据文件类型来确定回应头
            switch (type) {
                case 1:
                    http += "Content-Type: text/" + this->fileType + "; charset=utf-8\r\n";
                    break;
                case 2:
                    http += "Content-Type: image/" + this->fileType + "\r\n";
                    break;
                case 3:
                    http += "Content-Type: application/javascript\r\n";
                    break;
                case 4:
                    http += "Content-Type: application/" + this->fileType + "\r\n";
                default:
                    http += "Content-Type: text/txt; charset=utf-8\r\n";
                    break;
            }
            http += "Content-Length: " + to_string(fileSize) + "\r\n\r\n";

            msg = "200 OK";
        } else {
            // 二进制打开文件以传输图片
            infile.open("D:/ServerData/notFound.html", ios::binary);
            if (!infile.is_open()) {
                fileSize = 0;
            } else {

                // 获取文件的大小

                infile.seekg(0, ios::end);
                fileSize = (int) infile.tellg();
                infile.seekg(0, ios::beg);
            }

            // 失败的回应头
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
        // 先把报文头发送
        send(client_sock, http.c_str(), http.size(), 0);

        int length = 10240;
        char *buffer = new char[length + 1];

        // 发送文件的内容
        while (!infile.eof()) {
            infile.read(buffer, sizeof(char) * length);
            send(client_sock, buffer, length, 0);
        }

        return 0;
    }

    /**
     * 析构函数 释放资源
     */
    virtual ~Data() {
        if (rcvData)
            delete[] rcvData;
    }
};

// 初始化本地路径
string Data::dir = "D:/ServerData/";

// 工作线程
map<SOCKET, thread *> threads;

// 完工线程
queue<thread *> doneThreads;

/**
 * 将各个模块综合，并负责打印出提示信息
 * @param client_sock 本次连接的
 * @param client_sockaddr
 * @return
 */
int respond(SOCKET client_sock, struct sockaddr_in client_sockaddr) {
    Data data;

    int recvLen = recv(client_sock, data.rcvData, 1000, 0);

    if (recvLen == SOCKET_ERROR) {
        cout << "receive Error" << endl;
        return -1;
    }


    data.getRequest();

    data.sendData(client_sock);

    // 通过锁来实现IO流的互斥
    my_mutex.lock();

    string result =
            "address: " + string(inet_ntoa(client_sockaddr.sin_addr)) + "\tport" + to_string(client_sockaddr.sin_port) +
            "\trequest: " + data.requestOder + data.path + "\tresult: " + data.msg + "\n";

    cout << "address: " << inet_ntoa(client_sockaddr.sin_addr) << "\tport: " << client_sockaddr.sin_port
         << "\trequest: " + data.requestOder + data.path << "\tresult: " << data.msg << endl;
    thread *t = ::threads[client_sock];
    doneThreads.push(t);
    ::threads.erase(client_sock);
    my_mutex.unlock();

    this_thread::sleep_for(chrono::seconds(5));

    closesocket(client_sock);


    return 0;
}

/**
 * 当服务器未终止时检查完工线程队列中是否还有线程未被回收资源
 * 若有则回收，否则等待
 * @param isEnd
 */
void deleteThread(int *isEnd) {
    thread *t;
    while (*isEnd == 0) {
        this_thread::sleep_for(chrono::seconds(2));// 减少CPU利用率

        if (!doneThreads.empty()) {// 防止不停地上锁解锁提高响应速度
            my_mutex.lock();
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
            my_mutex.unlock();
        }
    }

    cout << "work done" << endl;

}
/**
 * 配置模块，主要负责读取配置信息
 */
class Config {
public:
    u_long address;
    u_short port;
    string dirFromFile;
    /**
     * 初始化为默认的配置信息
     */
    Config() {
        address = INADDR_ANY;
        port = 80;
        dirFromFile = "D:/ServerData/";
    }
    /**
     * 配置文件名默认为config.dat
     */
    void read() {
        ifstream infile("config.dat");
        string line;
        getline(infile, line);

        if (line == "ALL") { // 如果为'ALL'则将监听IP设为全部
            address = INADDR_ANY;
        } else {
            address = inet_addr(line.c_str());
        }
        getline(infile, line);
        port = (u_short) stoi(line); // 字符串转u_short
        getline(infile, line);
        dirFromFile = line;
    }

};

int end = 0;

void runServer() {
    Config c;

    //读取配置信息
    c.read();

    HttpServer hp;

    // 初始化
    hp.start();

    hp.setSocket();

    // 配置服务器
    hp.bind_hs(c.address, c.port);
    hp.listen_hs();
    int isEnd = 0;

    Data::dir = c.dirFromFile;

    // 创建线程池中的销毁线程
    thread thread2(deleteThread, &isEnd);

    while (::end == 1) {
        struct sockaddr_in client_sockaddr;
        int client_sockaddr_size = sizeof(client_sockaddr);

        // 阻塞式调用accept函数
        SOCKET client_sock = accept(hp.getServerID(), (LPSOCKADDR) &client_sockaddr, &client_sockaddr_size);
        if (client_sock == -1) {// 接受请求时发生了错误
            cout << "accept Error" << endl;
            break;

        }
        // 每次收到一个请求就创建一个新的线程来对请求进行处理
        thread *t = new thread(respond, client_sock, client_sockaddr);
        // 将请求的SOCKET作为线程的标识放入进程map中
        threads[client_sock] = t;
    }
    // 关闭服务器
    closesocket(hp.getServerID());
}


int main() {
    while (::end != 1 && ::end != 2) {
        cout << "Server is waiting for suitable input:(1 for start and 2 for end)" << endl;
        cin >> ::end;
    }
    if (::end == 1)
        cout << "Server is Began" << endl;
    else if (::end == 2) {
        cout << "Server is End\n";
        return 0;
    }
    thread t(runServer);
    t.detach();

    while (::end != 2) {
        cin >> ::end;
    }
    cout << "Server is End\n";

    return 0;
}