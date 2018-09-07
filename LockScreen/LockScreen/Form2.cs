using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace WindowsApplication1
{
    public partial class Form2 : Form
    {
        public Form2()
        {
            InitializeComponent();
        }
        //Info q = new Info();
        private void button1_Click(object sender, EventArgs e)
        {


            if (textBox1.Text == "")
            {
                Info.Name = "暂离中，勿动";

                if (textBox2.Text.Length < 4)
                {
                    MessageBox.Show("密码至少要四位数以上");
                }
                else
                {
                    if (textBox2.Text != textBox3.Text)
                    {
                        MessageBox.Show("两次输入密码不一致");
                    }
                    else
                    {

                        
                        Info.Pass = textBox2.Text;

                        Form3 f = new Form3();
                        f.Show();
                        this.Hide();
                        return;
                    }
                }

            }
            else
            {
                //5|1|a|s|p|x
                if (textBox2.Text.Length < 4)
                {
                    MessageBox.Show("密码至少要四位数以上");
                }
                else
                {
                    if (textBox2.Text != textBox3.Text)
                    {
                        MessageBox.Show("两次输入密码不一致");
                    }
                    else
                    {

                        Info.Name = textBox1.Text;
                        Info.Pass = textBox2.Text;

                        Form1 f = new Form1();
                        f.Show();
                        this.Hide();
                        return;
                    }
                }
            }
        }

        private void textBox1_MouseUp(object sender, MouseEventArgs e)
        {
            textBox1.Text = "";
        }

        private void textBox1_MouseDown(object sender, MouseEventArgs e)
        {
            textBox1.Text = "";
        }


    }
}