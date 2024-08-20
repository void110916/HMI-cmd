#include "serialPort.h"
#include <string>
#include <algorithm>
#include <iostream>

// #define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/bind/bind.hpp>



using namespace std;
using namespace boost;

TimeoutSerial::TimeoutSerial(): io(), port(io), timer(io),
        timeout(boost::posix_time::seconds(0)) {}

TimeoutSerial::TimeoutSerial(const std::string& devname, unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
        : io(), port(io), timer(io), timeout(boost::posix_time::seconds(0))
{
    open(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop);
}

void TimeoutSerial::open(const std::string& devname, unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
{
    if(isOpen()) close();
    port.open(devname);
    port.set_option(asio::serial_port_base::baud_rate(baud_rate));
    port.set_option(opt_parity);
    port.set_option(opt_csize);
    port.set_option(opt_flow);
    port.set_option(opt_stop);
}

bool TimeoutSerial::isOpen() const
{
    return port.is_open();
}

void TimeoutSerial::close()
{
    if(isOpen()==false) return;
    port.close();
}

void TimeoutSerial::setTimeout(const int t)
{
    timeout=boost::posix_time::seconds(t);
}

void TimeoutSerial::write(const char *data, size_t size)
{
    asio::write(port,asio::buffer(data,size));
}

void TimeoutSerial::write(const std::vector<char>& data)
{
    asio::write(port,asio::buffer(&data[0],data.size()));
}

void TimeoutSerial::writeString(const std::string& s)
{
    asio::write(port,asio::buffer(s.c_str(),s.size()));
}

void TimeoutSerial::read(char *data, size_t size)
{
    if(readData.size()>0)//If there is some data from a previous read
    {
        istream is(&readData);
        size_t toRead=min(readData.size(),size);//How many bytes to read?
        is.read(data,toRead);
        data+=toRead;
        size-=toRead;
        if(size==0) return;//If read data was enough, just return
    }
    
    setupParameters=ReadSetupParameters(data,size);
    performReadSetup(setupParameters);

    //For this code to work, there should always be a timeout, so the
    //request for no timeout is translated into a very long timeout
    if(timeout!=boost::posix_time::seconds(0)) timer.expires_from_now(timeout);
    else timer.expires_from_now(boost::posix_time::hours(100000));
    
    timer.async_wait(boost::bind(&TimeoutSerial::timeoutExpired,this,
                asio::placeholders::error));
    
    result=resultInProgress;
    bytesTransferred=0;
    for(;;)
    {
        io.run_one();
        switch(result)
        {
            case resultSuccess:
                timer.cancel();
                return;
            case resultTimeoutExpired:
                port.cancel();
                throw(serialExcept("Timeout expired"));
            case resultError:
                timer.cancel();
                port.cancel();
                throw(boost::system::system_error(boost::system::error_code(),
                        "Error while reading"));
            //if resultInProgress remain in the loop
        }
    }
}

std::vector<char> TimeoutSerial::read(size_t size)
{
    vector<char> result(size,'\0');//Allocate a vector with the desired size
    read(&result[0],size);//Fill it with values
    return result;
}

std::string TimeoutSerial::readString(size_t size)
{
    string result(size,'\0');//Allocate a string with the desired size
    read(&result[0],size);//Fill it with values
    return result;
}

std::string TimeoutSerial::readStringUntil(const std::string& delim)
{
    // Note: if readData contains some previously read data, the call to
    // async_read_until (which is done in performReadSetup) correctly handles
    // it. If the data is enough it will also immediately call readCompleted()
    setupParameters=ReadSetupParameters(delim);
    performReadSetup(setupParameters);

    //For this code to work, there should always be a timeout, so the
    //request for no timeout is translated into a very long timeout
    if(timeout!=boost::posix_time::seconds(0)) timer.expires_from_now(timeout);
    else timer.expires_from_now(boost::posix_time::hours(100000));

    timer.async_wait(boost::bind(&TimeoutSerial::timeoutExpired,this,
                asio::placeholders::error));

    result=resultInProgress;
    bytesTransferred=0;
    for(;;)
    {
        io.run_one();
        switch(result)
        {
            case resultSuccess:
                {
                    timer.cancel();
                    bytesTransferred-=delim.size();//Don't count delim
                    istream is(&readData);
                    string result(bytesTransferred,'\0');//Alloc string
                    is.read(&result[0],bytesTransferred);//Fill values
                    is.ignore(delim.size());//Remove delimiter from stream
                    return result;
                }
            case resultTimeoutExpired:
                port.cancel();
                throw(serialExcept("Timeout expired"));
            case resultError:
                timer.cancel();
                port.cancel();
                throw(boost::system::system_error(boost::system::error_code(),
                        "Error while reading"));
            //if resultInProgress remain in the loop
        }
    }
}

TimeoutSerial::~TimeoutSerial() {}

void TimeoutSerial::performReadSetup(const ReadSetupParameters& param)
{
    if(param.fixedSize)
    {
        asio::async_read(port,asio::buffer(param.data,param.size),boost::bind(
                &TimeoutSerial::readCompleted,this,asio::placeholders::error,
                asio::placeholders::bytes_transferred));
    } else {
        asio::async_read_until(port,readData,param.delim,boost::bind(
                &TimeoutSerial::readCompleted,this,asio::placeholders::error,
                asio::placeholders::bytes_transferred));
    }
}

void TimeoutSerial::timeoutExpired(const boost::system::error_code& error)
{
     if(!error && result==resultInProgress) result=resultTimeoutExpired;
}

void TimeoutSerial::readCompleted(const boost::system::error_code& error,
        const size_t bytesTransferred)
{
    if(!error)
    {
        result=resultSuccess;
        this->bytesTransferred=bytesTransferred;
        return;
    }

    //In case a asynchronous operation is cancelled due to a timeout,
    //each OS seems to have its way to react.
    #ifdef _WIN32
    if(error.value()==995) return; //Windows spits out error 995
    #elif defined(__APPLE__)
    if(error.value()==45)
    {
        //Bug on OS X, it might be necessary to repeat the setup
        //http://osdir.com/ml/lib.boost.asio.user/2008-08/msg00004.html
        performReadSetup(setupParameters);
        return;
    }
    #else //Linux
    if(error.value()==125) return; //Linux outputs error 125
    #endif

    result=resultError;
}

// #include <iostream>


#ifdef _WIN32

#include <windows.h>

serialPort::serialPort(std::string portName)
    : serialPort(portName, CBR_38400, 8, NOPARITY, ONESTOPBIT) {}

serialPort::serialPort(std::string portName, int baudRate, int byteSize,
                       int parity, int stopBits)
    : _portName(portName),
      _baudRate(baudRate),
      _byteSize(byteSize),
      _parity(parity),
      _stopBits(stopBits) {
  _portName = "\\\\.\\" + _portName;

  __handle = CreateFileA(_portName.c_str(), GENERIC_READ | GENERIC_WRITE,
                         0,     //  must be opened with exclusive-access
                         NULL,  //  default security attributes
                         OPEN_EXISTING,         //  must use OPEN_EXISTING
                         FILE_FLAG_OVERLAPPED,  //  not overlapped I/O
                         NULL);  //  hTemplate must be NULL for comm devices

  if (__handle == INVALID_HANDLE_VALUE) throw serialExcept("Error opening");

  DCB dcb = {0};
  dcb.DCBlength = sizeof(DCB);

  if (!GetCommState(__handle, &dcb)) throw serialExcept("Error get setting");

  dcb.fBinary = TRUE;
  dcb.fAbortOnError = FALSE;
  dcb.fNull = FALSE;
  dcb.fErrorChar = FALSE;

  if (dcb.fDtrControl == DTR_CONTROL_HANDSHAKE)
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
  if (dcb.fRtsControl == RTS_CONTROL_HANDSHAKE)
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

  dcb.BaudRate = _baudRate;
  dcb.ByteSize = _byteSize;
  dcb.Parity = _parity;
  dcb.fParity = false;

  dcb.StopBits = _stopBits;

  dcb.fOutX = false;
  dcb.fInX = false;
  dcb.fOutxCtsFlow = false;

  if (!SetCommState(__handle, &dcb)) throw serialExcept("Error set setting");

  COMMTIMEOUTS timeout = {0};
  // if (!GetCommTimeouts(__handle, &timeout))
  //   throw serialExcept("Error get timeout");

  timeout.ReadIntervalTimeout = 50;
  timeout.ReadTotalTimeoutConstant = 50;
  timeout.ReadTotalTimeoutMultiplier = 10;
  timeout.WriteTotalTimeoutConstant = 50;
  timeout.WriteTotalTimeoutMultiplier = 10;

  if (!GetCommTimeouts(__handle, &timeout))
    throw serialExcept("Error get timeout");

  EscapeCommFunction(__handle, CLRDTR);  // qt setDataTerminalReady
  EscapeCommFunction(__handle, CLRRTS);  // qt setRequestToSend

  DWORD flags = PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR;  // qt clear
  PurgeComm(__handle, flags);
}

serialPort::~serialPort() { CloseHandle(__handle); }

bool serialPort::writeString(std::string str) {
  //   bool success = false;
  DWORD byteSend;
  if (!WriteFile(__handle, str.c_str(), str.size(), &byteSend, 0)) return false;
  return true;
}

std::string serialPort::readString() {
  std::string str("");
  // DWORD error;
  // COMSTAT status;
  DWORD bytesIn;
  // ClearCommError(__handle, &error, &status);
  // str.resize(status.cbInQue);
  str.resize(10);
  ReadFile(__handle, &str[0], 10, &bytesIn, NULL);//status.cbInQue
  
  return str;
}

#endif  //_WIN32

#include <thread>
// #include <mutex>
// #include <boost/bind.hpp>
#include <boost/shared_array.hpp>

using namespace std::placeholders;

class AsyncSerialImpl: private boost::noncopyable
{
public:
    AsyncSerialImpl(): io(), port(io), backgroundThread(), open(false),
            error(false) {}

    boost::asio::io_service io; ///< Io service object
    boost::asio::serial_port port; ///< Serial port object
    std::thread backgroundThread; ///< Thread that runs read/write operations
    bool open; ///< True if port open
    bool error; ///< Error flag
    mutable std::mutex errorMutex; ///< Mutex for access to error

    /// Data are queued here before they go in writeBuffer
    std::vector<char> writeQueue;
    boost::shared_array<char> writeBuffer; ///< Data being written
    size_t writeBufferSize; ///< Size of writeBuffer
    std::mutex writeQueueMutex; ///< Mutex for access to writeQueue
    char readBuffer[AsyncSerial::readBufferSize]; ///< data being read

    /// Read complete callback
    std::function<void (const char*, size_t)> callback;
};

AsyncSerial::AsyncSerial(): pimpl(new AsyncSerialImpl)
{

}

AsyncSerial::AsyncSerial(const std::string& devname, unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
        : pimpl(new AsyncSerialImpl)
{
    open(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop);
}

void AsyncSerial::open(const std::string& devname, unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
{
    if(isOpen()) close();

    setErrorStatus(true);//If an exception is thrown, error_ remains true
    pimpl->port.open(devname);
    pimpl->port.set_option(asio::serial_port_base::baud_rate(baud_rate));
    pimpl->port.set_option(opt_parity);
    pimpl->port.set_option(opt_csize);
    pimpl->port.set_option(opt_flow);
    pimpl->port.set_option(opt_stop);

    //This gives some work to the io_service before it is started
    pimpl->io.post(boost::bind(&AsyncSerial::doRead, this));

    thread t(boost::bind(&asio::io_service::run, &pimpl->io));
    pimpl->backgroundThread.swap(t);
    setErrorStatus(false);//If we get here, no error
    pimpl->open=true; //Port is now open
}

bool AsyncSerial::isOpen() const
{
    return pimpl->open;
}

bool AsyncSerial::errorStatus() const
{
    lock_guard<mutex> l(pimpl->errorMutex);
    return pimpl->error;
}

void AsyncSerial::close()
{
    if(!isOpen()) return;

    pimpl->open=false;
    pimpl->io.post(boost::bind(&AsyncSerial::doClose, this));
    pimpl->backgroundThread.join();
    pimpl->io.reset();
    if(errorStatus())
    {
        throw(boost::system::system_error(boost::system::error_code(),
                "Error while closing the device"));
    }
}

void AsyncSerial::write(const char *data, size_t size)
{
    {
        lock_guard<mutex> l(pimpl->writeQueueMutex);
        pimpl->writeQueue.insert(pimpl->writeQueue.end(),data,data+size);
    }
    pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
}

void AsyncSerial::write(const std::vector<char>& data)
{
    {
        lock_guard<mutex> l(pimpl->writeQueueMutex);
        pimpl->writeQueue.insert(pimpl->writeQueue.end(),data.begin(),
                data.end());
    }
    pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
}

void AsyncSerial::writeString(const std::string& s)
{
    {
        lock_guard<mutex> l(pimpl->writeQueueMutex);
        pimpl->writeQueue.insert(pimpl->writeQueue.end(),s.begin(),s.end());
    }
    pimpl->io.post(boost::bind(&AsyncSerial::doWrite, this));
}

AsyncSerial::~AsyncSerial()
{
    if(isOpen())
    {
        try {
            close();
        } catch(...)
        {
            //Don't throw from a destructor
        }
    }
}

void AsyncSerial::doRead()
{
    pimpl->port.async_read_some(asio::buffer(pimpl->readBuffer,readBufferSize),
            boost::bind(&AsyncSerial::readEnd,
            this,
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
}

void AsyncSerial::readEnd(const boost::system::error_code& error,
        size_t bytes_transferred)
{
    if(error)
    {
        #ifdef __APPLE__
        if(error.value()==45)
        {
            //Bug on OS X, it might be necessary to repeat the setup
            //http://osdir.com/ml/lib.boost.asio.user/2008-08/msg00004.html
            doRead();
            return;
        }
        #endif //__APPLE__
        //error can be true even because the serial port was closed.
        //In this case it is not a real error, so ignore
        if(isOpen())
        {
            doClose();
            setErrorStatus(true);
        }
    } else {
        if(pimpl->callback) pimpl->callback(pimpl->readBuffer,
                bytes_transferred);
        doRead();
    }
}

void AsyncSerial::doWrite()
{
    //If a write operation is already in progress, do nothing
    if(pimpl->writeBuffer==0)
    {
        lock_guard<mutex> l(pimpl->writeQueueMutex);
        pimpl->writeBufferSize=pimpl->writeQueue.size();
        pimpl->writeBuffer.reset(new char[pimpl->writeQueue.size()]);
        copy(pimpl->writeQueue.begin(),pimpl->writeQueue.end(),
                pimpl->writeBuffer.get());
        pimpl->writeQueue.clear();
        async_write(pimpl->port,asio::buffer(pimpl->writeBuffer.get(),
                pimpl->writeBufferSize),
                boost::bind(&AsyncSerial::writeEnd, this, asio::placeholders::error));
    }
}

void AsyncSerial::writeEnd(const boost::system::error_code& error)
{
    if(!error)
    {
        lock_guard<mutex> l(pimpl->writeQueueMutex);
        if(pimpl->writeQueue.empty())
        {
            pimpl->writeBuffer.reset();
            pimpl->writeBufferSize=0;
            
            return;
        }
        pimpl->writeBufferSize=pimpl->writeQueue.size();
        pimpl->writeBuffer.reset(new char[pimpl->writeQueue.size()]);
        copy(pimpl->writeQueue.begin(),pimpl->writeQueue.end(),
                pimpl->writeBuffer.get());
        pimpl->writeQueue.clear();
        async_write(pimpl->port,asio::buffer(pimpl->writeBuffer.get(),
                pimpl->writeBufferSize),
                boost::bind(&AsyncSerial::writeEnd, this, asio::placeholders::error));
    } else {
        setErrorStatus(true);
        doClose();
    }
}

void AsyncSerial::doClose()
{
    boost::system::error_code ec;
    pimpl->port.cancel(ec);
    if(ec) setErrorStatus(true);
    pimpl->port.close(ec);
    if(ec) setErrorStatus(true);
}

void AsyncSerial::setErrorStatus(bool e)
{
    lock_guard<mutex> l(pimpl->errorMutex);
    pimpl->error=e;
}

void AsyncSerial::setReadCallback(const std::function<void (const char*, size_t)>& callback)
{
    pimpl->callback=callback;
}

void AsyncSerial::clearReadCallback()
{
    std::function<void (const char*, size_t)> empty;
    pimpl->callback.swap(empty);
}

//
//Class BufferedAsyncSerial
//

BufferedAsyncSerial::BufferedAsyncSerial(): AsyncSerial()
{
    setReadCallback(std::bind(&BufferedAsyncSerial::readCallback, this, _1, _2));
}

BufferedAsyncSerial::BufferedAsyncSerial(const std::string& devname,
        unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
{
    setReadCallback(std::bind(&BufferedAsyncSerial::readCallback, this, _1, _2));
}

size_t BufferedAsyncSerial::read(char *data, size_t size)
{
    lock_guard<mutex> l(readQueueMutex);
    size_t result=min(size,readQueue.size());
    vector<char>::iterator it=readQueue.begin()+result;
    copy(readQueue.begin(),it,data);
    readQueue.erase(readQueue.begin(),it);
    return result;
}

std::vector<char> BufferedAsyncSerial::read()
{
    lock_guard<mutex> l(readQueueMutex);
    vector<char> result;
    result.swap(readQueue);
    return result;
}

std::string BufferedAsyncSerial::readString()
{
    lock_guard<mutex> l(readQueueMutex);
    string result(readQueue.begin(),readQueue.end());
    readQueue.clear();
    return result;
}

std::string BufferedAsyncSerial::readStringUntil(const std::string delim)
{
    lock_guard<mutex> l(readQueueMutex);
    vector<char>::iterator it=findStringInVector(readQueue,delim);
    if(it==readQueue.end()) return "";
    string result(readQueue.begin(),it);
    it+=delim.size();//Do remove the delimiter from the queue
    readQueue.erase(readQueue.begin(),it);
    return result;
}

void BufferedAsyncSerial::readCallback(const char *data, size_t len)
{
    lock_guard<mutex> l(readQueueMutex);
    readQueue.insert(readQueue.end(),data,data+len);
}

std::vector<char>::iterator BufferedAsyncSerial::findStringInVector(
        std::vector<char>& v,const std::string& s)
{
    if(s.size()==0) return v.end();

    vector<char>::iterator it=v.begin();
    for(;;)
    {
        vector<char>::iterator result=find(it,v.end(),s[0]);
        if(result==v.end()) return v.end();//If not found return

        for(size_t i=0;i<s.size();i++)
        {
            vector<char>::iterator temp=result+i;
            if(temp==v.end()) return v.end();
            if(s[i]!=*temp) goto mismatch;
        }
        //Found
        return result;

        mismatch:
        it=result+1;
    }
}

BufferedAsyncSerial::~BufferedAsyncSerial()
{
    clearReadCallback();
}