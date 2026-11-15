// Servidor WebSocket mínimo para controle local (simulador ↔ motor).
//
// Só o necessário para localhost: handshake HTTP/Upgrade, frames de texto
// mascarados do navegador, ping/pong e envio de texto para todos os clientes.
// Não é para exposição à internet. Implementa codex::ControlServer.
#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "hal.h"

namespace codex {
namespace wsdetail {

// --- SHA-1 (domínio público, compacto) ---
struct Sha1 {
    uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    static uint32_t rol(uint32_t v, int b) { return (v << b) | (v >> (32 - b)); }
    void block(const uint8_t* p) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = (p[i*4] << 24) | (p[i*4+1] << 16) | (p[i*4+2] << 8) | p[i*4+3];
        for (int i = 16; i < 80; ++i) w[i] = rol(w[i-3]^w[i-8]^w[i-14]^w[i-16], 1);
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i<20){f=(b&c)|(~b&d);k=0x5A827999;}
            else if (i<40){f=b^c^d;k=0x6ED9EBA1;}
            else if (i<60){f=(b&c)|(b&d)|(c&d);k=0x8F1BBCDC;}
            else {f=b^c^d;k=0xCA62C1D6;}
            uint32_t t=rol(a,5)+f+e+k+w[i]; e=d;d=c;c=rol(b,30);b=a;a=t;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;
    }
    static void hash(const std::string& in, uint8_t out[20]) {
        Sha1 s; std::vector<uint8_t> m(in.begin(), in.end());
        uint64_t bits = m.size() * 8ULL;
        m.push_back(0x80);
        while (m.size() % 64 != 56) m.push_back(0);
        for (int i = 7; i >= 0; --i) m.push_back((bits >> (i*8)) & 0xFF);
        for (size_t i = 0; i < m.size(); i += 64) s.block(m.data() + i);
        for (int i = 0; i < 5; ++i) {
            out[i*4]=s.h[i]>>24; out[i*4+1]=s.h[i]>>16; out[i*4+2]=s.h[i]>>8; out[i*4+3]=s.h[i];
        }
    }
};

inline std::string base64(const uint8_t* d, size_t n) {
    static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string o;
    for (size_t i = 0; i < n; i += 3) {
        uint32_t v = d[i] << 16;
        if (i+1 < n) v |= d[i+1] << 8;
        if (i+2 < n) v |= d[i+2];
        o += t[(v>>18)&63]; o += t[(v>>12)&63];
        o += (i+1 < n) ? t[(v>>6)&63] : '=';
        o += (i+2 < n) ? t[v&63] : '=';
    }
    return o;
}

}  // namespace wsdetail

class WsServer : public ControlServer {
public:
    explicit WsServer(uint16_t port) : port_(port) {}
    ~WsServer() override { stop(); }

    bool start(MessageHandler onMessage) override {
        onMsg_ = std::move(onMessage);
        listen_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listen_ < 0) return false;
        int yes = 1; ::setsockopt(listen_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        sockaddr_in addr{}; addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // só localhost
        addr.sin_port = htons(port_);
        if (::bind(listen_, (sockaddr*)&addr, sizeof(addr)) < 0) { ::close(listen_); return false; }
        if (::listen(listen_, 4) < 0) { ::close(listen_); return false; }
        run_ = true;
        thread_ = std::thread([this] { loop(); });
        return true;
    }

    void stop() override {
        run_ = false;
        if (listen_ >= 0) { ::shutdown(listen_, SHUT_RDWR); ::close(listen_); listen_ = -1; }
        if (thread_.joinable()) thread_.join();
        for (int c : clients_) ::close(c);
        clients_.clear();
    }

    void push(const std::string& json) override {
        std::lock_guard<std::mutex> lk(outMx_);
        outbox_.push(json);
    }

private:
    struct Client { int fd; bool handshaked = false; std::string rx; };

