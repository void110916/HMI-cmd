#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <array>
#include <boost/asio.hpp>
// #include <boost/bind.hpp>
#include <boost/utility.hpp>
#include <chrono>
#include <future>
#include <thread>
// #include <stdexcept>

class SerialPort {
 private:
  boost::asio::io_context ioc_;
  boost::asio::io_context::work work_;
  boost::asio::serial_port port_;

#define READBUFFERSIZE 256
  std::array<char, READBUFFERSIZE> rbuf_;

  std::vector<char> read_buffer_;
  std::unique_ptr<std::thread> asyncReadThread;
  mutable std::mutex readBufferMtx;
  void asyncRead();
  void asyncReadHandler(boost::system::error_code const& error,
                        size_t bytes_transferred);
  int16_t readByte();
  std::vector<char> readBuffer(size_t len);
  std::vector<char> readBufferTimeout(size_t len);

  mutable std::mutex errMtx;
  int error_value{};
  void setError(const int error_value);
  int getError() const;

  unsigned int timeoutVal = 60000;

  mutable std::mutex writeMtx;
  std::condition_variable writeCv;
  bool writeLocked = false;
  void asyncWriteHandler(const boost::system::error_code& error,
                         std::size_t bytes_transferred);

 public:
  SerialPort();
  ~SerialPort();
  SerialPort& operator=(const SerialPort&) = delete;
  SerialPort& operator=(SerialPort&&) = delete;

  void open(const std::string& port_name, uint32_t baudrate = 38400);
  bool isOpen() const;
  void close();
  size_t write(const uint8_t* buffer, size_t size);
  size_t write(std::vector<uint8_t> const& data);
  size_t writeAsync(const uint8_t* buffer, size_t size);
  size_t writeAsync(std::string str);
  std::future<std::vector<char>> readAsync(const size_t size = READBUFFERSIZE);
  std::future<std::vector<char>> readAsync(const size_t size,
                                              unsigned int timeout);
};

// blocking serialport
// class serialExcept : public std::runtime_error {
//  public:
//   serialExcept() : std::runtime_error("serialExcept") {}
//   serialExcept(const std::string& msg) : std::runtime_error(msg) {}
// };

// class TimeoutSerial : private boost::noncopyable {
//  public:
//   TimeoutSerial();

//   /**
//    * Opens a serial device. By default timeout is disabled.
//    * \param devname serial device name, example "/dev/ttyS0" or "COM1"
//    * \param baud_rate serial baud rate
//    * \param opt_parity serial parity, default none
//    * \param opt_csize serial character size, default 8bit
//    * \param opt_flow serial flow control, default none
//    * \param opt_stop serial stop bits, default 1
//    * \throws boost::system::system_error if cannot open the
//    * serial device
//    */
//   TimeoutSerial(const std::string& devname, unsigned int baud_rate,
//                 boost::asio::serial_port_base::parity opt_parity =
//                     boost::asio::serial_port_base::parity(
//                         boost::asio::serial_port_base::parity::none),
//                 boost::asio::serial_port_base::character_size opt_csize =
//                     boost::asio::serial_port_base::character_size(8),
//                 boost::asio::serial_port_base::flow_control opt_flow =
//                     boost::asio::serial_port_base::flow_control(
//                         boost::asio::serial_port_base::flow_control::none),
//                 boost::asio::serial_port_base::stop_bits opt_stop =
//                     boost::asio::serial_port_base::stop_bits(
//                         boost::asio::serial_port_base::stop_bits::one));

//   /**
//    * Opens a serial device.
//    * \param devname serial device name, example "/dev/ttyS0" or "COM1"
//    * \param baud_rate serial baud rate
//    * \param opt_parity serial parity, default none
//    * \param opt_csize serial character size, default 8bit
//    * \param opt_flow serial flow control, default none
//    * \param opt_stop serial stop bits, default 1
//    * \throws boost::system::system_error if cannot open the
//    * serial device
//    */
//   void open(const std::string& devname, unsigned int baud_rate,
//             boost::asio::serial_port_base::parity opt_parity =
//                 boost::asio::serial_port_base::parity(
//                     boost::asio::serial_port_base::parity::none),
//             boost::asio::serial_port_base::character_size opt_csize =
//                 boost::asio::serial_port_base::character_size(8),
//             boost::asio::serial_port_base::flow_control opt_flow =
//                 boost::asio::serial_port_base::flow_control(
//                     boost::asio::serial_port_base::flow_control::none),
//             boost::asio::serial_port_base::stop_bits opt_stop =
//                 boost::asio::serial_port_base::stop_bits(
//                     boost::asio::serial_port_base::stop_bits::one));

