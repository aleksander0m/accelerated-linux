/* vi: set sw=4 ts=4 sts=4: */
/*
 * main.c -- Main program for the GoAhead WebServer (LINUX version)
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: goahead.c,v 1.120.2.1 2010-02-10 12:02:02 winfred Exp $
 */

/******************************** Description *********************************/

/*
 *	Main program for for the GoAhead WebServer. This is a demonstration
 *	main program to initialize and configure the web server.
 */

/********************************* Includes ***********************************/

#include	"uemf.h"
#include	"wsIntrn.h"
#include	"nvram.h"
//#include	"ralink_gpio.h"
#include	"internet.h"
#if defined (RTDEV_SUPPORT)
#include	"inic.h"
#elif defined (CONFIG_RT2561_AP) || defined (CONFIG_RT2561_AP_MODULE)
#include	"legacy.h"
#endif
#include	"utils.h"
#include	"wireless.h"
#include	"firewall.h" 
#include	"management.h"
#include	"station.h"
#include	"usb.h"
#include	"media.h"
#include	<signal.h>
#include	<unistd.h> 
#include	<sys/types.h>
#include	<sys/wait.h>
#include	"linux/autoconf.h"
#include	"config/autoconf.h" //user config

#ifdef CONFIG_RALINKAPP_SWQOS
#include      "qos.h"
#endif

#ifdef WEBS_SSL_SUPPORT
#include	"websSSL.h"
#endif

#ifdef USER_MANAGEMENT_SUPPORT
#include	"um.h"
void	formDefineUserMgmt(void);
#endif


/*********************************** Locals ***********************************/
/*
 *	Change configuration here
 */

static char_t		*rootWeb = T("/etc_ro/web");		/* Root web directory */
static char_t		*password = T("");				/* Security password */
static int			port = 80;						/* Server port */
static int			retries = 5;					/* Server port retries */
static int			finished;						/* Finished flag */
static char_t		*gopid = T("/var/run/goahead.pid");	/* pid file */

/****************************** Forward Declarations **************************/

static int	writeGoPid(void);
static int 	initSystem(void);
static int 	initWebs(void);
static int  websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
				int arg, char_t *url, char_t *path, char_t *query);
extern void defaultErrorHandler(int etype, char_t *msg);
extern void defaultTraceHandler(int level, char_t *buf);
extern void ripdRestart(void);
#ifdef B_STATS
static void printMemStats(int handle, char_t *fmt, ...);
static void memLeaks();
#endif
extern void WPSAPPBCStartAll(void);
extern void WPSSingleTriggerHandler(int);
#if defined (RTDEV_SUPPORT) || defined (CONFIG_RT2561_AP) || defined (CONFIG_RT2561_AP_MODULE)
void RaixWPSSingleTriggerHandler(int);
#endif
#if defined CONFIG_USB
extern void hotPluglerHandler(int);
#endif


#ifdef CONFIG_RT2860V2_STA_WSC
extern void WPSSTAPBCStartEnr(void);
#endif

#ifdef CONFIG_DUAL_IMAGE
static int set_stable_flag(void);
#endif
/*********************************** Code *************************************/
/*
 *	Main -- entry point from LINUX
 */

int main(int argc, char** argv)
{
/*
 *	Initialize the memory allocator. Allow use of malloc and start 
 *	with a 60K heap.  For each page request approx 8KB is allocated.
 *	60KB allows for several concurrent page requests.  If more space
 *	is required, malloc will be used for the overflow.
 */
	bopen(NULL, (60 * 1024), B_USE_MALLOC);
	signal(SIGPIPE, SIG_IGN);

	if (writeGoPid() < 0)
		return -1;
	if (initSystem() < 0)
		return -1;

/*
 *	Initialize the web server
 */
	if (initWebs() < 0) {
		return -1;
	}

#ifdef CONFIG_DUAL_IMAGE
/* Set stable flag after the web server is started */
	set_stable_flag();
#endif

#ifdef WEBS_SSL_SUPPORT
	websSSLOpen();
#endif

/*
 *	Basic event loop. SocketReady returns true when a socket is ready for
 *	service. SocketSelect will block until an event occurs. SocketProcess
 *	will actually do the servicing.
 */
	while (!finished) {
		if (socketReady(-1) || socketSelect(-1, 1000)) {
			socketProcess(-1);
		}
		websCgiCleanup();
		emfSchedProcess();
	}

#ifdef WEBS_SSL_SUPPORT
	websSSLClose();
#endif

#ifdef USER_MANAGEMENT_SUPPORT
	umClose();
#endif

/*
 *	Close the socket module, report memory leaks and close the memory allocator
 */
	websCloseServer();
	socketClose();
#ifdef B_STATS
	memLeaks();
#endif
	bclose();
	return 0;
}

