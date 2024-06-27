#pragma once
#include "CServer.h"
#include <map>
#include "Logger.h"
#include "HttpParser.h"
#include "MysqlClient.h"
#include "encrypt/Crypto.h"
#include "json.h"
using json = nlohmann::json;
// 错误处理
#define ERR_RETURN(ret, err)                                             \
    if (ret != 0)                                                        \
    {                                                                    \
        TRACEE("ret=%d errno=%d msg=[%s]", ret, errno, strerror(errno)); \
        return err;                                                      \
    }

#define WARN_CONTINUE(ret)                                               \
    if (ret != 0)                                                        \
    {                                                                    \
        TRACEW("ret=%d errno=%d msg=[%s]", ret, errno, strerror(errno)); \
        continue;                                                        \
    }

// 函数对象
template <typename _FUNCTION_, typename... _ARGS_>
class CConnectedFunction : public CFunctionBase // 函数对象类
{
public:
    CConnectedFunction(_FUNCTION_ func, _ARGS_... args)
        : m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) // 绑定函数
    {
    }
    virtual ~CConnectedFunction(){};
    virtual int operator()(CSocketBase *pClient) override // 括号运算符重载
    {
        return m_binder(pClient); // 返回绑定对象
    }

private:
    typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder; // 绑定器
};

template <typename _FUNCTION_, typename... _ARGS_>
class CReveiFunction : public CFunctionBase // 函数对象类
{
public:
    CReveiFunction(_FUNCTION_ func, _ARGS_... args)
        : m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...) // 绑定函数
    {
    }
    virtual ~CReveiFunction(){};
    virtual int operator()(CSocketBase *pClient, const Buffer &data) override // 括号运算符重载
    {
        return m_binder(pClient, data); // 返回绑定对象
    }

private:
    typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder; // 绑定器
};

// 业务类抽象类
class CBusiness
{
public:
    CBusiness()
        : m_connectdcallback(NULL), // 初始化回调
          m_recvcallback(NULL){};

    virtual int BusinessProcess(CProcess *proc) = 0; // 业务处理子进程

    template <typename _FUNCTION_, typename... _ARGS_>
    int setConnectCallback(_FUNCTION_ func, _ARGS_... args) // 注册连接回调函数
    {
        m_connectdcallback = new CConnectedFunction<_FUNCTION_, _ARGS_...>(func, args...);
        if (m_connectdcallback == NULL)
            return -1;
        return 0;
    }

    template <typename _FUNCTION_, typename... _ARGS_>
    int setRecvCallback(_FUNCTION_ func, _ARGS_... args) // 注册通信回调函数
    {
        m_recvcallback = new CReveiFunction<_FUNCTION_, _ARGS_...>(func, args...);
        if (m_recvcallback == NULL)
            return -1;
        return 0;
    }

protected:
    CFunctionBase *m_connectdcallback; // 连接回调
    CFunctionBase *m_recvcallback;     // 通信回调
};

// 业务处理类
class CEdoyunPlayerServer : public CBusiness
{
public:
    CEdoyunPlayerServer(unsigned count);

    ~CEdoyunPlayerServer();

    int BusinessProcess(CProcess *proc) override; // 业务处理子进程

private:
    int Connected(CSocketBase *pCLient); // 连接回调

    int Received(CSocketBase *pCLient, const Buffer &data); // 通信回调

    int HttpParser(const Buffer &data);

    Buffer MakeResponse(int ret);

private:
    int ThreadFunc(); // 线程函数

private:
    CEpoll m_epoll;
    ThreadPool m_pool;
    std::map<int, CSocketBase *> m_mapClients; // 客户端缓冲区
    unsigned m_count;
    CDatabaseClient *m_db;
};

// 表
DECLARE_TABLE_CLASS(edoyunLogin_user_mysql, _mysql_table_)
DECLARE_MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")              // QQ号
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_phone, DEFAULT, "VARCHAR", "(11)", "18888888888", "") // 手机
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_name, NOT_NULL, "TEXT", "", "", "")                      // 姓名
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_nick, NOT_NULL, "TEXT", "", "", "")                      // 昵称
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat_id, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_address, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_province, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_country, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_age, DEFAULT | CHECK, "INTEGER", "", "18", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_male, DEFAULT, "BOOL", "", "1", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_flags, DEFAULT, "TEXT", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_experience, DEFAULT, "REAL", "", "0.0", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_level, DEFAULT | CHECK, "INTEGER", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_class_priority, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_time_per_viewer, DEFAULT, "REAL", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_career, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_password, NOT_NULL, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_birthday, NONE, "DATETIME", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_describe, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_education, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_register_time, DEFAULT, "DATETIME", "", "CURRENT_TIMESTAMP", "")
DECLARE_TABLE_CLASS_EDN()

// ntype, name, attr, type, size, default_, check