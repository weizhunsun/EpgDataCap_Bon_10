﻿﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;
using System.Collections;
using Microsoft.Win32;
using System.Runtime.InteropServices;
using System.ComponentModel;

namespace EpgTimer
{
    class IniFileHandler
    {
        [DllImport("KERNEL32.DLL")]
        public static extern uint
          GetPrivateProfileString(string lpAppName,
          string lpKeyName, string lpDefault,
          StringBuilder lpReturnedString, uint nSize,
          string lpFileName);

        [DllImport("KERNEL32.DLL",
            EntryPoint = "GetPrivateProfileStringA")]
        public static extern uint
          GetPrivateProfileStringByByteArray(string lpAppName,
          string lpKeyName, string lpDefault,
          byte[] lpReturnedString, uint nSize,
          string lpFileName);

        [DllImport("KERNEL32.DLL")]
        public static extern int
          GetPrivateProfileInt(string lpAppName,
          string lpKeyName, int nDefault, string lpFileName);

        [DllImport("KERNEL32.DLL")]
        public static extern uint WritePrivateProfileString(
          string lpAppName,
          string lpKeyName,
          string lpString,
          string lpFileName);
    }

    class SettingPath
    {
        public static string CommonIniPath
        {
            get
            {
                string iniPath = System.IO.Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
                iniPath += "\\Common.ini";
                return iniPath;
            }
        }
        public static string TimerSrvIniPath
        {
            get
            {
                string iniPath = System.IO.Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
                iniPath += "\\EpgTimerSrv.ini";
                return iniPath;
            }
        }
        public static string DefSettingFolderPath
        {
            get
            {
//                string defSetPath = System.Environment.GetFolderPath(Environment.SpecialFolder.Personal);
//                defSetPath += "\\EpgTimerBon";
                string defSetPath = System.IO.Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
                defSetPath += "\\Setting";

                return defSetPath;
            }
        }
        public static string SettingFolderPath
        {
            get
            {
                string iniPath = System.IO.Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
                iniPath += "\\Common.ini";
                string defSetPath = System.IO.Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
                defSetPath += "\\Setting";
                StringBuilder buff = new StringBuilder(512);
                IniFileHandler.GetPrivateProfileString("SET", "DataSavePath", defSetPath, buff, 512, iniPath);
                return  buff.ToString();
            }
        }
        public static void CheckFolderPath(ref string folderPath)
        {
            if( folderPath.LastIndexOf("\\") == folderPath.Length-1 ){
                folderPath = folderPath.Remove(folderPath.Length - 1);
            }
        }
        public static string ModulePath
        {
            get
            {
                return System.IO.Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
            }
        }
    }

    public class Settings
    {
        private bool useCustomEpgView;
        private List<CustomEpgTabInfo> customEpgTabList;
        private double minHeight;
        private double serviceWidth;
        private double scrollSize;
        private string fontName;
        private double fontSize;
        private string fontNameTitle;
        private double fontSizeTitle;
        private bool fontBoldTitle;
        private bool noToolTip;
        private bool noBallonTips;
        private double dragScroll;
        private List<string> contentColorList;
        private List<UInt32> contentCustColorList;
        private string reserveRectColorNormal;
        private string reserveRectColorNo;
        private string reserveRectColorNoTuner;
        private string reserveRectColorWarning;
        private bool reserveRectBackground;
        private bool epgToolTip;
        private bool epgTitleIndent;
        private bool epgToolTipNoViewOnly;
        private int epgToolTipViewWait;
        private double resColumnWidth0;
        private double resColumnWidth1;
        private double resColumnWidth2;
        private double resColumnWidth3;
        private double resColumnWidth4;
        private double resColumnWidth5;
        private double resColumnWidth6;
        private double resColumnWidth7;
        private string resColumnHead;
        private ListSortDirection resSortDirection;
        private System.Windows.WindowState lastWindowState;
        private double mainWndLeft;
        private double mainWndTop;
        private double mainWndWidth;
        private double mainWndHeight;
        private bool closeMin;
        private bool wakeMin;
        private List<string> viewButtonList;
        private List<string> taskMenuList;
        private string cust1BtnName;
        private string cust1BtnCmd;
        private string cust1BtnCmdOpt;
        private string cust2BtnName;
        private string cust2BtnCmd;
        private string cust2BtnCmdOpt;
        private List<string> andKeyList;
        private List<string> notKeyList;
        private bool searchKeyRegExp;
        private bool searchKeyTitleOnly;
        private bool searchKeyAimaiFlag;
        private bool searchKeyNotContent;
        private bool searchKeyNotDate;
        private List<ContentKindInfo> searchKeyContentList;
        private List<DateItem> searchKeyDateItemList;
        private List<Int64> searchKeyServiceList;
        private byte searchKeyFreeCA;
        private List<RecPresetItem> recPresetList;
        private double recInfoColumnWidth0;
        private double recInfoColumnWidth1;
        private double recInfoColumnWidth2;
        private double recInfoColumnWidth3;
        private double recInfoColumnWidth4;
        private double recInfoColumnWidth5;
        private double recInfoColumnWidth6;
        private string recInfoColumnHead;
        private ListSortDirection recInfoSortDirection;
        private string tvTestExe;
        private string tvTestCmd;
        private bool nwTvMode;
        private bool nwTvModeUDP;
        private bool nwTvModeTCP;
        private string filePlayExe;
        private string filePlayCmd;
        private List<IEPGStationInfo> iEpgStationList;
        private string nwServerIP;
        private UInt32 nwServerPort;
        private UInt32 nwWaitPort;
        private string nwMacAdd;
        private bool wakeReconnectNW;
        private bool suspendCloseNW;
        private bool ngAutoEpgLoadNW;

