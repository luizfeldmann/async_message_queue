// Project
#include "async_message_queue/async_message_queue.hpp"

// Testing
#include <gtest/gtest.h>

// Depends
#include <boost/interprocess/ipc/message_queue.hpp>

// STD
#include <cinttypes>

// Namespaces
using namespace testing;
using namespace async_message_queue;

// Fixtures
class CProducerConsumer
    : public Test
{
protected:
    //! Type of message queue used here
    typedef 
        async_message_queue_<boost::asio::any_io_executor, boost::interprocess::message_queue>
        async_queue;

    // Context to run the queue and the async tasks
    boost::asio::io_context m_ioctx;

    //! Number of messages sent
    size_t m_uSent;

    //! Number of messages received
    size_t m_uRecv;

    // Number of messages to test
    const size_t m_nMsgs;

    //! Ownership of the sending buffer
    std::string m_sendStorage;

    //! Ownership of the storage
    std::array<char, 256> m_recvStorage;

    //! Queue existed and was removed
    bool m_bQueueExisted;

    //! Message queue being tested
    std::shared_ptr<async_queue> m_pQueue;

    //! Sends the next message
    void SendNext()
    {
        // Format the number
        m_sendStorage = std::to_string(m_uSent);
        std::cout << "send: " << m_sendStorage << std::endl;

        // Count the sent data
        ++m_uSent;

        // Send via the queue
        boost::asio::async_write(*m_pQueue, boost::asio::buffer(m_sendStorage),
            [this](boost::system::error_code ec, std::size_t bytes_transferred) {
                // Check error
                EXPECT_FALSE(ec) << ec.message();
                if (ec)
                    return;

                // Check end
                if (m_uSent >= m_nMsgs)
                    return;

                // Continue to send the next message
                boost::asio::post(
                    m_pQueue->get_executor(),
                    boost::beast::bind_front_handler(&CProducerConsumer::SendNext, this));
            });
    }

    //! Receives one message
    void ReceiveNext()
    {
        // Send via the queue
        boost::asio::async_read(*m_pQueue, boost::asio::buffer(m_recvStorage),
            [this](boost::system::error_code ec, std::size_t bytes_transferred) {
                // This means the received message was smaller than the buffer (requested size), but that's fine
                if (ec == boost::asio::error::message_size && bytes_transferred > 0)
                    ec = {};

                // Check error
                EXPECT_FALSE(ec) << ec.message();
                if (ec)
                    return;

                // Parse the received data
                const std::string sRecvMsg{ (char*)m_recvStorage.data(), bytes_transferred };
                const auto uRecvValue = std::stoul(sRecvMsg);

                std::cout << "recv: " << sRecvMsg << std::endl;

                // Check the received counter matches the expected counter
                EXPECT_EQ(uRecvValue, m_uRecv);

                // Count received and check end
                ++m_uRecv;
                if (m_uRecv >= m_nMsgs)
                    return;

                // Continue to receive next
                boost::asio::post(
                    m_pQueue->get_executor(),
                    boost::beast::bind_front_handler(&CProducerConsumer::ReceiveNext, this));
            });
    }

public:
    //! Test setup
    CProducerConsumer()
        : m_ioctx()
        , m_uSent(0)
        , m_uRecv(0)
        , m_nMsgs(1000)
        , m_bQueueExisted(boost::interprocess::message_queue::remove("my_queue"))
        , m_pQueue(std::make_shared<async_queue>(
            m_ioctx.get_executor(), boost::interprocess::create_only, "my_queue", 256, 256))
    {

    }
};

TEST_F(CProducerConsumer, SingleThread)
{
    // Schedule the first pair of operations
    SendNext();
    ReceiveNext();

    // Run until completion
    m_ioctx.run();

    // Check the send and received counters match
    EXPECT_EQ(m_uSent, m_uRecv);
}
