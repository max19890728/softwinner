/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Net/ux363_network_manager.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <thread>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <stdbool.h>

#include "Device/US363/Data/wifi_config.h"
#include "Device/US363/System/sys_time.h"
#include "device_model/system/net/net_manager.h"
#include "device_model/system/net/wifi_connector.h"
#include "device_model/system/net/softap_controller.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US363NetworkManager"

using namespace EyeseeLinux;

char wifiApSsidOsc[32] = "US_0000.OSC\0", wifiApPasswordOsc[16] = "88888888\0";
char wifiStaSsid[32] = "StaSsid\0", wifiStaPassword[16] = "00000000\0";
int wifiApChannel = 6;

int startWifiAp(char *ssid, char *password, int channel, int type)
{
	NetManager::NetLinkType netlink_type;
	
	checkWifiApPassword(password);
	sprintf(wifiApSsidOsc, "%s.OSC", ssid);
	sprintf(wifiApPasswordOsc, "%s", password);
    wifiApChannel = channel;
	
	NetManager::GetInstance()->GetNetLinkType(netlink_type);
	if(netlink_type != NetManager::NETLINK_NONE)
		NetManager::GetInstance()->DisableAllNetLink();
	std::thread([] { NetManager::GetInstance()->SwitchToSoftAp(&wifiApSsidOsc[0], &wifiApPasswordOsc[0], wifiApChannel, 0, 0, 0); }).join();
	return 1;
}

void stopWifiAp()
{
	NetManager::NetLinkType netlink_type;
	NetManager::GetInstance()->GetNetLinkType(netlink_type);
	if(netlink_type == NetManager::NETLINK_WIFI_SOFTAP)
		std::thread([] { NetManager::GetInstance()->DisableSoftap(); }).join();
}

bool isWifiApEnabled()
{
	NetManager::NetLinkType netlink_type;
	NetManager::GetInstance()->GetNetLinkType(netlink_type);
	return ((netlink_type == NetManager::NETLINK_WIFI_SOFTAP)? true: false);
}

void stratWifiSta(char *ssid, char *password, int linkType)
{
	NetManager::NetLinkType netlink_type;
	
	sprintf(wifiStaSsid, "%s", ssid);
	sprintf(wifiStaPassword, "%s", password);
	if(netlink_type != NetManager::NETLINK_NONE)
		NetManager::GetInstance()->DisableAllNetLink();
	std::thread([] { NetManager::GetInstance()->EnableMonitorMode("wlan0", wifiStaSsid, wifiStaPassword); }).join();
}

