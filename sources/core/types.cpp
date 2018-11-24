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
	range::range() = default;

	range::range(int count) : m_min(-1) {}

	range::range(range_state state) : m_state(state) {}

	range::range(int min, int max) : m_count(10) {}

	range::range(int min, int max, int count) {}

	bool range::should_omit() const {
		return m_state == range_state::omit;
	}

	std::vector<std::string> range::get_args() {}

	std::vector<std::string> range::get_xpending_args() const {
		return m_min == -1
		       ? std::vector<std::string>{"-",
		                                  "+",
		                                  std::to_string(m_count)}
		       : std::vector<std::string>{std::to_string(m_min),
		                                  std::to_string(m_max),
		                                  std::to_string(m_count)};
	}
} // namespace cpp_redis