        public bool UseCustomEpgView
        {
            get { return useCustomEpgView; }
            set { useCustomEpgView = value; }
        }
        public List<CustomEpgTabInfo> CustomEpgTabList
        {
            get { return customEpgTabList; }
            set { customEpgTabList = value; }
        }
        public double MinHeight
        {
            get { return minHeight; }
            set { minHeight = value; }
        }
        public double ServiceWidth
        {
            get { return serviceWidth; }
            set { serviceWidth = value; }
        }
        public double ScrollSize
        {
            get { return scrollSize; }
            set { scrollSize = value; }
        }
        public string FontName
        {
            get { return fontName; }
            set { fontName = value; }
        }
        public double FontSize
        {
            get { return fontSize; }
            set { fontSize = value; }
        }
        public string FontNameTitle
        {
            get { return fontNameTitle; }
            set { fontNameTitle = value; }
        }
        public double FontSizeTitle
        {
            get { return fontSizeTitle; }
            set { fontSizeTitle = value; }
        }
        public bool FontBoldTitle
        {
            get { return fontBoldTitle; }
            set { fontBoldTitle = value; }
        }
        public bool NoToolTip
        {
            get { return noToolTip; }
            set { noToolTip = value; }
        }
        public bool NoBallonTips
        {
            get { return noBallonTips; }
            set { noBallonTips = value; }
        }
        public double DragScroll
        {
            get { return dragScroll; }
            set { dragScroll = value; }
        }
        public List<string> ContentColorList
        {
            get { return contentColorList; }
            set { contentColorList = value; }
        }
        public List<UInt32> ContentCustColorList
        {
            get { return contentCustColorList; }
            set { contentCustColorList = value; }
        }
        public string ReserveRectColorNormal
        {
            get { return reserveRectColorNormal; }
            set { reserveRectColorNormal = value; }
        }
        public string ReserveRectColorNo
        {
            get { return reserveRectColorNo; }
            set { reserveRectColorNo = value; }
        }
        public string ReserveRectColorNoTuner
        {
            get { return reserveRectColorNoTuner; }
            set { reserveRectColorNoTuner = value; }
        }
        public string ReserveRectColorWarning
        {
            get { return reserveRectColorWarning; }
            set { reserveRectColorWarning = value; }
        }
        public bool ReserveRectBackground
        {
            get { return reserveRectBackground; }
            set { reserveRectBackground = value; }
        }
        public bool EpgToolTip
        {
            get { return epgToolTip; }
            set { epgToolTip = value; }
        }
        public bool EpgTitleIndent
        {
            get { return epgTitleIndent; }
            set { epgTitleIndent = value; }
        }
        public bool EpgToolTipNoViewOnly
        {
            get { return epgToolTipNoViewOnly; }
            set { epgToolTipNoViewOnly = value; }
        }
        public int EpgToolTipViewWait
        {
            get { return epgToolTipViewWait; }
            set { epgToolTipViewWait = value; }
        }
        public double ResColumnWidth0
        {
            get { return resColumnWidth0; }
            set { resColumnWidth0 = value; }
        }
        public double ResColumnWidth1
        {
            get { return resColumnWidth1; }
            set { resColumnWidth1 = value; }
        }
        public double ResColumnWidth2
        {
            get { return resColumnWidth2; }
            set { resColumnWidth2 = value; }
        }
        public double ResColumnWidth3
        {
            get { return resColumnWidth3; }
            set { resColumnWidth3 = value; }
        }
        public double ResColumnWidth4
        {
            get { return resColumnWidth4; }
            set { resColumnWidth4 = value; }
        }
        public double ResColumnWidth5
        {
            get { return resColumnWidth5; }
            set { resColumnWidth5 = value; }
        }
        public double ResColumnWidth6
        {
            get { return resColumnWidth6; }
            set { resColumnWidth6 = value; }
        }
        public double ResColumnWidth7
        {
            get { return resColumnWidth7; }
            set { resColumnWidth7 = value; }
        }
        public string ResColumnHead
        {
            get { return resColumnHead; }
            set { resColumnHead = value; }
        }
        public ListSortDirection ResSortDirection
        {
            get { return resSortDirection; }
            set { resSortDirection = value; }
        }
        public System.Windows.WindowState LastWindowState
        {
            get { return lastWindowState; }
            set { lastWindowState = value; }
        }
        public double MainWndLeft
        {
            get { return mainWndLeft; }
            set { mainWndLeft = value; }
        }
        public double MainWndTop
        {
            get { return mainWndTop; }
            set { mainWndTop = value; }
        }
        public double MainWndWidth
        {
            get { return mainWndWidth; }
            set { mainWndWidth = value; }
        }
        public double MainWndHeight
        {
            get { return mainWndHeight; }
            set { mainWndHeight = value; }
        }
        public bool CloseMin
        {
            get { return closeMin; }
            set { closeMin = value; }
        }
        public bool WakeMin
        {
            get { return wakeMin; }
            set { wakeMin = value; }
        }
        public List<string> ViewButtonList
        {
            get { return viewButtonList; }
            set { viewButtonList = value; }
        }
        public List<string> TaskMenuList
        {
            get { return taskMenuList; }
            set { taskMenuList = value; }
        }
        public string Cust1BtnName
        {
            get { return cust1BtnName; }
            set { cust1BtnName = value; }
        }
        public string Cust1BtnCmd
        {
            get { return cust1BtnCmd; }
            set { cust1BtnCmd = value; }
        }
        public string Cust1BtnCmdOpt
        {
            get { return cust1BtnCmdOpt; }
            set { cust1BtnCmdOpt = value; }
        }
        public string Cust2BtnName
        {
            get { return cust2BtnName; }
            set { cust2BtnName = value; }
        }
        public string Cust2BtnCmd
        {
            get { return cust2BtnCmd; }
            set { cust2BtnCmd = value; }
        }
        public string Cust2BtnCmdOpt
        {
            get { return cust2BtnCmdOpt; }
            set { cust2BtnCmdOpt = value; }
        }
        public List<string> AndKeyList
        {
            get { return andKeyList; }
            set { andKeyList = value; }
        }
        public List<string> NotKeyList
        {
            get { return notKeyList; }
            set { notKeyList = value; }
        }
        public bool SearchKeyRegExp
        {
            get { return searchKeyRegExp; }
            set { searchKeyRegExp = value; }
        }
        public bool SearchKeyTitleOnly
        {
            get { return searchKeyTitleOnly; }
            set { searchKeyTitleOnly = value; }
        }
        public bool SearchKeyAimaiFlag
        {
            get { return searchKeyAimaiFlag; }
            set { searchKeyAimaiFlag = value; }
        }
        public bool SearchKeyNotContent
        {
            get { return searchKeyNotContent; }
            set { searchKeyNotContent = value; }
        }
        public bool SearchKeyNotDate
        {
            get { return searchKeyNotDate; }
            set { searchKeyNotDate = value; }
        }
        public List<ContentKindInfo> SearchKeyContentList
        {
            get { return searchKeyContentList; }
            set { searchKeyContentList = value; }
        }
        public List<DateItem> SearchKeyDateItemList
        {
            get { return searchKeyDateItemList; }
            set { searchKeyDateItemList = value; }
        }
        public List<Int64> SearchKeyServiceList
        {
            get { return searchKeyServiceList; }
            set { searchKeyServiceList = value; }
        }
        public byte SearchKeyFreeCA
        {
            get { return searchKeyFreeCA; }
            set { searchKeyFreeCA = value; }
        }
        public List<RecPresetItem> RecPresetList
        {
            get { return recPresetList; }
            set { recPresetList = value; }
        }
        public double RecInfoColumnWidth0
        {
            get { return recInfoColumnWidth0; }
            set { recInfoColumnWidth0 = value; }
        }
        public double RecInfoColumnWidth1
        {
            get { return recInfoColumnWidth1; }
            set { recInfoColumnWidth1 = value; }
        }
        public double RecInfoColumnWidth2
        {
            get { return recInfoColumnWidth2; }
            set { recInfoColumnWidth2 = value; }
        }
        public double RecInfoColumnWidth3
        {
            get { return recInfoColumnWidth3; }
            set { recInfoColumnWidth3 = value; }
        }
        public double RecInfoColumnWidth4
        {
            get { return recInfoColumnWidth4; }
            set { recInfoColumnWidth4 = value; }
        }
        public double RecInfoColumnWidth5
        {
            get { return recInfoColumnWidth5; }
            set { recInfoColumnWidth5 = value; }
        }
        public double RecInfoColumnWidth6
        {
            get { return recInfoColumnWidth6; }
            set { recInfoColumnWidth6 = value; }
        }
        public string RecInfoColumnHead
        {
            get { return recInfoColumnHead; }
            set { recInfoColumnHead = value; }
        }
        public ListSortDirection RecInfoSortDirection
        {
            get { return recInfoSortDirection; }
            set { recInfoSortDirection = value; }
        }
        public string TvTestExe
        {
            get { return tvTestExe; }
            set { tvTestExe = value; }
        }
        public string TvTestCmd
        {
            get { return tvTestCmd; }
            set { tvTestCmd = value; }
        }
        public bool NwTvMode
        {
            get { return nwTvMode; }
            set { nwTvMode = value; }
        }
        public bool NwTvModeUDP
        {
            get { return nwTvModeUDP; }
            set { nwTvModeUDP = value; }
        }
        public bool NwTvModeTCP
        {
            get { return nwTvModeTCP; }
            set { nwTvModeTCP = value; }
        }
        public string FilePlayExe
        {
            get { return filePlayExe; }
            set { filePlayExe = value; }
        }
        public string FilePlayCmd
        {
            get { return filePlayCmd; }
            set { filePlayCmd = value; }
        }
        public List<IEPGStationInfo> IEpgStationList
        {
            get { return iEpgStationList; }
            set { iEpgStationList = value; }
        }
        public string NWServerIP
        {
            get { return nwServerIP; }
            set { nwServerIP = value; }
        }
        public UInt32 NWServerPort
        {
            get { return nwServerPort; }
            set { nwServerPort = value; }
        }
        public UInt32 NWWaitPort
        {
            get { return nwWaitPort; }
            set { nwWaitPort = value; }
        }
        public string NWMacAdd
        {
            get { return nwMacAdd; }
            set { nwMacAdd = value; }
        }
        public bool WakeReconnectNW
        {
            get { return wakeReconnectNW; }
            set { wakeReconnectNW = value; }
        }
        public bool SuspendCloseNW
        {
            get { return suspendCloseNW; }
            set { suspendCloseNW = value; }
        }
        public bool NgAutoEpgLoadNW
        {
            get { return ngAutoEpgLoadNW; }
            set { ngAutoEpgLoadNW = value; }
        }

