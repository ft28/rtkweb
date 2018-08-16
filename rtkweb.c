#include <unistd.h>

#include "rtklib.h"
#include "rtkweb.h"


static const char rcsid[]="$Id:$";

#define MAXCMD      256                 /* max length of a command */
#define MAXSTR      1024                /* max length of a stream */
#define MAXRCVCMD   4096                /* max receiver command */
#define OPTSDIR     "."                 /* default config directory */
#define OPTSFILE    "rtkweb.conf"       /* default config file */
#define NAVIFILE    "rtkweb.nav"        /* navigation save file */
#define STATFILE    "rtkweb_%Y%m%d%h%M.stat"  /* solution status file */
#define TRACEFILE   "rtkweb_%Y%m%d%h%M.trace" /* debug trace file */
#define INTKEEPALIVE 1000               /* keep alive interval (ms) */


#define SQRT(x)     ((x)<=0.0?0.0:sqrt(x))

/* function prototypes -------------------------------------------------------*/
extern FILE *popen(const char *, const char *);
extern int pclose(FILE *);

/* global variables ----------------------------------------------------------*/
static rtksvr_t svr;                    /* rtk server struct */

static int strtype[]={                  /* stream types */
    STR_SERIAL,STR_NONE,STR_NONE,STR_NONE,STR_NONE,STR_NONE,STR_NONE,STR_NONE
};
static char strpath[8][MAXSTR]={""};    /* stream paths */
static int strfmt[]={                   /* stream formats */
    STRFMT_UBX,STRFMT_RTCM3,STRFMT_SP3,SOLF_LLH,SOLF_NMEA
};
static int svrcycle     =10;            /* server cycle (ms) */
static int timeout      =10000;         /* timeout time (ms) */
static int reconnect    =10000;         /* reconnect interval (ms) */
static int nmeacycle    =5000;          /* nmea request cycle (ms) */
static int buffsize     =32768;         /* input buffer size (bytes) */
static int navmsgsel    =0;             /* navigation mesaage select */
static char proxyaddr[256]="";          /* http/ntrip proxy */
static int nmeareq      =0;             /* nmea request type (0:off,1:lat/lon,2:single) */
static double nmeapos[] ={0,0};         /* nmea position (lat/lon) (deg) */
static char rcvcmds[3][MAXSTR]={""};    /* receiver commands files */
static int modflgr[256] ={0};           /* modified flags of receiver options */
static int modflgs[256] ={0};           /* modified flags of system options */
static char startcmd[MAXSTR]="";        /* start command */
static char stopcmd [MAXSTR]="";        /* stop command */
static int fswapmargin  =30;            /* file swap margin (s) */

static prcopt_t prcopt;                 /* processing options */
static solopt_t solopt[2]={{0}};        /* solution options */
static filopt_t filopt  ={""};          /* file options */

/* receiver options table ----------------------------------------------------*/
#define TIMOPT  "0:gpst,1:utc,2:jst,3:tow"
#define CONOPT  "0:dms,1:deg,2:xyz,3:enu,4:pyl"
#define FLGOPT  "0:off,1:std+2:age/ratio/ns"
#define ISTOPT  "0:off,1:serial,2:file,3:tcpsvr,4:tcpcli,7:ntripcli,8:ftp,9:http"
#define OSTOPT  "0:off,1:serial,2:file,3:tcpsvr,4:tcpcli,6:ntripsvr"
#define FMTOPT  "0:rtcm2,1:rtcm3,2:oem4,3:oem3,4:ubx,5:ss2,6:hemis,7:skytraq,8:gw10,9:javad,10:nvs,11:binex,12:rt17,15:sp3"
#define NMEOPT  "0:off,1:latlon,2:single"
#define SOLOPT  "0:llh,1:xyz,2:enu,3:nmea"
#define MSGOPT  "0:all,1:rover,2:base,3:corr"

