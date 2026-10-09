#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iomanip>
int id=0;
char opcode=0;
bool rd=0;
std::string createHeader (const char* query, int read)
{
    std::string ret;
    ret.resize(12);
    for (int i=0;i<2;i++)
    {
        for (int j=0;j<=7;j++)
            ret[i] |=(((1 << 15-8*i-j) & id) >> ((1-i)*8));
    }
    ret[2]=0b10000000;
    for (int i=0;i<=3;i++)
        ret[2] |=(1 << 3)*(opcode & (1 << i));
    ret[2] |=rd;
    ret[3]=0b00000000;
    ret[5]=0b00000001;
    ret[7]=0b00000001;
    if (opcode!=0)
        ret[3]=0b00000100;
    return ret;
}
std::string createQuestion (const char* query, int read)
{
    std::string ret="";
    for (int i=12;i<read;i++)
    {
        unsigned char ch = query[i];
        ret.append(1, ch);
    }
    return ret;
}
std::string createResponse (const char* query, int read)
{
    std::string ret="";
    for (int i=12;i<read;i++)
    {
        unsigned char ch = query[i];
        ret.append(1, ch);
    }
    ret.append(1, 0);
    ret.append(1, 0b00000001);
    ret.append(1, 0);
    ret.append(1, 0b00000001);
    ret.append(3, 0);
    ret.append(1, 0b01000011); // 67 (this time it actually is that)
    ret.append(1, 0);
    ret.append(1, 0b00000100);
    ret.append(4, '\x08');
    return ret;
}
void getInfo (const char *query, int read)
{
    id=opcode=rd=0;
    for (int i=0;i<2;i++)
    {
        for (int j=0;j<=7;j++)
        {
            id+=(1 << 8*(1-i))*((1 << j) & query[i]);
        }
    }
    for (int i=3;i<=6;i++)
    {
        opcode |=(query[2] & (1 << i)) >> 3;
    }
    rd=query[2] & 1;
}
int main() {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Disable output buffering
    setbuf(stdout, NULL);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here!" << std::endl;


    int udpSocket;
    struct sockaddr_in clientAddress;

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "..." << std::endl;
        return 1;
    }

    // Since the tester restarts your program quite often, setting REUSE_PORT
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "SO_REUSEPORT failed: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in serv_addr = { .sin_family = AF_INET,
                              .sin_port = htons(2053),
                              .sin_addr = { htonl(INADDR_ANY) },
                            };

    if (bind(udpSocket, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) != 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        return 1;
    }

    int bytesRead;
    char buffer[4096];
    socklen_t clientAddrLen = sizeof(clientAddress);
    while (true) {
        // Receive data
        bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddrLen);
        if (bytesRead == -1) {
            perror("Error receiving data");
            break;
        }
        getInfo(buffer, bytesRead);
        std::cout << id << "\n";
        buffer[bytesRead] = '\0';
        std::cout << "Received " << bytesRead << " bytes: " << buffer << std::endl;

        std::string response = createHeader(buffer, bytesRead)+createQuestion(buffer, bytesRead)+createResponse(buffer, bytesRead);
        // Send response
        if (sendto(udpSocket, response.c_str(), (ssize_t)response.size(), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }

    close(udpSocket);

    return 0;
}
