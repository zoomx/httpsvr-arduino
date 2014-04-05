///////////////////////////////////////////////////////////////////////////////
//
//  HttpSvr.h - Declaration of class HttpSvr
//  ECA 31Mar2013
//
//  ----------------------
//
//  HttpSvr is a small library for Arduino that implements a basic web server.
//  It uses Ethernet and SD libraries and provides a partial implementation of
//  HTTP/1.1 according to RFC 2616
//
//  ----------------------
//
//  This file is free software; you can redistribute it and/or modify
//  it under the terms of either the GNU General Public License version 2
//  or the GNU Lesser General Public License version 2.1, both as
//  published by the Free Software Foundation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HTTPSVR_H
#define HTTPSVR_H

#include <Arduino.h>
#include <IPAddress.h>

#include "HttpSvrDefs.h"
#include "ClientProxy.h"
#include "utility/SdSvr.h"

///////////////////////////////////////////////////////////////////////////////
// The class implementing the HTTP server

class HttpSvr
{
public:
  // HttpSvr uses SD card and the SD card library. You can choose to either let HttpSvr
  // initialize the SD library and relevant object for you, or initialize it by yourself,
  // before using it in HttpSvr.

  // The constructor and destructor methods.
  HttpSvr();
  virtual ~HttpSvr();

  // This version of method begin initializes Ethernet port but not SD card.
  // It should be used in case SD card is initialized somewhere else.
  // Parameter "the_port" lets you specify which is the TCP port number used by HttpSvr
  // to listen for clients. Usually it is port 80.
  /* void begin_wDHCP (const uint8_t* the_macAddress, const IPAddress& the_ipAddress, uint16_t the_port); */
  void begin_noDHCP(uint16_t the_port);
  
  // This version of method begin initializes SD card and Ethernet.
  // Here the SS and CS pin numbers are required for initialization of SD card library.
  // On the Ethernet Shield, CS is pin 4; SS pin is 10 on most Arduino boards, 53 on the Mega.
  // See relevant documentation for other boards.
  // See above for details on other parameters and the meaning of "_wDHCP" vs. "_noDHCP".
  /* void begin_wDHCP (int the_sdPinSS, int the_sdPinCS, const uint8_t* the_macAddress, const IPAddress& the_ipAddress, uint16_t the_port); */
  void begin_noDHCP(int the_sdPinSS,
                    int the_sdPinCS,
                    uint16_t the_port);
  
  // The function to be called on exit.                   
  void terminate();

public:
  // Resource Binding
  // Resources are the basic object of a HTTP request: any HTTP request message is basically
  // aimed to obtain something in response - a resource.
  // Resources are identified by a Unified Resource Identifier (URI), that is a string
  // whose format is described in detail in RFC 3986. A special case of URI is the URL, the Unified
  // Resource Locator, that is a resource identifier that indicates a location where the resource
  // can be found. URLs are widely used in the web browser's address bar, and are those strings
  // usually starting with "http://www.something.org/..."
  // In HttpSvr, each URL can be bound to a callback function that will be in charge of providing
  // the corresponding resource. This gives means for dynamic building of the resource when required,
  // e.g. reporting the current status of sensors or other information.
  // Resources that are not bound to any callback are searched as static HTML pages in the SD card's
  // file system.
  typedef bool (*url_callback_t)(ClientProxy&, http_e::method, const char *);
  bool            bindUrl               (const char * the_url, url_callback_t the_callback);
  bool            isUrlBound            (const char * the_url);
  bool            resetUrlBinding       (const char * the_url);
  void            resetAllBindings      ();
  
public:
  // Client connection management
  // HttpSvr waits for connections from clients. It can wait in several ways:
  // - it can wait forever, until a client connects; this way is called "blocking".
  // - it can just look for a client connection at a given moment and immediately
  //   return in any case; if a client is trying to connect, it will be served, otherwise
  //   HttpSvr just reports that no client is connecting. This way is called "non-blocking".
  // - it can wait for a given time interval. If a connection occurs in this interval, HttpSvr
  //   function returns with a valid client; if the time elapses without any connection, HttpSvr
  //   function returns reporting no connection.
  // Each function returns an ClientProxy whose method "isConnected" evaluates to "true"
  // if a connection has been detected, otherwise it evaluates to "false", meaning that it can
  // be tested in a statement like:
  //    ClientProxy aClient = pollClient_nonBlk();
  //    if (aClient.isConnected())
  //      // do things with aClient - it is connected
  static const unsigned long msTimeout_infinite = static_cast<unsigned long>(-1);
  ClientProxy      pollClient            (http_e::poll_type the_bt = http_e::poll_nonBlocking) const;
  ClientProxy      pollClient_nonBlk     () const;
  ClientProxy      pollClient_blk        (unsigned long the_msTimeout) const;
  void             resetConnection       (ClientProxy&) const;

