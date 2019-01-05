#include <cstdlib>
#include <ctime>
#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <sstream>
#include <string>
#include <vector>
#include <openssl/sha.h>

using boost::asio::ip::tcp;

char* getsuffix(const char *s, unsigned difficulty);

enum { max_length = 1024 };

class client {
public:
  client(boost::asio::io_context& io_context,
      boost::asio::ssl::context& context,
      tcp::resolver::results_type endpoints)
    : socket_(io_context, context)
  {
    socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    socket_.set_verify_callback(
        boost::bind(&client::verify_certificate, this, _1, _2));

    boost::asio::async_connect(socket_.lowest_layer(), endpoints,
        boost::bind(&client::handle_connect, this,
          boost::asio::placeholders::error));
  }

  bool verify_certificate(bool preverified,
      boost::asio::ssl::verify_context& ctx)
  {
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    std::cout << "Verifying " << subject_name << "\n";

    return preverified;
  }

  void handle_connect(const boost::system::error_code& error)
  {
    if (!error)
    {
      socket_.async_handshake(boost::asio::ssl::stream_base::client,
          boost::bind(&client::handle_handshake, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Connect failed: " << error.message() << "\n";
    }
  }

  void handle_handshake(const boost::system::error_code& error)
  {
    if (!error)
    {
      std::cout << "reading...\n";
      socket_.async_read_some(
          boost::asio::buffer(reply_, max_length),
          boost::bind(&client::handle_write, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      std::cout << "Handshake failed: " << error.message() << "\n";
    }
  }

  static std::string SHA1s(const std::string &s) {
      unsigned char digest[SHA_DIGEST_LENGTH];
      char md[SHA_DIGEST_LENGTH*2 + 1];
      SHA_CTX ctx;
      SHA1_Init(&ctx);
      SHA1_Update(&ctx, s.data(), s.length());
      SHA1_Final(digest, &ctx);
      for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
          sprintf(&md[i*2], "%02x", (unsigned)digest[i]);
      return md;
  }

  void handle_write(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      std::vector<std::string> args;
      std::string reply(reply_);
      boost::split(args, reply, boost::is_any_of("\n"));
      reply = args[0];
      boost::trim_if(reply, boost::is_any_of(" \t\n\r\x7f"));
      boost::split(args, reply, boost::is_any_of(" "));
      std::cout.write(reply_, bytes_transferred);
      if (args[0] == "HELO") {
          strcpy(request_, "EHLO\n");
          std::cout << "Got HELO send EHLO\n";
      }
      else if (args[0] == "POW") {
          std::string cksum_in_hex, zeros;
          unsigned difficulty = std::stoi(args[2]);
          authdata = args[1];
          unsigned long start = time(0);
          char *suffix = getsuffix(authdata.c_str(), difficulty);
          strcpy(request_, suffix);
          strcat(request_, "\n");
          std::cout << "Got POW " << args[1] << " " << args[2] << " send " << suffix 
              << " taking " << time(0) - start << "s for the suffix computation\n";
      }
      else if (args[0] == "END") {
          strcpy(request_, "OK\n");
          std::cout << "writing " << request_ << "\n";
          boost::asio::async_write(socket_,
            boost::asio::buffer(request_, strlen(request_)),
            boost::bind(&client::handle_finalize, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
          std::cout << "Got END, finishing\n";
          return;
      }
      else if (args[0] == "ERROR") {
          std::cout << "ERROR:";
          for (int i = 1; i < args.size(); ++i)
              std::cout << " " << args[i];
          std::cout << std::endl;
          return;
      }
      else if (args[0] == "NAME") {
          std::string ts = SHA1s(authdata + args[1]) + " Vladimir Lidovski\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else if (args[0] == "MAILNUM") {
          std::string ts = SHA1s(authdata + args[1]) + " 1\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else if (args[0] == "MAIL1") {
          std::string ts = SHA1s(authdata + args[1]) + " vol.litwr@gmail.com\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else if (args[0] == "SKYPE") {
          std::string ts = SHA1s(authdata + args[1]) + " vol.litwr\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else if (args[0] == "BIRTHDATE") {
          std::string ts = SHA1s(authdata + args[1]) + " 26.10.1969\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else if (args[0] == "COUNTRY") {
          std::string ts = SHA1s(authdata + args[1]) + " Russia\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else if (args[0] == "ADDRNUM") {
          std::string ts = SHA1s(authdata + args[1]) + " 2\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else if (args[0] == "ADDRLINE1") {
          std::string ts = SHA1s(authdata + args[1]) + " ulica Mira 18-46\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else if (args[0] == "ADDRLINE2") {
          std::string ts = SHA1s(authdata + args[1]) + " 142921 Kashira Moscow region\n";
          strcpy(request_, ts.c_str());
          std::cout << "Got " << args[0] << " " << args[1] << " send " << ts << "  for " << authdata + args[1] << "\n";
      }
      else {
          std::cout << "unexpected event, got";
          for (int i = 0; i < args.size(); ++i)
              std::cout << " " << args[i];
          std::cout << std::endl;
          return;
      }
      std::cout << "writing " << request_ << "\n";
      boost::asio::async_write(socket_,
          boost::asio::buffer(request_, strlen(request_)),
          boost::bind(&client::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
      std::cout << "Write failed: " << error.message() << "\n";
  }

  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error) {
      std::cout << "reading again...\n";
      socket_.async_read_some(
          boost::asio::buffer(reply_, max_length),
          boost::bind(&client::handle_write, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
      std::cout << "Handshake failed: " << error.message() << "\n";
  }

  void handle_finalize(boost::system::error_code error, size_t bytes_transferred) {
       boost::ignore_unused(bytes_transferred);
       std::cout << "finalizing\n";
       if (!error) {
          // Gracefully close the socket
          socket_.async_shutdown(
            std::bind(
                &client::on_shutdown, this,
                std::placeholders::_1));
       }
       else
          std::cout << "Handshake failed: " << error.message() << "\n";
  }

  void on_shutdown(boost::system::error_code error) {
       std::cout << "shutting down\n";
       if (error == boost::asio::error::eof) {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            error.assign(0, error.category());
       }
       if (error)
            std::cout << "Shutdown failed: " << error.message() << "\n";

        // If we get here then the connection is closed gracefully
    }
private:
  boost::asio::ssl::stream<tcp::socket> socket_;
  std::string authdata;
  char request_[max_length];
  char reply_[max_length];
};

int main(int argc, char* argv[]) {
  try
  {
    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints =
    resolver.resolve("exatest.dynu.net", "8443"); //3333, 8080, 8443, 49152, 3478, 65535

    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    ctx.set_default_verify_paths();
    ctx.load_verify_file("./getcert.pem");
    ctx.use_certificate_chain_file("./cert1.pem");
    ctx.use_private_key_file("./rsa.pem", boost::asio::ssl::context::pem);
    client c(io_context, ctx, endpoints);

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
