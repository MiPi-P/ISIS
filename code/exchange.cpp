#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>

using Clock = std::chrono::steady_clock;
void print_time(Clock::time_point start);

void check(int result, const char* operation) {
    if (result < 0) {
        std::perror(operation);
        std::exit(1);
    }
}
// argc - кол-во элементов командной строки вместе с именем программы
// argv - сами строки
int main(int argc, char** argv) {
    if (argc < 2 || argc > 3 || argv[1][0] < '0' || argv[1][0] > '2' || argv[1][1] != '\0') {
        std::fprintf(stderr, "Usage: ./isis_node <0|1|2> [local-ip]\n");
        return 1;
    }
    const unsigned char id = argv[1][0] - '0';
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    check(fd, "socket");

    sockaddr_in group{};
    group.sin_family = AF_INET;
    group.sin_port = htons(5000);
    inet_pton(AF_INET, "239.10.0.1", &group.sin_addr);

    ip_mreq membership{};
    membership.imr_multiaddr = group.sin_addr;
    if (inet_pton(AF_INET, argc == 3 ? argv[2] : "127.0.0.1", &membership.imr_interface) != 1) {
        std::fprintf(stderr, "Expected a local IPv4 address\n");
        return 1;
    }
    const int reuse = 1;
    check(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)), "SO_REUSEADDR");
    check(bind(fd, reinterpret_cast<const sockaddr*>(&group), sizeof(group)), "bind");
    check(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &membership.imr_interface,
                     sizeof(membership.imr_interface)), "IP_MULTICAST_IF");
    check(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership, sizeof(membership)), "IP_ADD_MEMBERSHIP");
    std::printf("Node %u joined\n", static_cast<unsigned>(id));
    std::fflush(stdout);

    for (int round = 0; round < 10; ++round) {
        const auto start = Clock::now();
        // Each turn is one state of the exchange: send or wait for that sender.
        for (unsigned char turn = 0; turn < 3; ++turn) {
            if (turn == id) {
                check(sendto(fd, &id, sizeof(id), 0,
                             reinterpret_cast<const sockaddr*>(&group), sizeof(group)), "sendto");
            } else {
                unsigned char sender = 255;
                int size;
                do {
                    size = recvfrom(fd, &sender, sizeof(sender), MSG_TRUNC, nullptr, nullptr);
                    check(size, "recvfrom");
                } while (size != 1 || sender != turn);
            }
        }
        print_time(start);
    }
    close(fd);
}
