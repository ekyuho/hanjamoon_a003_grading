#include <Ticker.h>
Ticker tick, h_tick;
#include <QuickStats.h>
QuickStats stats;

#include <Pms3003.h>
Pms3003 dust;

#define ON digitalWrite(2, HIGH)
#define OFF digitalWrite(2, LOW)

int LOGGING = 2;
float adjust25 = 1.0;
float adjust10 = 1.0;
unsigned long mark;
int yyyy,mo,dd,hh,mm,ss;
bool newtick = false, h_newtick = false;

class Bucket {
	public:
		Bucket(void);
		int numreadings;
		int pm25readings[100];
		int pm10readings[100];
		void append(int, int);
		void reset(void);
};

Bucket::Bucket(void) {
	numreadings = 0;
};

void Bucket::reset(void) {
	numreadings = 0;
};

void Bucket::append(int pm25, int pm10) {
	pm25readings[numreadings] = pm25;
	pm10readings[numreadings++] = pm10;
	if (numreadings == 100) {
		Serial.printf("\n folding numreadings to 0 ");
		numreadings = 0;
	}
		
};

Bucket h_dust;

int get_median(int a[], int cnt) {
	if (false && cnt < 10) {
		Serial.println(" D cnt = "+ String(cnt));
		return 0;
	}
	yield();
	float qbuf[80];
	if (LOGGING > 10) Serial.printf("\n (%d) ", cnt);
	for (int i=0; i<cnt; i++) {
		qbuf[i] = float(a[i]);
		if (LOGGING > 10) Serial.printf(" %d", a[i]);
	}
	float r = stats.median(qbuf, cnt);
	yield();
	return(int(r));
}


void got_dust() {
	int a[2], p2, p10;
	dust.last(a);
	p2 = a[0];
	p10 = a[1];
	
	if (p2 > 1000 || p10 > 1000) {
		if (LOGGING > 1) Serial.printf(" ValueError pm2.5=%d, pm10=%d ", p2, p10);
		if (dust.numreadings>1) dust.numreadings--;
	} else {
		unsigned long t1 = millis()/1000 - mark;
		
		int ss2 = ss + t1;
		int mm2 = mm;
		int hh2 = hh;
		int dd2 = dd;
		int mo2 = mo;
		int yyyy2 = yyyy;
		if (ss2/60 > 0) { mm2 += ss2/60; ss2 %= 60; }
		if (mm2/60 > 0) { hh2 += mm2/60; mm2 %= 60; }
		if (hh2/24 > 0) { dd2 += hh2/24; hh2 %= 24; }
		if (dd2/31 > 0) { mo2 += dd2/31; dd2 %= 31; }
		if (LOGGING > 1) Serial.printf("\n DATA(1sec): %04d-%02d-%02d %02d:%02d:%02d    PM2.5,PM10=  %4d    %4d", yyyy2,mo2,dd2,hh2,mm2,ss2, p2, p10);
	}
}

void parse(String s) {
	if (s == "debug=on") {
		LOGGING = 2;
		Serial.printf("\n Turn on debug message");
		return;
	}
	if (s == "debug=off") {
		LOGGING = 1;
		Serial.printf("\n Turn off debug message");
		return;
	}
	if (s.startsWith("adjust=")) {
		String t = s.substring(7);
		int i = t.indexOf(',');
		String a25 = t.substring(0, i);
		String a10 = t.substring(i+1);
		//Serial.printf("\npm25,pm10=%s,%s", a25.c_str(), a10.c_str());
		adjust25 = a25.toFloat();
		adjust10 = a10.toFloat();
		Serial.printf("\n adjust=%f,%f", adjust25, adjust10);
		return;
	}
	if (s.length() == 0) {
		Serial.printf("\n\n 날자시간설정형식 (24시간형식): YYYYMODD HHMMSS\n debug=on\n debug=off\n adjust=1.0,1.0");
		return;
	}
	if (s.charAt(8) != ' ') {
		Serial.printf("\n\n 공란위치에 글자가 있습니다. %s\nSetting Date and Time: YYYYMODD HHMMSS\n", s.c_str());
		return;
	}
	if (s.length() != 15) {
		Serial.printf("\n\n 공란포함 정확히 15글자이어야 합니다. %s\nSetting Date and Time: YYYYMODD HHMMSS\n", s.c_str());
		return;
	}
	
	for (int i=0; i<12; i++) {
		if (i == 8) continue;
		if (s.charAt(i) < '0' || s.charAt(i) > '9') {
			Serial.printf("\n\n 숫자가 아닌 것이 포함되었습니다. %s\nSetting Date and Time: YYYYMODD HHMMSS\n", s.c_str());
			return;
		}
	}
	yyyy = atoi((const char*)s.substring(0,4).c_str());
	mo = atoi((const char*)s.substring(4,6).c_str());
	dd = atoi((const char*)s.substring(6,8).c_str());
	hh = atoi((const char*)s.substring(9,11).c_str());
	mm = atoi((const char*)s.substring(11,13).c_str());
	ss = atoi((const char*)s.substring(13,15).c_str());
	
	Serial.printf("\n\n 입력: %s", s.c_str());
	Serial.printf("\n 시간설정 성공: %04d-%02d-%02d %02d:%02d:%02d\n", yyyy,mo,dd,hh,mm,ss);
	
	mark = millis()/1000;
}

