#include<iostream>
#include<winsock2.h>
#include <ws2tcpip.h>
#include<string.h>
#include<ctime>
#include<fstream>
#include <limits> // 包含 std::numeric_limits 的定义
using namespace std;
ofstream outfile("./ClientLog.txt",std::ios::app);
std::time_t now;
std::tm* local_now;
string name;
bool IsOver = false;
SOCKET sockClient; // 全局变量，用于保存客户端套接字
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE hEvent;  // 事件句柄
void LogSet(string name){
    now = std::time(0);  // 获取当前时间
    local_now = std::localtime(&now);  // 转换为本地时间
    outfile<<asctime(local_now);
    outfile<<name<<endl;
}
DWORD WINAPI ThreadFunction(LPVOID lpParam) {

    SOCKET ClientSocket =(SOCKET)lpParam;
    char  recvBuf[50];
    memset(recvBuf,0,sizeof(recvBuf));   
    int recvSize;
    
while(true){
    
    if(IsOver == true)break;
    // 接收数据并输出
    recvSize=recv(ClientSocket,recvBuf,sizeof(recvBuf),0);
    
    if(recvSize == SOCKET_ERROR){
        cout<<WSAGetLastError()<<endl;
        LogSet("ERROR"+to_string(WSAGetLastError()));
        // 重置字体颜色为默认值
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        // 关闭套接字
        closesocket(ClientSocket);
        WSACleanup();
        LogSet("Socket closed Successfully");
        exit(1);
    }
    
    else{
        recvBuf[recvSize] = '\0'; // 确保字符串以NULL结尾
        // 重置字体颜色为默认值
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        cout<<endl;
        if(strcmp(recvBuf, "OK! I got it") == 0){
            IsOver = true;
            send(sockClient, "SuccessExit!", strlen("SuccessExit!"), 0);
            closesocket(ClientSocket);
            continue;
        }
        
        if(strcmp(recvBuf, "Server will be closed in 5 seconds") == 0){
            
            cout<<"Server will be closed in 5 seconds"<<endl;
            LogSet("Server will be closed in 5 seconds");
            recv(ClientSocket,recvBuf,sizeof(recvBuf),0);
            cout<<recvBuf[0]<<endl;
            recv(ClientSocket,recvBuf,sizeof(recvBuf),0);
            cout<<recvBuf[0]<<endl;
            recv(ClientSocket,recvBuf,sizeof(recvBuf),0);
            cout<<recvBuf[0]<<endl;
            recv(ClientSocket,recvBuf,sizeof(recvBuf),0);
            cout<<recvBuf[0]<<endl;
            recv(ClientSocket,recvBuf,sizeof(recvBuf),0);
            cout<<recvBuf[0]<<endl;
             for(int i=0;i<50;i++){
                 recvBuf[i] = ' ';
            }
            recv(ClientSocket,recvBuf,sizeof(recvBuf),0);
            cout<<recvBuf<<endl;
            LogSet(recvBuf);

            if(send(ClientSocket, (name+": have exited!").c_str(), strlen((name+": have exited!").c_str()), 0)== SOCKET_ERROR){
                cout<<"Client have exited!--Send failed"<<endl;
                LogSet("Client have exited!--Send failed");
            }
            //关闭套接字
            closesocket(ClientSocket);
            WSACleanup();
            cout<<"Socket closed Successfully"<<endl;
            cout<<"Client Exit Successfully"<<endl;
            LogSet("Client Exit Successfully");
            exit(0);
        }
        cout<<recvBuf<<endl;
        LogSet(recvBuf);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        cout<<name<<":";
        LogSet(name+":");
    }

    }
    // 完成工作后设置事件
    //cout<<"SetEventSuccess"<<endl;
    SetEvent(hEvent);
    return 0;
}
int main()
{
    LogSet("Client Start");
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
L1:    
    cout<<"Please input your name:"<<endl;
    cin>>name;
    if(strlen(name.c_str())>10){
        cout<<"Name is too long"<<endl;
        goto L1;
    }
     // 清空 cin 的缓冲区
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 丢弃缓冲区中的字符
    std::cin.clear(); // 清除错误标志

    // 创建套接字
    sockClient = socket(AF_INET, SOCK_STREAM, 0);
    if(sockClient == -1)
        {
            cout<<"Socket creation failed"<<endl;
            cout<<WSAGetLastError()<<endl;
            LogSet("Socket creation failed");
            exit(0);
        }
    else
        {
            cout<<"Socket created successfully"<<endl;
            LogSet("Socket created successfully");
        }
    
    // 定义地址结构--服务端
    sockaddr_in addressSrv;
    addressSrv.sin_family = AF_INET; // 使用IPv4
    addressSrv.sin_addr.s_addr = inet_addr("127.0.0.1"); // 使用本地回环地址
    addressSrv.sin_port = htons(8087); // 端口号，htons确保端口字节序正确
    
    // 连接服务器
    if(connect(sockClient, (sockaddr*)&addressSrv, sizeof(addressSrv)) == SOCKET_ERROR)
        {
            cout<<"Connection failed"<<endl;
            cout<<WSAGetLastError()<<endl;
            LogSet("Connection failed.Error Code:"+to_string(WSAGetLastError()));
            closesocket(sockClient);
            WSACleanup();
            LogSet("Socket closed Successfully");
        }
    else
        {
            cout<<"Connected to server"<<endl;
            LogSet("Connected to server");
        }

    send(sockClient, (name+" Enter.").c_str(), strlen(name.c_str())+7, 0);
    cout<<endl;
    cout<<endl;
    cout<<endl;
    
    //建立线程接收数据
    HANDLE hThread = CreateThread(
                NULL, // 默认安全属性
                0,    // 默认堆栈大小
                ThreadFunction, // 线程函数
                LPVOID(sockClient), // 传递给线程函数的参数
                0,    // 创建立即运行的线程
                NULL  // 不需要线程ID
            );
            if (hThread == NULL) {
                printf("Create thread failed\n");
                LogSet("Create thread failed");
            }
            CloseHandle(hThread);

    hEvent = CreateEvent(
        NULL,                // 默认安全性
        TRUE,                // 手动重置事件
        FALSE,               // 初始状态为无信号
        NULL                 // 匿名事件
    );

    char sendBuf[50];
    memset(sendBuf,0,sizeof(sendBuf)); 
    string buffer;

    cout<<"Press './exit' to exit"<<endl;
while(true){

    for(int i=0;i<50;i++){
        sendBuf[i]=0;
    }
    // 设置字体颜色为绿色
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    cout<<name<<":";
    getline(cin, buffer);
    if(strcmp(buffer.c_str(), "./exit") == 0){
        if(send(sockClient, (name+" Exit").c_str(), strlen(name.c_str())+6, 0) == SOCKET_ERROR)
                {
                    cout<<"Send failed"<<endl;
                    LogSet("Message:"+(name+" Exit")+"Send failed");
                }
        ///睡眠以等待发送成功
        Sleep(1000);
        break;
    }
    for(int i=0;i<strlen(name.c_str());i++)
        {
            sendBuf[i]=name[i];
        }
    sendBuf[strlen(name.c_str())]=':';
    for(int t=strlen(name.c_str())+1;t<strlen(name.c_str())+1+strlen(buffer.c_str());t++)
        {
            sendBuf[t]=buffer[t-strlen(name.c_str())-1];
        }
    
    if(send(sockClient, sendBuf, strlen(sendBuf)+1, 0)== SOCKET_ERROR)
        cout<<"Send failed"<<endl;
    else
        LogSet("Message:"+string(sendBuf)+"Send successfully");
    
}
    // 等待子线程结束
    WaitForSingleObject(hEvent, INFINITE);
    cout<<"Thread Over!"<<endl;
    LogSet("Thread Over!");
    // 关闭套接字及句柄
    CloseHandle(hEvent);
    closesocket(sockClient);
    WSACleanup();
    cout<<"Client closed Success"<<endl;
    LogSet("Client closed Success");
    return 0;
}