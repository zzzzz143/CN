//实验3：基于UDP服务设计可靠传输协议并编程实现

//3-1：利用数据报套接字在用户空间实现面向连接的可靠数据传输，功能包括：建立连接、差错检测、确认重传等。流量控制采用停等机制
//实验要求：
//1.实现单向传输
//2.给出详细的协议设计
//3.完成给定测试文件的传输，显示传输时间和平均吞吐率
//4.给出实现的拥塞控制算法的原理
//5.性能测试指标包括吞吐率、文件传输时延等，给出图形结果并进行分析

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
#define MAX_TIMEOUT 1000
#define RouterPort  8088


int msgseq; //全局变量，用于记录下一个要发送的序列号
int expectedseq; //全局变量，用于记录下一个期望接收的序列号
bool CanNext = false; //全局变量，用于传递是否可以发送下一个数据包的信号
bool IsSendFINACK =false; //全局变量，用于传递是否要发送FINACK的信号
bool IsTimeOut = false; //全局变量，用于传递是否超时的信号
DWORD startTime = 99999999; // 记录开始时间
bool IsThreadOver = false;
DWORD FileSize;
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // 获取控制台句柄

//数据包结构
struct Packet {
    char flag; // 标志位，用于控制连接建立和断开----0001：建立连接-SYN，0010：确认数据-ACK，0100：发送数据，1000:断开连接-FIN   0x21：空包--!  00010000:文件名  11111111(0xff):表示数据发送完毕
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

//三次握手建立连接---发送SYN
void ThreeHand_1(SOCKET socket, struct sockaddr_in serverAddr,int msgseq){
    //模拟三次握手建立连接
    Packet syn;
    syn.flag = 0x01; // SYN
    syn.msgseq = msgseq;
    syn.ackseq = 0;    //无Ack，默认为0
    syn.SendIP = inet_addr("127.0.0.1");
    syn.RecvIP = inet_addr("127.0.0.1");
    syn.SendPort = htons(0);
    syn.RecvPort = htons(RouterPort);
    syn.filelength = 0;
    syn.checksum = 0;
    memset(syn.msg, 0, sizeof(syn.msg));
    syn.checksum = checksum(&syn, sizeof(syn));
    sendto(socket, (char*)&syn, sizeof(syn), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    cout<<"[Checksum]";
    SetConsoleTextAttribute(hConsole, 0x07);
    cout<<syn.checksum<<endl;
}
/// 三次握手建立连接---接收SYN-ACK
Packet ThreeHand_2(SOCKET socket, struct sockaddr_in serverAddr,int syn_msgSEQ){
    bool SYN_ACK_Received = false;
    Packet receivedsynAck;
    int len = sizeof(serverAddr);
    DWORD startTime = GetTickCount(); // 记录开始时间
    while(!SYN_ACK_Received){
        if(GetTickCount() - startTime > MAX_TIMEOUT){    //超时了，应该重传SYN
            receivedsynAck.flag = 0x21;  //空包
            return receivedsynAck;
        }
        int result = recvfrom(socket, (char*)&receivedsynAck, sizeof(receivedsynAck), 0, (struct sockaddr*)&serverAddr, &len);
        if(result == SOCKET_ERROR){   //没有数据可读
            continue;
        }
        //这里验证ACK和SYN是否正确
        if(receivedsynAck.flag == 0x03 && receivedsynAck.ackseq == syn_msgSEQ){
            /////这里验证校验和是否正确
            if(calculatePacketChecksum(receivedsynAck)==false){
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Error Checksum]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"ReceivedPacket Checksum: "<<receivedsynAck.checksum<<endl;
                continue;
            }
            else {
                SYN_ACK_Received = true;
            }
            
        }
        else { 
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[Error Package]"<<endl;
        }
        
        
    }
    return receivedsynAck;
    
}
/// 三次握手建立连接---发送ACK
void ThreeHand_3(SOCKET socket, struct sockaddr_in serverAddr,int syn_seq,int synAck_msgseq){

    Packet ack;
    ack.flag = 0x02; // ACK
    ack.msgseq = syn_seq;
    ack.ackseq = synAck_msgseq;
    ack.SendIP = inet_addr("127.0.0.1");
    ack.RecvIP = inet_addr("127.0.0.1");
    ack.SendPort = htons(0);
    ack.RecvPort = htons(RouterPort);
    ack.filelength = 0;
    ack.checksum = 0;
    memset(ack.msg, 0, sizeof(ack.msg));
    ack.checksum = checksum(&ack, sizeof(ack));
    while(true){
        int result = sendto(socket, (char*)&ack, sizeof(ack), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if(result == SOCKET_ERROR){
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[Error in sending ACK]"<<endl;
            continue;
        }
        else{
            break;
        }
    }    
}


// 四次挥手断开连接---发送FIN
void FourHand_1(SOCKET socket, struct sockaddr_in serverAddr,int FIN_msgseq){

    Packet finRequest;
    finRequest.flag = 0x08; // FIN
    finRequest.msgseq = FIN_msgseq;
    finRequest.ackseq = 0;  //无Ack，默认为0
    finRequest.SendIP = inet_addr("127.0.0.1");
    finRequest.RecvIP = inet_addr("127.0.0.1");
    finRequest.SendPort = htons(0);
    finRequest.RecvPort = htons(RouterPort);
    finRequest.filelength = 0; 
    finRequest.checksum = 0;
    memset(finRequest.msg, 0, sizeof(finRequest.msg));
    finRequest.checksum = checksum(&finRequest, sizeof(finRequest));

    while(true){
        int result = sendto(socket, (char*)&finRequest, sizeof(finRequest), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
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
// 四次挥手断开连接---接收ACK
Packet FourHand_2(SOCKET socket, struct sockaddr_in serverAddr,int ACK_seq){
    bool ACK_Received = false;
    Packet receivedACK;
    int len = sizeof(serverAddr);
    DWORD startTime = GetTickCount(); //记录开始时间
    while(!ACK_Received){
        if(GetTickCount() - startTime > MAX_TIMEOUT){ //超时
            receivedACK.flag = 0x21;
            return receivedACK;
        }
        int result = recvfrom(socket, (char*)&receivedACK, sizeof(receivedACK), 0, (struct sockaddr*)&serverAddr, &len);
        if(result == SOCKET_ERROR){  //没有数据可读
            continue;
        }
        //这里验证ACK及序列号是否正确
        if(receivedACK.flag == 0x02 && receivedACK.ackseq == ACK_seq){
            ////验证校验和
            if(calculatePacketChecksum(receivedACK) == true){
                ACK_Received = true;
            }
            else{
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Checksum Error]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"ReceivedACK Checksum: "<<receivedACK.checksum<<endl;
            }
            
        }
        else{
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[ERROR PACKAGE]";
            SetConsoleTextAttribute(hConsole, 0x07);
            cout<<"   ReceivedACK: "<<receivedACK.ackseq<<"   ExpectedACK: "<<ACK_seq<<"   Receivedflag:"<<receivedACK.flag<<endl;
            continue;
        }
        
    }
    return receivedACK;
}


// 四次挥手断开连接---接收FIN-ACK-1010
Packet FourHand_3(SOCKET socket, struct sockaddr_in serverAddr,int msg_seq,int ACK_seq){
    bool FINACK_Received = false;
    Packet receivedFINACK;
    int len = sizeof(serverAddr);
    DWORD startTime = GetTickCount(); //记录开始时间
    while(!FINACK_Received){
        if(GetTickCount() - startTime > MAX_TIMEOUT){ //超时
            
            receivedFINACK.flag = 0x21;
            return receivedFINACK;
        }
        int result = recvfrom(socket, (char*)&receivedFINACK, sizeof(receivedFINACK), 0, (struct sockaddr*)&serverAddr, &len);
        if(result == SOCKET_ERROR){  //没有数据可读
            continue;
        }
        //这里验证ACK及序列号是否正确
        if(receivedFINACK.flag == 0x0A && receivedFINACK.ackseq == ACK_seq && receivedFINACK.msgseq == msg_seq){
            ////验证校验和
            if(calculatePacketChecksum(receivedFINACK) == true){
                FINACK_Received = true;
            }
            else{
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Checksum Error]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"receivedFINACK Checksum: "<<receivedFINACK.checksum<<endl;
            }
            
        }
        else{
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[ERROR PACKAGE]";
            SetConsoleTextAttribute(hConsole, 0x07);
            cout<<"   receivedFINACK: "<<receivedFINACK.ackseq<<"   ExpectedACK: "<<ACK_seq<<"   Receivedflag:"<<receivedFINACK.flag<<endl;
            continue;
        }
        
    }
    return receivedFINACK;
}
//四次挥手断开连接---发送ACK
void FourHand_4(SOCKET socket, struct sockaddr_in serverAddr,int msgseq,int FIN_ACK_seq){
    Packet ACK;
    ACK.flag = 0x02; // ACK
    ACK.msgseq = msgseq;
    ACK.ackseq = FIN_ACK_seq;  //无Ack，默认为0
    ACK.SendIP = inet_addr("127.0.0.1");
    ACK.RecvIP = inet_addr("127.0.0.1");
    ACK.SendPort = htons(0);
    ACK.RecvPort = htons(RouterPort);
    ACK.filelength = 0; 
    ACK.checksum = 0;
    memset(ACK.msg, 0, sizeof(ACK.msg));
    ACK.checksum = checksum(&ACK, sizeof(ACK));

    while(true){
        int result = sendto(socket, (char*)&ACK, sizeof(ACK), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
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


struct ThreadParams{
    SOCKET socket;
    struct sockaddr_in ServerAddr;
};
DWORD WINAPI ReceiveACKThread(LPVOID lpParam) {     //用于不断地接收ACK
    ThreadParams params = *(ThreadParams*)lpParam;
    SOCKET socket = params.socket;
    struct sockaddr_in ServerAddr = params.ServerAddr;
    int serverAddrSize = sizeof(ServerAddr);

    u_long mode = 1; // 非阻塞模式
    // 设置非阻塞模式
    ioctlsocket(socket, FIONBIO, &mode);

    Packet receivedPacket;

    while (true) {
        if(IsThreadOver == true){
            break;
        }
        if(GetTickCount() - startTime > MAX_TIMEOUT){ //超时
            //需要重发数据包DataPacket
            IsTimeOut = true;
            startTime = GetTickCount();//重置开始时间 
            continue;  
        }
        
        int result = recvfrom(socket, (char*)&receivedPacket, sizeof(Packet), 0, (struct sockaddr*)&ServerAddr, &serverAddrSize);
        if (result == SOCKET_ERROR) {   //表示没数据可以接收
            //cout << "Receive failed with error: " << WSAGetLastError() << endl;
        } else {
            // 处理接收到的数据
            //首先计算校验和，错了丢弃，对了继续
            if(calculatePacketChecksum(receivedPacket)== false){
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Checksum Error]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"Checksum: "<<receivedPacket.checksum<<endl;
                continue;
            }
            //到这里的话，标志位是对的，根据标志位判断包的类型
            switch(receivedPacket.flag){
                case 0x02: //ACK------
                    //收到ACk包后，可以更新seq
                    //首先验证一下，这个包ACK的是不是我们上一个发送的数据包
                    if(receivedPacket.ackseq != msgseq+1){
                        //表示不是，需要重发
                        //通过手动超时的方式
                        IsTimeOut = true;
                        continue;
                    }
                    //到这个位置，表示是，可以发下一个了
                    startTime = GetTickCount();//重置定时器
                    //首先，收到ACK包说明之前发的数据包已经到达，现在可以发送下一个数据包
                    msgseq = receivedPacket.ackseq; //更新seq,receivedPacket.ackseq表示的是对方期望收到的下一个数据包的序列号
                    CanNext = true;  //可以发送下一个数据包了
                    expectedseq = receivedPacket.msgseq + 1; //更新期望收到的下一个数据包的序列号

                break;
                default:
                break;
            }
            
        }
    }

    return 0;
}

void readAndSplitFile(const char* filePath, std::vector<Packet>& packets,ThreadParams params) {

    FILE* file = fopen(filePath, "rb"); // 以二进制读取模式打开文件
    if (!file) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        std::cerr << "[Failed to open file]" << std::endl;
        return;
    } else {
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        std::cout << "[File Opened]" << std::endl;
    }

    char buffer[MAX_BUFFER_SIZE];
    int seqNum = msgseq + 1;

    // 先放进去msgseq个空包
    for (int i = 0; i < msgseq; i++) {
        Packet Data;
        Data.flag = 0x04; // Data数据
        Data.msgseq = -1; // 序列号，发完就加
        Data.ackseq = 0; // 确认序列号---这里不需要
        packets[i] = Data;
    }

    // 读取文件并分割成数据包
    while (true) {
        size_t bytesRead = fread(buffer, 1, MAX_BUFFER_SIZE, file); // 使用fread读取
        if (bytesRead == 0) break; // 如果读取0字节，表示到达文件末尾或发生错误

        Packet Data;
        Data.flag = 0x04; // Data数据
        Data.msgseq = seqNum; // 序列号，发完就加
        Data.ackseq = 0; // 确认序列号---这里不需要
        Data.SendIP = inet_addr("127.0.0.1"); // 发送者的IP地址
        Data.RecvIP = inet_addr("127.0.0.1"); // 接收者的IP地址
        Data.SendPort = htons(0); // 发送者的端口号
        Data.RecvPort = htons(RouterPort); // 接收者的端口号
        Data.filelength = bytesRead; // 文件长度
        Data.checksum = 0; // 校验和，根据实际情况计算
        memset(Data.msg, 0, sizeof(Data.msg)); // 数据内容初始化为空
        memcpy(Data.msg, buffer, bytesRead); // 复制数据内容
        Data.checksum = checksum(&Data, sizeof(Data)); // 计算校验和
        packets[seqNum++] = Data;

        FileSize += bytesRead;
    }

    fclose(file); // 关闭文件
}

int main(){
    
    //SetConsoleOutputCP(65001);//设置控制台输出编码为UTF-8
    cout<<"Client Start!!!"<<endl;
    WSADATA wsaData;
    u_long mode = 1; // 非阻塞模式
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }
    //创建UDP套接字
    SOCKET socket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    cout<<"UDP Create Success!!!"<<endl;



    // 设置非阻塞模式
    ioctlsocket(socket, FIONBIO, &mode);

    //定义服务器地址
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(RouterPort); // 服务器端口
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 服务器IP地址

    //初始化序列号
    msgseq = 0;

    
    Packet synAck;///接收SYN-ACK
    // 发送SYN
    while(true){
        ThreeHand_1(socket, serverAddr, msgseq);///序列号是0
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        cout<<"[SYN Send]"<<endl;
        SetConsoleTextAttribute(hConsole, 0x07);
        cout<<"Waiting for SYN-ACK package..."<<endl;
        
        // 等待SYN-ACK
        synAck = ThreeHand_2(socket, serverAddr, msgseq+1);
        if(synAck.flag == 0x21){  //空包，表示超时，需要重传
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[SYN-ACK Time Out]"<<endl;
            continue;//超时重传
        }
        else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            cout<<"[SYN-ACK Receive Success]"<<endl;
            expectedseq = synAck.msgseq+1;
            break;
        }
    }
    ///在成功建立连接后，服务端会Slepp6s，也就是说若超时后未收到重复的SYN-ACK包，说明连接建立成功;若收到重复SYN-ACK包，则说明需要重传ACK包
    
    
    msgseq++;
    while(true){
        ThreeHand_3(socket, serverAddr, msgseq, expectedseq);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        cout<<"[ACK Send Success]"<<endl;
        SetConsoleTextAttribute(hConsole, 0x07);
        cout<<"Waiting for 2 seconds to ensure that the connection is established..."<<endl;
        ///等待2s若未收到重复的SYN-ACK包，则认为连接建立成功
        // 等待SYN-ACK
        int Count = 0;
        while(true){
            Packet synAck2 = ThreeHand_2(socket, serverAddr, msgseq);
            if(synAck2.flag == 0x21){   //表示超时，超时2次就是2秒
                Count++;
                if(Count == 2)break;  
            }
            else {   //在2s内收到了SYN-ACK包，说明连接建立失败，需要重传ACK包
                break;
            }
        }
        if(Count == 2)break;
        else {
            continue;  //这种情况表示在2s内收到了SYN-ACK包，说明连接建立失败，需要重传ACK包
        }
    }
    cout<<"-----Connection Established!!!--------------"<<endl;
    msgseq++;///连接建立成功后，序列号再加1
   //---------------------------------------------------------------------------------------------- 


    // 创建线程参数结构体
    ThreadParams params;
    params.socket = socket; 
    params.ServerAddr = serverAddr;

    

    // 读取文件并分割成数据包
    std::vector<Packet> packets;

while(true){
    char FileName[256]; // 假设文件名不超过 255 个字符
    cout << "Please Input the File Name:(Press './exit' to exit)" << endl;
    cin.getline(FileName, 256); // 使用 getline 以允许空格

    if (strcmp(FileName, "./exit") == 0) {
        cout << "Exiting..." << endl;
        break;
    }
    const char* FileName2 = FileName;
    cout<<"FileName:"<<FileName2<<endl;

    packets.clear();
    packets.resize(30000);
    readAndSplitFile(FileName2, packets,params);

    ///packets[msgseq]存放一个只有文件名的包
    Packet filenamepacket;
    filenamepacket.flag = 0x10; // 文件名
    filenamepacket.msgseq = msgseq; // 序列号，发完就加
    filenamepacket.ackseq = 0; // 确认序列号---这里不需要
    filenamepacket.SendIP = inet_addr("127.0.0.1"); // 发送者的IP地址
    filenamepacket.RecvIP = inet_addr("127.0.0.1"); // 接收者的IP地址
    filenamepacket.SendPort = htons(0); // 发送者的端口号
    filenamepacket.RecvPort = htons(RouterPort); // 接收者的端口号
    filenamepacket.checksum = 0; // 校验和，根据实际情况计算
    memset(filenamepacket.msg, 0, sizeof(filenamepacket.msg)); // 数据内容初始化为空
    memcpy(filenamepacket.msg, FileName, sizeof(FileName)); // 复制数据内容
    filenamepacket.checksum = checksum(&filenamepacket, sizeof(filenamepacket)); // 计算校验和
    packets[msgseq]=(filenamepacket);

    // 创建一个线程用于接收ACK
    IsThreadOver = false;
    HANDLE hThread = CreateThread(NULL, 0, ReceiveACKThread, &params, 0, NULL);


   //接下来是不断地传输数据
    Packet packet;
    packet = packets[msgseq];
    

    DWORD FileTransmitTime = clock();

    while(true){

        if(CanNext == true){
            packet = packets[msgseq];
            CanNext = false;
            if(packet.flag == 0x00){
                //数据发送完毕，准备关闭连接
                packets.clear();
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"Data Send Finish!!!"<<endl;
                cout<<"FileName:"<<FileName2<<"   FileTransmitTime:"<<clock()-FileTransmitTime<<"ms"<<endl;
                cout<<"InOut Rate:"<<msgseq*1024*CLOCKS_PER_SEC/(clock()-FileTransmitTime)<<"byte/s"<<endl;
                IsThreadOver = true;
                break;
            }
            
        }

            // 发送数据包--可能需要重发
            int sendResult = sendto(socket, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            
            IsTimeOut = false;
            startTime = GetTickCount();//启动定时器
            if (sendResult == SOCKET_ERROR) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Data SendError]"<<endl;
                continue;
            }
            else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                cout<<"[Data Send]";
                SetConsoleTextAttribute(hConsole, 0x07);
                cout<<"Packet Seq:";
                cout<<"msgseq:"<<msgseq<<endl;
                cout<<packet.msgseq<<"CheckSum:"<<packet.checksum<<endl;
            }

        while(1){
            if(CanNext == true){
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                cout<<"[Preparing for Next Data]"<<endl;
                break;
            }
            else if(IsTimeOut == true){
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout<<"[Waiting for ACK Time Out]"<<endl;
                break;
            }
            
        }
        
    }
}
    
