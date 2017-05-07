开发环境：Xilinx Vivado HLS 2012.4

采用HLS C语言描述，可实现视频90度旋转，如将1280X720分辨率改为720X1280分辨率输出。
该模块数据接口采用AXI Stream接口，与其他系统部件无缝连接，RGB888像素数据下最高可支持到100MHz像素时钟。
该模块集成到EDK系统中在Spartan6 LX45上实测通过并稳定工作。

设计思路是使用Block RAM做多行的Local Interleaver，以维持较高的Burst Transfer Length，以提高内存访问效率。
