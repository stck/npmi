#ifndef NPM_TLS_HPP
#define NPM_TLS_HPP

#include <random>
#include <vector>

namespace tls {
  using packet_inner_t = unsigned char;
  using packet_t       = tcp::buffer_t;

  namespace {
#define scp static_cast<packet_inner_t>
#define init_1b(a) scp(a)
#define init_2b(a) scp(((a) &0xff00) >> 8), scp((a) &0xff)
#define init_3b(a) scp(((a) &0xff0000) >> 16), scp(((a) &0xff00) >> 8), scp((a) &0xff)
#define put_2b(b, a)                         \
  (b).emplace_back(scp(((a) &0xff00) >> 8)); \
  (b).emplace_back(scp((a) &0xff))
#define put_packet(a, b) a.insert((a).end(), (b).begin(), (b).end())
#define get_1b(a, b) ((a)[(b)])
#define get_2b(a, b) ((a)[(b)] << 8 | (a)[(b) + 1])
#define get_3b(a, b) ((a)[(b)] << 16 | (a)[(b) + 1] << 8 | (a)[(b) + 2])

    const int MODULUS_HEAD = 0x308201;
    const int EXPONENT     = 0x010001;

    enum HandshakeType : packet_inner_t {
      ClientHello      = 0x01,
      ServerHello      = 0x02,
      Certificate      = 0x0b,
      ServerHelloDone  = 0x0e,
      ChangeCipherSpec = 0x14,
    };

    enum ContentType : packet_inner_t {
      Handshake       = 0x16,
      ApplicationData = 0x17
    };
    enum TLSVersion {
      TLS10 = 0x0301,
      TLS12 = 0x0303
    };
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> ibe;

    inline auto random_bytes(int size) -> packet_t {
      packet_t data(size);
      std::generate(data.begin(), data.end(), std::ref(ibe));

      return data;
    }

    inline auto client_hello(const packet_t& random) -> packet_t {
      packet_t extensions{};
      packet_t session_id{init_1b(0x00)};  //todo: session renegotation
      packet_t cipher_suites{
          init_2b(0x003c), // TLS_RSA_WITH_AES_128_CBC_SHA256
      };
      packet_t compression_methods{init_2b(0x0100)};

      packet_t handshake{init_2b(TLSVersion::TLS12)};
      put_packet(handshake, random);
      put_packet(handshake, session_id);
      put_2b(handshake, cipher_suites.size());
      put_packet(handshake, cipher_suites);
      put_packet(handshake, compression_methods);
      put_2b(handshake, extensions.size());
      put_packet(handshake, extensions);

      packet_t pack{
          init_1b(ContentType::Handshake),
          init_2b(TLSVersion::TLS10),
          init_2b(handshake.size() + 4),
          init_1b(HandshakeType::ClientHello),
          init_3b(handshake.size()),
      };
      put_packet(pack, handshake);

      return pack;
    }

    inline auto extract_random(const packet_t& data) -> std::tuple<packet_t, packet_t> {
      ssize_t offset = 7;
      packet_t random(32);
      packet_t sessionId(32);

      std::copy_n(&data[offset], random.size(), random.begin());
      std::copy_n(&data[offset + random.size() + 1], sessionId.size(), sessionId.begin());

      return std::make_tuple(random, sessionId);
    }

    inline auto extract_certificate(const packet_t& data) -> packet_t {
      ssize_t  offset      = 7;  // to get first cert length
      auto     cert_length = get_3b(data, offset);
      packet_t modulus(257);

      for (auto i = offset + 3; i < offset + cert_length - 3; i++) {
        if (get_3b(data, i) == MODULUS_HEAD && get_3b(data, i + 8 + modulus.size() + 2) == EXPONENT) {
          std::copy_n(&data[i + 8], modulus.size(), modulus.begin());
          break;
        }
      }

      return modulus;
    }

    inline auto receive_packets(const tcp::buffer_t& data) -> std::map<int, packet_t> {
      std::map<int, packet_t> packets{};
      ssize_t                 offset = 0;

      do {
        auto handshake  = get_1b(data, offset + 0);
        auto tlsVersion = get_2b(data, offset + 1);
        auto length     = get_2b(data, offset + 3);
        auto type       = get_1b(data, offset + 5);

        if (handshake != ContentType::Handshake) {
          offset++;
          continue;
        }
        assert(tlsVersion == TLSVersion::TLS12);

        packet_t contents(length);
        std::copy_n(&data[offset + 5], length, contents.begin());

        offset += 5 + length;
        packets.emplace(type, contents);
      } while (offset < data.size());

      return packets;
    }
  }  // namespace

  inline auto handshake(const tcp::socket_t& sock) {
    auto mac_key_length = 32; // for sha-256
    auto enc_key_length = 16;

    auto clientRandom = random_bytes(32);
    auto clientHello  = client_hello(clientRandom);

    tcp::send_bytes(sock, clientHello);
    auto bytes   = tcp::receive_bytes(sock);
    auto packets = receive_packets(bytes);

    auto serverHello        = packets.at(HandshakeType::ServerHello);
    auto certificatesPacket = packets.at(HandshakeType::Certificate);
    auto serverHelloDone    = packets.at(HandshakeType::ServerHelloDone);

    assert(!serverHello.empty());
    assert(!certificatesPacket.empty());
    assert(!serverHelloDone.empty());

    auto certificate = extract_certificate(certificatesPacket);
    auto [serverRandom, serverSessionId] = extract_random(serverHello);

    int b{};
  }
}  // namespace tls

#endif  // NPM_TLS_HPP