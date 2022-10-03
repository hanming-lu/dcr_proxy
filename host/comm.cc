#include <chrono>
#include <utility>
#include <stdlib.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <zmq.hpp>

#include "capsule.pb.h"
#include "comm.hpp"
#include "util/logging.hpp"

Comm::Comm()
    : m_context(1)
{
    m_ip = std::to_string(NET_PROXY_IP);
    m_join_mcast_port = std::to_string(NET_PROXY_RECV_DC_SERVER_JOIN_PORT);
    m_write_port = std::to_string(NET_PROXY_RECV_WRITE_REQ_PORT);
    m_write_addr = m_ip + ":" + m_write_port;
    m_ack_port = std::to_string(NET_PROXY_RECV_ACK_PORT);
}

void Comm::run_dc_proxy_listen_write_req_and_join_mcast()
{
    // to receive write msg from client
    zmq::socket_t socket_from_write(m_context, ZMQ_PULL);
    socket_from_write.bind("tcp://*:" + m_write_port);
    
    // to receive join mcast msg from dc server
    zmq::socket_t socket_from_join_mcast(m_context, ZMQ_PULL);
    socket_from_join_mcast.bind("tcp://*:" + m_join_mcast_port);

    // poll for new messages
    std::vector<zmq::pollitem_t> pollitems = {
        {static_cast<void *>(socket_from_write), 0, ZMQ_POLLIN, 0},
        {static_cast<void *>(socket_from_join_mcast), 0, ZMQ_POLLIN, 0},
    };

    Logger::log(LogLevel::DEBUG, "[DC Proxy] run_dc_proxy_listen_write_req_and_join_mcast() start polling.");
    while (true)
    {
        zmq::poll(pollitems.data(), pollitems.size(), 0);
        /* write req */
        if (pollitems[0].revents & ZMQ_POLLIN)
        {
            // Received a write msg from client
            std::string msg = this->recv_string(&socket_from_write);
            Logger::log(LogLevel::DEBUG, "[DC Proxy] Received a write message: " + msg);
            
            std::string mcast_msg = enc_handle_write(msg);

            for (auto &p : m_multicast_dc_server_addrs)
            {
                send_string(mcast_msg, p.second);
            }
        }
        /* join mcast */
        if (pollitems[1].revents & ZMQ_POLLIN) 
        {
            // Received a join mcast msg
            std::string dc_server_addr = this->recv_string(&socket_from_join_mcast);
            zmq::socket_t *socket_send_write = new zmq::socket_t(m_context, ZMQ_PUSH);
            socket_send_write->connect("tcp://" + dc_server_addr);
            m_multicast_dc_server_addrs[dc_server_addr] = socket_send_write;

            Logger::log(LogLevel::DEBUG, "[DC Proxy] Received join mcast for addr: " + dc_server_addr);
        }
    }
}

void Comm::run_dc_proxy_listen_ack()
{
    // to receive ack msg from DCR server
    zmq::socket_t socket_from_ack(m_context, ZMQ_PULL);
    socket_from_ack.bind("tcp://*:" + m_ack_port);

    // poll for new messages
    std::vector<zmq::pollitem_t> pollitems = {
        {static_cast<void *>(socket_from_ack), 0, ZMQ_POLLIN, 0},
    };

    Logger::log(LogLevel::DEBUG, "[DC Proxy] run_dc_proxy_listen_ack() start polling.");
    while (true)
    {
        zmq::poll(pollitems.data(), pollitems.size(), 0);
        /* ack msg */
        if (pollitems[0].revents & ZMQ_POLLIN)
        {
            // Received an ack msg from a DC server
            std::string msg = this->recv_string(&socket_from_ack);
            Logger::log(LogLevel::DEBUG, "[DC Proxy] Received an ack message: " + msg);
            
            enc_handle_ack(msg);
        }
    }
}

void Comm::dc_proxy_send_ack_to_replyaddr(std::string &out_msg, std::string &replyaddr)
{
    std::unordered_map<std::string, zmq::socket_t *> socket_send_ack_map;

    auto got = socket_send_ack_map.find(replyaddr);
    if ( got == socket_send_ack_map.end() )
    {
        zmq::socket_t *socket_send_ack = new zmq::socket_t(m_context, ZMQ_PUSH);
        socket_send_ack->connect("tcp://" + replyaddr);
        socket_send_ack_map[replyaddr] = socket_send_ack;
        Logger::log(LogLevel::DEBUG, "[DC Proxy] Connected to Client for ack. Addr: "+ replyaddr);
    }

    this->send_string(out_msg, socket_send_ack_map[replyaddr]);
    Logger::log(LogLevel::DEBUG, "[DC Proxy] Sent an ack msg: " + out_msg +
                                    " to client: " + replyaddr);
}