//   /**
//    * \return true if serial device is open
//    */
//   bool isOpen() const;

//   /**
//    * Close the serial device
//    * \throws boost::system::system_error if any error
//    */
//   void close();

//   /**
//    * Set the timeout on read/write operations.
//    * To disable the timeout, call setTimeout(0);
//    */
//   void setTimeout(const int t);

//   /**
//    * Write data
//    * \param data array of char to be sent through the serial device
//    * \param size array size
//    * \throws boost::system::system_error if any error
//    */
//   void write(const char* data, size_t size);

//   /**
//    * Write data
//    * \param data to be sent through the serial device
//    * \throws boost::system::system_error if any error
//    */
//   void write(const std::vector<char>& data);

//   /**
//    * Write a string. Can be used to send ASCII data to the serial device.
//    * To send binary data, use write()
//    * \param s string to send
//    * \throws boost::system::system_error if any error
//    */
//   void writeString(const std::string& s);

//   /**
//    * Read some data, blocking
//    * \param data array of char to be read through the serial device
//    * \param size array size
//    * \return numbr of character actually read 0<=return<=size
//    * \throws boost::system::system_error if any error
//    * \throws timeout_exception in case of timeout
//    */
//   void read(char* data, size_t size);

//   /**
//    * Read some data, blocking
//    * \param size how much data to read
//    * \return the receive buffer. It iempty if no data is available
//    * \throws boost::system::system_error if any error
//    * \throws timeout_exception in case of timeout
//    */
//   std::vector<char> read(size_t size);

//   /**
//    * Read a string, blocking
//    * Can only be used if the user is sure that the serial device will not
//    * send binary data. For binary data read, use read()
//    * The returned string is empty if no data has arrived
//    * \param size hw much data to read
//    * \return a string with the received data.
//    * \throws boost::system::system_error if any error
//    * \throws timeout_exception in case of timeout
//    */
//   std::string readString(size_t size);

//   /**
//    * Read a line, blocking
//    * Can only be used if the user is sure that the serial device will not
//    * send binary data. For binary data read, use read()
//    * The returned string is empty if the line delimiter has not yet arrived.
//    * \param delimiter line delimiter, default="\n"
//    * \return a string with the received data. The delimiter is removed from
//    * the string.
//    * \throws boost::system::system_error if any error
//    * \throws timeout_exception in case of timeout
//    */
//   std::string readStringUntil(const std::string& delim = "\n");

//   ~TimeoutSerial();

//  private:
//   /**
//    * Parameters of performReadSetup.
//    * Just wrapper class, no encapsulation provided
//    */
//   class ReadSetupParameters {
//    public:
//     ReadSetupParameters() : fixedSize(false), delim(""), data(0), size(0) {}

//     explicit ReadSetupParameters(const std::string& delim)
//         : fixedSize(false), delim(delim), data(0), size(0) {}

//     ReadSetupParameters(char* data, size_t size)
//         : fixedSize(true), delim(""), data(data), size(size) {}

//     // Using default copy constructor, operator=

//     bool fixedSize;     ///< True if need to read a fixed number of
//     parameters std::string delim;  ///< String end delimiter (valid if
//     fixedSize=false) char* data;         ///< Pointer to data array (valid if
//     fixedSize=true) size_t size;        ///< Array size (valid if
//     fixedSize=true)
//   };

//   /**
//    * This member function sets up a read operation, both reading a specified
//    * number of characters and reading until a delimiter string.
//    */
//   void performReadSetup(const ReadSetupParameters& param);

//   /**
//    * Callack called either when the read timeout is expired or canceled.
//    * If called because timeout expired, sets result to resultTimeoutExpired
//    */
//   void timeoutExpired(const boost::system::error_code& error);

//   /**
//    * Callback called either if a read complete or read error occurs
//    * If called because of read complete, sets result to resultSuccess
//    * If called because read error, sets result to resultError
//    */
//   void readCompleted(const boost::system::error_code& error,
//                      const size_t bytesTransferred);

//   /**
//    * Possible outcome of a read. Set by callbacks, read from main code
//    */
//   enum ReadResult {
//     resultInProgress,
//     resultSuccess,
//     resultError,
//     resultTimeoutExpired
//   };