  // The following top-level function can be used for extra-simple http connection management.
  // It can be used directly in the loop function and implements typical management of HTTP clients
  uint8_t          serveHttpConnections ();
  
public:
  // Request serving
  // Functions "serveRequest_XXX" are high-level entry points for serving a client request.
  // These functions read the message start line and call the resource provider (callback function)
  // bound to the URI contained therein, if any. If no resource provider has been bound,
  // a resource corresponding to the request-URI is searched on the SD card.
  // If non is found, a 404 Not Found is sent in response.
  // The other functions in this group are used inside serveRequest, but are made publicly available
  // as building blocks for alternative management of requests.
  // In "_GET" variant, only GET and HEAD request methods are served (this is the most commonly used)
  // In "_POST" variant, only POST and HEAD request methods are served (useful for upload and AJAX operation, but memory hungry)
  // In "_GETPOST" variant, GET, POST and HEAD request methods are served (useful for upload and AJAX operation, but memory hungry)
  // If you know in advance what kind of requests you are going to serve, call only the most specific function and do not
  // call the others. Doing this, you will allow the compiler wipe out functions that are not called, thus
  // reducing the code size.
  bool            serveRequest_GET       (ClientProxy&, char *, uint16_t);
  bool            serveRequest_POST      (ClientProxy&, char *, uint16_t);
  bool            serveRequest_GETPOST   (ClientProxy&, char *, uint16_t);
  bool            readRequestLine        (ClientProxy&, http_e::method&, char *, uint16_t) const;
  bool            dispatchRequest_GET    (ClientProxy&, http_e::method, const char *);
  bool            dispatchRequest_POST   (ClientProxy&, http_e::method, const char *);
  bool            dispatchRequest_GETPOST(ClientProxy&, http_e::method, const char *);
  bool            readNextHeader         (ClientProxy&, char *, uint16_t , char *, uint16_t) const;
  bool            skipNextHeader         (ClientProxy&) const;
  bool            skipHeaders            (ClientProxy&) const;
  uint16_t        skipToBody             (ClientProxy&) const;
  bool            sendResFile            (ClientProxy&, const char *);

  // Request-URI parse utilities - typically for internal use only
  const char *    uriFindEndOfPath        (const char *) const;
  const char *    uriFindStartOfQuery     (const char *) const;
  const char *    uriExtractFirstQueryNVP (const char *, char *, uint16_t, char *, uint16_t) const;
  const char *    uriExtractNextQueryNVP  (const char *, char *, uint16_t, char *, uint16_t) const;
  const char *    uriFindStartOfFragment  (const char *) const;
  
public:
  // Response generation utilities
  // These functions may be used as shortcuts for response generation, either for
  // errors or for success. Other fuctions can be added for generating other kinds of responses
  bool            sendResponse                    (ClientProxy&, const char *) const;
  bool            sendResponseOk                  (ClientProxy&) const;
  bool            sendResponseOkWithContent       (ClientProxy&, uint32_t) const;
  bool            sendResponseBadRequest          (ClientProxy&) const;
  bool            sendResponseNotFound            (ClientProxy&) const;
  bool            sendResponseMethodNotAllowed    (ClientProxy&) const;
  bool            sendResponseInternalServerError (ClientProxy&) const;
  bool            sendResponseRequestUriTooLarge  (ClientProxy&) const;
  
public:
  // Message analysis

  // TODO : Provide some utility for analyzing/serving AJAX requests
    
public:
  // Connection and status information
  IPAddress       localIpAddr           () const;

private:
  void            prv_resetSocket       (W5100::socket_e the_sn, uint16_t the_port) const;
  bool            prv_readRequestLine   (ClientProxy& the_client, http_e::method& the_method, char * the_urlBuffer, uint16_t the_bufferLen) const;
  uint8_t         prv_boundResIdx       (ClientProxy&, const char *);
  bool            prv_dispatchGET       (ClientProxy&, const char *);
  bool            prv_dispatchPOST      (ClientProxy&, const char *);
  bool            prv_sendString        (ClientProxy& the_client, const char * the_str) const;

private:
  struct res_fn_pair
  { 
    res_fn_pair() : crc(0), fn(0) {}
    res_fn_pair(uint16_t the_crc, url_callback_t the_fn) : crc(the_crc), fn(the_fn) {}
    ~res_fn_pair() {}
    
    uint16_t       crc;
    url_callback_t fn;
  };  
  
  static const uint8_t smy_resMap_size = 16;
  res_fn_pair          my_resMap[smy_resMap_size];
  SdSvr                my_sdSvr;
};

///////////////////////////////////////////////////////////////////////////////

#endif // #ifndef HTTPSVR_H

