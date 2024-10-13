#pragma once
#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
class UdpPacketA
{
public:
    static const int BASE_HEADER_SIZE; 
    static const int MAX_SIZE_FOR_SENDING = 4096; //bytes
    static const int MAX_DATA_SIZE;

    //define types
    typedef unsigned char packetType_t;
    typedef uint32_t dataDistId_t;

    enum PacketType : packetType_t
    {
        ChunkRequest,
        FilePacket,
        PacketsRequest,
        Unknown
    };
    UdpPacketA(UdpPacketA &packet);

    //copies the buffer to a buffer of size == bytesReceived 
    //The buffer here should be the buffer filled by the udp listenter
    UdpPacketA(boost::asio::mutable_buffers_1 buffer, uint32_t bytesReceived);

    //Should only be used by derived class or for testing
    UdpPacketA(PacketType packetType, dataDistId_t dataDistId, uint32_t dataSize);
    ~UdpPacketA(); //this needs to be binded to asyc_send
    PacketType getPacketType();
    dataDistId_t getDataDistID();
    uint32_t getSize();
    boost::asio::mutable_buffers_1 getBuffer();

protected:    
    boost::shared_array<unsigned char> _data;

    template<typename t>
    void setInData(t value, int offset);
    template<typename t>
    t getFromData(int offset);

private:
    uint32_t _size;

    static const int PACKET_TYPE_SIZE = sizeof(packetType_t);
    static const int DATA_DIST_ID_SIZE = sizeof(dataDistId_t);
    void setPacketType(PacketType packetType);
    void setDataDistID(dataDistId_t dataDistId);
    void copyBuffer(boost::asio::mutable_buffers_1 &buffer, uint32_t bytesReceived);

};

template<typename t>
inline void UdpPacketA::setInData(t value, int offset)
{
    auto pointer = _data.get() + offset;
    ((t*)pointer)[0] = value;
}

template<typename t>
inline t UdpPacketA::getFromData(int offset)
{
    auto pointer = _data.get() + offset;
    return ((t*)pointer)[0];
}
