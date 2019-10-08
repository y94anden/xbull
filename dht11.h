#ifndef DHT11_H__
#define DHT11_H__

#include <stdint.h>

// Read temperature from DHT11 / DHT22. Returns data in supplied buffer:
// buf[0]: Humidity integral part
// buf[1]: Humidity decimal part
// buf[2]: Temperature integral part
// buf[3]: Temperature decimal part
// buf[4]: Checksum. Validated by dht_read()
//
// Returns 0 on success.
uint8_t dht_read(uint8_t buf[5]);

#endif