static int timetype     =0;             /* time format (0:gpst,1:utc,2:jst,3:tow) */
static int soltype      =0;             /* sol format (0:dms,1:deg,2:xyz,3:enu,4:pyl) */
static int solflag      =2;             /* sol flag (1:std+2:age/ratio/ns) */
static char passwd[10];
static opt_t rcvopts[]={
    {"console-passwd",  2,  (void *)passwd,              ""     },
    {"console-timetype",3,  (void *)&timetype,           TIMOPT },
    {"console-soltype", 3,  (void *)&soltype,            CONOPT },
    {"console-solflag", 0,  (void *)&solflag,            FLGOPT },
    {"inpstr1-type",    3,  (void *)&strtype[0],         ISTOPT },
    {"inpstr2-type",    3,  (void *)&strtype[1],         ISTOPT },
    {"inpstr3-type",    3,  (void *)&strtype[2],         ISTOPT },
    {"inpstr1-path",    2,  (void *)strpath [0],         ""     },
    {"inpstr2-path",    2,  (void *)strpath [1],         ""     },
    {"inpstr3-path",    2,  (void *)strpath [2],         ""     },
    {"inpstr1-format",  3,  (void *)&strfmt [0],         FMTOPT },
    {"inpstr2-format",  3,  (void *)&strfmt [1],         FMTOPT },
    {"inpstr3-format",  3,  (void *)&strfmt [2],         FMTOPT },
    {"inpstr2-nmeareq", 3,  (void *)&nmeareq,            NMEOPT },
    {"inpstr2-nmealat", 1,  (void *)&nmeapos[0],         "deg"  },
    {"inpstr2-nmealon", 1,  (void *)&nmeapos[1],         "deg"  },
    {"outstr1-type",    3,  (void *)&strtype[3],         OSTOPT },
    {"outstr2-type",    3,  (void *)&strtype[4],         OSTOPT },
    {"outstr1-path",    2,  (void *)strpath [3],         ""     },
    {"outstr2-path",    2,  (void *)strpath [4],         ""     },
    {"outstr1-format",  3,  (void *)&strfmt [3],         SOLOPT },
    {"outstr2-format",  3,  (void *)&strfmt [4],         SOLOPT },
    {"logstr1-type",    3,  (void *)&strtype[5],         OSTOPT },
    {"logstr2-type",    3,  (void *)&strtype[6],         OSTOPT },
    {"logstr3-type",    3,  (void *)&strtype[7],         OSTOPT },
    {"logstr1-path",    2,  (void *)strpath [5],         ""     },
    {"logstr2-path",    2,  (void *)strpath [6],         ""     },
    {"logstr3-path",    2,  (void *)strpath [7],         ""     },
    
    {"misc-svrcycle",   0,  (void *)&svrcycle,           "ms"   },
    {"misc-timeout",    0,  (void *)&timeout,            "ms"   },
    {"misc-reconnect",  0,  (void *)&reconnect,          "ms"   },
    {"misc-nmeacycle",  0,  (void *)&nmeacycle,          "ms"   },
    {"misc-buffsize",   0,  (void *)&buffsize,           "bytes"},
    {"misc-navmsgsel",  3,  (void *)&navmsgsel,          MSGOPT },
    {"misc-proxyaddr",  2,  (void *)proxyaddr,           ""     },
    {"misc-fswapmargin",0,  (void *)&fswapmargin,        "s"    },
    
    {"misc-startcmd",   2,  (void *)startcmd,            ""     },
    {"misc-stopcmd",    2,  (void *)stopcmd,             ""     },
    
    {"file-cmdfile1",   2,  (void *)rcvcmds[0],          ""     },
    {"file-cmdfile2",   2,  (void *)rcvcmds[1],          ""     },
    {"file-cmdfile3",   2,  (void *)rcvcmds[2],          ""     },
    
    {"",0,NULL,""}
};

/* read receiver commands ----------------------------------------------------*/
static int readcmd(const char *file, char *cmd, int type)
{
    FILE *fp;
    char buff[MAXSTR],*p=cmd;
    int i=0;

    trace(3,"readcmd: file=%s\n",file);

    if (!(fp=fopen(file,"r"))) return 0; 
    while (fgets(buff,sizeof(buff),fp)) {
        if (*buff=='@') i=1;
        else if (i==type&&p+strlen(buff)+1<cmd+MAXRCVCMD) {
            p+=sprintf(p,"%s",buff);
        }
    }
    fclose(fp);
    return 1;
}

