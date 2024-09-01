#include "serialPort.h"

#include <algorithm>
#include <iostream>
#include <string>

// #define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/bind/bind.hpp>

using namespace std;
using namespace boost;

#include <thread>
// #include <mutex>
// #include <boost/bind.hpp>
#include <boost/shared_array.hpp>
// reference from: https://github.com/karthickai/serial
using namespace std::placeholders;

void SerialPort::open(const std::string& port_name, uint32_t baudrate) {
  port_.open(port_name);
  if (!port_.is_open()) return;

  port_.set_option(asio::serial_port_base::baud_rate(baudrate));
  port_.set_option(asio::serial_port_base::character_size(8));
  port_.set_option(asio::serial_port_base::stop_bits(
      asio::serial_port_base::stop_bits::one));
  port_.set_option(
      asio::serial_port_base::parity(asio::serial_port_base::parity::none));
  port_.set_option(asio::serial_port_base::flow_control(
      asio::serial_port_base::flow_control::none));

  asyncReadThread.reset(new std::thread([this] { ioc_.run(); }));
  asyncRead();
}

SerialPort::SerialPort()
    : ioc_(), port_(ioc_), work_(ioc_), asyncReadThread(nullptr), rbuf_{} {}

SerialPort::~SerialPort() {
  if (port_.is_open()) close();
}
bool SerialPort::isOpen() const { return port_.is_open(); }

void SerialPort::close() {
  if (!port_.is_open()) return;

  port_.cancel();

  ioc_.stop();
  asyncReadThread->join();

  port_.close();
}

size_t SerialPort::write(const uint8_t* buffer, size_t size) {
  return boost::asio::write(port_, boost::asio::buffer(buffer, size));
}

void SerialPort::asyncRead() {
  port_.async_read_some(boost::asio::buffer(rbuf_, rbuf_.size()),
                        [this](const boost::system::error_code& error,
                               std::size_t bytes_transferred) {
                          asyncReadHandler(error, bytes_transferred);
                        });
}

void SerialPort::asyncReadHandler(boost::system::error_code const& error,
                                  size_t bytes_transferred) {
  if (error) setError(error.value());

  std::unique_lock<std::mutex> lk(readBufferMtx);
  for (auto i = 0; i < bytes_transferred; i++) {
    read_buffer_.push_back(rbuf_[i]);
  }

  if (read_buffer_.size() > READBUFFERSIZE) {
    unsigned int overflow = read_buffer_.size() - READBUFFERSIZE;
    read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + overflow);
  }
  lk.unlock();

  asyncRead();
}

void SerialPort::asyncWriteHandler(const boost::system::error_code& error,
                                   std::size_t bytes_transferred) {
  if (error) setError(error.value());

  std::unique_lock<std::mutex> lk(writeMtx);
  writeLocked = false;
  lk.unlock();
  writeCv.notify_one();
}

int16_t SerialPort::readByte() {
  std::unique_lock<std::mutex> lk(readBufferMtx);

  if (!read_buffer_.size()) return -1;

  int res = read_buffer_[0];
  read_buffer_.erase(read_buffer_.begin());
  return res;
}

std::vector<char> SerialPort::readBuffer(size_t len) {
  std::vector<char> res;
  while (res.size() < len) {
    const int16_t b = readByte();
    if (b > -1) {
      res.push_back((uint8_t)b);
    }
  }
  return res;
}

std::vector<char> SerialPort::readBufferTimeout(size_t len) {
  auto start = std::chrono::system_clock::now();
  bool timeout = false;
  std::vector<char> res;

  while (res.size() < len && !timeout) {
    auto now = std::chrono::system_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    if (elapsed.count() >= timeoutVal) {
      timeout = true;
    } else {
      const int16_t b = readByte();
      if (b > -1) {
        res.push_back((char)b);
      }
    }
  }
  return res;
}

std::future<std::vector<char>> SerialPort::readAsync(const size_t len) {
  return std::async(std::launch::deferred,
                    boost::bind(&SerialPort::readBuffer, this, len));
}

