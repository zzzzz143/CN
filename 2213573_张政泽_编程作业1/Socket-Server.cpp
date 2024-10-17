#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<windows.h>
#include<list>
#include<vector>
#include <algorithm>
#include <ctime>
#include<fstream>
#include <locale>
using namespace std;
ofstream outfile("./log.txt",std::ios::app);
std::time_t now;
std::tm* local_now;
bool TF;
int MaxClientNum = 10;
vector<SOCKET> ClientSocketList; // 存储客户端套接字
SOCKET sockSer; // 服务器套接字
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // 获取控制台句柄
void LogSet(string str){
    now = std::time(0);  // 获取当前时间
    local_now = std::localtime(&now);  // 转换为本地时间
    outfile<<asctime(local_now);
    outfile<<str<<endl;
}
DWORD WINAPI ListenExit(LPVOID lpParam) {
    string message;
    while(true){
        getline(cin,message);
        if(message == "./exit"){
            
            //发送响应
         for(int i=0;i<ClientSocketList.size();i++)
                if(send(ClientSocketList[i], "Server will be closed in 5 seconds", strlen("Server will be closed in 5 seconds"), 0)== SOCKET_ERROR){
                    cout<<i<<"send error"<<endl;
                }
            cout<<"Server will be closed in 5 seconds"<<endl;
            LogSet("Server will be closed in 5 seconds");
            for(int i=0;i<ClientSocketList.size();i++)
                if(send(ClientSocketList[i], "5", strlen("5"), 0)== SOCKET_ERROR){
                    cout<<i<<"send error"<<endl;
                }
            cout<<"5"<<endl;
            
            Sleep(1000);
            for(int i=0;i<ClientSocketList.size();i++)
                if(send(ClientSocketList[i], "4", strlen("4"), 0)== SOCKET_ERROR){
                    cout<<i<<"send error"<<endl;
                }
            cout<<"4"<<endl;
            
            Sleep(1000);
            for(int i=0;i<ClientSocketList.size();i++)
                if(send(ClientSocketList[i], "3", strlen("3"), 0)== SOCKET_ERROR){
                    cout<<i<<"send error"<<endl;
                }
            cout<<"3"<<endl;
            
            Sleep(1000);
            for(int i=0;i<ClientSocketList.size();i++)
                if(send(ClientSocketList[i], "2", strlen("2"), 0)== SOCKET_ERROR){
                    cout<<i<<"send error"<<endl;
                }
            cout<<"2"<<endl;
            
            Sleep(1000);
            for(int i=0;i<ClientSocketList.size();i++)
                if(send(ClientSocketList[i], "1", strlen("1"), 0)== SOCKET_ERROR){
                    cout<<i<<"send error"<<endl;
                }
            cout<<"1"<<endl;
            
            for(int i=0;i<ClientSocketList.size();i++)
                if(send(ClientSocketList[i], "Exiting...", strlen("Exiting..."), 0)== SOCKET_ERROR){
                    cout<<i<<"send error"<<endl;
                }
            cout<<"Exiting..."<<endl;
            LogSet("Exiting...");
            if(ClientSocketList.size() == 0){
                cout<<"No client connected"<<endl;
                LogSet("No client connected");
                // 关闭套接字
                closesocket(sockSer);
                break;
            }
            TF = false;
            Sleep(1000);///为了等待客户端退出
            break;
        }
    }
}
DWORD WINAPI ThreadFunction(LPVOID lpParam) {

    SOCKET ClientSocket =(SOCKET)lpParam;
    ClientSocketList.push_back(ClientSocket);

    char  recvBuf[50];
    memset(recvBuf,0,sizeof(recvBuf));   
    int recvSize;

while(true){
    if(TF == false) break;
    // 接收数据并输出
    recvSize=recv(ClientSocket,recvBuf,sizeof(recvBuf),0);
    if(recvSize == SOCKET_ERROR){ //说明退出了
        
        cout<<"Client disconnected"<<endl;
        LogSet("Client disconnected");
        // 从列表中移除客户端套接字
        auto it = std::find(ClientSocketList.begin(), ClientSocketList.end(), ClientSocket);
        // 如果找到了元素，计算它的位置并删除
        if (it != ClientSocketList.end())  ClientSocketList.erase(it);
        // 关闭套接字
        closesocket(ClientSocket);
        break;
    }
    
    else{
        recvBuf[recvSize] = '\0'; // 确保字符串以NULL结尾
        
        SOCKET valueToFind = ClientSocket;
        auto it = std::find(ClientSocketList.begin(), ClientSocketList.end(), valueToFind);
        int position;
        // 如果找到了元素，计算它的位置
        if (it != ClientSocketList.end())  position = std::distance(ClientSocketList.begin(), it);
        
        if(strstr(recvBuf, ": have exited!") != NULL){
            cout<<recvBuf<<endl;
            LogSet(recvBuf);

            Sleep(1000);
            // 关闭套接字
            for(int i=0;i<ClientSocketList.size();i++)
                closesocket(ClientSocketList[i]);
            closesocket(sockSer);
            LogSet("SOCKET closed Successfully!");
            break;
            
        }
        if(strcmp(recvBuf, "SuccessExit!") == 0){
            ClientSocketList.erase(it);
            LogSet("SOCKET erased Successfully!");
            // 关闭套接字
            closesocket(ClientSocket);
            break;
        }
        
        // 设置字体颜色
        switch(position){
            case 0: SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);break;
            case 1: SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);break;
            case 2: SetConsoleTextAttribute(hConsole, FOREGROUND_RED);break;
            case 3: SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);break;
            case 4: SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE);break;
            case 5: SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE);break;
            case 6: SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);break;
            case 7: SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);break;
            case 8: SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);break;
            case 9: SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);break;
            }    
        cout<<recvBuf<<endl;
        LogSet(recvBuf);
        // 重置字体颜色为默认值
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        //发送响应
        for(int i=0;i<ClientSocketList.size();i++)
            if(ClientSocketList[i]!=ClientSocket)
                if(send(ClientSocketList[i], recvBuf, recvSize, 0)== SOCKET_ERROR){
                    cout<<i<<"send error"<<endl;
                }
        
        if(strstr(recvBuf, "Exit") != NULL){
            if(send(ClientSocket, "OK! I got it", strlen("OK! I got it"), 0)== SOCKET_ERROR){
                    cout<<WSAGetLastError()<<endl;
                    LogSet("send error. Errno:"+to_string(WSAGetLastError()));
                    cout<<"send error"<<endl;
                }
        }
    }

    }

    return 0;
}

