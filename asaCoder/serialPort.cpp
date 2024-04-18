#include "serialPort.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

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

void TimeoutSerial::setTimeout(const boost::posix_time::time_duration& t)
{
    timeout=t;
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


// #ifdef _WIN32

// #include <windows.h>

// serialPort::serialPort(std::string portName)
//     : serialPort(portName, CBR_38400, 8, NOPARITY, ONESTOPBIT) {}

// serialPort::serialPort(std::string portName, int baudRate, int byteSize,
//                        int parity, int stopBits)
//     : _portName(portName),
//       _baudRate(baudRate),
//       _byteSize(byteSize),
//       _parity(parity),
//       _stopBits(stopBits) {
//   _portName = "\\\\.\\" + _portName;

//   __handle = CreateFileA(_portName.c_str(), GENERIC_READ | GENERIC_WRITE,
//                          0,     //  must be opened with exclusive-access
//                          NULL,  //  default security attributes
//                          OPEN_EXISTING,         //  must use OPEN_EXISTING
//                          FILE_FLAG_OVERLAPPED,  //  not overlapped I/O
//                          NULL);  //  hTemplate must be NULL for comm devices

//   if (__handle == INVALID_HANDLE_VALUE) throw serialExcept("Error opening");

//   DCB dcb = {0};
//   dcb.DCBlength = sizeof(DCB);

//   if (!GetCommState(__handle, &dcb)) throw serialExcept("Error get setting");

//   dcb.fBinary = TRUE;
//   dcb.fAbortOnError = FALSE;
//   dcb.fNull = FALSE;
//   dcb.fErrorChar = FALSE;

//   if (dcb.fDtrControl == DTR_CONTROL_HANDSHAKE)
//     dcb.fDtrControl = DTR_CONTROL_DISABLE;
//   if (dcb.fRtsControl == RTS_CONTROL_HANDSHAKE)
//     dcb.fRtsControl = RTS_CONTROL_DISABLE;

//   dcb.BaudRate = _baudRate;
//   dcb.ByteSize = _byteSize;
//   dcb.Parity = _parity;
//   dcb.fParity = false;

//   dcb.StopBits = _stopBits;

//   dcb.fOutX = false;
//   dcb.fInX = false;
//   dcb.fOutxCtsFlow = false;

//   if (!SetCommState(__handle, &dcb)) throw serialExcept("Error set setting");

//   COMMTIMEOUTS timeout = {0};
//   // if (!GetCommTimeouts(__handle, &timeout))
//   //   throw serialExcept("Error get timeout");

//   timeout.ReadIntervalTimeout = 50;
//   timeout.ReadTotalTimeoutConstant = 50;
//   timeout.ReadTotalTimeoutMultiplier = 10;
//   timeout.WriteTotalTimeoutConstant = 50;
//   timeout.WriteTotalTimeoutMultiplier = 10;

//   if (!GetCommTimeouts(__handle, &timeout))
//     throw serialExcept("Error get timeout");

//   EscapeCommFunction(__handle, CLRDTR);  // qt setDataTerminalReady
//   EscapeCommFunction(__handle, CLRRTS);  // qt setRequestToSend

//   DWORD flags = PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR;  // qt clear
//   PurgeComm(__handle, flags);
// }

// serialPort::~serialPort() { CloseHandle(__handle); }

// bool serialPort::writeString(std::string str) {
//   //   bool success = false;
//   DWORD byteSend;
//   if (!WriteFile(__handle, str.c_str(), str.size(), &byteSend, 0)) return false;
//   return true;
// }

// std::string serialPort::readString() {
//   std::string str("");
//   // DWORD error;
//   // COMSTAT status;
//   DWORD bytesIn;
//   // ClearCommError(__handle, &error, &status);
//   // str.resize(status.cbInQue);
//   str.resize(10);
//   ReadFile(__handle, &str[0], 10, &bytesIn, NULL);//status.cbInQue
  
//   return str;
// }

// #endif  //_WIN32