
#include <iostream>
#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <fstream>


using namespace std;

#define MAX_BUFFER_SIZE 1024

#define TIMEOUT 1000 // 超时时间，单位为毫秒


int msgseq; //全局变量，用于记录下一个要发送的序列号
int expectedseq; //全局变量，用于记录下一个期望接收的序列号
bool IsSendACK = false; //全局变量，用于传递是否要发送ACK的信号
bool IsSendFINACK =false; //全局变量，用于传递是否要发送FINACK的信号
DWORD startTime = 0; // 记录开始时间
string filename; // 文件名
bool IsThreadOver = false; // 线程是否结束
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // 获取控制台句柄


//数据包结构
struct Packet {
    char flag; // 标志位，用于控制连接建立和断开----0001：建立连接-SYN，  0010：确认数据-ACK， 0100：发送数据，1000:断开连接-FIN  0x21：空包--!  00010000(0x10):文件名    11111111(0xff):表示数据发送完毕
    DWORD SendIP, RecvIP; // 发送和接收的IP地址
    u_short SendPort, RecvPort; // 发送和接收的端口号
    int msgseq; // 消息序列号，用于确认和重传
    int ackseq; // 确认序列号，确认收到的消息
    int filelength; // 文件长度
    u_short checksum; // 校验和，用于差错检测
    char msg[MAX_BUFFER_SIZE]; // 数据内容
};
// 计算 Internet 校验和
uint16_t checksum(void *b, int len) {
    uint32_t sum = 0;
    uint16_t *buf = (uint16_t *)b;

    while (len > 1) {    //累加所有16位字
        sum += *buf++;
        len -= 2;
    }
    if (len == 1) {   // 处理最后一个字节
        sum += *(uint8_t *)buf;
    }    
    while (sum >> 16) {    // 处理进位
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return ~sum;   // 返回反码
}

// 计算 Packet 的校验和
bool calculatePacketChecksum(Packet &packet) {
    // 计算校验和
    uint16_t cs = checksum(&packet, sizeof(Packet));
    if(cs == 0x0000) return true;
    else return false;
}

//三次握手建立连接---接收SYN
Packet ThreeHand_1(SOCKET socket, struct sockaddr_in &clientAddr){
    bool SYN_Received = false;
    Packet receivedSYN;
    int len = sizeof(clientAddr);
    while(!SYN_Received){
        int result = recvfrom(socket, (char*)&receivedSYN, sizeof(receivedSYN), 0, (struct sockaddr*)&clientAddr, &len);
        if(result == SOCKET_ERROR){   //没有数据可读
            continue;
        }
        //这里验证标志位SYN是否正确
        if(receivedSYN.flag == 0x01){
            /////这里验证校验和是否正确
            if(calculatePacketChecksum(receivedSYN)==false){
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Checksum Error]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"Waiting for a new SYN packet."<<endl;
                continue;
            }
            else
                SYN_Received = true;
        }
        else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[Error Package]";
            SetConsoleTextAttribute(hConsole, 0x07);
            cout<<"ReceivedSYN.flag"<<receivedSYN.flag<<endl;
            continue;
        }
        
    }
    return receivedSYN;
}
/// 三次握手建立连接---发送SYN-ACK
void ThreeHand_2(SOCKET socket, struct sockaddr_in clientAddr,int syn_msgSEQ,int msgseq){
    Packet synAck;
    synAck.flag = 0x03; // SYN-ACK
    synAck.msgseq = msgseq;
    synAck.ackseq = syn_msgSEQ;
    synAck.SendIP = inet_addr("127.0.0.1");
    synAck.RecvIP = inet_addr("127.0.0.1");
    synAck.SendPort = htons(8078);
    synAck.RecvPort = htons(clientAddr.sin_port);
    synAck.filelength = 0;
    synAck.checksum = 0;
    memset(synAck.msg, 0, sizeof(synAck.msg));
    synAck.checksum = checksum(&synAck, sizeof(synAck));
    while(true){
        int result = sendto(socket, (char*)&synAck, sizeof(synAck), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        if(result == SOCKET_ERROR){
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[Send SYN-ACK Error]"<<endl;
            continue;
        }
        else return;
    }
    
}
/// 三次握手建立连接---接收ACK
Packet ThreeHand_3(SOCKET socket, struct sockaddr_in clientAddr,int ACK_SEQ){
    bool ACK_Received = false;
    Packet receivedACK;
    int len = sizeof(clientAddr);
    DWORD startTime = GetTickCount();
    while(!ACK_Received){
        if(GetTickCount() - startTime > TIMEOUT){
            receivedACK.flag = 0x21; // 超时标志
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[ACK Received Timeout]"<<endl;
            return receivedACK;
        }
        int result = recvfrom(socket, (char*)&receivedACK, sizeof(receivedACK), 0, (struct sockaddr*)&clientAddr, &len);
        if(result == SOCKET_ERROR){   //没有数据可读
            continue;
        }
        //这里验证标志位ACK及序列号是否正确
        if(receivedACK.flag == 0x02 && receivedACK.ackseq == ACK_SEQ){
            /////这里验证校验和是否正确
            if(calculatePacketChecksum(receivedACK)==false){
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Checksum Error]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"Waiting for a new ACK packet."<<endl;
                continue;
            }
            else ACK_Received = true;
            
        }
        else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
             cout<<"[ERROR Package]"<<endl;
        }
        
 
    }
    return receivedACK;
}

