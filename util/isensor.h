/*
 * isensor.h
 * common routines from isensor.c 
 */
#define SDR_SZ    80     /*max SDR size*/

typedef struct {
  ushort recid;
  uchar  sdrver;  /*usu. 0x51 = v1.5*/
  uchar  rectype; /* 01, 02, 11, 12, c0 */
  uchar  reclen;
  uchar  sens_ownid;
  uchar  sens_ownlun;
  uchar  sens_num;  /*sdr[7]*/
  uchar  entity_id;
  uchar  entity_inst;
  uchar  sens_init; 
  uchar  sens_capab; 
  uchar  sens_type;  /*sdr[12]*/
  uchar  ev_type;    /*sdr[13]*/
  uchar  data1[6];   /*masks*/
  uchar  sens_units; /*sdr[20]*/
  uchar  sens_base;
  uchar  sens_mod;
  uchar  linear;
  uchar  m;
  uchar  m_t;
  uchar  b;
  uchar  b_a;
  uchar  a_ax;
  uchar  rx_bx;
  uchar  flags;
  uchar  nom_reading;
  uchar  norm_max;
  uchar  norm_min;
  uchar  sens_max_reading;
  uchar  sens_min_reading;
  uchar  unr_threshold;
  uchar  ucr_threshold;
  uchar  unc_threshold;
  uchar  lnr_threshold;
  uchar  lcr_threshold;
  uchar  lnc_threshold;
  uchar  pos_hysteresis;
  uchar  neg_hysteresis;
  uchar  data3[3];
  uchar  id_strlen;
  uchar  id_string[16];
  } SDR01REC;

typedef struct {
  ushort recid;
  uchar  sdrver;  /*usu. 0x51 = v1.5*/
  uchar  rectype; /* 01, 02, 11, 12, c0 */
  uchar  reclen;
  uchar  sens_ownid;
  uchar  sens_ownlun;
  uchar  sens_num;
  uchar  entity_id;
  uchar  entity_inst;
  uchar  sens_init; 
  uchar  sens_capab; 
  uchar  sens_type; 
  uchar  ev_type;
  uchar  data1[6];
  uchar  sens_units;
  uchar  sens_base;
  uchar  sens_mod;
  uchar  shar_cnt;
  uchar  shar_off;
  uchar  pos_hysteresis;
  uchar  neg_hysteresis;
  uchar  data2[4];
  uchar  id_strlen;
  uchar  id_string[16];
  } SDR02REC;

typedef struct {  /* 0x08 = Entity Association record */
  ushort recid;
  uchar  sdrver;  /*usu. 0x51 = v1.5*/
  uchar  rectype; /* 01, 02, 11, 12, c0 */
  uchar  reclen;
  uchar  contid;
  uchar  continst;
  uchar  flags;
  uchar  edata[8];
  } SDR08REC;

typedef struct {  /*0x14 = BMC Message Channel Info record */
  ushort recid;
  uchar  sdrver;  /*usu. 0x51 = v1.5*/
  uchar  rectype; /* 01, 02, 11, 12, c0 */
  uchar  reclen;
  uchar  mdata[8];
  uchar  mint;
  uchar  eint;
  uchar  rsvd;
  } SDR14REC;

typedef struct {  /*0x11 = FRU Locator*/
  ushort recid;
  uchar  sdrver;  /*usu. 0x51 = v1.5*/
  uchar  rectype; /* 01, 02, 11, 12, c0 */
  uchar  reclen;
  uchar  dev_access_adr; /*usu sa*/
  uchar  dev_slave_adr;  /*usu fru_id*/
  uchar  access_lun;
  uchar  chan_num;
  uchar  reserved;
  uchar  dev_type; 
  uchar  dev_typemod; 
  uchar  entity_id; 
  uchar  entity_inst; 
  uchar  oem; 
  uchar  id_strlen;
  uchar  id_string[16];
  } SDR11REC;

typedef struct {  /*0x12 = IPMB Locator, for MCs*/
  ushort recid;
  uchar  sdrver;  /*usu. 0x51 = v1.5*/
  uchar  rectype; /* 01, 02, 11, 12, c0 */
  uchar  reclen;
  uchar  dev_slave_adr;
  uchar  chan_num;
  uchar  power_state;
  uchar  dev_capab; 
  uchar  reserved[3]; 
  uchar  entity_id; 
  uchar  entity_inst; 
  uchar  oem; 
  uchar  id_strlen;
  uchar  id_string[16];
  } SDR12REC;

typedef struct {  /*0xc0 = OEM Record*/
  ushort recid;
  uchar  sdrver;  /*usu. 0x51 = v1.5*/
  uchar  rectype; /* 01, 02, 11, 12, c0 */
  uchar  reclen;
  uchar  manuf_id[3]; /*Intel = 0x57,0x01,0x00 = 343.*/
  uchar  oem_data[60]; /* (reclen-3 bytes)*/
  } SDRc0REC;

int  get_sdr_cache(uchar **pcache);  
void free_sdr_cache(uchar *pcache);
int  get_sdr_file(char *sdrfile, uchar **sdrlist);
int find_nsdrs(uchar *pcache);
int find_sdr_next(uchar *psdr, uchar *pcache, ushort id);
int find_sdr_by_snum(uchar *psdr, uchar *pcache, uchar snum, uchar sa);
int find_sdr_by_tag(uchar *psdr, uchar *pcache, char *tag, uchar dbg);
int find_sdr_by_id(uchar *psdr, uchar *pcache, ushort id);

void ShowSDR(char *tag, uchar *sdr);
int GetSDRRepositoryInfo(int *nret, int *fdev);
int GetSensorThresholds(uchar sens_num, uchar *data);
int GetSensorReading(uchar sens_num, void *psdr, uchar *sens_data);
int GetSensorReadingFactors(uchar snum, uchar raw, int *m, int *b, int *  b_exp,
            int *r, int *a);
double RawToFloat(uchar raw, uchar *psdr);
char *decode_entity_id(int id);
char *get_unit_type(uchar iunits, uchar ibase, uchar imod, int fshort);

/* 
 * decode_comp_reading
 * 
 * Decodes the readings from compact SDR sensors.
 * Use sensor_dstatus array for sensor reading types and meaning strings.
 * Refer to IPMI Table 36-1 and 36-2 for this.
 * Note that decoding should be based on sensor type and ev_type only,
 * except for end cases.
 * 
 * reading1 = sens_reading[2], reading2 = sens_reading[3]
 */
int decode_comp_reading(uchar type, uchar evtype, uchar num, 
		    uchar reading1, uchar reading2);

/* end isensor.h */
