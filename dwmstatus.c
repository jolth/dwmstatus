#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tzpst = "America/Bogota";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	bzero(buf, sizeof(buf));
	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		perror("localtime");
		exit(1);
	}

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		exit(1);
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0) {
		perror("getloadavg");
		exit(1);
	}

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char *
getbattery(char *base)
{
	char *path, line[513];
	FILE *fd;
	int descap, remcap;

	descap = -1;
	remcap = -1;

	path = smprintf("%s/info", base);
	fd = fopen(path, "r");
	if (fd == NULL) {
		perror("fopen");
		exit(1);
	}
	free(path);
	while (!feof(fd)) {
		if (fgets(line, sizeof(line)-1, fd) == NULL)
			break;

		if (!strncmp(line, "present", 7)) {
			if (strstr(line, " no")) {
				descap = 1;
				break;
			}
		}
		if (!strncmp(line, "design capacity", 15)) {
			if (sscanf(line+16, "%*[ ]%d%*[^\n]", &descap))
				break;
		}
	}
	fclose(fd);

	path = smprintf("%s/state", base);
	fd = fopen(path, "r");
	if (fd == NULL) {
		perror("fopen");
		exit(1);
	}
	free(path);
	while (!feof(fd)) {
		if (fgets(line, sizeof(line)-1, fd) == NULL)
			break;

		if (!strncmp(line, "present", 7)) {
			if (strstr(line, " no")) {
				remcap = 1;
				break;
			}
		}
		if (!strncmp(line, "remaining capacity", 18)) {
			if (sscanf(line+19, "%*[ ]%d%*[^\n]", &remcap))
				break;
		}
	}
	fclose(fd);

	if (remcap < 0 || descap < 0)
		return NULL;

	return smprintf("%.0f", ((float)remcap / (float)descap) * 100);
}

char *
chargeStatus(char *path)
{
	FILE* fp;
	char line[40];
	fp = fopen(path, "r");
	if (fp == NULL) {
		exit(1);
		return NULL;
	}
	fgets(line, sizeof(line)-1, fp);
	fclose(fp);
	if (line == NULL) {
		exit(1);
		return NULL;
	}
	if (strncmp(line+25, "on-line", 7) == 0) {
		return "chg";
	}
	else {
		return "dis";
	}
}

char*
runcmd(char* cmd) {
	FILE* fp = popen(cmd, "r");
	if (fp == NULL) return NULL;
	char ln[30];
	fgets(ln, sizeof(ln)-1, fp);
	pclose(fp);
	ln[strlen(ln)-1]='\0';
	return smprintf("%s", ln);
}

static unsigned long long lastTotalUser[4], lastTotalUserLow[4], lastTotalSys[4], lastTotalIdle[4];
    
char trash[5];

void initcore(){
        FILE* file = fopen("/proc/stat", "r");
            	char ln[100];

        for (int i = 0; i < 5; i++) {
        	fgets(ln, 99, file);
        	if (i < 1) continue;
        	sscanf(ln, "%s %Ld %Ld %Ld %Ld", trash, &lastTotalUser[i-1], &lastTotalUserLow[i-1],
                &lastTotalSys[i-1], &lastTotalIdle[i-1]);
        }
     fclose(file);

}
    
void getcore(char cores[4][5]){
        double percent;
        FILE* file;
        unsigned long long totalUser[4], totalUserLow[4], totalSys[4], totalIdle[4], total[4];
    
    	    	char ln[100];

        file = fopen("/proc/stat", "r");
        for (int i = 0; i < 5; i++) {
        	fgets(ln, 99, file);
        	if (i < 1) continue;
	        sscanf(ln, "%s %Ld %Ld %Ld %Ld", trash, &totalUser[i-1], &totalUserLow[i-1],
	                &totalSys[i-1], &totalIdle[i-1]);
	    }
        fclose(file);
    
    	for (int i = 0; i < 4; i++) {
	        if (totalUser[i] < lastTotalUser[i] || totalUserLow[i]< lastTotalUserLow[i] ||
	                totalSys[i] < lastTotalSys[i] || totalIdle[i] < lastTotalIdle[i]){
	                //Overflow detection. Just skip this value.
	                percent = -1.0;
	        }
	        else{
	                total[i] = (totalUser[i] - lastTotalUser[i]) + (totalUserLow[i] - lastTotalUserLow[i]) +
	                        (totalSys[i] - lastTotalSys[i]);
	                percent = total[i];
	                total[i] += (totalIdle[i] - lastTotalIdle[i]);
	                percent /= total[i];
	                percent *= 100;
	        }
	        strcpy(cores[i], smprintf("%d%%", (int)percent));
	    }
    
    	for (int i = 0; i < 4; i++) {
	        lastTotalUser[i] = totalUser[i];
	        lastTotalUserLow[i] = totalUserLow[i];
	        lastTotalSys[i] = totalSys[i];
	        lastTotalIdle[i] = totalIdle[i];
	    }
   
}

#define BATTERY "/proc/acpi/battery/BAT1"
#define ADAPTER "/proc/acpi/ac_adapter/ADP1/state"
#define VOLCMD "echo $(amixer get Master | tail -n1 | sed -r 's/.*\\[(.*)%\\].*/\\1/')%"
#define MEMCMD "echo $(free -m | awk '/buffers\\/cache/ {print $3}')M"
#define RXCMD "cat /sys/class/net/eth0/statistics/rx_bytes"
#define TXCMD "cat /sys/class/net/eth0/statistics/tx_bytes"
//#define RXCMD "cat /sys/class/net/wlan0/statistics/rx_bytes"
//#define TXCMD "cat /sys/class/net/wlan0/statistics/tx_bytes"

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

int main(void) {
	char *status;
	char *avgs;
	char *bat;
	char *date;
	char *charge;
	char *tme;
	char* vol;
	char cores[4][5];
	char *mem;
	char *rx_old, *rx_now, *tx_old, *tx_now;
	initcore();
	int rx_rate, tx_rate; //kilo bytes
	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}
	rx_old = runcmd(RXCMD);
	tx_old = runcmd(TXCMD);
	for (;;sleep(1)) {
		//avgs = loadavg();
		//NO USAGE: bat = getbattery(BATTERY);
		date = mktimes("%a, %d %b", tzpst);
		tme = mktimes("%H:%M", tzpst);
		//NO USAGE: charge = chargeStatus(ADAPTER);
		vol = runcmd(VOLCMD);
		mem = runcmd(MEMCMD);
		//get transmitted and recv'd bytes
		rx_now = runcmd(RXCMD);
		tx_now = runcmd(TXCMD);
		rx_rate = (atoi(rx_now) - atoi(rx_old)) / 1024;
		tx_rate = (atoi(tx_now) - atoi(tx_old)) / 1024;
		getcore(cores);
		status = smprintf("[ WLAN0: %dK / %dK ][ VOL: %s ][ CPU: %s / %s / %s / %s ][ RAM: %s ][ %s ][ %s ]",
				  rx_rate, tx_rate, vol, cores[0], cores[1], cores[2], cores[3], mem, date, tme);
		strcpy(rx_old, rx_now);
		strcpy(tx_old, tx_now);
		//printf("%s\n", status);
		setstatus(status);
		//free(avgs);
		free(rx_now);
		free(tx_now);
		//NO USAGE: free(bat);
		free(vol);
		free(date);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}
