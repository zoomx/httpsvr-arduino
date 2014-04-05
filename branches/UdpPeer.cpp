////////////////////////////////////////////////////////////////////////////////
//
//  UdpPeer.cpp - Definition of HTTP Client Proxy
//
//  ----------------------
//
// This file is free software; you can redistribute it and/or modify
// it under the terms of either the GNU General Public License version 2
// or the GNU Lesser General Public License version 2.1, both as
// published by the Free Software Foundation.
//
////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "UdpPeer.h"
#include "utility/W5100.h"

///////////////////////////////////////////////////////////////////////////////

UdpPeer::UdpPeer()
: my_sn(W5100::socket_undefined)
{}

UdpPeer::~UdpPeer()
{ close(); }

///////////////////////////////////////////////////////////////////////////////

bool UdpPeer::open(uint16_t the_srcPort, const IPAddress& the_dstIpAddr, uint16_t the_dstPort)
{
  // Check if this port is available
  for (uint8_t sn = W5100::socket_begin; sn < W5100::socket_end; ++sn)
  {
    if (W5100::isClosed(W5100::socket_cast(sn))) continue;
    uint16_t usedSrcPort = W5100::read_Sn_R16(W5100::socket_cast(sn), W5100_Sn_PORT);
    if (usedSrcPort == the_srcPort) return false;
  }

  // Try to open a socket with this srcPort
  for (uint8_t sn = W5100::socket_begin; sn < W5100::socket_end; ++sn)
  {
    switch (W5100::openUDP(W5100::socket_cast(sn), the_srcPort))
    {
    case W5100::rc_ok:
      my_sn = W5100::socket_cast(sn);
      my_dstIpAddr = the_dstIpAddr;
      my_dstPort = the_dstPort;
      return true;
      
    default:
      break;
    }
  }

  close();
  return false;
}

///////////////////////////////////////////////////////////////////////////////

bool UdpPeer::close()
{ 
  if (!prv_isValidSn()) return true;
  
  while (!W5100::isClosed(my_sn))
    if (W5100::close(my_sn) != W5100::rc_ok)
      return false;
      
  my_sn = W5100::socket_undefined;

  return true;
}

bool UdpPeer::isOpen() const
{
  if (!prv_isValidSn()) return false;
  return W5100::isOpen(my_sn);
}

void UdpPeer::triggerConnTimeout()
{ my_connIdleStart = millis(); }

bool UdpPeer::connTimeoutExpired() const
{ return millis() - my_connIdleStart > 5000; }

///////////////////////////////////////////////////////////////////////////////

W5100::socket_e UdpPeer::socket() const
{ return my_sn; }

uint16_t UdpPeer::localPort() const
{ return (prv_isValidSn() ? W5100::read_Sn_R16(my_sn, W5100_Sn_PORT) : 0); }

uint16_t UdpPeer::remotePort() const
{ return (prv_isValidSn() ? W5100::read_Sn_R16(my_sn, W5100_Sn_DPORT) : 0); }

IPAddress UdpPeer::remoteIpAddr() const
{
  W5100::ipv4_address_t ipAddr = prv_isValidSn() ? W5100::ipv4_address_t(my_sn) : W5100::ipv4_address_t(0,0,0,0);
  return IPAddress(ipAddr.ip0(), ipAddr.ip1(), ipAddr.ip2(), ipAddr.ip3());
}

W5100::mac_address_t UdpPeer::remoteMacAddr() const
{ return (prv_isValidSn() ? W5100::mac_address_t(my_sn) : W5100::mac_address_t(0,0,0,0,0,0)); }

IPAddress UdpPeer::dstIpAddr() const
{ return IPAddress(my_dstIpAddr.ip0(), my_dstIpAddr.ip1(), my_dstIpAddr.ip2(), my_dstIpAddr.ip3()); }

uint16_t UdpPeer::dstPort() const
{ return my_dstPort; }

///////////////////////////////////////////////////////////////////////////////

bool UdpPeer::readByte(uint8_t& the_byte)
{
  if (!prv_isValidSn()) return false;

  if (my_unreadByteAvail) 
  { 
    my_unreadByteAvail = false;
    the_byte = my_unreadByte;
    ++my_totRead;
    return true;; 
  }

  if (W5100::waitReceivePending(my_sn) != W5100::rc_ok)
  {
    close();
    return false;
  }

  uint16_t uRead = W5100::receive(my_sn, &the_byte, 1);
  my_totRead += uRead;
  return (uRead == 1);
}

uint16_t UdpPeer::readBuffer(uint8_t* the_buffer, uint16_t the_size)
{
  if (!prv_isValidSn()) return 0;
  if (!the_buffer || !the_size) return 0;
  
  uint16_t uRead = 0;
  if (my_unreadByteAvail)
  {
    my_unreadByteAvail = false;
    *the_buffer = my_unreadByte;
    the_buffer++;
    uRead++;
    the_size--;
  }
  
  if (W5100::waitReceivePending(my_sn) != W5100::rc_ok)
  { my_totRead += uRead; close(); return uRead; }
  
  uRead += W5100::receive(my_sn, the_buffer, the_size);
  my_totRead += uRead;
  return uRead;
}

bool UdpPeer::unreadByte(uint8_t the_byte)
{
  if (!prv_isValidSn()) return false;
  if (my_unreadByteAvail) return false;
  my_unreadByte = the_byte;
  my_unreadByteAvail = true;
  --my_totRead;
  return true;
}

bool UdpPeer::peekByte(uint8_t& the_byte)
{ return (readByte(the_byte) && unreadByte(the_byte)); }

bool UdpPeer::anyDataReceived() const
{
  if (!prv_isValidSn()) return false;
  return (W5100::checkReceivePending(my_sn) == W5100::rc_ok);
}

void UdpPeer::flushRead()
{
  if (!prv_isValidSn()) return;
  uint8_t c;
  while (W5100::checkReceivePending(my_sn) == W5100::rc_ok)
    readByte(c);
}

///////////////////////////////////////////////////////////////////////////////

bool UdpPeer::writeByte(uint8_t the_byte)
{
  if (!prv_isValidSn()) return false;
  if (!W5100::canTransmitData(my_sn)) { close(); return false; }
  if (W5100::send(my_sn, my_dstIpAddr, my_dstPort, &the_byte, 1) != 1) return false;
//  if (W5100::waitSendCompleted(my_sn) != W5100::rc_ok) { close(); return false; }
  ++my_totWrite;
  return true;
}

uint16_t UdpPeer::writeBuffer(uint8_t * the_buffer, uint16_t the_size)
{
  if (!prv_isValidSn()) return 0;
  if (!W5100::canTransmitData(my_sn)) { close(); return 0; }
  the_size = W5100::send(my_sn, my_dstIpAddr, my_dstPort, the_buffer, the_size);
//  if (W5100::waitSendCompleted(my_sn) != W5100::rc_ok) { close(); return 0; }
  my_totWrite += the_size;
  return the_size;
}

void UdpPeer::flushWrite()
{
  if (prv_isValidSn()) return;
  if (W5100::waitSendCompleted(my_sn) != W5100::rc_ok) close();
}
  
///////////////////////////////////////////////////////////////////////////////

bool UdpPeer::prv_isValidSn() const
{ return my_sn != W5100::socket_undefined; }

///////////////////////////////////////////////////////////////////////////////