std::future<std::vector<char>> SerialPort::readAsync(const size_t len,
                                                        unsigned int timeout) {
  timeoutVal = timeout;
  return std::async(std::launch::deferred,
                    boost::bind(&SerialPort::readBufferTimeout, this, len));
}

size_t SerialPort::write(std::vector<uint8_t> const& data) {
  return boost::asio::write(port_, boost::asio::buffer(data));
}

std::size_t SerialPort::writeAsync(const uint8_t* buffer, size_t size) {
  std::unique_lock<std::mutex> lk(writeMtx);
  if (writeLocked) writeCv.wait(lk);

  writeLocked = true;
  lk.unlock();

  boost::asio::async_write(port_, boost::asio::buffer(buffer, size),
                           [this](const boost::system::error_code& error,
                                  std::size_t bytes_transferred) {
                             asyncWriteHandler(error, bytes_transferred);
                           });

  return size;
}

std::size_t SerialPort::writeAsync(std::string str) {
  return writeAsync(reinterpret_cast<uint8_t *>(str.data()), str.size());
}

void SerialPort::setError(const int error) {
  std::unique_lock<std::mutex> elk(errMtx);
  error_value = error;
}

int SerialPort::getError() const {
  std::unique_lock<std::mutex> lk(errMtx);
  return error_value;
}
// class AsyncSerialImpl: private boost::noncopyable
// {
// public:
//     AsyncSerialImpl(): io(), port(io), backgroundThread(), open(false),
//             error(false) {}

//     boost::asio::io_service io; ///< Io service object
//     boost::asio::serial_port port; ///< Serial port object
//     std::thread backgroundThread; ///< Thread that runs read/write operations
//     bool open; ///< True if port open
//     bool error; ///< Error flag
//     mutable std::mutex errorMutex; ///< Mutex for access to error

//     /// Data are queued here before they go in writeBuffer
//     std::vector<char> writeQueue;
//     boost::shared_array<char> writeBuffer; ///< Data being written
//     size_t writeBufferSize; ///< Size of writeBuffer
//     std::mutex writeQueueMutex; ///< Mutex for access to writeQueue
//     char readBuffer[AsyncSerial::readBufferSize]; ///< data being read

//     /// Read complete callback
//     std::function<void (const char*, size_t)> callback;
// };

// AsyncSerial::AsyncSerial(): pimpl(new AsyncSerialImpl)
// {

// }

// AsyncSerial::AsyncSerial(const std::string& devname, unsigned int baud_rate,
//         asio::serial_port_base::parity opt_parity,
//         asio::serial_port_base::character_size opt_csize,
//         asio::serial_port_base::flow_control opt_flow,
//         asio::serial_port_base::stop_bits opt_stop)
//         : pimpl(new AsyncSerialImpl)
// {
//     open(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop);
// }

// void AsyncSerial::open(const std::string& devname, unsigned int baud_rate,
//         asio::serial_port_base::parity opt_parity,
//         asio::serial_port_base::character_size opt_csize,
//         asio::serial_port_base::flow_control opt_flow,
//         asio::serial_port_base::stop_bits opt_stop)
// {
//     if(isOpen()) close();

//     setErrorStatus(true);//If an exception is thrown, error_ remains true
//     pimpl->port.open(devname);
//     pimpl->port.set_option(asio::serial_port_base::baud_rate(baud_rate));
//     pimpl->port.set_option(opt_parity);
//     pimpl->port.set_option(opt_csize);
//     pimpl->port.set_option(opt_flow);
//     pimpl->port.set_option(opt_stop);

//     //This gives some work to the io_service before it is started
//     pimpl->io.post(boost::bind(&AsyncSerial::doRead, this));

//     thread t(boost::bind(&asio::io_service::run, &pimpl->io));
//     pimpl->backgroundThread.swap(t);
//     setErrorStatus(false);//If we get here, no error
//     pimpl->open=true; //Port is now open
// }

// bool AsyncSerial::isOpen() const
// {
//     return pimpl->open;
// }

// bool AsyncSerial::errorStatus() const
// {
//     lock_guard<mutex> l(pimpl->errorMutex);
//     return pimpl->error;
// }

