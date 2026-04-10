// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 James Turner

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <cerrno>

#include <simgear/simgear_config.h>

#include "HTTPClient.hxx"
#include "HTTPRequest.hxx"

#include "test_HTTP.hxx"

#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/test_macros.hxx>

#include <curl/multi.h>


#ifdef SG_WINDOWS
    #include <winsock2.h>
    #define close(s) closesocket(s)
typedef int socklen_t;
typedef SOCKET raw_socket_t;
#else
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
typedef int raw_socket_t;
#endif


using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::stringstream;

using namespace simgear;

const char* BODY1 = "The quick brown fox jumps over a lazy dog.";
const char* BODY3 = "Cras ut neque nulla. Duis ut velit neque, sit amet "
                    "pharetra risus. In est ligula, lacinia vitae congue in, sollicitudin at "
                    "libero. Mauris pharetra pretium elit, nec placerat dui semper et. Maecenas "
                    "magna magna, placerat sed luctus ac, commodo et ligula. Mauris at purus et "
                    "nisl molestie auctor placerat at quam. Donec sapien magna, venenatis sed "
                    "iaculis id, fringilla vel arcu. Duis sed neque nisi. Cras a arcu sit amet "
                    "risus ultrices various. Integer sagittis euismod dui id various. Cras vel "
                    "justo gravida metus.";

const unsigned int body2Size = 8 * 1024;
char body2[body2Size];


class TestRequest : public HTTP::Request
{
public:
    bool complete;
    bool failed;
    string bodyData;

    TestRequest(const std::string& url, const std::string method = "GET") :
        HTTP::Request(url, method),
        complete(false),
        failed(false)
    {

    }

    std::map<string, string> headers;
protected:

    void onDone() override
    {
        complete = true;
    }

    void onFail() override
    {
        failed = true;
    }

    void gotBodyData(const char* s, int n) override
    {
        bodyData += string(s, n);
    }

    void responseHeader(const string& header, const string& value) override
    {
        Request::responseHeader(header, value);
        headers[header] =  value;
    }
};

TestServer testServer;

void waitForComplete(HTTP::Client* cl, TestRequest* tr)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  10000) {
        cl->update();

        if (tr->complete) {
            return;
        }
        SGTimeStamp::sleepForMSec(15);
    }

    cerr << "timed out" << endl;
}

void waitForFailed(HTTP::Client* cl, TestRequest* tr)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  10000) {
        cl->update();

        if (tr->failed) {
            return;
        }
        SGTimeStamp::sleepForMSec(15);
    }

    cerr << "timed out waiting for failure" << endl;
}

using CompletionCheck = std::function<bool()>;

bool waitFor(HTTP::Client* cl, CompletionCheck ccheck)
{
    SGTimeStamp start(SGTimeStamp::now());
    while (start.elapsedMSec() <  10000) {
        cl->update();

        if (ccheck()) {
            return true;
        }
        SGTimeStamp::sleepForMSec(15);
    }

    cerr << "timed out" << endl;
    return false;
}

