# SSD_PLAYER
Ssplaer base on SSD UI

IDE:
	基于ZK UI播放器的IDE。
	编译方法：
	
	1.使用FlyThings IDE导入IDE工程；
	2.选中项目，点击右键选择清空项目，构建项目；
	3.customer_player.tar.gz和customer_without_player.tar.gz是程序的运行环境：
	  customer_player.tar.gz为开启用播放器功能的运行环境，会依赖于ffmpeg库；
	  customer_without_player.tar.gz为关闭播放器功能的运行环境，不依赖于ffmpeg库。
	  根据配置情况解压对应的包即可。
	  如果有修改UI资源相关部分，需要将构建项目后生成的ui文件夹替换到解压后的res目录中。
		
app:
	用来点屏的app，它会call到IDE中编译出来的so
	编译方法：
		1.根据需要点的panel修改sstardisp.c打开对应的配置：
			#define UI_1024_600 1
		2.make clean;make即可
		
customer_zk:
	拷贝到板子运行app的环境
	1.将IDE编译好的IDE\libs\armeabi\libzkgui.so替换到customer_zk\lib
	2.将需要的ffmpeg lib拷贝到customer_zk\lib
	
tool:
	用来改变触屏分辨率配置的文件：
	echo 1024x600.bin > /sys/bus/i2c/devices/1-005d/gtcfg
	echo 800x480.bin > /sys/bus/i2c/devices/1-005d/gtcfg

运行播放器：
	1.将app编译出来的zkgui copy到customer分区；
	2.将customer_zk的res和lib copy到customer分区；
	3.将customer_zk的etc下面的文件拷贝到板子/etc下面；
	4.cd /customer/lib,执行：export LD_LIBRARY_PATH=/lib:$PWD:$LD_LIBRARY_PATH
	5.运行zkgui: ./zkgui
	
	注：如果没有声音可能是功放的gpio没有拉高，可以参考如下步骤拉高：
	1. cd /sys/class/gpio/
	2. echo 12 > export
	3. echo out > direction
	4. echo 1 > value