/* read antenna file ---------------------------------------------------------*/
static void readant(prcopt_t *opt, nav_t *nav)
{
    const pcv_t pcv0={0};
    pcvs_t pcvr={0},pcvs={0};
    pcv_t *pcv;
    gtime_t time=timeget();
    int i;
    
    trace(3,"readant:\n");
    
    opt->pcvr[0]=opt->pcvr[1]=pcv0;
    if (!*filopt.rcvantp) return;
    
    if (readpcv(filopt.rcvantp,&pcvr)) {
        for (i=0;i<2;i++) {
            if (!*opt->anttype[i]) continue;
            if (!(pcv=searchpcv(0,opt->anttype[i],time,&pcvr))) {
                continue;
            }
            opt->pcvr[i]=*pcv;
        }
    }
    
    if (readpcv(filopt.satantp,&pcvs)) {
        for (i=0;i<MAXSAT;i++) {
            if (!(pcv=searchpcv(i+1,"",time,&pcvs))) continue;
            nav->pcvs[i]=*pcv;
        }
    }
    
    free(pcvr.pcv); free(pcvs.pcv);
}

int start_rtksvr()
{
    double pos[3],npos[3];
    char s[3][MAXRCVCMD]={"","",""},*cmds[]={NULL,NULL,NULL};
    char *ropts[]={"","",""};
    char *paths[]={
        strpath[0],strpath[1],strpath[2],strpath[3],strpath[4],strpath[5],
        strpath[6],strpath[7]
    };
    int i,ret,stropt[8]={0};
    
    trace(3,"startsvr:\n");
    
    /* read start commads from command files */
    for (i=0;i<3;i++) {
        if (!*rcvcmds[i]) continue;
        if (!readcmd(rcvcmds[i],s[i],0)) {
        }
        else cmds[i]=s[i];
    }
    if (prcopt.refpos==4) { /* rtcm */
        for (i=0;i<3;i++) prcopt.rb[i]=0.0;
    }
    pos[0]=nmeapos[0]*D2R;
    pos[1]=nmeapos[1]*D2R;
    pos[2]=0.0;
    pos2ecef(pos,npos);
    
    /* read antenna file */
    readant(&prcopt,&svr.nav);
    
    /* open geoid data file */
    if (solopt[0].geoid>0&&!opengeoid(solopt[0].geoid,filopt.geoid)) {
        trace(2,"geoid data open error: %s\n",filopt.geoid);
    }

    for (i=0;*rcvopts[i].name;i++) modflgr[i]=0;
    for (i=0;*sysopts[i].name;i++) modflgs[i]=0;
    
    /* set stream options */
    stropt[0]=timeout;
    stropt[1]=reconnect;
    stropt[2]=1000;
    stropt[3]=buffsize;
    stropt[4]=fswapmargin;
    strsetopt(stropt);
    
    if (strfmt[2]==8) strfmt[2]=STRFMT_SP3;
    
    /* set ftp/http directory and proxy */
    strsetdir(filopt.tempdir);
    strsetproxy(proxyaddr);
    
    /* execute start command */
    if (*startcmd && (ret=system(startcmd))) {
        trace(2,"command exec error: %s (%d)\n",startcmd,ret);
    }
    solopt[0].posf=strfmt[3];
    solopt[1].posf=strfmt[4];
    
    /* start rtk server */
    if (!rtksvrstart(&svr,svrcycle,buffsize,strtype,paths,strfmt,navmsgsel,
                     cmds,ropts,nmeacycle,nmeareq,npos,&prcopt,solopt,NULL)) {
        trace(2,"rtk server start error\n");
        return 0;
    }
    trace(2, "rtk server start");
    return 1;
}

void stop_rtksvr()
{
    char s[3][MAXRCVCMD]={"","",""},*cmds[]={NULL,NULL,NULL};
    int i,ret;
    
    trace(3,"stopsvr:\n");
    
    if (!svr.state) return;
    
    /* read stop commads from command files */
    for (i=0;i<3;i++) {
        if (!*rcvcmds[i]) continue;
        else cmds[i]=s[i];
    }
    /* stop rtk server */
    rtksvrstop(&svr,cmds);
    
    /* execute stop command */

    if (*stopcmd&&(ret=system(stopcmd))) {
        trace(2,"command exec error: %s (%d)\n",stopcmd,ret);
    }

    if (solopt[0].geoid>0) closegeoid();
}

static int outstat       = 0;
static int trace_level   = 0;

