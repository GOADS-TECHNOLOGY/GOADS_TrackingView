#include <zmq.hpp>
#include <string>
#include <iostream>

int main () {
    // Prepare our context and subscriber
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    subscriber.connect("tcp://localhost:140799");
    subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);

    while (true) {
        zmq::message_t update;
        int count;

        // Receive a message
        subscriber.recv(&update);

        // Unpack the message
        std::istringstream iss(static_cast<char*>(update.data()));
        iss >> count;

        // Print out the count
        std::cout << "Received count: " << count << std::endl;
    }
    return 0;
}