// 四次挥手断开连接---接收FIN
Packet FourHand_1(SOCKET socket, struct sockaddr_in clientAddr,int expectedseq){

    bool FIN_Received = false;
    Packet receivedFIN;
    int len = sizeof(clientAddr);
    DWORD startTime = GetTickCount();
    while(!FIN_Received){
        if(GetTickCount() - startTime > TIMEOUT){
            receivedFIN.flag = 0x21; // 超时标志
            
            return receivedFIN;
        }
        int result = recvfrom(socket, (char*)&receivedFIN, sizeof(receivedFIN), 0, (struct sockaddr*)&clientAddr, &len);
        if(result == SOCKET_ERROR){   //没有数据可读
            continue;
        }
        //这里验证标志位是否正确
        if(receivedFIN.flag == 0x08 && receivedFIN.msgseq == expectedseq){
            /////这里验证校验和是否正确
            if(calculatePacketChecksum(receivedFIN)==false){
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Checksum Error]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"Waiting for a new FIN packet."<<endl;
                continue;
            }
            else FIN_Received = true;
            
        }
        else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
             cout<<"[ERROR Package]";
             SetConsoleTextAttribute(hConsole, 0x07);
             cout<<"ReceivedPacket.msgseq"<<receivedFIN.msgseq<<"   ExpectedPacket.msgseq"<<expectedseq<<endl;
        }
        
 
    }
    return receivedFIN;
}
// 四次挥手断开连接---发送ACK
void FourHand_2(SOCKET socket, struct sockaddr_in clientAddr,int msgseq,int ACK_seq){
    Packet ACK;
    ACK.flag = 0x02; // ACK
    ACK.msgseq = msgseq;
    ACK.ackseq = ACK_seq;  //无Ack，默认为0
    ACK.SendIP = inet_addr("127.0.0.1");
    ACK.RecvIP = inet_addr("127.0.0.1");
    ACK.SendPort = htons(8078);
    ACK.RecvPort = htons(clientAddr.sin_port);
    ACK.filelength = 0; 
    ACK.checksum = 0;
    memset(ACK.msg, 0, sizeof(ACK.msg));
    ACK.checksum = checksum(&ACK, sizeof(ACK));

    while(true){
        int result = sendto(socket, (char*)&ACK, sizeof(ACK), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        if(result == SOCKET_ERROR){
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[Error in sending FIN]"<<endl;
            continue;
        }
        else{
            break;
        }
    }
}

// 四次挥手断开连接---发送FIN-ACK
void FourHand_3(SOCKET socket, struct sockaddr_in clientAddr,int msgseq,int ACK_seq){
    Packet FINACK;
    FINACK.flag = 0x0A; // FINACK
    FINACK.msgseq = msgseq;
    FINACK.ackseq = ACK_seq;  //无Ack，默认为0
    FINACK.SendIP = inet_addr("127.0.0.1");
    FINACK.RecvIP = inet_addr("127.0.0.1");
    FINACK.SendPort = htons(8078);
    FINACK.RecvPort = htons(clientAddr.sin_port);
    FINACK.filelength = 0; 
    FINACK.checksum = 0;
    memset(FINACK.msg, 0, sizeof(FINACK.msg));
    FINACK.checksum = checksum(&FINACK, sizeof(FINACK));

    while(true){
        int result = sendto(socket, (char*)&FINACK, sizeof(FINACK), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        if(result == SOCKET_ERROR){
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[Error in sending FIN]"<<endl;
            continue;
        }
        else{
            break;
        }
    }
}
//四次挥手断开连接---接收ACK
Packet FourHand_4(SOCKET socket, struct sockaddr_in clientAddr,int msgseq,int ACK_seq){
    bool ACK_Received = false;
    Packet receivedACK;
    int len = sizeof(clientAddr);
    DWORD startTime = GetTickCount(); //记录开始时间
    while(!ACK_Received){
        if(GetTickCount() - startTime > TIMEOUT){ //超时
            receivedACK.flag = 0x21;
            return receivedACK;
        }
        int result = recvfrom(socket, (char*)&receivedACK, sizeof(receivedACK), 0, (struct sockaddr*)&clientAddr, &len);
        if(result == SOCKET_ERROR){  //没有数据可读
            continue;
        }
        //这里验证ACK及序列号是否正确
        if(receivedACK.flag == 0x02 && receivedACK.ackseq == ACK_seq && receivedACK.msgseq == msgseq){
            ////验证校验和
            if(calculatePacketChecksum(receivedACK) == true){
                ACK_Received = true;
            }
            else{
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Checksum Error]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"Checksum:"<<receivedACK.checksum<<endl;
            }
            
        }
        else{
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[ERROR PACKAGE]";
            SetConsoleTextAttribute(hConsole, 0x07);
            cout<<"ReceivedPacket.msgseq"<<receivedACK.msgseq<<endl;
            continue;
        }
        
    }
    return receivedACK;
}

// 保存数据包到文件
void SavePacketToFile(const Packet& packet, const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "ab"); // 以追加和二进制模式打开文件
    if (!file) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        std::cerr << "[Failed to open file for writing.]" << std::endl;
        return;
    }
    size_t bytesWritten = fwrite(packet.msg, sizeof(char), packet.filelength, file);//strlen(packet.msg)
    if (bytesWritten < strlen(packet.msg)) {
        // 如果写入的字节数小于要写入的字节数，可能发生了错误
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        std::cerr << "[Error writing to file.]" << std::endl;
    }
    fclose(file); // 关闭文件
}


