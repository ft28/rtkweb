(function() {
  var socket = new WebSocket('ws://localhost:7681/', 'rtkweb-protocol');
  socket.binaryType = 'arraybuffer';

  try {
    socket.onopen = function() {
      console.log("opened");
    } 
    socket.onmessage =function got_packet(msg) {
      print(msg.data);
    } 

    socket.onclose = function(){
      console.log("closed");
    }

  } catch(exception) {
    alert('Error' + exception);  
  }
  
  document.getElementById("button_start").onclick = function() {
    socket.send("start\n");
    console.log("start\n");
  };

  document.getElementById("button_stop").onclick = function() {
    socket.send("stop\n");
    console.log("stop\n");
  };

  function print(data) {
    var record = parse(data);
    var buf = get_buf_status(record);
    buf += get_buf_observ(record);
    buf += get_buf_satellite(record);
    document.getElementById('result').innerHTML = buf;

  }

  function get_buf_status(record) {
    var buf =  "<div class='status'><h2>rtk status</h2>";

    buf += "<table>";
    buf += "<tr>";
    buf += "<th>rtkweb status</th>";
    buf += "<td>" + record["state"] + "</td>";
    buf += "</tr>";

    if (record["state"] === "stopped") {
      document.getElementById("button_stop").disabled = "disabled";
      document.getElementById("button_start").disabled = "";
    } else {
      document.getElementById("button_stop").disabled = "";
      document.getElementById("button_start").disabled = "disabled";
    }

    buf += "<tr>";
    buf += "<th>solution status</th>";
    buf += "<td>" + record["sol"] + "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th>time of reciver clock rover</th>";
    buf += "<td>" + record["datetime"] + "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th>ratio for ar validation</th>";
    buf += "<td>" + record["ratio"].toFixed(3) + "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th># of satellites rover</th>";
    buf += "<td>" + record["num_sats_rover"] + "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th># of satellites base</th>";
    buf += "<td>" + record["num_sats_base"] + "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th># of valid satellites</th>";
    buf += "<td>" + record["num_sats_valid"] + "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th>gdop/pdop/hdop/vdop</th>";
    buf += "<td>";
    buf += (Array.from(record["dops"]).map(x => x.toFixed(1))).join(", ");
    buf += "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th>pos llh (deg,m) rover</th>";
    buf += "<td>";
    buf += record["rover"]["llh"][0].toFixed(7) + ", ";
    buf += record["rover"]["llh"][1].toFixed(7) + ", ";
    buf += record["rover"]["llh"][2].toFixed(3);
    buf += "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th>pos llh (deg,m) rover</th>";
    buf += "<td>";
    buf += record["rover"]["enu"][0].toFixed(3) + ", ";
    buf += record["rover"]["enu"][0].toFixed(3) + ", ";
    buf += record["rover"]["enu"][0].toFixed(3);
    buf += "</td>";
    buf += "</tr>";

    buf += "<tr>";
    buf += "<th>vel enu (m/s) rover</th>";
    buf += "<td>";
    buf += (Array.from(record["rover"]["vel"]).map(x => x.toFixed(3))).join(", ");
    buf += "</td>";
    buf += "</tr>";
   
    buf += "<tr>";
    buf += "<th>pos llh (deg,m) base</th>";
    buf += "<td>";
    buf += record["base"]["llh"][0].toFixed(7) + ", ";
    buf += record["base"]["llh"][1].toFixed(7) + ", ";
    buf += record["base"]["llh"][2].toFixed(3);
    buf += "</td>";
    buf += "</tr>";

    buf += "</table>";
    buf += "</div>";
    return buf;
  }

  function get_buf_observ(record) {
    var buf =  "<div class='observation'><h2>observation data</h2>";
    buf += "<table>";
    
    var infos = {};
    Array.from(record["rover"]["sats"]).forEach(function(sat, index, array) {
      if (!(sat in infos)) {
        infos[sat] = [record["rover"]["cn0"][index], 0];
      }
    });

    Array.from(record["base"]["sats"]).forEach(function(sat, index, array) {
      if (sat in infos) {
        infos[sat][1] = record["base"]["cn0"][index];
      } else {
        infos[sat] =[0, record["base"]["cn0"][index]];
      }
    });
    
    var sats = Object.keys(infos);
    sats.sort(); 
   
    buf += "<tr><th>sat</th>"; 
    sats.forEach(function(sat) {
      buf += "<th>" + sat + "</th>";
    });
 
    buf += "<tr><th>rover</th>"; 
    sats.forEach(function(sat) {
      if (infos[sat][0] == 0) {
        buf += "<td></td>";
      } else {
        buf += "<td>" + infos[sat][0] + "</td>";
      }
    });
    buf += "</tr>";

    buf += "<tr><th>base</th>"; 
    sats.forEach(function(sat) {
      if (infos[sat][1] == 0) {
        buf += "<td></td>";
      } else {
        buf += "<td>" + infos[sat][1] + "</td>";
      }
    });
    buf += "</tr>";

    buf += "</table>";
    buf += "</div>";
    return buf;
  }

  function get_buf_satellite(record) {
    var buf = "<div class='satellite'><h2>satellite status</h2>";
    buf += "<table>";

    buf += "<tr><th>sat</th>";
    Array.from(record["sats"]).forEach(function(sat, index, array) {
      buf += "<th>" + sat + "</th>";
    }); 
    buf += "</tr>";

    buf += "<tr><th>azimuth</th>";
    Array.from(record["sats"]).forEach(function(sat, index, array) {
      buf += "<td>" + record['azimuthes'][index].toFixed(0) + "</td>";
    });
    buf += "</tr>";

    buf += "<tr><th>elevation</th>";
    Array.from(record["sats"]).forEach(function(sat, index, array) {
      buf += "<td>" + record['elevations'][index].toFixed(0) + "</td>";
    });
    buf += "</tr>";
    buf += "</table>";

    buf += "</div>";
    return buf;
  }

  function parse(data) {
    /*
     rtkweb.h

     typedef struct {
       uint32_t timestamp;
       uint16_t msec;
       uint16_t ratio;          // v = x * 1000

       uint32_t llh_rover[3];
       uint32_t llh_base[3];

       int32_t enu_mm[3];
       int32_t vel_enu_mm[3];

       uint8_t num_sats;
       uint8_t num_rovers;
       uint8_t num_bases;
       uint8_t num_valid;

       uint16_t elevations[MAX_NUM_SATS]; // v = (uint16_t)(x * 100)
       uint16_t azimuthes[MAX_NUM_SATS];  // v = (uint16_t)(x * 100)
       uint8_t sats[MAX_NUM_SATS];

       uint8_t sat_rovers[MAX_NUM_SATS];
       uint8_t cn0_rovers[MAX_NUM_SATS];  // v = x /0.25

       uint8_t sat_bases[MAX_NUM_SATS];
       uint8_t cn0_bases[MAX_NUM_SATS];   // v = x / 0.25
    
       uint8_t dops[4];         // v = x * 10  ,"GDOP/PDOP/HDOP/VDOP"
    
       uint8_t sol;             // 0: -, 1: fix, 2: float, 3: SBAS, 4: DGPS, 5: single, 6:ppp
       uint8_t state;           // 0: stop, 1:run
     } Record;

    buf += "</table>";
    buf += "</div>";
    return buf;
  }

  function parse(data) {
    /*
     rtkweb.h

     typedef struct {
       uint32_t timestamp;
       uint16_t msec;
       uint16_t ratio;          // v = x * 1000

       uint32_t llh_rover[3];
       uint32_t llh_base[3];

       int32_t enu_mm[3];
       int32_t vel_enu_mm[3];

       uint8_t num_sats;
       uint8_t num_rovers;
       uint8_t num_bases;
       uint8_t num_valid;

       uint16_t elevations[MAX_NUM_SATS]; // v = (uint16_t)(x * 100)
       uint16_t azimuthes[MAX_NUM_SATS];  // v = (uint16_t)(x * 100)
       uint8_t sats[MAX_NUM_SATS];

       uint8_t sat_rovers[MAX_NUM_SATS];
       uint8_t cn0_rovers[MAX_NUM_SATS];  // v = x /0.25

       uint8_t sat_bases[MAX_NUM_SATS];
       uint8_t cn0_bases[MAX_NUM_SATS];   // v = x / 0.25
    
       uint8_t dops[4];         // v = x * 10  ,"GDOP/PDOP/HDOP/VDOP"
    
       uint8_t sol;             // 0: -, 1: fix, 2: float, 3: SBAS, 4: DGPS, 5: single, 6:ppp
       uint8_t state;           // 0: stop, 1:run
     } Record;
    */
    var MAX_NUM_SATS = 72;

    var view = new DataView(data);
    var ret = {};
    var offset = 0;

    var num_sats = 0;
    var num_sats_rover = 0;
    var num_sats_base = 0;

    ret['rover'] = {};
    ret['base'] = {};

    ret['datetime'] = get_time(view.getUint32(0, true), view.getUint16(4, true));
    offset = 4 + 2;

    ret['ratio'] = get_ratio(view.getUint16(offset, true));
    offset += 2;

    ret['rover']['llh'] = get_llh(new Uint32Array(data, offset, 3));
    offset += 4 * 3;
    ret['base']['llh'] = get_llh(new Uint32Array(data, offset, 3));
    offset += 4 * 3;
    ret['rover']['enu'] = get_m(new Int32Array(data, offset, 3));
    offset += 4 * 3;
    ret['rover']['vel'] = get_m(new Int32Array(data, offset, 3));
    offset += 4 * 3;

    num_sats = view.getUint8(offset, true);
    ret['num_sats'] = num_sats;
    offset += 1;

    num_sats_rover = view.getUint8(offset, true);
    ret['num_sats_rover'] = num_sats_rover;
    offset += 1;

    num_sats_base = view.getUint8(offset, true);
    ret['num_sats_base'] = num_sats_base;
    offset += 1;

    ret['num_sats_valid'] = view.getUint8(offset, true);
    offset += 1;

    ret['elevations'] = get_ea(new Uint16Array(data, offset, num_sats));
    offset += 2 * MAX_NUM_SATS;
    ret['azimuthes'] = get_ea(new Uint16Array(data, offset, num_sats));
    offset += 2 * MAX_NUM_SATS;
    ret['sats'] = get_sats(new Uint8Array(data, offset, num_sats));
    offset += MAX_NUM_SATS;

    ret['rover']['sats'] = get_sats(new Uint8Array(data, offset, num_sats_rover));
    offset += MAX_NUM_SATS;
    ret['rover']['cn0'] = get_cn0(new Uint8Array(data, offset, num_sats_rover));
    offset += MAX_NUM_SATS;

    ret['base']['sats'] = get_sats(new Uint8Array(data, offset, num_sats_base));
    offset += MAX_NUM_SATS;
    ret['base']['cn0'] = get_cn0(new Uint8Array(data, offset, num_sats_base));
    offset += MAX_NUM_SATS;

    ret['dops'] = get_dops(new Uint8Array(data, offset, 4));
    offset += 4;

    ret['sol'] = get_sol(view.getUint8(offset, true));
    offset += 1; 
    ret['state'] = get_state(view.getUint8(offset, true));
   
    return ret;
  } 

  function get_time(timestamp, msec) {
    var date = new Date(timestamp * 1000 + msec);
    var buf = "";
    buf += date.getFullYear();
    buf += "/";
    buf += ("0" + (date.getMonth() + 1)).slice(-2);
    buf += "/";
    buf += ("0" + date.getDate()).slice(-2);
    buf += " ";
    buf += ("0" + date.getHours()).slice(-2);
    buf += ":";
    buf += ("0" + date.getMinutes()).slice(-2);
    buf += ":";
    buf += ("0" + date.getSeconds()).slice(-2);
    buf += ".";
    buf += ("000" + date.getMilliseconds()).slice(-3);
    return buf;  
  }

  function get_ratio(ratio) {
    return ratio * 0.001
  }

  function get_llh(llh) {
    return [llh[0] * 0.0000001, llh[1] * 0.0000001, llh[2] * 0.0001];
  }

  function get_m(values) {
    return Float32Array.from(values).map(x => x * 0.001);
  }

  function get_ea(values) {
    return Float32Array.from(values).map(x => x * 0.01);
  }
 
  function get_sats(values) {
    return Array.from(values).map( x => satno2id(x));
  }

  function get_cn0(values, num_sats) {
    return Float32Array.from(values).map(x => x * 0.25);
  }

  function get_dops(values) {
    return Float32Array.from(values).map(x => x * 0.1);
  }

  function get_sol(value) {
    var sols =["-","fix","float","SBAS","DGPS","single","PPP",""];
    return sols[value];
  }

  function get_state(value) {
    var states = ["stopped","running"];
    console.log("state:" + value);
    return states[value];
  }

  function satno2id(sat) {
    /* copy logic rtkcmn.c: satno2id */

    // rtklib.h values
    var SYS_NONE = 0x00
    var SYS_GPS = 0x01;
    var SYS_SBS = 0x02;
    var SYS_GLO = 0x04;
    var SYS_GAL = 0x08;
    var SYS_QZS = 0x10;
    var SYS_CMP = 0x20;
    var SYS_LEO = 0x80;

    var MIN_PRN_GPS = 1;
    var MAX_PRN_GPS = 32;
    var NSAT_GPS = MAX_PRN_GPS - MIN_PRN_GPS + 1;

    var MIN_PRN_GLO = 1;
    var MAX_PRN_GLO = 24;
    var NSAT_GLO = MAX_PRN_GLO - MIN_PRN_GLO + 1;

    // rtkweb dafault compile setting is enable GLO
    var MIN_PRN_GAL = 1;
    var MAX_PRN_GAL = 30;
    var NSAT_GAL = MAX_PRN_GAL - MIN_PRN_GAL + 1;

    /*
    var MIN_PRN_GAL = 0;
    var MAX_PRN_GAL = 0;
    var NSAT_GAL = 0;
    */

    var MIN_PRN_QZS = 193;
    var MAX_PRN_QZS = 199;
    var NSAT_QZS = MAX_PRN_QZS - MIN_PRN_QZS + 1;

    var MIN_PRN_CMP = 1;
    var MAX_PRN_CMP = 35;
    var NSAT_CMP = MAX_PRN_CMP - MIN_PRN_CMP + 1;

    var MIN_PRN_SBS = 120;
    var MAX_PRN_SBS = 142;
    var NSAT_SBS = MAX_PRN_SBS - MIN_PRN_SBS + 1;

    // rtkweb dafault compile setting is disable LEO
    var MIN_PRN_LEO = 0;
    var MAX_PRN_LEO = 0;
    var NSAT_LEO = 0

    var MAX_SAT = (NSAT_GPS + NSAT_GLO + NSAT_GAL + NSAT_QZS + NSAT_CMP + NSAT_SBS + NSAT_LEO);

    var get_prnsys = function(sat) {
      var sys = SYS_NONE;
      if (sat <= 0 || MAX_SAT < sat) {
        sat = 0;
      }
      else if (sat <= NSAT_GPS) {
        sys = SYS_GPS;
        sat += MIN_PRN_GPS - 1;
      }
      else if ((sat -= NSAT_GPS) <= NSAT_GLO) {
        sys = SYS_GLO;
        sat += MIN_PRN_GLO - 1;
      }
      else if ((sat -= NSAT_GLO) <= NSAT_GAL) {
        sys = SYS_GAL;
        sat += MIN_PRN_GAL - 1;
      }
      else if ((sat -= NSAT_GAL) <= NSAT_QZS) {
        sys = SYS_QZS;
        sat += MIN_PRN_QZS-1;
      }
      else if ((sat -= NSAT_QZS) <= NSAT_CMP) {
        sys = SYS_CMP;
        sat += MIN_PRN_CMP - 1;
      }
      else if ((sat -= NSAT_CMP) <= NSAT_LEO) {
        sys = SYS_LEO;
        sat += MIN_PRN_LEO-1;
      }
      else if ((sat -= NSAT_LEO) <= NSAT_SBS) {
        sys = SYS_SBS;
        sat += MIN_PRN_SBS - 1;
      }
      else {
        sat=0;
      }
      return [sat, sys];
    };
   
    var prn, sys;
    [prn, sys] = get_prnsys(sat);

    switch (sys) {
        case SYS_GPS: return "G" + ("00" + (prn - MIN_PRN_GPS + 1)).slice(-2);
        case SYS_GLO: return "R" + ("00" + (prn - MIN_PRN_GLO + 1)).slice(-2);
        case SYS_GAL: return "E" + ("00" + (prn - MIN_PRN_GAL + 1)).slice(-2);
        case SYS_QZS: return "J" + ("00" + (prn - MIN_PRN_QZS + 1)).slice(-2);
        case SYS_CMP: return "C" + ("00" + (prn - MIN_PRN_CMP + 1)).slice(-2);
        case SYS_LEO: return "L" +_("00" + (prn - MIN_PRN_LEO + 1)).slice(-2);
        case SYS_SBS: return ("000" + prn).slice(-3);
    }
    return "---"
  }
})();
