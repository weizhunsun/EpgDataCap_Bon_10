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

namespace EpgTimer.EpgView
{
    /// <summary>
    /// DateView.xaml の相互作用ロジック
    /// </summary>
    public partial class DateView : UserControl
    {
        public event RoutedEventHandler TimeButtonClick = null;

        public DateView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_time.Children.Clear();
            stackPanel_day.Children.Clear();
        }

        public void SetTime(System.Collections.SortedList timeList)
        {
            try
            {
                stackPanel_day.Children.Clear();
                stackPanel_time.Children.Clear();
                if (timeList.Count == 0)
                {
                    return;
                }

                TimePosInfo startPos = timeList.GetByIndex(0) as TimePosInfo;
                DateTime startTime = startPos.Time;
                TimePosInfo endPos = timeList.GetByIndex(timeList.Count - 1) as TimePosInfo;
                DateTime endTime = endPos.Time;
                DateTime itemTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, 0, 0, 0);
                while (itemTime < endTime)
                {
                    Button day = new Button();
                    if (Settings.Instance.NoStyle == 0)
                    {
                        day.Style = (Style)App.Current.Resources["ButtonStyle1"];
                    }
                    day.Width = 120;
                    day.Content = itemTime.ToString("M/d(ddd)");
                    if (itemTime.DayOfWeek == DayOfWeek.Saturday)
                    {
                        day.Foreground = Brushes.Blue;
                    }
                    else if (itemTime.DayOfWeek == DayOfWeek.Sunday)
                    {
                        day.Foreground = Brushes.Red;
                    }
                    day.DataContext = itemTime;
                    day.Click += new RoutedEventHandler(button_time_Click);

                    stackPanel_day.Children.Add(day);

                    Button hour6 = new Button();
                    if (Settings.Instance.NoStyle == 0)
                    {
                        hour6.Style = (Style)App.Current.Resources["ButtonStyle1"];
                    }
                    hour6.Width = 40;
                    hour6.Content = itemTime.ToString("6時");
                    hour6.DataContext = itemTime.AddHours(6);
                    hour6.Click += new RoutedEventHandler(button_time_Click);
                    stackPanel_time.Children.Add(hour6);

                    Button hour12 = new Button();
                    if (Settings.Instance.NoStyle == 0)
                    {
                        hour12.Style = (Style)App.Current.Resources["ButtonStyle1"];
                    }
                    hour12.Width = 40;
                    hour12.Content = itemTime.ToString("12時");
                    hour12.DataContext = itemTime.AddHours(12);
                    hour12.Click += new RoutedEventHandler(button_time_Click);
                    stackPanel_time.Children.Add(hour12);

                    Button hour18 = new Button();
                    if (Settings.Instance.NoStyle == 0)
                    {
                        hour18.Style = (Style)App.Current.Resources["ButtonStyle1"];
                    }
                    hour18.Width = 40;
                    hour18.Content = itemTime.ToString("18時");
                    hour18.DataContext = itemTime.AddHours(18);
                    hour18.Click += new RoutedEventHandler(button_time_Click);
                    stackPanel_time.Children.Add(hour18);

                    itemTime = itemTime.AddDays(1);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        void button_time_Click(object sender, RoutedEventArgs e)
        {
            if (TimeButtonClick != null)
            {
                TimeButtonClick(sender, e);
            }
        }
    }
}
