#ifndef _ASYNC_MESSAGE_QUEUE_HPP_
#define _ASYNC_MESSAGE_QUEUE_HPP_

// Depends
#include <boost/asio.hpp>
#include <boost/beast/core/bind_handler.hpp>

namespace async_message_queue {

//! Extension of boost's interprocess message queue with async capabilities
template<typename Executor, typename MessageQueue> 
class async_message_queue_
    : public MessageQueue
    , public std::enable_shared_from_this<async_message_queue_<Executor, MessageQueue>>
{
public:
    //! The type of message queue being wrapped
    typedef MessageQueue message_queue_type;

    //! The type of the executor associated with the object.
    typedef Executor executor_type;

    //! Rebinds this type to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        //! This type when rebound to the specified executor.
        typedef async_message_queue_<message_queue_type, Executor1> other;
    };

    //! Get the executor associated with the object.
    const executor_type& get_executor() const
    {
        return m_executor;
    }

    //! Constructs the extension using the provided executor
    template<typename Ex, typename...ArgsMQ>
    explicit async_message_queue_(Ex&& executor, ArgsMQ&&...args_message_queue)
        : MessageQueue(std::forward<ArgsMQ>(args_message_queue)...)
        , m_executor(std::forward<Ex>(executor))
    {
    }

    //! Initiates an asynchronous write
    template <typename ConstBufferSequence, typename WriteToken>
    auto async_write_some(ConstBufferSequence&& buffers, WriteToken&& token)
    {
        // Infer the signature of the handler
        using handler_type = BOOST_ASIO_HANDLER_TYPE(WriteToken, void(boost::system::error_code, std::size_t));

        // Initialize the asynchronous operation
        return boost::asio::async_initiate<WriteToken, void(boost::system::error_code, std::size_t)>(
            boost::beast::bind_front_handler(
                &async_message_queue_::initiate_async_write_some<handler_type, ConstBufferSequence>, 
                shared_from_this()),
            std::forward<WriteToken>(token), 
            std::forward<ConstBufferSequence>(buffers));
    }

    //! Initiates an asynchronous read
    template <typename MutableBufferSequence, typename ReadToken>
    auto async_read_some(MutableBufferSequence&& buffers, ReadToken&& token)
    {
        // Infer the signature of the handler
        using handler_type = BOOST_ASIO_HANDLER_TYPE(ReadToken, void(boost::system::error_code, std::size_t));
        boost::asio::async_completion<ReadToken, void(boost::system::error_code, std::size_t)> completionHandler{ token };

        // Initialize the asynchronous operation
        return boost::asio::async_initiate<ReadToken, void(boost::system::error_code, std::size_t)>(
            boost::beast::bind_front_handler(
                &async_message_queue_::initiate_async_read_some<handler_type, MutableBufferSequence>, 
                shared_from_this()),
            std::forward<ReadToken>(token),
            std::forward<MutableBufferSequence>(buffers));
    }

protected:
    //! The associated executor object
    executor_type m_executor;

    //! Tracks cancellation requests
    boost::asio::cancellation_signal m_cancel_signal; 

    //! Callback to post the initiation of the write operation.
    //! The handler cannot execute immediately as the operation is initiated.
    template <typename WriteHandler, typename ConstBufferSequence>
    void initiate_async_write_some(WriteHandler&& handler, ConstBufferSequence&& buffers)
    {
        BOOST_ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

        boost::asio::post(
            get_executor(),
            boost::beast::bind_front_handler(
                &async_message_queue_::do_async_write_some<WriteHandler, ConstBufferSequence>,
                shared_from_this(), 
                std::forward<WriteHandler>(handler), 
                std::forward<ConstBufferSequence>(buffers)));
    }

    //! Callback to post the initiation of the read operation.
    template <typename ReadHandler, typename MutableBufferSequence>
    void initiate_async_read_some(ReadHandler&& handler, MutableBufferSequence&& buffers)
    {
        BOOST_ASIO_READ_HANDLER_CHECK(ReadHandler, handler) type_check;

        boost::asio::post(
            get_executor(),
            boost::beast::bind_front_handler(
                &async_message_queue_::do_async_read_some<ReadHandler, MutableBufferSequence>,
                shared_from_this(), 
                std::forward<ReadHandler>(handler),
                std::forward<MutableBufferSequence>(buffers)));
    }

    //! Actual logic of the write operation
    template <typename WriteHandler, typename ConstBufferSequence>
    void do_async_write_some(WriteHandler&& handler, ConstBufferSequence&& buffers)
    {
        // Try the write operation
        try
        {
            const std::size_t bytes_written = boost::asio::buffer_size(buffers);
            if (try_send(boost::asio::buffer_cast<const void*>(buffers), bytes_written, 0))
            {
                // Operation success
                handler(boost::system::error_code{}, bytes_written);
            }
            else
            {
                // Defer and retry later
                boost::asio::defer(
                    get_executor(),
                    boost::beast::bind_front_handler(
                        &async_message_queue_::do_async_write_some<WriteHandler, ConstBufferSequence>,
                        shared_from_this(), 
                        std::forward<WriteHandler>(handler), 
                        std::forward<ConstBufferSequence>(buffers)));
            }
        }
        catch (boost::interprocess::interprocess_exception const& ex)
        {
            // The operation failed with some error
            handler(boost::asio::error::no_recovery, 0);
        }
    }

    //! Actual logic of the read operation
    template <typename ReadHandler, typename MutableBufferSequence>
    void do_async_read_some(ReadHandler&& handler, MutableBufferSequence&& buffers)
    {
        // Get the total size of the buffer
        std::size_t buffer_size = boost::asio::buffer_size(buffers);

        // If the buffer is too small for a message, we must not try the read
        if (buffer_size < get_max_msg_size())
        {
            handler(boost::asio::error::message_size, 0);
            return;
        }

        // Try the write operation
        try
        {
            unsigned int priority = 0;
            std::size_t bytes_received = 0;

            // If data was successfully received, invoke the handler
            if (try_receive(boost::asio::buffer_cast<void*>(buffers), buffer_size, bytes_received, priority))
            {
                handler(boost::system::error_code{}, bytes_received);
            }
            else
            {
                // Defer to retry later
                boost::asio::defer(
                    get_executor(),
                    boost::beast::bind_front_handler(
                        &async_message_queue_::do_async_read_some<ReadHandler, MutableBufferSequence>,
                        shared_from_this(), 
                        std::forward<ReadHandler>(handler), 
                        std::forward<MutableBufferSequence>(buffers)));
            }
        }
        catch (boost::interprocess::interprocess_exception const& ex)
        {
            // If an error occurs invoke the handler with no recovery
            handler(boost::asio::error::no_recovery, 0);
        }
    }

private:
    //! Non copy-constructible
    async_message_queue_(async_message_queue_ const&) = delete;

    //! Non copy-assignable
    async_message_queue_& operator=(async_message_queue_ const&) = delete;
};

//! Template deduction helper to construct an async_message_queue
template<typename MessageQueue, typename Executor, typename...ArgsMQ>
auto make_async_message_queue(const Executor& executor, ArgsMQ&&...args_message_queue)
{
    typedef async_message_queue_<Executor, MessageQueue> mq_type;

    return std::make_shared<mq_type>(
        executor, std::forward<ArgsMQ>(args_message_queue)...);
}

} // namespace async_message_queue

#endif
