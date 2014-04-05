////////////////////////////////////////////////////////////////////////////////
//
//  UdpPeer.h - Definition of HTTP Client Proxy
//
//  ----------------------
//
// This file is free software; you can redistribute it and/or modify
// it under the terms of either the GNU General Public License version 2
// or the GNU Lesser General Public License version 2.1, both as
// published by the Free Software Foundation.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef UdpPeer_H
#define UdpPeer_H

#include <Arduino.h>
#include <IPAddress.h>

#include "utility/W5100.h"
#include "utility/vinit.h"

////////////////////////////////////////////////////////////////////////////////

class UdpPeer
{
public:
  UdpPeer();
  virtual ~UdpPeer();

public:
  // Connection management functions
  bool                  open              (uint16_t the_srcPort, const IPAddress& the_dstIpAddr, uint16_t the_dstPort);
  bool                  close             ();
  bool                  isOpen            () const;
  void                  triggerConnTimeout();
  bool                  connTimeoutExpired() const;
  
  // Connection info functions
  W5100::socket_e       socket            () const;
  uint16_t              localPort         () const;
  uint16_t              remotePort        () const;
  IPAddress             remoteIpAddr      () const;
  W5100::mac_address_t  remoteMacAddr     () const;
  IPAddress             dstIpAddr         () const;
  uint16_t              dstPort           () const;

  // Low level read functions
  bool                  readByte          (uint8_t&);
  uint16_t              readBuffer        (uint8_t* the_buffer, uint16_t the_size);
  bool                  unreadByte        (uint8_t);
  bool                  peekByte          (uint8_t&);
  bool                  anyDataReceived   () const;
  void                  flushRead         ();
  uint32_t              totRead           () const { return my_totRead; }

  // Low level write functions
  bool                  writeByte         (uint8_t the_byte);
  uint16_t              writeBuffer       (uint8_t * the_buffer, uint16_t the_size);
  void                  flushWrite        ();
  uint32_t              totWrite          () const { return my_totWrite; }

private:
  bool                  prv_isValidSn     () const;
  
private:
  W5100::socket_e       my_sn;
  ext::vinit<uint8_t>   my_unreadByte;
  ext::vinit<bool>      my_unreadByteAvail;
  ext::vinit<uint32_t>  my_totRead;
  ext::vinit<uint32_t>  my_totWrite;
  ext::vinit<uint32_t>  my_connIdleStart;
  W5100::ipv4_address_t my_dstIpAddr;
  ext::vinit<uint16_t>  my_dstPort;
};

////////////////////////////////////////////////////////////////////////////////

#endif // #ifndef UdpPeer_H