struct ThreadParams{
    SOCKET socket;
    struct sockaddr_in clientAddr;
};
DWORD WINAPI ReceiveDataThread(LPVOID lpParam) {     //用于接收数据
    ThreadParams params = *(ThreadParams*)lpParam;
    SOCKET socket = params.socket;
    struct sockaddr_in clientAddr = params.clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    u_long mode = 1; // 非阻塞模式
    // 设置非阻塞模式
    ioctlsocket(socket, FIONBIO, &mode);

    Packet receivedPacket;
    char buffer[MAX_BUFFER_SIZE];

    while (true) {
        if(IsThreadOver == true) break;
        if(GetTickCount() - startTime > TIMEOUT){ //超时
            IsSendACK = true;  //IsSendACK设置为true后会重发ACK
            startTime = GetTickCount();//重置开始时间
        }

        int result = recvfrom(socket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (result == SOCKET_ERROR) {   //表示没数据可以接收
            
        } else {
            // 处理接收到的数据
            //首先计算校验和，错了丢弃，对了继续
            if(calculatePacketChecksum(receivedPacket) == false){
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Checksum Error]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"Checksum:"<<receivedPacket.checksum<<endl;
                continue;
            }
            //到这里的话，标志位是对的，根据标志位判断包的类型---
            //同时，收到数据的话还需要取消定时器---在这里就通过将starttime设置为负数的方式来实现
            switch(receivedPacket.flag){

                case 0x04: //Data
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                    cout<<"[Data Received]"<<endl;
                    //接收到了数据包，说明服务端的上一个发的ACk成功收到了，因此发送序号可以增加
                    //这里需要更新序列号，但在更新序列号前，需要判断是否需要重传------------------
                    if(receivedPacket.msgseq != expectedseq){    //真正收到的与期望收到的不一致，需要重发ACK
                        IsSendACK = true;  //IsSendACK设置为true后会重发ACK
                        continue;   //接着等待
                    }
                    //否则不需要重传

                    //这一部分为处理收到的数据的代码
                    SavePacketToFile(receivedPacket, "./COPY-"+filename); // 保存数据包到文件

                    expectedseq = receivedPacket.msgseq + 1; //更新期望的序列号
                    //更新msgseq
                    msgseq = msgseq + 1;/////因为收到了数据包说明上一个ACK已经发送成功了
                    startTime = -1; //取消定时器
                    IsSendACK = true;
                break;

                case 0x08: //FIN
                    //收到FIN包，结束接收和发送线程，回到主线程完成四次挥手
                    
                    expectedseq = receivedPacket.msgseq + 1; //更新期望的序列号

                    //更新msgseq
                    msgseq = msgseq + 1;/////因为收到了数据包说明上一个ACK已经发送成功了
                    IsThreadOver = true;
                
                break;

                case 0x10: //文件名
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                    cout<<"[Filename Received]"<<endl;
                    //接收到了数据包，说明服务端的上一个发的ACk成功收到了，因此发送序号可以增加
                    //这里需要更新序列号，但在更新序列号前，需要判断是否需要重传------------------
                    if(receivedPacket.msgseq != expectedseq){    //真正收到的与期望收到的不一致，需要重发ACK
                        IsSendACK = true;  //IsSendACK设置为true后会重发ACK
                        continue;   //接着等待
                    }
                    //否则不需要重传

                    //这一部分为处理收到的数据的代码
                    filename = receivedPacket.msg; // 保存文件名

                    expectedseq = receivedPacket.msgseq + 1; //更新期望的序列号
                    //更新msgseq
                    msgseq = msgseq + 1;/////因为收到了数据包说明上一个ACK已经发送成功了
                    startTime = -1; //取消定时器
                    IsSendACK = true;

                break;

                default:
                break;
            }
            
        }
        
    }

    return 0;
}
Packet CreateACKPacket(int msgseq, int ackseq,ThreadParams* params){
    SOCKET socket = params->socket;
    struct sockaddr_in clientAddr = params->clientAddr;

    Packet ACK;
    ACK.flag = 0x02; // ACK标志位
    ACK.msgseq = msgseq; // 序列号，发完就加
    ACK.ackseq = ackseq; // 确认序列号
    ACK.SendIP = inet_addr("127.0.0.1"); // 发送者的IP地址
    ACK.RecvIP = inet_addr("127.0.0.1"); // 接收者的IP地址
    ACK.SendPort = htons(8078); // 发送者的端口号
    ACK.RecvPort = clientAddr.sin_port; // 接收者的端口号
    ACK.checksum = 0; // 校验和，根据实际情况计算
    memset(ACK.msg, 0, sizeof(ACK.msg)); // 数据内容初始化为空
    ACK.checksum = checksum(&ACK, sizeof(ACK)); // 计算校验和

    return ACK;

}

