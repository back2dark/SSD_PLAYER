/*
 * wifiInfo.c
 *
 *  Created on: 2019年8月15日
 *      Author: koda.xu
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef SUPPORT_WLAN_MODULE
#include "wifiInfo.h"
#include "cJSON.h"


#define WIFI_SETTING_CFG	"/appconfigs/wifisetting.json"
#define WPA_CFG				"/appconfigs/wpa_supplicant.conf"
#define WPA_CFG_BACKUP		"/appconfigs/wpa_supplicant.conf_bak"

typedef struct
{
	int index;
	char ssid[36];
	char pwd[40];
} WLAN_CONN_t;


static bool g_bWifiSupport = true;
static bool g_bWifiEnable = true;
static bool g_bConnected = false;
static WLAN_HANDLE g_hWlan = -1;
static MI_WLAN_ConnectParam_t g_stConnectParam;	// abandon
static cJSON *g_pRoot = NULL;
static cJSON *g_pWifi = NULL;
static cJSON *g_pSavedSsid = NULL;
static cJSON *g_pConnect = NULL;
static std::vector<WLAN_CONN_t> g_vecConn;

static void *malloc_fn(size_t sz)
{
	return malloc(sz);
}

static void free_fn(void *ptr)
{
	free(ptr);
}

bool getWifiSupportStatus()
{
	return g_bWifiSupport;
}

void setWifiSupportStatus(bool enable)
{
	g_bWifiSupport = enable;
}

bool getWifiEnableStatus()
{
	return g_bWifiEnable;
}

void setWifiEnableStatus(bool enable)
{
	g_bWifiEnable = enable;
}

bool getConnectionStatus()
{
	return g_bConnected;
}

void setConnectionStatus(bool enable)
{
	g_bConnected = enable;
}

WLAN_HANDLE getWlanHandle()
{
	return g_hWlan;
}

void setWlanHandle(WLAN_HANDLE handle)
{
	g_hWlan = handle;
}

MI_WLAN_ConnectParam_t * getConnectParam()
{
	return &g_stConnectParam;
}

void saveConnectParam(MI_WLAN_ConnectParam_t *pConnParam)
{
	memset(&g_stConnectParam, 0, sizeof(MI_WLAN_ConnectParam_t));
	memcpy(&g_stConnectParam, pConnParam, sizeof(MI_WLAN_ConnectParam_t));
}


// init: 文件不存在或文件存在但内容为空时，创建文件并写入默认配置;
// 		 文件存在且内容不为空时，读取文件内容


int initWifiConfig2()
{
	FILE* fp = NULL;
	long long len = 0;
	char * pConfData = NULL;
	char * ssid = NULL;
	cJSON * connect;
	cJSON * item;
	cJSON_Hooks hooks;
	int ret = -1;

	memset(&g_stConnectParam, 0, sizeof(MI_WLAN_ConnectParam_t));
	g_stConnectParam.eSecurity = E_MI_WLAN_SECURITY_WPA;
	g_stConnectParam.OverTimeMs = 5000;

	fp = fopen(WIFI_SETTING_CFG,"r+");
	if (!fp)
	{
		printf("should open json file first\n");
		return -1;
	}

	printf("open %s success\n", WIFI_SETTING_CFG);

	g_vecConn.clear();

	hooks.free_fn = free_fn;
	hooks.malloc_fn = malloc_fn;
	cJSON_InitHooks(&hooks);

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	pConfData = (char *)malloc(len + 1);
	fseek(fp, 0, SEEK_SET);
	fread(pConfData, len, 1, fp);
	fclose(fp);
	fp = NULL;

	// read config
	cJSON_Minify(pConfData);
	g_pRoot = cJSON_Parse(pConfData);
	if (!g_pRoot)
		goto exit;

	g_pWifi = cJSON_GetObjectItem(g_pRoot, "wifi");
	if (!g_pWifi)
		goto exit;

	printf("parse json success\n");
	item = cJSON_GetObjectItem(g_pWifi, "isSupport");
	if (item)
		g_bWifiSupport = cJSON_IsTrue(item);
	printf("isSupport: %d\n", g_bWifiSupport);

	item = cJSON_GetObjectItem(g_pWifi, "isEnable");
	if (item)
		g_bWifiEnable = cJSON_IsTrue(item);
	printf("isSupport: %d\n", g_bWifiEnable);

	g_pSavedSsid = cJSON_GetObjectItem(g_pWifi, "ssid");
	if (!g_pSavedSsid)
		goto exit;

	ssid = cJSON_GetStringValue(g_pSavedSsid);
	if (!ssid)
		goto exit;

	g_pConnect = cJSON_GetObjectItem(g_pWifi, "connections");
	if (g_pConnect)
	{
		connect = cJSON_GetObjectItem(g_pConnect, ssid);
		if(connect)
		{
			strcpy((char*)g_stConnectParam.au8SSId, ssid);
			item = cJSON_GetObjectItem(connect, "id");
			g_hWlan = (WLAN_HANDLE)atoi(cJSON_GetStringValue(item));
			item = cJSON_GetObjectItem(connect, "passwd");
			strcpy((char*)g_stConnectParam.au8Password, cJSON_GetStringValue(item));
			ret = 0;
		}
	}

	printf("isSupport:%d isEnable:%d id:%d ssid:%s passwd:%s\n", g_bWifiSupport, g_bWifiEnable, g_hWlan, (char*)g_stConnectParam.au8SSId, (char*)g_stConnectParam.au8Password);

exit:
	if (pConfData)
		free(pConfData);
	return ret;


}

//int saveWifiConfig()
//{
//	FILE* fp = NULL;
//	cJSON * root;
//	cJSON * obj;
//	cJSON * connections;
//	cJSON * connect;
//	cJSON * item;
//	char id[8];
//
//	fp = fopen(WIFI_SETTING_CFG,"w+");
//	if (!fp)
//	{
//		printf("should open json file first\n");
//		return -1;
//	}
//
//	printf("open %s success\n", WIFI_SETTING_CFG);
//	root = cJSON_CreateObject();
//	obj = cJSON_AddObjectToObject(root, "wifi");
//	item = cJSON_AddBoolToObject(obj, "isSupport", isWifiSupport);
//	item = cJSON_AddBoolToObject(obj, "isEnable", isWifiEnable);
//	item = cJSON_AddStringToObject(obj, "ssid", (char*)stConnectParam.au8SSId);
//	connections = cJSON_AddObjectToObject(obj, "connections");
//	connect = cJSON_AddObjectToObject(connections, (char*)stConnectParam.au8SSId);
//	memset(id, 0, sizeof(id));
//	sprintf(id, "%d", wlanHdl);
//	item = cJSON_AddStringToObject(connect, "id", id);
//	item = cJSON_AddStringToObject(connect, "passwd", (char*)stConnectParam.au8Password);
//	printf("%s %d %s \n",__FUNCTION__,__LINE__,cJSON_Print(root));
//
//	fseek(fp, 0, SEEK_SET);
//	fwrite(cJSON_Print(root),strlen(cJSON_Print(root)),1,fp);
//	fclose(fp);
//	fp = NULL;
//
//	return 0;
//}

int saveWifiConfig2()
{
	FILE* fp = NULL;
	char id[8];

	fp = fopen(WIFI_SETTING_CFG,"w+");
	if (!fp)
	{
		printf("should open json file first\n");
		return -1;
	}

	fseek(fp, 0, SEEK_SET);

	// update json info



	fwrite(cJSON_Print(g_pRoot),strlen(cJSON_Print(g_pRoot)),1,fp);
	fclose(fp);
	fp = NULL;
	cJSON_Delete(g_pRoot);
	g_pRoot = NULL;

	return 0;
}

// read json -> do init
// query ssid in json
// add ssid to json
// delete ssid from json
int addNewWifiConnect()
{


}

int deleteWifiConnect()
{


}

int updateWifiConnect()
{

}

// update passwd of ssid in json
// CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string);
// CJSON_PUBLIC(void) cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);


int initWifiConfig()
{
	FILE* fp = NULL;
	long long len = 0;
	char * pConfData = NULL;
	cJSON * root;
	cJSON * obj;
	cJSON * param;
	cJSON * item;
	cJSON_Hooks hooks;

	memset(&g_stConnectParam, 0, sizeof(MI_WLAN_ConnectParam_t));
	g_stConnectParam.eSecurity = E_MI_WLAN_SECURITY_WPA;
	g_stConnectParam.OverTimeMs = 5000;

	fp = fopen(WIFI_SETTING_CFG,"r+");
	if (!fp)
	{
		printf("should open json file first\n");
		return -1;
	}

	printf("open %s success\n", WIFI_SETTING_CFG);

	hooks.free_fn = free_fn;
	hooks.malloc_fn = malloc_fn;
	cJSON_InitHooks(&hooks);

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	pConfData = (char *)malloc(len + 1);
	fseek(fp, 0, SEEK_SET);
	fread(pConfData, len, 1, fp);
	fclose(fp);
	fp = NULL;

	// read config
	cJSON_Minify(pConfData);
	root = cJSON_Parse(pConfData);
	if (!root)
		return -1;

	obj = cJSON_GetObjectItem(root, "wifi");
	if (!obj)
		return -1;

	printf("parse json success\n");
	item = cJSON_GetObjectItem(obj, "isSupport");
	if (item)
		g_bWifiSupport = cJSON_IsTrue(item);
	printf("isSupport: %d\n", g_bWifiSupport);

	item = cJSON_GetObjectItem(obj, "isEnable");
	if (item)
		g_bWifiEnable = cJSON_IsTrue(item);
	printf("isSupport: %d\n", g_bWifiEnable);

	param = cJSON_GetObjectItem(obj, "param");
	if (param)
	{
		item = cJSON_GetObjectItem(param, "id");
		if(item)
		{
			g_hWlan = (WLAN_HANDLE)atoi(cJSON_GetStringValue(item));
			item = cJSON_GetObjectItem(param, "ssid");
			strcpy((char*)g_stConnectParam.au8SSId, cJSON_GetStringValue(item));
			item = cJSON_GetObjectItem(param, "passwd");
			strcpy((char*)g_stConnectParam.au8Password, cJSON_GetStringValue(item));
		}
	}

	printf("isSupport:%d isEnable:%d id:%d ssid:%s passwd:%s\n", g_bWifiSupport, g_bWifiEnable, g_hWlan, (char*)g_stConnectParam.au8SSId, (char*)g_stConnectParam.au8Password);

	cJSON_Delete(root);
	free(pConfData);
	return 0;
}

int saveWifiConfig()
{
	FILE* fp = NULL;
	cJSON * root;
	cJSON * param;
	cJSON * obj;
	cJSON * item;
	char id[8];

	fp = fopen(WIFI_SETTING_CFG,"w+");
	if (!fp)
	{
		printf("should open json file first\n");
		return -1;
	}

	printf("open %s success\n", WIFI_SETTING_CFG);
	root = cJSON_CreateObject();
	obj = cJSON_AddObjectToObject(root, "wifi");
	item = cJSON_AddBoolToObject(obj, "isSupport", g_bWifiSupport);
	item = cJSON_AddBoolToObject(obj, "isEnable", g_bWifiEnable);
	param = cJSON_AddObjectToObject(obj, "param");
	memset(id, 0, sizeof(id));
	sprintf(id, "%d", g_hWlan);
	item = cJSON_AddStringToObject(param, "id", id);
	item = cJSON_AddStringToObject(param, "ssid", (char*)g_stConnectParam.au8SSId);
	item = cJSON_AddStringToObject(param, "passwd", (char*)g_stConnectParam.au8Password);
	printf("%s %d %s \n",__FUNCTION__,__LINE__,cJSON_Print(root));

	fseek(fp, 0, SEEK_SET);
	fwrite(cJSON_Print(root),strlen(cJSON_Print(root)),1,fp);
	//fsync(fp);
	cJSON_Delete(root);
	fclose(fp);
	fp = NULL;

	return 0;
}


int checkProfile()
{
	FILE *pCfgFile = NULL;
	FILE *pBackupFile = NULL;
	char cmd[128];

	pCfgFile = fopen(WPA_CFG, "r+");
	if (pCfgFile)
	{
		if (!feof(pCfgFile))
		{
			fclose(pCfgFile);
			return 0;
		}

		fclose(pCfgFile);
	}

	// recover cfg file
	pBackupFile = fopen(WPA_CFG_BACKUP, "r+");
	if (!pBackupFile)
		return -1;

	fclose(pBackupFile);
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "cp %s %s", WPA_CFG_BACKUP, WPA_CFG);
	system(cmd);

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "rm %s", WIFI_SETTING_CFG);
	system(cmd);

	return 0;
}
#endif