// void AsyncSerial::close()
// {
//     if(!isOpen()) return;

//     pimpl->open=false;
//     pimpl->io.post(boost::bind(&AsyncSerial::doClose, this));
//     pimpl->backgroundThread.join();
//     pimpl->io.reset();
//     if(errorStatus())
//     {
//         throw(boost::system::system_error(boost::system::error_code(),
//                 "Error while closing the device"));
//     }
// }

// void AsyncSerial::write(const char *data, size_t size)
// {
//     {
//         lock_guard<mutex> l(pimpl->writeQueueMutex);
//         pimpl->writeQueue.insert(pimpl->writeQueue.end(),data,data+size);
//     }
//     pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
// }

// void AsyncSerial::write(const std::vector<char>& data)
// {
//     {
//         lock_guard<mutex> l(pimpl->writeQueueMutex);
//         pimpl->writeQueue.insert(pimpl->writeQueue.end(),data.begin(),
//                 data.end());
//     }
//     pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
// }

// void AsyncSerial::writeString(const std::string& s)
// {
//     {
//         lock_guard<mutex> l(pimpl->writeQueueMutex);
//         pimpl->writeQueue.insert(pimpl->writeQueue.end(),s.begin(),s.end());
//     }
//     pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
// }

// AsyncSerial::~AsyncSerial()
// {
//     if(isOpen())
//     {
//         try {
//             close();
//         } catch(...)
//         {
//             //Don't throw from a destructor
//         }
//     }
// }

// void AsyncSerial::doRead()
// {
//     pimpl->port.async_read_some(asio::buffer(pimpl->readBuffer,readBufferSize),
//             boost::bind(&AsyncSerial::readEnd,
//             this,
//             asio::placeholders::error,
//             asio::placeholders::bytes_transferred));
// }

// void AsyncSerial::readEnd(const boost::system::error_code& error,
//         size_t bytes_transferred)
// {
//     if(error)
//     {
//         #ifdef __APPLE__
//         if(error.value()==45)
//         {
//             //Bug on OS X, it might be necessary to repeat the setup
//             //http://osdir.com/ml/lib.boost.asio.user/2008-08/msg00004.html
//             doRead();
//             return;
//         }
//         #endif //__APPLE__
//         //error can be true even because the serial port was closed.
//         //In this case it is not a real error, so ignore
//         if(isOpen())
//         {
//             doClose();
//             setErrorStatus(true);
//         }
//     } else {
//         if(pimpl->callback) pimpl->callback(pimpl->readBuffer,
//                 bytes_transferred);
//         doRead();
//     }
// }

// void AsyncSerial::doWrite()
// {
//     //If a write operation is already in progress, do nothing
//     if(pimpl->writeBuffer==0)
//     {
//         lock_guard<mutex> l(pimpl->writeQueueMutex);
//         pimpl->writeBufferSize=pimpl->writeQueue.size();
//         pimpl->writeBuffer.reset(new char[pimpl->writeQueue.size()]);
//         copy(pimpl->writeQueue.begin(),pimpl->writeQueue.end(),
//                 pimpl->writeBuffer.get());
//         pimpl->writeQueue.clear();
//         async_write(pimpl->port,asio::buffer(pimpl->writeBuffer.get(),
//                 pimpl->writeBufferSize),
//                 boost::bind(&AsyncSerial::writeEnd, this,
//                 asio::placeholders::error));
//     }
// }

// void AsyncSerial::writeEnd(const boost::system::error_code& error)
// {
//     if(!error)
//     {
//         lock_guard<mutex> l(pimpl->writeQueueMutex);
//         if(pimpl->writeQueue.empty())
//         {
//             pimpl->writeBuffer.reset();
//             pimpl->writeBufferSize=0;

//             return;
//         }
//         pimpl->writeBufferSize=pimpl->writeQueue.size();
//         pimpl->writeBuffer.reset(new char[pimpl->writeQueue.size()]);
//         copy(pimpl->writeQueue.begin(),pimpl->writeQueue.end(),
//                 pimpl->writeBuffer.get());
//         pimpl->writeQueue.clear();
//         async_write(pimpl->port,asio::buffer(pimpl->writeBuffer.get(),
//                 pimpl->writeBufferSize),
//                 boost::bind(&AsyncSerial::writeEnd, this,
//                 asio::placeholders::error));
//     } else {
//         setErrorStatus(true);
//         doClose();
//     }
// }

