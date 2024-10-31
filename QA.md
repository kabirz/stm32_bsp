调试时遇到的问题：

1. 在调试h743的littefs时，开机后没有任何log， 后经过长时间debug发现是main线程栈溢出了
(特别要注意的问题， POST_KERNEL阶段的初始化如果出现异常，很可能无法打印异常log， 应该log系统还没有初始化)，因为littefs调用flash的操作太深了, 把main线程的栈空间改大就行了。

2. 在使用udp组播通信时绑定固定ip时，发现Windows通信没有问题，Ubuntu抓包能接受到组播包，但是程序中recvfrom收不到包，后改为不绑定ip和端口发现可以了。

3. 在使用tcp时发现多client连接多开几个之后就连不上了。最后发现因为mcu资源有限默认支持的连接个数有限，需要修改参数指定连接的数量
  CONFIG_NET_MAX_CONN: 设置最大的连接数量，根据需要修改
  CONFIG_ZVFS_POLL_MAX: 如果使用了select这样的操作这个值需要注意，很多的连接可能会导致奔溃。

4. 使用ftp下载文件时发现网速奇慢无比，1M的文件下载需要1分多钟，后经过调整CONFIG_NET_BUF_DATA_SIZE为256后，5s就下载完成了，默认值时128. 调试了很久才解决。

操作方法
1. host设置固定ip
```shell
sudo ip a add 192.168.12.12/24 dev enp1s0
```

2. host设置组播路由到指定网卡，防止被过滤
linux:
```shell
sudo ip route add 224.0.0.1 dev enp1s0
```
windows(administrator):
```shell
route -p add 224.0.0.1 mask 255.255.255.255 192.168.12.12
```

3. 查看路由表(双网卡，其中一个连外网，另一个连开发板)
❯  ip route show
default via 192.168.10.1 dev eno1 proto dhcp src 192.168.10.12 metric 20101
192.168.10.0/24 dev eno1 proto kernel scope link src 192.168.10.12 metric 101
192.168.12.0/24 dev enp1s0 proto kernel scope link src 192.168.12.12
224.0.0.1 dev enp1s0 scope link



工具:

pymodbus:

```shell
# 安装
pip install "pymodbus[all]==3.7.2"
# 使用
pymodbus.console tcp --host 192.168.12.101 --port 502

```
读保持18个寄存器：client.read_holding_registers address=0 slave=1 count=18