void setupRoutes()
{
    testServer.svr.Get("/test1", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(BODY1, "text/plain");
    });

    testServer.svr.Get("/testLorem", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(BODY3, "text/plain");
    });

    testServer.svr.Get("/test_zero_length_content", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("", "text/plain");
    });

    testServer.svr.Get("/test_headers", [](const httplib::Request& req, httplib::Response& res) {
        SG_CHECK_EQUAL(req.get_header_value("X-Foo"), string("Bar"));
        SG_CHECK_EQUAL(req.get_header_value("X-AnotherHeader"), string("A longer value"));
        res.set_content(BODY1, "text/plain");
    });

    testServer.svr.Get("/test2", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(string(body2, body2Size), "application/octet-stream");
    });

    testServer.svr.Get("/testchunked", [](const httplib::Request&, httplib::Response& res) {
        std::string fullBody = "ABCDEFGHABCDEFABCDSTUVABCDSTUV";
        res.set_chunked_content_provider(
            "text/plain",
            [fullBody](size_t offset, httplib::DataSink& sink) {
                if (offset < fullBody.size()) {
                    sink.write(fullBody.data() + offset, fullBody.size() - offset);
                }
                sink.done_with_trailer({{"X-Foobar", "wibble"}});
                return true;
            });
    });

    testServer.svr.Get("/test_args", [](const httplib::Request& req, httplib::Response& res) {
        if (req.get_param_value("foo") != "abc" ||
            req.get_param_value("bar") != "1234" ||
            req.get_param_value("username") != "johndoe") {
            res.status = 400;
            res.set_content("bad arguments", "text/plain");
            return;
        }
        res.set_content(BODY1, "text/plain");
    });

    testServer.svr.Post("/test_post", [](const httplib::Request& req, httplib::Response& res) {
        if (req.get_header_value("Content-Type").find("application/x-www-form-urlencoded") == string::npos) {
            res.status = 400;
            res.set_content("bad content type", "text/plain");
            return;
        }

        if (req.get_param_value("foo") != "abc" ||
            req.get_param_value("bar") != "1234" ||
            req.get_param_value("username") != "johndoe") {
            res.status = 400;
            res.set_content("bad arguments", "text/plain");
            return;
        }

        res.status = 204;
    });

    testServer.svr.Put("/test_put", [](const httplib::Request& req, httplib::Response& res) {
        if (req.get_header_value("Content-Type") != "x-application/foobar") {
            res.status = 400;
            res.set_content("bad content type", "text/plain");
            return;
        }

        SG_CHECK_EQUAL(req.body, string(BODY3));
        res.status = 204;
    });

    testServer.svr.Put("/test_create", [](const httplib::Request& req, httplib::Response& res) {
        if (req.get_header_value("Content-Type") != "x-application/foobar") {
            res.status = 400;
            res.set_content("bad content type", "text/plain");
            return;
        }

        SG_CHECK_EQUAL(req.body, string(BODY3));

        std::string entityStr = testServer.url("/something.txt");
        res.status = 201;
        res.set_header("Location", entityStr);
        res.set_content(entityStr, "text/plain");
    });

    // HTTP/1.0 style: Connection: close, no Content-Length
    testServer.svr.Get(R"(/test_1_0(.*))", [](const httplib::Request& req, httplib::Response& res) {
        string suffix = req.matches[1];
        string contentStr(BODY1);
        if (suffix == "/B") {
            contentStr = BODY3;
        }
        res.set_header("Connection", "close");
        res.set_content(contentStr, "text/plain");
    });

    testServer.svr.Get("/test_close", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Connection", "close");
        res.set_content(BODY1, "text/plain");
    });

    testServer.svr.Get("/test_get_during_send", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(BODY3, "text/plain");
    });

    testServer.svr.Get("/test_get_during_send_2", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(BODY1, "text/plain");
    });

    testServer.svr.Get("/test_redirect", [](const httplib::Request&, httplib::Response& res) {
        res.status = 302;
        res.set_header("Location", testServer.url("/was_redirected"));
        res.set_content("<html>See <a href=\"wibble\">Here</a></html>", "text/html");
    });

    testServer.svr.Get("/was_redirected", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(BODY1, "text/plain");
    });

    // Proxy-style routes: when curl sends a proxy request, the full URL is the path
    testServer.svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        if (req.path == "http://www.google.com/test2") {
            if (req.get_header_value("Host") != "www.google.com") {
                res.status = 400;
                res.set_content("bad destination", "text/plain");
                return httplib::Server::HandlerResponse::Handled;
            }

            if (!req.get_header_value("Proxy-Authorization").empty()) {
                res.status = 401;
                res.set_content("bad auth, not empty", "text/plain");
                return httplib::Server::HandlerResponse::Handled;
            }

            res.set_content(string(body2, body2Size), "application/octet-stream");
            return httplib::Server::HandlerResponse::Handled;
        }

        if (req.path == "http://www.google.com/test3") {
            if (req.get_header_value("Host") != "www.google.com") {
                res.status = 400;
                res.set_content("bad destination", "text/plain");
                return httplib::Server::HandlerResponse::Handled;
            }

            string credentials = req.get_header_value("Proxy-Authorization");
            if (credentials.substr(0, 5) != "Basic") {
                res.status = 407;
                res.set_header("WWW-Authenticate", "Basic realm=\"simgear\"");
                return httplib::Server::HandlerResponse::Handled;
            }

            std::vector<unsigned char> userAndPass;
            strutils::decodeBase64(credentials.substr(6), userAndPass);
            std::string decodedUserPass((char*)userAndPass.data(), userAndPass.size());

            if (decodedUserPass != "johndoe:swordfish") {
                res.status = 401;
                res.set_content("bad auth, not as set", "text/plain");
                return httplib::Server::HandlerResponse::Handled;
            }

            res.set_content(string(body2, body2Size), "application/octet-stream");
            return httplib::Server::HandlerResponse::Handled;
        }

        return httplib::Server::HandlerResponse::Unhandled;
    });
}


