调试时遇到的问题：

1. 在调试h743的littefs时，开机后没有任何log， 后经过长时间debug发现是main线程栈溢出了
(特别要注意的问题， POST_KERNEL阶段的初始化如果出现异常，很可能无法打印异常log， 应该log系统还没有初始化)，因为littefs调用flash的操作太深了, 把main线程的栈空间改大就行了。
