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

namespace EpgTimer
{
    /// <summary>
    /// EpgDateView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDateView : UserControl
    {
        public event RoutedEventHandler TimeButtonClick = null;
        public EpgDateView()
        {
            InitializeComponent();
        }

        public void SetTime(DateTime startTime, DateTime endTime)
        {
            stackPanel_day.Children.Clear();
            stackPanel_time.Children.Clear();
            DateTime itemTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, 0, 0, 0);
            while (itemTime < endTime)
            {
                Button day = new Button();
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
                hour6.Width = 40;
                hour6.Content = itemTime.ToString("6時");
                hour6.Foreground = Brushes.Black;
                hour6.DataContext = itemTime.AddHours(6);
                hour6.Click += new RoutedEventHandler(button_time_Click);
                stackPanel_time.Children.Add(hour6);

                Button hour12 = new Button();
                hour12.Width = 40;
                hour12.Content = itemTime.ToString("12時");
                hour12.Foreground = Brushes.Black;
                hour12.DataContext = itemTime.AddHours(12);
                hour12.Click += new RoutedEventHandler(button_time_Click);
                stackPanel_time.Children.Add(hour12);

                Button hour18 = new Button();
                hour18.Width = 40;
                hour18.Content = itemTime.ToString("18時");
                hour18.Foreground = Brushes.Black;
                hour18.DataContext = itemTime.AddHours(18);
                hour18.Click += new RoutedEventHandler(button_time_Click);
                stackPanel_time.Children.Add(hour18);

                itemTime = itemTime.AddDays(1);
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
