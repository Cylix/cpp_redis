/*
 *
 * Created by nick on 11/22/18.
 *
 * Copyright(c) 2018 Iris. All rights reserved.
 *
 * Use and copying of this software and preparation of derivative
 * works based upon this software are  not permitted.  Any distribution
 * of this software or derivative works must comply with all applicable
 * Canadian export control laws.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL IRIS OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <cpp_redis/cpp_redis>
#include <tacopie/tacopie>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <signal.h>

#ifdef _WIN32
#include <Winsock2.h>
#endif /* _WIN32 */

std::condition_variable should_exit;

void
sigint_handler(int) {
	should_exit.notify_all();
}

int
main(void) {
#ifdef _WIN32
	//! Windows netword DLL init
  WORD version = MAKEWORD(2, 2);
  WSADATA data;

  if (WSAStartup(version, &data) != 0) {
    std::cerr << "WSAStartup() failure" << std::endl;
    return -1;
  }
#endif /* _WIN32 */

	//! Enable logging
	cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	cpp_redis::consumer_options_t opts;

	opts.group_name = "groupone";
	opts.session_name = "sessone";

	cpp_redis::consumer sub(opts);


	sub.connect("127.0.0.1", 6379, [](const std::string& host, std::size_t port, cpp_redis::consumer::connect_state status) {
			if (status == cpp_redis::consumer::connect_state::dropped) {
				std::cout << "client disconnected from " << host << ":" << port << std::endl;
				should_exit.notify_all();
			}
	});

	//! authentication if server-server requires it
	// sub.auth("some_password", [](const cpp_redis::reply& reply) {
	//   if (reply.is_error()) { std::cerr << "Authentication failed: " << reply.as_string() << std::endl; }
	//   else {
	//     std::cout << "successful authentication" << std::endl;
	//   }
	// });

	sub.subscribe("sessone", "groupone", [](const std::string& chan, const std::map<std::string, std::string> & msg) {
			std::cout << "MESSAGE " << chan << ": " << msg.at("id") << std::endl;
	});
	sub.commit();

	signal(SIGINT, &sigint_handler);
	std::mutex mtx;
	std::unique_lock<std::mutex> l(mtx);
	should_exit.wait(l);

#ifdef _WIN32
	WSACleanup();
#endif /* _WIN32 */

	return 0;
}
