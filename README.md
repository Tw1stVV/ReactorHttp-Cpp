# HTTP服务器项目

### 项目概述

本项目是一个基于 C++ 实现的多线程 HTTP 服务器，采用 I/O 多路复用（epoll）与线程池结合的并发模型，支持处理 HTTP GET 请求并提供静态资源（文件和目录）访问服务。服务器具备高效的请求解析、响应构建和资源管理能力，可通过配置端口和工作目录提供服务。

### 项目结构

```
ReactorHttp-Cpp/
├── Buffer.*              # 缓冲区管理
├── Channel.*             # 文件描述符通道封装
├── Dispatcher.*          # 事件分发器基类
├── EpollDispatcher.*     # epoll实现的事件分发器
├── EventLoop.*           # 事件循环核心
├── HttpRequest.*         # HTTP请求解析
├── HttpResponse.*        # HTTP响应生成
├── Log.*                 # 日志系统
├── TcpConnection.*       # TCP连接管理
├── TcpServer.*           # TCP服务器实现
├── ThreadPool.*          # 线程池管理
└── WorkerThread.*        # 工作线程实现
```

### 核心组件

- **Buffer**
  
  - **作用：提供高效的内存缓冲区，支持数据读写、扩容及套接字 I/O 操作**
  - **功能：**
    - 动态内存管理：自动扩容机制
    - 读写位置追踪：维护`m_readPos`指针和`m_writePos`指针追踪读写位置
    - 网络I/O操作：
      - `socketRead()`：使用readv()从socket读取数据
      - `sendData`()：使用send()发送数据到socket
    - 数据处理
      - `findCRLF()`：查找"\r\n"换行符
      - `appendString()`：添加数据到缓冲区
    - 内存优化
      - `extendRoom()`：智能内存扩展(复用已读内存或扩容
  
- **Dispatcher**

  - **作用：事件分发器基类**
  - **功能：**
    - 定义事件分发接口
      - `add()`：添加监听事件
      - `modify()`：修改监听事件类型
      - `delete()`：删除监听事件
      - `dispatch()`：事件分发循环
    - 管理事件循环和通道
      - `setChannel()`：绑定Channel对象

- **EpollDispatcher**

  - **作用：基于epoll实现 I/O 多路复用机制，检测 fd 上的事件并通知处理**
  - **功能：**
    - epoll实例管理
      - 创建epoll实例：`m_epfd`
      - 管理epoll事件数组
    - 实现Dispatcher接口
      - `add()`：EPOLL_CTL_ADD
      - `remove()`：EPOLL_CTL_DEL
      - `modify()`：EPOLL_CTL_MOD
      - `dispatch()`：`epoll_wait()`等待事件响应
    - 事件分发
      - 事件响应将事件转发给`EventLoop`

- **EventLoop**

  - **作用：基于 epoll 的事件循环，负责 I/O 事件的检测与分发**
  - **功能：**
    - 事件循环执行
      - `run()`：启动事件循环
      - 调用Dispatcher的`dispatch()`方法
      - `active()`：事件响应处理
    - 任务队列管理
      - `addTask()`：添加`ADD`/`MODIFY`/`DELETE`任务
      - `processTask()`：处理任务队列
    - Channel管理
      - `m_channelMap`：保存每个fd对应的Channel
      - `add()`：将Channel添加到m_channelMap和EpollDispatcher的事件检测模型
      - `delete()`：将Channel从m_channelMap和EpollDispatcher的事件检测模型中删除
      - `modify()`：修改m_channelMap和EpollDispatcher事件检测模型中的Channel
    - 线程通信
      - `taskWakeup()`：通过socketpair唤醒阻塞线程
    - 线程管理
      - 维护线程ID和名称
      - 跨线程任务调度

- **Channel**

  - **作用：封装文件描述符和事件类型**
  - **功能：**
    - 封装文件描述符和实例类型
    - 事件回调管理
      - `readCallback()`：读事件响应回调
      - `writeCallback()`：写事件响应回调
      - `destroyCallback()`：销毁回调
    - 事件状态控制
      - `setWriteEventEnable()`：启用/禁用写事件
    - 属性接口
      - 获取保存的fd，事件状态，回调参数

- **ThreadPool**

  - **作用：管理线程池**
  - **功能：**
    - 工作线程管理
      - `run()`：创建WorkerThread
      - `m_workerThreads`：工作线程集合
    - 负载均衡
      - `takeWorkerThreadEventLoop()`：轮询分配工作线程
    - 主从模式
      - 主线程处理连接
      - 子线程处理通信

- **WorkerTherad**

  - **作用：工作线程实现**
  - **功能：**
    - 线程生命周期管理
      - `run()`：创建工作线程
      - `running()`：工作线程执行函数
    - 事件循环创建
      - 为每个线程创建独立的EventLoop
    - 线程同步
      - 使用`mutex`和`condition_variable`确保安全初始化

- **TcpServer**

  - **作用：管理监听套接字，接收客户端连接并分配工作线程**
  - **功能：**
    - 服务器初始化
      - `socketListen()`：创建监听套接字
    - 启动服务器
      - `run()`：启动线程池，注册监听fd的Channel，注册读事件，启动主线程EventLoop接受连接
    - 客户端连接
      - `acceptConnection()`：接受客户端连接，分配通信fd给工作线程
    - 客户端通信
      - 创建`TcpConnection`处理客户端通信

- **HttpRequest**

  - **作用：解析 HTTP 请求报文，处理静态资源请求**

  - **功能：**

    - 请求解析状态机

      - ```c++
        enum HttpRequestState {
            ParseReqLine,    // 解析请求行
            ParseReqHeaders, // 解析请求头
            ParseReqBody,    // 解析请求体
            ParseReqDone     // 解析完成
        };
        ```

    - 解析HTTP请求

      - `parseHttpRequest()`：解析入口，根据解析状态依次解析请求行，请求头

    - URL处理

      - `decodeMsg()`：URL解码(处理%xx格式的特殊字符)

    - 资源处理

      - `getFileType()`：根据扩展名获取MIME类型
      - `sendFile()`：发送文件内容
      - `sendDir()`：发送目录列表(HTML格式)
      - `processHttpRequest()`：处理请求

- **HttpResponse**

  - **作用：根据请求处理结果构建 HTTP 响应报文（状态行、响应头、响应体）**
  - **功能：**
    - `prepareMsg()`：构建响应报文
    - `sendDataFun`：关联函数(`sendDir()`/`sendFile()`)发送响应体

- **TcpConnection**

  - **作用：管理单个 TCP 连接的生命周期，协调缓冲区、HTTP 解析、事件处理的交互**
  - **功能：**
    - I/O处理
      - processRead()：接收HTTP请求
        - 读取套接字数据到读缓冲区
      - processWrite()：响应HTTP请求
        - 将组织的响应数据发送到客户端
    - HTTP协议处理
      - 关联HttpRequest和HttpResponse解析和响应Http请求
    - 资源管理
      - 包含读写缓冲区
      - destroy()：销毁连接，释放资源(缓冲区，Channel，Http对象等)

### 类间关系与协作流程

1. **启动流程**：
   `TcpServer`初始化监听套接字 → 启动`ThreadPool`创建工作线程（每个线程含`EventLoop`） → 主线程`EventLoop`启动，监听新连接。
2. **连接处理流程**：
   新连接到达 → `TcpServer`接受连接 → 线程池轮询分配`WorkerThread` → 创建`TcpConnection` → 绑定`Channel`到工作线程的`EventLoop`。
3. **请求处理流程**：
   客户端数据到达 → `Channel`触发读事件 → `TcpConnection::processRead`读取数据 → `HttpRequest`解析请求 → `HttpResponse`构建响应 → `TcpConnection::processWrite`发送响应。
4. **事件驱动流程**：
   `EventLoop`通过`EpollDispatcher`检测事件 → 触发`Channel`回调 → 调用`TcpConnection`的读写处理方法 → 完成后释放资源。