void got_tick() {
	newtick = true;
}

void got_h_tick() {
	h_newtick = true;
}


void do_tick() {
	int p2, p10;
	if (false && LOGGING > 10) {
		Serial.println();
		for (int i=0; i<dust.numreadings; i++) Serial.printf("(%d)%d,%d ", i, dust.pm25readings[i],dust.pm10readings[i]);
	}
	p2 = get_median(dust.pm25readings, dust.numreadings);
	p10 = get_median(dust.pm10readings, dust.numreadings);
	dust.reset();
	
	unsigned long t1 = millis()/1000 - mark;
	int ss2 = ss + int(t1);
	int mm2 = mm;
	int hh2 = hh;
	int dd2 = dd;
	int mo2 = mo;
	int yyyy2 = yyyy;
	if (ss2/60 > 0) { mm2 += ss2/60; ss2 %= 60; }
	if (mm2/60 > 0) { hh2 += mm2/60; mm2 %= 60; }
	if (hh2/24 > 0) { dd2 += hh2/24; hh2 %= 24; }
	if (dd2/31 > 0) { mo2 += dd2/31; dd2 %= 31; }
	if (LOGGING > 1) {
		Serial.printf("\nDATA(1min): %04d-%02d-%02d %02d:%02d:%02d", yyyy2,mo2,dd2,hh2,mm2,ss2);
		Serial.printf("          PM2.5,PM10=  %.1f,%.1f", p2*adjust25, p10*adjust10);
	}
	
	h_dust.append(p2, p10);
}

void do_h_tick() {
	if (false && LOGGING > 10) {
		Serial.println();
		for (int i=0; i<h_dust.numreadings; i++) Serial.printf(" (%d)%d,%d ", i, h_dust.pm25readings[i], h_dust.pm10readings[i]);
	}
	int p2= get_median(h_dust.pm25readings, h_dust.numreadings);
	int p10 = get_median(h_dust.pm10readings, h_dust.numreadings);
	h_dust.reset();
	
	unsigned long t1 = millis()/1000 - mark;
	int ss2 = ss + t1;
	int mm2 = mm;
	int hh2 = hh;
	int dd2 = dd;
	int mo2 = mo;
	int yyyy2 = yyyy;
	if (ss2/60 > 0) { mm2 += ss2/60; ss2 %= 60; }
	if (mm2/60 > 0) { hh2 += mm2/60; mm2 %= 60; }
	if (hh2/24 > 0) { dd2 += hh2/24; hh2 %= 24; }
	if (dd2/31 > 0) { mo2 += dd2/31; dd2 %= 31; }
	Serial.printf("\nDATA(1hour)%04d-%02d-%02d %02d:%02d:%02d", yyyy2,mo2,dd2,hh2,mm2,ss2);
	Serial.printf("    PM2.5,PM10=  %4f    %4f", p2*adjust25, p10*adjust10);
}

void setup() {
	Serial.begin(9600);
	Serial.println("\n");
	Serial.println(" Dust Sensor V3.0 Begin: 1 hour interval");
	Serial.println(" debug=on or debug=off  for debugging message");
	Serial.printf(" adjust=%.1f,%.1f   type adjust=X,Y to change pm2.5 and pm10 each.\n", adjust25, adjust10);
	Serial1.begin(9600, SERIAL_8N1, 13, 5);
  pinMode(2, OUTPUT);
	dust.set_model("PMSA003");
	mark = millis()/1000;
	yyyy = 2019;
	mo = 12;
	dd = 1;
	hh = 15;
	mm = 0;
	ss = 0;
	tick.attach(60, got_tick);  //min
	h_tick.attach(60*60, got_h_tick);  //hour
	//parse("debug=on");
}

void loop() {
	if (newtick) {
		newtick = false;
		do_tick();  //min
	}
	if (h_newtick) {
		h_newtick = false;
		do_h_tick(); //hour
	}
	while (Serial1.available()) {
    ON;
		if (dust.append(Serial1.read())) got_dust();
    OFF;
	}
	
	while (Serial.available()) {
		char buf[50];

		Serial.readBytesUntil('\n', buf, 50);
		char *p = strchr(buf, '\r'); if (p) *p = 0;
		p = strchr(buf, '\n'); if (p) *p = 0;
		yield();
		parse(String(buf));
	}
	yield();
}
