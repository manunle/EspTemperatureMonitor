
/*
**
**  NTP
**
*/

#ifndef NTP_H
#define NTP_H

static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define LEAP_YEAR(Y) ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

WiFiUDP UDPNTPClient;		
long NTP_Time = 0;									// NTP Client

struct  strDateTime{
  byte hour;
  byte minute;
  byte second;
  int year;
  byte month;
  byte day;
  byte wday;
  unsigned long NTPtime;
} ;

strDateTime DateTime;                      // Global DateTime structure, will be refreshed every Second
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];

/***************************************************
/*
 * © Francesco Potortì 2013 - GPLv3 - Revision: 1.13
 *
 * Send an NTP packet and wait for the response, return the Unix time
 *
 * To lower the memory footprint, no buffers are allocated for sending
 * and receiving the NTP packets.  Four bytes of memory are allocated
 * for transmision, the rest is random garbage collected from the data
 * memory segment, and the received packet is read one byte at a time.
 * The Unix time is returned, that is, seconds from 1970-01-01T00:00.
 */
/*unsigned long inline ntpUnixTime ()
{
  Serial.println("Getting time....");
  static int udpInited = UDPNTPClient.begin(123); // open socket on arbitrary port

  const char timeServer[] = "1.ubuntu.pool.ntp.org";  // NTP server

  // Only the first four bytes of an outgoing NTP packet need to be set
  // appropriately, the rest can be whatever.
  const long ntpFirstFourBytes = 0xEC0600E3; // NTP request header

  // Fail if WiFiUdp.begin() could not init a socket
  if (! udpInited)
  {
    Serial.println("not inited");
    return 0;
  }

  // Clear received data from possible stray received packets
  UDPNTPClient.flush();

  // Send an NTP request
  if (! (UDPNTPClient.beginPacket(timeServer, 123) // 123 is the NTP port
	 && UDPNTPClient.write((byte *)&ntpFirstFourBytes, 48) == 48
	 && UDPNTPClient.endPacket()))
  {
    Serial.println("Sending request failed");
    return 0;				// sending request failed
  }
  // Wait for response; check every pollIntv ms up to maxPoll times
  const int pollIntv = 150;		// poll every this many ms
  const byte maxPoll = 15;		// poll up to this many times
  int pktLen;				// received packet length
  for (byte i=0; i<maxPoll; i++) {
    if ((pktLen = UDPNTPClient.parsePacket()) == 48)
      break;
    Serial.println("delaying");  
    delay(pollIntv);
  }
  Serial.println("got " + String(pktLen));
  if (pktLen != 48)
  {
    Serial.println("No correct packet received " + String(pktLen));
    return 0;				// no correct packet received
  }
  // Read and discard the first useless bytes
  // Set useless to 32 for speed; set to 40 for accuracy.
  const byte useless = 40;
  for (byte i = 0; i < useless; ++i)
    UDPNTPClient.read();

  // Read the integer part of sending time
  unsigned long time = UDPNTPClient.read();	// NTP time
  for (byte i = 1; i < 4; i++)
    time = time << 8 | UDPNTPClient.read();

  // Round to the nearest second if we want accuracy
  // The fractionary part is the next byte divided by 256: if it is
  // greater than 500ms we round to the next second; we also account
  // for an assumed network delay of 50ms, and (0.5-0.05)*256=115;
  // additionally, we account for how much we delayed reading the packet
  // since its arrival, which we assume on average to be pollIntv/2.
  time += (UDPNTPClient.read() > 115 - pollIntv/8);

  // Discard the rest of the packet
  UDPNTPClient.flush();
  Serial.println(String(time));
  return time - 2208988800ul;		// convert NTP time to Unix time
}
*/
/***************************************************/

