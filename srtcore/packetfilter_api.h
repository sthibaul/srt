/*
 * SRT - Secure, Reliable, Transport
 * Copyright (c) 2018 Haivision Systems Inc.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * 
 */

#ifndef INC__PACKETFILTER_API_H
#define INC__PACKETFILTER_API_H

enum SrtPktHeaderFields
{
    SRT_PH_SEQNO = 0, //< sequence number
    SRT_PH_MSGNO = 1, //< message number
    SRT_PH_TIMESTAMP = 2, //< time stamp
    SRT_PH_ID = 3,  //< socket ID

    // Must be the last value - this is size of all, not a field id
    SRT_PH__SIZE
};


enum SRT_ARQLevel
{
    SRT_ARQ_NEVER,   //< Never send LOSSREPORT
    SRT_ARQ_ONREQ,  //< Only record the loss, but report only those that are returned in receive()
    SRT_ARQ_ALWAYS, //< always send LOSSREPORT immediately after detecting a loss
};


struct SrtFilterConfig
{
    std::string type;
    std::map<std::string, std::string> parameters;
};

struct SrtFilterInitializer
{
    SRTSOCKET socket_id;
    int32_t snd_isn;
    int32_t rcv_isn;
    size_t payload_size;
};

struct SrtPacket
{
    uint32_t hdr[SRT_PH__SIZE];
    char buffer[SRT_LIVE_MAX_PLSIZE];
    size_t length;

    SrtPacket(size_t size): length(size)
    {
        memset(hdr, 0, sizeof(hdr));
    }

    uint32_t header(SrtPktHeaderFields field) { return hdr[field]; }
    char* data() { return buffer; }
    size_t size() { return length; }
};


bool ParseCorrectorConfig(std::string s, SrtFilterConfig& out);


class SrtPacketFilterBase
{
    SrtFilterInitializer initParams;

protected:

    SRTSOCKET socketID() { return initParams.socket_id; }
    int32_t sndISN() { return initParams.snd_isn; }
    int32_t rcvISN() { return initParams.rcv_isn; }
    size_t payloadSize() { return initParams.payload_size; }

    friend class PacketFilter;

    // Beside the size of the rows, special values:
    // 0: if you have 0 specified for rows, there are only columns
    // -1: Only during the handshake, use the value specified by peer.
    // -N: The N value still specifies the size, but in particular
    //     dimension there is no filter control packet formed nor expected.

public:

    typedef std::vector< std::pair<int32_t, int32_t> > loss_seqs_t;

protected:

    SrtPacketFilterBase(const SrtFilterInitializer& i): initParams(i)
    {
    }

    // General configuration
    virtual size_t extraSize() = 0;

    // Sender side

    // This function creates and stores the filter control packet with
    // a prediction to be immediately sent. This is called in the function
    // that normally is prepared for extracting a data packet from the sender
    // buffer and send it over the channel.
    virtual bool packControlPacket(SrtPacket& packet, int32_t seq) = 0;

    // This is called at the moment when the sender queue decided to pick up
    // a new packet from the scheduled packets. This should be then used to
    // continue filling the group, possibly followed by final calculating the
    // control packet ready to send.
    virtual void feedSource(CPacket& packet) = 0;


    // Receiver side

    // This function is called at the moment when a new data packet has
    // arrived (no matter if subsequent or recovered). The 'state' value
    // defines the configured level of loss state required to send the
    // loss report.
    virtual bool receive(const CPacket& pkt, loss_seqs_t& loss_seqs) = 0;

    // Backward configuration.
    // This should have some stable value after the configuration is parsed,
    // and it should be a stable value set ONCE, after the filter module is ready.
    virtual SRT_ARQLevel arqLevel() = 0;

    virtual ~SrtPacketFilterBase()
    {
    }
};



#endif