        public Settings()
        {
            useCustomEpgView = false;
            customEpgTabList = new List<CustomEpgTabInfo>();
            minHeight = 2;
            serviceWidth = 150;
            scrollSize = 240;
            fontName = "メイリオ";
            fontSize = 12;
            fontNameTitle = "メイリオ";
            fontSizeTitle = 12;
            fontBoldTitle = true;
            noToolTip = false;
            dragScroll = 1.5;
            contentColorList = new List<string>();
            contentCustColorList = new List<uint>();
            reserveRectColorNormal = "Lime";
            reserveRectColorNo = "Black";
            reserveRectColorNoTuner = "Red";
            reserveRectColorWarning = "Yellow";
            reserveRectBackground = false;
            epgToolTip = false;
            epgTitleIndent = true;
            epgToolTipNoViewOnly = true;
            epgToolTipViewWait = 1500;
            resColumnHead = "";
            resSortDirection = ListSortDirection.Ascending;
            lastWindowState = System.Windows.WindowState.Normal;
            mainWndLeft = 0;
            mainWndTop = 0;
            mainWndWidth = 0;
            mainWndHeight = 0;
            closeMin = false;
            wakeMin = false;
            viewButtonList = new List<string>();
            taskMenuList = new List<string>();
            cust1BtnName = "";
            cust1BtnCmd = "";
            cust1BtnCmdOpt = "";
            cust2BtnName = "";
            cust2BtnCmd = "";
            cust2BtnCmdOpt = "";
            andKeyList = new List<string>();
            notKeyList = new List<string>();
            searchKeyRegExp = false;
            searchKeyTitleOnly = false;
            searchKeyAimaiFlag = false;
            searchKeyNotContent = false;
            searchKeyNotDate = false;
            searchKeyFreeCA = 0;
            searchKeyContentList = new List<ContentKindInfo>();
            searchKeyDateItemList = new List<DateItem>();
            searchKeyServiceList = new List<Int64>();
            recPresetList = new List<RecPresetItem>();
            recInfoColumnHead = "";
            recInfoSortDirection = ListSortDirection.Ascending;
            tvTestExe = "";
            tvTestCmd = "";
            nwTvMode = false;
            nwTvModeUDP = false;
            nwTvModeTCP = false;
            filePlayExe = "";
            filePlayCmd = "\"$FilePath$\"";
            iEpgStationList = new List<IEPGStationInfo>();
            nwServerIP = "";
            nwServerPort = 4510;
            nwWaitPort = 4520;
            nwMacAdd = "";
            wakeReconnectNW = false;
            suspendCloseNW = false;
            ngAutoEpgLoadNW = false;
        }

