#include "UdpPacketA.h"

const int UdpPacketA::BASE_HEADER_SIZE = UdpPacketA::PACKET_TYPE_SIZE + UdpPacketA::DATA_DIST_ID_SIZE;
const int UdpPacketA::MAX_DATA_SIZE = MAX_SIZE_FOR_SENDING - BASE_HEADER_SIZE;

UdpPacketA::UdpPacketA(UdpPacketA & packet)
{
    this->_size = packet.getSize();
    this->_data = packet._data;
}

UdpPacketA::UdpPacketA(boost::asio::mutable_buffers_1 buffer, uint32_t bytesReceived)
{
    this->_size = bytesReceived;
    this->copyBuffer(buffer, bytesReceived);
}

UdpPacketA::UdpPacketA(PacketType packetType, dataDistId_t dataDistId, uint32_t dataSize)
{
    this->_size = dataSize + BASE_HEADER_SIZE;
    if (_size > MAX_SIZE_FOR_SENDING)
        throw new std::runtime_error("Data size exceeds MAX_SIZE");
    this->_data = boost::shared_array<unsigned char>(new unsigned char[_size]);
    this->setPacketType(packetType);
    this->setDataDistID(dataDistId);
}

UdpPacketA::~UdpPacketA()
{
}

UdpPacketA::PacketType UdpPacketA::getPacketType()
{
    return (PacketType)getFromData<packetType_t>(0);
}

UdpPacketA::dataDistId_t UdpPacketA::getDataDistID()
{
    return getFromData<dataDistId_t>(PACKET_TYPE_SIZE);
}

uint32_t UdpPacketA::getSize()
{
    return _size;
}

boost::asio::mutable_buffers_1 UdpPacketA::getBuffer()
{
    return boost::asio::mutable_buffers_1(_data.get(), _size);
}

//private:

void UdpPacketA::setPacketType(PacketType packetType)
{
    setInData((packetType_t)packetType, 0);
}

void UdpPacketA::setDataDistID(dataDistId_t dataDistId)
{
    setInData(dataDistId, PACKET_TYPE_SIZE);
}

void UdpPacketA::copyBuffer(boost::asio::mutable_buffers_1 & buffer, uint32_t bytesReceived)
{
    this->_data = boost::shared_array<unsigned char>(new unsigned char[bytesReceived]);
    boost::asio::mutable_buffers_1 buff = boost::asio::mutable_buffers_1(_data.get(), bytesReceived);
    boost::asio::buffer_copy(buff, buffer, bytesReceived);
}