int main(int argc, char* argv[])
{
    sglog().setLogLevels( SG_ALL, SG_INFO );

    setupRoutes();
    testServer.start();

    const int port = testServer.port();

    HTTP::Client cl;
    // force all requests to use the same connection for this test
    cl.setMaxConnections(1);

// test URL parsing
    {
        auto tr1 = std::make_unique<TestRequest>("http://localhost.woo.zar:2000/test1?foo=bar"); // codespell:ignore zar
        SG_CHECK_EQUAL(tr1->scheme(), "http");
        SG_CHECK_EQUAL(tr1->hostAndPort(), "localhost.woo.zar:2000"); // codespell:ignore zar
        SG_CHECK_EQUAL(tr1->host(), "localhost.woo.zar");             // codespell:ignore zar
        SG_CHECK_EQUAL(tr1->port(), 2000);
        SG_CHECK_EQUAL(tr1->path(), "/test1");
    }

    {
        auto tr2 = std::make_unique<TestRequest>("http://192.168.1.1/test1/dir/thing/file.png");
        SG_CHECK_EQUAL(tr2->scheme(), "http");
        SG_CHECK_EQUAL(tr2->hostAndPort(), "192.168.1.1");
        SG_CHECK_EQUAL(tr2->host(), "192.168.1.1");
        SG_CHECK_EQUAL(tr2->port(), 80);
        SG_CHECK_EQUAL(tr2->path(), "/test1/dir/thing/file.png");
    }

// basic get request
    {
        TestRequest* tr = new TestRequest(testServer.url("/test1"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }

    {
        TestRequest* tr = new TestRequest(testServer.url("/testLorem"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY3));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY3));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY3));
    }

    {
        TestRequest* tr = new TestRequest(testServer.url("/test_args?foo=abc&bar=1234&username=johndoe"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
    }

    {
        TestRequest* tr = new TestRequest(testServer.url("/test_headers"));
        HTTP::Request_ptr own(tr);
        tr->requestHeader("X-Foo") = "Bar";
        tr->requestHeader("X-AnotherHeader") = "A longer value";
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }

// larger get request
    for (unsigned int i=0; i<body2Size; ++i) {
        // this contains embedded 0s on purpose, i.e it's
        // not text data but binary
        body2[i] = (i << 4) | (i >> 2);
    }

    {
        TestRequest* tr = new TestRequest(testServer.url("/test2"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseBytesReceived(), body2Size);
        SG_CHECK_EQUAL(tr->bodyData, string(body2, body2Size));
    }

    cerr << "testing chunked transfer encoding" << endl;
    {
        TestRequest* tr = new TestRequest(testServer.url("/testchunked"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);

        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
        SG_CHECK_EQUAL(tr->responseBytesReceived(), 30);
        SG_CHECK_EQUAL(tr->bodyData, "ABCDEFGHABCDEFABCDSTUVABCDSTUV");
    // check trailers made it too
        SG_CHECK_EQUAL(tr->headers["x-foobar"], string("wibble"));
    }

// test 404
    {
        TestRequest* tr = new TestRequest(testServer.url("/not-found"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 404);
        SG_CHECK_EQUAL(tr->responseReason(), string("Not Found"));
    }

    cout << "done 404 test" << endl;

    {
        TestRequest* tr = new TestRequest(testServer.url("/test_args?foo=abc&bar=1234&username=johndoe"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
    }

    cout << "done1" << endl;
// test HTTP/1.0
    {
        TestRequest* tr = new TestRequest(testServer.url("/test_1_0"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }

    cout << "done2" << endl;
// test HTTP/1.1 Connection::close
    {
        TestRequest* tr = new TestRequest(testServer.url("/test_close"));
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForComplete(&cl, tr);
        SG_CHECK_EQUAL(tr->responseCode(), 200);
        SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
        SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
    }
    cout << "done3" << endl;
// test connectToHost failure

    {
        TestRequest* tr = new TestRequest("http://not.found/something");
        HTTP::Request_ptr own(tr);
        cl.makeRequest(tr);
        waitForFailed(&cl, tr);

        const int HOST_NOT_FOUND_CODE = CURLE_COULDNT_RESOLVE_HOST;
        SG_CHECK_EQUAL(tr->responseCode(), HOST_NOT_FOUND_CODE);
    }

  cout << "testing abrupt close" << endl;
  // test server-side abrupt close: use a raw TCP socket that accepts
  // a connection and immediately closes it, producing CURLE_GOT_NOTHING
  {
      raw_socket_t listen_sock = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in addr = {};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      addr.sin_port = 0;
      ::bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr));
      ::listen(listen_sock, 1);

      socklen_t addrlen = sizeof(addr);
      getsockname(listen_sock, (struct sockaddr*)&addr, &addrlen);
      int abrupt_port = ntohs(addr.sin_port);

      std::thread acceptThread([listen_sock]() {
          raw_socket_t client = accept(listen_sock, nullptr, nullptr);
#ifdef SG_WINDOWS
          if (client != INVALID_SOCKET) {
#else
          if (client >= 0) {
#endif
              // Read the incoming request so the TCP connection is fully
              // established from curl's perspective, then close without
              // sending any response data.
              char buf[4096];
              recv(client, buf, sizeof(buf), 0);
              ::close(client);
          }
          ::close(listen_sock);
      });

      string abruptUrl = "http://127.0.0.1:" + std::to_string(abrupt_port) + "/test_abrupt_close";
      TestRequest* tr = new TestRequest(abruptUrl);
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);
      waitForFailed(&cl, tr);

      const int SERVER_NO_DATA_CODE = CURLE_GOT_NOTHING;
      SG_CHECK_EQUAL(tr->responseCode(), SERVER_NO_DATA_CODE);

      acceptThread.join();
  }

  cout << "testing proxy" << endl;
  // test proxy
  {
      cl.setProxy("127.0.0.1", port);
      TestRequest* tr = new TestRequest("http://www.google.com/test2");
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);
      waitForComplete(&cl, tr);
      SG_CHECK_EQUAL(tr->responseCode(), 200);
      SG_CHECK_EQUAL(tr->responseLength(), body2Size);
      SG_CHECK_EQUAL(tr->bodyData, string(body2, body2Size));
  }

  {
      cl.setProxy("127.0.0.1", port, "johndoe:swordfish");
      TestRequest* tr = new TestRequest("http://www.google.com/test3");
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);
      waitForComplete(&cl, tr);
      SG_CHECK_EQUAL(tr->responseCode(), 200);
      SG_CHECK_EQUAL(tr->responseBytesReceived(), body2Size);
      SG_CHECK_EQUAL(tr->bodyData, string(body2, body2Size));
  }

  // pipelining: cpp-httplib serves HTTP/1.1 with keep-alive, so multiple
  // concurrent requests on the same connection work correctly.
  cout << "testing HTTP 1.1 pipelining" << endl;

  {
      cl.clearAllConnections();

      cl.setProxy("", 80);
      TestRequest* tr = new TestRequest(testServer.url("/test1"));
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);


      TestRequest* tr2 = new TestRequest(testServer.url("/testLorem"));
      HTTP::Request_ptr own2(tr2);
      cl.makeRequest(tr2);

      TestRequest* tr3 = new TestRequest(testServer.url("/test1"));
      HTTP::Request_ptr own3(tr3);
      cl.makeRequest(tr3);

      SG_VERIFY(waitFor(&cl, [tr, tr2, tr3]() {
          return tr->complete && tr2->complete && tr3->complete;
      }));

      SG_CHECK_EQUAL(tr->bodyData, string(BODY1));

      SG_CHECK_EQUAL(tr2->responseLength(), strlen(BODY3));
      SG_CHECK_EQUAL(tr2->responseBytesReceived(), strlen(BODY3));
      SG_CHECK_EQUAL(tr2->bodyData, string(BODY3));

      SG_CHECK_EQUAL(tr3->bodyData, string(BODY1));
  }

  // multiple requests with an HTTP 1.0 server
  {
      cout << "http 1.0 multiple requests" << endl;

      cl.setProxy("", 80);
      TestRequest* tr = new TestRequest(testServer.url("/test_1_0/A"));
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);

      TestRequest* tr2 = new TestRequest(testServer.url("/test_1_0/B"));
      HTTP::Request_ptr own2(tr2);
      cl.makeRequest(tr2);

      TestRequest* tr3 = new TestRequest(testServer.url("/test_1_0/C"));
      HTTP::Request_ptr own3(tr3);
      cl.makeRequest(tr3);

      SG_VERIFY(waitFor(&cl, [tr, tr2, tr3]() {
          return tr->complete && tr2->complete && tr3->complete;
      }));

      SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
      SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
      SG_CHECK_EQUAL(tr->bodyData, string(BODY1));

      SG_CHECK_EQUAL(tr2->responseLength(), strlen(BODY3));
      SG_CHECK_EQUAL(tr2->responseBytesReceived(), strlen(BODY3));
      SG_CHECK_EQUAL(tr2->bodyData, string(BODY3));
      SG_CHECK_EQUAL(tr3->bodyData, string(BODY1));
  }

  // POST
  {
      cout << "testing POST" << endl;
      TestRequest* tr = new TestRequest(testServer.url("/test_post?foo=abc&bar=1234&username=johndoe"), "POST");
      tr->setBodyData("", "application/x-www-form-urlencoded");

      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);
      waitForComplete(&cl, tr);
      SG_CHECK_EQUAL(tr->responseCode(), 204);
  }

  // PUT
  {
      cout << "testing PUT" << endl;
      TestRequest* tr = new TestRequest(testServer.url("/test_put"), "PUT");
      tr->setBodyData(BODY3, "x-application/foobar");

      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);
      waitForComplete(&cl, tr);
      SG_CHECK_EQUAL(tr->responseCode(), 204);
  }

  {
      cout << "testing PUT create" << endl;
      TestRequest* tr = new TestRequest(testServer.url("/test_create"), "PUT");
      tr->setBodyData(BODY3, "x-application/foobar");

      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);
      waitForComplete(&cl, tr);
      SG_CHECK_EQUAL(tr->responseCode(), 201);
  }

  // test_zero_length_content
  {
      cout << "zero-length-content-response" << endl;
      TestRequest* tr = new TestRequest(testServer.url("/test_zero_length_content"));
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);
      waitForComplete(&cl, tr);
      SG_CHECK_EQUAL(tr->responseCode(), 200);
      SG_CHECK_EQUAL(tr->bodyData, string());
      SG_CHECK_EQUAL(tr->responseBytesReceived(), 0);
  }

  // test cancel
  {
      cout << "cancel  request" << endl;
      cl.clearAllConnections();

      cl.setProxy("", 80);
      TestRequest* tr = new TestRequest(testServer.url("/test1"));
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);

      TestRequest* tr2 = new TestRequest(testServer.url("/testLorem"));
      HTTP::Request_ptr own2(tr2);
      cl.makeRequest(tr2);

      TestRequest* tr3 = new TestRequest(testServer.url("/test1"));
      HTTP::Request_ptr own3(tr3);
      cl.makeRequest(tr3);

      cl.cancelRequest(tr, "my reason 1");

      cl.cancelRequest(tr2, "my reason 2");

      waitForComplete(&cl, tr3);

      SG_CHECK_EQUAL(tr->responseCode(), -1);
      SG_CHECK_EQUAL(tr2->responseReason(), "my reason 2");

      SG_CHECK_EQUAL(tr3->responseLength(), strlen(BODY1));
      SG_CHECK_EQUAL(tr3->responseBytesReceived(), strlen(BODY1));
      SG_CHECK_EQUAL(tr3->bodyData, string(BODY1));
  }

  // test cancel
  {
      cout << "cancel middle request" << endl;
      cl.clearAllConnections();

      cl.setProxy("", 80);
      TestRequest* tr = new TestRequest(testServer.url("/test1"));
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);

      TestRequest* tr2 = new TestRequest(testServer.url("/testLorem"));
      HTTP::Request_ptr own2(tr2);
      cl.makeRequest(tr2);

      TestRequest* tr3 = new TestRequest(testServer.url("/test1"));
      HTTP::Request_ptr own3(tr3);
      cl.makeRequest(tr3);

      cl.cancelRequest(tr2, "middle request");

      waitForComplete(&cl, tr3);

      SG_CHECK_EQUAL(tr->responseCode(), 200);
      SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
      SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
      SG_CHECK_EQUAL(tr->bodyData, string(BODY1));

      SG_CHECK_EQUAL(tr2->responseCode(), -1);

      SG_CHECK_EQUAL(tr3->responseLength(), strlen(BODY1));
      SG_CHECK_EQUAL(tr3->responseBytesReceived(), strlen(BODY1));
      SG_CHECK_EQUAL(tr3->bodyData, string(BODY1));
  }

  // make a request mid-response: cpp-httplib handles keep-alive connections
  // so the second request is processed after the first completes.
  {
      cout << "get-during-response-send" << endl;
      cl.clearAllConnections();

      TestRequest* tr = new TestRequest(testServer.url("/test_get_during_send"));
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);

      // kick things along
      for (int i = 0; i < 10; ++i) {
          SGTimeStamp::sleepForMSec(1);
          cl.update();
      }

      TestRequest* tr2 = new TestRequest(testServer.url("/test_get_during_send_2"));
      HTTP::Request_ptr own2(tr2);
      cl.makeRequest(tr2);

      SG_VERIFY(waitFor(&cl, [tr, tr2]() {
          return tr->isComplete() && tr2->isComplete();
      }));

      SG_CHECK_EQUAL(tr->responseCode(), 200);
      SG_CHECK_EQUAL(tr->bodyData, string(BODY3));
      SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY3));
      SG_CHECK_EQUAL(tr2->responseCode(), 200);
      SG_CHECK_EQUAL(tr2->bodyData, string(BODY1));
      SG_CHECK_EQUAL(tr2->responseBytesReceived(), strlen(BODY1));
  }

  {
      cout << "redirect test" << endl;
      // redirect test
      cl.clearAllConnections();

      TestRequest* tr = new TestRequest(testServer.url("/test_redirect"));
      HTTP::Request_ptr own(tr);
      cl.makeRequest(tr);

      waitForComplete(&cl, tr);
      SG_CHECK_EQUAL(tr->responseCode(), 200);
      SG_CHECK_EQUAL(tr->responseReason(), string("OK"));
      SG_CHECK_EQUAL(tr->responseLength(), strlen(BODY1));
      SG_CHECK_EQUAL(tr->responseBytesReceived(), strlen(BODY1));
      SG_CHECK_EQUAL(tr->bodyData, string(BODY1));
  }

  cout << "all tests passed ok" << endl;
  return EXIT_SUCCESS;
}