void getNTPtime(){
  unsigned long _unixTime = 0;

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Getting time");
    UDPNTPClient.begin(2390);  // Port for NTP receive
    IPAddress timeServerIP;
    WiFi.hostByName(config.ntpServerName.c_str(), timeServerIP);

    //Serial.println("sending NTP packet...");
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;
    UDPNTPClient.beginPacket(timeServerIP, 123);
    UDPNTPClient.write(packetBuffer, NTP_PACKET_SIZE);
    UDPNTPClient.endPacket();

    delay(100);

    int cb = UDPNTPClient.parsePacket();
    if (cb == 0) {
      Serial.println("No NTP packet yet");
    }
    else
    {
      Serial.print("NTP packet received, length=");
      Serial.println(cb);
      UDPNTPClient.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      _unixTime = secsSince1900 - seventyYears;

    }
  } else {
    Serial.println("Internet yet not connected");
    delay(500);
  }
  yield();
  if (_unixTime > 0) UnixTimestamp = _unixTime; // store universally available time stamp
  Serial.println(String(UnixTimestamp));
}

strDateTime ConvertUnixTimeStamp( unsigned long _tempTimeStamp) {
  strDateTime _tempDateTime;
  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;

  time = (uint32_t)_tempTimeStamp;
  _tempDateTime.second = time % 60;
  time /= 60; // now it is minutes
  _tempDateTime.minute = time % 60;
  time /= 60; // now it is hours
  _tempDateTime.hour = time % 24;
  time /= 24; // now it is days
  _tempDateTime.wday = ((time + 4) % 7) + 1;  // Sunday is day 1

  year = 0;
  days = 0;
  while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  _tempDateTime.year = year; // year is offset from 1970

  days -= LEAP_YEAR(year) ? 366 : 365;
  time  -= days; // now it is days in this year, starting at 0

  days = 0;
  month = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++) {
    if (month == 1) { // february
      if (LEAP_YEAR(year)) {
        monthLength = 29;
      } else {
        monthLength = 28;
      }
    } else {
      monthLength = monthDays[month];
    }

    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }
  _tempDateTime.month = month + 1;  // jan is month 1
  _tempDateTime.day = time + 1;     // day of month
  _tempDateTime.year += 1970;

  return _tempDateTime;
}

//
// Summertime calculates the daylight saving time for middle Europe. Input: Unixtime in UTC
//
boolean summerTime(unsigned long _timeStamp ) {
  strDateTime  _tempDateTime = ConvertUnixTimeStamp(_timeStamp);
  // printTime("Innerhalb ", _tempDateTime);

  if (_tempDateTime.month < 3 || _tempDateTime.month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (_tempDateTime.month > 3 && _tempDateTime.month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (_tempDateTime.month == 3 && (_tempDateTime.hour + 24 * _tempDateTime.day) >= (3 +  24 * (31 - (5 * _tempDateTime.year / 4 + 4) % 7)) || _tempDateTime.month == 10 && (_tempDateTime.hour + 24 * _tempDateTime.day) < (3 +  24 * (31 - (5 * _tempDateTime.year / 4 + 1) % 7)))
    return true;
  else
    return false;
}

unsigned long adjustTimeZone(unsigned long _timeStamp, int _timeZone, bool _isDayLightSavingSaving) {
  _timeStamp += _timeZone *  360; // adjust timezone
  // printTime("Innerhalb adjustTimeZone ", ConvertUnixTimeStamp(_timeStamp));
  if (_isDayLightSavingSaving && summerTime(_timeStamp)) _timeStamp += 3600; // Sommerzeit beachten
  return _timeStamp;
}

void ISRsecondTick(){
  AdminTimeOutCounter++;
  cNTP_Update++;
  cHeartbeat++;
  TempTime++;
  UnixTimestamp++;
  absoluteActualTime = adjustTimeZone(UnixTimestamp, config.timeZone, config.isDayLightSaving);
  DateTime = ConvertUnixTimeStamp(absoluteActualTime);  //  convert to DateTime format
  actualTime = 3600 * DateTime.hour + 60 * DateTime.minute + DateTime.second;
//  Serial.println(String(DateTime.hour) + ":" + String(DateTime.minute) + ":" + String(DateTime.second) + "|" + String(cNTP_Update));

  if (millis() - customWatchdog > 30000){
    Serial.println("CustomWatchdog bites. Bye");
      //ESP.reset();
      ESP.restart();
  }
}

#endif
