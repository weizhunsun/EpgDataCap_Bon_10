﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// SearchWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchWindow : Window
    {
        private List<SearchItem> resultList = new List<SearchItem>();
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;

        string _lastHeaderClicked = null;
        ListSortDirection _lastDirection = ListSortDirection.Ascending;
        string _lastHeaderClicked2 = null;
        ListSortDirection _lastDirection2 = ListSortDirection.Ascending;

        private UInt32 autoAddID = 0;

        public SearchWindow()
        {
            InitializeComponent();

            try
            {
                EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
                Settings.GetDefSearchSetting(ref defKey);

                searchKeyView.SetSearchKey(defKey);
            }
            catch
            {
            }
        }

        public void GetSearchKey(ref EpgSearchKeyInfo key)
        {
            searchKeyView.GetSearchKey(ref key);
        }

        public void SetSearchDefKey(EpgSearchKeyInfo key)
        {
            searchKeyView.SetSearchKey(key);
        }

        public void SetRecInfoDef(RecSettingData set)
        {
            recSettingView.SetDefSetting(set);
        }

        public void SetViewMode(UInt16 mode)
        {
            if (mode == 1)
            {
                button_add_reserve.Visibility = System.Windows.Visibility.Hidden;
                button_add_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;

                Title = "EPG予約条件";
            }
            else if (mode == 2)
            {
                button_add_reserve.Visibility = System.Windows.Visibility.Hidden;
                button_add_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;

                Title = "EPG予約条件";
            }
            else
            {
                button_add_reserve.Visibility = System.Windows.Visibility.Visible;
                button_add_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;

                Title = "検索";
            }
        }

        public void SetChgAutoAddID(UInt32 id)
        {
            autoAddID = id;
        }

        private void tabItem_searchKey_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SearchPg();
            }
        }
        
        private void button_search_Click(object sender, RoutedEventArgs e)
        {
            SearchPg();
        }

        private void SearchPg()
        {
            try
            {
                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_result.DataContext);
                if (dataView != null)
                {
                    dataView.SortDescriptions.Clear();
                    dataView.Refresh();
                }
                listView_result.DataContext = null;

                resultList.Clear();


                EpgSearchKeyInfo key = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref key);
                List<EpgSearchKeyInfo> keyList = new List<EpgSearchKeyInfo>();

                keyList.Add(key);
                List<EpgEventInfo> list = new List<EpgEventInfo>();

                cmd.SendSearchPg(keyList, ref list);
                foreach (EpgEventInfo info in list)
                {
                    SearchItem item = new SearchItem();
                    item.EventInfo = info;
                    foreach (ReserveData info2 in CommonManager.Instance.DB.ReserveList.Values)
                    {
                        if (info.original_network_id == info2.OriginalNetworkID &&
                            info.transport_stream_id == info2.TransportStreamID &&
                            info.service_id == info2.ServiceID &&
                            info.event_id == info2.EventID)
                        {
                            item.ReserveInfo = info2;
                            break;
                        }
                    }

                    UInt64 serviceKey = CommonManager.Create64Key(info.original_network_id, info.transport_stream_id, info.service_id);
                    if (ChSet5.Instance.ChList.ContainsKey(serviceKey) == true)
                    {
                        item.ServiceName = ChSet5.Instance.ChList[serviceKey].ServiceName;
                    }
                    resultList.Add(item);
                }

                listView_result.DataContext = resultList;
                if (_lastHeaderClicked != null)
                {
                    Sort(_lastHeaderClicked, _lastDirection);
                }
                else
                {
                    string header = ((Binding)gridView_result.Columns[1].DisplayMemberBinding).Path.Path;
                    Sort(header, _lastDirection);
                    _lastHeaderClicked = header;
                }

                searchKeyView.SaveSearchLog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_add_reserve_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = listView_result.SelectedItem as SearchItem;
                    EpgEventInfo eventInfo = item.EventInfo;

                    if (eventInfo.StartTimeFlag == 0)
                    {
                        MessageBox.Show("開始時間未定のため予約できません");
                        return;
                    }

                    ReserveData reserveInfo = new ReserveData();
                    if (eventInfo.ShortInfo != null)
                    {
                        reserveInfo.Title = eventInfo.ShortInfo.event_name;
                    }

                    reserveInfo.StartTime = eventInfo.start_time;
                    reserveInfo.StartTimeEpg = eventInfo.start_time;

                    if (eventInfo.DurationFlag == 0)
                    {
                        reserveInfo.DurationSecond = 10 * 60;
                    }
                    else
                    {
                        reserveInfo.DurationSecond = eventInfo.durationSec;
                    }

                    UInt64 key = CommonManager.Create64Key(eventInfo.original_network_id, eventInfo.transport_stream_id, eventInfo.service_id);
                    if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                    {
                        reserveInfo.StationName = ChSet5.Instance.ChList[key].ServiceName;
                    }
                    reserveInfo.OriginalNetworkID = eventInfo.original_network_id;
                    reserveInfo.TransportStreamID = eventInfo.transport_stream_id;
                    reserveInfo.ServiceID = eventInfo.service_id;
                    reserveInfo.EventID = eventInfo.event_id;

                    RecSettingData setInfo = new RecSettingData();
                    recSettingView.GetRecSetting(ref setInfo);
                    reserveInfo.RecSetting = setInfo;

                    List<ReserveData> list = new List<ReserveData>();
                    list.Add(reserveInfo);
                    ErrCode err = (ErrCode)cmd.SendAddReserve(list);
                    if (err == ErrCode.CMD_ERR_CONNECT)
                    {
                        MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                    }
                    if (err == ErrCode.CMD_ERR_TIMEOUT)
                    {
                        MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                    }
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show("予約登録でエラーが発生しました。終了時間がすでに過ぎている可能性があります。");
                    }

                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_add_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                EpgAutoAddData addItem = new EpgAutoAddData();
                EpgSearchKeyInfo searchKey = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref searchKey);

                RecSettingData recSetKey = new RecSettingData();
                recSettingView.GetRecSetting(ref recSetKey);

                addItem.searchInfo = searchKey;
                addItem.recSetting = recSetKey;

                List<EpgAutoAddData> addList = new List<EpgAutoAddData>();
                addList.Add(addItem);

                if (cmd.SendAddEpgAutoAdd(addList) != 1)
                {
                    MessageBox.Show("追加に失敗しました");
                }
                else
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_chg_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                EpgAutoAddData addItem = new EpgAutoAddData();
                addItem.dataID = autoAddID;
                EpgSearchKeyInfo searchKey = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref searchKey);

                RecSettingData recSetKey = new RecSettingData();
                recSettingView.GetRecSetting(ref recSetKey);

                addItem.searchInfo = searchKey;
                addItem.recSetting = recSetKey;

                List<EpgAutoAddData> addList = new List<EpgAutoAddData>();
                addList.Add(addItem);

                if (cmd.SendChgEpgAutoAdd(addList) != 1)
                {
                    MessageBox.Show("変更に失敗しました");
                }
                else
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void listView_result_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = listView_result.SelectedItem as SearchItem;
                    if (item.IsReserved == true)
                    {
                        ChangeReserve(item.ReserveInfo);
                    }
                    else
                    {
                        AddReserve(item.EventInfo);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void ChangeReserve(ReserveData reserveInfo)
        {
            try
            {
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetReserveInfo(reserveInfo);
                if (dlg.ShowDialog() == true)
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void AddReserve(EpgEventInfo eventInfo)
        {
            try
            {
                AddReserveEpgWindow dlg = new AddReserveEpgWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetEventInfo(eventInfo);
                if (dlg.ShowDialog() == true)
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            ListSortDirection direction;

            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    string header = "Reserved";
                    if (headerClicked.Column.DisplayMemberBinding != null)
                    {
                        header = ((Binding)headerClicked.Column.DisplayMemberBinding).Path.Path;
                    }
                    if (String.Compare(header, _lastHeaderClicked) != 0)
                    {
                        direction = ListSortDirection.Ascending;
                        _lastHeaderClicked2 = _lastHeaderClicked;
                        _lastDirection2 = _lastDirection;
                    }
                    else
                    {
                        if (_lastDirection == ListSortDirection.Ascending)
                        {
                            direction = ListSortDirection.Descending;
                        }
                        else
                        {
                            direction = ListSortDirection.Ascending;
                        }
                    }

                    Sort(header, direction);

                    _lastHeaderClicked = header;
                    _lastDirection = direction;
                }
            }
        }

        private void Sort(string sortBy, ListSortDirection direction)
        {
            try
            {
                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_result.DataContext);

                dataView.SortDescriptions.Clear();

                SortDescription sd = new SortDescription(sortBy, direction);
                dataView.SortDescriptions.Add(sd);
                if (_lastHeaderClicked2 != null)
                {
                    if (String.Compare(sortBy, _lastHeaderClicked2) != 0)
                    {
                        SortDescription sd2 = new SortDescription(_lastHeaderClicked2, _lastDirection2);
                        dataView.SortDescriptions.Add(sd2);
                    }
                }
                dataView.Refresh();

                //Settings.Instance.ResColumnHead = sortBy;
                //Settings.Instance.ResSortDirection = direction;

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
    }
}
