sample_virvi2vo测试流程：
    该sample测试mpi_vi和mpi_vo组件的绑定组合。创建mpi_vi和mpi_vo，将它们绑定，再分别启动。mpi_vi采集图像，直接传输给mpi_vo显示。
    到达测试时间后，分别停止运行并销毁。也可以手动按ctrl-c，终止测试。

读取测试参数的流程：
    sample提供了sample_virvi2vo.conf，测试参数都写在该文件中。
    启动sample_virvi2vo时，在命令行参数中给出sample_virvi2vo.conf的具体路径，sample_virvi2vo会读取sample_virvi2vo.conf，完成参数解析。
    然后按照参数运行测试。
    从命令行启动sample_virvi2vo的指令：
    ./sample_virvi2vo -path /mnt/extsd/sample_virvi2vo.conf
    "-path /mnt/extsd/sample_virvi2vo.conf"指定了测试参数配置文件的路径。

测试参数的说明：
(1)capture_width：指定camera采集的图像宽度
(2)capture_height：指定camera采集的图像高度
(3)pic_format：指定camera采集的图像格式
(4)frame_rate：指定camera采集的帧率
(5)test_duration：指定测试时间
(6)dev_num：指定VIPP设备号