void open_rtksvr(char* basedir, int _trace_level, int _outstat)
{
    outstat = _outstat;    
    trace_level = _trace_level;

    if (strlen(basedir) != 0) {
      if (chdir(basedir) <0 ) {
        fprintf(stderr, "failed to chdir %s\n", basedir);
      }
    }

    if (trace_level > 0) {
        traceopen(TRACEFILE);
        tracelevel(trace_level);
    } 

    resetsysopts();

    /* initialize rtk server */
    rtksvrinit(&svr);

    if (!loadopts(OPTSFILE, rcvopts) || !loadopts(OPTSFILE, sysopts)) {
        fprintf(stderr,"no options file: %s. defaults used\n", OPTSFILE);
    }
    getsysopts(&prcopt, solopt, &filopt);

    /* read navigation data */
    if (!readnav(NAVIFILE, &svr.nav)) {
        fprintf(stderr,"no navigation data: %s\n", NAVIFILE);
    }

    if (outstat > 0) {
        rtkopenstat(STATFILE, outstat);
    }
}

void close_rtksvr()
{
    stop_rtksvr();
    if (outstat > 0) rtkclosestat();

    if (trace_level > 0) traceclose();
    
    /* save navigation data */
    if (!savenav(NAVIFILE, &svr.nav)) {
        fprintf(stderr,"navigation data save error: %s\n", NAVIFILE);
    }
}

void update_record(Record *record)
{
    rtk_t rtk;
    int i, n;
   
    double dop[4]={0};

    double pos[3];
    double rr[3];
    double enu[3];

    double azel[MAXSAT*2];
    double vel[3];    

    int state, rtkstat, num_sats;
    int obs_rover_n;
    int obs_base_n;
    obsd_t obs_rover[MAXOBS];    
    obsd_t obs_base[MAXOBS];
    double az, el;
    
    rtksvrlock(&svr);

    rtk = svr.rtk;
    state = svr.state;
    rtkstat = svr.rtk.sol.stat;

    obs_rover_n = 0;
    for (i=0; i<svr.obs[0][0].n; i++) {
        obs_rover[obs_rover_n++]=svr.obs[0][0].data[i];
    }

    obs_base_n = 0;    
    for (i=0; i<svr.obs[1][0].n; i++) {
        obs_base[obs_base_n++]=svr.obs[1][0].data[i];
    }
    rtksvrunlock(&svr);

    record->timestamp = (uint32_t)(rtk.sol.time.time);
    record->msec = (uint16_t)(rtk.sol.time.sec * 1e3);
    
    record->state = state;
    record->sol = rtkstat;

    record->num_valid = (uint8_t)(rtk.sol.ns);

    record->ratio = (uint16_t)(rtk.sol.ratio * 1000);


    for (i=0; i<3; i++) {
      rr[i] = rtk.sol.rr[i] - rtk.rb[i];
    }
    ecef2pos(rtk.rb, pos);
    /* base lat, lon, height */
    record->llh_base[0] = (uint32_t)(pos[0] * R2D * 10000000);
    record->llh_base[1] = (uint32_t)(pos[1] * R2D * 10000000);
    record->llh_base[2] = (uint32_t)(pos[2] * 1000);

    ecef2enu(pos, rr, enu);

    /* rover lat, lon, alt */    
    ecef2pos(rtk.sol.rr, pos); 
    record->llh_rover[0] = (uint32_t)(pos[0] * R2D * 10000000);
    record->llh_rover[1] = (uint32_t)(pos[1] * R2D * 10000000);
    record->llh_rover[2] = (uint32_t)(pos[2] * 1000);

    /* e, n, u */    
    record->enu_mm[0] = (uint32_t)(enu[0] * 1000);
    record->enu_mm[1] = (uint32_t)(enu[1] * 1000);
    record->enu_mm[2] = (uint32_t)(enu[2] * 1000);
 
    /* ve, vn, vu */
    ecef2enu(pos, rtk.sol.rr+3, vel);    
    record->vel_enu_mm[0] = (uint32_t)(vel[0] * 1000);
    record->vel_enu_mm[1] = (uint32_t)(vel[1] * 1000);
    record->vel_enu_mm[2] = (uint32_t)(vel[2] * 1000);
    
    /* elevation, azimuthes */
    num_sats = 0;
    for (i=0; i<MAXSAT; i++) {
      if (rtk.ssat[i].azel[1] <= 0.0) {
        continue;
      }

      record->sats[num_sats] = i + 1;
      az = rtk.ssat[i].azel[0] * R2D;
      if (az<0.0) {
        az+=360.0;
      }
      el = rtk.ssat[i].azel[1] * R2D;

      record->elevations[num_sats] = (uint16_t)(el * 100);
      record->azimuthes[num_sats] = (uint16_t)(az * 100);
      num_sats += 1;        
    }
    record->num_sats = num_sats;
	
    /* cn0 */
    if (obs_rover_n > MAX_NUM_SATS) {
      obs_rover_n = MAX_NUM_SATS;
    }

    record->num_rovers = obs_rover_n;
    for (i=0; i<obs_rover_n; i++) {
      record->sat_rovers[i] = obs_rover[i].sat;
      /* only l1 */
      record->cn0_rovers[i] = obs_rover[i].SNR[0];
    }

    /* cn0 */
    if (obs_base_n > MAX_NUM_SATS) {
      obs_base_n = MAX_NUM_SATS;
    }
    record->num_bases = obs_base_n;
    for (i=0 ;i<obs_base_n; i++) {
      record->sat_bases[i] = obs_base[i].sat;
      /* only l1 */
      record->cn0_bases[i] = obs_base[i].SNR[0];
    }

    /* dops */
    n = 0;
    for (i=0; i<MAXSAT; i++) {
        if (rtk.opt.mode == PMODE_SINGLE && !rtk.ssat[i].vs) continue;
        if (rtk.opt.mode != PMODE_SINGLE && !rtk.ssat[i].vsat[0]) continue;
        azel[  n*2] = rtk.ssat[i].azel[0];
        azel[1+n*2] = rtk.ssat[i].azel[1];
        n++;
    }
    dops(n, azel, 0.0, dop);
    record->dops[0] = (uint8_t)(dop[0] * 10);
    record->dops[1] = (uint8_t)(dop[1] * 10);
    record->dops[2] = (uint8_t)(dop[2] * 10);
    record->dops[3] = (uint8_t)(dop[3] * 10);
}

