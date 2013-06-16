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
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// SearchKeyDescView.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchKeyDescView : UserControl
    {
        private EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
        private List<ServiceItem> serviceList = new List<ServiceItem>();
        private Dictionary<Int64, ServiceItem> serviceDict = new Dictionary<long, ServiceItem>();

        public SearchKeyDescView()
        {
            InitializeComponent();
            try
            {
                if (Settings.Instance.NoStyle == 1)
                {
                    button_content_add.Style = null;
                    button_content_del.Style = null;
                    button_all_on.Style = null;
                    button_video_on.Style = null;
                    button_bs_on.Style = null;
                    button_cs_on.Style = null;
                    button_tere_on.Style = null;
                    button_1seg_on.Style = null;
                    button_other_on.Style = null;
                    button_all_off.Style = null;
                    button_bs_on2.Style = null;
                    button_cs_on2.Style = null;
                    button_tere_on2.Style = null;
                    button_1seg_on2.Style = null;
                    button_other_on2.Style = null;
                    button_date_del.Style = null;
                    button_timeAdd.Style = null;
                    button_weekAdd.Style = null;
                }

                foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                {
                    ServiceItem item = new ServiceItem();

                    item.ServiceInfo = CommonManager.ConvertChSet5To(info);
                    serviceList.Add(item);
                    serviceDict.Add((Int64)item.ID, item);
                }
                listView_service.ItemsSource = serviceList;

                comboBox_content.DataContext = CommonManager.Instance.ContentKindDictionary.Values;
                comboBox_content.SelectedIndex = 0;

                comboBox_time_sw.DataContext = CommonManager.Instance.DayOfWeekDictionary.Values;
                comboBox_time_sw.SelectedIndex = 0;
                comboBox_time_sh.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_time_sh.SelectedIndex = 0;
                comboBox_time_sm.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_time_sm.SelectedIndex = 0;
                comboBox_time_ew.DataContext = CommonManager.Instance.DayOfWeekDictionary.Values;
                comboBox_time_ew.SelectedIndex = 6;
                comboBox_time_eh.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_time_eh.SelectedIndex = 23;
                comboBox_time_em.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_time_em.SelectedIndex = 59;
                comboBox_week_sh.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_week_sh.SelectedIndex = 0;
                comboBox_week_sm.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_week_sm.SelectedIndex = 0;
                comboBox_week_eh.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_week_eh.SelectedIndex = 23;
                comboBox_week_em.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_week_em.SelectedIndex = 59;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SetSearchKey(EpgSearchKeyInfo key)
        {
            defKey = key;
            UpdateView();
        }

        public void GetSearchKey(ref EpgSearchKeyInfo key)
        {
            try
            {
                if (checkBox_regExp.IsChecked == true)
                {
                    key.regExpFlag = 1;
                    key.aimaiFlag = 0;
                }
                else
                {
                    key.regExpFlag = 0;
                    if (checkBox_aimai.IsChecked == true)
                    {
                        key.aimaiFlag = 1;
                    }
                    else
                    {
                        key.aimaiFlag = 0;
                    }
                }
                if (checkBox_titleOnly.IsChecked == true)
                {
                    key.titleOnlyFlag = 1;
                }
                else
                {
                    key.titleOnlyFlag = 0;
                }

                key.contentList.Clear();
                foreach (ContentKindInfo info in listBox_content.Items)
                {
                    EpgContentData item = new EpgContentData();
                    item.content_nibble_level_1 = info.Nibble1;
                    item.content_nibble_level_2 = info.Nibble2;
                    key.contentList.Add(item);
                }
                if (checkBox_notContent.IsChecked == true)
                {
                    key.notContetFlag = 1;
                }
                else
                {
                    key.notContetFlag = 0;
                }

                key.serviceList.Clear();
                foreach (ServiceItem info in listView_service.Items)
                {
                    if (info.IsSelected == true)
                    {
                        key.serviceList.Add((Int64)info.ID);
                    }
                }

                key.dateList.Clear();
                foreach (DateItem info in listBox_date.Items)
                {
                    key.dateList.Add(info.DateInfo);
                }
                if (checkBox_notDate.IsChecked == true)
                {
                    key.notDateFlag = 1;
                }
                else
                {
                    key.notDateFlag = 0;
                }

                if (radioButton_free_2.IsChecked == true)
                {
                    //無料
                    key.freeCAFlag = 1;
                }
                else if (radioButton_free_3.IsChecked == true)
                {
                    //有料
                    key.freeCAFlag = 2;
                }
                else
                {
                    key.freeCAFlag = 0;
                }

                if (checkBox_chkRecEnd.IsChecked == true)
                {
                    key.chkRecEnd = 1;
                }
                else
                {
                    key.chkRecEnd = 0;
                }
                key.chkRecDay = Convert.ToUInt16(textBox_chkRecDay.Text.ToString());
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void UpdateView()
        {
            try
            {
                if (defKey.regExpFlag == 1)
                {
                    checkBox_regExp.IsChecked = true;
                    checkBox_aimai.IsChecked = false;
                }
                else
                {
                    checkBox_regExp.IsChecked = false;
                    if (defKey.aimaiFlag == 1)
                    {
                        checkBox_aimai.IsChecked = true;
                    }
                    else
                    {
                        checkBox_aimai.IsChecked = false;
                    }
                }
                if (defKey.titleOnlyFlag == 1)
                {
                    checkBox_titleOnly.IsChecked = true;
                }
                else
                {
                    checkBox_titleOnly.IsChecked = false;
                }

                listBox_content.Items.Clear();
                foreach (EpgContentData item in defKey.contentList)
                {
                    int nibble1 = item.content_nibble_level_1;
                    int nibble2 = item.content_nibble_level_2;
                    UInt16 contentKey = (UInt16)(nibble1 << 8 | nibble2);
                    if (CommonManager.Instance.ContentKindDictionary.ContainsKey(contentKey) == true)
                    {
                        listBox_content.Items.Add(CommonManager.Instance.ContentKindDictionary[contentKey]);
                    }
                }

                foreach (ServiceItem info in serviceDict.Values)
                {
                    info.IsSelected = false;
                }
                foreach (Int64 info in defKey.serviceList)
                {
                    if (serviceDict.ContainsKey(info) == true)
                    {
                        serviceDict[info].IsSelected = true;
                    }
                }

                listBox_date.Items.Clear();
                foreach (EpgSearchDateInfo info in defKey.dateList)
                {
                    DateItem item = new DateItem();

                    String viewText = "";

                    viewText = CommonManager.Instance.DayOfWeekDictionary[info.startDayOfWeek].DisplayName + " " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                        " ～ " + CommonManager.Instance.DayOfWeekDictionary[info.endDayOfWeek].DisplayName + " " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

                    item.DateInfo = info;
                    item.ViewText = viewText;

                    listBox_date.Items.Add(item);
                }

                if (defKey.notContetFlag == 1)
                {
                    checkBox_notContent.IsChecked = true;
                }
                else
                {
                    checkBox_notContent.IsChecked = false;
                }
                if (defKey.notDateFlag == 1)
                {
                    checkBox_notDate.IsChecked = true;
                }
                else
                {
                    checkBox_notDate.IsChecked = false;
                }

                if (defKey.freeCAFlag == 1)
                {
                    radioButton_free_2.IsChecked = true;
                }
                else if (defKey.freeCAFlag == 2)
                {
                    radioButton_free_3.IsChecked = true;
                }
                else
                {
                    radioButton_free_1.IsChecked = true;
                }

                if (defKey.chkRecEnd == 1)
                {
                    checkBox_chkRecEnd.IsChecked = true;
                }
                else
                {
                    checkBox_chkRecEnd.IsChecked = false;
                }
                textBox_chkRecDay.Text = defKey.chkRecDay.ToString();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_content_add_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_content.SelectedItem != null)
            {
                foreach (ContentKindInfo info in listBox_content.Items)
                {
                    ContentKindInfo select = comboBox_content.SelectedItem as ContentKindInfo;
                    if (select.Nibble1 == info.Nibble1 && select.Nibble2 == info.Nibble2)
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listBox_content.Items.Add(comboBox_content.SelectedItem);
            }
        }

        private void button_content_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_content.SelectedItem != null)
            {
                listBox_content.Items.RemoveAt(listBox_content.SelectedIndex);
            }
        }

        private void button_all_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                info.IsSelected = true;
            }
        }

        private void button_video_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (info.ServiceInfo.service_type == 0x01 || info.ServiceInfo.service_type == 0xA5)
                {
                    info.IsSelected = true;
                }
                else
                {
                    info.IsSelected = false;
                }
            }
        }

        private void button_bs_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (info.ServiceInfo.ONID == 0x04 &&
                    (info.ServiceInfo.service_type == 0x01 || info.ServiceInfo.service_type == 0xA5))
                {
                    info.IsSelected = true;
                }
                else
                {
                    info.IsSelected = false;
                }
            }
        }

        private void button_cs_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if ((info.ServiceInfo.ONID == 0x06 || info.ServiceInfo.ONID == 0x07) &&
                    (info.ServiceInfo.service_type == 0x01 || info.ServiceInfo.service_type == 0xA5))
                {
                    info.IsSelected = true;
                }
                else
                {
                    info.IsSelected = false;
                }
            }
        }

        private void button_tere_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if ((0x7880 <= info.ServiceInfo.ONID && info.ServiceInfo.ONID <= 0x7FE8) &&
                    (info.ServiceInfo.service_type == 0x01 || info.ServiceInfo.service_type == 0xA5))
                {
                    info.IsSelected = true;
                }
                else
                {
                    info.IsSelected = false;
                }
            }
        }

        private void button_1seg_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if ((0x7880 <= info.ServiceInfo.ONID && info.ServiceInfo.ONID <= 0x7FE8) &&
                    info.ServiceInfo.partialReceptionFlag == 1)
                {
                    info.IsSelected = true;
                }
                else
                {
                    info.IsSelected = false;
                }
            }
        }

        private void button_other_on_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (info.ServiceInfo.ONID != 0x04 &&
                    info.ServiceInfo.ONID != 0x06 &&
                    info.ServiceInfo.ONID != 0x07 &&
                    !(0x7880 <= info.ServiceInfo.ONID && info.ServiceInfo.ONID <= 0x7FE8)
                    )
                {
                    info.IsSelected = true;
                }
                else
                {
                    info.IsSelected = false;
                }
            }
        }

        private void button_all_off_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                info.IsSelected = false;
            }
        }

        private void button_timeAdd_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_time_sw.SelectedItem == null ||
                comboBox_time_sh.SelectedItem == null ||
                comboBox_time_sm.SelectedItem == null ||
                comboBox_time_ew.SelectedItem == null ||
                comboBox_time_eh.SelectedItem == null ||
                comboBox_time_em.SelectedItem == null)
            {
                return;
            }

            DateItem item = new DateItem();
            EpgSearchDateInfo info = new EpgSearchDateInfo();
            DayOfWeekInfo startWeek = comboBox_time_sw.SelectedItem as DayOfWeekInfo;
            DayOfWeekInfo endWeek = comboBox_time_ew.SelectedItem as DayOfWeekInfo;

            info.startDayOfWeek = startWeek.Value;
            info.startHour = (UInt16)comboBox_time_sh.SelectedItem;
            info.startMin = (UInt16)comboBox_time_sm.SelectedItem;
            info.endDayOfWeek = endWeek.Value;
            info.endHour = (UInt16)comboBox_time_eh.SelectedItem;
            info.endMin = (UInt16)comboBox_time_em.SelectedItem;

            String viewText = "";
            viewText = startWeek.DisplayName + " " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                " ～ " + endWeek.DisplayName + " " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

            item.DateInfo = info;
            item.ViewText = viewText;

            listBox_date.Items.Add(item);
        }

        private void button_weekAdd_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_week_sh.SelectedItem == null ||
                comboBox_week_sm.SelectedItem == null ||
                comboBox_week_eh.SelectedItem == null ||
                comboBox_week_em.SelectedItem == null)
            {
                return;
            }
            Int32 start = ((UInt16)comboBox_week_sh.SelectedItem) * 60;
            start += (UInt16)comboBox_week_sm.SelectedItem;
            Int32 end = ((UInt16)comboBox_week_eh.SelectedItem) * 60;
            end += (UInt16)comboBox_week_em.SelectedItem;
            if (end < start)
            {
                MessageBox.Show("開始時間が終了時間より前です");
                return;
            }

            if (checkBox_mon.IsChecked == true)
            {
                DateItem item = new DateItem();
                EpgSearchDateInfo info = new EpgSearchDateInfo();
                info.startDayOfWeek = 1;
                info.startHour = (UInt16)comboBox_week_sh.SelectedItem;
                info.startMin = (UInt16)comboBox_week_sm.SelectedItem;
                info.endDayOfWeek = 1;
                info.endHour = (UInt16)comboBox_week_eh.SelectedItem;
                info.endMin = (UInt16)comboBox_week_em.SelectedItem;

                String viewText = "";
                viewText = "月 " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                    " ～ " + "月 " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

                item.DateInfo = info;
                item.ViewText = viewText;
                listBox_date.Items.Add(item);
            }
            if (checkBox_tue.IsChecked == true)
            {
                DateItem item = new DateItem();
                EpgSearchDateInfo info = new EpgSearchDateInfo();
                info.startDayOfWeek = 2;
                info.startHour = (UInt16)comboBox_week_sh.SelectedItem;
                info.startMin = (UInt16)comboBox_week_sm.SelectedItem;
                info.endDayOfWeek = 2;
                info.endHour = (UInt16)comboBox_week_eh.SelectedItem;
                info.endMin = (UInt16)comboBox_week_em.SelectedItem;

                String viewText = "";
                viewText = "火 " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                    " ～ " + "火 " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

                item.DateInfo = info;
                item.ViewText = viewText;
                listBox_date.Items.Add(item);
            }
            if (checkBox_wen.IsChecked == true)
            {
                DateItem item = new DateItem();
                EpgSearchDateInfo info = new EpgSearchDateInfo();
                info.startDayOfWeek = 3;
                info.startHour = (UInt16)comboBox_week_sh.SelectedItem;
                info.startMin = (UInt16)comboBox_week_sm.SelectedItem;
                info.endDayOfWeek = 3;
                info.endHour = (UInt16)comboBox_week_eh.SelectedItem;
                info.endMin = (UInt16)comboBox_week_em.SelectedItem;

                String viewText = "";
                viewText = "水 " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                    " ～ " + "水 " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

                item.DateInfo = info;
                item.ViewText = viewText;
                listBox_date.Items.Add(item);
            }
            if (checkBox_thu.IsChecked == true)
            {
                DateItem item = new DateItem();
                EpgSearchDateInfo info = new EpgSearchDateInfo();
                info.startDayOfWeek = 4;
                info.startHour = (UInt16)comboBox_week_sh.SelectedItem;
                info.startMin = (UInt16)comboBox_week_sm.SelectedItem;
                info.endDayOfWeek = 4;
                info.endHour = (UInt16)comboBox_week_eh.SelectedItem;
                info.endMin = (UInt16)comboBox_week_em.SelectedItem;

                String viewText = "";
                viewText = "木 " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                    " ～ " + "木 " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

                item.DateInfo = info;
                item.ViewText = viewText;
                listBox_date.Items.Add(item);
            }
            if (checkBox_fri.IsChecked == true)
            {
                DateItem item = new DateItem();
                EpgSearchDateInfo info = new EpgSearchDateInfo();
                info.startDayOfWeek = 5;
                info.startHour = (UInt16)comboBox_week_sh.SelectedItem;
                info.startMin = (UInt16)comboBox_week_sm.SelectedItem;
                info.endDayOfWeek = 5;
                info.endHour = (UInt16)comboBox_week_eh.SelectedItem;
                info.endMin = (UInt16)comboBox_week_em.SelectedItem;

                String viewText = "";
                viewText = "金 " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                    " ～ " + "金 " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

                item.DateInfo = info;
                item.ViewText = viewText;
                listBox_date.Items.Add(item);
            }
            if (checkBox_sat.IsChecked == true)
            {
                DateItem item = new DateItem();
                EpgSearchDateInfo info = new EpgSearchDateInfo();
                info.startDayOfWeek = 6;
                info.startHour = (UInt16)comboBox_week_sh.SelectedItem;
                info.startMin = (UInt16)comboBox_week_sm.SelectedItem;
                info.endDayOfWeek = 6;
                info.endHour = (UInt16)comboBox_week_eh.SelectedItem;
                info.endMin = (UInt16)comboBox_week_em.SelectedItem;

                String viewText = "";
                viewText = "土 " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                    " ～ " + "土 " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

                item.DateInfo = info;
                item.ViewText = viewText;
                listBox_date.Items.Add(item);
            }
            if (checkBox_sun.IsChecked == true)
            {
                DateItem item = new DateItem();
                EpgSearchDateInfo info = new EpgSearchDateInfo();
                info.startDayOfWeek = 0;
                info.startHour = (UInt16)comboBox_week_sh.SelectedItem;
                info.startMin = (UInt16)comboBox_week_sm.SelectedItem;
                info.endDayOfWeek = 0;
                info.endHour = (UInt16)comboBox_week_eh.SelectedItem;
                info.endMin = (UInt16)comboBox_week_em.SelectedItem;

                String viewText = "";
                viewText = "日 " + info.startHour.ToString("00") + ":" + info.startMin.ToString("00") +
                    " ～ " + "日 " + info.endHour.ToString("00") + ":" + info.endMin.ToString("00");

                item.DateInfo = info;
                item.ViewText = viewText;
                listBox_date.Items.Add(item);
            }
        }

        private void button_date_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_date.SelectedItem != null)
            {
                listBox_date.Items.RemoveAt(listBox_date.SelectedIndex);
            }
        }

        private void button_bs_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (info.ServiceInfo.ONID == 0x04 &&
                    (info.ServiceInfo.service_type == 0x01 || info.ServiceInfo.service_type == 0xA5))
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_cs_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if ((info.ServiceInfo.ONID == 0x06 || info.ServiceInfo.ONID == 0x07) &&
                    (info.ServiceInfo.service_type == 0x01 || info.ServiceInfo.service_type == 0xA5))
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_tere_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if ((0x7880 <= info.ServiceInfo.ONID && info.ServiceInfo.ONID <= 0x7FE8) &&
                    (info.ServiceInfo.service_type == 0x01 || info.ServiceInfo.service_type == 0xA5))
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_1seg_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if ((0x7880 <= info.ServiceInfo.ONID && info.ServiceInfo.ONID <= 0x7FE8) &&
                    info.ServiceInfo.partialReceptionFlag == 1)
                {
                    info.IsSelected = true;
                }
            }
        }

        private void button_other_on2_Click(object sender, RoutedEventArgs e)
        {
            foreach (ServiceItem info in listView_service.Items)
            {
                if (info.ServiceInfo.ONID != 0x04 &&
                    info.ServiceInfo.ONID != 0x06 &&
                    info.ServiceInfo.ONID != 0x07 &&
                    !(0x7880 <= info.ServiceInfo.ONID && info.ServiceInfo.ONID <= 0x7FE8)
                    )
                {
                    info.IsSelected = true;
                }
            }
        }

    }
}
