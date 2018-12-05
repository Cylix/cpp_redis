/*
 *
 * Created by nick on 11/23/18.
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


#include <cpp_redis/core/types.hpp>

namespace cpp_redis {

	xmessage::xmessage(const reply_t &data) {
		auto d = data.as_array();
		set_id(d[0].as_string());
		std::string key;
		auto value_arr = d[1].as_array();
		push(value_arr.begin(), value_arr.end());
	}

	xstream::xstream(const reply &data) {
		auto d = data.as_array();
		Stream = d[0].as_string();
		for (auto &s : d[1].as_array()) {
			Messages.emplace_back(s);
		}
	}

	std::ostream &operator<<(std::ostream &os, const xmessage &xm) {
		os << "\n\t\t\"id\": " << xm.get_id() << "\n\t\t\"values\": {";
		for (auto &v : xm.get_values()) {
			auto re = v.second;
			os << "\n\t\t\t\"" << v.first << "\": " << re << ",";
		}
		os << "\n\t\t}";
		return os;
	}

	xmessage::xmessage() = default;

	std::ostream &operator<<(std::ostream &os, const xstream &xs) {
		os << "{\n\t\"stream\": " << xs.Stream << "\n\t\"messages\": [";
		for (auto &m : xs.Messages) {
			os << m;
		}
		os << "\n\t]\n}";
		return os;
	}

	xstream_reply::xstream_reply(const reply &data) {
		if (data.is_null()) {
			return;
		}
		for (auto &d : data.as_array()) {
			emplace_back(xstream(d));
		}
	}

	std::ostream &operator<<(std::ostream &os, const xstream_reply &xs) {
		for (auto &x : xs) {
			os << x;
		}
		return os;
	}

	xinfo_reply::xinfo_reply(const cpp_redis::reply &data) {
		if ( data.is_array()) {
			auto da = data.as_array();
			//reply z = da[0];
			//std::cout << data.as_string() << std::endl;
			Length = da[1].as_integer();
			RadixTreeKeys = da[3].as_integer();
			RadixTreeNodes = da[5].as_integer();
			Groups = da[7].as_integer();
			LastGeneratedId = da[9].as_string();
			xmessage_t x((da[11]));
			FirstEntry = x;
			xmessage_t y(da[13]);
			LastEntry = y;
		}
	}
} // namespace cpp_redis
