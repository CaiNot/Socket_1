//
// Created by CaiNot on 2018/11/6 18:41.
//

#ifndef SOCKET_1_WINSOCKENV_H
#define SOCKET_1_WINSOCKENV_H
#pragma once

class WinsockEnv
{
private:
    WinsockEnv(void);
    ~WinsockEnv(void);
public:
    static int Startup();
};


#endif //SOCKET_1_WINSOCKENV_H