/*public String linkWifi(String newSSID, String newPassword, int linkType){
	
	List<WifiConfiguration> configList = wifiManager.getConfiguredNetworks();
	boolean configExist = false;
	int i = 0;
	String defaultlink = "\""+ newSSID + "\"";
	if(configList != null && configList.size() > 0){
	   for (i = 0; i < configList.size(); i++) {
		   String configSSID = configList.get(i).SSID;
		   Log.d("webService", "config ssid" + configSSID);
		   wifiManager.forget(configList.get(i).networkId, null);
		   //wifiManager.removeNetwork(configList.get(i).networkId);
	   }
	}
	try {
		Thread.sleep(50);
	} catch (InterruptedException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
	int reTry = 0;
	boolean isFind = false;
	int keyMgmt = 0;
	while(reTry < 500){
		List<ScanResult> scanResultList = wifiManager.getScanResults();
		if(scanResultList.size() <= 0){
			//Log.d("webService", "noScan any wifi");
		}else{
			for (int x = 0; x < scanResultList.size(); x++) {
				String scanListSSID = scanResultList.get(x).SSID;
				Log.d("webService", "scan ssid: " + scanListSSID);
				Log.d("webService", scanResultList.get(x).capabilities +" / " + scanResultList.get(x).BSSID);
				if(newSSID.equals(scanListSSID)){
					String capabilities = scanResultList.get(x).capabilities;
					if(capabilities.indexOf("WPA2-PSK") != -1){
						keyMgmt = 0;
					}else if(capabilities.indexOf("WPA-PSK") != -1){
						keyMgmt = 1;
					}else if(capabilities.indexOf("WPA-EAP") != -1){
						keyMgmt = 2;
					}
					Log.d("webService", "keyMgmt type: " + keyMgmt);
					//scanResultList.get(x).
					isFind = true;
					break;
				}
			}
		}
		if(isFind){
			break;
		}
		try {
			Thread.sleep(20);
			reTry++;
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	if(reTry >= 500){
		Log.d("webService", "noScan " + newSSID + " wifi");
		return "";
	}
	if(configExist){
		boolean success = wifiManager.enableNetwork(configList.get(i).networkId, true);
	   if (success) {
			Log.d("webService", "showWIFI: success " + configList.get(i).SSID);
			WifiInfo wifiInfo = wifiManager.getConnectionInfo();
			int ipAddress = wifiInfo.getIpAddress();
			String ip = String.format("%d.%d.%d.%d",(ipAddress & 0xff),(ipAddress >> 8 & 0xff),(ipAddress >> 16 & 0xff),(ipAddress >> 24 & 0xff));
			String ssid = Main.parseSSID(wifiInfo.getSSID());
			if(Main.wifiSSID.equals(ssid)) {
				Log.d("webService", "linkStart() 07-2");
				return ip;
			}
	   }
	}else{
		WifiConfiguration wifiConfig = new WifiConfiguration();
		wifiConfig.SSID = "\"" + newSSID + "\"";
		wifiConfig.BSSID = null;
		wifiConfig.status = WifiConfiguration.Status.ENABLED;
		wifiConfig.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);
		wifiConfig.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.CCMP);
		wifiConfig.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
		wifiConfig.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.TKIP);
		wifiConfig.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.CCMP);
		// String password = "88888888";
								
		if (newPassword.matches("[0-9A-Fa-f]{64}")) {
			wifiConfig.preSharedKey = newPassword;
		} else {
			wifiConfig.preSharedKey = '"' + newPassword + '"';
		}
		Log.d("webService", "config password: " + wifiConfig.preSharedKey);
		wifiConfig.networkId = wifiManager.addNetwork(wifiConfig);
		configList = wifiManager.getConfiguredNetworks();
		for (int x = 0; x < configList.size(); x++) {
			   String configSSID = configList.get(x).SSID;
			   Log.d("webService", "config ssid" + configSSID + ", ID: " + configList.get(x).networkId);
			   if(defaultlink.equals(configSSID)){
				   wifiConfig = configList.get(x);
				   break;
			   }
		}
		if(wifiConfig.networkId <= -1){
			Log.d("webService", "wifiConfig Error getID: " + wifiConfig.networkId);
			return "";
		}
		wifiManager.saveConfiguration();
		boolean success = wifiManager.enableNetwork(wifiConfig.networkId, true);
		if (success) {
			Log.d("webService", "showWIFI2: success " + wifiConfig.SSID);
			reTry = 0;
			WifiInfo wifiInfo;
			int ipAddress;
			String ip, ssid;
			while(reTry < 150){
				wifiInfo = wifiManager.getConnectionInfo();
				ipAddress = wifiInfo.getIpAddress();
				ip = String.format("%d.%d.%d.%d",(ipAddress & 0xff),(ipAddress >> 8 & 0xff),(ipAddress >> 16 & 0xff),(ipAddress >> 24 & 0xff));
				ssid = Main.parseSSID(wifiInfo.getSSID());
				Log.d("webService", "get ssid: " + ssid + " ip: " + ip);
				if(Main.wifiSSID.equals(ssid) && !ip.equals("0.0.0.0")) {
					Log.d("webService", "linkStart() 07-2");
					return ip;
				}
				try {
					Thread.sleep(200);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				reTry++;
				if(reTry % 50 == 0){
					configList = wifiManager.getConfiguredNetworks();
					for (int x = 0; x < configList.size(); x++) {
						   String configSSID = configList.get(x).SSID;
						   Log.d("webService", "config ssid" + configSSID + ", ID: " + configList.get(x).networkId);
					}
					Log.d("webService", "link ID: " + wifiConfig.networkId);
					wifiManager.enableNetwork(wifiConfig.networkId, true);
				}
			}
			
		}
	}
	
	return "";
}*/