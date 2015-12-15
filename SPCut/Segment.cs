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
using SegmentLibrary;

namespace SPCut
{
    class Segment
    {
        int a = 1;
        int b = 2;
        int test()
        {
            SegmentLibrary.Calculator.Add(1, 2);
            return 0;
        }
    }
}
