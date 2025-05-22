//
// Created by ilya on 22.05.2025.
//

#include "network.hpp"

#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <fstream>

using boost::asio::ip::tcp;

Network::Network(Peer &self, const std::string &address, unsigned short port) : self_(self), ctx_(), acceptor_(ctx_) {
	tcp::endpoint ep(boost::asio::ip::make_address(address), port);
	acceptor_.open(ep.protocol());
	acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
	acceptor_.bind(ep);
	acceptor_.listen();
}

void Network::start() {
	accept();
	threads_.emplace_back([this] { gossip(); });
	ctx_.run();
}

void Network::accept() {
	acceptor_.async_accept(
		[this](const boost::system::error_code &ec, tcp::socket socket) {
			if (!ec) {
				std::thread(&Network::handle, this, std::move(socket)).detach();
			}
			accept();
		}
	);
}

void Network::handle(tcp::socket socket) {
	try {
		boost::asio::streambuf input;
		boost::asio::read_until(socket, input, '\n');
		std::istream is(&input);
		std::string raw;
		std::getline(is, raw);
		auto message = Message::deserialize(raw);
		switch (message->type) {
			case MessageType::TEXT: {
				auto *t = static_cast<TextMessage *>(message.get()); {
					std::lock_guard<std::mutex> lock(mutex_);
					messages_.push_back(*t);
				}
				std::cout << std::format("[{}] {}", t->sender, t->text) << std::endl;
				self_.add_peer(t->sender);
				break;
			}
			case MessageType::FILE: {
				auto *f = static_cast<FileMessage *>(message.get()); {
					std::lock_guard<std::mutex> lock(mutex_);
					messages_.push_back(*f);
				}
				std::ofstream out(f->filename, std::ios::binary);
				out.write(f->bytes.data(), f->bytes.size());
				std::cout << std::format("+ {} ({} bytes) (from {})", f->filename, f->bytes.size(), f->sender) <<
						std::endl;
				self_.add_peer(f->sender);
				break;
			}
			case MessageType::REQUEST: {
				auto *req = static_cast<RequestMessage *>(message.get());
				std::ifstream in(req->filename, std::ios::binary);
				if (in) {
					std::vector<char> buffer{
						std::istreambuf_iterator<char>(in),
						std::istreambuf_iterator<char>()
					};
					FileMessage resp;
					// resp.id = make_id
					resp.sender = self_.address();
					resp.timestamp = std::chrono::system_clock::now();
					resp.filename = req->filename;
					resp.bytes = std::move(buffer);
					send(req->sender, resp.serialize() + '\n');
				}
				break;
			}
		}
	} catch (std::exception &e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
}

void Network::broadcast(const Message &message) {
	auto data = message.serialize() + '\n';
	const auto peers = self_.get_peers();
	for (auto &peer: peers) {
		send(peer, data);
	}
}

void Network::send(const std::string &address, const std::string &data) {
	auto pos = address.find(':');
	auto host = address.substr(0, pos);
	auto port = static_cast<unsigned short>(
		std::stoi(address.substr(pos + 1)));
	try {
		tcp::socket sock(ctx_);
		sock.connect({boost::asio::ip::make_address(host), port});
		boost::asio::write(sock, boost::asio::buffer(data));
	} catch (...) {
	}
}

void Network::gossip() {
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		std::vector<Message> copy; {
			std::lock_guard<std::mutex> lock(mutex_);
			copy = messages_;
		}
		for (auto &message: copy) {
			broadcast(message);
		}
	}
}
