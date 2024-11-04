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

场景: linux主机A和B在同一个局域网内，C网络设备单独和A相连，问如何让主机B和C进行通信

假设：
1. 主机A连接局域网的网卡eth0 IP为: 192.168.1.13/24
2. 主机B连接局域网的网卡eth0 IP为: 192.168.1.10/24, 连接C的网卡eth1 IP为: 192.168.12.12/24
3. C设备的网卡IP为: 192.168.12.101/24

主机A无法直接访问到C设备，需要通过B中转，A发给C的包先发给B，需要设置路由
```shell
sudo ip route add 192.168.12.0/24 via 192.168.1.10
```
这样A发给C的包都会直接发给B

B主机收到包后需要将包转发给C，所以B需要开启ip转发功能

1. 临时开启，重启失效

```shell
sudo sysctl -w net.ipv4.ip_forward=1
```
2. 长期有效, 添加下面一行到`/etc/sysctl.conf`

```shell
net.ipv4.ip_forward = 1
```
并且执行
```shell
sudo sysctl -p /etc/sysctl.conf
```

但是这样C仍然收不到A发过来的包，还需要使用iptables配置ip地址转换

```shell
sudo iptables -t nat -A POSTROUTING -o eth1 -j MASQUERADE
```
各部分解释
`iptables`： iptables是Linux系统中用于配置和管理IP包过滤规则和NAT规则的工具。

`-t nat`： 这表示我们要操作的是NAT表。`iptables`有几个不同的表，每个表用于不同的用途：

`filter`表：用于处理包过滤（防火墙规则）。
`nat`表：用于处理地址转换（NAT）。
`mangle`表：用于修改包头。
`raw`表：用于配置不被连接追踪处理的规则。
`-A POSTROUTING`： `-A`表示添加（append）一条规则到指定链。`POSTROUTING`链是NAT表中的一个链，表示在包即将离开路由器时处理。POSTROUTING链中的规则会在包路由决定完成后应用。

`-o eth1`： 这里的-o表示输出接口（output interface），即这条规则仅适用于从指定接口（在这里是eth1）出去的流量。

`-j MASQUERADE`： `-j`表示跳转（jump）到指定的目标链或动作。MASQUERADE是NAT的一种形式，用于动态地替换源IP地址和端口号。它通常用于动态分配IP地址的连接（例如通过DHCP获得的IP地址），因为它不需要指定具体的IP地址。
