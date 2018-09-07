using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace WindowsApplication1
{
    public partial class Form3 : Form
    {
        public Form3()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            

            string dd = Info.Pass;

            if (textBox1.Text == dd)
            {
                Application.Exit();
                //Process.GetCurrentProcess().Kill(); 
            }
            else
            {
                MessageBox.Show("密码错误，请检查后输入...");
            }
        }
    }
}