int main()
{

    LogSet("Server Start");

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
    // 创建套接字
    sockSer = socket(AF_INET, SOCK_STREAM, 0);
    if(sockSer == -1)
    {   
        cout<<"Socket creation failed"<<endl;
        LogSet("Socket creation failed");
    }
       
    else
    {
        cout<<"Socket created successfully"<<endl;
        LogSet("Socket created successfully");
    }


    // 定义地址结构--服务端
    sockaddr_in addressSrv;
    addressSrv.sin_family = AF_INET; // 使用IPv4
    addressSrv.sin_addr.s_addr = INADDR_ANY; // 绑定到所有可用接口
    addressSrv.sin_port = htons(8087); // 端口号，htons确保端口字节序正确
    // 绑定套接字到地址和端口
    if(bind(sockSer,(sockaddr *)&addressSrv, sizeof(addressSrv)) == -1)
        {
            cout<<"Bind failed"<<endl;
            LogSet("Bind failed");
        }
    else
        {
            cout<<"Bind success"<<endl;
            LogSet("Bind success");
        }

    // 监听连接
    if(listen(sockSer, 5) == -1)
    {
        cout<<"Listen failed"<<endl;
        LogSet("Listen failed");
    }
        
    else
        {
            cout<<"Listen success"<<endl;
            LogSet("Listen success");
        }


    //创建线程监听服务器退出指令
    HANDLE ListenhThread = CreateThread(
                NULL, // 默认安全属性
                0,    // 默认堆栈大小
                ListenExit, // 线程函数
                NULL, // 传递给线程函数的参数
                0,    // 创建立即运行的线程
                NULL  );

    sockaddr_in  clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    HANDLE hThread;
    TF=true;
    while(1){
        if(TF==false) break;
        if(MaxClientNum==ClientSocketList.size()) {
            //超出了最大连接数，等待一段时间再次尝试
            Sleep(1000); // 等待一段时间再次尝试
            continue; // 跳过当前循环的剩余部分
        }
        SOCKET sockConnection = accept(sockSer, (SOCKADDR*)&clientAddr, &clientAddrSize);
        if(sockConnection == INVALID_SOCKET){
            if(TF==false) break;
            int error = WSAGetLastError();
            if(error == WSAEWOULDBLOCK){
                // 没有新的连接，稍后重试
                Sleep(100); // 等待一段时间再次尝试
                continue; // 跳过当前循环的剩余部分
            }else{
                // 输出其他错误信息
                printf("Accept failed with error: %d\n", error);
                LogSet("Accept failed with error: "+to_string(error));
                break; // 退出循环
            }
        }
        else
            {
            hThread = CreateThread(
                NULL, // 默认安全属性
                0,    // 默认堆栈大小
                ThreadFunction, // 线程函数
                LPVOID(sockConnection), // 传递给线程函数的参数
                0,    // 创建立即运行的线程
                NULL  
            );
            if (hThread == NULL) {
                printf("Create thread failed\n");
                LogSet("Create thread failed");
            }
            CloseHandle(hThread);
            }
    
    }
    Sleep(2000);///等待线程结束
    WSACleanup();
    cout<<"Exit Success"<<endl;
    LogSet("Exit Success");
    LogSet("Server End");
    return 0;
}