//   boost::asio::io_service io;                ///< Io service object
//   boost::asio::serial_port port;             ///< Serial port object
//   boost::asio::deadline_timer timer;         ///< Timer for timeout
//   boost::posix_time::time_duration timeout;  ///< Read/write timeout
//   boost::asio::streambuf readData;  ///< Holds eventual read but not consumed
//   enum ReadResult result;           ///< Used by read with timeout
//   size_t bytesTransferred;          ///< Used by async read callback
//   ReadSetupParameters setupParameters;  ///< Global because used in the OSX
//   fix
// };

// // simple serialport
// class serialPort {
//  protected:
//   void* __handle;

//  private:
//   std::string _portName;
//   int _baudRate;
//   int _byteSize;
//   int _parity;
//   int _stopBits;

//  public:
//   // serialPort();
//   serialPort(std::string portName);
//   serialPort(std::string portName, int baudRate, int byteSize, int parity,
//              int stopBits);

//   ~serialPort();
//   bool writeString(std::string str);
//   std::string readString();
// };

// // unblocking serialport
// #include <vector>
// #include <memory>
// #include <functional>

// /**
//  * Used internally (pimpl)
//  */
// class AsyncSerialImpl;

// /**
//  * Asyncronous serial class.
//  * Intended to be a base class.
//  */
// class AsyncSerial: private boost::noncopyable
// {
// public:
//     AsyncSerial();

//     /**
//      * Constructor. Creates and opens a serial device.
//      * \param devname serial device name, example "/dev/ttyS0" or "COM1"
//      * \param baud_rate serial baud rate
//      * \param opt_parity serial parity, default none
//      * \param opt_csize serial character size, default 8bit
//      * \param opt_flow serial flow control, default none
//      * \param opt_stop serial stop bits, default 1
//      * \throws boost::system::system_error if cannot open the
//      * serial device
//      */
//     AsyncSerial(const std::string& devname, unsigned int baud_rate,
//         boost::asio::serial_port_base::parity opt_parity=
//             boost::asio::serial_port_base::parity(
//                 boost::asio::serial_port_base::parity::none),
//         boost::asio::serial_port_base::character_size opt_csize=
//             boost::asio::serial_port_base::character_size(8),
//         boost::asio::serial_port_base::flow_control opt_flow=
//             boost::asio::serial_port_base::flow_control(
//                 boost::asio::serial_port_base::flow_control::none),
//         boost::asio::serial_port_base::stop_bits opt_stop=
//             boost::asio::serial_port_base::stop_bits(
//                 boost::asio::serial_port_base::stop_bits::one));

//     /**
//     * Opens a serial device.
//     * \param devname serial device name, example "/dev/ttyS0" or "COM1"
//     * \param baud_rate serial baud rate
//     * \param opt_parity serial parity, default none
//     * \param opt_csize serial character size, default 8bit
//     * \param opt_flow serial flow control, default none
//     * \param opt_stop serial stop bits, default 1
//     * \throws boost::system::system_error if cannot open the
//     * serial device
//     */
//     void open(const std::string& devname, unsigned int baud_rate,
//         boost::asio::serial_port_base::parity opt_parity=
//             boost::asio::serial_port_base::parity(
//                 boost::asio::serial_port_base::parity::none),
//         boost::asio::serial_port_base::character_size opt_csize=
//             boost::asio::serial_port_base::character_size(8),
//         boost::asio::serial_port_base::flow_control opt_flow=
//             boost::asio::serial_port_base::flow_control(
//                 boost::asio::serial_port_base::flow_control::none),
//         boost::asio::serial_port_base::stop_bits opt_stop=
//             boost::asio::serial_port_base::stop_bits(
//                 boost::asio::serial_port_base::stop_bits::one));

//     /**
//      * \return true if serial device is open
//      */
//     bool isOpen() const;

//     /**
//      * \return true if error were found
//      */
//     bool errorStatus() const;

//     /**
//      * Close the serial device
//      * \throws boost::system::system_error if any error
//      */
//     void close();

//     /**
//      * Write data asynchronously. Returns immediately.
//      * \param data array of char to be sent through the serial device
//      * \param size array size
//      */
//     void write(const char *data, size_t size);

//      /**
//      * Write data asynchronously. Returns immediately.
//      * \param data to be sent through the serial device
//      */
//     void write(const std::vector<char>& data);

//     /**
//     * Write a string asynchronously. Returns immediately.
//     * Can be used to send ASCII data to the serial device.
//     * To send binary data, use write()
//     * \param s string to send
//     */
//     void writeString(const std::string& s);

//     virtual ~AsyncSerial()=0;

//     /**
//      * Read buffer maximum size
//      */
//     static const int readBufferSize=512;
// private:

