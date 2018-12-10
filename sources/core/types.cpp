// The MIT License (MIT)
//
// Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cpp_redis/core/types.hpp>

namespace cpp_redis {

	xmessage::xmessage(const reply_t &data) {
		if (data.is_array()) {
			auto k = data.as_array().front();
			auto v = data.as_array().back();
			if (k.is_string()) {
				set_id(k.as_string());
			}
			if (v.is_array()) {
				auto val_array = v.as_array();
				std::string key;
				int i = 1;
				for (auto &val : val_array) {
					if (i%2!=0) {
						key = val.as_string();
					} else {
						push(key, val);
					}
					i++;
				}
				//push(v.as_array().begin(), v.as_array().end());
			}
		}
	}

	xstream::xstream(const reply &data) {
		if (data.is_array()) {
			auto s = data.as_array().front();
			if (s.is_string()) {
				Stream = s.as_string();
			}
			auto m = data.as_array().back();
			if (m.is_array()) {
				for (auto &sm : m.as_array()) {
					Messages.emplace_back(xmessage(sm));
				}
			}
		}
	}

	std::ostream &operator<<(std::ostream &os, const xmessage &xm) {
		os << "\n\t\t\"id\": " << xm.get_id() << "\n\t\t\"values\": {";
		for (auto &v : xm.get_values()) {
			auto re = v.second;
			os << "\n\t\t\t\"" << v.first << "\": " << re.as_string() << ",";
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
		if (data.is_array()) {
			for (auto &d : data.as_array()) {
				emplace_back(xstream(d));
			}
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
