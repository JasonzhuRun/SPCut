using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Forms;
using System.IO;

using SegmentLibrary;
namespace SPCut
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        private Canvas mCanvas;
        private Rectangle mRectangle;
        private Point mPointStart;
        private Point mPointEnd;
        private Point mPointLT;
        private Point mPointRB;
        private bool isMouseDown = false;

        private String fileName;

        public MainWindow()
        {
            InitializeComponent();
            mCanvas = this.canvas_main;
        }
        /// <summary>
        /// 打开图像文件
        /// </summary>
        private void onOpenBtnClick(object sender, RoutedEventArgs e)
        {
            if (mRectangle != null)
            {
                mCanvas.Children.Remove(mRectangle);
            }
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Title = "选择文件";
            openFileDialog.Filter = "JPEG 图像文件|*.jpg|位图文件|*.bmp|GIF 图像文件|*.gif|PNG 图像文件|*.png";
            openFileDialog.FileName = string.Empty;
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            openFileDialog.DefaultExt = "zip";
            DialogResult result = openFileDialog.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK)
            {
                fileName = openFileDialog.FileName;
                BitmapImage image = new BitmapImage(new Uri(fileName));
                canvas_main.Height = image.PixelHeight;
                canvas_main.Width = image.PixelWidth;
                canvas_main.Background = new ImageBrush(image);
            }
        }
        /// <summary>
        /// 根据坐标点画矩形框
        /// </summary>
        private void onDrawBtnClick(object sender, RoutedEventArgs e)
        {
            if (!string.IsNullOrEmpty(this.tb_L.Text)&&
                !string.IsNullOrEmpty(this.tb_T.Text)&&
                !string.IsNullOrEmpty(this.tb_R.Text)&&
                !string.IsNullOrEmpty(this.tb_B.Text))
            {
                if (mRectangle != null)
                {
                    mCanvas.Children.Remove(mRectangle);
                }

                mRectangle = new Rectangle();
                mRectangle.StrokeThickness = 3;
                mRectangle.Stroke = Brushes.Red;
                mCanvas.Children.Add(mRectangle);
                mPointLT = new Point(double.Parse(this.tb_L.Text), double.Parse(this.tb_T.Text));
                mPointRB = new Point(double.Parse(this.tb_R.Text), double.Parse(this.tb_B.Text));
                mRectangle.SetValue(Canvas.LeftProperty, mPointLT.X);
                mRectangle.SetValue(Canvas.TopProperty, mPointLT.Y);
                mRectangle.Width = Math.Abs(mPointRB.X - mPointLT.X);
                mRectangle.Height = Math.Abs(mPointRB.Y - mPointLT.Y);
            }
        }
        /// <summary>
        /// 分割
        /// </summary>
        private void onSegmentBtnClick(object sender, RoutedEventArgs e)
        {
            if (mRectangle != null)
            {
                mCanvas.Children.Remove(mRectangle);
            }
            SegmentLibrary.SPSegment.Segment(fileName,0,0,0,0);
        }

        private void onCanvasMouseDown(object sender, System.Windows.Input.MouseEventArgs e)
        {
            Point point = e.GetPosition(mCanvas);
            this.tb_L.Text = point.X.ToString();
            this.tb_T.Text = point.Y.ToString();
            mPointStart = point;
            isMouseDown = true;

            mPointLT = new Point(mPointStart.X, mPointStart.X);
            mPointRB = new Point(mPointStart.X, mPointStart.X);
            if(mRectangle != null)
            {
                mCanvas.Children.Remove(mRectangle);
            }
          
            mRectangle = new Rectangle();
            mRectangle.StrokeThickness = 3;
            mRectangle.Stroke = Brushes.Red;
            mCanvas.Children.Add(mRectangle);
        }

        private void onCanvasMouseMove(object sender, System.Windows.Input.MouseEventArgs e)
        {
            Point point = e.GetPosition(mCanvas);
            this.tb_Index.Text = point.X + ";" + point.Y;
            if (isMouseDown&&mPointStart!=null&&mRectangle!=null)
            {
                mRectangle.Width = Math.Abs(mPointStart.X - point.X);
                mRectangle.Height = Math.Abs(mPointStart.Y - point.Y);
                mPointLT.X = mPointStart.X < point.X ? mPointStart.X : point.X;
                mPointRB.X = mPointStart.X > point.X ? mPointStart.X : point.X;

                mPointLT.Y = mPointStart.Y < point.Y ? mPointStart.Y : point.Y;
                mPointRB.Y = mPointStart.Y > point.Y ? mPointStart.Y : point.Y;

                this.tb_L.Text = mPointLT.X.ToString();
                this.tb_T.Text = mPointLT.Y.ToString();

                this.tb_R.Text = mPointRB.X.ToString();
                this.tb_B.Text = mPointRB.Y.ToString();

                mRectangle.SetValue(Canvas.LeftProperty, mPointLT.X);
                mRectangle.SetValue(Canvas.TopProperty, mPointLT.Y);
            }
        }

        private void onCanvasMouseUp(object sender, System.Windows.Input.MouseEventArgs e)
        {
            Point point = e.GetPosition(mCanvas);
            this.tb_R.Text = point.X + "";
            this.tb_B.Text = point.Y + "";
            isMouseDown = false;
            mPointEnd = point;
        }
    }
}
