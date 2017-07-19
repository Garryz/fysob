#include <iostream>
#include <engine/net/server.h>

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: server <port>\n";
            return 1;
        }

        using namespace std;
        using namespace engine;

        server s("127.0.0.1", atoi(argv[1]), 10);

        s.run();

        getchar();

        s.stop();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
