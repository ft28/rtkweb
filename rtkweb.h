#include <stdint.h>

#define MAX_NUM_SATS 72

typedef struct {
  uint32_t timestamp;
  uint16_t msec;
  uint16_t ratio;          /* v = x * 1000 */

  uint32_t llh_rover[3];
  uint32_t llh_base[3];
  int32_t enu_mm[3];
  int32_t vel_enu_mm[3];

  uint8_t num_sats;
  uint8_t num_rovers;
  uint8_t num_bases;
  uint8_t num_valid;

  uint16_t elevations[MAX_NUM_SATS]; /* v = (uint16_t)(x * 100) */
  uint16_t azimuthes[MAX_NUM_SATS];  /* v = (uint16_t)(x * 100) */
  uint8_t sats[MAX_NUM_SATS];

  uint8_t sat_rovers[MAX_NUM_SATS];
  uint8_t cn0_rovers[MAX_NUM_SATS];  /* v = x /0.25 */

  uint8_t sat_bases[MAX_NUM_SATS];
  uint8_t cn0_bases[MAX_NUM_SATS];   /* v = x / 0.25 */

  uint8_t dops[4];         /* v = x * 10  ,"GDOP/PDOP/HDOP/VDOP" */

  uint8_t sol;             /* 0: -, 1: fix, 2: float, 3: SBAS, 4: DGPS, 5: single, 6:ppp */      
  uint8_t state;           /* 0: stop, 1:run */
} Record;

void open_rtksvr(char* basedir, int _trace_level, int _outstat);
void close_rtksvr();

int start_rtksvr();
void stop_rtksvr();

void print_record(Record *record);
void update_record(Record *record);