        [NonSerialized()]
        private static Settings _instance;
        [System.Xml.Serialization.XmlIgnore]
        public static Settings Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new Settings();
                return _instance;
            }
            set { _instance = value; }
        }

        /// <summary>
        /// EpgTimer用設定ファイルロード関数
        /// </summary>
        public static void LoadFromXmlFile()
        {
            string path = GetSettingPath();

            try
            {
                FileStream fs = new FileStream(path,
                    FileMode.Open,
                    FileAccess.Read);
                System.Xml.Serialization.XmlSerializer xs =
                    new System.Xml.Serialization.XmlSerializer(
                        typeof(Settings));
                //読み込んで逆シリアル化する
                object obj = xs.Deserialize(fs);
                fs.Close();
                Instance = (Settings)obj;

            }
            catch
            {
            }
            finally
            {
                if (Instance.contentColorList.Count == 0)
                {
                    for (int i = 0; i < 0x11; i++)
                    {
                        Instance.contentColorList.Add("White");
                    }
                }
                else if (Instance.contentColorList.Count == 0x10)
                {
                    Instance.contentColorList.Add("White");
                }
                if (Instance.ContentCustColorList.Count == 0)
                {
                    for (int i = 0; i < 0x11+4; i++)
                    {
                        Instance.ContentCustColorList.Add(0xFFFFFFFF);
                    }
                }
                if (Instance.viewButtonList.Count == 0)
                {
                    Instance.viewButtonList.Add("設定");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("検索");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("スタンバイ");
                    Instance.viewButtonList.Add("休止");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("EPG取得");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("EPG再読み込み");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("終了");
                }
                if (Instance.taskMenuList.Count == 0)
                {
                    Instance.taskMenuList.Add("設定");
                    Instance.taskMenuList.Add("（セパレータ）");
                    Instance.taskMenuList.Add("スタンバイ");
                    Instance.taskMenuList.Add("休止");
                    Instance.taskMenuList.Add("（セパレータ）");
                    Instance.taskMenuList.Add("終了");
                }
            }
        }

        /// <summary>
        /// EpgTimerNW用の設定ファイルロード関数
        /// </summary>
        public static void LoadFromXmlFileNW()
        {
            string path = GetSettingPath();

            try
            {
                FileStream fs = new FileStream(path,
                    FileMode.Open,
                    FileAccess.Read);
                System.Xml.Serialization.XmlSerializer xs =
                    new System.Xml.Serialization.XmlSerializer(
                        typeof(Settings));
                //読み込んで逆シリアル化する
                object obj = xs.Deserialize(fs);
                fs.Close();
                Instance = (Settings)obj;

            }
            catch
            {
            }
            finally
            {
                if (Instance.contentColorList.Count == 0)
                {
                    for (int i = 0; i < 0x11; i++)
                    {
                        Instance.contentColorList.Add("White");
                    }
                }
                else if (Instance.contentColorList.Count == 0x10)
                {
                    Instance.contentColorList.Add("White");
                }
                if (Instance.ContentCustColorList.Count == 0)
                {
                    for (int i = 0; i < 0x11+4; i++)
                    {
                        Instance.ContentCustColorList.Add(0xFFFFFFFF);
                    }
                }
                if (Instance.viewButtonList.Count == 0)
                {
                    Instance.viewButtonList.Add("設定");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("再接続");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("検索");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("EPG取得");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("EPG再読み込み");
                    Instance.viewButtonList.Add("（空白）");
                    Instance.viewButtonList.Add("終了");
                }
                if (Instance.taskMenuList.Count == 0)
                {
                    Instance.taskMenuList.Add("設定");
                    Instance.taskMenuList.Add("（セパレータ）");
                    Instance.taskMenuList.Add("再接続");
                    Instance.taskMenuList.Add("（セパレータ）");
                    Instance.taskMenuList.Add("終了");
                }
            }
        }

        public static void SaveToXmlFile()
        {
            string path = GetSettingPath();

            try
            {
                FileStream fs = new FileStream(path,
                    FileMode.Create,
                    FileAccess.Write);
                System.Xml.Serialization.XmlSerializer xs =
                    new System.Xml.Serialization.XmlSerializer(
                    typeof(Settings));
                //シリアル化して書き込む
                xs.Serialize(fs, Instance);
                fs.Close();
            }
            catch
            {
            }
        }

        private static string GetSettingPath()
        {
            Assembly myAssembly = Assembly.GetEntryAssembly();
            string path = myAssembly.Location + ".xml";

            return path;
        }

        public static void GetDefRecSetting(UInt32 presetID, ref CtrlCmdCLI.Def.RecSettingData defKey)
        {
            StringBuilder buff = new StringBuilder(512);
            String defName = "REC_DEF";
            String defFolderName = "REC_DEF_FOLDER";
            String defFolder1SegName = "REC_DEF_FOLDER_1SEG";

            if (presetID > 0)
            {
                defName += presetID.ToString();
                defFolderName += presetID.ToString();
                defFolder1SegName += presetID.ToString();
            }

            defKey.RecMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "RecMode", 1, SettingPath.TimerSrvIniPath);
            defKey.Priority = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "Priority", 2, SettingPath.TimerSrvIniPath);
            defKey.TuijyuuFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "TuijyuuFlag", 1, SettingPath.TimerSrvIniPath);
            defKey.ServiceMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "ServiceMode", 0, SettingPath.TimerSrvIniPath);
            defKey.PittariFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "PittariFlag", 0, SettingPath.TimerSrvIniPath);

            buff.Clear();
            IniFileHandler.GetPrivateProfileString(defName, "BatFilePath", "", buff, 512, SettingPath.TimerSrvIniPath);
            defKey.BatFilePath = buff.ToString();

            int count = IniFileHandler.GetPrivateProfileInt(defFolderName, "Count", 0, SettingPath.TimerSrvIniPath);
            for (int i = 0; i < count; i++)
            {
                CtrlCmdCLI.Def.RecFileSetInfo folderInfo = new CtrlCmdCLI.Def.RecFileSetInfo();
                buff.Clear();
                IniFileHandler.GetPrivateProfileString(defFolderName, i.ToString(), "", buff, 512, SettingPath.TimerSrvIniPath);
                folderInfo.RecFolder = buff.ToString();
                buff.Clear();
                IniFileHandler.GetPrivateProfileString(defFolderName, "WritePlugIn" + i.ToString(), "Write_Default.dll", buff, 512, SettingPath.TimerSrvIniPath);
                folderInfo.WritePlugIn = buff.ToString();
                buff.Clear();
                IniFileHandler.GetPrivateProfileString(defFolderName, "RecNamePlugIn" + i.ToString(), "", buff, 512, SettingPath.TimerSrvIniPath);
                folderInfo.RecNamePlugIn = buff.ToString();

                defKey.RecFolderList.Add(folderInfo);
            }

            count = IniFileHandler.GetPrivateProfileInt(defFolder1SegName, "Count", 0, SettingPath.TimerSrvIniPath);
            for (int i = 0; i < count; i++)
            {
                CtrlCmdCLI.Def.RecFileSetInfo folderInfo = new CtrlCmdCLI.Def.RecFileSetInfo();
                buff.Clear();
                IniFileHandler.GetPrivateProfileString(defFolder1SegName, i.ToString(), "", buff, 512, SettingPath.TimerSrvIniPath);
                folderInfo.RecFolder = buff.ToString();
                buff.Clear();
                IniFileHandler.GetPrivateProfileString(defFolder1SegName, "WritePlugIn" + i.ToString(), "Write_Default.dll", buff, 512, SettingPath.TimerSrvIniPath);
                folderInfo.WritePlugIn = buff.ToString();
                buff.Clear();
                IniFileHandler.GetPrivateProfileString(defFolder1SegName, "RecNamePlugIn" + i.ToString(), "", buff, 512, SettingPath.TimerSrvIniPath);
                folderInfo.RecNamePlugIn = buff.ToString();

                defKey.PartialRecFolder.Add(folderInfo);
            }

            defKey.SuspendMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "SuspendMode", 0, SettingPath.TimerSrvIniPath);
            defKey.RebootFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "RebootFlag", 0, SettingPath.TimerSrvIniPath);
            defKey.UseMargineFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "UseMargineFlag", 0, SettingPath.TimerSrvIniPath);
            defKey.StartMargine = IniFileHandler.GetPrivateProfileInt(defName, "StartMargine", 0, SettingPath.TimerSrvIniPath);
            defKey.EndMargine = IniFileHandler.GetPrivateProfileInt(defName, "EndMargine", 0, SettingPath.TimerSrvIniPath);
            defKey.ContinueRecFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "ContinueRec", 0, SettingPath.TimerSrvIniPath);
            defKey.PartialRecFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "PartialRec", 0, SettingPath.TimerSrvIniPath);
            defKey.TunerID = (UInt32)IniFileHandler.GetPrivateProfileInt(defName, "TunerID", 0, SettingPath.TimerSrvIniPath);

        }

        public static void GetDefSearchSetting(ref CtrlCmdCLI.Def.EpgSearchKeyInfo defKey)
        {
            if (Settings.Instance.SearchKeyRegExp == true)
            {
                defKey.regExpFlag = 1;
            }
            if (Settings.Instance.SearchKeyAimaiFlag == true)
            {
                defKey.aimaiFlag = 1;
            }
            if (Settings.Instance.SearchKeyTitleOnly == true)
            {
                defKey.titleOnlyFlag = 1;
            }
            if (Settings.Instance.SearchKeyNotContent == true)
            {
                defKey.notContetFlag = 1;
            }
            if (Settings.Instance.SearchKeyNotDate == true)
            {
                defKey.notDateFlag = 1;
            }
            foreach (ContentKindInfo info in Settings.Instance.SearchKeyContentList)
            {
                CtrlCmdCLI.Def.EpgContentData item = new CtrlCmdCLI.Def.EpgContentData();
                item.content_nibble_level_1 = info.Nibble1;
                item.content_nibble_level_2 = info.Nibble2;
                defKey.contentList.Add(item);
            }
            foreach (DateItem info in Settings.Instance.SearchKeyDateItemList)
            {
                defKey.dateList.Add(info.DateInfo);
            }
            foreach (Int64 info in Settings.Instance.SearchKeyServiceList)
            {
                defKey.serviceList.Add(info);
            }
            defKey.freeCAFlag = Settings.Instance.SearchKeyFreeCA;
        }
    }
}