//     /**
//      * Callback called to start an asynchronous read operation.
//      * This callback is called by the io_service in the spawned thread.
//      */
//     void doRead();

//     /**
//      * Callback called at the end of the asynchronous operation.
//      * This callback is called by the io_service in the spawned thread.
//      */
//     void readEnd(const boost::system::error_code& error,
//         size_t bytes_transferred);

//     /**
//      * Callback called to start an asynchronous write operation.
//      * If it is already in progress, does nothing.
//      * This callback is called by the io_service in the spawned thread.
//      */
//     void doWrite();

//     /**
//      * Callback called at the end of an asynchronuous write operation,
//      * if there is more data to write, restarts a new write operation.
//      * This callback is called by the io_service in the spawned thread.
//      */
//     void writeEnd(const boost::system::error_code& error);

//     /**
//      * Callback to close serial port
//      */
//     void doClose();

//     std::shared_ptr<AsyncSerialImpl> pimpl;

// protected:

//     /**
//      * To allow derived classes to report errors
//      * \param e error status
//      */
//     void setErrorStatus(bool e);

//     /**
//      * To allow derived classes to set a read callback
//      */
//     void setReadCallback(const std::function<void (const char*, size_t)>&
//     callback);

//     /**
//      * To unregister the read callback in the derived class destructor so it
//      * does not get called after the derived class destructor but before the
//      * base class destructor
//      */
//     void clearReadCallback();

// };

// #include <mutex>
// class BufferedAsyncSerial: public AsyncSerial
// {
// public:
//     BufferedAsyncSerial();

//     /**
//     * Opens a serial device.
//     * \param devname serial device name, example "/dev/ttyS0" or "COM1"
//     * \param baud_rate serial baud rate
//     * \param opt_parity serial parity, default none
//     * \param opt_csize serial character size, default 8bit
//     * \param opt_flow serial flow control, default none
//     * \param opt_stop serial stop bits, default 1
//     * \throws boost::system::system_error if cannot open the
//     * serial device
//     */
//     BufferedAsyncSerial(const std::string& devname, unsigned int baud_rate,
//         boost::asio::serial_port_base::parity opt_parity=
//             boost::asio::serial_port_base::parity(
//                 boost::asio::serial_port_base::parity::none),
//         boost::asio::serial_port_base::character_size opt_csize=
//             boost::asio::serial_port_base::character_size(8),
//         boost::asio::serial_port_base::flow_control opt_flow=
//             boost::asio::serial_port_base::flow_control(
//                 boost::asio::serial_port_base::flow_control::none),
//         boost::asio::serial_port_base::stop_bits opt_stop=
//             boost::asio::serial_port_base::stop_bits(
//                 boost::asio::serial_port_base::stop_bits::one));

//     /**
//      * Read some data asynchronously. Returns immediately.
//      * \param data array of char to be read through the serial device
//      * \param size array size
//      * \return numbr of character actually read 0<=return<=size
//      */
//     size_t read(char *data, size_t size);

//     /**
//      * Read all available data asynchronously. Returns immediately.
//      * \return the receive buffer. It iempty if no data is available
//      */
//     std::vector<char> read();

//     /**
//      * Read a string asynchronously. Returns immediately.
//      * Can only be used if the user is sure that the serial device will not
//      * send binary data. For binary data read, use read()
//      * The returned string is empty if no data has arrived
//      * \return a string with the received data.
//      */
//     std::string readString();

//      /**
//      * Read a line asynchronously. Returns immediately.
//      * Can only be used if the user is sure that the serial device will not
//      * send binary data. For binary data read, use read()
//      * The returned string is empty if the line delimiter has not yet
//      arrived.
//      * \param delimiter line delimiter, default='\n'
//      * \return a string with the received data. The delimiter is removed from
//      * the string.
//      */
//     std::string readStringUntil(const std::string delim="\n");

//     virtual ~BufferedAsyncSerial();

// private:

//     /**
//      * Read callback, stores data in the buffer
//      */
//     void readCallback(const char *data, size_t len);

//     /**
//      * Finds a substring in a vector of char. Used to look for the delimiter.
//      * \param v vector where to find the string
//      * \param s string to find
//      * \return the beginning of the place in the vector where the first
//      * occurrence of the string is, or v.end() if the string was not found
//      */
//     static std::vector<char>::iterator findStringInVector(std::vector<char>&
//     v,
//             const std::string& s);

//     std::vector<char> readQueue;
//     std::mutex readQueueMutex;
// };

#endif  // SERIALPORT_H

// #include <string>