DWORD WINAPI SendDataThread(LPVOID lpParam) {
    // 从 lpParam 中取出参数
    ThreadParams* params = (ThreadParams*)lpParam;
    SOCKET socket = params->socket;
    struct sockaddr_in clientAddr = params->clientAddr;
    while(true){   //ACK需要超时重传
        if(IsThreadOver == true)break;
        if(IsSendACK == true){
            // 准备要发送的数据包
            Packet packet = CreateACKPacket(msgseq, expectedseq,params);
            // 发送数据
            int result = sendto(socket, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
            startTime =  GetTickCount(); // 记录开始时间
            if (result == SOCKET_ERROR) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout << "[Send failed]error: ";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout << WSAGetLastError() << endl;
            } else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                cout << "[ACK Send]" ;
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"    msgseq:"<<msgseq<< "   ACKSeq:"<<expectedseq<<endl;
            }
            IsSendACK = false;
        }
        
    }
    return 0;
}

int main() {

    //SetConsoleOutputCP(65001); // 设置控制台输出为UTF-8编码

    WSADATA wsaData;
    SOCKET socket;
    struct sockaddr_in serverAddr,clientAddr;
    int result;
    u_long mode = 1; // 非阻塞模式
    int seq;

    // 初始化WinSock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    // 创建UDP套接字
    socket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 设置非阻塞模式
    ioctlsocket(socket, FIONBIO, &mode);

    // 设置服务器地址信息
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8078); // 绑定到端口8078
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 绑定到所有可用接口

    // 绑定套接字
    result = bind(socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(socket);
        WSACleanup();
        return 1;
    }

    printf("UDP server bound to port %d\n", ntohs(serverAddr.sin_port));

    cout<<"Waiting for connection..."<<endl;

    //初始化序列号
    msgseq = 0;////这里会一直等直到收到SYN包
    Packet synPacket = ThreeHand_1(socket, clientAddr);
    expectedseq = synPacket.msgseq+1;//ACK序列号
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    cout<<"[SYN Received]"<<endl;

    Packet AckPacket;
    //等待接收SYN包
    while(true){
        //发送SYN-ACK
        ThreeHand_2(socket, clientAddr, expectedseq, msgseq);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        cout<<"[SYN-ACK Send]"<<endl;
        
        //等待接收ACK
        AckPacket = ThreeHand_3(socket, clientAddr,msgseq+1);
        if(AckPacket.flag == 0x21){   //空包，表示超时，需要重传----超时会重新发送SYN-ACK
            continue;
        }
        else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            cout<<"[ACK Received]"<<endl;
            break;
        }
    }
    Sleep(4000);//睡眠4s
    msgseq++;
    expectedseq = AckPacket.msgseq+1; //期望收到的下一个数据包的序列号
    cout<<"-----Connection Established!!!--------------"<<endl;
    ///////Ack序列号，每次在成功接收到数据后+1
    //////seq序列号，每次在收到ACK后+1
    
    //////从这里开始应该新建一个线程，用于接收数据
   
    // 创建线程参数结构体
    ThreadParams params;
    params.socket = socket; 
    params.clientAddr = clientAddr;

    // 创建线程
    HANDLE hThread = CreateThread(NULL, 0, ReceiveDataThread, &params, 0, NULL);

    //创建另一个线程用于发送ACK
    HANDLE hThread2 = CreateThread(NULL, 0, SendDataThread, &params, 0, NULL);
    
    cout<<"Thread created Success!!!"<<endl;
    while(true){
        if(IsThreadOver == true)break;
    }
    

    //四次挥手关闭连接
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    cout<<"[FIN Received]"<<endl;

    while(true){
        //发送ACK包
        FourHand_2(socket, clientAddr,msgseq,expectedseq);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        cout<<"[ACK Send]"<<endl;
        cout<<"Waiting for 2 seconds to ensure that the ACK package has been received!!!"<<endl;

        //等待接收FIN包
        Packet finPacket = FourHand_1(socket, clientAddr,expectedseq-1);
        if(finPacket.flag == 0x21){   //空包，表示超时---表示没有收到重复FIN
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            cout<<"[Ensure->ACK Received]"<<endl;
            break;
        }
        else {
            //2s内收到了FIN包，需要重传ACK包
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[Receive FIN Package Again]Preparing to resend ACK Package!!!"<<endl;
            continue;
        }
    }
    Packet finackPacket;
    //发送FIN-ACK包并等待接收ACK包
    while(true){
        //发送FIN-ACK包
        FourHand_3(socket, clientAddr,msgseq,expectedseq);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        cout<<"[FIN-ACK Send]"<<endl;

        //等待接收ACK包
        finackPacket = FourHand_4(socket, clientAddr,expectedseq,msgseq+1);
        if(finackPacket.flag == 0x21){   //空包，表示超时
            continue;
        }
        else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            cout<<"[FinalACK Received]"<<endl;
            break;
        }
    }
    SetConsoleTextAttribute(hConsole, 0x07);
    cout<<"Connection Closed Successfully!!!"<<endl;
    
    // 清理
    closesocket(socket);
    WSACleanup();
    return 0;
}