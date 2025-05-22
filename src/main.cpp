#include <boost/asio.hpp>
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <thread>
#include <vector>

#include "message.hpp"
#include "network.hpp"
#include "peer.hpp"

using boost::asio::ip::tcp;

template<typename M>
void send(const std::string &peer, const M &message) {
	try {
		boost::asio::io_context ctx;
		const auto pos = peer.find(':');
		if (pos == std::string::npos) {
			std::cerr << std::format("invalid peer ({})", peer) << std::endl;
			return;
		}
		const auto host = peer.substr(0, pos);
		auto port =
				static_cast<unsigned short>(std::stoi(peer.substr(pos + 1)));
		tcp::socket sock(ctx);
		sock.connect({boost::asio::ip::make_address(host), port});
		auto data = message.serialize() + '\n';
		boost::asio::write(sock, boost::asio::buffer(data));
	} catch (const std::exception &e) {
		std::cerr << std::format("send error to ({}): {}\n", peer, e.what())
				  << std::endl;
	}
}

void usage(const char *program) { std::cout << "usage" << std::endl; }

int main(int argc, char *argv[]) {
	std::string host;
	unsigned short port = 0;
	std::vector<std::string> peers;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--host" && i + 1 < argc) {
			host = argv[++i];
		} else if (arg == "--port" && i + 1 < argc) {
			port = static_cast<unsigned short>(std::stoi(argv[++i]));
		} else if (arg == "--peer" && i + 1 < argc) {
			peers.push_back(argv[++i]);
		} else {
			usage(argv[0]);
			return 1;
		}
	}

	if (host.empty() || port == 0) {
		usage(argv[0]);
		return 1;
	}

	Peer self(host, port);
	for (auto &peer: peers) {
		self.add_peer(peer);
	}

	Network network(self, host, port);

	std::thread net_thread([&network]() { network.start(); });
	net_thread.detach();

	std::cout << std::format("network started at: {}", self.address())
			  << std::endl;

	std::string line;
	while (std::getline(std::cin, line)) {
		if (line.starts_with("/exit")) break;
		if (line.starts_with("/peers")) {
			auto known_peers = self.get_peers();
			std::cout << std::format("known peers: {}", known_peers.size())
					  << std::endl;
			for (auto &peer: known_peers) {
				std::cout << std::format(" -—- [{}]", peer) << std::endl;
			}
			continue;
		}
		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;
		if (cmd == "/text") {
			std::string address;
			iss >> address;
			std::string txt;
			std::getline(iss, txt);
			if (!txt.empty() && txt[0] == ' ') txt.erase(0, 1);
			TextMessage message;
			message.id = Message::makeuid();
			message.sender = self.address();
			message.timestamp = std::chrono::system_clock::now();
			message.text = txt;
			send(address, message);
		}
	}

	return 0;
}