    void loop() {
        std::vector<Client> cs;
        while (run_) {
            std::vector<pollfd> pfds;
            pfds.push_back({listen_, POLLIN, 0});
            for (auto& c : cs) pfds.push_back({c.fd, POLLIN, 0});
            if (::poll(pfds.data(), pfds.size(), 50) < 0) break;

            if (pfds[0].revents & POLLIN) {
                int fd = ::accept(listen_, nullptr, nullptr);
                if (fd >= 0) cs.push_back({fd});
            }
            for (size_t i = 0; i < cs.size(); ) {
                auto& c = cs[i];
                short re = pfds[i+1].revents;
                bool drop = false;
                if (re & (POLLHUP | POLLERR)) drop = true;
                else if (re & POLLIN) drop = !onReadable(c);
                if (drop) { ::close(c.fd); cs.erase(cs.begin()+i); }
                else ++i;
            }
            flushOutbox(cs);
            syncClientList(cs);
        }
        for (auto& c : cs) ::close(c.fd);
    }

    bool onReadable(Client& c) {
        char tmp[2048];
        ssize_t n = ::recv(c.fd, tmp, sizeof(tmp), 0);
        if (n <= 0) return false;
        c.rx.append(tmp, n);
        if (!c.handshaked) return handshake(c);
        return parseFrames(c);
    }

    bool handshake(Client& c) {
        auto end = c.rx.find("\r\n\r\n");
        if (end == std::string::npos) return true;  // espera mais
        std::string key;
        size_t k = c.rx.find("Sec-WebSocket-Key:");
        if (k != std::string::npos) {
            k += 18; while (c.rx[k] == ' ') ++k;
            size_t e = c.rx.find("\r\n", k);
            key = c.rx.substr(k, e - k);
        }
        std::string accept = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        uint8_t dig[20]; wsdetail::Sha1::hash(accept, dig);
        std::string resp =
            "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\nSec-WebSocket-Accept: " + wsdetail::base64(dig, 20) + "\r\n\r\n";
        ::send(c.fd, resp.data(), resp.size(), 0);
        c.handshaked = true;
        c.rx.erase(0, end + 4);
        return true;
    }

    bool parseFrames(Client& c) {
        while (c.rx.size() >= 2) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(c.rx.data());
            uint8_t opcode = p[0] & 0x0F;
            bool masked = p[1] & 0x80;
            uint64_t len = p[1] & 0x7F;
            size_t hdr = 2;
            if (len == 126) { if (c.rx.size() < 4) return true; len = (p[2]<<8)|p[3]; hdr = 4; }
            else if (len == 127) return false;  // mensagens grandes não são esperadas
            uint8_t mask[4] = {0,0,0,0};
            if (masked) { if (c.rx.size() < hdr+4) return true; std::memcpy(mask, p+hdr, 4); hdr += 4; }
            if (c.rx.size() < hdr + len) return true;  // frame incompleto
            std::string payload(c.rx.begin()+hdr, c.rx.begin()+hdr+len);
            for (size_t i = 0; masked && i < payload.size(); ++i) payload[i] ^= mask[i & 3];
            c.rx.erase(0, hdr + len);
            if (opcode == 0x8) return false;                 // close
            else if (opcode == 0x9) sendFrame(c.fd, payload, 0xA);  // ping -> pong
            else if (opcode == 0x1 && onMsg_) onMsg_(payload);      // texto
        }
        return true;
    }

    static void sendFrame(int fd, const std::string& msg, uint8_t opcode = 0x1) {
        std::string f;
        f.push_back(static_cast<char>(0x80 | opcode));
        if (msg.size() < 126) f.push_back(static_cast<char>(msg.size()));
        else { f.push_back(126); f.push_back((msg.size()>>8)&0xFF); f.push_back(msg.size()&0xFF); }
        f += msg;
        ::send(fd, f.data(), f.size(), 0);
    }

    void flushOutbox(std::vector<Client>& cs) {
        std::queue<std::string> local;
        { std::lock_guard<std::mutex> lk(outMx_); std::swap(local, outbox_); }
        while (!local.empty()) {
            for (auto& c : cs) if (c.handshaked) sendFrame(c.fd, local.front());
            local.pop();
        }
    }

    void syncClientList(std::vector<Client>& cs) {
        clients_.clear();
        for (auto& c : cs) if (c.handshaked) clients_.push_back(c.fd);
    }

    uint16_t port_;
    int listen_ = -1;
    std::atomic<bool> run_{false};
    std::thread thread_;
    MessageHandler onMsg_;
    std::mutex outMx_;
    std::queue<std::string> outbox_;
    std::vector<int> clients_;
};

}  // namespace codex
