/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "tcp_utils.h"
#include <boost/asio.hpp>
#include <thread>


void waitPortOpen(unsigned short port)
{
    using namespace boost::asio;
    using ip::tcp;

    boost::system::error_code ec;
    do
    {
        {
            io_service svc;
            tcp::acceptor a(svc);
            a.open(tcp::v4(), ec) || a.bind({ tcp::v4(), port }, ec);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    while(ec != error::address_in_use);
}