// void AsyncSerial::doClose()
// {
//     boost::system::error_code ec;
//     pimpl->port.cancel(ec);
//     if(ec) setErrorStatus(true);
//     pimpl->port.close(ec);
//     if(ec) setErrorStatus(true);
// }

// void AsyncSerial::setErrorStatus(bool e)
// {
//     lock_guard<mutex> l(pimpl->errorMutex);
//     pimpl->error=e;
// }

// void AsyncSerial::setReadCallback(const std::function<void (const char*,
// size_t)>& callback)
// {
//     pimpl->callback=callback;
// }

// void AsyncSerial::clearReadCallback()
// {
//     std::function<void (const char*, size_t)> empty;
//     pimpl->callback.swap(empty);
// }

// //
// //Class BufferedAsyncSerial
// //

// BufferedAsyncSerial::BufferedAsyncSerial(): AsyncSerial()
// {
//     setReadCallback(std::bind(&BufferedAsyncSerial::readCallback, this, _1,
//     _2));
// }

// BufferedAsyncSerial::BufferedAsyncSerial(const std::string& devname,
//         unsigned int baud_rate,
//         asio::serial_port_base::parity opt_parity,
//         asio::serial_port_base::character_size opt_csize,
//         asio::serial_port_base::flow_control opt_flow,
//         asio::serial_port_base::stop_bits opt_stop)
//         :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
// {
//     setReadCallback(std::bind(&BufferedAsyncSerial::readCallback, this, _1,
//     _2));
// }

// size_t BufferedAsyncSerial::read(char *data, size_t size)
// {
//     lock_guard<mutex> l(readQueueMutex);
//     size_t result=min(size,readQueue.size());
//     vector<char>::iterator it=readQueue.begin()+result;
//     copy(readQueue.begin(),it,data);
//     readQueue.erase(readQueue.begin(),it);
//     return result;
// }

// std::vector<char> BufferedAsyncSerial::read()
// {
//     lock_guard<mutex> l(readQueueMutex);
//     vector<char> result;
//     result.swap(readQueue);
//     return result;
// }

// std::string BufferedAsyncSerial::readString()
// {
//     lock_guard<mutex> l(readQueueMutex);
//     string result(readQueue.begin(),readQueue.end());
//     readQueue.clear();
//     return result;
// }

// std::string BufferedAsyncSerial::readStringUntil(const std::string delim)
// {
//     lock_guard<mutex> l(readQueueMutex);
//     vector<char>::iterator it=findStringInVector(readQueue,delim);
//     if(it==readQueue.end()) return "";
//     string result(readQueue.begin(),it);
//     it+=delim.size();//Do remove the delimiter from the queue
//     readQueue.erase(readQueue.begin(),it);
//     return result;
// }

// void BufferedAsyncSerial::readCallback(const char *data, size_t len)
// {
//     lock_guard<mutex> l(readQueueMutex);
//     readQueue.insert(readQueue.end(),data,data+len);
// }

// std::vector<char>::iterator BufferedAsyncSerial::findStringInVector(
//         std::vector<char>& v,const std::string& s)
// {
//     if(s.size()==0) return v.end();

//     vector<char>::iterator it=v.begin();
//     for(;;)
//     {
//         vector<char>::iterator result=find(it,v.end(),s[0]);
//         if(result==v.end()) return v.end();//If not found return

//         for(size_t i=0;i<s.size();i++)
//         {
//             vector<char>::iterator temp=result+i;
//             if(temp==v.end()) return v.end();
//             if(s[i]!=*temp) goto mismatch;
//         }
//         //Found
//         return result;

//         mismatch:
//         it=result+1;
//     }
// }

// BufferedAsyncSerial::~BufferedAsyncSerial()
// {
//     clearReadCallback();
// }