/******************************************************************************/
/*
 *	Write pid to the pid file
 */
int writeGoPid(void)
{
	FILE *fp;

	fp = fopen(gopid, "w+");
	if (NULL == fp) {
		error(E_L, E_LOG, T("goahead.c: cannot open pid file"));
		return (-1);
	}
	fprintf(fp, "%d", getpid());
    fclose(fp);
	return 0;
}

#if 0 //DAVIDM
static void goaSigHandler(int signum)
{
#ifdef CONFIG_RT2860V2_STA_WSC
	const char *opmode = nvram_bufget(RT2860_NVRAM, "OperationMode");
	const char *ethCon = nvram_bufget(RT2860_NVRAM, "ethConvert");
#endif

	if (signum != SIGUSR1)
		return;

#ifdef CONFIG_RT2860V2_STA_WSC
	if(!strcmp(opmode, "2") || (!strcmp(opmode, "0") &&   !strcmp(ethCon, "1") ) ) {		// wireless isp mode
		nvram_bufset(RT2860_NVRAM, "staWPSMode", "0");
		nvram_commit(RT2860_NVRAM);
		WPSSTAPBCStartEnr();	// STA WPS default is "Enrollee mode".
	}
	else
#endif
		WPSAPPBCStartAll();
}
#endif

#ifndef CONFIG_RALINK_RT2880
static void goaInitGpio()
{
#if 0 //DAVIDM
	int fd;
	ralink_gpio_reg_info info;

	fd = open("/dev/gpio", O_RDONLY);
	if (fd < 0) {
		perror("/dev/gpio");
		return;
	}
	//set gpio direction to input
#ifdef CONFIG_RALINK_RT3883
	if (ioctl(fd, RALINK_GPIO3924_SET_DIR_IN, RALINK_GPIO(26-24)) < 0)
#else
	if (ioctl(fd, RALINK_GPIO_SET_DIR_IN, RALINK_GPIO(0)) < 0)
#endif
		goto ioctl_err;
	//enable gpio interrupt
	if (ioctl(fd, RALINK_GPIO_ENABLE_INTP) < 0)
		goto ioctl_err;
	//register my information
	info.pid = getpid();
#ifdef CONFIG_RALINK_RT3883
	info.irq = 26;
#else
	info.irq = 0;
#endif
	if (ioctl(fd, RALINK_GPIO_REG_IRQ, &info) < 0)
		goto ioctl_err;
	close(fd);

	//issue a handler to handle SIGUSR1
	signal(SIGUSR1, goaSigHandler);
	return;

ioctl_err:
	perror("ioctl");
	close(fd);
#endif
	return;
}
#endif

static void dhcpcHandler(int signum)
{
	firewall_init();
	ripdRestart();
#ifdef CONFIG_RALINKAPP_SWQOS
	QoSInit();
#endif
}


/******************************************************************************/
/*
 *	Initialize System Parameters
 */
static int initSystem(void)
{
	int setDefault(void);

	signal(SIGTSTP, dhcpcHandler);
	signal(SIGUSR2, SIG_IGN);

#if defined CONFIG_USB
	signal(SIGTTIN, hotPluglerHandler);
	//hotPluglerHandler(SIGTTIN);
#endif
#ifdef CONFIG_RALINK_RT2880
	signal(SIGUSR1, goaSigHandler);
#else
	goaInitGpio();
#endif
	signal(SIGXFSZ, WPSSingleTriggerHandler);
#if defined (RTDEV_SUPPORT) || defined (CONFIG_RT2561_AP) || defined (CONFIG_RT2561_AP_MODULE)
	signal(SIGWINCH, RaixWPSSingleTriggerHandler);
#endif

	if (setDefault() < 0)
		return (-1);
	if (initInternet() < 0)
		return (-1);

	return 0;
}

