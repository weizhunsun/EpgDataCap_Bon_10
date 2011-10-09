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

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// SetDefSearchSettingWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetDefSearchSettingWindow : Window
    {
        public SetDefSearchSettingWindow()
        {
            InitializeComponent();

            if (Settings.Instance.NoStyle == 1)
            {
                button_cancel.Style = null;
                button_OK.Style = null;
            }
        }

        public void SetDefSetting(EpgSearchKeyInfo key)
        {
            searchKey.SetSearchKey(key);
        }

        public void GetSetting(ref EpgSearchKeyInfo key)
        {
            searchKey.GetSearchKey(ref key);
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}
