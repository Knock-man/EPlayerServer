#pragma once
#include "Socket.h"
#include "http_parser.h"
#include <map>

// 解析http类
class CHttpParser
{
private:
    http_parser m_parser;
    http_parser_settings m_settings; // 设置回调结构体

    // 存储HTTP头字段和对应的值
    std::map<Buffer, Buffer> m_HeaderValues;

    // 用于存储HTTP状态行
    Buffer m_status;

    // 用于存储HTTP请求或响应URL
    Buffer m_url;

    // 用于存储HTTP消息体
    Buffer m_body;

    // 表示HTTP消息是否已经完全接收和解析
    bool m_complete;

    // 最后一个被解析的HTTP头字段名
    Buffer m_lastField;

public:
    CHttpParser();
    ~CHttpParser();
    CHttpParser(const CHttpParser &http);
    CHttpParser &operator=(const CHttpParser &http);

public:
    size_t Parser(const Buffer &data);
    // GET POST ... 参考http_parser.h HTTP_METHOD_MAP宏
    unsigned Method() const { return m_parser.method; } // 方法
    const std::map<Buffer, Buffer> &Headers() { return m_HeaderValues; }
    const Buffer &Status() const { return m_status; }      // 状态
    const Buffer &Url() const { return m_url; }            // url
    const Buffer &Body() const { return m_body; }          // 数据
    unsigned Errno() const { return m_parser.http_errno; } // 错误码

protected:
    // 回调函数 调用相应成员函数
    static int OnMessageBegin(http_parser *parser);
    static int OnUrl(http_parser *parser, const char *at, size_t length);
    static int OnStatus(http_parser *parser, const char *at, size_t length);
    static int OnHeaderField(http_parser *parser, const char *at, size_t length);
    static int OnHeaderValue(http_parser *parser, const char *at, size_t length);
    static int OnHeadersComplete(http_parser *parser);
    static int OnBody(http_parser *parser, const char *at, size_t length);
    static int OnMessageComplete(http_parser *parser);

    // 成员函数缓存http各个值
    int OnMessageBegin();
    int OnUrl(const char *at, size_t length);
    int OnStatus(const char *at, size_t length);
    int OnHeaderField(const char *at, size_t length);
    int OnHeaderValue(const char *at, size_t length);
    int OnHeadersComplete();
    int OnBody(const char *at, size_t length);
    int OnMessageComplete();
};

// 解析url类
class UrlParser
{
public:
    UrlParser(const Buffer &url);
    ~UrlParser() {}
    int Parser();
    Buffer operator[](const Buffer &name) const;

    Buffer Protocol() const { return m_protocol; } // 协议
    Buffer Host() const { return m_host; }         // 域名
    // 默认返回80
    int Port() const { return m_port; } // 端口
    void SetUrl(const Buffer &url);     // uri
    const Buffer Uri() const { return m_uri; };

private:
    Buffer m_url;                      // url
    Buffer m_protocol;                 // 协议
    Buffer m_host;                     // 域名
    Buffer m_uri;                      // uri
    int m_port;                        // 端口
    std::map<Buffer, Buffer> m_values; // 存放uri key和value
};