/******************************************************************************/
/*
 *	Set Default should be done by nvram_daemon.
 *	We check the pid file's existence.
 */
int setDefault(void)
{
	FILE *fp;
	int i;

	//retry 15 times (15 seconds)
	for (i = 0; i < 15; i++) {
#ifdef CONFIG_USER_FLATFSD_FLATFSD
		fp = fopen("/var/run/flatfsd.pid", "r");
#else
		fp = fopen("/var/run/nvramd.pid", "r");
#endif
		if (fp == NULL) {
			if (i == 0)
				trace(0, T("goahead: waiting for nvram_daemon "));
			else
				trace(0, T(". "));
		}
		else {
			fclose(fp);
#if defined (CONFIG_RT2860V2_AP_MBSS)
			int bssidnum = strtol(nvram_get(RT2860_NVRAM, "BssidNum"), NULL, 10);
			char newBssidNum[3];
#ifdef CONFIG_RT2860V2_AP_MESH
			if (bssidnum > 6)
				bssidnum--;
#endif
#ifdef CONFIG_RT2860V2_AP_APCLI
			if (bssidnum > 6)
				bssidnum--;
#endif
			sprintf(newBssidNum, "%d", bssidnum);
			nvram_set(RT2860_NVRAM, "BssidNum", newBssidNum);
#endif
			nvram_init(RT2860_NVRAM);
#if defined (RTDEV_SUPPORT) || defined (CONFIG_RT2561_AP) || defined (CONFIG_RT2561_AP_MODULE)
			nvram_init(RTDEV_NVRAM);
#endif
			
			return 0;
		}
		Sleep(1);
	}
	error(E_L, E_LOG, T("goahead: please execute nvram_daemon first!"));
	return (-1);
}

/******************************************************************************/
/*
 *	Initialize the web server.
 */