    //数据传输完毕，准备四次挥手关闭连接 

    // 发送FIN包并等待ACK包
    Packet ACKPacket;
    while(true){
        FourHand_1(socket,serverAddr,msgseq);
        //等待接收ACK包
        cout<<"[FIN Send]";
        cout<<"msgseq:"<<msgseq<<endl;
        cout<<"   Msg Seq:"<<msgseq<<endl;
        ACKPacket = FourHand_2(socket,serverAddr,msgseq+1);
        if(ACKPacket.flag == 0x21){ ////空包，表示超时，需要重传
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[ACK Recive Time Out]"<<endl;
            continue;
        }
        else{
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            cout<<"[ACK Recive Success]"<<endl;
            msgseq = msgseq+1;
            expectedseq = ACKPacket.msgseq+1;
            break;
        }
    }

    //等待接收FIN-ACK包
    Packet FINACKPacket;
    cout<<"Waiting for FIN-ACK Package!!!"<<endl;
    while(true){
        FINACKPacket = FourHand_3(socket,serverAddr,expectedseq,msgseq);
        if(ACKPacket.flag == 0x21){ ////空包，表示超时,这时候接着等待不需要重传
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout<<"[FIN-ACK Recive Time Out]"<<endl;
            continue;
        }
        else{   
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            cout<<"[FIN-ACK Recive Success]"<<endl;
            expectedseq = ACKPacket.msgseq+1;
            break;
        }
    
    }
    //回复一个ACK后关闭连接
    while(true){
        FourHand_4(socket,serverAddr,msgseq,expectedseq);
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        cout<<"[ACK Send]";
        cout<<"msgseq:"<<msgseq<<endl;
        cout<<"   Msg Seq:"<<msgseq<<endl;
        SetConsoleTextAttribute(hConsole, 0x07);
        cout<<"Waiting for 2 seconds to ensure that the ACK package has been received by the server!!!"<<endl;
        //等待2s，若未收到重复的的FIN-ACK包，则认为可以关闭连接
        //等待FIN-ACK
        int Count = 0;
        while(true){
            Packet FinAck = FourHand_3(socket,serverAddr,expectedseq-1,msgseq);
            if(FinAck.flag == 0x21){  //表示超时
                Count++;
                if(Count == 2)break;
            }
            else{   //表示在2s内收到了FIN-ACK包，说明ACK包未接收到，需要重传ACK包
                continue;
            }
        }
        if(Count == 2)break;
    }
    SetConsoleTextAttribute(hConsole, 0x07);
    cout<<"Client Close Successfully!!!"<<endl;
    // 关闭套接字并清理
    WSACleanup(); // 程序结束时清理WinSock
    return 0;

}