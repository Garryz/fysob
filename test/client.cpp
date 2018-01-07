#include <iostream>
#include <third_party/asio.hpp>

using asio::ip::tcp;

enum { max_length = 1024 };

int main(int argc, char* argv[])
{
    try {
        if (argc != 3) {
            std::cerr << "Usage: client <host> <port> \n";
            return 1;
        }

        asio::io_service io_service;

        tcp::socket s(io_service);
        tcp::resolver resolver(io_service);
        asio::connect(s, resolver.resolve({argv[1], argv[2]}));

        char request[max_length];
        while (true) {
            std::cout << "Enter messsge: ";
            std::cin.getline(request, max_length);
            size_t request_length = std::strlen(request);
            asio::write(s, asio::buffer(request, request_length));
            asio::read(s, asio::buffer(request, 1));
            std::cout << "Receive message: " << request << std::endl;
       }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
    }

    return 0;
}