void print_record(Record *record)
{
  int i;
  printf("timestamp: %d\n", (int)record->timestamp);
  printf("msec: %d\n", (int)record->msec);

  printf("state: %d\n", record->state);
  printf("sol: %d\n", record->sol);

  printf("base llh: %3.7f, %3.7f, %5.2f\n",
         record->llh_base[0] / 10000000.0,
         record->llh_base[1] / 10000000.0,
         record->llh_base[2] / 1000.0);
  
  printf("rover llh: %3.7f, %3.7f, %5.2f\n",
         record->llh_rover[0] / 10000000.0,
         record->llh_rover[1] / 10000000.0,
         record->llh_rover[2] / 1000.0);
         
  printf("enu: %d, %d, %d\n",
         (int)record->enu_mm[0],
         (int)record->enu_mm[1],
         (int)record->enu_mm[2]);

  printf("vel enu: %d, %d, %d\n",
         (int)record->vel_enu_mm[0],
         (int)record->vel_enu_mm[1],
         (int)record->vel_enu_mm[2]);

  printf("num sats: %d\n", record->num_sats);
  printf("num rovers: %d\n", record->num_rovers);
  printf("num bases: %d\n", record->num_bases);
  printf("num valid: %d\n", record->num_valid);

  printf("sat info\n");
  printf("sats:");
  for (i=0; i<record->num_sats; i++) {
    printf("%d,", record->sats[i]);
  }
  printf("\n");  
  printf("sat elevations:");
  for (i=0; i<record->num_sats; i++) {
    printf("%3.1f,", record->elevations[i] / 100.0);
  }
  printf("\n");  
  printf("sat azimuthes:");
  for (i=0; i<record->num_sats; i++) {
    printf("%3.1f,", record->azimuthes[i] / 100.0);
  }
  printf("\n");
 
  printf("rover cn0\n");
  printf("sats:");
  for (i=0; i<record->num_rovers; i++) {
    printf("%d, ", record->sat_rovers[i]);
  }
  printf("\n");
  printf("cn0:");
  for (i=0; i<record->num_rovers; i++) {
    printf("%2.0f, ", 0.25 * record->cn0_rovers[i]);
  }
  printf("\n");  
 
  printf("base cn0\n");
  printf("sats:");
  for (i=0; i<record->num_bases; i++) {
    printf("%d, ", record->sat_bases[i]);
  }
  printf("\n");
  printf("cn0:");
  for (i=0; i<record->num_bases; i++) {
    printf("%2.0f, ", 0.25 * record->cn0_bases[i]);
  }
  printf("\n");  
}