static int initWebs(void)
{
	struct in_addr	intaddr;
#ifdef GA_HOSTNAME_SUPPORT
	struct hostent	*hp;
	char			host[128];
#else
	const char			*lan_ip = nvram_bufget(RT2860_NVRAM, "lan_ipaddr");
#endif
	char			webdir[128];
	char			*cp;
	char_t			wbuf[128];

/*
 *	Initialize the socket subsystem
 */
	socketOpen();

#ifdef USER_MANAGEMENT_SUPPORT
/*
 *	Initialize the User Management database
 */
	char *admu = (char *) nvram_bufget(RT2860_NVRAM, "Login");
	char *admp = (char *) nvram_bufget(RT2860_NVRAM, "Password");
	umOpen();
	//umRestore(T("umconfig.txt"));
	//winfred: instead of using umconfig.txt, we create 'the one' adm defined in nvram
	umAddGroup(T("adm"), 0x07, AM_DIGEST, FALSE, FALSE);
	if (admu && strcmp(admu, "") && admp && strcmp(admp, "")) {
		umAddUser(admu, admp, T("adm"), FALSE, FALSE);
		umAddAccessLimit(T("/"), AM_DIGEST, FALSE, T("adm"));
	}
	else
		error(E_L, E_LOG, T("gohead.c: Warning: empty administrator account or password"));
#endif

#ifdef GA_HOSTNAME_SUPPORT
/*
 *	Define the local Ip address, host name, default home page and the 
 *	root web directory.
 */
	if (gethostname(host, sizeof(host)) < 0) {
		error(E_L, E_LOG, T("gohead.c: Can't get hostname"));
		return -1;
	}
	if ((hp = gethostbyname(host)) == NULL) {
		error(E_L, E_LOG, T("gohead.c: Can't get host address"));
		return -1;
	}
	memcpy((char *) &intaddr, (char *) hp->h_addr_list[0],
		(size_t) hp->h_length);
#else
/*
 * get ip address from nvram configuration (we executed initInternet)
 */
	if (NULL == lan_ip) {
		error(E_L, E_LOG, T("initWebs: cannot find lan_ip in NVRAM"));
		return -1;
	}
	intaddr.s_addr = inet_addr(lan_ip);
	if (intaddr.s_addr == INADDR_NONE) {
		error(E_L, E_LOG, T("initWebs: failed to convert %s to binary ip data"),
				lan_ip);
		return -1;
	}
#endif

/*
 *	Set rootWeb as the root web. Modify this to suit your needs
 */
	sprintf(webdir, "%s", rootWeb);

/*
 *	Configure the web server options before opening the web server
 */
	websSetDefaultDir(webdir);
	cp = inet_ntoa(intaddr);
	ascToUni(wbuf, cp, min(strlen(cp) + 1, sizeof(wbuf)));
	websSetIpaddr(wbuf);
#ifdef GA_HOSTNAME_SUPPORT
	ascToUni(wbuf, host, min(strlen(host) + 1, sizeof(wbuf)));
#else
	//use ip address (already in wbuf) as host
#endif
	websSetHost(wbuf);

/*
 *	Configure the web server options before opening the web server
 */
	websSetDefaultPage(T("default.asp"));
	websSetPassword(password);

/* 
 *	Open the web server on the given port. If that port is taken, try
 *	the next sequential port for up to "retries" attempts.
 */
	websOpenServer(port, retries);

/*
 * 	First create the URL handlers. Note: handlers are called in sorted order
 *	with the longest path handler examined first. Here we define the security 
 *	handler, forms handler and the default web page handler.
 */
	websUrlHandlerDefine(T(""), NULL, 0, websSecurityHandler, 
		WEBS_HANDLER_FIRST);
	websUrlHandlerDefine(T("/goform"), NULL, 0, websFormHandler, 0);
	websUrlHandlerDefine(T("/cgi-bin"), NULL, 0, websCgiHandler, 0);
	websUrlHandlerDefine(T(""), NULL, 0, websDefaultHandler, 
		WEBS_HANDLER_LAST); 

/*
 *	Define our functions
 */
	formDefineUtilities();
	formDefineInternet();
#if defined CONFIG_RALINKAPP_SWQOS
	formDefineQoS();
#endif
#if defined CONFIG_USB
	formDefineUSB();
#endif
#if defined CONFIG_RALINKAPP_MPLAYER
	formDefineMedia();
#endif
	formDefineWireless();
#if defined (RTDEV_SUPPORT)
	formDefineInic();
#elif defined (CONFIG_RT2561_AP) || defined (CONFIG_RT2561_AP_MODULE)
	formDefineLegacy();
#endif
#if defined CONFIG_RT2860V2_STA || defined CONFIG_RT2860V2_STA_MODULE
	formDefineStation();
#endif
	formDefineFirewall();
	formDefineManagement();

/*
 *	Create the Form handlers for the User Management pages
 */
#ifdef USER_MANAGEMENT_SUPPORT
	//formDefineUserMgmt();  winfred: we do it ourselves
#endif

/*
 *	Create a handler for the default home page
 */
	websUrlHandlerDefine(T("/"), NULL, 0, websHomePageHandler, 0); 
	return 0;
}

/******************************************************************************/
/*
 *	Home page handler
 */

