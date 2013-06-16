﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Drawing;
using System.ComponentModel;
using System.Windows;
using System.Windows.Forms;

namespace EpgTimer
{
    class TaskTrayClass : IDisposable
    {
        private NotifyIcon notifyIcon = new NotifyIcon();
        private Window targetWindow;

        public string Text {
            get { return notifyIcon.Text; }
            set
            {
                if (value.Length > 63)
                {
                    notifyIcon.Text = value.Substring(0,60) + "...";
                }
                else
                {
                    notifyIcon.Text = value;
                }
            }
        }
        public Icon Icon {
            get { return notifyIcon.Icon; }
            set { notifyIcon.Icon = value; }
        }
        public bool Visible{
            get { return notifyIcon.Visible; }
            set { notifyIcon.Visible = value; }
        }
        public WindowState LastViewState
        {
            get;
            set;
        }
        public event EventHandler ContextMenuClick = null;

        public TaskTrayClass(Window target)
        {
            Text = "";
            notifyIcon.BalloonTipIcon = ToolTipIcon.Info;
            notifyIcon.Click += NotifyIcon_Click;
            notifyIcon.BalloonTipClicked += NotifyIcon_Click;
            // 接続先ウィンドウ
            targetWindow = target;
            LastViewState = targetWindow.WindowState;
            // ウィンドウに接続
            if (targetWindow != null) {
                targetWindow.Closing += new System.ComponentModel.CancelEventHandler(target_Closing);
            }
            notifyIcon.ContextMenuStrip = new ContextMenuStrip();
        }

        public void SetContextMenu(List<Object> list)
        {
            if( list.Count == 0 )
            {
                notifyIcon.ContextMenuStrip = null;
            }else{
                ContextMenuStrip menu = new ContextMenuStrip();
                foreach(Object item in list)
                {
                    ToolStripMenuItem newcontitem = new ToolStripMenuItem();
                    if (item.ToString().Length > 0)
                    {
                        newcontitem.Tag = item;
                        newcontitem.Text = item.ToString();
                        newcontitem.Click +=new EventHandler(newcontitem_Click);
                        menu.Items.Add(newcontitem);
                    }
                    else
                    {
                        menu.Items.Add(new ToolStripSeparator());
                    }

                }
                notifyIcon.ContextMenuStrip = menu;
            }
        }

        public void ShowBalloonTip(String title, String tips, Int32 timeOutMSec)
        {
            try
            {
                if (title.Length > 0)
                {
                    notifyIcon.BalloonTipTitle = title;
                }
                else
                {
                    notifyIcon.BalloonTipTitle = " ";
                }
                if (tips.Length > 0)
                {
                    notifyIcon.BalloonTipText = tips;
                }
                else
                {
                    notifyIcon.BalloonTipText = " ";
                }
                notifyIcon.BalloonTipIcon = ToolTipIcon.Info;
                if (Settings.Instance.NoBallonTips == false)
                {
                    notifyIcon.ShowBalloonTip(timeOutMSec);
                }
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        void  newcontitem_Click(object sender, EventArgs e)
        {
            if (sender.GetType() == typeof(ToolStripMenuItem))
            {
                if (ContextMenuClick != null)
                {
                    ToolStripMenuItem item = sender as ToolStripMenuItem;
                    ContextMenuClick(item.Tag, e);
                }
            }
        }

        public void Dispose()
        {
            // ウィンドウから切断
            if (targetWindow != null)
            {
                targetWindow.Closing -= new System.ComponentModel.CancelEventHandler(target_Closing);
                targetWindow = null;
            }
        }

        private void target_Closing(object sender, CancelEventArgs e)
        {
            if (e.Cancel == false)
            {
                notifyIcon.Dispose();
                notifyIcon = null;
            }
        }

        private void NotifyIcon_Click(object sender, EventArgs e)
        {
            if (e.GetType() == typeof(MouseEventArgs))
            {
                MouseEventArgs mouseEvent = e as MouseEventArgs;
                if (mouseEvent.Button == MouseButtons.Left)
                {
                    //左クリック
                    if (targetWindow != null)
                    {
                        targetWindow.Show();
                        targetWindow.WindowState = LastViewState;
                        targetWindow.Activate();
                    }
                }
            }
        }   
    }
}