static int websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
	int arg, char_t *url, char_t *path, char_t *query)
{
/*
 *	If the empty or "/" URL is invoked, redirect default URLs to the home page
 */
	if (*url == '\0' || gstrcmp(url, T("/")) == 0) {
		websRedirect(wp, T("home.asp"));
		return 1;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Default error handler.  The developer should insert code to handle
 *	error messages in the desired manner.
 */

void defaultErrorHandler(int etype, char_t *msg)
{
	write(1, msg, gstrlen(msg));
}

/******************************************************************************/
/*
 *	Trace log. Customize this function to log trace output
 */

void defaultTraceHandler(int level, char_t *buf)
{
/*
 *	The following code would write all trace regardless of level
 *	to stdout.
 */
	if (buf) {
		if (0 == level)
			write(1, buf, gstrlen(buf));
	}
}

/******************************************************************************/
/*
 *	Returns a pointer to an allocated qualified unique temporary file name.
 *	This filename must eventually be deleted with bfree();
 */
#if defined CONFIG_USB_STORAGE && defined CONFIG_USER_STORAGE
char_t *websGetCgiCommName(webs_t wp)
{
	char *force_mem_upgrade = nvram_bufget(RT2860_NVRAM, "Force_mem_upgrade");
	char_t	*pname1 = NULL, *pname2 = NULL;
	char *part;

	if(!strcmp(force_mem_upgrade, "1")){
		pname1 = (char_t *)tempnam(T("/var"), T("cgi"));
	}else if(wp && (wp->flags & WEBS_CGI_FIRMWARE_UPLOAD) ){
		// see if usb disk is present and available space is enough?
		if( (part = isStorageOK()) )
			pname1 = (char_t *)tempnam(part, T("cgi"));
		else
			pname1 = (char_t *)tempnam(T("/var"), T("cgi"));
	}else{
		pname1 = (char_t *)tempnam(T("/var"), T("cgi"));
	}

	pname2 = bstrdup(B_L, pname1);
	free(pname1);

	return pname2;
}
#else
char_t *websGetCgiCommName(webs_t wp)
{
	char_t	*pname1, *pname2;

	pname1 = (char_t *)tempnam(T("/var"), T("cgi"));
	pname2 = bstrdup(B_L, pname1);
	free(pname1);

	return pname2;
}
#endif
/******************************************************************************/
/*
 *	Launch the CGI process and return a handle to it.
 */

int websLaunchCgiProc(char_t *cgiPath, char_t **argp, char_t **envp,
					  char_t *stdIn, char_t *stdOut)
{
	int	pid, fdin, fdout, hstdin, hstdout, rc;

	fdin = fdout = hstdin = hstdout = rc = -1; 
	if ((fdin = open(stdIn, O_RDWR | O_CREAT, 0666)) < 0 ||
		(fdout = open(stdOut, O_RDWR | O_CREAT, 0666)) < 0 ||
		(hstdin = dup(0)) == -1 ||
		(hstdout = dup(1)) == -1 ||
		dup2(fdin, 0) == -1 ||
		dup2(fdout, 1) == -1) {
		goto DONE;
	}

 	rc = pid = fork();
 	if (pid == 0) {
/*
 *		if pid == 0, then we are in the child process
 */
		if (execve(cgiPath, argp, envp) == -1) {
			printf("content-type: text/html\n\n"
				"Execution of cgi process failed\n");
		}
		exit (0);
	} 

DONE:
	if (hstdout >= 0) {
		dup2(hstdout, 1);
      close(hstdout);
	}
	if (hstdin >= 0) {
		dup2(hstdin, 0);
      close(hstdin);
	}
	if (fdout >= 0) {
		close(fdout);
	}
	if (fdin >= 0) {
		close(fdin);
	}
	return rc;
}

/******************************************************************************/
/*
 *	Check the CGI process.  Return 0 if it does not exist; non 0 if it does.
 */

int websCheckCgiProc(int handle, int *status)
{
/*
 *	Check to see if the CGI child process has terminated or not yet.  
 */
	if (waitpid(handle, status, WNOHANG) == handle) {
		return 0;
	} else {
		return 1;
	}
}

/******************************************************************************/

#ifdef B_STATS
static void memLeaks() 
{
	int		fd;

	if ((fd = gopen(T("leak.txt"), O_CREAT | O_TRUNC | O_WRONLY, 0666)) >= 0) {
		bstats(fd, printMemStats);
		close(fd);
	}
}

/******************************************************************************/
/*
 *	Print memory usage / leaks
 */

static void printMemStats(int handle, char_t *fmt, ...)
{
	va_list		args;
	char_t		buf[256];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	write(handle, buf, strlen(buf));
}
#endif

/******************************************************************************/

/* added by YYhuang 07/04/02 */
int getGoAHeadServerPort(void)
{
    return port;
}

#ifdef CONFIG_DUAL_IMAGE
static int set_stable_flag(void)
{
	int set = 0;
	char *wordlist = nvram_get(UBOOT_NVRAM, "Image1Stable");

	if (wordlist) {
		if (strcmp(wordlist, "1") != 0)
			set = 1;
	}
	else
		set = 1;

	if (set) {
		printf("Set Image1 stable flag\n");
		nvram_set(UBOOT_NVRAM, "Image1Stable", "1");
	}
	
	return 0;

}
#endif
